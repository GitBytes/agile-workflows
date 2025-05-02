#ifndef MAIN_H_
#define MAIN_H_

#include <map>
#include <tuple>
#include <math.h>
#include <limits.h>

#include "shad/data_structures/set.h"
#include "shad/data_structures/array.h"
#include "shad/data_structures/hashmap.h"
#include "shad/data_structures/multimap.h"
#include "shad/core/algorithm.h"
#include "shad/core/numeric.h"

//BM
#include <queue>
#include <condition_variable>
#include <thread>
#include <mutex>
#include "shad/extensions/data_types/data_types.h"
#include "agile/wk2_partial/graphTypes.h"

#define AGILE_TINY   5000
#define AGILE_SMALL  500000
#define AGILE_MEDIUM 5000000
#define AGILE_LARGE  50000000

namespace agile::wk2_partial {
using Graph_t     = std::map<std::string, uint64_t>;
using AllEdge     = std::tuple<uint64_t, std::string>;
using AllEdgeType = shad::Multimap<uint64_t, AllEdge>;
using AllEdgeOID  = shad::ObjectIdentifier<AllEdgeType>;

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
  uint64_t SubPattern1_OID;
  uint64_t SubPattern2_OID;
  uint64_t SubPattern12_OID;
  uint64_t SubPattern3_OID;
  uint64_t SubPattern4_OID;
  uint64_t SubPattern5_OID;
  uint64_t SubPattern6_OID;
  uint64_t SubPattern7_OID;
  uint64_t SubPattern13_OID;
  uint64_t SubPattern14_OID;
  double start_time;
  char filename [120];
};


uint64_t String_to_Uint(std::string &);
double String_to_Double(std::string &);
double String_to_Date(std::string &);

using Handle = shad::rt::Handle;
// using IntArray = shad::Array<int64_t>;
// using IntArrayOID = shad::ObjectIdentifier<IntArray>;
// using intSet = shad::Set<uint64_t>;
// using intSetOID = shad::ObjectIdentifier<intSet>;

using Graph_t = std::map<std::string, uint64_t>;
void readFile(std::string & filename, Graph_t & graph);
TYPES insertToGraph(std::string &, Graph_t &);
TYPES insertToGraphBuffered(Handle & handle,std::string &, Graph_t &);
// void CSR(uint64_t &, uint64_t &, Graph_t & graph);
void WMD_pattern(Graph_t & graph);
} // namespace agile::wk2_partial

#endif  // MAIN_H
