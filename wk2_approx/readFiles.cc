#include "agile/wk2_approx/graph.h"

namespace agile::wk2_approx {

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
  Handle handle;
  std::string line;
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

  auto Purchases    = PurchaseEdgeType::GetPtr( (PurchaseEdgeOID) graph["Purchases"] );
  auto Sales        = SaleEdgeType::GetPtr( (SaleEdgeOID) graph["Sales"] );
  auto Authors      = AuthorEdgeType::GetPtr( (AuthorEdgeOID) graph["Authors"] );
  auto Includes     = IncludesEdgeType::GetPtr( (IncludesEdgeOID) graph["Includes"] );
  auto HasTopic     = HasTopicEdgeType::GetPtr( (HasTopicEdgeOID) graph["HasTopic"] );
  auto HasOrg       = HasOrgEdgeType::GetPtr( (HasOrgEdgeOID) graph["HasOrg"] );

  while (getline(file, line)) {
    if (line[0] == '#') continue;     // skip comments
    std::vector <std::string> tokens = split(line, ',', 10);     // delimiter and # tokens set for wmd data file

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
    } else if (tokens[0] == "Sale") {
         SaleEdge record(tokens);
         Sales->BufferedAsyncInsert(handle, record.key(), record);

         PurchaseEdge record1(tokens);
         Purchases->BufferedAsyncInsert(handle, record1.key(), record1);
    } else if (tokens[0] == "Author") {
         AuthorEdge record(tokens);
         Authors->BufferedAsyncInsert(handle, record.key(), record);
    } else if (tokens[0] == "Includes") {
         IncludesEdge record(tokens);
         Includes->BufferedAsyncInsert(handle, record.key(), record);
    } else if (tokens[0] == "HasTopic") {
         HasTopicEdge record(tokens);
         HasTopic->BufferedAsyncInsert(handle, record.key(), record);
    } else if (tokens[0] == "HasOrg") {
         HasOrgEdge record(tokens);
         HasOrg->BufferedAsyncInsert(handle, record.key(), record);
  } }

  shad::rt::waitForCompletion(handle);

  Persons->WaitForBufferedInsert();
  ForumEvents->WaitForBufferedInsert();
  Forums->WaitForBufferedInsert();
  Publications->WaitForBufferedInsert();
  Topics->WaitForBufferedInsert();

  Purchases->WaitForBufferedInsert();
  Sales->WaitForBufferedInsert();
  Authors->WaitForBufferedInsert();
  Includes->WaitForBufferedInsert();
  HasTopic->WaitForBufferedInsert();
  HasOrg->WaitForBufferedInsert();

  file.close();
}

} // namespace agile::wk2_approx
