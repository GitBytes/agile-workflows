#ifndef PATTERN_H_
#define PATTERN_H_

#include "shad/data_structures/hashmap.h"

namespace agile::wk2_exact {

template <typename T>
struct intTimeInserter {
  intTimeInserter() { }

  bool operator()(T * const lhs, const T & rhs, bool same_key) {
    if (same_key) {     // entry in hashmap, value = min(current value, new value)
       if (* lhs > rhs) * lhs = rhs;
    } else {            // entry not in hashmap, value = new value
       * lhs = rhs;
    }

    return true;
  }

  bool Insert(T *const lhs, const T &rhs, bool same_key) {
    if (same_key) {     // entry in hashmap, value = minimum(current value, new value)
       if (* lhs > rhs) * lhs = rhs;
    } else {            // entry not in hashmap, value = new value
       * lhs = rhs;
    }
       
    return true;
  }
};

using intTimeMap = shad::Hashmap<uint64_t, time_t, shad::MemCmp<uint64_t>, intTimeInserter<time_t> >;
using intTimeMapOID  = shad::ObjectIdentifier<intTimeMap>;

} // namespace agile::wk2_exact

#endif // PATTERN_H
