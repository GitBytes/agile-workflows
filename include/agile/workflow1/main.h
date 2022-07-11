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

namespace agile::workflow1 {
using Handle = shad::rt::Handle;
using Graph_t     = std::map<std::string, uint64_t>;

void readFile(std::string & filename, Graph_t & graph);
void CSR(uint64_t &, uint64_t &, Graph_t & graph);

} // namespace agile::workflow1

#endif  // MAIN_H
