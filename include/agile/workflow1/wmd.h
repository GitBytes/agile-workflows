/*===------------------------------------------------------------*- C++ -*-===
 *
 *                            The AGILE Workflows
 *
 *===----------------------------------------------------------------------===
 *
 * Copyright (c) 2025 Battelle Memorial Institute
 *
 * Battelle Memorial Institute (hereinafter Battelle) hereby grants permission
 * to any person or entity lawfully obtaining a copy of this software and
 * associated documentation files (hereinafter “the Software”) to redistribute
 * and use the Software in source and binary forms, with or without
 * modification. Such person or entity may use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and may permit
 * others to do so, subject to the following conditions:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimers.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Other than as used herein, neither the name Battelle Memorial Institute or
 *    Battelle may be used in any form whatsoever without the express written
 *    consent of Battelle.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL BATTELLE OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *===----------------------------------------------------------------------===*/
#ifndef AGILE_WORKFLOW1_WMD_H
#define AGILE_WORKFLOW1_WMD_H

#include <chrono>
#include <cstdint>

#include "agile/workflow1/graph.h"
#include "shad/data_structures/array.h"
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
  WMDData(edge_index_type ei, features_type fs, labels_type ls, mask_type mask, mask_type bm=torch::Tensor())
      : EdgeIndex(std::move(ei)), Features(std::move(fs)),
        Labels(std::move(ls)), Mask(std::move(mask)), Batch_Mask(std::move(bm)) {}

  edge_index_type EdgeIndex;
  features_type Features;
  labels_type Labels;
  mask_type Mask;
  mask_type Batch_Mask;
};
} // namespace agile::workflow1

namespace torch::data::transforms {
template <>
struct Stack<agile::workflow1::WMDData<>>
    : public Collation<agile::workflow1::WMDData<>> {
  agile::workflow1::WMDData<>
  apply_batch(std::vector<agile::workflow1::WMDData<>> examples) override {
    int64_t offset = 0;

    std::vector<torch::Tensor> ei, fs, ls, ms, bm;

    for (size_t i = 0; i < examples.size(); ++i) {
      ei.push_back(examples[i].EdgeIndex.add(offset));
      fs.push_back(examples[i].Features);
      ls.push_back(examples[i].Labels);
      ms.push_back(examples[i].Mask);
      bm.push_back(torch::full({examples[i].Features.size(0)},int(i)));
      offset += examples[i].Features.size(0);
    }
    return {torch::cat(ei, 1).to(torch::kLong), torch::cat(fs), torch::cat(ls),
      torch::cat(ms), torch::cat(bm)};
  }
};
} // namespace torch::data::transforms

namespace agile::workflow1 {
class WMDDataset {
public:
  using ArrayOID = typename shad::Array<uint64_t>::ObjectID;

protected:
  VertexOID _verticesOID;
  XEdgeOID _edgesOID;
  ArrayOID _featuresOID;

public:
  using Data = WMDData<>;

  WMDDataset()
      : _verticesOID(VertexOID::kNullID), _edgesOID(XEdgeOID::kNullID),
        _featuresOID(ArrayOID::kNullID) {}

  WMDDataset(const WMDDataset &O)
      : _verticesOID(O._verticesOID), _edgesOID(O._edgesOID),
        _featuresOID(O._featuresOID) {}

  WMDDataset(WMDDataset &&O)
      : _verticesOID(O._verticesOID), _edgesOID(O._edgesOID),
        _featuresOID(O._featuresOID) {}

  WMDDataset &operator=(const WMDDataset &O) {
    this->_verticesOID = O._verticesOID;
    this->_edgesOID = O._edgesOID;
    this->_featuresOID = O._featuresOID;
    return *this;
  }

  WMDDataset(const VertexOID &VertexArrayID, const XEdgeOID &EdgeArrayOID,
             const ArrayOID &FeaturesArrayID)
      : _verticesOID(VertexArrayID), _edgesOID(EdgeArrayOID),
        _featuresOID(FeaturesArrayID) {}

