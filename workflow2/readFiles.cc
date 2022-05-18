#include <limits>
#include <string>

#include "agile/workflow2/main.h"
#include "agile/workflow2/graph.h"

namespace agile::workflow2 {

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
  auto GlobalIDS    = GlobalIDType::GetPtr( (GlobalIDOID) graph["GlobalIDS"] );

  while (getline(file, line)) {
    if (line[0] == '#') continue;     // skip comments
    std::vector <std::string> tokens = split(line, ',', 10);     // delimiter and # tokens set for wmd data file

    if (tokens[0] == "Person") {
         PersonVertex record(tokens);
         Persons->BufferedAsyncInsert(handle, record.key(), record);
         GlobalIDS->BufferedAsyncInsert(handle, record.key(), Vertex(0, 0, TYPES::PERSON));
    } else if (tokens[0] == "ForumEvent") {
         ForumEventVertex record(tokens);
         ForumEvents->BufferedAsyncInsert(handle, record.key(), record);
         GlobalIDS->BufferedAsyncInsert(handle, record.key(), Vertex(0, 0, TYPES::FORUMEVENT));
    } else if (tokens[0] == "Forum") {
         ForumVertex record(tokens);
         Forums->BufferedAsyncInsert(handle, record.key(), record);
         GlobalIDS->BufferedAsyncInsert(handle, record.key(), Vertex(0, 0, TYPES::FORUM));
    } else if (tokens[0] == "Publication") {
         PublicationVertex record(tokens);
         Publications->BufferedAsyncInsert(handle, record.key(), record);
         GlobalIDS->BufferedAsyncInsert(handle, record.key(), Vertex(0, 0, TYPES::PUBLICATION));
    } else if (tokens[0] == "Topic") {
         TopicVertex record(tokens);
         Topics->BufferedAsyncInsert(handle, record.key(), record);
         GlobalIDS->BufferedAsyncInsert(handle, record.key(), Vertex(0, 0, TYPES::TOPIC));
    } else if (tokens[0] == "Sale") {
         SaleEdge record(tokens);
         Sales->BufferedAsyncInsert(handle, record.key(), record);
         GlobalIDS->BufferedAsyncInsert(handle, record.src(), Vertex(0, 1, record.src_type));
         GlobalIDS->BufferedAsyncInsert(handle, record.dst(), Vertex(0, 0, record.dst_type));

         PurchaseEdge record1(tokens);
         Purchases->BufferedAsyncInsert(handle, record1.key(), record1);
         GlobalIDS->BufferedAsyncInsert(handle, record1.src(), Vertex(0, 1, record1.src_type));
         GlobalIDS->BufferedAsyncInsert(handle, record1.dst(), Vertex(0, 0, record1.dst_type));
    } else if (tokens[0] == "Author") {
         AuthorEdge record(tokens);
         Authors->BufferedAsyncInsert(handle, record.key(), record);
         GlobalIDS->BufferedAsyncInsert(handle, record.src(), Vertex(0, 1, record.src_type));
         GlobalIDS->BufferedAsyncInsert(handle, record.dst(), Vertex(0, 0, record.dst_type));
    } else if (tokens[0] == "Includes") {
         IncludesEdge record(tokens);
         Includes->BufferedAsyncInsert(handle, record.key(), record);
         GlobalIDS->BufferedAsyncInsert(handle, record.src(), Vertex(0, 1, record.src_type));
         GlobalIDS->BufferedAsyncInsert(handle, record.dst(), Vertex(0, 0, record.dst_type));
    } else if (tokens[0] == "HasTopic") {
         HasTopicEdge record(tokens);
         HasTopic->BufferedAsyncInsert(handle, record.key(), record);
         GlobalIDS->BufferedAsyncInsert(handle, record.src(), Vertex(0, 1, record.src_type));
         GlobalIDS->BufferedAsyncInsert(handle, record.dst(), Vertex(0, 0, record.dst_type));
    } else if (tokens[0] == "HasOrg") {
         HasOrgEdge record(tokens);
         HasOrg->BufferedAsyncInsert(handle, record.key(), record);
         GlobalIDS->BufferedAsyncInsert(handle, record.src(), Vertex(0, 1, record.src_type));
         GlobalIDS->BufferedAsyncInsert(handle, record.dst(), Vertex(0, 0, record.dst_type));
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
  GlobalIDS->WaitForBufferedInsert();

  file.close();
}


TYPES insertToGraph(std::string & dataLine, Graph_t & graph) {
  shad::rt::Handle handle;
  TYPES t = TYPES::NONE;

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
  auto GlobalIDS    = GlobalIDType::GetPtr( (GlobalIDOID) graph["GlobalIDS"] );


    std::vector <std::string> tokens = split(dataLine, ',', 10);     // delimiter and # tokens set for wmd data file

    if (tokens[0] == "Person") {
         PersonVertex record(tokens);
         Persons->AsyncInsert(handle, record.key(), record);
         GlobalIDS->AsyncInsert(handle, record.key(), Vertex(0, 0, TYPES::PERSON));
    } else if (tokens[0] == "ForumEvent") {
         ForumEventVertex record(tokens);
         ForumEvents->AsyncInsert(handle, record.key(), record);
         GlobalIDS->AsyncInsert(handle, record.key(), Vertex(0, 0, TYPES::FORUMEVENT));
    } else if (tokens[0] == "Forum") {
         ForumVertex record(tokens);
         Forums->AsyncInsert(handle, record.key(), record);
         GlobalIDS->AsyncInsert(handle, record.key(), Vertex(0, 0, TYPES::FORUM));
    } else if (tokens[0] == "Publication") {
         PublicationVertex record(tokens);
         Publications->AsyncInsert(handle, record.key(), record);
         GlobalIDS->AsyncInsert(handle, record.key(), Vertex(0, 0, TYPES::PUBLICATION));
    } else if (tokens[0] == "Topic") {
         TopicVertex record(tokens);
         Topics->AsyncInsert(handle, record.key(), record);
         GlobalIDS->AsyncInsert(handle, record.key(), Vertex(0, 0, TYPES::TOPIC));
    } else if (tokens[0] == "Sale") {
         SaleEdge record(tokens);
         Sales->AsyncInsert(handle, record.key(), record);
         GlobalIDS->AsyncInsert(handle, record.src(), Vertex(0, 1, record.src_type));
         GlobalIDS->AsyncInsert(handle, record.dst(), Vertex(0, 0, record.dst_type));

         PurchaseEdge record1(tokens);
         Purchases->AsyncInsert(handle, record1.key(), record1);
         GlobalIDS->AsyncInsert(handle, record1.src(), Vertex(0, 1, record1.src_type));
         GlobalIDS->AsyncInsert(handle, record1.dst(), Vertex(0, 0, record1.dst_type));
    } else if (tokens[0] == "Author") {
         AuthorEdge record(tokens);
         Authors->AsyncInsert(handle, record.key(), record);
         GlobalIDS->AsyncInsert(handle, record.src(), Vertex(0, 1, record.src_type));
         GlobalIDS->AsyncInsert(handle, record.dst(), Vertex(0, 0, record.dst_type));
    } else if (tokens[0] == "Includes") {
         IncludesEdge record(tokens);
         Includes->AsyncInsert(handle, record.key(), record);
         GlobalIDS->AsyncInsert(handle, record.src(), Vertex(0, 1, record.src_type));
         GlobalIDS->AsyncInsert(handle, record.dst(), Vertex(0, 0, record.dst_type));
    } else if (tokens[0] == "HasTopic") {
         HasTopicEdge record(tokens);
         HasTopic->AsyncInsert(handle, record.key(), record);
         GlobalIDS->AsyncInsert(handle, record.src(), Vertex(0, 1, record.src_type));
         GlobalIDS->AsyncInsert(handle, record.dst(), Vertex(0, 0, record.dst_type));
    } else if (tokens[0] == "HasOrg") {
         HasOrgEdge record(tokens);
         HasOrg->AsyncInsert(handle, record.key(), record);
         GlobalIDS->AsyncInsert(handle, record.src(), Vertex(0, 1, record.src_type));
         GlobalIDS->AsyncInsert(handle, record.dst(), Vertex(0, 0, record.dst_type));
    }

    shad::rt::waitForCompletion(handle);

//   Persons->WaitForBufferedInsert();
//   ForumEvents->WaitForBufferedInsert();
//   Forums->WaitForBufferedInsert();
//   Publications->WaitForBufferedInsert();
//   Topics->WaitForBufferedInsert();

//   Purchases->WaitForBufferedInsert();
//   Sales->WaitForBufferedInsert();
//   Authors->WaitForBufferedInsert();
//   Includes->WaitForBufferedInsert();
//   HasTopic->WaitForBufferedInsert();
//   HasOrg->WaitForBufferedInsert();
//   GlobalIDS->WaitForBufferedInsert();

  return t;
}

} // namespace agile::workflow2
