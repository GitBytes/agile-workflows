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
  bool cmplx;
  Graph_t graph;
  Handle handle;
  std::string dataFile;
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
  auto ServerToServer  = ServerToServerEdgeType ::Create(MEDIUM);

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
  graph["ServerToServer"]  = (uint64_t) (ServerToServer->GetGlobalID());

  RF_args_t args;
  args.handle             = handle;
  args.Persons_OID        = graph["Persons"];
  args.Purchases_OID      = graph["Purchases"];
  args.Sales_OID          = graph["Sales"];
  args.Friends_OID        = graph["Friends"];
  args.Servers_OID        = graph["Servers"];
  args.Sends_OID          = graph["Sends"];
  args.Uses_OID           = graph["Uses"];
  args.ServerToServer_OID = graph["ServerToServer"];

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

  waitForCompletion(handle);
  Persons->WaitForBufferedInsert();
  Purchases->WaitForBufferedInsert();
  Sales->WaitForBufferedInsert();
  Friends->WaitForBufferedInsert();
  Servers->WaitForBufferedInsert();
  Sends->WaitForBufferedInsert();
  Uses->WaitForBufferedInsert();

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

// Select coffee submarket;
  uint64_t product = 8486;     // coffee market
  args.CoffeeTraders_OID   = graph["CoffeeTraders"];                      // coffee trader vertices
  args.CoffeeSales_OID     = graph["CoffeeSales"];                        // coffee sale edges
  args.CoffeePurchases_OID = graph["CoffeePurchases"];                    // coffee purchase edges
  Sales->AsyncForEachEntry(handle, SelectSalesMarket, product, args);     // select coffee sales market

  if (argc >= 6) { dataFile = argv[5]; cmplx = (dataFile == "complex"); printf("complex switch = %lu\n", cmplx);
  } else { printf("No simple/complex switch\n"); exit(-1); }

  if (cmplx) {
     Friends->AsyncForEachEntry(handle, FriendsEdgeWeights, args);        // update friend edge with weights
     Uses->AsyncForEachEntry(handle, UsesEdgeWeights, args);              // update uses edge with weights 
     Sends->AsyncForEachEntry(handle, SendsEdgeWeights, args);            // create server to server edges
  }

  waitForCompletion(handle);
  CoffeeTraders->WaitForBufferedInsert();
  CoffeeSales->WaitForBufferedInsert();
  CoffeePurchases->WaitForBufferedInsert();
  ServerToServer->WaitForBufferedInsert();

  CoffeeSales->AsyncForEachEntry(handle, CoffeeSalesWeight, args);        // update coffee sale edges with weights
  waitForCompletion(handle);

  printf("\nTime for Kernel 2 - Coffee subgraph and sale weights = %lf\n", my_timer() - time1);
  printf("Number of coffee traders   = %lu\n", CoffeeTraders->Size());
  printf("Number of coffee sales     = %lu\n", CoffeeSales->Size());
  printf("Number of coffee purchases = %lu\n", CoffeePurchases->Size());
  if (cmplx) printf("Number of Server to Server edges = %lu\n\n", ServerToServer->Size());

  // ... output input file for influence maximization kernel ... exit ...
  // ... and run influence maximization kernel off line ...
  if (argc <= 7) {
     dataFile = argv[6];
     memcpy(args.filename, dataFile.c_str(), dataFile.size() + 1);

     std::ofstream file;
     file.open(dataFile);
     if (! file.is_open()) { printf("Cannot open file %s\n", dataFile.c_str()); exit(-1); }

     if (cmplx) {
        for (auto loc : shad::rt::allLocalities()) rt::executeAt(loc, PrintWeightedSaleEdgesComplex, args);
        printf("weighted sales edges printed ...\n");

        for (auto loc : shad::rt::allLocalities()) rt::executeAt(loc, PrintWeightedFriendEdges, args);
        printf("weighted friends edges printed ...\n");

        for (auto loc : shad::rt::allLocalities()) rt::executeAt(loc, PrintWeightedUsesEdges, args);
        printf("weighted uses edges printed ...\n");

        for (auto loc : shad::rt::allLocalities()) rt::executeAt(loc, PrintWeightedServerToServerEdges, args);
        printf("weighted server to server edges printed ... exiting\n");

     } else {
       for (auto loc : shad::rt::allLocalities()) rt::executeAt(loc, PrintWeightedSaleEdgesSimple, args);
       printf("weighted sales edges printed ... exiting\n");
     }

     file.close();
     exit(0);
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