  WMDDataset &operator=(WMDDataset &&O) {
    this->_verticesOID = O._verticesOID;
    this->_edgesOID = O._edgesOID;
    this->_featuresOID = O._featuresOID;
    return *this;
  }

protected:
  std::tuple<torch::Tensor, std::map<uint64_t, Vertex>>
  _build_ego_graph(int64_t *rootB, int64_t *rootE);
};

class VertexClassificationWMDDataset
    : public torch::data::Dataset<VertexClassificationWMDDataset, WMDData<>>,
      public WMDDataset {
public:
  VertexClassificationWMDDataset() : WMDDataset() {}

  VertexClassificationWMDDataset(const VertexClassificationWMDDataset &O)
      : WMDDataset(O) {}

  VertexClassificationWMDDataset(VertexClassificationWMDDataset &&O)
      : WMDDataset(O) {}

  VertexClassificationWMDDataset &
  operator=(const VertexClassificationWMDDataset &O) {
    WMDDataset::operator=(O);
    return *this;
  }

  VertexClassificationWMDDataset(const VertexOID &VertexArrayID,
                                 const XEdgeOID &EdgeArrayOID,
                                 const ArrayOID &FeaturesArrayID)
      : WMDDataset(VertexArrayID, EdgeArrayOID, FeaturesArrayID) {}

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
    auto bool_tensor = torch::TensorOptions().dtype(torch::kBool);
    auto options = torch::TensorOptions().dtype(torch::kLong);

    auto [graph, vertex_set] = _build_ego_graph(&root, (&root) + 1);

    int64_t num_vertices = vertex_set.size();

    // create type and feature vector
    std::vector<int64_t> vertexTypes(num_vertices);
    std::vector<int64_t> featureVectors(num_vertices * NUM_FEATURES);

    shad::rt::Handle handle;
    auto Features = shad::Array<uint64_t>::GetPtr(_featuresOID);
    for (auto itr = vertex_set.begin(); itr != vertex_set.end(); ++itr) {
      int64_t glbID = (*itr).first;
      int64_t localID = (*itr).second.id;
      int64_t type = (int64_t)(*itr).second.type;

      vertexTypes[localID] = type;
      auto ptr = (uint64_t *)(featureVectors.data() + localID * NUM_FEATURES);
      Features->AsyncGetElements(handle, ptr, glbID * NUM_FEATURES,
                                 NUM_FEATURES);
    }

    // The vertex tensor stores a bitmask representing vertices to be used
    // as part of the training process. We are using roughly 75% of the
    // ego-graph.
    auto vertex = torch::zeros(num_vertices, bool_tensor);
    // auto indices = torch::randint(0, num_vertices,
    //                               {num_vertices - num_vertices / 4},
    //                               options);

    vertex.index_put_({0}, true); // set root's bit to true
    // vertex.index_put_({indices}, true); // set choosen vertices' bits to true

    shad::rt::waitForCompletion(handle);     // wait for feature vector data to arrive
    std::vector<float> floatFeatures(featureVectors.begin(),
                                     featureVectors.end());

    // The features tensor stores the two hop features of the ego-graph vertices
    auto features =
        torch::from_blob(floatFeatures.data(), {num_vertices, NUM_FEATURES},
                         torch::TensorOptions().dtype(torch::kFloat))
            .clone();

    // The labels tensor stores the type of the ego-graph vertices
    auto labels =
        torch::from_blob(vertexTypes.data(), {num_vertices}, options).clone();
    return {graph, features, labels, vertex};
  }

  torch::optional<size_t> size() const override {
    return VertexType::GetPtr(_verticesOID)->Size() - 1;
  }
};

