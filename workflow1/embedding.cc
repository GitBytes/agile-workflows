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

#include "agile/workflow1/gnn.h"
#include "agile/workflow1/graph.h"
#include "agile/workflow1/main.h"
#include "agile/workflow1/wmd.h"

namespace agile::workflow1 {
using Emb_t = shad::Array<uint64_t>;
using EmbeddingType = shad::Array<uint64_t>;
using EmbeddingOID = shad::ObjectIdentifier<EmbeddingType>;

struct Args_t {
  uint64_t edgesOID;
  uint64_t embeddingsOID;
};

// Aggregate neigbors histograms ... store into second half of the feature
// vector
void TwoHopFeatures(Handle &handle, const uint64_t ndx, Vertex &vertex,
                    Args_t &args) {
  uint64_t num_edges = vertex.edges;
  if (num_edges == 0)
    return;

  auto Edges = XEdgeType::GetPtr((XEdgeOID)args.edgesOID);
  auto Embeddings = EmbeddingType::GetPtr((EmbeddingOID)args.embeddingsOID);

  Handle my_handle;
  std::vector<Edge> edges(num_edges);
  Edges->AsyncGetElements(my_handle, edges.data(), vertex.start, num_edges);

  waitForCompletion(my_handle);
  uint64_t num_bins = NUM_FEATURES / 2;
  std::vector<uint64_t> my_histogram(num_bins, 0);

  std::vector<uint64_t> neighbor_histograms(num_edges * num_bins);
  uint64_t *data = neighbor_histograms.data();

  for (uint64_t i = 0; i < num_edges; i++, data += num_bins) {
    uint64_t embedding_ndx = edges[i].dst_glbid * NUM_FEATURES;
    Embeddings->AsyncGetElements(my_handle, data, embedding_ndx, num_bins);
  }

  waitForCompletion(my_handle);
  for (uint64_t i = 0; i < num_edges * num_bins; i++)
    my_histogram[i % num_bins] += neighbor_histograms[i];

  uint64_t embedding_ndx =
      ndx * NUM_FEATURES +
      num_bins; // store aggregated histogram in second half
  Embeddings->AsyncInsertAt(handle, embedding_ndx, my_histogram.data(),
                            num_bins);
}

// Histogram edge and neigbor vertex types ... store in first half of feature
// vector
void OneHopFeatures(Handle &handle, const uint64_t ndx, Vertex &vertex,
                    Args_t &args) {
  uint64_t num_edges = vertex.edges;
  if (num_edges == 0)
    return;

  Handle my_handle;
  auto Edges = XEdgeType::GetPtr((XEdgeOID)args.edgesOID);
  auto Embeddings = EmbeddingType::GetPtr((EmbeddingOID)args.embeddingsOID);

  std::vector<Edge> edges(num_edges);
  Edges->AsyncGetElements(my_handle, edges.data(), vertex.start, num_edges);

  waitForCompletion(my_handle);
  uint64_t num_bins = NUM_FEATURES / 2;
  std::vector<uint64_t> histogram(num_bins, 0);

  for (auto &edge : edges) {
    histogram[(uint64_t)edge.type]++;
    histogram[(uint64_t)edge.dst_type]++;
  }

  Embeddings->AsyncInsertAt(handle, ndx * NUM_FEATURES, histogram.data(),
                            num_bins);
}

typename shad::Array<
    agile::workflow1::TrainingState<VertexClassificationWMDDataset>>::ObjectID
GNN(uint64_t &num_edges, uint64_t &num_vertices, Graph_t &graph,
    std::string modelFileName) {
  Handle handle;
  auto Vertices = VertexType::GetPtr((VertexOID)graph["Vertices"]);
  auto Embeddings = EmbeddingType::Create(num_vertices * NUM_FEATURES, 0);

  Embeddings->FillPtrs();
  graph["Embeddings"] = (uint64_t)(Embeddings->GetGlobalID());
  Args_t args = {graph["XEdges"], graph["Embeddings"]};

  Vertices->AsyncForEachInRange(handle, 0, num_vertices, OneHopFeatures, args);
  waitForCompletion(handle);

  Vertices->AsyncForEachInRange(handle, 0, num_vertices, TwoHopFeatures, args);
  waitForCompletion(handle);

  std::cout << "Embeddings created" << std::endl;

  size_t parallelThreads = shad::rt::numLocalities() * shad::rt::impl::getConcurrency();
  TrainingState<VertexClassificationWMDDataset> initState;
  auto TSs = shad::Array<TrainingState<VertexClassificationWMDDataset>>::Create(
      parallelThreads, initState);
  auto reducerArrayOID =
      shad::Array<uint64_t>::Create(parallelThreads, 0ul)
          ->GetGlobalID();
  auto localSamplesProcessedOID =
      shad::Array<uint64_t>::Create(parallelThreads, 0ul)
          ->GetGlobalID();
  auto localSamplesCorrectOID =
      shad::Array<uint64_t>::Create(parallelThreads, 0ul)
          ->GetGlobalID();
  SetUpTrainingContext<VertexClassificationWMDDataset> setup(
      Vertices->GetGlobalID(), (XEdgeOID)graph["XEdges"],
      Embeddings->GetGlobalID(), reducerArrayOID, localSamplesProcessedOID,
      localSamplesCorrectOID, modelFileName);
  // shad::for_each(shad::distributed_parallel_tag{}, TSs->begin(), TSs->end(),
  //                setup);
  TSs->ForEach([](size_t tid, TrainingState<VertexClassificationWMDDataset> & TS,
                  SetUpTrainingContext<VertexClassificationWMDDataset> &setup) {
    setup(tid, TS);
  }, setup);

  std::cout << "Initialized Training State" << std::endl;

  const size_t numEpochs = 200;
  auto localSamplesProcessedPtr =
      shad::Array<uint64_t>::GetPtr(localSamplesProcessedOID);
  auto localSamplesCorrectPtr =
      shad::Array<uint64_t>::GetPtr(localSamplesCorrectOID);
  for (size_t epoch = 0; epoch < numEpochs; ++epoch) {
    std::cout << "-- Epoch " << epoch + 1
              << " ------------------------------------------------------------"
                 "--------"
              << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    shad::for_each(shad::distributed_parallel_tag{}, TSs->begin(), TSs->end(),
                   agile::workflow1::vcTrainLoop<
                       TrainingState<VertexClassificationWMDDataset>>);

    auto train_size = shad::reduce(shad::distributed_parallel_tag{},
                                   localSamplesProcessedPtr->begin(),
                                   localSamplesProcessedPtr->end());

    auto train_correct = shad::reduce(shad::distributed_parallel_tag{},
                                      localSamplesCorrectPtr->begin(),
                                      localSamplesCorrectPtr->end());

    std::cout << "Train Accuracy: " << train_correct << "/" << train_size
              << " = " << static_cast<float>(train_correct) / train_size
              << std::endl;
    if (shad::rt::numLocalities() > 1) {
      vcReduceGradients<TrainingState<VertexClassificationWMDDataset>>(
          TSs->begin(), TSs->end());
    }

    shad::for_each(shad::distributed_parallel_tag{}, TSs->begin(), TSs->end(),
                   agile::workflow1::vcBackPropAndEvaluationLoop<
                       TrainingState<VertexClassificationWMDDataset>>);
    auto test_size = shad::reduce(shad::distributed_parallel_tag{},
                                  localSamplesProcessedPtr->begin(),
                                  localSamplesProcessedPtr->end());

    auto test_correct = shad::reduce(shad::distributed_parallel_tag{},
                                     localSamplesCorrectPtr->begin(),
                                     localSamplesCorrectPtr->end());

    std::cout << "Test Accuracy: " << test_correct << "/" << test_size << " = "
              << static_cast<float>(test_correct) / test_size << std::endl;

    auto end = std::chrono::high_resolution_clock::now();

    std::cout << shad::rt::thisLocality() << " Time (s) : "
              << std::chrono::duration_cast<std::chrono::duration<double>>(
                     end - start)
                     .count()
              << std::endl;
  }

  std::cout << "Model Trained" << std::endl;

  return TSs->GetGlobalID();
}
} // namespace agile::workflow1
