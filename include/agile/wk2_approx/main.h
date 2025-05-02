#ifndef MAIN_H_
#define MAIN_H_

#include <math.h>
#include "shad/data_structures/array.h"
#include "shad/data_structures/hashmap.h"
#include "shad/data_structures/multimap.h"

#define AGILE_TINY   5000
#define AGILE_SMALL  500000
#define AGILE_MEDIUM 5000000
#define AGILE_LARGE  50000000

namespace agile::wk2_approx {

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
void createBipartite(Graph_t &, Graph_t &, uint64_t &, uint64_t &);
void ApproxMatching(uint64_t &, uint64_t &);
void getMatching(uint64_t &, uint64_t &);
} // namespace agile::wk2_approx

#endif  // MAIN_H