class LinkPredictionWMDDataset
    : public torch::data::Dataset<LinkPredictionWMDDataset, WMDData<>>,
      public WMDDataset {
public:
  LinkPredictionWMDDataset() : WMDDataset(), _LinkArrayOID(XEdgeOID::kNullID) {}

  LinkPredictionWMDDataset(const LinkPredictionWMDDataset &O)
      : WMDDataset(O), _LinkArrayOID(XEdgeOID::kNullID) {
    _LinkArrayOID = O._LinkArrayOID;
  }

  LinkPredictionWMDDataset(LinkPredictionWMDDataset &&O)
      : WMDDataset(O), _LinkArrayOID(XEdgeOID::kNullID) {
    _LinkArrayOID = O._LinkArrayOID;
  }

  LinkPredictionWMDDataset &operator=(const LinkPredictionWMDDataset &O) {
    WMDDataset::operator=(O);
    _LinkArrayOID = O._LinkArrayOID;
    return *this;
  }

  LinkPredictionWMDDataset(const VertexOID &VertexArrayID,
                           const XEdgeOID &EdgeArrayOID,
                           const ArrayOID &FeaturesArrayID,
                           const XEdgeOID &LinkArrayOID)
    : WMDDataset(VertexArrayID, EdgeArrayOID, FeaturesArrayID)
    , _LinkArrayOID(LinkArrayOID) {}

  WMDDataset &operator=(WMDDataset &&O) {
    WMDDataset::operator=(O);
    return *this;
  }

  //! Returns the i-th data point from the dataset.
  //!
  //! The data point returned contains:
  //!  + a subgraph obtained by exploring the neighboorhoods of the (i,j)
  //!  vertices;
  //!  + the feature vector of each of the vertices in the subgraph;
  //!  + a mask selecting which vertex to use in training;
  //!
  //! Each data point is constructed on the fly by querying the CSR
  //! representation that is built at the beginning of the workflow.
  WMDData<> get(size_t idx) override {
    auto edges = XEdgeType::GetPtr(_LinkArrayOID);
    auto edge = edges->At(idx);
    int64_t i = edge.src_glbid;
    int64_t j = edge.dst_glbid;
    int64_t root[2] = {i, j};
    auto [graph, vertex_set] = _build_ego_graph(root, root + 2);

    int64_t num_vertices = vertex_set.size();

    // create type and feature vector
    std::vector<int64_t> featureVectors(num_vertices * NUM_FEATURES);

    shad::rt::Handle handle;
    auto Features = shad::Array<uint64_t>::GetPtr(_featuresOID);
    for (auto itr = vertex_set.begin(); itr != vertex_set.end(); ++itr) {
      int64_t glbID = (*itr).first;
      int64_t localID = (*itr).second.id;
      int64_t type = (int64_t)(*itr).second.type;

      auto ptr = (uint64_t *)(featureVectors.data() + localID * NUM_FEATURES);
      Features->AsyncGetElements(handle, ptr, glbID * NUM_FEATURES,
                                 NUM_FEATURES);
    }

    shad::rt::waitForCompletion(handle);

    std::vector<float> floatFeatures(featureVectors.begin(),
                                     featureVectors.end());

    // The features tensor stores the two hop features of the ego-graph vertices
    auto features =
        torch::from_blob(floatFeatures.data(), {num_vertices, NUM_FEATURES},
                         torch::TensorOptions().dtype(torch::kFloat))
            .clone();

    // The labels tensor stores the type of the ego-graph vertices
    float edgeExists = idx > (edges->Size() / 2) ? 0.0 : 1.0;
    auto label = torch::tensor({edgeExists});
    auto mask = torch::full({graph.size(1)},1);
    bool ij_exist = false;

    int64_t i_local = vertex_set[i].id;
    int64_t j_local = vertex_set[j].id;

    
    //loop to find if (i,j) edge is in the graph --> edgeexist actually tells that to us! 
    for(int r=0; r<graph.size(1); r++){
      int64_t v_src = graph[0][r].item<int>();
      int64_t v_dest = graph[1][r].item<int>();;

      if((v_src == i_local && v_dest == j_local) || (v_src == j_local && v_dest == i_local)){
        mask[r] = 0;
        ij_exist=true;
      }
    }

    if(!ij_exist){
        //add 2 new columns to the graph for 2 directional i - j edges
        int64_t datarow1[] = {i_local,j_local};
        int64_t datarow2[] = {j_local,i_local};

        auto t = torch::zeros({2,2});
        t[0][0] = i_local; 
        t[1][0] = j_local; 
        t[0][1] = j_local;
        t[1][1] = i_local;
        graph = torch::cat({ t,graph }, 1);
        mask = torch::full({graph.size(1)},1);
        mask[0] = 0;
        mask[1] = 0;
    }

    float vertexCount = VertexType::GetPtr(_verticesOID)->Size() - 1;
    auto normalizedFeatures = features/vertexCount;
    return {graph, normalizedFeatures, label, mask};
  }

  torch::optional<size_t> size() const override {
    return XEdgeType::GetPtr(_LinkArrayOID)->Size();
  }

private:
  XEdgeOID _LinkArrayOID; // The globl array storing the edge set.
};
} // namespace agile::workflow1

#endif
