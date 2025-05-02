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
#ifndef AGILE_WORKFLOW1_LINKPREDICTION_H
#define AGILE_WORKFLOW1_LINKPREDICTION_H

#include <cstdint>
#include <memory>
#include <vector>
#include <random>
#include "agile/workflow1/graph.h"
#include "agile/workflow1/main.h"
#include "agile/workflow1/wmd.h"
#include "agile/workflow1/utils.h"

#include "shad/core/algorithm.h"
#include "shad/data_structures/array.h"
#include "shad/extensions/collectives/mpi_reduce.h"
#include "torch/script.h"
#include "torch/torch.h"

namespace agile::workflow1 {
struct EndPointsEdgeCompare {
  bool operator()(const Edge *e1, const Edge *e2) const {
    // This is because SHAD uses memcmp convention instead of the C++ comparator convention.
    return !((e1->src_glbid == e2->src_glbid && e1->dst_glbid == e2->dst_glbid) ||
             (e1->src_glbid == e2->dst_glbid && e1->dst_glbid == e2->src_glbid));
  }
};

inline auto GenerateFalseEdges(
    size_t numEdges,
    typename shad::Set<Edge, EndPointsEdgeCompare>::SharedPtr Edges,
    size_t numVertices) {
  auto result = XEdgeType::Create(numEdges, Edge());

  auto EdgesOID = Edges->GetGlobalID();

  shad::generate(
      shad::distributed_parallel_tag{}, result->begin(), result->end(), [=]() {
        auto Edges = shad::Set<Edge, EndPointsEdgeCompare>::GetPtr(EdgesOID);
        std::random_device rd;
        std::default_random_engine G(rd());
        std::uniform_int_distribution<uint64_t> dist(0, numVertices - 1);

        Edge e;
        do {
          // throw a dart in the matrix
          e.src_glbid = dist(G);
          e.dst_glbid = dist(G);
          if (e.src_glbid > e.dst_glbid)
            std::swap(e.src_glbid, e.dst_glbid);

        } while (Edges->Find(e));
        // found a missing edge
        return e;
      });

  return result;
}

using EdgeSetType = shad::Set<Edge, EndPointsEdgeCompare>;
using cp_args_t = std::tuple<size_t, XEdgeType::ObjectID, EdgeSetType::ObjectID>;


static void copy_fn(shad::rt::Handle& h, const cp_args_t& args) {
  auto xoid = std::get<1>(args);
  auto soid = std::get<2>(args);
  auto set = EdgeSetType::GetPtr(soid);
  auto xptr = XEdgeType::GetPtr(xoid);
  auto lset = set->GetLocalSet();
  auto start_idx = std::get<0>(args);
  size_t next_idx = start_idx + lset->Size();
  uint32_t next_loc = static_cast<uint32_t>(shad::rt::thisLocality()) + 1;
  //std::cout << shad::rt::thisLocality() << " start_idx " << start_idx << std::endl;
  auto fel = [](shad::rt::Handle & h, const Edge &entry,
                XEdgeType::SharedPtr &ptr,
                size_t *&cntPtr) {
    auto idx = __sync_fetch_and_add(cntPtr, 1);
    ptr->AsyncInsertAt(h, idx, entry);
  };
  if (next_loc < shad::rt::numLocalities()) {
    cp_args_t cpargs(next_idx, xoid, soid);
    shad::rt::asyncExecuteAt(h, shad::rt::Locality(next_loc), copy_fn, cpargs);
  }
  size_t* cntPtr = &start_idx;
  shad::rt::Handle h2;
  lset->AsyncForEachElement(h2, fel, xptr, cntPtr);
  shad::rt::waitForCompletion(h2);
  //std::cout << shad::rt::thisLocality() << " cnt " << start_idx << std::endl;
}

// static void copy_fn(shad::rt::Handle& h, const cp_args_t& args) {
//   auto xoid = std::get<1>(args);
//   auto soid = std::get<2>(args);
//   auto set = EdgeSetType::GetPtr(soid);
//   auto lset = set->GetLocalSet();
//   auto start_idx = std::get<0>(args);
//   size_t next_idx = start_idx + lset->Size();
//   uint32_t next_loc = static_cast<uint32_t>(shad::rt::thisLocality()) + 1;
//   std::cout << shad::rt::thisLocality() << " start_idx " << start_idx << std::endl;
//   auto fel = [](shad::rt::Handle &, const Edge &entry,
//                 XEdgeType::SharedPtr& arrayPtr,
//                 size_t *&cntPtr) {
//     arrayPtr->AsyncInsertAt(h, *cntPtr, entry);
//     __sync_fetch_and_add(cntPtr, 1);
//   };
//   if (next_loc < shad::rt::numLocalities()) {
//     cp_args_t cpargs(next_idx, xoid, soid);
//     shad::rt::asyncExecuteAt(h, shad::rt::Locality(next_loc), copy_fn, cpargs);
//   }
//   size_t* cntPtr = &start_idx;
//   uint64_t magicValue = 42;
//   shad::rt::Handle h2;
//   lset->AsyncForEachElement(h2, fel, cntPtr);
//   shad::rt::waitForCompletion(h2);
//   std::cout << shad::rt::thisLocality() << " cnt " << start_idx << std::endl;
// }

inline auto GenerateLinkPredictionDataSet(VertexOID VertexArrayID, XEdgeOID EdgeArrayOID, 
                                  float train,float validation) {

  auto Vertices = VertexType::GetPtr((VertexOID) VertexArrayID);
  auto allEdges = XEdgeType::GetPtr((XEdgeOID) EdgeArrayOID);

  // Get lower triangular part
  shad::rt::Handle h = shad::rt::impl::createHandle();
  // using EdgeSetType = shad::Set<Edge, EndPointsEdgeCompare>;
  auto EdgeSet = EdgeSetType::Create(allEdges->Size() / 2);
  auto EdgeSetOID = EdgeSet->GetGlobalID();

  allEdges->AsyncForEach(
      h,
      [](shad::rt::Handle &h, size_t i, Edge &e,
         EdgeSetType::ObjectID &EdgeSetOID) {
        if (e.src_glbid < e.dst_glbid) {
          auto set = EdgeSetType::GetPtr(EdgeSetOID);
          set->AsyncInsert(h, e);
        }
      },
      EdgeSetOID);
  shad::rt::waitForCompletion(h);

  auto EdgeSetSize = EdgeSet->Size();
  auto VerticesArraySize = Vertices->Size();
  auto Edges = XEdgeType::Create(EdgeSetSize, Edge());

  //auto setCopyBegin = my_timer();
//  copy(shad::distributed_parallel_tag{}, EdgeSet->begin(), EdgeSet->end(), Edges->begin());
 
  auto xgid = Edges->GetGlobalID();
 
  cp_args_t cpargs(0, xgid, EdgeSetOID);
  shad::rt::asyncExecuteAt(h, shad::rt::thisLocality(), copy_fn, cpargs);
  shad::rt::waitForCompletion(h);
 
 
  //auto setCopyEnd = my_timer();

  //std::cout << "Set copy time: " << setCopyEnd - setCopyBegin << std::endl; 


  size_t trueTrainEdgesNum = std::floor(EdgeSetSize * train);
  size_t trueValidationEdgesNum = std::floor(EdgeSetSize * validation);
  size_t trueTestEdgesNum = EdgeSetSize - trueTrainEdgesNum - trueValidationEdgesNum;

  size_t falseEdgesTotal = std::min(((VerticesArraySize - 1) * (VerticesArraySize - 1)) - EdgeSetSize, EdgeSetSize);
  size_t falseTrainEdgesNum = std::floor(falseEdgesTotal * train);
  size_t falseValidationEdgesNum = std::floor(falseEdgesTotal * validation);
  size_t falseTestEdgesNum = falseEdgesTotal - falseTrainEdgesNum - falseValidationEdgesNum;

  auto trainingSet = XEdgeType::Create(trueTrainEdgesNum + falseTrainEdgesNum, Edge());
  auto validationSet = XEdgeType::Create(trueValidationEdgesNum + falseValidationEdgesNum, Edge());
  auto testSet = XEdgeType::Create(trueTestEdgesNum + falseTestEdgesNum, Edge());

  auto falseEdgeArray = GenerateFalseEdges(falseEdgesTotal, EdgeSet, Vertices->Size() - 1);
  
  auto begin = Edges->begin();
  auto end = begin + trueTrainEdgesNum;
  auto falseTrainItrB = copy(shad::distributed_parallel_tag{}, begin, end, trainingSet->begin());

  begin = Edges->begin() + trueTrainEdgesNum;
  end = begin + trueValidationEdgesNum;
  auto falseValItrB = copy(shad::distributed_parallel_tag{}, begin, end, validationSet->begin());
 
  begin = Edges->begin() + trueTrainEdgesNum + trueValidationEdgesNum;
  end = begin + trueTestEdgesNum;
  auto falseTestItrB = copy(shad::distributed_parallel_tag{}, begin, end, testSet->begin());

  // Generate False Edges
  begin = falseEdgeArray->begin();
  end = begin + falseTrainEdgesNum;
  copy(shad::distributed_parallel_tag{}, begin, end, falseTrainItrB);
  begin = falseEdgeArray->begin() + falseTrainEdgesNum;
  end = begin + falseValidationEdgesNum;
  copy(shad::distributed_parallel_tag{}, begin, end, falseValItrB);
  begin = falseEdgeArray->begin() + falseTrainEdgesNum + falseValidationEdgesNum;
  end = falseEdgeArray->end();
  copy(shad::distributed_parallel_tag{}, begin, end, falseTestItrB);

  // Create Observed Graph
  Graph_t observedGraph;

  auto observedGraphVertices = VertexType::Create(Vertices->Size(), Vertex());
  auto observedGraphVerticesOID = observedGraphVertices->GetGlobalID();
  observedGraph["Vertices"] = static_cast<uint64_t>(observedGraphVerticesOID);

  auto observedEdges = EdgeType::Create(AGILE_LARGE);
  auto observedEdgesOID = observedEdges->GetGlobalID();
  observedGraph["Edges"] = static_cast<uint64_t>(observedEdgesOID);

  auto observedXEdgesOID = XEdgeType::Create(trueTrainEdgesNum + trueValidationEdgesNum, Edge())->GetGlobalID();
  observedGraph["XEdges"] = static_cast<uint64_t>(observedXEdgesOID);

  auto edgeInserter = [](shad::rt::Handle &h, size_t i, Edge &e,
                         size_t &edgeMapOID) {
    auto observedEdges = EdgeType::GetPtr(EdgeType::ObjectID(edgeMapOID));
    observedEdges->BufferedAsyncInsert(h, e.src, e);
  };

  // 1 - Insert all the edges in Train+Validation in an hashmap
  trainingSet->AsyncForEachInRange(h, 0, trueTrainEdgesNum, edgeInserter,
                                   observedGraph["Edges"]);

     
  validationSet->AsyncForEachInRange(h, 0, trueValidationEdgesNum, edgeInserter,
                                     observedGraph["Edges"]);
  shad::rt::waitForCompletion(h);
  observedEdges->WaitForBufferedInsert();


  shad::transform(shad::distributed_parallel_tag{}, Vertices->begin(),
                  Vertices->end(), observedGraphVertices->begin(),
                  [observedEdgesOID](auto &v) {
                    auto edges = EdgeType::GetPtr(observedEdgesOID);
                    Vertex out = v;
                    typename EdgeType::LookupResult res;
                    out.start=0;
                    edges->Lookup(out.id, &res);
                    if (res.found) {
                      out.edges = res.size;
                    } else {
                      out.edges = 0;
                    }
                    return out;
                  });
  
  // 2 - compute prefix scan of the neighboorhoods size
  exclusiveScanVertices<Vertex>(
      observedGraph["Vertices"]); // convert # edges to start location


  // 3 - copy the content of the hashmap into the CSR
  // shad::for_each(shad::distributed_parallel_tag{},
  // observedEdges->key_begin(),
  //                observedEdges->key_end(),
  //                [h, observedXEdgesOID, observedGraphVerticesOID](auto &t) {
  //                  auto key = std::get<0>(t);
  //                  auto &edges = std::get<1>(t);
  //                  auto XEdges = XEdgesType::GetPtr(observedXEdgesOID);
  //                  auto Vertices =
  //                  VertexType::GetPtr(observedGraphVerticesOID);

  //                  auto pos = Vertices->At(key).start;
  //                  for (auto &e : edges) {
  //                    XEdges->AsyncInsertAt(h, pos, e);
  //                    ++pos;
  //                  }
  //                });

  observedEdges->AsyncForEachEntry(
      h,
      [](shad::rt::Handle &h, const auto key, auto &edges,
         auto &observedXEdgesOID, auto &observedGraphVerticesOID) {

        if (edges.empty()) return;

        auto XEdges = XEdgeType::GetPtr(observedXEdgesOID);
        auto Vertices = VertexType::GetPtr(observedGraphVerticesOID);
        auto pos = Vertices->At(edges[0].src_glbid).start;
        for (auto &e : edges) {
          XEdges->AsyncInsertAt(h, pos, e);
          ++pos;
        }
      },
      observedXEdgesOID, observedGraphVerticesOID);
  shad::rt::waitForCompletion(h);

  // Freeing up temporaries
  EdgeSetType::Destroy(EdgeSetOID);
  XEdgeType::Destroy(Edges->GetGlobalID());
  return std::make_tuple(observedGraph, trainingSet->GetGlobalID(),
                         validationSet->GetGlobalID(), testSet->GetGlobalID());
}



template <typename Dataset> struct lpTrainingState {
public:
  using ArrayOID = typename shad::Array<uint64_t>::ObjectID;
  using ArrayOIDfloat = typename shad::Array<float>::ObjectID;
  using sampler_type = torch::data::samplers::DistributedRandomSampler;
  using dataset_type = torch::data::datasets::MapDataset<
      Dataset, torch::data::transforms::Stack<typename Dataset::Data>>;
  using data_loader_type =
      torch::data::StatelessDataLoader<dataset_type, sampler_type>;

  lpTrainingState() = default;
  lpTrainingState(const lpTrainingState &) = default;
  lpTrainingState(lpTrainingState &&) = default;

  lpTrainingState &operator=(const lpTrainingState &) = default;
  lpTrainingState &operator=(lpTrainingState &&) = default;

  torch::jit::script::Module Module;
  //torch::nn::BCEWithLogitsLoss criterion;
  Dataset TrainDataset;
  Dataset TestDataset;
  Dataset ValidationDataset;
  std::shared_ptr<data_loader_type> TrainDataLoader{nullptr};
  std::shared_ptr<data_loader_type> TestDataLoader{nullptr};
  std::shared_ptr<data_loader_type> ValidationDataLoader{nullptr};
  std::shared_ptr<torch::optim::Adam> Adam{nullptr};
  std::vector<torch::jit::IValue> Inputs;
  ArrayOID ReducerLocalPtrOID{ArrayOID::kNullID};
  ArrayOID LocalSamplesProcessedOID{ArrayOID::kNullID};
  ArrayOIDfloat LocalSamplesCorrectOID{ArrayOIDfloat::kNullID};
  size_t TID{0};
};

template <typename Dataset> class lpSetUpTrainingContext {
  using ArrayOID = typename shad::Array<uint64_t>::ObjectID;
  using ArrayOIDfloat = typename shad::Array<float>::ObjectID;

  char modelFileName_[256];
  VertexOID _verticesOID;
  XEdgeOID _edgesOID;
  XEdgeOID _edgesOIDTrain;
  XEdgeOID _edgesOIDTest;
  XEdgeOID _edgesOIDValidation;
  ArrayOID _featuresOID;
  ArrayOID _reducerArrayOID;
  ArrayOID _localSamplesProcessedOID;
  ArrayOIDfloat _localSamplesCorrectOID;

public:
  lpSetUpTrainingContext(const VertexOID &VertexArrayID,
                       const XEdgeOID &EdgeArrayOID,
                       const ArrayOID &FeaturesArrayID,
                       const XEdgeOID &EdgeArrayOIDTrain,
                       const XEdgeOID &EdgeArrayOIDTest,
                       const XEdgeOID &EdgeArrayOIDValidation,
                       const ArrayOID &ReducerArrayOID,
                       const ArrayOID &LocalSamplesProcessedOID,
                       const ArrayOIDfloat &LocalSamplesCorrectOID,
                       std::string modelFileName)
      : _verticesOID(VertexArrayID), _edgesOID(EdgeArrayOID),
        _featuresOID(FeaturesArrayID), _edgesOIDTrain(EdgeArrayOIDTrain), 
        _edgesOIDTest(EdgeArrayOIDTest), _edgesOIDValidation(EdgeArrayOIDValidation), _reducerArrayOID(ReducerArrayOID),
        _localSamplesProcessedOID(LocalSamplesProcessedOID),
        _localSamplesCorrectOID(LocalSamplesCorrectOID) {
    if (modelFileName.size() > 256)
      throw "Filename too long";

    std::strcpy(modelFileName_, modelFileName.c_str());
  }

  void operator()(size_t tid, lpTrainingState<Dataset> &TS) {
    // TID
    TS.TID = tid;
    // Load Module
    TS.Module = torch::jit::load(modelFileName_);
    // // Load Dataset
    // TS.DataSet = Dataset(_verticesOID, _edgesOID, _featuresOID);
    TS.TrainDataset = Dataset(_verticesOID,  _edgesOID,_featuresOID, _edgesOIDTrain);
    TS.TestDataset = Dataset(_verticesOID,  _edgesOID,_featuresOID, _edgesOIDTest);
    TS.ValidationDataset = Dataset(_verticesOID,  _edgesOID,_featuresOID, _edgesOIDValidation);

    // Number threads
    size_t total_ranks =
        shad::rt::numLocalities() * shad::rt::impl::getConcurrency();
    const int64_t trainingSetSize = TS.TrainDataset.size().value();
    const int64_t testSetSize = TS.TestDataset.size().value();
    const int64_t validationSetSize = TS.ValidationDataset.size().value();
    const size_t batchSize = std::min<size_t>(32, trainingSetSize / total_ranks);

    //size_t numVertices = TS.DataSet.size().value();
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
    auto validation_sampler = torch::data::samplers::DistributedRandomSampler(
        validationSetSize, total_ranks, tid, false);

    auto stackedDataSetTrain = TS.TrainDataset.map(
        torch::data::transforms::Stack<typename Dataset::Data>());

    auto stackedDataSetTest = TS.TestDataset.map(
        torch::data::transforms::Stack<typename Dataset::Data>());

    auto stackedDataSetValidation = TS.ValidationDataset.map(
        torch::data::transforms::Stack<typename Dataset::Data>());
    torch::data::DataLoaderOptions DLOptions(batchSize);

    TS.TrainDataLoader =
        torch::data::make_data_loader(stackedDataSetTrain, train_sampler, DLOptions);
    TS.TestDataLoader =
        torch::data::make_data_loader(stackedDataSetTest, test_sampler, DLOptions);
    TS.ValidationDataLoader =
        torch::data::make_data_loader(stackedDataSetValidation, validation_sampler, DLOptions);

    // Create Optimizer
    std::vector<at::Tensor> parameters;
    for (const auto &params : TS.Module.parameters()) {
      if (params.requires_grad()) {
        parameters.push_back(params);
      }
    }

    const double learningRate = 0.00005;
    TS.Adam = std::make_unique<torch::optim::Adam>(
        parameters, torch::optim::AdamOptions(learningRate).weight_decay(0));

    // Set inputs
    TS.Inputs.resize(4);
    TS.ReducerLocalPtrOID = _reducerArrayOID;
    TS.LocalSamplesProcessedOID = _localSamplesProcessedOID;
    TS.LocalSamplesCorrectOID = _localSamplesCorrectOID;
  }
};

template <typename lpTrainingState> void lpTrainLoop(lpTrainingState &TS) {
  float train_correct = 0;
  size_t train_size = 0;
  //torch::AutoGradMode enable_grad(true);
  TS.Module.train();
  auto tempTensor = torch::Tensor();
  auto criterion = torch::nn::BCEWithLogitsLoss();
  for (auto &batch : *TS.TrainDataLoader) {
    TS.Inputs[0] = batch.Features;
    TS.Inputs[1] = batch.EdgeIndex;
    TS.Inputs[2] = batch.Mask;
    TS.Inputs[3] = batch.Batch_Mask;

    train_size += 1;
    auto groundTruth = batch.Labels;
    auto output = TS.Module.forward(TS.Inputs).toTensor();
    auto loss = criterion(output.view(-1), groundTruth);
    TS.Adam->zero_grad();
    loss.backward();
    TS.Adam->step();
    
    train_correct += loss.template item<float>();
  }
  auto localSamplesProcessedPtr =
      shad::Array<uint64_t>::GetPtr(TS.LocalSamplesProcessedOID);
  auto localSamplesCorrectPtr =
      shad::Array<float>::GetPtr(TS.LocalSamplesCorrectOID);
  localSamplesProcessedPtr->InsertAt(TS.TID, train_size);
  localSamplesCorrectPtr->InsertAt(TS.TID, train_correct);
}

template <typename lpTrainingState> void lpTestLoop(lpTrainingState &TS) {
  float test_correct = 0;
  size_t test_size = 0;
  TS.Module.eval();
  auto criterion = torch::nn::BCEWithLogitsLoss();
  //torch::NoGradGuard no_grad;
  
  for (auto &batch :  *TS.TestDataLoader) {
    test_size += 1;
    TS.Inputs[0] = batch.Features;
    TS.Inputs[1] = batch.EdgeIndex;
    TS.Inputs[2] = batch.Mask;
    TS.Inputs[3] = batch.Batch_Mask;
    auto groundTruth = batch.Labels;
    auto output = TS.Module.forward(TS.Inputs).toTensor();
    auto loss = criterion(output.view(-1), groundTruth);

    test_correct += loss.template item<float>();
  }
  auto localSamplesProcessedPtr =
      shad::Array<uint64_t>::GetPtr(TS.LocalSamplesProcessedOID);
  auto localSamplesCorrectPtr =
      shad::Array<float>::GetPtr(TS.LocalSamplesCorrectOID);
  localSamplesProcessedPtr->InsertAt(TS.TID , test_size);
  localSamplesCorrectPtr->InsertAt(TS.TID, test_correct);
}

template <typename lpTrainingState> void lpValidateLoop(lpTrainingState &TS) {
  float test_correct = 0;
  size_t test_size = 0;

  TS.Module.eval();
  auto criterion = torch::nn::BCEWithLogitsLoss();
  {
    //torch::NoGradGuard no_grad;
    //torch::AutoGradMode enable_grad(false);

    
    for (auto &batch :  *TS.ValidationDataLoader) {
      test_size +=1;

      TS.Inputs[0] = batch.Features;
      TS.Inputs[1] = batch.EdgeIndex;
      TS.Inputs[2] = batch.Mask;
      TS.Inputs[3] = batch.Batch_Mask;
      auto groundTruth = batch.Labels;
      auto output = TS.Module.forward(TS.Inputs).toTensor();
      auto loss = criterion(output.view(-1), groundTruth);
      test_correct += loss.template item<float>();
    }
    ////torch::AutoGradMode turn_grad(true);
    //enable_grad=true;
  }

  auto localSamplesProcessedPtr =
      shad::Array<uint64_t>::GetPtr(TS.LocalSamplesProcessedOID);
  auto localSamplesCorrectPtr =
      shad::Array<float>::GetPtr(TS.LocalSamplesCorrectOID);
  localSamplesProcessedPtr->InsertAt(TS.TID , test_size);
  localSamplesCorrectPtr->InsertAt(TS.TID, test_correct);
}




typename shad::Array<
    agile::workflow1::lpTrainingState<LinkPredictionWMDDataset>>::ObjectID
LinkPredictor(uint64_t &num_edges, uint64_t &num_vertices, Graph_t &graph,
              std::string modelFileName);

}

#endif
