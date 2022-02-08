#include <vector>
#include <string>

#include "shad/runtime/runtime.h"

#include "agile/workflow1/main.h"

namespace agile::workflow1 {

std::vector <std::string> split(std::string & line, char delim, uint64_t size = 0) {
  uint64_t ndx = 0, start = 0, end = 0;
  std::vector <std::string> tokens(size);

  for (end = 0; end < line.length(); end ++) {

    if (line[end] == delim) {
       tokens[ndx] = line.substr(start, end - start);
       start = end + 1;
       ndx ++;
  } }

  tokens[ndx] = line.substr(start, end - start);     // add last token
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

  auto Persons      = PersonVertexType::GetPtr( (PersonVertexOID) graph[TYPES::PERSON] );
  auto ForumEvents  = ForumEventVertexType::GetPtr( (ForumEventVertexOID) graph[TYPES::FORUMEVENT] );
  auto Forums       = ForumVertexType::GetPtr( (ForumVertexOID) graph[TYPES::FORUM ] );
  auto Publications = PublicationVertexType::GetPtr( (PublicationVertexOID) graph[TYPES::PUBLICATION] );
  auto Topics       = TopicVertexType::GetPtr( (TopicVertexOID) graph[TYPES::TOPIC] );

  auto Purchases    = PurchaseEdgeType::GetPtr( (PurchaseEdgeOID) graph[TYPES::PURCHASE] );
  auto Sales        = SaleEdgeType::GetPtr( (SaleEdgeOID) graph[TYPES::SALE] );
  auto Authors      = AuthorEdgeType::GetPtr( (AuthorEdgeOID) graph[TYPES::AUTHOR] );
  auto OccursAt     = OccursAtEdgeType::GetPtr( (OccursAtEdgeOID) graph[TYPES::OCCURSAT] );
  auto HasTopic     = HasTopicEdgeType::GetPtr( (HasTopicEdgeOID) graph[TYPES::HASTOPIC] );
  auto HasOrg       = HasOrgEdgeType::GetPtr( (HasOrgEdgeOID) graph[TYPES::HASORG] );

  auto Edges        = EdgeType::GetPtr( (EdgeOID) graph[TYPES::EDGE] );
  auto Vertices     = VertexType::GetPtr( (VertexOID) graph[TYPES::VERTEX] );

  while (getline(file, line)) {
    if (line[0] == '#') continue;     // skip comments
    std::vector <std::string> tokens = split(line, ',', size);

    if (tokens[0] == "Person") {
         PersonVertex record(tokens);
         Vertex vertex(TYPES::PERSON);
         Persons->BufferedAsyncInsert(handle, record.get_key(), record);
         Vertices->BufferedAsyncInsert(handle, record.get_key(), vertex);

    } else if (tokens[0] == "ForumEvent") {
         ForumEventVertex record(tokens);
         Vertex vertex(TYPES::FORUMEVENT);
         ForumEvents->BufferedAsyncInsert(handle, record.get_key(), record);
         Vertices->BufferedAsyncInsert(handle, record.get_key(), vertex);

    } else if (tokens[0] == "Forum") {
         ForumVertex record(tokens);
         Vertex vertex(TYPES::FORUM);
         Forums->BufferedAsyncInsert(handle, record.get_key(), record);
         Vertices->BufferedAsyncInsert(handle, record.get_key(), vertex);

    } else if (tokens[0] == "Publication") {
         PublicationVertex record(tokens);
         Vertex vertex(TYPES::PUBLICATION);
         Publications->BufferedAsyncInsert(handle, record.get_key(), record);
         Vertices->BufferedAsyncInsert(handle, record.get_key(), vertex);

    } else if (tokens[0] == "Topic") {
         TopicVertex record(tokens);
         Vertex vertex(TYPES::TOPIC);
         Topics->BufferedAsyncInsert(handle, record.get_key(), record);
         Vertices->BufferedAsyncInsert(handle, record.get_key(), vertex);

    } else if (tokens[0] == "Purchase") {
         PurchaseEdge record(tokens);
         Edge edge(record.get_src(), record.get_dst(), TYPES::PURCHASE);
         Purchases->BufferedAsyncInsert(handle, record.get_key(), record);
         Edges->BufferedAsyncInsert(handle, record.get_src(), edge);

    } else if (tokens[0] == "Sale") {
         SaleEdge record(tokens);
         Edge edge(record.get_src(), record.get_dst(), TYPES::SALE);
         Sales->BufferedAsyncInsert(handle, record.get_key(), record);
         Edges->BufferedAsyncInsert(handle, record.get_src(), edge);

    } else if (tokens[0] == "Author") {
         AuthorEdge record(tokens);
         Edge edge(record.get_src(), record.get_dst(), TYPES::AUTHOR);
         Authors->BufferedAsyncInsert(handle, record.get_key(), record);
         Edges->BufferedAsyncInsert(handle, record.get_src(), edge);

    } else if (tokens[0] == "OccursAt") {
         OccursAtEdge record(tokens);
         Edge edge(record.get_src(), record.get_dst(), TYPES::OCCURSAT);
         OccursAt->BufferedAsyncInsert(handle, record.get_key(), record);
         Edges->BufferedAsyncInsert(handle, record.get_src(), edge);

    } else if (tokens[0] == "HasTopic") {
         HasTopicEdge record(tokens);
         Edge edge(record.get_src(), record.get_dst(), TYPES::HASTOPIC);
         HasTopic->BufferedAsyncInsert(handle, record.get_key(), record);
         Edges->BufferedAsyncInsert(handle, record.get_src(), edge);

    } else if (tokens[0] == "HasOrg") {
         HasOrgEdge record(tokens);
         Edge edge(record.get_src(), record.get_dst(), TYPES::HASORG);
         HasOrg->BufferedAsyncInsert(handle, record.get_key(), record);
         Edges->BufferedAsyncInsert(handle, record.get_src(), edge);
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
  OccursAt->WaitForBufferedInsert();
  HasTopic->WaitForBufferedInsert();
  HasOrg->WaitForBufferedInsert();

  Edges->WaitForBufferedInsert();
}

}
