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
  CoraDataset(const std::string &edgeIndex, const std::string& featureVectors,
              const std::string &label);

  torch::data::Example<> get(size_t idx) override;

  torch::Tensor edge_index() const noexcept { return _edgeIndex; }

  void filter(torch::Tensor &mask);

  torch::optional<size_t> size() const override {
    return _featureVectors.size(0);
  }
};

}

#endif
