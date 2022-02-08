#ifndef AGILE_WORKFLOW1_MAIN_H
#define AGILE_WORKFLOW1_MAIN_H

#include <string>
#include "agile/workflow1/graph.h"

#define SMALL  500000
#define MEDIUM 5000000
#define LARGE  50000000

namespace agile::workflow1 {

void readFile(std::string & filename, Graph_t & graph);
void GNN(Graph_t &);

}

#endif
