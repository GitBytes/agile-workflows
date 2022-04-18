#include <fstream>
#include <string>
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

torch::data::Example<> CoraDataset::get(size_t idx) {
  return { _featureVectors[idx], _labels[idx] };
}

void CoraDataset::filter(torch::Tensor &mask) {
  _featureVectors = _featureVectors.index({mask});
  _labels = _labels.index({mask});
}

}
