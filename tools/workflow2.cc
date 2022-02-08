#include "agile/workflow2/main.h"
#include "agile/workflow2/graph.h"

namespace shad {
  using namespace agile::workflow2;

int main(int argc, char *argv[]) {
  double time1 = my_timer();

  auto Persons      = PersonVertexType::Create(MEDIUM);
  auto ForumEvents  = ForumEventVertexType::Create(MEDIUM);
  auto Forums       = ForumVertexType::Create(SMALL);
  auto Publications = PublicationVertexType::Create(SMALL);
  auto Topics       = TopicVertexType::Create(SMALL);

  auto AllEdges     = AllEdgeType::Create(LARGE);
  auto Purchases    = PurchaseEdgeType::Create(MEDIUM);
  auto Sales        = SaleEdgeType::Create(MEDIUM);
  auto Authors      = AuthorEdgeType::Create(LARGE);
  auto OccursAt     = OccursAtEdgeType::Create(MEDIUM);
  auto HasTopic     = HasTopicEdgeType::Create(LARGE);
  auto HasOrg       = HasOrgEdgeType::Create(MEDIUM);

  Graph_t graph;
  graph["Persons"]      = (uint64_t) (Persons->GetGlobalID());
  graph["ForumEvents"]  = (uint64_t) (ForumEvents->GetGlobalID());
  graph["Forums"]       = (uint64_t) (Forums->GetGlobalID());
  graph["Publications"] = (uint64_t) (Publications->GetGlobalID());
  graph["Topics"]       = (uint64_t) (Topics->GetGlobalID());

  graph["AllEdges"]     = (uint64_t) (AllEdges->GetGlobalID());
  graph["Purchases"]    = (uint64_t) (Purchases->GetGlobalID());
  graph["Sales"]        = (uint64_t) (Sales->GetGlobalID());
  graph["Authors"]      = (uint64_t) (Authors->GetGlobalID());
  graph["OccursAt"]     = (uint64_t) (OccursAt->GetGlobalID());
  graph["HasTopic"]     = (uint64_t) (HasTopic->GetGlobalID());
  graph["HasOrg"]       = (uint64_t) (HasOrg->GetGlobalID());

  std::string dataFile = argv[1];
  readFile(dataFile, graph);
  printf("Time for graph construction = %lf\n", my_timer() - time1);

  uint64_t personSize = Persons->Size();
  uint64_t forumEventSize = ForumEvents->Size();
  uint64_t forumSize = Forums->Size();
  uint64_t publicationSize = Publications->Size();
  uint64_t topicSize = Topics->Size();

  printf("\n");
  printf("Number of persons      = %lu\n", personSize);
  printf("Number of forum_events = %lu\n", forumEventSize);
  printf("Number of forums       = %lu\n", forumSize);
  printf("Number of publications = %lu\n", publicationSize);
  printf("Number of topics       = %lu\n", topicSize);

  printf("\n");
  printf("Number of purchase edges = %lu\n", Purchases->Size());
  printf("Number of sale edges     = %lu\n", Sales->Size());
  printf("Number of author edges   = %lu\n", Authors->Size());
  printf("Number of occursAt edges = %lu\n", OccursAt->Size());
  printf("Number of hasTopic edges = %lu\n", HasTopic->Size());
  printf("Number of hasOrg edges   = %lu\n", HasOrg->Size());

  printf("\n");
  printf("Total number of edges    = %lu\n", AllEdges->Size());
  printf("Total number of vertices = %lu\n", personSize + forumEventSize + forumSize + publicationSize + topicSize);

  // WMD_pattern(graph);
  return 0;
}

}
