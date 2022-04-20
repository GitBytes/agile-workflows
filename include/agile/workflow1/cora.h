#ifndef AGILE_CORA_H
#define AGILE_CORA_H

#include <string>

#include "torch/torch.h"
#include "torch/data/example.h"

namespace agile {

class CoraDataset : public torch::data::Dataset<CoraDataset> {
private:
  torch::Tensor _edgeIndex;
  torch::Tensor _featureVectors;
  torch::Tensor _labels;

public:
  CoraDataset() = default;
  CoraDataset(const CoraDataset& O)
    : _edgeIndex(O._edgeIndex)
    , _featureVectors(O._featureVectors)
    , _labels(O._labels)
  {}

  CoraDataset(CoraDataset&& O)
    : _edgeIndex(std::move(O._edgeIndex))
    , _featureVectors(std::move(O._featureVectors))
    , _labels(std::move(O._labels))
  {}

  CoraDataset(const std::string &edgeIndex, const std::string& featureVectors,
              const std::string &label);

  CoraDataset & operator=(const CoraDataset& O) {
    this->_edgeIndex = O._edgeIndex;
    this->_featureVectors = O._featureVectors;
    this->_labels = O._labels;
    return *this;
  }
  CoraDataset & operator=(CoraDataset && O) {
    this->_edgeIndex = std::move(O._edgeIndex);
    this->_featureVectors = std::move(O._featureVectors);
    this->_labels = std::move(O._labels);
    return *this;
  }

  torch::data::Example<> get(size_t idx) override;

  torch::Tensor edge_index() const noexcept { return _edgeIndex; }

  void filter(torch::Tensor &mask);

  torch::optional<size_t> size() const override {
    return _featureVectors.size(0);
  }
};

}

#endif
