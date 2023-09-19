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

#ifndef GRAPH_H_
#define GRAPH_H_

#include <cstdint>
#include <limits>
#include <vector>

#include "shad/data_structures/hashmap.h"
#include "shad/extensions/data_types/data_types.h"

#include "agile/wk_multihop/main.h"

#define UINT   shad::data_types::UINT
#define DOUBLE shad::data_types::DOUBLE
#define USDATE shad::data_types::USDATE
#define ENCODE shad::data_types::encode

namespace agile::wk_multihop {


class EntityEmbedding {
public:
  uint64_t id_;
  std::array<float, EMBEDDING_DIMENSION> embedding_;
  
  EntityEmbedding () {
    id_   = shad::data_types::kNullValue<uint64_t>;
  }

  EntityEmbedding (uint64_t id, std::vector <float> & embedding) {
    id_ = id;
    std::copy(embedding.begin(), embedding.end(), embedding_.begin());
#ifdef DEBUG
    std::cout << "Printing the vector\n";
    for (auto val : embedding_) {
      std::cout << val << " ";
    }
    std::cout << std::endl;
#endif
  }

  uint64_t key() { return id_;}
};

class PersonVertex {
public:
  uint64_t id_;
  
  PersonVertex () {
    id_   = shad::data_types::kNullValue<uint64_t>;
  }

  PersonVertex (uint64_t id) {
    id_ = id;
  }

  uint64_t key() { return id_;}
};

class UniversityVertex {
public:
  uint64_t id_;
  
  UniversityVertex () {
    id_   = shad::data_types::kNullValue<uint64_t>;
  }

  UniversityVertex (uint64_t id) {
    id_ = id;
  }

  uint64_t key() { return id_;}
};

class WikiDataEdge {
  public:
    uint64_t src_v;            
    uint64_t dst_v;           
    uint64_t edge_type_id;

    WikiDataEdge () {
      src_v   = shad::data_types::kNullValue<uint64_t>;
      dst_v  = shad::data_types::kNullValue<uint64_t>;
      edge_type_id = shad::data_types::kNullValue<uint64_t>;
    }

    WikiDataEdge (std::vector <std::uint64_t> & tokens) {
      src_v    =   tokens[1];
      dst_v   =   tokens[3];
      edge_type_id  =   tokens[2];
    }

    uint64_t key() { return src_v; }
    uint64_t src() { return src_v; }
    uint64_t dst() { return dst_v; }
};


class RelationEmbedding {
public:
  uint64_t id_;
  std::array<float, EMBEDDING_DIMENSION> embedding_;
  
  RelationEmbedding () {
    id_   = shad::data_types::kNullValue<uint64_t>;
  }

  RelationEmbedding (uint64_t id, std::vector <float> & embedding) {
    id_ = id;
    std::copy(embedding.begin(), embedding.end(), embedding_.begin());
#ifdef DEBUG    
    std::cout << "Printing the embedding\n";
    for (auto val : embedding_) {
      std::cout << val << " ";
    }
    std::cout << std::endl;
#endif  
  }

  uint64_t key() { return id_;}
};

class EntityEmbeddingBasedScore {
public:
  uint64_t locale_;
  uint64_t id_;
  float score_;
  
  EntityEmbeddingBasedScore () {
    locale_ = shad::data_types::kNullValue<uint64_t>;
    id_   = shad::data_types::kNullValue<uint64_t>;
    score_   = shad::data_types::kNullValue<float>;
  }

  EntityEmbeddingBasedScore (uint64_t locale, uint64_t id, float score) {
    locale_ = locale;
    id_ = id;
    score_ = score;
  }

  uint64_t key() { return locale_;}
};


class EntityScoreInfo {
public:
  uint64_t id_;
  float score_;
  
   EntityScoreInfo () {
     id_   = shad::data_types::kNullValue<uint64_t>;
     score_   = shad::data_types::kNullValue<float>;
  }

  EntityScoreInfo (float score, int64_t id) {
    id_ = id;
    score_ = score;
  }

  uint64_t key() { return score_;}
};

struct EntityScoreComparator {
  bool operator()(const EntityScoreInfo & s1, 
		  const EntityScoreInfo & s2) const {
    return s1.score_ > s2.score_;
  }
};

using WikiDataEdgeType = shad::Multimap<uint64_t, WikiDataEdge>;
using WikiDataEdgeOID  = shad::ObjectIdentifier<WikiDataEdgeType>;

using EntityEmbeddingType = shad::Hashmap<uint64_t, EntityEmbedding>;
using EntityEmbeddingOID  = shad::ObjectIdentifier<EntityEmbeddingType>;

using RelationEmbeddingType = shad::Hashmap<uint64_t, RelationEmbedding>;
using RelationEmbeddingOID  = shad::ObjectIdentifier<RelationEmbeddingType>;

using PersonVertexType = shad::Hashmap<uint64_t, PersonVertex>;
using PersonVertexOID  = shad::ObjectIdentifier<PersonVertexType>;

using UniversityVertexType = shad::Hashmap<uint64_t, UniversityVertex>;
using UniversityVertexOID  = shad::ObjectIdentifier<UniversityVertexType>;

using EntityScoreType = shad::Multimap<uint64_t, EntityEmbeddingBasedScore>;
using EntityScoreOID  = shad::ObjectIdentifier<EntityScoreType>;


} // namespace agile::wk_multihop

#endif // GRAPH_H
