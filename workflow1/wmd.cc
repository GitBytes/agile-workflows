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
WMDDataset::_build_ego_graph(int64_t idx) {
  auto EdgesPtr = EdgeType::GetPtr(_edgesOID);
  auto VerticesPtr = VertexType::GetPtr(_verticesOID);

  using namespace torch::indexing;
  auto bool_tensor = torch::TensorOptions().dtype(torch::kBool);
  auto vertex_mask = torch::zeros(VerticesPtr->Size() - 1, bool_tensor);

  std::vector<int64_t> sources;
  std::vector<int64_t> destinations;
  std::deque<int64_t> frontier({idx});
  std::set<int64_t> vertex_set;
  int64_t level = 0;
  int64_t position = 0;
  auto next = frontier.begin();
  auto end_of_level = frontier.end();
  while (level < _levels.size(0) && next != end_of_level) {
    auto v = *next++;
    if (vertex_mask[v].item<bool>() == false) {
      vertex_mask[v] = true;
      vertex_set.insert(v);

      int startEL = VerticesPtr->At(v).edges;
      int endEL = VerticesPtr->At(v + 1).edges;

      int num_neighbors =
          std::min<int>(endEL - startEL, _levels[level].item<int64_t>());
      std::vector<Edge> neighborhood(num_neighbors);

      shad::rt::Handle h;
      EdgesPtr->AsyncGetElements(
          h, neighborhood.data(), startEL,
          num_neighbors);
      shad::rt::waitForCompletion(h);

      for (int i = 0; i < neighborhood.size(); ++i) {
        auto u = neighborhood[i].dst;
        frontier.push_back(u);
      }
    }

    if (next == end_of_level) {
      end_of_level = frontier.end();
      level += 1;
    }
  }

  std::map<int64_t, int64_t> vertex_mapping;
  int64_t id = 0;
  for (auto itr = vertex_set.begin(); itr != vertex_set.end(); ++itr, ++id) {
    vertex_mapping.insert({*itr, id});
  }

  std::vector<int64_t> vertexTypes(vertex_mapping.size());
  for (auto itr = vertex_set.begin(); itr != vertex_set.end(); ++itr) {
    auto ThisVertex = VerticesPtr->At(*itr);

    vertexTypes[vertex_mapping[*itr]] = int64_t(ThisVertex.type);

    int startEL = ThisVertex.edges;
    int endEL = VerticesPtr->At(*itr + 1).edges;

    int num_neighbors = endEL - startEL;
    std::vector<Edge> neighborhood(num_neighbors);

    shad::rt::Handle h;
    EdgesPtr->AsyncGetElements(
        h, neighborhood.data(), startEL,
        num_neighbors);
    shad::rt::waitForCompletion(h);

    for (int64_t i = 0; i < neighborhood.size(); ++i) {
      int64_t v = neighborhood[i].dst;
      if (vertex_mask[v].item<bool>()) {
        sources.push_back(vertex_mapping[*itr]);
        destinations.push_back(vertex_mapping[v]);
      }
    }
  }

  auto options = torch::TensorOptions().dtype(torch::kLong);
  std::vector<int64_t> featureVectors(vertex_mapping.size() * NumFeauters);
  shad::rt::Handle h;

  auto FeaturesPtr = shad::Array<uint64_t>::GetPtr(_featuresOID);
  for (auto &kv : vertex_mapping) {
    auto key = std::get<0>(kv);
    auto value = std::get<1>(kv);
    FeaturesPtr->AsyncGetElements(
        h,
        reinterpret_cast<uint64_t *>(featureVectors.data() +
                                     value * NumFeauters),
        key * NumFeauters, NumFeauters);
  }

  // The result tensor stores the edge list of the ego-graph.
  auto result = torch::zeros({2, sources.size()});
  result.slice(0, 0, 1) =
      torch::from_blob(sources.data(), {sources.size()}, options).clone();
  result.slice(0, 1, 2) =
      torch::from_blob(destinations.data(), {destinations.size()}, options)
          .clone();

  // The vertex tensor stores a bitmaks representing vertices to be used
  // as part of the training process. We are using roughly 75% of the ego-graph.
  auto vertex = torch::zeros(vertex_set.size(), bool_tensor);
  auto indices =
      torch::randint(0, vertex_set.size(),
                     {vertex_set.size() - vertex_set.size() / 4}, options);
  vertex.index_put_({indices}, true);
  vertex.index_put_({vertex_mapping[idx]}, true);

  shad::rt::waitForCompletion(h);

  // The features as computed by the TwoHopFeatures function.
  auto features = torch::from_blob(featureVectors.data(),
                                   {vertex_set.size(), NumFeauters}, options).clone();
  // The vertex type as for each of the vertices in the ego-graph.
  auto labels =  torch::from_blob(vertexTypes.data(),
                                  {vertex_set.size()}, options).clone();

  return std::make_tuple(result, features, labels, vertex);
}
} // namespace agile
