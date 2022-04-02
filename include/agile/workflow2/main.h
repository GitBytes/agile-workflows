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

#define TINY   5000
#define SMALL  500000
#define MEDIUM 5000000
#define LARGE  50000000

namespace agile::workflow2 {
using Graph_t     = std::map<std::string, uint64_t>;
using AllEdge     = std::tuple<uint64_t, std::string>;
using AllEdgeType = shad::Multimap<uint64_t, AllEdge>;
using AllEdgeOID  = shad::ObjectIdentifier<AllEdgeType>;

uint64_t String_to_Uint(std::string &);
double String_to_Double(std::string &);
double String_to_Date(std::string &);
void readFile(std::string & filename, Graph_t & graph);
void approxMatching();

using IntArray = shad::Array<int64_t>;
using IntArrayOID = shad::ObjectIdentifier<IntArray>;
using intSet = shad::Set<uint64_t>;
using intSetOID = shad::ObjectIdentifier<intSet>;

using Graph_t = std::map<std::string, uint64_t>;

void readFile(std::string & filename, Graph_t & graph);
void CSR(uint64_t &, uint64_t &, Graph_t & graph);
void WMD_pattern(Graph_t &);

} // namespace agile::workflow2

#endif  // MAIN_H
