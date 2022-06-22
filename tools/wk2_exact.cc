#include "agile/wk2_exact/main.h"
#include "agile/wk2_exact/graph.h"

namespace shad {
  using namespace agile::wk2_exact;

int main(int argc, char *argv[]) {
  double time1 = my_timer();

  Handle handle;
  Graph_t graph;
  std::string dataFile = argv[1];

  auto Persons      = PersonVertexType::Create(MEDIUM);
  auto ForumEvents  = ForumEventVertexType::Create(MEDIUM);
  auto Forums       = ForumVertexType::Create(SMALL);
  auto Publications = PublicationVertexType::Create(SMALL);
  auto Topics       = TopicVertexType::Create(SMALL);
  auto Purchases    = PurchaseEdgeType::Create(MEDIUM);
  auto Sales        = SaleEdgeType::Create(MEDIUM);
  auto Authors      = AuthorEdgeType::Create(LARGE);
  auto Includes     = IncludesEdgeType::Create(LARGE);
  auto HasTopic     = HasTopicEdgeType::Create(LARGE);
  auto HasOrg       = HasOrgEdgeType::Create(MEDIUM);

  graph["Persons"]      = (uint64_t) (Persons->GetGlobalID());
  graph["ForumEvents"]  = (uint64_t) (ForumEvents->GetGlobalID());
  graph["Forums"]       = (uint64_t) (Forums->GetGlobalID());
  graph["Publications"] = (uint64_t) (Publications->GetGlobalID());
  graph["Topics"]       = (uint64_t) (Topics->GetGlobalID());
  graph["Purchases"]    = (uint64_t) (Purchases->GetGlobalID());
  graph["Sales"]        = (uint64_t) (Sales->GetGlobalID());
  graph["Authors"]      = (uint64_t) (Authors->GetGlobalID());
  graph["Includes"]     = (uint64_t) (Includes->GetGlobalID());
  graph["HasTopic"]     = (uint64_t) (HasTopic->GetGlobalID());
  graph["HasOrg"]       = (uint64_t) (HasOrg->GetGlobalID());

  RF_args_t args;
  args.Persons_OID = graph["Persons"];
  args.ForumEvents_OID = graph["ForumEvents"];
  args.Forums_OID = graph["Forums"];
  args.Publications_OID = graph["Publications"];
  args.Topics_OID = graph["Topics"];
  args.Purchases_OID = graph["Purchases"];
  args.Sales_OID = graph["Sales"];
  args.Authors_OID = graph["Authors"];
  args.Includes_OID = graph["Includes"];
  args.HasTopic_OID = graph["HasTopic"];
  args.HasOrg_OID = graph["HasOrg"];
  memcpy(args.filename, dataFile.c_str(), dataFile.size() + 1);

  printf("Reading data file %s\n",  dataFile.c_str());    // read file, create tables, assign locale ids
  shad::rt::asyncExecuteOnAll(handle, readFile, args);
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

  printf("Time for graph construction = %lf\n\n", my_timer() - time1);

  printf("Number of persons      = %lu\n", Persons->Size());
  printf("Number of forum_events = %lu\n", ForumEvents->Size());
  printf("Number of forums       = %lu\n", Forums->Size());
  printf("Number of publications = %lu\n", Publications->Size());
  printf("Number of topics       = %lu\n", Topics->Size());

  printf("\n");
  printf("Number of purchase edges = %lu\n", Purchases->Size());
  printf("Number of sale edges     = %lu\n", Sales->Size());
  printf("Number of author edges   = %lu\n", Authors->Size());
  printf("Number of include edges  = %lu\n", Includes->Size());
  printf("Number of hasTopic edges = %lu\n", HasTopic->Size());
  printf("Number of hasOrg edges   = %lu\n", HasOrg->Size());

  printf("\n");
  printf("Total number of vertices = %lu\n",
       Persons->Size() + ForumEvents->Size() + Forums->Size() + Publications->Size() + Topics->Size());
  printf("Total number of edges    = %lu\n", 
       Purchases->Size() + Sales->Size() + Authors->Size() + Includes->Size() + HasTopic->Size() + HasOrg->Size());

  time1 = my_timer();

  WMD_pattern(graph);
  printf("Time for exact pattern matching = %lf\n", my_timer() - time1);

  return 0;
}

}
