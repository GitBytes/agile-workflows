//===------------------------------------------------------------*- C++ -*-===//
//
//                            The AGILE Workflows
//
//===----------------------------------------------------------------------===//
// ** Pre-Copyright Notice
//
// This computer software was prepared by Battelle Memorial Institute,
// hereinafter the Contractor, under Contract No. DE-AC05-76RL01830 with the
// Department of Energy (DOE). All rights in the computer software are reserved
// by DOE on behalf of the United States Government and the Contractor as
// provided in the Contract. You are authorized to use this computer software
// for Governmental purposes but it is not to be released or distributed to the
// public. NEITHER THE GOVERNMENT NOR THE CONTRACTOR MAKES ANY WARRANTY, EXPRESS
// OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE. This
// notice including this sentence must appear on any copies of this computer
// software.
//
// ** Disclaimer Notice
//
// This material was prepared as an account of work sponsored by an agency of
// the United States Government. Neither the United States Government nor the
// United States Department of Energy, nor Battelle, nor any of their employees,
// nor any jurisdiction or organization that has cooperated in the development
// of these materials, makes any warranty, express or implied, or assumes any
// legal liability or responsibility for the accuracy, completeness, or
// usefulness or any information, apparatus, product, software, or process
// disclosed, or represents that its use would not infringe privately owned
// rights. Reference herein to any specific commercial product, process, or
// service by trade name, trademark, manufacturer, or otherwise does not
// necessarily constitute or imply its endorsement, recommendation, or favoring
// by the United States Government or any agency thereof, or Battelle Memorial
// Institute. The views and opinions of authors expressed herein do not
// necessarily state or reflect those of the United States Government or any
// agency thereof.
//
//                    PACIFIC NORTHWEST NATIONAL LABORATORY
//                                 operated by
//                                   BATTELLE
//                                   for the
//                      UNITED STATES DEPARTMENT OF ENERGY
//                       under Contract DE-AC05-76RL01830
//===----------------------------------------------------------------------===//

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
    return (e1->src_glbid == e2->src_glbid && e1->dst_glbid == e2->dst_glbid) ||
           (e1->src_glbid == e2->dst_glbid && e1->dst_glbid == e2->src_glbid);
  }
};

inline auto GenerateFalseEdges(
    size_t numEdges,
    typename shad::Set<Edge, EndPointsEdgeCompare>::SharedPtr Edges,
    size_t numVertices) {
  auto result = XEdgeType::Create(numEdges, Edge());

  auto EdgesOID = Edges->GetGlobalID();

  shad::generate(
      shad::distributed_parallel_tag{}, result->begin(), result->end()-1, [=]() {
        auto Edges = shad::Set<Edge, EndPointsEdgeCompare>::GetPtr(EdgesOID);
        // std::cout << static_cast<uint64_t>(EdgesOID) << std::endl;
        // std::cout<<Edges<<std::endl;
        std::random_device rd;
        std::default_random_engine G(rd());
        std::uniform_int_distribution<uint64_t> dist(0,
                                                     (numVertices * numVertices) - 1);

        Edge e;
        do {
          // throw a dart in the matrix
          uint64_t idx = dist(G);
          e.src_glbid = idx / numVertices;
          e.dst_glbid = idx % numVertices;
          // std::cout<<e.src_glbid<<" "<<e.dst_glbid<<std::endl;
          if (e.src_glbid > e.dst_glbid)
            std::swap(e.src_glbid, e.dst_glbid);
        } while (false && Edges->Find(e)); //TODO : fix issue with Find crashinbg
        // found a missing edge
        return e;
      });

  return result;
}

