#include <chrono>
#include <iostream>
#include <iomanip>

#include "agile/workflow1/cora.h"

#include <torch/torch.h>
#include <torch/script.h>

#include <shad/core/vector.h>
#include <shad/data_structures/array.h>
#include <shad/core/algorithm.h>

#include "mpi.h"

torch::Tensor reload(const std::string &path) {
  std::ifstream file(path, std::ios::binary);
  std::vector<char> buffer(std::istreambuf_iterator<char>(file), {});
  return torch::pickle_load(buffer).toTensor();
}

void getArg(int argc, char* argv[], int i, char *out) {
  size_t len = std::strlen(argv[i]);
  std::memcpy(out, argv[i], len);
  out[len] = '\0';
}

struct TrainingState {
public:
  using sampler_type = torch::data::samplers::DistributedSequentialSampler;
  using dataset_type = torch::data::datasets::MapDataset<agile::CoraDataset, torch::data::transforms::Stack<>>;
  using data_loader_type = torch::data::StatelessDataLoader<dataset_type, sampler_type>;

  TrainingState() = default;
  TrainingState(const TrainingState &) = default;
  TrainingState(TrainingState &&) = default;

  TrainingState & operator=(const TrainingState &) = default;
  TrainingState & operator=(TrainingState &&) = default;

  torch::jit::script::Module Module;
  agile::CoraDataset DataSet;
  at::Tensor TrainingMask;
  at::Tensor TestMask;
  std::shared_ptr<data_loader_type> DataLoader{nullptr};
  std::shared_ptr<torch::optim::Adam> Adam{nullptr};
  std::vector<torch::jit::IValue> Inputs;
};

class SetUpFunctor {
public:
  SetUpFunctor(int argc, char * argv[]) {
    getArg(argc, argv, 1, modelFileName_);
    getArg(argc, argv, 2, edgeIndexFileName_);
    getArg(argc, argv, 3, featureVectorsFileName_);
    getArg(argc, argv, 4, labelFileName_);
  }

  void operator()(TrainingState &TS) {
    // Load Module
    TS.Module = torch::jit::load(modelFileName_);

    // Load Dataset
    TS.DataSet = agile::CoraDataset(edgeIndexFileName_, featureVectorsFileName_, labelFileName_);
    size_t numVertices = TS.DataSet.size().value();

    // Partition in Training/Test Set
    using namespace torch::indexing;
    auto options = torch::TensorOptions().dtype(torch::kBool);
    TS.TrainingMask = torch::zeros({numVertices}, options);
    TS.TestMask = torch::zeros({numVertices}, options);

    int64_t trainingSetSize = 500;
    int64_t testSetSize = 500;
    auto indices = torch::randint(0, numVertices, {trainingSetSize + testSetSize}, torch::TensorOptions().dtype(torch::kLong));
    TS.TrainingMask.index_put_({indices.index({Slice(None, trainingSetSize)})}, true);
    TS.TestMask.index_put_({indices.index({Slice(trainingSetSize, None)})}, true);

    trainingSetSize = TS.TrainingMask.sum().item<int64_t>();
    testSetSize = TS.TestMask.sum().item<int64_t>();

    std::cout << shad::rt::thisLocality()
              << ">Training set size : " << trainingSetSize
              << ", Test set size : " << testSetSize
              << std::endl;

    // Create DataLoader
    size_t batchSize = numVertices / shad::rt::numLocalities();
    uint32_t thisLocality = static_cast<uint32_t>(shad::rt::thisLocality());
    auto data_sampler= torch::data::samplers::DistributedSequentialSampler(numVertices, shad::rt::numLocalities(), thisLocality, false);
    auto stackedDataSet = TS.DataSet.map(torch::data::transforms::Stack<>());
    TS.DataLoader = torch::data::make_data_loader(std::move(stackedDataSet), data_sampler, batchSize);

    // Create Optimizer
    std::vector<at::Tensor> parameters;
    for (const auto & params : TS.Module.parameters()) {
      parameters.push_back(params);
    }

    const double learningRate = 0.01;
    const int numEpochs = 200;
    TS.Adam = std::make_unique<torch::optim::Adam>(parameters, torch::optim::AdamOptions(learningRate).weight_decay(5e-4));

    // Set inputs
    TS.Inputs.resize(2);
    TS.Inputs[1] = TS.DataSet.edge_index();
  }

  const char * modelFileName() const { return modelFileName_; }
  const char * edgeIndexFileName() const { return edgeIndexFileName_; }
  const char * featureVectorsFileName() const { return featureVectorsFileName_; }
  const char * labelFileName() const { return labelFileName_; }
private:
  char modelFileName_[256];
  char edgeIndexFileName_[256];
  char featureVectorsFileName_[256];
  char labelFileName_[256];
};

void trainLoop(TrainingState & TS) {
  auto start = std::chrono::high_resolution_clock::now();
  size_t train_correct = 0;
  size_t test_correct = 0;
  double total_loss = 0.0;
  size_t trainingSetSize = TS.TrainingMask.sum().item<int64_t>();
  size_t testSetSize = TS.TestMask.sum().item<int64_t>();
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

  for (auto & batch : *TS.DataLoader) {
    TS.Inputs[0] = batch.data;
    auto groundTruth = batch.target;

    std::cout << batch.data.sizes() << std::endl;

    TS.Module.train();
    auto output = TS.Module.forward(TS.Inputs).toTensor();

    auto loss = torch::nn::functional::nll_loss(output.index({TS.TrainingMask}), groundTruth.index({TS.TrainingMask}));
    total_loss += loss.item<double>();

    TS.Adam->zero_grad();
    loss.backward();

    for (const auto &param : TS.Module.named_parameters()) {
        MPI_Allreduce(MPI_IN_PLACE, param.value.mutable_grad().data_ptr(),
                      param.value.mutable_grad().numel(),
                      torchToMPITypes.at(param.value.mutable_grad().scalar_type()),
                      MPI_SUM, MPI_COMM_WORLD);

        param.value.mutable_grad().data() = param.value.grad().data()/numRanks;
    }

    TS.Adam->step();

    TS.Module.eval();
    auto prediction = std::get<1>(output.max(1));
    auto equal = prediction.eq(groundTruth);
    train_correct += equal.index({TS.TrainingMask}).sum().item<int64_t>();
    test_correct += equal.index({TS.TestMask}).sum().item<int64_t>();
  }
  auto end = std::chrono::high_resolution_clock::now();
#if 0
  std::cout
    << shad::rt::thisLocality()
    << " Train Accuracy: "
    << static_cast<float>(train_correct) / trainingSetSize
    << ", Test Accuracy: " << static_cast<float>(test_correct) / testSetSize
    << " | Loss: " << total_loss
    << " | Time (s) : " << std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count()
    << std::endl;
#endif
}


namespace shad {
int main(int argc, char *argv[]) {
  size_t parallelThreads= shad::rt::numLocalities() * shad::rt::impl::getConcurrency();
  TrainingState initState;
  auto TSs = shad::Array<TrainingState>::Create(parallelThreads, initState);

  SetUpFunctor setUp(argc, argv);

  std::cout << "Loading module" << std::endl;
  shad::for_each(shad::distributed_parallel_tag{}, TSs->begin(), TSs->end(), setUp);


  const size_t numEpochs = 200;
  for (size_t epoch = 0; epoch < numEpochs; ++epoch) {
    shad::for_each(shad::distributed_parallel_tag{}, TSs->begin(), TSs->end(), trainLoop);
  }

  return EXIT_SUCCESS;
}
}
