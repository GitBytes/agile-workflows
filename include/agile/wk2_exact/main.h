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

void readFile(std::string & filename, Graph_t & graph);
void WMD_pattern(Graph_t & graph);

} // namespace agile::wk2_exact

#endif  // MAIN_H
