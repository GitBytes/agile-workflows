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

#include "agile/wk2_partial/main.h"
#include "agile/wk2_partial/graph.h"

namespace agile::wk2_partial {


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


    std::vector <std::string> tokens = split(dataLine, ',', 10);     // delimiter and # tokens set for wmd data file

    if (tokens[0] == "Person") {
         t = TYPES::PERSON;
         PersonVertex record(tokens);
         Persons->AsyncInsert(handle, record.key(), record);
    } else if (tokens[0] == "ForumEvent") {
         t = TYPES::FORUMEVENT;
         ForumEventVertex record(tokens);
         ForumEvents->AsyncInsert(handle, record.key(), record);
    } else if (tokens[0] == "Forum") {
         t = TYPES::FORUM;
         ForumVertex record(tokens);
         Forums->AsyncInsert(handle, record.key(), record);
    } else if (tokens[0] == "Publication") {
         t = TYPES::PUBLICATION;
         PublicationVertex record(tokens);
         Publications->AsyncInsert(handle, record.key(), record);
    } else if (tokens[0] == "Topic") {
         t = TYPES::TOPIC;
         TopicVertex record(tokens);
         Topics->AsyncInsert(handle, record.key(), record);
    } else if (tokens[0] == "Sale") {
         t = TYPES::SALE;
         SaleEdge record(tokens);
         Sales->AsyncInsert(handle, record.key(), record);
         PurchaseEdge record1(tokens);
         Purchases->AsyncInsert(handle, record1.key(), record1);
    } else if (tokens[0] == "Author") {
         t = TYPES::AUTHOR;
         AuthorEdge record(tokens);
         Authors->AsyncInsert(handle, record.key(), record);
    } else if (tokens[0] == "Includes") {
         t = TYPES::INCLUDES;
         IncludesEdge record(tokens);
         Includes->AsyncInsert(handle, record.key(), record);
    } else if (tokens[0] == "HasTopic") {
         t = TYPES::HASTOPIC;
         HasTopicEdge record(tokens);
         HasTopic->AsyncInsert(handle, record.key(), record);
    } else if (tokens[0] == "HasOrg") {
         t = TYPES::HASORG;
         HasOrgEdge record(tokens);
         HasOrg->AsyncInsert(handle, record.key(), record);
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

TYPES insertToGraphBuffered(Handle & handle,std::string & dataLine, Graph_t & graph) {
//   shad::rt::Handle handle;
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


    std::vector <std::string> tokens = split(dataLine, ',', 10);     // delimiter and # tokens set for wmd data file

    if (tokens[0] == "Person") {
         t = TYPES::PERSON;
         PersonVertex record(tokens);
         Persons->BufferedAsyncInsert(handle, record.key(), record);
    } else if (tokens[0] == "ForumEvent") {
         t = TYPES::FORUMEVENT;
         ForumEventVertex record(tokens);
         ForumEvents->BufferedAsyncInsert(handle, record.key(), record);
    } else if (tokens[0] == "Forum") {
         t = TYPES::FORUM;
         ForumVertex record(tokens);
         Forums->BufferedAsyncInsert(handle, record.key(), record);
    } else if (tokens[0] == "Publication") {
         t = TYPES::PUBLICATION;
         PublicationVertex record(tokens);
         Publications->BufferedAsyncInsert(handle, record.key(), record);
    } else if (tokens[0] == "Topic") {
         t = TYPES::TOPIC;
         TopicVertex record(tokens);
         Topics->BufferedAsyncInsert(handle, record.key(), record);
    } else if (tokens[0] == "Sale") {
         t = TYPES::SALE;
         SaleEdge record(tokens);
         Sales->BufferedAsyncInsert(handle, record.key(), record);
         PurchaseEdge record1(tokens);
         Purchases->BufferedAsyncInsert(handle, record1.key(), record1);
    } else if (tokens[0] == "Author") {
         t = TYPES::AUTHOR;
         AuthorEdge record(tokens);
         Authors->BufferedAsyncInsert(handle, record.key(), record);
    } else if (tokens[0] == "Includes") {
         t = TYPES::INCLUDES;
         IncludesEdge record(tokens);
         Includes->BufferedAsyncInsert(handle, record.key(), record);
    } else if (tokens[0] == "HasTopic") {
         t = TYPES::HASTOPIC;
         HasTopicEdge record(tokens);
         HasTopic->BufferedAsyncInsert(handle, record.key(), record);
    } else if (tokens[0] == "HasOrg") {
         t = TYPES::HASORG;
         HasOrgEdge record(tokens);
         HasOrg->BufferedAsyncInsert(handle, record.key(), record);
    }

    //shad::rt::waitForCompletion(handle);

  return t;
}


} // namespace agile::wk2_partial
