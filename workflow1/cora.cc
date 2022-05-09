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
                         const std::string &label, torch::Tensor levels)
  : _edgeIndex(reload(edgeIndex))
  , _featureVectors(reload(featureVectors))
  , _labels(reload(label))
  , _levels(std::move(levels)) {
  _CSR_idx = torch::zeros(_featureVectors.size(0) + 1, torch::TensorOptions().dtype(torch::kLong));

  for (int i = 0; i < _edgeIndex[0].size(0); ++i) {
    _CSR_idx[_edgeIndex[0][i] + 1] += 1;
  }

  for (int i = 1; i < _CSR_idx.size(0); ++i) {
    _CSR_idx[i] += _CSR_idx[i-1];
  }

  auto [sorted, index] = torch::sort(_edgeIndex[0]);
  _edgeIndex = torch::stack({sorted, _edgeIndex[1].index({index})}, 0).contiguous();
}

CoraData<> CoraDataset::get(size_t idx) {
  auto [t, m, v] = _build_ego_graph(idx);
  return { t, _featureVectors.index({m}), _labels.index({m}), v };
}

void CoraDataset::filter(torch::Tensor &mask) {
  _featureVectors = _featureVectors.index({mask});
  _labels = _labels.index({mask});
}

std::tuple<torch::Tensor, torch::Tensor, torch::Tensor> CoraDataset::_build_ego_graph(size_t idx) {
  using namespace torch::indexing;
  auto bool_tensor = torch::TensorOptions().dtype(torch::kBool);
  auto vertex_mask = torch::zeros(_featureVectors.size(0), bool_tensor);

  std::vector<int64_t> sources;
  std::vector<int64_t> destinations;
  std::deque<int64_t> frontier({idx});
  std::set<int64_t> vertex_set;
  int64_t level = 0;
  int64_t position = 0;
  auto next = frontier.begin();
  auto end_of_level = frontier.end();
  while (level < 3 && next != end_of_level) {
    auto v = *next++;
    if (vertex_mask[v].item<bool>() == false) {
      vertex_mask[v] = true;
      vertex_set.insert(v);
      auto neighborhood =
        _edgeIndex[1].slice(0, _CSR_idx.index({v}).item<int64_t>(),
                            _CSR_idx.index({int64_t(v + 1)}).item<int64_t>())
        .slice(0, 0, _levels[level].item<int64_t>());

      for (int i = 0; i < neighborhood.size(0); ++i) {
        auto u = neighborhood[i].item<int64_t>();
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

  for (auto itr = vertex_set.begin(); itr != vertex_set.end(); ++itr) {
    auto neghborhood =
      _edgeIndex[1].slice(0, _CSR_idx.index({*itr}).item<int64_t>(),
                          _CSR_idx.index({*itr + 1}).item<int64_t>());

    for (int64_t i = 0; i < neghborhood.size(0); ++i) {
      int64_t v = neghborhood[i].item<int64_t>();
      if (vertex_mask[v].item<bool>()) {
        sources.push_back(vertex_mapping[*itr]);
        destinations.push_back(vertex_mapping[v]);
      }
    }
  }

  auto options = torch::TensorOptions().dtype(torch::kLong);
  auto result = torch::zeros({2, sources.size()});
  result.slice(0, 0, 1) = torch::from_blob(sources.data(), {sources.size()}, options).clone();
  result.slice(0, 1, 2) = torch::from_blob(destinations.data(), {destinations.size()}, options).clone();

  // auto vertex = torch::zeros(vertex_set.size(), bool_tensor).index_put_({vertex_mapping[idx]}, true);
  auto vertex = torch::zeros(vertex_set.size(), bool_tensor);
  auto indices = torch::randint(0, vertex_set.size(), {vertex_set.size() - vertex_set.size() / 4}, options);
  vertex.index_put_({indices}, true);
  vertex.index_put_({vertex_mapping[idx]}, true);

  return std::make_tuple(result, vertex_mask, vertex);
}

}
