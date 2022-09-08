#ifndef AGILE_WORKFLOW1_WMD_H
#define AGILE_WORKFLOW1_WMD_H

#include <cstdint>

#include "shad/data_structures/array.h"
#include "agile/workflow1/graph.h"
#include "torch/torch.h"

#define NUM_FEATURES 30

namespace agile::workflow1 {
template <typename EdgeIndexType = torch::Tensor,
          typename FeaturesType = torch::Tensor,
          typename LabelsType = torch::Tensor,
          typename MaskType = torch::Tensor>
struct WMDData {
  using edge_index_type = EdgeIndexType;
  using features_type = FeaturesType;
  using labels_type = LabelsType;
  using mask_type = MaskType;

  WMDData() = default;
  WMDData(edge_index_type ei, features_type fs, labels_type ls, mask_type mask)
      : EdgeIndex(std::move(ei)), Features(std::move(fs)),
        Labels(std::move(ls)), Mask(std::move(mask)) {}

  edge_index_type EdgeIndex;
  features_type Features;
  labels_type Labels;
  mask_type Mask;
};
} // namespace agile::workflow1

namespace torch::data::transforms {
template <>
struct Stack<agile::workflow1::WMDData<>>
    : public Collation<agile::workflow1::WMDData<>> {
  agile::workflow1::WMDData<>
  apply_batch(std::vector<agile::workflow1::WMDData<>> examples) override {
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

namespace agile::workflow1 {
class WMDDataset  {
public:
  using ArrayOID = typename shad::Array<uint64_t>::ObjectID;

protected:
  VertexOID _verticesOID;
  XEdgeOID _edgesOID;
  ArrayOID _featuresOID;

public:
  using Data = WMDData<>;

  WMDDataset()
      : _verticesOID(VertexOID::kNullID),
        _edgesOID(XEdgeOID::kNullID), _featuresOID(ArrayOID::kNullID) {}

  WMDDataset(const WMDDataset &O)
      : _verticesOID(O._verticesOID),
        _edgesOID(O._edgesOID), _featuresOID(O._featuresOID) {}

  WMDDataset(WMDDataset &&O)
      : _verticesOID(O._verticesOID),
        _edgesOID(O._edgesOID), _featuresOID(O._featuresOID) {}

  WMDDataset &operator=(const WMDDataset &O) {
    this->_verticesOID = O._verticesOID;
    this->_edgesOID = O._edgesOID;
    this->_featuresOID = O._featuresOID;
    return *this;
  }

  WMDDataset(const VertexOID &VertexArrayID, const XEdgeOID &EdgeArrayOID,
             const ArrayOID &FeaturesArrayID)
      :  _verticesOID(VertexArrayID),
        _edgesOID(EdgeArrayOID), _featuresOID(FeaturesArrayID) {}

  WMDDataset &operator=(WMDDataset &&O) {
    this->_verticesOID = O._verticesOID;
    this->_edgesOID = O._edgesOID;
    this->_featuresOID = O._featuresOID;
    return *this;
  }


protected:
  std::tuple<torch::Tensor, torch::Tensor, torch::Tensor, torch::Tensor>
  _build_ego_graph(int64_t *rootB, int64_t *rootE);
};

class VertexClassificationWMDDataset
  : public torch::data::Dataset<VertexClassificationWMDDataset, WMDData<>>,
    public WMDDataset {
public:
  VertexClassificationWMDDataset()
    : WMDDataset() {}

  VertexClassificationWMDDataset(const VertexClassificationWMDDataset &O)
    : WMDDataset(O) {}

  VertexClassificationWMDDataset(VertexClassificationWMDDataset &&O)
    : WMDDataset(O) {}

  VertexClassificationWMDDataset &operator=(const VertexClassificationWMDDataset &O) {
    WMDDataset::operator=(O);
    return *this;
  }

  VertexClassificationWMDDataset(const VertexOID &VertexArrayID,
                                 const XEdgeOID &EdgeArrayOID,
                                 const ArrayOID &FeaturesArrayID)
    :  WMDDataset(VertexArrayID, EdgeArrayOID, FeaturesArrayID) {}

  WMDDataset &operator=(WMDDataset &&O) {
    WMDDataset::operator=(O);
    return *this;
  }

  //! Returns the i-th data point from the dataset.
  //!
  //! The data point returned contains:
  //!  + the ego-graph of the idx vertex;
  //!  + the feature vector of each of the vertices in the ego-graph;
  //!  + the labels for each of the vertices in the ego-graphs;
  //!  + a mask selecting which vertex to use in training;
  //!
  //! Each data point is constructed on the fly by querying the CSR
  //! representation that is built at the beginning of the workflow.
  WMDData<> get(size_t idx) override {
    int64_t root = idx;
    auto [t, f, l, m] = _build_ego_graph(&root, (&root) + 1);
    return {t, f, l, m};
  }

  torch::optional<size_t> size() const override {
    return VertexType::GetPtr(_verticesOID)->Size() - 1;
  }
};

class LinkPredictionWMDDataset
  : public torch::data::Dataset<LinkPredictionWMDDataset, WMDData<>>,
    public WMDDataset {
  LinkPredictionWMDDataset()
    : WMDDataset() {}

  LinkPredictionWMDDataset(const LinkPredictionWMDDataset &O)
    : WMDDataset(O) {}

  LinkPredictionWMDDataset(LinkPredictionWMDDataset &&O)
    : WMDDataset(O) {}

  LinkPredictionWMDDataset &operator=(const LinkPredictionWMDDataset &O) {
    WMDDataset::operator=(O);
    return *this;
  }

  LinkPredictionWMDDataset(const VertexOID &VertexArrayID,
                           const XEdgeOID &EdgeArrayOID,
                           const ArrayOID &FeaturesArrayID)
    :  WMDDataset(VertexArrayID, EdgeArrayOID, FeaturesArrayID) {}

  WMDDataset &operator=(WMDDataset &&O) {
    WMDDataset::operator=(O);
    return *this;
  }

  //! Returns the i-th data point from the dataset.
  //!
  //! The data point returned contains:
  //!  + a subgraph obtained by exploring the neighboorhoods of the (i,j) vertices;
  //!  + the feature vector of each of the vertices in the subgraph;
  //!  + a mask selecting which vertex to use in training;
  //!
  //! Each data point is constructed on the fly by querying the CSR
  //! representation that is built at the beginning of the workflow.
  WMDData<> get(size_t idx) override {
    int64_t n = VertexType::GetPtr(_verticesOID)->Size() - 1;
    int64_t i = idx / n;
    int64_t j = idx % n;
    int64_t root[2] = { i, j };
    auto [t, f, l, m] = _build_ego_graph(root, root + 2);
    return {t, f, l, m};
  }
};
} // namespace agile::workflow1

#endif
