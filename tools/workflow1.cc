#include "agile/workflow1/main.h"

using namespace agile::workflow1;

namespace shad {

int main(int argc, char *argv[]) {
  double time1 = my_timer();

  auto Persons      = PersonVertexType::Create(MEDIUM);
  auto ForumEvents  = ForumEventVertexType::Create(MEDIUM);
  auto Forums       = ForumVertexType::Create(SMALL);
  auto Publications = PublicationVertexType::Create(SMALL);
  auto Topics       = TopicVertexType::Create(SMALL);

  auto Purchases    = PurchaseEdgeType::Create(MEDIUM);
  auto Sales        = SaleEdgeType::Create(MEDIUM);
  auto Authors      = AuthorEdgeType::Create(LARGE);
  auto OccursAt     = OccursAtEdgeType::Create(MEDIUM);
  auto HasTopic     = HasTopicEdgeType::Create(LARGE);
  auto HasOrg       = HasOrgEdgeType::Create(MEDIUM);

  Graph_t graph;
  graph[TYPES::PERSON]      = (uint64_t) (Persons->GetGlobalID());
  graph[TYPES::FORUMEVENT]  = (uint64_t) (ForumEvents->GetGlobalID());
  graph[TYPES::FORUM]       = (uint64_t) (Forums->GetGlobalID());
  graph[TYPES::PUBLICATION] = (uint64_t) (Publications->GetGlobalID());
  graph[TYPES::TOPIC]       = (uint64_t) (Topics->GetGlobalID());

  graph[TYPES::PURCHASE]    = (uint64_t) (Purchases->GetGlobalID());
  graph[TYPES::SALE]        = (uint64_t) (Sales->GetGlobalID());
  graph[TYPES::AUTHOR]      = (uint64_t) (Authors->GetGlobalID());
  graph[TYPES::OCCURSAT]    = (uint64_t) (OccursAt->GetGlobalID());
  graph[TYPES::HASTOPIC]    = (uint64_t) (HasTopic->GetGlobalID());
  graph[TYPES::HASORG]      = (uint64_t) (HasOrg->GetGlobalID());

  auto Vertices        = VertexType::Create(MEDIUM);
  graph[TYPES::VERTEX] = (uint64_t) (Vertices->GetGlobalID());

  auto Edges         = EdgeType::Create(LARGE);
  graph[TYPES::EDGE] = (uint64_t) (Edges->GetGlobalID());

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
  printf("Total number of edges    = %lu\n", Edges->Size());
  printf("Total number of vertices = %lu\n", personSize + forumEventSize + forumSize + publicationSize + topicSize);

  GNN(graph);
  return 0;
}

}
