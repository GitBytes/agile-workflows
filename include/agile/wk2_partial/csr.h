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
