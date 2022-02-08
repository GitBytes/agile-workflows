#include "agile/workflow1/main.h"

#define NUM_FEATURES 11

namespace agile::workflow1 {
using Emb_t = shad::Array<uint64_t>;

struct Args_1_t {
  EdgeType::SharedPtr Edges;
  VertexType::SharedPtr Vertices;
  Emb_t::ShadArrayPtr embeddings;
};

void TwoHopFeatures(const uint64_t & seed, std::vector<Edge> & edges, Args_1_t & args) {
  auto Edges = args.Edges;
  auto Vertices = args.Vertices;
  std::array<uint64_t, NUM_FEATURES> features = {0};

  for (auto & edge : edges) {
    Vertex neighbor;
    Vertices->Lookup(edge.get_dst(), & neighbor);     // get neighbor vertex

    features[ (uint64_t) edge.get_type() ] ++;        // increment edge type feature
    features[ (uint64_t) neighbor.get_type() ] ++;    // increment vertex type feature

     EdgeType::LookupResult neighborEdges;
     Edges->Lookup(edge.get_dst(), & neighborEdges);     // get neighbor's edges

    for (auto & edge : neighborEdges.value) {
      Vertex neighbor;
      Vertices->Lookup(edge.get_dst(), & neighbor);     // get neighbor's neighbor vertex

      features[ (uint64_t) edge.get_type() ] ++;        // increment neighbor's edge type feature
      features[ (uint64_t) neighbor.get_type() ] ++;    // increment neighbor's vertex type feature
} } }


void GNN(Graph_t & graph) {
  auto Edges = EdgeType::GetPtr( (EdgeOID) graph[TYPES::EDGE] );
  auto Vertices = VertexType::GetPtr( (VertexOID) graph[TYPES::VERTEX] );
  auto embeddings = Emb_t::Create(Edges->Size() * NUM_FEATURES, 0);

  Args_1_t args_1 = {Edges, Vertices, embeddings};
  Edges->ForEachEntry(TwoHopFeatures, args_1);
}
}
