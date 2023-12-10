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
