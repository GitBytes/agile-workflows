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
