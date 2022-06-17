#ifndef MAIN_H_
#define MAIN_H_

#include <math.h>
#include "shad/data_structures/array.h"
#include "shad/data_structures/hashmap.h"
#include "shad/data_structures/multimap.h"

#define TINY   5000
#define SMALL  500000
#define MEDIUM 5000000
#define LARGE  50000000

namespace agile::wk2_approx {

using Handle = shad::rt::Handle;
using Graph_t = std::map<std::string, uint64_t>;

void readFile(std::string &, Graph_t &);
void createBipartite(Graph_t &, Graph_t &, uint64_t &, uint64_t &);
void ApproxMatching(uint64_t &, uint64_t &);
void getMatching(uint64_t &, uint64_t &);
} // namespace agile::wk2_approx

#endif  // MAIN_H
