#ifndef AGILE_WORKFLOW1_UTILS
#define AGILE_WORKFLOW1_UTILS

#include "shad/core/algorithm.h"
#include "shad/core/execution.h"

namespace agile::workflow1 {
//! The identity function.
template <typename T> T id(const T &v) { return v; }

template <typename InItr, typename OutItr, typename ExecutionPolicy>
OutItr copy(ExecutionPolicy &&policy, InItr B, InItr E, OutItr O) {
  return shad::transform(shad::distributed_parallel_tag{}, B, E, O,
                         id<typename InItr::value_type>);
}
} // namespace agile::workflow1

#endif
