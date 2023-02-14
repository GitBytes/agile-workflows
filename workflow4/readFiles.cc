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

void readFileCoffee(Handle & handle, const RF_args_t & args) {
  std::string line;
  struct stat stats;
  std::string filename = args.filename;
  uint64_t this_locale = (uint32_t) shad::rt::thisLocality();
  uint64_t num_locales = (uint64_t) shad::rt::numLocalities();

  std::ifstream file(filename);
  if (! file.is_open()) { printf("Locale %lu cannot open file %s\n", this_locale, filename.c_str()); exit(-1); }

  stat(filename.c_str(), & stats);

  uint64_t num_bytes = stats.st_size / num_locales;                      // file size / number of locales
  uint64_t start = this_locale * num_bytes;
  uint64_t end = start + num_bytes;

  if (this_locale != 0) {                                       // check for partial line
     file.seekg(start - 1);
     getline(file, line); 
     if (line[0] != '\n') start += line.size();                 // if not at start of a line, discard partial line
  } 

  if (this_locale == num_locales - 1) end = stats.st_size;      // last locale processes to end of file

  auto CoffeeTraders   = TraderVertexType::GetPtr( (TraderVertexOID) args.CoffeeTraders_OID);
  auto CoffeeSales     = SaleEdgeType::GetPtr( (SaleEdgeOID) args.CoffeeSales_OID);
  auto CoffeePurchases = PurchaseEdgeType::GetPtr( (PurchaseEdgeOID) args.CoffeePurchases_OID);

  while (start < end) {
    getline(file, line);
    start += line.size() + 1;
    if (line[0] == '#') continue;                               // skip comments
    std::vector <std::string> tokens = split(line, ',', 8);     // delimiter and # tokens set for wmd data file

    if (tokens[0] == "Sale") {
       SaleEdge sale(tokens);
       uint64_t id = sale.seller;
       CoffeeSales->BufferedAsyncInsert(handle, id, sale);
       CoffeeTraders->BufferedAsyncInsert(handle, id, TraderVertex(id, sale.amount, 0.0, 0.0));
       // Insert into Persons vertex list?

    } else if (tokens[0] == "Purchase") {
       PurchaseEdge purchase(tokens);
       uint64_t id  = purchase.buyer;
       CoffeePurchases->BufferedAsyncInsert(handle, id, purchase);
       CoffeeTraders->BufferedAsyncInsert(handle, id, TraderVertex(id, 0.0, purchase.amount, purchase.amount));
       // Insert into Persons vertex list?
  } }

  file.close();
}

void readFileSocial(Handle & handle, const RF_args_t & args) {
  std::string line;
  struct stat stats;
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

  auto Friends = FriendOfEdgeType::GetPtr ((FriendOfEdgeOID) args.Friends_OID);
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

    FriendOfEdge friends(tokens);
    Friends->BufferedAsyncInsert(handle, friends.key(), friends);

    FriendOfEdge friends2(tokens);
    std::swap(friends2.person1, friends2.person2);
    Friends->BufferedAsyncInsert(handle, friends2.key(), friends2);
  }

  file.close();
}

void readFileCyber(Handle & handle, const RF_args_t & args) {
  std::string line;
  struct stat stats;
  std::string filename = args.filename;
  uint64_t this_locale = (uint32_t) shad::rt::thisLocality();
  uint64_t num_locales = (uint64_t) shad::rt::numLocalities();

  std::ifstream file(filename);
  if (! file.is_open()) { printf("Locale %lu cannot open file %s\n", this_locale, filename.c_str()); exit(-1); }

  stat(filename.c_str(), & stats);

  uint64_t num_bytes = stats.st_size / num_locales;              // file size / number of locales
  uint64_t start = this_locale * num_bytes;
  uint64_t end = start + num_bytes;
  uint64_t num_lines = 0;

  if (this_locale != 0) {                                        // check for partial line
     file.seekg(start - 1);
     getline(file, line); 
     if (line[0] != '\n') start += line.size();                  // if not at start of a line, discard partial line
  }

  if (this_locale == num_locales - 1) end = stats.st_size;       // last locale processes to end of file

  auto Servers = ServerVertexType::GetPtr( (ServerVertexOID) args.Servers_OID);
  auto Sends   = SendsEdgeType::GetPtr( (SendsEdgeOID) args.Sends_OID);

  while (start < end) {
    num_lines ++;
    getline(file, line);
    start += line.size() + 1;
    if (line[0] == '#') continue;                                // skip comments
    std::vector <std::string> tokens = split(line, ',', 11);     // delimiter and # tokens set for wmd data file

    SendsEdge record(tokens);
    Sends->BufferedAsyncInsert(handle, record.key(), record);

    ServerVertex server1(tokens[0]);
    ServerVertex server2(tokens[1]);
    Servers->BufferedAsyncInsert(handle, server1.key(), server1);
    Servers->BufferedAsyncInsert(handle, server2.key(), server2);
  }

  file.close();
}

void readFileUses(Handle & handle, const RF_args_t & args) {
  std::string line;
  struct stat stats;
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

void readFileCommercial(Handle & handle, const RF_args_t & args) {
  std::string line;
  struct stat stats;
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
    Sales->BufferedAsyncInsert(handle, sale.key(), sale);

    PurchaseEdge purchase(tokens);
    std::swap(purchase.buyer, purchase.seller);
    Purchases->BufferedAsyncInsert(handle, purchase.key(), purchase);

    PersonVertex seller(tokens[1]);
    PersonVertex buyer(tokens[2]);
    Persons->BufferedAsyncInsert(handle, seller.key(), seller);
    Persons->BufferedAsyncInsert(handle, buyer.key(), buyer);
  }

  file.close();
}

} // namespace agile::workflow4
