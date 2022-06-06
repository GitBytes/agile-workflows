#include <chrono>
#include <iomanip>
#include <iostream>

#include "agile/workflow1/cora.h"
#include "agile/workflow1/gnn.h"

#include <torch/script.h>
#include <torch/torch.h>

#include <shad/core/algorithm.h>
#include <shad/core/vector.h>
#include <shad/data_structures/array.h>

#include "mpi.h"

torch::Tensor reload(const std::string &path) {
  std::ifstream file(path, std::ios::binary);
  std::vector<char> buffer(std::istreambuf_iterator<char>(file), {});
  return torch::pickle_load(buffer).toTensor();
}

void getArg(int argc, char *argv[], int i, char *out) {
  size_t len = std::strlen(argv[i]);
  std::memcpy(out, argv[i], len);
  out[len] = '\0';
}

const int64_t trainingSetSize = 500;
const int64_t testSetSize = 500;
const size_t batchSize = 100;

class SetUpFunctor {
public:
  SetUpFunctor(int argc, char *argv[]) {
    getArg(argc, argv, 1, modelFileName_);
    getArg(argc, argv, 2, edgeIndexFileName_);
    getArg(argc, argv, 3, featureVectorsFileName_);
    getArg(argc, argv, 4, labelFileName_);
  }

  void operator()(agile::workflow1::TrainingState &TS) {
    // Load Module
    TS.Module = torch::jit::load(modelFileName_);

    // Load Dataset
    auto levels = torch::tensor({5, 3, 2, 1});
    TS.DataSet = agile::CoraDataset(edgeIndexFileName_, featureVectorsFileName_,
                                    labelFileName_, levels);
    size_t numVertices = TS.DataSet.size().value();

    // Partition in Training/Test Set
    using namespace torch::indexing;
    auto options = torch::TensorOptions().dtype(torch::kBool);

    // Create DataLoader
    uint32_t thisLocality = static_cast<uint32_t>(shad::rt::thisLocality());
    auto train_sampler = torch::data::samplers::DistributedRandomSampler(
        trainingSetSize, shad::rt::numLocalities(), thisLocality, false);
    auto test_sampler = torch::data::samplers::DistributedRandomSampler(
        testSetSize, shad::rt::numLocalities(), thisLocality, false);
    auto stackedDataSet =
        TS.DataSet.map(torch::data::transforms::Stack<agile::CoraData<>>());
    TS.TrainDataLoader =
        torch::data::make_data_loader(stackedDataSet, train_sampler, batchSize);
    TS.TestDataLoader =
        torch::data::make_data_loader(stackedDataSet, test_sampler, batchSize);

    // Create Optimizer
    std::vector<at::Tensor> parameters;
    for (const auto &params : TS.Module.parameters()) {
      parameters.push_back(params);
    }

    const double learningRate = 0.01;
    const int numEpochs = 200;
    TS.Adam = std::make_unique<torch::optim::Adam>(
        parameters, torch::optim::AdamOptions(learningRate).weight_decay(5e-4));

    // Set inputs
    TS.Inputs.resize(2);
  }

  const char *modelFileName() const { return modelFileName_; }
  const char *edgeIndexFileName() const { return edgeIndexFileName_; }
  const char *featureVectorsFileName() const { return featureVectorsFileName_; }
  const char *labelFileName() const { return labelFileName_; }

private:
  char modelFileName_[256];
  char edgeIndexFileName_[256];
  char featureVectorsFileName_[256];
  char labelFileName_[256];
};


void trainLoop(agile::workflow1::TrainingState &TS) {
  auto start = std::chrono::high_resolution_clock::now();
  size_t train_correct = 0;
  size_t test_correct = 0;
  size_t train_size = 0;
  size_t test_size = 0;
  double total_loss = 0.0;
  int64_t numRanks = shad::rt::numLocalities();

  std::map<at::ScalarType, MPI_Datatype> torchToMPITypes = {
      {at::kByte, MPI_UNSIGNED_CHAR},
      {at::kChar, MPI_CHAR},
      {at::kDouble, MPI_DOUBLE},
      {at::kFloat, MPI_FLOAT},
      {at::kInt, MPI_INT},
      {at::kLong, MPI_LONG},
      {at::kShort, MPI_SHORT},
  };

  for (auto &batch : *TS.TrainDataLoader) {
    TS.Inputs[0] = batch.Features;
    TS.Inputs[1] = batch.EdgeIndex;
    train_size += batch.Features.size(0);
    auto groundTruth = batch.Labels;

    TS.Module.train();
    auto output = TS.Module.forward(TS.Inputs).toTensor();

    auto loss = torch::nn::functional::nll_loss(
        output.index({batch.Mask}), groundTruth.index({batch.Mask}));
    total_loss += loss.item<double>();

    loss.backward();
    TS.Module.eval();
    auto prediction = std::get<1>(output.max(1));
    auto equal = prediction.eq(groundTruth);
    train_correct += equal.index({batch.Mask}).sum().item<int64_t>();
  }

  for (const auto &param : TS.Module.named_parameters()) {
    MPI_Allreduce(MPI_IN_PLACE, param.value.mutable_grad().data_ptr(),
                  param.value.mutable_grad().numel(),
                  torchToMPITypes.at(param.value.mutable_grad().scalar_type()),
                  MPI_SUM, MPI_COMM_WORLD);
    param.value.mutable_grad().data() =
        param.value.mutable_grad().data() / numRanks;
  }

  TS.Adam->step();
  TS.Adam->zero_grad();

  for (auto &batch : *TS.TestDataLoader) {
    test_size += batch.Features.size(0);
    TS.Module.eval();
    TS.Inputs[0] = batch.Features;
    TS.Inputs[1] = batch.EdgeIndex;

    auto groundTruth = batch.Labels;
    auto output = TS.Module.forward(TS.Inputs).toTensor();
    auto prediction = std::get<1>(output.max(1));
    auto equal = prediction.eq(groundTruth);
    test_correct += equal.index({batch.Mask}).sum().item<int64_t>();
  }
  auto end = std::chrono::high_resolution_clock::now();

  std::cout << shad::rt::thisLocality() << " Train Accuracy: "
            << static_cast<float>(train_correct) / train_size
            << ", Test Accuracy: "
            << static_cast<float>(test_correct) / test_size
            << " | Loss: " << total_loss << " | Time (s) : "
            << std::chrono::duration_cast<std::chrono::duration<double>>(end -
                                                                         start)
                   .count()
            << std::endl;
}

namespace shad {
int main(int argc, char *argv[]) {
  size_t parallelThreads = shad::rt::numLocalities();
  agile::workflow1::TrainingState initState;
  auto TSs = shad::Array<agile::workflow1::TrainingState>::Create(parallelThreads, initState);

  SetUpFunctor setUp(argc, argv);

  std::cout << "Loading module" << std::endl;
  shad::for_each(shad::distributed_parallel_tag{}, TSs->begin(), TSs->end(),
                 setUp);

  std::cout << "Setup done" << std::endl;

  const size_t numEpochs = 200;
  for (size_t epoch = 0; epoch < numEpochs; ++epoch) {
    shad::for_each(shad::distributed_parallel_tag{}, TSs->begin(), TSs->end(),
                   trainLoop);
  }

  return EXIT_SUCCESS;
}
} // namespace shad
