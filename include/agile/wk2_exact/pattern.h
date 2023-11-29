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

#include "shad/data_structures/hashmap.h"

namespace agile::wk2_exact {

template <typename T>
struct ForumMapInserter {

  bool operator()(Handle & handle, T * const lhs, const T & rhs, bool same_key) {
    if (same_key) {     // entry in hashmap, update value
       (* lhs).FE4 |= rhs.FE4;
       (* lhs).FE5 |= rhs.FE5;
       (* lhs).date = std::min((* lhs).date, rhs.date);
     } else {            // entry not in hashmap, set value
       * lhs = std::move(rhs);
     }

     return true;
  }

  bool Insert(Handle & handle, T * const lhs, const T & rhs, bool same_key) {
    if (same_key) {     // entry in hashmap, update value
       (* lhs).FE4 |= rhs.FE4;
       (* lhs).FE5 |= rhs.FE5;
       (* lhs).date = std::min((* lhs).date, rhs.date);
     } else {            // entry not in hashmap, set value
       * lhs = std::move(rhs);
     }

     return true;
  }
};


class ForumMapVertex {
  public:
    uint64_t forum;
    bool     FE4;
    bool     FE5;
    time_t   date;

    ForumMapVertex () {
      forum  = shad::data_types::kNullValue<uint64_t>;
      FE4 = false;
      FE5 = false;
      date = shad::data_types::kNullValue<time_t>;
    }

    ForumMapVertex (uint64_t forum_, bool FE4_, bool FE5_, time_t date_) {
      forum = forum_;
      FE4 = FE4_;
      FE5 = FE5_;
      date = date_;
    }

    uint64_t key() { return forum; }
};

using ForumMap = shad::Hashmap<uint64_t, ForumMapVertex, shad::MemCmp<uint64_t>, ForumMapInserter<ForumMapVertex>>;
using ForumMapOID = shad::ObjectIdentifier<ForumMap>;

} // namespace agile::wk2_exact

#endif // PATTERN_H
