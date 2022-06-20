#ifndef AGILE_WORKFLOW1_GNN_H
#define AGILE_WORKFLOW1_GNN_H

#include <cstdint>
#include <memory>
#include <vector>

#include "agile/workflow1/globalIDS.h"
#include "agile/workflow1/main.h"
#include "agile/workflow1/wmd.h"

#include "shad/data_structures/array.h"
#include "torch/script.h"
#include "torch/torch.h"

#include "mpi.h" // TODO: removed dependency

namespace agile::workflow1 {

template <typename Dataset> struct TrainingState {
public:
  using sampler_type = torch::data::samplers::DistributedRandomSampler;
  using dataset_type = torch::data::datasets::MapDataset<
      Dataset, torch::data::transforms::Stack<typename Dataset::Data>>;
  using data_loader_type =
      torch::data::StatelessDataLoader<dataset_type, sampler_type>;

  TrainingState() = default;
  TrainingState(const TrainingState &) = default;
  TrainingState(TrainingState &&) = default;

  TrainingState &operator=(const TrainingState &) = default;
  TrainingState &operator=(TrainingState &&) = default;

  torch::jit::script::Module Module;
  Dataset DataSet;
  std::shared_ptr<data_loader_type> TrainDataLoader{nullptr};
  std::shared_ptr<data_loader_type> TestDataLoader{nullptr};
  std::shared_ptr<torch::optim::Adam> Adam{nullptr};
  std::vector<torch::jit::IValue> Inputs;
};

template <typename Dataset> class SetUpTrainingContext {
  using ArrayOID = typename shad::Array<uint64_t>::ObjectID;

  const int64_t trainingSetSize = 500;
  const int64_t testSetSize = 500;
  const size_t batchSize = 100;

  char modelFileName_[256];
  VertexOID _verticesOID;
  EdgeOID _edgesOID;
  ArrayOID _featuresOID;

public:
  SetUpTrainingContext(const VertexOID &VertexArrayID,
                       const EdgeOID &EdgeArrayOID,
                       const ArrayOID &FeaturesArrayID,
                       std::string modelFileName)
      : _verticesOID(VertexArrayID), _edgesOID(EdgeArrayOID),
        _featuresOID(FeaturesArrayID) {
    if (modelFileName.size() > 256)
      throw "Filename too long";

    std::strcpy(modelFileName_, modelFileName.c_str());
  }

  void operator()(TrainingState<Dataset> &TS) {
    // Load Module
    TS.Module = torch::jit::load(modelFileName_);
    // Load Dataset
    TS.DataSet = Dataset(_verticesOID, _edgesOID, _featuresOID);

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
    auto stackedDataSet = TS.DataSet.map(
        torch::data::transforms::Stack<typename Dataset::Data>());
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
};

template <typename TrainingState> void vcTrainLoop(TrainingState &TS) {
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
    total_loss += loss.template item<double>();

    loss.backward();
    TS.Module.eval();
    auto prediction = std::get<1>(output.max(1));
    auto equal = prediction.eq(groundTruth);
    train_correct += equal.index({batch.Mask}).sum().template item<int64_t>();
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
    test_correct += equal.index({batch.Mask}).sum().template item<int64_t>();
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

typename shad::Array<agile::workflow1::TrainingState<WMDDataset>>::ObjectID
GNN(uint64_t &num_edges, uint64_t &num_vertices, Graph_t &graph, std::string modelFileName);

} // namespace agile::workflow1

#endif
