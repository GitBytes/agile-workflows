#include "agile/workflow1/main.h"
#include "agile/workflow1/graph.h"

#define NUM_FEATURES 11

namespace agile::workflow1 {
using Emb_t = shad::Array<uint64_t>;
using EmbeddingType = shad::Array<uint64_t>;
using EmbeddingOID  = shad::ObjectIdentifier<EmbeddingType>;


struct Args_t {
  uint64_t EdgesOID;
  uint64_t VerticesOID;
  uint64_t EmbeddingsOID;
};


// Histogram of two hop edge and neigbor vertex types
void TwoHopFeatures(Handle & handle, const uint64_t ndx, Vertex & vertex, Args_t & args) {
  auto Edges = EdgeType::GetPtr((EdgeOID) args.EdgesOID);
  auto Vertices = VertexType::GetPtr((VertexOID) args.VerticesOID);
  auto Embeddings = EmbeddingType::GetPtr((EmbeddingOID) args.EmbeddingsOID);

  Vertex nextVertex = Vertices->At(ndx + 1);
  uint64_t num_edges = nextVertex.edges - vertex.edges;
  if (num_edges == 0) return;

  Handle my_handle;
  std::vector<Edge> edges(num_edges);
  std::vector<uint64_t> features(NUM_FEATURES, 0);

  Edges->AsyncGetElements(my_handle, edges.data(), vertex.edges, num_edges);
  waitForCompletion(my_handle);

  for (auto & edge : edges) {
       features[ (uint64_t) edge.type ] ++;
       features[ (uint64_t) edge.dst_type ] ++;
  }

  Embeddings->AsyncInsertAt(handle, ndx * NUM_FEATURES, features.data(), NUM_FEATURES);
}


void GNN(uint64_t & num_edges, uint64_t & num_vertices, Graph_t & graph) {
  Handle handle;
  auto Vertices = VertexType::GetPtr((VertexOID) graph["Vertices"]);
  auto Embeddings = EmbeddingType::Create(num_vertices * NUM_FEATURES, 0);

  Embeddings->FillPtrs();
  graph["Embeddings"] = (uint64_t) (Embeddings->GetGlobalID());

  Args_t args;
  args.EdgesOID = graph["Edges"];
  args.VerticesOID = graph["Vertices"];
  args.EmbeddingsOID = graph["Embeddings"];

  Vertices->AsyncForEachInRange(handle, 0, num_vertices, TwoHopFeatures, args);
  waitForCompletion(handle);
}
}