inline auto GenerateLinkPredictionDataSet(VertexOID VertexArrayID, XEdgeOID EdgeArrayOID, 
                                  float train,float validation) {

  auto Vertices = VertexType::GetPtr((VertexOID) VertexArrayID);
  auto allEdges = XEdgeType::GetPtr((XEdgeOID) EdgeArrayOID);

  // Get lower triangular part
  shad::rt::Handle h = shad::rt::impl::createHandle();
  using EdgeSetType = shad::Set<Edge, EndPointsEdgeCompare>;
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
  std::cout << "heyo1" << std::endl;
  auto Edges = XEdgeType::Create(EdgeSet->Size(), Edge());
  copy(shad::distributed_parallel_tag{}, EdgeSet->begin(), EdgeSet->end(),
       Edges->begin());
  std::cout << "heyo2" << std::endl;
  size_t trueTrainEdgesNum = std::floor(Edges->Size() * train);
  size_t trueValidationEdgesNum = std::floor(Edges->Size() * validation);
  size_t trueTestEdgesNum =
      Edges->Size() - trueTrainEdgesNum - trueValidationEdgesNum;
  std::cout << "heyo3" << std::endl;
  size_t falseEdgesTotal =
      std::min((Vertices->Size() - 1) * (Vertices->Size() - 1), Edges->Size());
  size_t falseTrainEdgesNum = std::floor(falseEdgesTotal * train);
  size_t falseValidationEdgesNum = std::floor(falseEdgesTotal * validation);
  size_t falseTestEdgesNum =
      falseEdgesTotal - falseTestEdgesNum - falseValidationEdgesNum;

  auto trainingSet =
      XEdgeType::Create(trueTrainEdgesNum + falseTrainEdgesNum, Edge());
  auto validationSet = XEdgeType::Create(
      trueValidationEdgesNum + falseValidationEdgesNum, Edge());
  auto testSet =
      XEdgeType::Create(trueTestEdgesNum + falseTestEdgesNum, Edge());

  auto falseEdgeArray =
      GenerateFalseEdges(falseEdgesTotal, EdgeSet, Vertices->Size() - 1);

  auto begin = Edges->begin();
  auto end = begin + trueTrainEdgesNum;
  auto falseTrainItrB =
      copy(shad::distributed_parallel_tag{}, begin, end, trainingSet->begin());

  begin = end;
  end += trueValidationEdgesNum;
  auto falseValItrB = copy(shad::distributed_parallel_tag{}, begin, end,
                           validationSet->begin());

  begin = end;
  end += trueTestEdgesNum;
  auto falseTestItrB =
      copy(shad::distributed_parallel_tag{}, begin, end, testSet->begin());
  std::cout << "let's go1" << std::endl;
  // Generate False Edges
  begin = falseEdgeArray->begin();
  end = begin + falseTrainEdgesNum;
  copy(shad::distributed_parallel_tag{}, begin, end, falseTrainItrB);
  std::cout << "let's go2" << std::endl;
  begin = end;
  end += falseValidationEdgesNum;
  std::cout<<begin<<" "<<end<< " " <<falseEdgeArray->Size()<<std::endl;
  copy(shad::distributed_parallel_tag{}, begin, end, falseValItrB);
  std::cout << "let's go3" << std::endl;
  begin = end;
  end += falseTestEdgesNum;
  
  std::cout<<begin<<" "<<end<< " " <<falseEdgeArray->Size()<<std::endl;
  copy(shad::distributed_parallel_tag{}, begin, end, falseTestItrB);
  std::cout << "let's go4" << std::endl;
  // Create Observed Graph
  Graph_t observedGraph;
  auto observedGraphVertices = VertexType::Create(Vertices->Size(), Vertex());
  auto observedGraphVerticesOID = observedGraphVertices->GetGlobalID();
  observedGraph["Vertices"] = static_cast<uint64_t>(observedGraphVerticesOID);
  auto observedEdges = EdgeType::Create(AGILE_LARGE);
  observedGraph["Edges"] = static_cast<uint64_t>(observedEdges->GetGlobalID());
  auto observedXEdgesOID =
      XEdgeType::Create(trueTrainEdgesNum + trueValidationEdgesNum, Edge())
          ->GetGlobalID();
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

  auto observedEdgesOID = observedEdges->GetGlobalID();

  shad::transform(shad::distributed_parallel_tag{}, Vertices->begin(),
                  Vertices->end(), observedGraphVertices->begin(),
                  [observedEdgesOID](auto &v) {
                    auto edges = EdgeType::GetPtr(observedEdgesOID);
                    Vertex out = v;
                    typename EdgeType::LookupResult res;
                    edges->Lookup(out.id, &res);
                    out.edges = res.size;
                    return out;
                  });
  std::cout << "let's go5" << std::endl;
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
      [](shad::rt::Handle &h, const auto &key, auto &edges,
         auto &observedXEdgesOID, auto &observedGraphVerticesOID) {
        auto XEdges = XEdgeType::GetPtr(observedXEdgesOID);
        auto Vertices = VertexType::GetPtr(observedGraphVerticesOID);

        auto pos = Vertices->At(key).start;
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
  ArrayOID LocalSamplesCorrectOID{ArrayOID::kNullID};
  size_t TID{0};
};

template <typename Dataset> class lpSetUpTrainingContext {
  using ArrayOID = typename shad::Array<uint64_t>::ObjectID;

  char modelFileName_[256];
  VertexOID _verticesOID;
  XEdgeOID _edgesOID;
  XEdgeOID _edgesOIDTrain;
  XEdgeOID _edgesOIDTest;
  XEdgeOID _edgesOIDValidation;
  ArrayOID _featuresOID;
  ArrayOID _reducerArrayOID;
  ArrayOID _localSamplesProcessedOID;
  ArrayOID _localSamplesCorrectOID;

public:
  lpSetUpTrainingContext(const VertexOID &VertexArrayID,
                       const XEdgeOID &EdgeArrayOID,
                       const ArrayOID &FeaturesArrayID,
                       const XEdgeOID &EdgeArrayOIDTrain,
                       const XEdgeOID &EdgeArrayOIDTest,
                       const XEdgeOID &EdgeArrayOIDValidation,
                       const ArrayOID &ReducerArrayOID,
                       const ArrayOID &LocalSamplesProcessedOID,
                       const ArrayOID &LocalSamplesCorrectOID,
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
  
    //auto [observedGraph, trainSet, validationSet, testSet] = GenerateLinkPredictionDataSet(_verticesOID, _edgesOID, 0.85, 0.05);
    TS.TrainDataset = Dataset(_verticesOID,  _edgesOID,_featuresOID, _edgesOIDTrain);
    TS.TestDataset = Dataset(_verticesOID,  _edgesOID,_featuresOID, _edgesOIDTest);
    TS.ValidationDataset = Dataset(_verticesOID,  _edgesOID,_featuresOID, _edgesOIDValidation);

    std::cout<<"hey"<<std::endl;
    // Number threads
    size_t total_ranks =
        shad::rt::numLocalities() * shad::rt::impl::getConcurrency();
    const int64_t trainingSetSize = TS.TrainDataset.size().value() / int64_t(4);
    const int64_t testSetSize = TS.TestDataset.size().value();
    const int64_t validationSetSize = TS.ValidationDataset.size().value();
    const size_t batchSize = std::min<size_t>(128, trainingSetSize / total_ranks);

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

// struct InplaceFunctor {
//   void *operator()() {
//     auto ptr = shad::Array<uint64_t>::GetPtr(oid_);
//     uint64_t address = ptr->At(static_cast<uint32_t>(shad::rt::thisLocality()));
//     return reinterpret_cast<void *>(address);
//   }

//   shad::Array<uint64_t>::ObjectID oid_;
// };


template <typename lpTrainingState> void lpTrainLoop(lpTrainingState &TS) {
  size_t train_correct = 0;
  size_t train_size = 0;

  for (auto &batch : *TS.TrainDataLoader) {
    TS.Inputs[0] = batch.Features;
    TS.Inputs[1] = batch.EdgeIndex;
    TS.Inputs[2] = batch.Mask;
    TS.Inputs[3] = torch::ones({batch.Fetures.sizes()[0]});
    
    train_size += torch::sum(batch.Mask).template item<int64_t>();
    auto groundTruth = batch.Labels;

    TS.Module.train();
    auto output = TS.Module.forward(TS.Inputs).toTensor();

    auto criterion = torch::nn::BCEWithLogitsLoss();
    auto loss = criterion(output.index({batch.Mask}), groundTruth.index({batch.Mask}));
    TS.Adam->zero_grad();
    loss.backward();
    TS.Adam->step();
    
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

typename shad::Array<
    agile::workflow1::lpTrainingState<LinkPredictionWMDDataset>>::ObjectID
LinkPredictor(uint64_t &num_edges, uint64_t &num_vertices, Graph_t &graph,
              std::string modelFileName);

}

#endif