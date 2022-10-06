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

  size_t parallelThreads = shad::rt::numLocalities();
  TrainingState<VertexClassificationWMDDataset> initState;
  auto TSs = shad::Array<TrainingState<VertexClassificationWMDDataset>>::Create(
      parallelThreads, initState);
  auto reducerArrayOID =
      shad::Array<uint64_t>::Create(shad::rt::numLocalities(), 0ul)
          ->GetGlobalID();
  SetUpTrainingContext<VertexClassificationWMDDataset> setup(
      Vertices->GetGlobalID(), (XEdgeOID)graph["XEdges"],
      Embeddings->GetGlobalID(), reducerArrayOID, modelFileName);
  shad::for_each(shad::distributed_parallel_tag{}, TSs->begin(), TSs->end(),
                 setup);

  std::cout << "Initialized Training State" << std::endl;

  const size_t numEpochs = 200;
  for (size_t epoch = 0; epoch < numEpochs; ++epoch) {
    auto start = std::chrono::high_resolution_clock::now();
    shad::for_each(shad::distributed_parallel_tag{}, TSs->begin(), TSs->end(),
                   agile::workflow1::vcTrainLoop<
                       TrainingState<VertexClassificationWMDDataset>>);

    vcReduceGradients<TrainingState<VertexClassificationWMDDataset>>(
        TSs->begin(), TSs->end());

    shad::for_each(shad::distributed_parallel_tag{}, TSs->begin(), TSs->end(),
                   agile::workflow1::vcBackPropAndEvaluationLoop<
                       TrainingState<VertexClassificationWMDDataset>>);
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
