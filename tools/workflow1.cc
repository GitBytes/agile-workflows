#include "agile/workflow1/main.h"
#include "agile/workflow1/graph.h"
#include "agile/workflow1/gnn.h"

namespace shad {
  using namespace agile::workflow1;

int main(int argc, char *argv[]) {
  double time1 = my_timer();

  Handle handle;
  Graph_t graph;
  std::string dataFile = argv[1];
  auto Edges  = EdgeType::Create(LARGE);
  auto GlobalIDS = GlobalIDType::Create(MEDIUM);
  graph["Edges"] = (uint64_t) (Edges->GetGlobalID());
  graph["GlobalIDS"] = (uint64_t) (GlobalIDS->GetGlobalID());

  RF_args_t args;
  args.Edges_OID     = graph["Edges"];
  args.GlobalIDS_OID = graph["GlobalIDS"];
  memcpy(args.filename, dataFile.c_str(), dataFile.size() + 1);

  shad::rt::asyncExecuteOnAll(handle, readFile, args);
  waitForCompletion(handle);

  Edges->WaitForBufferedInsert();
  GlobalIDS->WaitForBufferedInsert();

/********** CREATE COMPRESSED EDGE ARRAY AND VERTEX ARRAY **********/
  uint64_t num_vertices = GlobalIDS->Size();
  uint64_t num_edges = Edges->Size();

  CSR(graph, num_vertices, num_edges);
  printf("Time for graph construction = %lf\n", my_timer() - time1);
  printf("Total number of vertices = %lu\n", num_vertices);
  printf("Total number of edges    = %lu\n", num_edges);

  time1 = my_timer();

  auto trainedModels = GNN(num_edges, num_vertices, graph, argv[2]);
  printf("Time for workflow 1 = %lf\n", my_timer() - time1);

  return 0;
}

}
