#include "agile/workflow2/main.h"
#include "agile/workflow2/graph.h"

namespace shad {
  using namespace agile::workflow2;

int main(int argc, char *argv[]) {
  double time1 = my_timer();

  Graph_t graph;
  std::string dataFile = argv[1];
  uint64_t num_edges, num_vertices;

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
  auto GlobalIDS    = GlobalIDType::Create(LARGE);

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
  graph["GlobalIDS"]    = (uint64_t) (GlobalIDS->GetGlobalID());

  readFile(dataFile, graph);               // read file, create vertex and edge tables, assign locale ids
  CSR(num_edges, num_vertices, graph);     // create vertex and compressed edge data structures

  printf("Time for graph construction = %lf\n", my_timer() - time1);

  printf("\n");
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
  printf("Total number of edges    = %lu\n", num_edges);
  printf("Total number of vertices = %lu\n\n", num_vertices);

  time1 = my_timer();

  WMD_pattern(graph);
  printf("Time for exact pattern matching = %lf\n", my_timer() - time1);

  for (auto itr = GlobalIDS->begin(); itr != GlobalIDS->end(); itr ++)
    printf("id = %lu glbid = %lu edges = %lu type = %lu\n", (* itr).first,
         (* itr).second.id, (* itr).second.edges, (uint64_t) (* itr).second.type);

  printf("\n***********\n");
  auto Vertices = VertexType::GetPtr((VertexOID) graph["Vertices"]);

  for (uint64_t i = 0; i < Vertices->Size(); i ++) {
    Vertex tmp = Vertices->At(i);
    printf("glbid = %lu id = %lu edges = %lu type = %lu\n", i, tmp.id, tmp.edges, (uint64_t) tmp.type);
  }

  printf("\n***********\n");
  auto Edges = EdgeType::GetPtr((EdgeOID) graph["Edges"]);

  for (uint64_t i = 0; i < Edges->Size(); i ++) {
    Edge tmp = Edges->At(i);
    printf("src = %lu dst = %lu type = %lu src_type = %lu dst_type = %lu src_glbid = %lu dst_glbid = %lu\n",
         tmp.src, tmp.dst, (uint64_t) tmp.type, (uint64_t) tmp.src_type,
         (uint64_t) tmp.dst_type, tmp.src_glbid, tmp.dst_glbid);
  }

  return 0;
}

}
