#include <limits>
#include <string>

#include "agile/workflow2/main.h"
#include "agile/workflow2/graph.h"

namespace agile::workflow2 {
uint64_t String_to_Uint(std::string & str) {
  if (str == "") return std::numeric_limits<uint64_t>::max();

  uint64_t val;
  try {val = stoull(str);}
  catch(...) {val = 0;}
  return val;
}

double String_to_Double(std::string & str) {
  if (str == "") return std::numeric_limits<uint64_t>::max();

  double val;
  try {val = stod(str);}
  catch(...) {val = 0.0;}
  return val;
}

double String_to_Date(std::string & str) {
  if (str == "") return std::numeric_limits<uint64_t>::max();

  double val;
  struct tm date{};
  date.tm_isdst = -1;
  strptime(str.c_str(), "%m/%d/%y", & date);

  try {val = (double) mktime(& date);}
  catch(...) {val = 0.0;}

  return val;
}

std::vector <std::string> split(std::string & line, char delim, uint64_t size = 0) {
  uint64_t ndx = 0, start = 0;
  std::vector <std::string> tokens(size);

  for (uint64_t end = 0; end < line.length(); end ++) {

    if ( (line[end] == delim) || (line[end] == '\n') ) {
       tokens[ndx] = line.substr(start, end - start);
       start = end + 1;
       ndx ++;
  } }

  return tokens;
}

void readFile(std::string & filename, Graph_t & graph) {
  std::string line;
  uint64_t size = 10;
  char delimiter = ',';
  shad::rt::Handle handle;
  std::ifstream file(filename);

  if (file.is_open()) {
     printf("reading file %s\n", filename.c_str());
  } else {
     printf("Cannot open file %s\n", filename.c_str());
     exit(-1);
  }

  auto Persons      = PersonVertexType::GetPtr( (PersonVertexOID) graph["Persons"] );
  auto ForumEvents  = ForumEventVertexType::GetPtr( (ForumEventVertexOID) graph["ForumEvents"] );
  auto Forums       = ForumVertexType::GetPtr( (ForumVertexOID) graph["Forums"] );
  auto Publications = PublicationVertexType::GetPtr( (PublicationVertexOID) graph["Publications"] );
  auto Topics       = TopicVertexType::GetPtr( (TopicVertexOID) graph["Topics"] );

  auto AllEdges     = AllEdgeType::GetPtr( (AllEdgeOID) graph["AllEdges"] );
  auto Purchases    = PurchaseEdgeType::GetPtr( (PurchaseEdgeOID) graph["Purchases"] );
  auto Sales        = SaleEdgeType::GetPtr( (SaleEdgeOID) graph["Sales"] );
  auto Authors      = AuthorEdgeType::GetPtr( (AuthorEdgeOID) graph["Authors"] );
  auto OccursAt     = OccursAtEdgeType::GetPtr( (OccursAtEdgeOID) graph["OccursAt"] );
  auto HasTopic     = HasTopicEdgeType::GetPtr( (HasTopicEdgeOID) graph["HasTopics"] );
  auto HasOrg       = HasOrgEdgeType::GetPtr( (HasOrgEdgeOID) graph["HasOrgs"] );

  while (getline(file, line)) {
    if (line[0] == '#') continue;     // skip comments
    std::vector <std::string> tokens = split(line, ',', size);

    if (tokens[0] == "Person") {
         PersonVertex record(tokens);
         Persons->BufferedAsyncInsert(handle, record.key(), record);
    } else if (tokens[0] == "ForumEvent") {
         ForumEventVertex record(tokens);
         ForumEvents->BufferedAsyncInsert(handle, record.key(), record);
    } else if (tokens[0] == "Forum") {
         ForumVertex record(tokens);
         Forums->BufferedAsyncInsert(handle, record.key(), record);
    } else if (tokens[0] == "Publication") {
         PublicationVertex record(tokens);
         Publications->BufferedAsyncInsert(handle, record.key(), record);
    } else if (tokens[0] == "Topic") {
         TopicVertex record(tokens);
         Topics->BufferedAsyncInsert(handle, record.key(), record);
    } else if (tokens[0] == "Purchase") {
         PurchaseEdge record(tokens);
         Purchases->BufferedAsyncInsert(handle, record.key(), record);
         AllEdges->BufferedAsyncInsert(handle, record.src(), std::make_tuple(record.dst(), "Purchases"));
    } else if (tokens[0] == "Sale") {
         SaleEdge record(tokens);
         Sales->BufferedAsyncInsert(handle, record.key(), record);
         AllEdges->BufferedAsyncInsert(handle, record.src(), std::make_tuple(record.dst(), "Sales"));
    } else if (tokens[0] == "Author") {
         AuthorEdge record(tokens);
         Authors->BufferedAsyncInsert(handle, record.key(), record);
         AllEdges->BufferedAsyncInsert(handle, record.src(), std::make_tuple(record.dst(), "Authors"));
    } else if (tokens[0] == "OccursAt") {
         OccursAtEdge record(tokens);
         OccursAt->BufferedAsyncInsert(handle, record.key(), record);
         AllEdges->BufferedAsyncInsert(handle, record.src(), std::make_tuple(record.dst(), "OccursAt"));
    } else if (tokens[0] == "HasTopic") {
         HasTopicEdge record(tokens);
         HasTopic->BufferedAsyncInsert(handle, record.key(), record);
         AllEdges->BufferedAsyncInsert(handle, record.src(), std::make_tuple(record.dst(), "HasTopic"));
    } else if (tokens[0] == "HasOrg") {
         HasOrgEdge record(tokens);
         HasOrg->BufferedAsyncInsert(handle, record.key(), record);
         AllEdges->BufferedAsyncInsert(handle, record.src(), std::make_tuple(record.dst(), "HasOrg"));
  } }

  shad::rt::waitForCompletion(handle);

  Persons->WaitForBufferedInsert();
  ForumEvents->WaitForBufferedInsert();
  Forums->WaitForBufferedInsert();
  Publications->WaitForBufferedInsert();
  Topics->WaitForBufferedInsert();

  AllEdges->WaitForBufferedInsert();
  Purchases->WaitForBufferedInsert();
  Sales->WaitForBufferedInsert();
  Authors->WaitForBufferedInsert();
  OccursAt->WaitForBufferedInsert();
  HasTopic->WaitForBufferedInsert();
  HasOrg->WaitForBufferedInsert();
}
} // namespace agile::workflow2
