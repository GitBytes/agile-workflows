#include <cstddef>
#include <cstdint>
#include <torch/csrc/autograd/generated/variable_factories.h>

#include "agile/workflow1/wmd.h"

namespace agile::workflow1 {
WMDData<> WMDDataset::get(size_t idx) {
  auto [t, f, l, m] = _build_ego_graph(int64_t(idx));
  return {t, f, l, m};
}

std::tuple<torch::Tensor, torch::Tensor, torch::Tensor, torch::Tensor>
WMDDataset::_build_ego_graph(int64_t root) {

  shad::rt::Handle handle;
  auto Edges = XEdgeType::GetPtr(_edgesOID);
  auto Vertices = VertexType::GetPtr(_verticesOID);
  auto Features = shad::Array<uint64_t>::GetPtr(_featuresOID);

  using namespace torch::indexing;
  auto bool_tensor = torch::TensorOptions().dtype(torch::kBool);

  uint64_t localID = 0;
  std::deque<uint64_t> frontier;
  std::map<uint64_t, Vertex> vertex_set;
  std::vector<uint64_t> levels{5, 3, 2, 1, 0};              // last 0 required to flush frontier
  std::set<std::pair<uint64_t, uint64_t>> edges;

  Vertex V = Vertices->At(root);                            // get root
  uint64_t V_localID = localID ++;                          // get next local ID

  V.id = V_localID;                                         // assign V a local ID
  vertex_set[root] = V;                                     // insert V into vertex set
  frontier.push_back(root);                                 // push V's global id onto frontier
  edges.insert( std::make_pair(V_localID, V_localID) );     // insert self edge into edge set

  uint64_t level = 0;                                       // BFS controls
  auto next = frontier.begin();
  auto end_of_level = frontier.end();
  uint64_t added_neighbors = 0;
  uint64_t max_neighbors = levels[0];

  while (level < levels.size()) {
    if (next == end_of_level) break;                        // BFS is exhausted

    uint64_t glbID = (* next) ++;                           // advance frontier
    Vertex V = vertex_set[glbID];                           // get next vertex
    uint64_t V_localID = V.id;                              // get V's local id

    uint64_t startEL = V.start;                             // get V's neighbor list
    uint64_t endEL = startEL + V.edges;
    uint64_t num_neighbors = endEL - startEL;
    std::vector<Edge> neighborhood(num_neighbors);
    Edges->AsyncGetElements(handle, neighborhood.data(), startEL, num_neighbors);

    shad::rt::waitForCompletion(handle);

    for (uint64_t i = 0; i < num_neighbors; ++ i) {
      uint64_t glbID = neighborhood[i].dst_glbid;
      Vertex U = Vertices->At(glbID);
        
      if (vertex_set.find(glbID) == vertex_set.end()) {            // U is not visited
         if (added_neighbors < max_neighbors) continue;            // ... if no more neighbors to add, continue

         added_neighbors ++;
         uint64_t U_localID = localID ++;                          // ... get next local id

         U.id = U_localID;                                         // ... assign U a local id
         vertex_set[glbID] = U;                                    // ... insert U into vertex set
         frontier.push_back(glbID);                                // ... push U's global id onto frontier
         edges.insert( std::make_pair(U_localID, U_localID) );     // ... insert self edge into edge set
         edges.insert( std::make_pair(V_localID, U_localID) );     // ... insert V-U edge into edge set
         edges.insert( std::make_pair(U_localID, V_localID) );     // ... insert U-V edge into edge set

      } else {                                                     // U is visited
         uint64_t U_localID = vertex_set[glbID].id;                // ... get U's local id
         edges.insert( std::make_pair(V_localID, U_localID) );     // ... insert V-U edge into edge set
         edges.insert( std::make_pair(U_localID, V_localID) );     // ... insert U-V edge into edge set
    } }

    if (next == end_of_level) {     // go to next level
       level ++;
       added_neighbors = 0;
       max_neighbors = levels[level];
       end_of_level  = frontier.end();
  } }

  int64_t num_edges = edges.size();
  int64_t num_vertices = vertex_set.size();

  // create source and destination vectors
  std::vector<int64_t> sources;
  std::vector<int64_t> destinations;

  for (auto edge : edges) {
    sources.push_back((int64_t) edge.first);
    destinations.push_back((int64_t) edge.second);
  }

  // create type and feature vector
  std::vector<int64_t> vertexTypes(num_vertices);
  std::vector<int64_t> featureVectors(num_vertices * NUM_FEATURES);

  for (auto itr = vertex_set.begin(); itr != vertex_set.end(); ++ itr) {
    int64_t glbID = (* itr).first;
    int64_t localID = (* itr).second.id;
    int64_t type = (int64_t) (* itr).second.type;

    vertexTypes[localID] = type;
    auto ptr = (uint64_t *) (featureVectors.data() + localID * NUM_FEATURES);
    Features->AsyncGetElements(handle, ptr, glbID * NUM_FEATURES, NUM_FEATURES);
  }

  // The result tensor stores the edge list of the ego-graph.
  auto options = torch::TensorOptions().dtype(torch::kLong);
  auto result = torch::zeros({2, num_edges}, options);

  result.slice(0, 0, 1) = torch::from_blob(sources.data(),      {num_edges}, options).clone();
  result.slice(0, 1, 2) = torch::from_blob(destinations.data(), {num_edges}, options).clone();

  // The vertex tensor stores a bitmask representing vertices to be used
  // as part of the training process. We are using roughly 75% of the ego-graph.
  auto vertex = torch::zeros(num_vertices, bool_tensor);
  auto indices = torch::randint(0, num_vertices, {num_vertices - num_vertices / 4}, options);

  vertex.index_put_({0}, true);           // set root's bit to true
  vertex.index_put_({indices}, true);     // set choosen vertices' bits to true
  std::vector<float> floatFeatures(featureVectors.begin(), featureVectors.end());

  shad::rt::waitForCompletion(handle);

  // The features tensor stores the two hop features of the ego-graph vertices
  auto features = torch::from_blob(floatFeatures.data(), {num_vertices, NUM_FEATURES},
                                   torch::TensorOptions().dtype(torch::kFloat)).clone();

  // The labels tensor stores the type of the ego-graph vertices
  auto labels = torch::from_blob(vertexTypes.data(), {num_vertices}, options).clone();

  return std::make_tuple(result, features, labels, vertex);
}
} // namespace agile::workflow1
