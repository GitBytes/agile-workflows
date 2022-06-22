#include <iomanip>
#include <iostream>
#include <string>

#include "agile/wk2_approx/main.h"
#include "agile/wk2_approx/graph.h"

namespace shad {
  using namespace agile::wk2_approx;
  using Weight = std::pair<uint64_t, double>;
  
int main(int argc, char *argv[]) {
  Handle handle;
  std::string patternFile = argv[1];
  std::string dataFile    = argv[2];
  uint64_t Top_K = std::stod(argv[3]);

  double time1 = my_timer();

// CONSTRUCT PATTERN GRAPH
  Graph_t A;
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

  A["Persons"]      = (uint64_t) (Persons->GetGlobalID());
  A["ForumEvents"]  = (uint64_t) (ForumEvents->GetGlobalID());
  A["Forums"]       = (uint64_t) (Forums->GetGlobalID());
  A["Publications"] = (uint64_t) (Publications->GetGlobalID());
  A["Topics"]       = (uint64_t) (Topics->GetGlobalID());

  A["Purchases"]    = (uint64_t) (Purchases->GetGlobalID());
  A["Sales"]        = (uint64_t) (Sales->GetGlobalID());
  A["Authors"]      = (uint64_t) (Authors->GetGlobalID());
  A["Includes"]     = (uint64_t) (Includes->GetGlobalID());
  A["HasTopic"]     = (uint64_t) (HasTopic->GetGlobalID());
  A["HasOrg"]       = (uint64_t) (HasOrg->GetGlobalID());

  RF_args_t args;
  args.Persons_OID = A["Persons"];
  args.ForumEvents_OID = A["ForumEvents"];
  args.Forums_OID = A["Forums"];
  args.Publications_OID = A["Publications"];
  args.Topics_OID = A["Topics"];
  args.Purchases_OID = A["Purchases"];
  args.Sales_OID = A["Sales"];
  args.Authors_OID = A["Authors"];
  args.Includes_OID = A["Includes"];
  args.HasTopic_OID = A["HasTopic"];
  args.HasOrg_OID = A["HasOrg"];
  memcpy(args.filename, patternFile.c_str(), patternFile.size() + 1);

  printf("Reading pattern file %s\n",  patternFile.c_str());    // read file, create tables, assign locale ids
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

  uint64_t A_num_vertices = 
     Persons->Size() + ForumEvents->Size() + Forums->Size() + Publications->Size() + Topics->Size();
  uint64_t A_num_edges = 
     Purchases->Size() + Sales->Size() + Authors->Size() + Includes->Size() + HasTopic->Size() + HasOrg->Size();

  printf("Time to read Pattern File = %lf\n", my_timer() - time1);
  printf("Pattern Graph has %lu vertices and %lu edges\n\n", A_num_vertices, A_num_edges);
  time1 = my_timer();

// CONSTRUCT DATA GRAPH
  Graph_t B;
  Persons      = PersonVertexType::Create(MEDIUM);
  ForumEvents  = ForumEventVertexType::Create(MEDIUM);
  Forums       = ForumVertexType::Create(SMALL);
  Publications = PublicationVertexType::Create(SMALL);
  Topics       = TopicVertexType::Create(SMALL);

  Purchases    = PurchaseEdgeType::Create(MEDIUM);
  Sales        = SaleEdgeType::Create(MEDIUM);
  Authors      = AuthorEdgeType::Create(LARGE);
  Includes     = IncludesEdgeType::Create(LARGE);
  HasTopic     = HasTopicEdgeType::Create(LARGE);
  HasOrg       = HasOrgEdgeType::Create(MEDIUM);

  B["Persons"]      = (uint64_t) (Persons->GetGlobalID());
  B["ForumEvents"]  = (uint64_t) (ForumEvents->GetGlobalID());
  B["Forums"]       = (uint64_t) (Forums->GetGlobalID());
  B["Publications"] = (uint64_t) (Publications->GetGlobalID());
  B["Topics"]       = (uint64_t) (Topics->GetGlobalID());

  B["Purchases"]    = (uint64_t) (Purchases->GetGlobalID());
  B["Sales"]        = (uint64_t) (Sales->GetGlobalID());
  B["Authors"]      = (uint64_t) (Authors->GetGlobalID());
  B["Includes"]     = (uint64_t) (Includes->GetGlobalID());
  B["HasTopic"]     = (uint64_t) (HasTopic->GetGlobalID());
  B["HasOrg"]       = (uint64_t) (HasOrg->GetGlobalID());

  args.Persons_OID = B["Persons"];
  args.ForumEvents_OID = B["ForumEvents"];
  args.Forums_OID = B["Forums"];
  args.Publications_OID = B["Publications"];
  args.Topics_OID = B["Topics"];
  args.Purchases_OID = B["Purchases"];
  args.Sales_OID = B["Sales"];
  args.Authors_OID = B["Authors"];
  args.Includes_OID = B["Includes"];
  args.HasTopic_OID = B["HasTopic"];
  args.HasOrg_OID = B["HasOrg"];
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

  uint64_t B_num_vertices = 
     Persons->Size() + ForumEvents->Size() + Forums->Size() + Publications->Size() + Topics->Size();
  uint64_t B_num_edges = 
     Purchases->Size() + Sales->Size() + Authors->Size() + Includes->Size() + HasTopic->Size() + HasOrg->Size();

  printf("Time to read Data File = %lf\n", my_timer() - time1);
  printf("Data Graph has %lu vertices and %lu edges\n\n", B_num_vertices, B_num_edges);
  time1 = my_timer();

// CONSTRUCT BIPARTITE VERTICES WITH EDGES
  auto LHS = VertexType::Create(TINY);
  auto RHS = VertexType::Create(LARGE);
  uint64_t LHS_OID = (uint64_t) (LHS->GetGlobalID());
  uint64_t RHS_OID = (uint64_t) (RHS->GetGlobalID());
  createBipartite(A, B, LHS_OID, RHS_OID);
  printf("Time to construct bipartite graph = %lf\n", my_timer() - time1);
  
  time1 = my_timer();

  for (uint64_t i = 0; i < Top_K; ++ i) ApproxMatching(LHS_OID, RHS_OID);
  printf("Time to Match = %lf\n", my_timer() - time1);
  return 0;

}

} /// namespace
