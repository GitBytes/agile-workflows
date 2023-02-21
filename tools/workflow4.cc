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

#include "agile/workflow4/main.h"
#include "agile/workflow4/graph.h"

namespace shad {
  using namespace agile::workflow4;

int main(int argc, char *argv[]) {
  Graph_t graph;
  Handle handle;
  double time1 = my_timer();

/********** KERNEL 1 - Graph Construction **********/
  auto Persons         = PersonVertexType::Create(MEDIUM);
  auto Purchases       = PurchaseEdgeType::Create(MEDIUM);
  auto Sales           = SaleEdgeType::Create(MEDIUM);
  auto Friends         = FriendOfEdgeType::Create(MEDIUM);
  auto Servers         = ServerVertexType::Create(MEDIUM);
  auto Sends           = SendsEdgeType::Create(MEDIUM);
  auto Uses            = UsesEdgeType::Create(MEDIUM);
  auto CoffeeTraders   = TraderVertexType::Create(MEDIUM);
  auto CoffeeSales     = SaleEdgeType::Create(MEDIUM);
  auto CoffeePurchases = PurchaseEdgeType::Create(MEDIUM);

  graph["Persons"]         = (uint64_t) (Persons->GetGlobalID());
  graph["Purchases"]       = (uint64_t) (Purchases->GetGlobalID());
  graph["Sales"]           = (uint64_t) (Sales->GetGlobalID());
  graph["Friends"]         = (uint64_t) (Friends->GetGlobalID());
  graph["Servers"]         = (uint64_t) (Servers->GetGlobalID());
  graph["Sends"]           = (uint64_t) (Sends->GetGlobalID());
  graph["Uses"]            = (uint64_t) (Uses->GetGlobalID());
  graph["CoffeeTraders"]   = (uint64_t) (CoffeeTraders->GetGlobalID());
  graph["CoffeeSales"]     = (uint64_t) (CoffeeSales->GetGlobalID());
  graph["CoffeePurchases"] = (uint64_t) (CoffeePurchases->GetGlobalID());

  RF_args_t args;
  args.Persons_OID         = graph["Persons"];
  args.Purchases_OID       = graph["Purchases"];
  args.Sales_OID           = graph["Sales"];
  args.Friends_OID         = graph["Friends"];
  args.Servers_OID         = graph["Servers"];
  args.Sends_OID           = graph["Sends"];
  args.Uses_OID            = graph["Uses"];
  args.CoffeeTraders_OID   = graph["CoffeeTraders"];
  args.CoffeeSales_OID     = graph["CoffeeSales"];
  args.CoffeePurchases_OID = graph["CoffeePurchases"];

  std::string dataFile = argv[1];
  memcpy(args.filename, dataFile.c_str(), dataFile.size() + 1);
  shad::rt::asyncExecuteOnAll(handle, readFileCoffee, args);

  dataFile = argv[2];
  memcpy(args.filename, dataFile.c_str(), dataFile.size() + 1);
  shad::rt::asyncExecuteOnAll(handle, readFileSocial, args);

  dataFile = argv[3];
  memcpy(args.filename, dataFile.c_str(), dataFile.size() + 1);
  shad::rt::asyncExecuteOnAll(handle, readFileCyber, args);

  dataFile = argv[4];
  memcpy(args.filename, dataFile.c_str(), dataFile.size() + 1);
  shad::rt::asyncExecuteOnAll(handle, readFileUses, args);

  dataFile = argv[5];
  memcpy(args.filename, dataFile.c_str(), dataFile.size() + 1);
  shad::rt::asyncExecuteOnAll(handle, readFileCommercial, args);

  waitForCompletion(handle);
  Persons->WaitForBufferedInsert();
  Purchases->WaitForBufferedInsert();
  Sales->WaitForBufferedInsert();
  Friends->WaitForBufferedInsert();
  Servers->WaitForBufferedInsert();
  Sends->WaitForBufferedInsert();
  Uses->WaitForBufferedInsert();
  CoffeeTraders->WaitForBufferedInsert();
  CoffeeSales->WaitForBufferedInsert();
  CoffeePurchases->WaitForBufferedInsert();

  printf("Time for Kernel 1 - Graph Construction = %lf\n\n", my_timer() - time1);

  printf("Number of persons          = %lu\n", Persons->Size());
  printf("Number of servers          = %lu\n", Servers->Size());
  printf("Number of purchase edges   = %lu\n", Purchases->Size());
  printf("Number of sale edges       = %lu\n", Sales->Size());
  printf("Number of friends edges    = %lu\n", Friends->Size());
  printf("Number of sends edges      = %lu\n", Sends->Size());
  printf("Number of uses edges       = %lu\n", Uses->Size());
  printf("Number of coffee traders   = %lu\n", CoffeeTraders->Size());
  printf("Number of coffee sales     = %lu\n", CoffeeSales->Size());
  printf("Number of coffee purchases = %lu\n", CoffeePurchases->Size());

  printf("\n");
  printf("Total number of vertices = %lu\n", Persons->Size() + Servers->Size()) + CoffeeTraders->Size();
  printf("Total number of edges    = %lu\n", Purchases->Size() + Sales->Size() + Friends->Size() +
                          Sends->Size() + Uses->Size() + CoffeeSales->Size() + CoffeePurchases->Size());

/********** KERNEL 3 - Identify most influential coffee suppliers **********/
  dataFile = argv[6];
  memcpy(args.filename, dataFile.c_str(), dataFile.size() + 1);
  
  // ... weight edges of coffee market ...
  CoffeeSales->AsyncForEachEntry(handle, CoffeeSalesWeight, args);
  waitForCompletion(handle);

  // ... output input file for influence maximization kernel ...
  for (auto loc : shad::rt::allLocalities())
    rt::executeAt(loc, PrintWeightedSalesEdgesToFile, args);

/********** KERNEL 4 - Adjust coffee market **********/
  std::vector<uint64_t> influencers = 
      { 1276402, 1041682, 99655, 125113, 54395, 417055, 790367, 738985, 642085, 1476713,
        491166, 837634, 370202, 1485592, 182127, 1340335, 652543, 136186, 557953, 1067753,
        1013377, 1586874, 289695, 255697, 951545, 213279, 1621392, 1359593, 1229956, 719015,
        712014, 607583, 295133, 289867, 277417, 977555, 481573, 230275, 1797815, 1701255,
        1527648, 1163681, 262232, 463828, 1858132, 454940, 443748, 872776, 842009, 208529,
        741705, 1359431, 1289355, 1172519, 1038751, 1007456, 233838, 918225, 224854, 440910,
        1792993, 1781210, 209475, 1628381, 1542292, 1471802, 345107, 147652, 1225937, 1225168,
        1214361, 569771, 1069017, 28965, 962566, 1851269, 451242, 1726941, 1586581, 181938,
        596887, 588302, 575761, 536758, 1055255, 1028881, 501041, 1001567, 970385, 452394,
        399192, 1628453, 808231, 1623532, 1554815, 1542047, 1391446, 1386498, 641673, 1272139
      }; // lost traders
  // std::vector<uint64_t> influencers = {886128, 827536};

  for (uint64_t influencer : influencers)
    CoffeeTraders->AsyncApply(handle, influencer, CancelCoffeeTrader, args);

  waitForCompletion(handle);
  CoffeeTraders->WaitForBufferedInsert();
  CoffeeSales->WaitForBufferedInsert();
  CoffeePurchases->WaitForBufferedInsert();

  printf("Time for Kernel 4 - Graph Adjustment = %lf\n\n", my_timer() - time1);

  printf("Number of coffee traders   = %lu\n", CoffeeTraders->Size());
  printf("Number of coffee sales     = %lu\n", CoffeeSales->Size());
  printf("Number of coffee purchases = %lu\n", CoffeePurchases->Size());

  return 0;
}

}
