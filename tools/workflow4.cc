#include "nlohmann/json.hpp"
#include "agile/workflow4/main.h"
#include "agile/workflow4/graph.h"

namespace shad {
  using namespace agile::workflow4;

int main(int argc, char *argv[]) {
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

  RF_args_t args;
  args.handle        = handle;
  args.Persons_OID   = (uint64_t) (Persons->GetGlobalID());
  args.Purchases_OID = (uint64_t) (Purchases->GetGlobalID());
  args.Sales_OID     = (uint64_t) (Sales->GetGlobalID());
  args.Friends_OID   = (uint64_t) (Friends->GetGlobalID());
  args.Servers_OID   = (uint64_t) (Servers->GetGlobalID());
  args.Sends_OID     = (uint64_t) (Sends->GetGlobalID());
  args.Uses_OID      = (uint64_t) (Uses->GetGlobalID());

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
  printf("Number of friend edges   = %lu\n", Friends->Size());
  printf("Number of send edges     = %lu\n", Sends->Size());
  printf("Number of use edges      = %lu\n", Uses->Size());

/******** KERNEL 2 - Prepare graph for influence maximization kernel ********/
  time1 = my_timer();      

  bool cmplx;
  uint64_t product       = 8486;     // coffee id
  auto CoffeeTraders     = TraderVertexType::Create(MEDIUM);
  auto CoffeeSales       = SaleEdgeType::Create(MEDIUM);
  auto CoffeePurchases   = PurchaseEdgeType::Create(MEDIUM);
  auto CoffeeFriends     = FriendEdgeType::Create(MEDIUM);
  auto CoffeeServers     = ServerVertexType::Create(MEDIUM);
  auto CoffeeSends       = SendEdgeType::Create(MEDIUM);
  auto CoffeeUses        = UsesEdgeType::Create(MEDIUM);
  auto CoffeeServerSends = ServerSendEdgeType::Create(MEDIUM);

  RF_args_t coffeeArgs;
  coffeeArgs.handle          = handle;
  coffeeArgs.Persons_OID     = (uint64_t) (CoffeeTraders->GetGlobalID());
  coffeeArgs.Sales_OID       = (uint64_t) (CoffeeSales->GetGlobalID());
  coffeeArgs.Purchases_OID   = (uint64_t) (CoffeePurchases->GetGlobalID());
  coffeeArgs.Friends_OID     = (uint64_t) (CoffeeFriends->GetGlobalID());
  coffeeArgs.Servers_OID     = (uint64_t) (CoffeeServers->GetGlobalID());
  coffeeArgs.Sends_OID       = (uint64_t) (CoffeeSends->GetGlobalID());
  coffeeArgs.Uses_OID        = (uint64_t) (CoffeeUses->GetGlobalID());
  coffeeArgs.ServerSends_OID = (uint64_t) (CoffeeServerSends->GetGlobalID());

  // select coffee traders, sales, and purchases
  Sales->AsyncForEachEntry(handle, SelectSalesMarket, product, coffeeArgs);
  waitForCompletion(handle);

  CoffeeTraders->AsyncWaitForBufferedInsert(handle);
  CoffeeSales->AsyncWaitForBufferedInsert(handle);
  CoffeePurchases->AsyncWaitForBufferedInsert(handle);
  waitForCompletion(handle);

  CoffeeSales->AsyncForEachEntry(handle, SaleWeights, coffeeArgs);                    // add weigths to sale edges
  waitForCompletion(handle);

  // set simple/complex scenario switch
  if (argc >= 6) cmplx = ( strcmp(argv[5], "complex") == 0 );
  else { printf("No simple/complex switch\n"); exit(-1); }

  if (cmplx) {
     CoffeeTraders->AsyncForEachEntry(handle, FriendsSubgraph, coffeeArgs, args);     // coffee friend edges
     CoffeeTraders->AsyncForEachEntry(handle, ServersSubgraph, coffeeArgs, args);     // coffee servers & use edges
     waitForCompletion(handle);

     CoffeeFriends->AsyncWaitForBufferedInsert(handle);
     CoffeeServers->AsyncWaitForBufferedInsert(handle);
     CoffeeUses->AsyncWaitForBufferedInsert(handle);
     waitForCompletion(handle);

     CoffeeServers->AsyncForEachEntry(handle, SendsSubgraph, coffeeArgs, args);       // coffee send edges
     waitForCompletion(handle);

     CoffeeSends->WaitForBufferedInsert();

     CoffeeFriends->AsyncForEachEntry(handle, FriendWeights, coffeeArgs);       // add weigths to friend edges
     CoffeeUses->AsyncForEachEntry(handle, UseWeights, coffeeArgs);             // add weigths to use edges
     CoffeeSends->AsyncForEachEntry(handle, ServerSendWeights, coffeeArgs);     // create coffee server send edges
     waitForCompletion(handle);

     CoffeeServerSends->WaitForBufferedInsert();
  }

  printf("\nTime for Kernel 2 - Coffee subgraph selection = %lf\n", my_timer() - time1);
  printf("Number of coffee traders      = %lu\n", CoffeeTraders->Size());
  printf("Number of coffee sales        = %lu\n", CoffeeSales->Size());
  printf("Number of coffee purchases    = %lu\n", CoffeePurchases->Size());

  if (cmplx) {
     printf("Number of coffee friends      = %lu\n", CoffeeFriends->Size());
     printf("Number of coffee servers      = %lu\n", CoffeeServers->Size());
     printf("Number of coffee use edges    = %lu\n", CoffeeUses->Size());
     printf("Number of coffee send edges   = %lu\n", CoffeeSends->Size());
     printf("Number of coffee server sends = %lu\n", CoffeeServerSends->Size());
  }

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
        printf("weighted friend edges printed ...\n");

        for (auto loc : shad::rt::allLocalities()) rt::executeAt(loc, PrintUsesEdges, coffeeArgs);
        printf("weighted use edges printed ...\n");

        for (auto loc : shad::rt::allLocalities()) rt::executeAt(loc, PrintServerSendEdges, coffeeArgs);
        printf("weighted server send edges printed ... exiting\n");

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
