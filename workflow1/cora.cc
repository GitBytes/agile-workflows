#include <fstream>
#include <string>
#include <map>
#include <tuple>
#include <vector>

#include "agile/workflow1/cora.h"

namespace agile {

torch::Tensor reload(const std::string&path) {
  std::ifstream file(path, std::ios::binary);
  std::vector<char> buffer(std::istreambuf_iterator<char>(file), {});
  return torch::pickle_load(buffer).toTensor();
}

CoraDataset::CoraDataset(const std::string &edgeIndex, const std::string &featureVectors,
                         const std::string &label)
  : _edgeIndex(reload(edgeIndex))
  , _featureVectors(reload(featureVectors))
  , _labels(reload(label)) {}

CoraData<> CoraDataset::get(size_t idx) {
  auto [t, m, v] = _build_ego_graph(idx);
  return { t, _featureVectors.index({m}), _labels.index({m}), v };
}

void CoraDataset::filter(torch::Tensor &mask) {
  _featureVectors = _featureVectors.index({mask});
  _labels = _labels.index({mask});
}

std::tuple<torch::Tensor, torch::Tensor, torch::Tensor> CoraDataset::_build_ego_graph(size_t idx) {
  std::set<int64_t> vertex_set;
  vertex_set.insert(idx);

  for (size_t pos = 0; pos < _edgeIndex.size(1); ++pos) {
    int64_t i = _edgeIndex[0][pos].item<int64_t>();
    int64_t j = _edgeIndex[1][pos].item<int64_t>();

    if (i == idx) vertex_set.insert(j);
    else if (j == idx) vertex_set.insert(i);
  }
  std::vector<int64_t> vertices(vertex_set.begin(), vertex_set.end());

  std::map<int64_t, int64_t> vertex_mapping;
  for (size_t i = 0; i < vertices.size(); ++i) {
    vertex_mapping.insert({vertices[i], i});
  }

  std::vector<int64_t> sources;
  std::vector<int64_t> destinations;

  for (size_t pos = 0; pos < _edgeIndex.size(1); ++pos) {
    int64_t i = _edgeIndex[0][pos].item<int64_t>();
    int64_t j = _edgeIndex[1][pos].item<int64_t>();

    if (vertex_mapping.find(i) != vertex_mapping.end() &&
        vertex_mapping.find(j) != vertex_mapping.end()) {
      sources.push_back(vertex_mapping[i]);
      destinations.push_back(vertex_mapping[j]);
    }
  }
  auto options = torch::TensorOptions().dtype(torch::kLong);
  auto result = torch::zeros({2, sources.size()});
  result.slice(0, 0, 1) = torch::from_blob(sources.data(), {sources.size()}, options).clone();
  result.slice(0, 1, 2) = torch::from_blob(destinations.data(), {destinations.size()}, options).clone();

  auto selected_vertices = torch::from_blob(vertices.data(), {vertices.size()}, options);
  auto bool_tensor = torch::TensorOptions().dtype(torch::kBool);
  auto vertex_mask = torch::zeros(_featureVectors.size(0), bool_tensor).index_put_({selected_vertices}, true);

  auto vertex = torch::zeros(vertices.size(), bool_tensor).index_put_({vertex_mapping[idx]}, true);
  return std::make_tuple(result, vertex_mask, vertex);
}

}
