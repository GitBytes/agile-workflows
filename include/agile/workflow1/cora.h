//===------------------------------------------------------------*- C++ -*-===//
//
//                            The AGILE Workflows
//
//===----------------------------------------------------------------------===//
// ** Pre-Copyright Notice
//
// This computer software was prepared by Battelle Memorial Institute,
// hereinafter the Contractor, under Contract No. DE-AC05-76RL01830 with the
// Department of Energy (DOE). All rights in the computer software are reserved
// by DOE on behalf of the United States Government and the Contractor as
// provided in the Contract. You are authorized to use this computer software
// for Governmental purposes but it is not to be released or distributed to the
// public. NEITHER THE GOVERNMENT NOR THE CONTRACTOR MAKES ANY WARRANTY, EXPRESS
// OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE. This
// notice including this sentence must appear on any copies of this computer
// software.
//
// ** Disclaimer Notice
//
// This material was prepared as an account of work sponsored by an agency of
// the United States Government. Neither the United States Government nor the
// United States Department of Energy, nor Battelle, nor any of their employees,
// nor any jurisdiction or organization that has cooperated in the development
// of these materials, makes any warranty, express or implied, or assumes any
// legal liability or responsibility for the accuracy, completeness, or
// usefulness or any information, apparatus, product, software, or process
// disclosed, or represents that its use would not infringe privately owned
// rights. Reference herein to any specific commercial product, process, or
// service by trade name, trademark, manufacturer, or otherwise does not
// necessarily constitute or imply its endorsement, recommendation, or favoring
// by the United States Government or any agency thereof, or Battelle Memorial
// Institute. The views and opinions of authors expressed herein do not
// necessarily state or reflect those of the United States Government or any
// agency thereof.
//
//                    PACIFIC NORTHWEST NATIONAL LABORATORY
//                                 operated by
//                                   BATTELLE
//                                   for the
//                      UNITED STATES DEPARTMENT OF ENERGY
//                       under Contract DE-AC05-76RL01830
//===----------------------------------------------------------------------===//

#ifndef AGILE_CORA_H
#define AGILE_CORA_H

#include <string>
#include <tuple>

#include "torch/data/transforms/stack.h"
#include "torch/torch.h"

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
      : EdgeIndex(std::move(ei)), Features(std::move(fs)),
        Labels(std::move(ls)), Mask(std::move(mask)) {}

  edge_index_type EdgeIndex;
  features_type Features;
  labels_type Labels;
  mask_type Mask;
};
} // namespace agile

namespace torch::data::transforms {
template <>
struct Stack<agile::CoraData<>> : public Collation<agile::CoraData<>> {
  agile::CoraData<>
  apply_batch(std::vector<agile::CoraData<>> examples) override {
    int64_t offset = 0;

    std::vector<torch::Tensor> ei, fs, ls, ms;

    for (size_t i = 0; i < examples.size(); ++i) {
      ei.push_back(examples[i].EdgeIndex.add(offset));
      fs.push_back(examples[i].Features);
      ls.push_back(examples[i].Labels);
      ms.push_back(examples[i].Mask);

      offset += examples[i].Features.size(0);
    }

    return {torch::cat(ei, 1).to(torch::kLong), torch::cat(fs), torch::cat(ls),
            torch::cat(ms)};
  }
};
} // namespace torch::data::transforms

namespace agile {
class CoraDataset : public torch::data::Dataset<CoraDataset, CoraData<>> {
private:
  torch::Tensor _edgeIndex;
  torch::Tensor _featureVectors;
  torch::Tensor _labels;
  torch::Tensor _CSR_idx;
  torch::Tensor _levels;

public:
  using Data = CoraData<>;

  CoraDataset() = default;
  CoraDataset(const CoraDataset &O)
      : _edgeIndex(O._edgeIndex), _featureVectors(O._featureVectors),
        _labels(O._labels), _CSR_idx(O._CSR_idx), _levels(O._levels) {}

  CoraDataset(CoraDataset &&O)
      : _edgeIndex(std::move(O._edgeIndex)),
        _featureVectors(std::move(O._featureVectors)),
        _labels(std::move(O._labels)), _CSR_idx(std::move(O._CSR_idx)),
        _levels(std::move(O._levels)) {}

  CoraDataset(const std::string &edgeIndex, const std::string &featureVectors,
              const std::string &label, torch::Tensor levels);

  CoraDataset &operator=(const CoraDataset &O) {
    this->_edgeIndex = O._edgeIndex;
    this->_featureVectors = O._featureVectors;
    this->_labels = O._labels;
    this->_CSR_idx = O._CSR_idx.clone();
    this->_levels = O._levels.clone();
    return *this;
  }
  CoraDataset &operator=(CoraDataset &&O) {
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
  std::tuple<torch::Tensor, torch::Tensor, torch::Tensor>
  _build_ego_graph(int64_t idx);
};

} // namespace agile

#endif
