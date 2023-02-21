//===------------------------------------------------------------*- C++ -*-===//
//
//                            The AGILE Workflows
//
//===----------------------------------------------------------------------===//
// ** Pre-Copyright Notice
//
// This computer software was prepared by Battelle Memorial Institute,
// hereinafter the Contractor, under Contract No. DE-AC05-76RL01830 with the
// Department of Energy (DOE). All rights in the computer software are reserved
// by DOE on behalf of the United States Government and the Contractor as
// provided in the Contract. You are authorized to use this computer software
// for Governmental purposes but it is not to be released or distributed to the
// public. NEITHER THE GOVERNMENT NOR THE CONTRACTOR MAKES ANY WARRANTY, EXPRESS
// OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE. This
// notice including this sentence must appear on any copies of this computer
// software.
//
// ** Disclaimer Notice
//
// This material was prepared as an account of work sponsored by an agency of
// the United States Government. Neither the United States Government nor the
// United States Department of Energy, nor Battelle, nor any of their employees,
// nor any jurisdiction or organization that has cooperated in the development
// of these materials, makes any warranty, express or implied, or assumes any
// legal liability or responsibility for the accuracy, completeness, or
// usefulness or any information, apparatus, product, software, or process
// disclosed, or represents that its use would not infringe privately owned
// rights. Reference herein to any specific commercial product, process, or
// service by trade name, trademark, manufacturer, or otherwise does not
// necessarily constitute or imply its endorsement, recommendation, or favoring
// by the United States Government or any agency thereof, or Battelle Memorial
// Institute. The views and opinions of authors expressed herein do not
// necessarily state or reflect those of the United States Government or any
// agency thereof.
//
//                    PACIFIC NORTHWEST NATIONAL LABORATORY
//                                 operated by
//                                   BATTELLE
//                                   for the
//                      UNITED STATES DEPARTMENT OF ENERGY
//                       under Contract DE-AC05-76RL01830
//===----------------------------------------------------------------------===//

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
    uint64_t Purchases_OID;
    uint64_t Sales_OID;
    uint64_t CoffeeSales_OID;
    uint64_t CoffeePurchases_OID;
    uint64_t CoffeeTraders_OID;
    uint64_t Friends_OID;
    uint64_t Persons_OID;
    uint64_t Servers_OID;
    uint64_t Sends_OID;
    uint64_t Uses_OID;
    char filename [120];
};

void readFileCoffee(Handle & handle, const RF_args_t & args);
void readFileSocial(Handle & handle, const RF_args_t & args);
void readFileCyber(Handle & handle, const RF_args_t & args);
void readFileUses(Handle & handle, const RF_args_t & args);
void readFileCommercial(Handle & handle, const RF_args_t & args);
void PrintWeightedSalesEdgesToFile(const RF_args_t & args);

} // namespace agile::workflow4

#endif  // MAIN_H
