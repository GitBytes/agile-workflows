#include "agile/workflow1/graph.h"
#include "agile/workflow1/main.h"
#include "agile/workflow1/wmd.h"

#define NUM_FEATURES 22

namespace agile::workflow1 {
using Emb_t = shad::Array<uint64_t>;
using EmbeddingType = shad::Array<uint64_t>;
using EmbeddingOID = shad::ObjectIdentifier<EmbeddingType>;

struct Args_t {
  uint64_t edgesOID;
  uint64_t verticesOID;
  uint64_t embeddingsOID;
};

// Aggregate neigbors histograms ... store into second half of the feature
// vector
void TwoHopFeatures(Handle &handle, const uint64_t ndx, Vertex &vertex,
                    Args_t &args) {
  auto Edges = EdgeType::GetPtr((EdgeOID)args.edgesOID);
  auto Vertices = VertexType::GetPtr((VertexOID)args.verticesOID);
  auto Embeddings = EmbeddingType::GetPtr((EmbeddingOID)args.embeddingsOID);

  Vertex nextVertex = Vertices->At(ndx + 1);
  uint64_t num_edges = nextVertex.edges - vertex.edges;
  if (num_edges == 0)
    return;

  Handle my_handle;
  std::vector<Edge> edges(num_edges);
  Edges->AsyncGetElements(my_handle, edges.data(), vertex.edges, num_edges);

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
  auto Edges = EdgeType::GetPtr((EdgeOID)args.edgesOID);
  auto Vertices = VertexType::GetPtr((VertexOID)args.verticesOID);
  auto Embeddings = EmbeddingType::GetPtr((EmbeddingOID)args.embeddingsOID);

  Vertex nextVertex = Vertices->At(ndx + 1);
  uint64_t num_edges = nextVertex.edges - vertex.edges;
  if (num_edges == 0)
    return;

  Handle my_handle;
  std::vector<Edge> edges(num_edges);
  Edges->AsyncGetElements(my_handle, edges.data(), vertex.edges, num_edges);

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

void GNN(uint64_t &num_edges, uint64_t &num_vertices, Graph_t &graph) {
  Handle handle;
  auto Vertices = VertexType::GetPtr((VertexOID)graph["Vertices"]);
  auto Embeddings = EmbeddingType::Create(num_vertices * NUM_FEATURES, 0);

  Embeddings->FillPtrs();
  graph["Embeddings"] = (uint64_t)(Embeddings->GetGlobalID());
  Args_t args = {graph["Edges"], graph["Vertices"], graph["Embeddings"]};

  Vertices->AsyncForEachInRange(handle, 0, num_vertices, OneHopFeatures, args);
  waitForCompletion(handle);

  Vertices->AsyncForEachInRange(handle, 0, num_vertices, TwoHopFeatures, args);
  waitForCompletion(handle);

  size_t parallelThreads = shad::rt::numLocalities();
  TrainingState<WMDDataset> initState;
  auto TSs shad::Array<TrainingState<WMDDataset>>::Create(parallelThreads,
                                                          initState);

  SetUpTrainingContext<WMDDataset> setup(graph["Vertices"], graph["Edges"],
                                         graph["Embeddings"]);

  std::for_each(shad::distributed_parallel_tag{}, TSs->begin(), TSs->end(),
                vcTrainLoop<TrainingState<WMDDataset>>);
}
} // namespace agile::workflow1
