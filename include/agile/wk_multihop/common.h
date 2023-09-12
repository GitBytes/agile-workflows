#ifndef COMMON_H_
#define COMMON_H_

#include <array>
#include "agile/wk_multihop/main.h"

namespace agile::wk_multihop {
  using Graph_t = std::array<uint64_t, 1386>;
  extern Graph_t graph;
}
#endif
