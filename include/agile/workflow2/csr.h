#ifndef CSR_H_
#define CSR_H_

#include <cstdint>
#include <limits>
#include <vector>
#include <type_traits>

#include "agile/workflow2/main.h"
#include "shad/data_structures/hashmap.h"
#include "shad/extensions/data_types/data_types.h"

namespace agile::workflow2 {

template <typename VTYPE>
void updateGLBID(Handle & handle, const uint64_t & key, VTYPE & value, uint64_t & id) {
  value.glbid = id;
}


template <typename VTYPE>
void MoveTableEdges(Handle & handle, const uint64_t & key, std::vector<VTYPE> & value,
  uint64_t & ndx, uint64_t & globalIDSOID, uint64_t & edgesOID, uint8_t * ret, uint32_t * retSize) {

  TYPES type;
  uint64_t NE = 0;
  auto GlobalIDS = GlobalIDType::GetPtr((GlobalIDOID) globalIDSOID);

  if      (std::is_same <VTYPE, PurchaseEdge>::value) type = TYPES::PURCHASE;
  else if (std::is_same <VTYPE, SaleEdge>::value)     type = TYPES::SALE;
  else if (std::is_same <VTYPE, AuthorEdge>::value)   type = TYPES::AUTHOR;
  else if (std::is_same <VTYPE, IncludesEdge>::value) type = TYPES::INCLUDES;
  else if (std::is_same <VTYPE, HasTopicEdge>::value) type = TYPES::HASTOPIC;
  else if (std::is_same <VTYPE, HasOrgEdge>::value)   type = TYPES::HASORG;

  auto srcLambda = [] (Handle & handle, const uint64_t & src_id, Vertex & src_vertex,
       uint64_t & ndx, Edge & E2, uint64_t & globalIDSOID, uint64_t & edgesOID) {

    auto dstLambda = [] (Handle & handle, const uint64_t & dst_id, Vertex & dst_vertex,
         uint64_t & ndx, Edge & E2, uint64_t & edgesOID) {

      E2.dst_glbid = dst_vertex.id;
      EdgeType::GetPtr((EdgeOID) edgesOID)->AsyncInsertAt(handle, ndx, E2);
    };

    E2.src_glbid = src_vertex.id;
    GlobalIDType::GetPtr((GlobalIDOID) globalIDSOID)->AsyncApply(handle, E2.dst, dstLambda, ndx, E2, edgesOID);
  };

  for (auto & E1 : value) {
    Edge E2( E1.src(), E1.dst(), 0.0, type, E1.src_type, E1.dst_type, 0, 0 );
    GlobalIDS->AsyncApply(handle, E2.src, srcLambda, ndx, E2, globalIDSOID, edgesOID);
    ndx ++; NE ++;
  };

  * retSize = sizeof(uint64_t);
  memcpy(ret, & NE, sizeof(uint64_t));
};

void exclusiveScanVertices(uint64_t arrayOID);

} // namespace agile::workflow2

#endif // CSR_H
