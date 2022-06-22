#ifndef MAIN_H_
#define MAIN_H_

#include <map>
#include <math.h>
#include <limits.h>

#include "shad/data_structures/array.h"
#include "shad/data_structures/hashmap.h"
#include "shad/data_structures/multimap.h"

#define TINY   5000
#define SMALL  500000
#define MEDIUM 5000000
#define LARGE  50000000

namespace agile::wk2_exact {

using Handle = shad::rt::Handle;
using Graph_t = std::map<std::string, uint64_t>;

struct RF_args_t {
  uint64_t Persons_OID;
  uint64_t ForumEvents_OID;
  uint64_t Forums_OID;
  uint64_t Publications_OID;
  uint64_t Topics_OID;
  uint64_t Purchases_OID;
  uint64_t Sales_OID;
  uint64_t Authors_OID;
  uint64_t Includes_OID;
  uint64_t HasTopic_OID;
  uint64_t HasOrg_OID;
  char filename [120];
};

void readFile(Handle & handle, const RF_args_t & args);
void WMD_pattern(Graph_t & graph);

} // namespace agile::wk2_exact

#endif  // MAIN_H
