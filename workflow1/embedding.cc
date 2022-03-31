#include "agile/workflow1/main.h"
#include "agile/workflow1/graph.h"

#define NUM_FEATURES 22

namespace agile::workflow1 {
using Emb_t = shad::Array<uint64_t>;
using EmbeddingType = shad::Array<uint64_t>;
using EmbeddingOID  = shad::ObjectIdentifier<EmbeddingType>;


struct Args_t {
  uint64_t EdgesOID;
  uint64_t VerticesOID;
  uint64_t EmbeddingsOID;
};

void TwoHopFeatures(const uint64_t & ndx, Vertex & vertex, Args_t & args) {
  auto Edges = args.Edges;
  auto Vertices = args.Vertices;
  Vertex nextVertex = Vertices->At(ndx + 1);
  uint64_t num_edges = nextVertex.edges - vertex.edges;

  shad::rt::Handle handle;
  std::vector<Edge> edges(num_edges);
  std::vector<uint64_t> features(NUM_FEATURES, 0);
  Edges->AsyncGetElements(handle, vertex.edges, edges.data(), num_edges);

  for (auto & edge : edges) {
    feature[ (uint64_t) edge.type ] ++;
    Vertex neighbor;
    neighbor = Vertices->At(edge.dst);             // get neighbor vertex

    features[ (uint64_t) edge.type ] ++;           // increment edge type feature
    features[ (uint64_t) neighbor.type ] ++;       // increment vertex type feature

     EdgeType::LookupResult neighborEdges;
     Edges->At(edge.dst, & neighborEdges);         // get neighbor's edges

    for (auto & edge : neighborEdges.value) {
      Vertex neighbor;
      Vertices->At(edge.dst, & neighbor);          // get neighbor's neighbor vertex

      features[ (uint64_t) edge.type ] ++;         // increment neighbor's edge type feature
      features[ (uint64_t) neighbor.type ] ++;     // increment neighbor's vertex type feature
} } }


void GNN(Graph_t & graph) {
  auto Embeddings = EmbeddingType::Create(Vertices->Size() * NUM_FEATURES, 0);

  Embeddings->FillPtrs();
  graph["Embeddings"] = (uint64_t) (Embeddings->GetGlobalID());

  Args_t args;
  args.Edges = graph["Edges"];
  args.Vertices = graph["Vertices"];
  args.Embeddings = graph["Embeddings"];

  Vertices->ForEachEntry(TwoHopFeatures, args_1);
}
}
