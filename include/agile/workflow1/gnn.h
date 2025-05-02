/*===------------------------------------------------------------*- C++ -*-===
 *
 *                            The AGILE Workflows
 *
 *===----------------------------------------------------------------------===
 *
 * Copyright (c) 2025 Battelle Memorial Institute
 *
 * Battelle Memorial Institute (hereinafter Battelle) hereby grants permission
 * to any person or entity lawfully obtaining a copy of this software and
 * associated documentation files (hereinafter “the Software”) to redistribute
 * and use the Software in source and binary forms, with or without
 * modification. Such person or entity may use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and may permit
 * others to do so, subject to the following conditions:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimers.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Other than as used herein, neither the name Battelle Memorial Institute or
 *    Battelle may be used in any form whatsoever without the express written
 *    consent of Battelle.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL BATTELLE OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *===----------------------------------------------------------------------===*/
#ifndef AGILE_WORKFLOW1_GNN_H
#define AGILE_WORKFLOW1_GNN_H

#include <cstdint>
#include <memory>
#include <vector>

#include "agile/workflow1/graph.h"
#include "agile/workflow1/main.h"
#include "agile/workflow1/wmd.h"

#include "shad/core/algorithm.h"
#include "shad/data_structures/array.h"
#include "shad/extensions/collectives/mpi_reduce.h"
#include "torch/script.h"
#include "torch/torch.h"

namespace agile::workflow1 {

template <typename Dataset> struct TrainingState {
public:
  using ArrayOID = typename shad::Array<uint64_t>::ObjectID;
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
  ArrayOID ReducerLocalPtrOID{ArrayOID::kNullID};
  ArrayOID LocalSamplesProcessedOID{ArrayOID::kNullID};
  ArrayOID LocalSamplesCorrectOID{ArrayOID::kNullID};
  size_t TID{0};
};

template <typename Dataset> class SetUpTrainingContext {
  using ArrayOID = typename shad::Array<uint64_t>::ObjectID;

  char modelFileName_[256];
  VertexOID _verticesOID;
  XEdgeOID _edgesOID;
  ArrayOID _featuresOID;
  ArrayOID _reducerArrayOID;
  ArrayOID _localSamplesProcessedOID;
  ArrayOID _localSamplesCorrectOID;

public:
  SetUpTrainingContext(const VertexOID &VertexArrayID,
                       const XEdgeOID &EdgeArrayOID,
                       const ArrayOID &FeaturesArrayID,
                       const ArrayOID &ReducerArrayOID,
                       const ArrayOID &LocalSamplesProcessedOID,
                       const ArrayOID &LocalSamplesCorrectOID,
                       std::string modelFileName)
      : _verticesOID(VertexArrayID), _edgesOID(EdgeArrayOID),
        _featuresOID(FeaturesArrayID), _reducerArrayOID(ReducerArrayOID),
        _localSamplesProcessedOID(LocalSamplesProcessedOID),
        _localSamplesCorrectOID(LocalSamplesCorrectOID) {
    if (modelFileName.size() > 256)
      throw "Filename too long";

    std::strcpy(modelFileName_, modelFileName.c_str());
  }

  void operator()(size_t tid, TrainingState<Dataset> &TS) {
    // TID
    TS.TID = tid;
    // Load Module
    TS.Module = torch::jit::load(modelFileName_);
    // Load Dataset
    TS.DataSet = Dataset(_verticesOID, _edgesOID, _featuresOID);

    // Number threads
    size_t total_ranks =
        shad::rt::numLocalities() * shad::rt::impl::getConcurrency();
    const int64_t trainingSetSize = TS.DataSet.size().value() / int64_t(4);
    const int64_t testSetSize = trainingSetSize / 2;
    const size_t batchSize = std::min<size_t>(128, trainingSetSize / total_ranks);

    size_t numVertices = TS.DataSet.size().value();
    // Partition in Training/Test Set
    using namespace torch::indexing;
    auto options = torch::TensorOptions().dtype(torch::kBool);

    // Create DataLoader
    uint32_t thisLocality = static_cast<uint32_t>(shad::rt::thisLocality());
    auto train_sampler = torch::data::samplers::DistributedRandomSampler(
        trainingSetSize, total_ranks, tid, false);
    train_sampler.reset();
    auto test_sampler = torch::data::samplers::DistributedRandomSampler(
        testSetSize, total_ranks, tid, false);
    auto stackedDataSet = TS.DataSet.map(
        torch::data::transforms::Stack<typename Dataset::Data>());

    torch::data::DataLoaderOptions DLOptions(batchSize);

    TS.TrainDataLoader =
        torch::data::make_data_loader(stackedDataSet, train_sampler, DLOptions);
    TS.TestDataLoader =
        torch::data::make_data_loader(stackedDataSet, test_sampler, DLOptions);

    // Create Optimizer
    std::vector<at::Tensor> parameters;
    for (const auto &params : TS.Module.parameters()) {
      if (params.requires_grad()) {
        parameters.push_back(params);
      }
    }

    const double learningRate = 0.01;
    TS.Adam = std::make_unique<torch::optim::Adam>(
        parameters, torch::optim::AdamOptions(learningRate).weight_decay(5e-4));

    // Set inputs
    TS.Inputs.resize(2);
    TS.ReducerLocalPtrOID = _reducerArrayOID;
    TS.LocalSamplesProcessedOID = _localSamplesProcessedOID;
    TS.LocalSamplesCorrectOID = _localSamplesCorrectOID;
  }
};

struct InplaceFunctor {
  void *operator()() {
    auto ptr = shad::Array<uint64_t>::GetPtr(oid_);
    uint64_t address = ptr->At(static_cast<uint32_t>(shad::rt::thisLocality()));
    return reinterpret_cast<void *>(address);
  }

