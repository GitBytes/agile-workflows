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
#ifndef PATTERN_H_
#define PATTERN_H_

#include "shad/data_structures/replicated_hashmap.h"

namespace agile::wk2_exact {

template <typename T>
struct Inserter {

  bool operator()(Handle &, T * const lhs, const T & rhs, bool same_key) {
    if (same_key) {     // entry in hashmap, update value
       (* lhs).FE4   |= rhs.FE4;
       (* lhs).FE5   |= rhs.FE5;
       (* lhs).NYC   |= rhs.NYC;
       (* lhs).ELE   |= rhs.ELE;
       (* lhs).jihad += rhs.jihad;
       (* lhs).date   = std::min((* lhs).date, rhs.date);
     } else {            // entry not in hashmap, set value
       * lhs = std::move(rhs);
     }

     return true;
  }

  bool Insert(Handle &, T * const lhs, const T & rhs, bool same_key) {
    if (same_key) {     // entry in hashmap, update value
       (* lhs).FE4   |= rhs.FE4;
       (* lhs).FE5   |= rhs.FE5;
       (* lhs).NYC   |= rhs.NYC;
       (* lhs).ELE   |= rhs.ELE;
       (* lhs).jihad += rhs.jihad;
       (* lhs).date   = std::min((* lhs).date, rhs.date);
     } else {            // entry not in hashmap, set value
       * lhs = std::move(rhs);
     }

     return true;
  }

  bool operator()(T * const lhs, const T & rhs, bool same_key) {
    if (same_key) {     // entry in hashmap, update value
       (* lhs).FE4   |= rhs.FE4;
       (* lhs).FE5   |= rhs.FE5;
       (* lhs).NYC   |= rhs.NYC;
       (* lhs).ELE   |= rhs.ELE;
       (* lhs).jihad += rhs.jihad;
       (* lhs).date   = std::min((* lhs).date, rhs.date);
     } else {            // entry not in hashmap, set value
       * lhs = std::move(rhs);
     }

     return true;
  }

  bool Insert(T * const lhs, const T & rhs, bool same_key) {
    if (same_key) {     // entry in hashmap, update value
       (* lhs).FE4   |= rhs.FE4;
       (* lhs).FE5   |= rhs.FE5;
       (* lhs).NYC   |= rhs.NYC;
       (* lhs).ELE   |= rhs.ELE;
       (* lhs).jihad += rhs.jihad;
       (* lhs).date   = std::min((* lhs).date, rhs.date);
     } else {            // entry not in hashmap, set value
       * lhs = std::move(rhs);
     }

     return true;
  }
};


class TopicMapVertex {
  public:
    uint64_t id;
    bool     FE4;
    bool     FE5;
    bool     NYC;
    bool     ELE;
    uint64_t jihad;
    time_t   date;

    TopicMapVertex () {
      id    = shad::data_types::kNullValue<uint64_t>;
      FE4   = false;
      FE5   = false;
      NYC   = false;
      ELE   = false;
      jihad = 0;
      date  = shad::data_types::kNullValue<time_t>;
    }

    TopicMapVertex (uint64_t id_, bool FE4_, bool FE5_, bool NYC_, bool ELE_, uint64_t jihad_, time_t date_) {
      id    = id_;
      FE4   = FE4_;
      FE5   = FE5_;
      NYC   = NYC_;
      ELE   = ELE_;
      jihad = jihad_;
      date  = date_;
    }

    uint64_t key() { return id; }
};

using TopicMap = shad::Replicated_Hashmap<uint64_t, TopicMapVertex, shad::MemCmp<uint64_t>, Inserter<TopicMapVertex>>;
using TopicMapOID = shad::ObjectIdentifier<TopicMap>;

} // namespace agile::wk2_exact

#endif // PATTERN_H
