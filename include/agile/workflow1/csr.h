#ifndef CSR_H_
#define CSR_H_

#include <cstdint>
#include <limits>
#include <vector>

#include "agile/workflow1/main.h"
#include "shad/data_structures/hashmap.h"
#include "shad/extensions/data_types/data_types.h"

namespace agile::workflow1 {

struct MTE_args_t {
  TYPES type;
  uint64_t start;
  uint64_t edgesOID;
};

template <typename VTYPE>
void MoveTableEdges(shad::rt::Handle & handle, const uint64_t & key,
     std::vector<VTYPE> & value, MTE_args_t & args, uint8_t * ret, uint32_t * retSize) {

  uint64_t NE = 0;
  uint64_t start = args.start;
  auto Edges = EdgeType::GetPtr((EdgeOID) args.edgesOID);

  for (auto & E1 : value) {
    Edge E2( E1.src(), E1.dst(), 0.0, args.type, E1.src_type, E1.dst_type );
    Edges->BufferedAsyncInsertAt( handle, start, E2);
    start ++;
    NE ++;
  };

  * retSize = sizeof(uint64_t);
  memcpy(ret, & NE, sizeof(uint64_t));
};

} // namespace agile::workflow1

#endif // CSR_H
