#include <limits>
#include <string>
#include <sys/stat.h>

#include "agile/workflow4/main.h"
#include "agile/workflow4/graph.h"

namespace agile::workflow4 {

std::vector <std::string> split(std::string & line, char delim, uint64_t size = 0) {
  uint64_t ndx = 0, start = 0, end = 0;
  std::vector <std::string> tokens(size);

  for ( ; end < line.length(); end ++)  {
    if ( (line[end] == delim) || (line[end] == '\n') ) {
       tokens[ndx] = line.substr(start, end - start);
       start = end + 1;
       ndx ++;
  } }

  tokens[size - 1] = line.substr(start, end - start);     // flush last token
  return tokens;
}

void SelectSalesMarket(Handle & handle, const uint64_t & id,
     std::vector<SaleEdge> & sales, uint64_t & product, RF_args_t & args) {
  auto CoffeeTraders   = TraderVertexType::GetPtr( (TraderVertexOID) args.Persons_OID);
  auto CoffeeSales     = SaleEdgeType::GetPtr( (SaleEdgeOID) args.Sales_OID);
  auto CoffeePurchases = PurchaseEdgeType::GetPtr( (PurchaseEdgeOID) args.Purchases_OID);

  uint64_t retail = (uint64_t) TYPES::RETAIL;
  uint64_t grower = (uint64_t) TYPES::PRODUCER;
  uint64_t distributor = (uint64_t) TYPES::DISTRIBUTOR;

  for (auto & sale : sales) {
    if (sale.product != product) continue;

    if (sale.seller == sale.buyer) {     // self-edge represents product creation by seller, increase bought amonut
       TraderVertex vertex(sale.seller, 0.0, sale.amount, 0.0, grower);
       CoffeeTraders->BufferedAsyncInsert(handle, sale.seller, vertex);
    } else {
       PurchaseEdge purchase(sale);
       CoffeeSales->BufferedAsyncInsert(handle, sale.seller, sale);
       CoffeePurchases->BufferedAsyncInsert(handle, purchase.buyer, purchase);

       TraderVertex sV(sale.seller, sale.amount, 0.0, 0.0, distributor);
       TraderVertex bV(purchase.buyer, 0.0, purchase.amount, purchase.amount, retail);
       CoffeeTraders->BufferedAsyncInsert(handle, sale.seller, sV);
       CoffeeTraders->BufferedAsyncInsert(handle, purchase.buyer, bV);
} } }

void readFileSocial(const RF_args_t & args) {
  std::string line;
  struct stat stats;
  Handle handle = args.handle;
  std::string filename = args.filename;
  uint64_t this_locale = (uint32_t) shad::rt::thisLocality();
  uint64_t num_locales = (uint64_t) shad::rt::numLocalities();

  std::ifstream file(filename);
  if (! file.is_open()) { printf("Locale %lu cannot open file %s\n", this_locale, filename.c_str()); exit(-1); }

  stat(filename.c_str(), & stats);

  uint64_t num_bytes = stats.st_size / num_locales;             // file size / number of locales
  uint64_t start = this_locale * num_bytes;
  uint64_t end = start + num_bytes;

  if (this_locale != 0) {                                       // check for partial line
     file.seekg(start - 1);
     getline(file, line); 
     if (line[0] != '\n') start += line.size();                 // if not at start of a line, discard partial line
  } 

  if (this_locale == num_locales - 1) end = stats.st_size;      // last locale processes to end of file

  auto Friends = FriendEdgeType::GetPtr ((FriendEdgeOID) args.Friends_OID);
  auto Persons = PersonVertexType::GetPtr( (PersonVertexOID) args.Persons_OID);

  while (start < end) {
    getline(file, line);
    start += line.size() + 1;
    if (line[0] == '#') continue;                               // skip comments
    std::vector <std::string> tokens = split(line, ',', 2);     // delimiter and # tokens set for wmd data file

    PersonVertex person1(tokens[0]);
    PersonVertex person2(tokens[1]);
    Persons->BufferedAsyncInsert(handle, person1.key(), person1);
    Persons->BufferedAsyncInsert(handle, person2.key(), person2);

    FriendEdge friends(tokens);
    Friends->BufferedAsyncInsert(handle, friends.key(), friends);

    FriendEdge friends2(tokens);
    std::swap(friends2.person1, friends2.person2);
    Friends->BufferedAsyncInsert(handle, friends2.key(), friends2);
  }

  file.close();
}

void readFileCyber(const RF_args_t & args) {
  std::string line;
  struct stat stats;
  Handle handle = args.handle;
  std::string filename = args.filename;
  uint64_t this_locale = (uint32_t) shad::rt::thisLocality();
  uint64_t num_locales = (uint64_t) shad::rt::numLocalities();

  std::ifstream file(filename);
  if (! file.is_open()) { printf("Locale %lu cannot open file %s\n", this_locale, filename.c_str()); exit(-1); }

  stat(filename.c_str(), & stats);

  uint64_t num_bytes = stats.st_size / num_locales;              // file size / number of locales
  uint64_t start = this_locale * num_bytes;
  uint64_t end = start + num_bytes;

  if (this_locale != 0) {                                        // check for partial line
     file.seekg(start - 1);
     getline(file, line); 
     if (line[0] != '\n') start += line.size();                  // if not at start of a line, discard partial line
  }

  if (this_locale == num_locales - 1) end = stats.st_size;       // last locale processes to end of file

  auto Servers = ServerVertexType::GetPtr( (ServerVertexOID) args.Servers_OID);
  auto Sends   = SendEdgeType::GetPtr( (SendEdgeOID) args.Sends_OID);

  while (start < end) {
    getline(file, line);
    start += line.size() + 1;
    if (line[0] == '#') continue;                                // skip comments
    std::vector <std::string> tokens = split(line, ',', 11);     // delimiter and # tokens set for wmd data file

    SendEdge record(tokens);
    Sends->BufferedAsyncInsert(handle, record.key(), record);

    ServerVertex server1(tokens[0]);
    ServerVertex server2(tokens[1]);
    Servers->BufferedAsyncInsert(handle, server1.key(), server1);
    Servers->BufferedAsyncInsert(handle, server2.key(), server2);
  }

  file.close();
}

void readFileUses(const RF_args_t & args) {
  std::string line;
  struct stat stats;
  Handle handle = args.handle;
  std::string filename = args.filename;
  uint64_t this_locale = (uint32_t) shad::rt::thisLocality();
  uint64_t num_locales = (uint64_t) shad::rt::numLocalities();

  std::ifstream file(filename);
  if (! file.is_open()) { printf("Locale %lu cannot open file %s\n", this_locale, filename.c_str()); exit(-1); }

  stat(filename.c_str(), & stats);

  uint64_t num_bytes = stats.st_size / num_locales;             // file size / number of locales
  uint64_t start = this_locale * num_bytes;
  uint64_t end = start + num_bytes;

  if (this_locale != 0) {                                       // check for partial line
     file.seekg(start - 1);
     getline(file, line); 
     if (line[0] != '\n') start += line.size();                 // if not at start of a line, discard partial line
  } 

  if (this_locale == num_locales - 1) end = stats.st_size;      // last locale processes to end of file

  auto Servers = ServerVertexType::GetPtr( (ServerVertexOID) args.Servers_OID);
  auto Persons = PersonVertexType::GetPtr( (PersonVertexOID) args.Persons_OID);
  auto Uses    = UsesEdgeType::GetPtr( (UsesEdgeOID) args.Uses_OID);

  while (start < end) {
    getline(file, line);
    start += line.size() + 1;
    if (line[0] == '#') continue;                               // skip comments
    std::vector <std::string> tokens = split(line, ',', 2);     // delimiter and # tokens set for wmd data file

    PersonVertex person(tokens[0]);
    ServerVertex server(tokens[1]);
    Persons->BufferedAsyncInsert(handle, person.key(), person);
    Servers->BufferedAsyncInsert(handle, server.key(), server);
    
    UsesEdge record(tokens);
    Uses->BufferedAsyncInsert(handle, record.key(), record);
  }

  file.close();
}

void readFileCommercial(const RF_args_t & args) {
  std::string line;
  struct stat stats;
  Handle handle = args.handle;
  std::string filename = args.filename;
  uint64_t this_locale = (uint32_t) shad::rt::thisLocality();
  uint64_t num_locales = (uint64_t) shad::rt::numLocalities();

  std::ifstream file(filename);
  if (! file.is_open()) { printf("Locale %lu cannot open file %s\n", this_locale, filename.c_str()); exit(-1); }

  stat(filename.c_str(), & stats);

  uint64_t num_bytes = stats.st_size / num_locales;             // file size / number of locales
  uint64_t start = this_locale * num_bytes;
  uint64_t end = start + num_bytes;

  if (this_locale != 0) {                                       // check for partial line
     file.seekg(start - 1);
     getline(file, line); 
     if (line[0] != '\n') start += line.size();                 // if not at start of a line, discard partial line
  } 

  if (this_locale == num_locales - 1) end = stats.st_size;      // last locale processes to end of file

  auto Persons   = PersonVertexType::GetPtr( (PersonVertexOID) args.Persons_OID);
  auto Purchases = PurchaseEdgeType::GetPtr( (PurchaseEdgeOID) args.Purchases_OID);
  auto Sales     = SaleEdgeType::GetPtr( (SaleEdgeOID) args.Sales_OID);

  while (start < end) {
    getline(file, line);
    start += line.size() + 1;
    if (line[0] == '#') continue;                               // skip comments
    std::vector <std::string> tokens = split(line, ',', 8);     // delimiter and # tokens set for wmd data file

    SaleEdge sale(tokens);
    PurchaseEdge purchase(sale);
    Sales->BufferedAsyncInsert(handle, sale.key(), sale);
    Purchases->BufferedAsyncInsert(handle, purchase.key(), purchase);

    PersonVertex seller(tokens[1]);
    PersonVertex buyer(tokens[2]);
    Persons->BufferedAsyncInsert(handle, seller.key(), seller);
    Persons->BufferedAsyncInsert(handle, buyer.key(), buyer);
  }

  file.close();
}

} // namespace agile::workflow4