  shad::Array<uint64_t>::ObjectID oid_;
};

template <typename TrainingState> void vcTrainLoop(TrainingState &TS) {
  size_t train_correct = 0;
  size_t train_size = 0;

  for (auto &batch : *TS.TrainDataLoader) {
    TS.Inputs[0] = batch.Features;
    TS.Inputs[1] = batch.EdgeIndex;
    train_size += torch::sum(batch.Mask).template item<int64_t>();
    auto groundTruth = batch.Labels;

    TS.Module.train();
    auto output = TS.Module.forward(TS.Inputs).toTensor();

    auto loss = torch::nn::functional::nll_loss(
        output.index({batch.Mask}), groundTruth.index({batch.Mask}));

    loss.backward();
    TS.Module.eval();
    auto prediction = std::get<1>(output.max(1));
    auto equal = prediction.eq(groundTruth);
    train_correct += equal.index({batch.Mask}).sum().template item<int64_t>();
  }

  auto localSamplesProcessedPtr =
      shad::Array<uint64_t>::GetPtr(TS.LocalSamplesProcessedOID);
  auto localSamplesCorrectPtr =
      shad::Array<uint64_t>::GetPtr(TS.LocalSamplesCorrectOID);
  localSamplesProcessedPtr->InsertAt(TS.TID, train_size);
  localSamplesCorrectPtr->InsertAt(TS.TID, train_correct);
}

template <typename TrainingState, typename TrainingStateItr>
void vcReduceGradients(TrainingStateItr B, TrainingStateItr E) {
  auto start = std::chrono::high_resolution_clock::now();
  int64_t numRanks = shad::rt::numLocalities();

  static const std::map<at::ScalarType, MPI_Datatype> torchToMPITypes = {
      {at::kByte, MPI_UNSIGNED_CHAR},
      {at::kChar, MPI_CHAR},
      {at::kDouble, MPI_DOUBLE},
      {at::kFloat, MPI_FLOAT},
      {at::kInt, MPI_INT},
      {at::kLong, MPI_LONG},
      {at::kShort, MPI_SHORT},
  };

  const auto & trainState = (*B).get();

  InplaceFunctor F{trainState.ReducerLocalPtrOID};
  for (int i = 0; i < trainState.Module.parameters().size(); ++i) {
    shad::for_each(
        shad::distributed_parallel_tag{}, B, E, [=](TrainingState &TS) {
          auto itr = TS.Module.parameters().begin();
          for (int j = 0; j < i; ++j)
            ++itr;
          auto localPtrs = shad::Array<uint64_t>::GetPtr(TS.ReducerLocalPtrOID);
          localPtrs->InsertAt(
              static_cast<uint32_t>(shad::rt::thisLocality()),
              reinterpret_cast<uint64_t>((*itr).mutable_grad().data_ptr()));
        });

    auto itr2 = trainState.Module.parameters().begin();
    for (int j = 0; j < i; ++j)
      ++itr2;
    auto reducer = shad::MPIReducer<InplaceFunctor, InplaceFunctor>::Create(
        torchToMPITypes.at((*itr2).mutable_grad().scalar_type()), F, F);

    reducer->AllReduce(MPI_SUM, (*itr2).mutable_grad().numel());

    shad::for_each(shad::distributed_parallel_tag{}, B, E,
                   [=](TrainingState &TS) {
                     auto itr = TS.Module.parameters().begin();
                     // This loop is here because this iterator does not implement operator+
                     // and capturing it from outside this lambda would be incorrect
                     // in distributed settings.
                     for (int j = 0; j < i; ++j)
                       ++itr;
                     (*itr).mutable_grad().data() =
                         (*itr).mutable_grad().data() / numRanks;
                   });
  }
  auto end = std::chrono::high_resolution_clock::now();
  std::cout << shad::rt::thisLocality() << " Reduction Time (s) : "
            << std::chrono::duration_cast<std::chrono::duration<double>>(end -
                                                                         start)
                   .count()
            << std::endl;
}

template <typename TrainingState>
void vcBackPropAndEvaluationLoop(TrainingState &TS) {
  size_t test_correct = 0;
  size_t test_size = 0;
  TS.Adam->step();
  TS.Adam->zero_grad();

  for (auto &batch : *TS.TestDataLoader) {
    test_size += torch::sum(batch.Mask).template item<int64_t>();
    TS.Module.eval();
    TS.Inputs[0] = batch.Features;
    TS.Inputs[1] = batch.EdgeIndex;

    auto groundTruth = batch.Labels;
    auto output = TS.Module.forward(TS.Inputs).toTensor();
    auto prediction = std::get<1>(output.max(1));
    auto equal = prediction.eq(groundTruth);
    test_correct += equal.index({batch.Mask}).sum().template item<int64_t>();
  }

  auto localSamplesProcessedPtr =
      shad::Array<uint64_t>::GetPtr(TS.LocalSamplesProcessedOID);
  auto localSamplesCorrectPtr =
      shad::Array<uint64_t>::GetPtr(TS.LocalSamplesCorrectOID);
  localSamplesProcessedPtr->InsertAt(TS.TID , test_size);
  localSamplesCorrectPtr->InsertAt(TS.TID, test_correct);
}

typename shad::Array<
    agile::workflow1::TrainingState<VertexClassificationWMDDataset>>::ObjectID
GCN(uint64_t &num_edges, uint64_t &num_vertices, Graph_t &graph,
    std::string modelFileName);

// typename shad::Array<
//     agile::workflow1::TrainingState<LinkPredictionWMDDataset>>::ObjectID
// LinkPredictor(uint64_t &num_edges, uint64_t &num_vertices, Graph_t &graph,
//               std::string modelFileName);
} // namespace agile::workflow1

#endif
