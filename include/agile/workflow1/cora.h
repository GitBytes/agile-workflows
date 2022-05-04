#ifndef AGILE_CORA_H
#define AGILE_CORA_H

#include <string>
#include <tuple>

#include "torch/torch.h"
#include "torch/data/transforms/stack.h"


namespace agile {

template <typename EdgeIndexType = torch::Tensor,
          typename FeaturesType = torch::Tensor,
          typename LabelsType = torch::Tensor,
          typename MaskType = torch::Tensor>
struct CoraData {
  using edge_index_type = EdgeIndexType;
  using features_type = FeaturesType;
  using labels_type = LabelsType;
  using mask_type = MaskType;

  CoraData() = default;
  CoraData(edge_index_type ei, features_type fs, labels_type ls, mask_type mask)
    : EdgeIndex(std::move(ei))
    , Features(std::move(fs))
    , Labels(std::move(ls))
    , Mask(std::move(mask))
  {}

  edge_index_type EdgeIndex;
  features_type Features;
  labels_type Labels;
  mask_type Mask;
};
}

namespace torch::data::transforms {
template <>
struct Stack<agile::CoraData<>> : public Collation<agile::CoraData<>> {
  agile::CoraData<> apply_batch(std::vector<agile::CoraData<>> examples) override {
    int64_t offset = 0;

    std::vector<torch::Tensor> ei, fs, ls, ms;

    for (size_t i = 0; i < examples.size(); ++i) {
      ei.push_back(examples[i].EdgeIndex.add(offset));
      fs.push_back(examples[i].Features);
      ls.push_back(examples[i].Labels);
      ms.push_back(examples[i].Mask);

      offset += examples[i].Features.size(0);
    }

    return {torch::cat(ei, 1).to(torch::kLong),
      torch::cat(fs), torch::cat(ls), torch::cat(ms)};
  }
};
}

namespace agile {
class CoraDataset : public torch::data::Dataset<CoraDataset, CoraData<>> {
private:
  torch::Tensor _edgeIndex;
  torch::Tensor _featureVectors;
  torch::Tensor _labels;
  torch::Tensor _CSR_idx;
  torch::Tensor _levels;

public:
  CoraDataset() = default;
  CoraDataset(const CoraDataset& O)
    : _edgeIndex(O._edgeIndex)
    , _featureVectors(O._featureVectors)
    , _labels(O._labels)
    , _CSR_idx(O._CSR_idx)
    , _levels(O._levels)
  {}

  CoraDataset(CoraDataset&& O)
    : _edgeIndex(std::move(O._edgeIndex))
    , _featureVectors(std::move(O._featureVectors))
    , _labels(std::move(O._labels))
    , _CSR_idx(std::move(O._CSR_idx))
    , _levels(std::move(O._levels))
  {}

  CoraDataset(const std::string &edgeIndex, const std::string& featureVectors,
              const std::string &label, torch::Tensor levels);

  CoraDataset & operator=(const CoraDataset& O) {
    this->_edgeIndex = O._edgeIndex;
    this->_featureVectors = O._featureVectors;
    this->_labels = O._labels;
    this->_CSR_idx = O._CSR_idx.clone();
    this->_levels = O._levels.clone();
    return *this;
  }
  CoraDataset & operator=(CoraDataset && O) {
    this->_edgeIndex = std::move(O._edgeIndex);
    this->_featureVectors = std::move(O._featureVectors);
    this->_labels = std::move(O._labels);
    this->_CSR_idx = std::move(O._CSR_idx);
    this->_levels = std::move(O._levels);
    return *this;
  }

  CoraData<> get(size_t idx) override;


  torch::Tensor edge_index() const noexcept { return _edgeIndex; }

  void filter(torch::Tensor &mask);

  torch::optional<size_t> size() const override {
    return _featureVectors.size(0);
  }
private:
  std::tuple<torch::Tensor, torch::Tensor, torch::Tensor> _build_ego_graph(size_t idx);
};

}

#endif
