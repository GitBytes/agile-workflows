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

#include "nlohmann/json.hpp"
#include "agile/workflow4/main.h"
#include "agile/workflow4/graph.h"

namespace shad {
  using namespace agile::workflow4;

int main(int argc, char *argv[]) {
  Graph_t graph;
  Handle handle;
  std::string dataFile;
  double time1 = my_timer();

/********** KERNEL 1 - Graph Construction **********/
  auto Persons   = PersonVertexType::Create(MEDIUM);
  auto Purchases = PurchaseEdgeType::Create(MEDIUM);
  auto Sales     = SaleEdgeType::Create(MEDIUM);
  auto Friends   = FriendEdgeType::Create(MEDIUM);
  auto Servers   = ServerVertexType::Create(MEDIUM);
  auto Sends     = SendEdgeType::Create(MEDIUM);
  auto Uses      = UsesEdgeType::Create(MEDIUM);

  graph["Persons"]   = (uint64_t) (Persons->GetGlobalID());
  graph["Purchases"] = (uint64_t) (Purchases->GetGlobalID());
  graph["Sales"]     = (uint64_t) (Sales->GetGlobalID());
  graph["Friends"]   = (uint64_t) (Friends->GetGlobalID());
  graph["Servers"]   = (uint64_t) (Servers->GetGlobalID());
  graph["Sends"]     = (uint64_t) (Sends->GetGlobalID());
  graph["Uses"]      = (uint64_t) (Uses->GetGlobalID());

  RF_args_t args;
  args.handle        = handle;
  args.Persons_OID   = graph["Persons"];
  args.Purchases_OID = graph["Purchases"];
  args.Sales_OID     = graph["Sales"];
  args.Friends_OID   = graph["Friends"];
  args.Servers_OID   = graph["Servers"];
  args.Sends_OID     = graph["Sends"];
  args.Uses_OID      = graph["Uses"];

  if (argc >= 2) dataFile = argv[1]; else {printf("No social file\n"); exit(-1);}
  memcpy(args.filename, dataFile.c_str(), dataFile.size() + 1);
  shad::rt::executeOnAll(readFileSocial, args);

  if (argc >= 3) dataFile = argv[2]; else {printf("No cyber file\n"); exit(-1);}
  memcpy(args.filename, dataFile.c_str(), dataFile.size() + 1);
  shad::rt::executeOnAll(readFileCyber, args);

  if (argc >= 4) dataFile = argv[3]; else {printf("No uses file\n"); exit(-1);}
  memcpy(args.filename, dataFile.c_str(), dataFile.size() + 1);
  shad::rt::executeOnAll(readFileUses, args);

  if (argc >= 5) dataFile = argv[4]; else {printf("No commercial file\n"); exit(-1);}
  memcpy(args.filename, dataFile.c_str(), dataFile.size() + 1);
  shad::rt::executeOnAll(readFileCommercial, args);

  Persons->AsyncWaitForBufferedInsert(handle);
  Purchases->AsyncWaitForBufferedInsert(handle);
  Sales->AsyncWaitForBufferedInsert(handle);
  Friends->AsyncWaitForBufferedInsert(handle);
  Servers->AsyncWaitForBufferedInsert(handle);
  Sends->AsyncWaitForBufferedInsert(handle);
  Uses->AsyncWaitForBufferedInsert(handle);
  waitForCompletion(handle);

  printf("Time for Kernel 1 - Graph Construction = %lf\n\n", my_timer() - time1);

  printf("Number of persons        = %lu\n", Persons->Size());
  printf("Number of servers        = %lu\n", Servers->Size());
  printf("Number of purchase edges = %lu\n", Purchases->Size());
  printf("Number of sale edges     = %lu\n", Sales->Size());
  printf("Number of friends edges  = %lu\n", Friends->Size());
  printf("Number of sends edges    = %lu\n", Sends->Size());
  printf("Number of uses edges     = %lu\n", Uses->Size());

/******** KERNEL 2 - Prepare graph for influence maximization kernel ********/
  time1 = my_timer();      

  bool cmplx;
  Graph_t coffeeGraph;
  uint64_t product = 8486;                                                  // coffee id

  auto CoffeeTraders   = TraderVertexType::Create(MEDIUM);
  auto CoffeeSales     = SaleEdgeType::Create(MEDIUM);
  auto CoffeePurchases = PurchaseEdgeType::Create(MEDIUM);
  auto CoffeeFriends   = FriendEdgeType::Create(MEDIUM);
  auto CoffeeServers   = ServerVertexType::Create(MEDIUM);
  auto CoffeeSends     = SendEdgeType::Create(MEDIUM);
  auto CoffeeUses      = UsesEdgeType::Create(MEDIUM);
  auto ServerSends     = ServerSendEdgeType::Create(MEDIUM);

  coffeeGraph["CoffeeTraders"]   = (uint64_t) (CoffeeTraders->GetGlobalID());
  coffeeGraph["CoffeeSales"]     = (uint64_t) (CoffeeSales->GetGlobalID());
  coffeeGraph["CoffeePurchases"] = (uint64_t) (CoffeePurchases->GetGlobalID());
  coffeeGraph["CoffeeFriends"]   = (uint64_t) (CoffeeFriends->GetGlobalID());
  coffeeGraph["CoffeeServers"]   = (uint64_t) (CoffeeServers->GetGlobalID());
  coffeeGraph["CoffeeSends"]     = (uint64_t) (CoffeeSends->GetGlobalID());
  coffeeGraph["CoffeeUses"]      = (uint64_t) (CoffeeUses->GetGlobalID());
  coffeeGraph["ServerSends"]     = (uint64_t) (ServerSends->GetGlobalID());

  RF_args_t coffeeArgs;
  coffeeArgs.handle              = handle;
  coffeeArgs.Persons_OID         = coffeeGraph["CoffeeTraders"];
  coffeeArgs.Sales_OID           = coffeeGraph["CoffeeSales"];
  coffeeArgs.Purchases_OID       = coffeeGraph["CoffeePurchases"];
  coffeeArgs.Friends_OID         = coffeeGraph["CoffeeFriends"];
  coffeeArgs.Servers_OID         = coffeeGraph["CoffeeServers"];
  coffeeArgs.Sends_OID           = coffeeGraph["CoffeeSends"];
  coffeeArgs.Uses_OID            = coffeeGraph["CoffeeUses"];
  coffeeArgs.ServerSends_OID     = coffeeGraph["ServerSends"];

  // select coffee traders, sales, and purchases
  Sales->AsyncForEachEntry(handle, SelectSalesMarket, product, coffeeArgs);
  waitForCompletion(handle);

  CoffeeTraders->AsyncWaitForBufferedInsert(handle);
  CoffeeSales->AsyncWaitForBufferedInsert(handle);
  CoffeePurchases->AsyncWaitForBufferedInsert(handle);
  waitForCompletion(handle);

  CoffeeSales->AsyncForEachEntry(handle, CoffeeSalesWeight, coffeeArgs);              // add weigths to sale edges
  waitForCompletion(handle);

  // set simple/complex scenario switch
  if (argc >= 6) cmplx = ( strcmp(argv[5], "complex") == 0 );
  else { printf("No simple/complex switch\n"); exit(-1); }

  if (cmplx) {                                                                        // if complex ...
     CoffeeTraders->AsyncForEachEntry(handle, FriendsSubgraph, coffeeArgs, args);     // ... friend edgess
     CoffeeTraders->AsyncForEachEntry(handle, ServersSubgraph, coffeeArgs, args);     // ... servers and uses edges
     waitForCompletion(handle);

     CoffeeFriends->AsyncWaitForBufferedInsert(handle);
     CoffeeServers->AsyncWaitForBufferedInsert(handle);
     CoffeeUses->AsyncWaitForBufferedInsert(handle);
     waitForCompletion(handle);

     CoffeeServers->AsyncForEachEntry(handle, SendsSubgraph, coffeeArgs, args);       // ... select sends edges
     waitForCompletion(handle);

     CoffeeSends->AsyncWaitForBufferedInsert(handle);
     waitForCompletion(handle);
  }

  printf("\nTime for Kernel 2 - Coffee subgraph selection = %lf\n", my_timer() - time1);
  printf("Number of coffee traders    = %lu\n", CoffeeTraders->Size());
  printf("Number of coffee sales      = %lu\n", CoffeeSales->Size());
  printf("Number of coffee purchases  = %lu\n", CoffeePurchases->Size());

  if (cmplx) {
     printf("Number of coffee friends     = %lu\n", CoffeeFriends->Size());
     printf("Number of coffee servers     = %lu\n", CoffeeServers->Size());
     printf("Number of coffee uses edges  = %lu\n", CoffeeUses->Size());
     printf("Number of coffee sends edges = %lu\n", CoffeeSends->Size());
  }

  return 0;

  // ... output input file for influence maximization kernel ... exit ...
  // ... and run influence maximization kernel off line ...
  if (argc <= 7) {
     dataFile = argv[6];
     memcpy(coffeeArgs.filename, dataFile.c_str(), dataFile.size() + 1);

     std::ofstream file;
     file.open(dataFile);
     if (! file.is_open()) { printf("Cannot open file %s\n", dataFile.c_str()); exit(-1); }

     if (cmplx) {
        for (auto loc : shad::rt::allLocalities()) rt::executeAt(loc, PrintSaleEdgesComplex, coffeeArgs);
        printf("weighted sales edges printed ...\n");

        for (auto loc : shad::rt::allLocalities()) rt::executeAt(loc, PrintFriendEdges, coffeeArgs);
        printf("weighted friends edges printed ...\n");

        // for (auto loc : shad::rt::allLocalities()) rt::executeAt(loc, PrintServerSendEdges, coffeeArgs);
        // printf("weighted server send edges printed ... exiting\n");

        // for (auto loc : shad::rt::allLocalities()) rt::executeAt(loc, PrintUsesEdges, coffeeArgs);
        // printf("weighted uses edges printed ...\n");

     } else {
       for (auto loc : shad::rt::allLocalities()) rt::executeAt(loc, PrintSaleEdgesSimple, coffeeArgs);
       printf("weighted sales edges printed ... exiting\n");
     }

     file.close();
     return 0;
  }

  // ... read list of influencers ...
  dataFile = argv[6];
  std::ifstream file(dataFile.c_str());
  if (! file.is_open()) { printf("Cannot open file %s\n", dataFile.c_str()); exit(-1); }

  std::stringstream buffer;
  std::vector<uint64_t> influencers;

  buffer << file.rdbuf();
  auto json = nlohmann::json::parse(buffer.str());
  for (auto influencer : json[0]["Seeds"]) influencers.push_back(influencer);

  printf("Number of influencers = %lu\n\n", influencers.size());

/********** KERNEL 4 - Adjust coffee market **********/
  time1 = my_timer();

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

  // TraderVertex tmp;
  // CoffeeTraders->Lookup(35804, & tmp);
  // printf("grower 35804, desired = %lf, bought = %lf, sold = %lf\n", tmp.desired, tmp.bought, tmp.sold);

  return 0;
}

}
