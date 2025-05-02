#ifndef MAIN_H_
#define MAIN_H_

#include <map>
#include <math.h>
#include <limits.h>
#include <fstream>

#include "shad/data_structures/array.h"
#include "shad/data_structures/hashmap.h"
#include "shad/data_structures/multimap.h"

#define TINY   5000
#define SMALL  500000
#define MEDIUM 5000000
#define LARGE  50000000

namespace agile::workflow4 {

using Handle = shad::rt::Handle;
using Graph_t = std::map<std::string, uint64_t>;

struct RF_args_t {
    Handle handle;
    double to_buy;
    uint64_t buyer;;
    char filename [120];
    uint64_t Purchases_OID;
    uint64_t Sales_OID;
    uint64_t Friends_OID;
    uint64_t Persons_OID;
    uint64_t Servers_OID;
    uint64_t Sends_OID;
    uint64_t Uses_OID;
    uint64_t ServerSends_OID;
};

void readFileUses(const RF_args_t & args);
void readFileCyber(const RF_args_t & args);
void readFileSocial(const RF_args_t & args);
void readFileCommercial(const RF_args_t & args);
void PrintUsesEdges(const RF_args_t & args);
void PrintFriendEdges(const RF_args_t & args);
void PrintSaleEdgesSimple(const RF_args_t & args);
void PrintSaleEdgesComplex(const RF_args_t & args);
void PrintServerSendEdges(const RF_args_t & args);

} // namespace agile::workflow4

#endif  // MAIN_H
