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

#ifndef CSR_H_
#define CSR_H_

#include <cstdint>
#include <limits>
#include <vector>
#include <type_traits>
#include "shad/data_structures/hashmap.h"
#include "shad/extensions/data_types/data_types.h"

#include "agile/wk2_partial/main.h"
#include "agile/wk2_partial/graphTypes.h"

namespace agile::wk2_partial {

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

} // namespace agile::wk2_partial

#endif // CSR_H
