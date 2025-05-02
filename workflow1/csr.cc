#include "agile/workflow1/main.h"
#include "agile/workflow1/graph.h"

namespace agile::workflow1 {

struct Args_t { uint64_t delta; uint64_t oid; };

// Update the global ids on this locale and spawn updateIDS on next locale.
void updateIDS(Handle & handle, const Args_t & args) {
  auto GlobalIDS = GlobalIDType::GetPtr((GlobalIDOID) args.oid);
  auto updateLambda = [] (const uint64_t & key, Vertex & value, const uint64_t & delta) {value.id += delta;};

  auto locale = (uint32_t) shad::rt::thisLocality();
  auto my_map = GlobalIDS->GetLocalHashmap();

  if (locale < shad::rt::numLocalities() - 1) {     // if not last locale, spawn updateIDS on next locale
    Args_t my_args = {args.delta + my_map->Size(), args.oid};
    shad::rt::asyncExecuteAt(handle, shad::rt::Locality(locale + 1), updateIDS, my_args);
  }

  my_map->ForEachEntry(updateLambda, args.delta);
}


// Fill in Vertices ... insert {key, value.edges, value.type} at index value.id
void moveVertex(Handle & handle, const uint64_t & key, Vertex & value, uint64_t & verticesOID) {
  auto Vertices = VertexType::GetPtr((VertexOID) verticesOID);
  Vertices->AsyncInsertAt(handle, value.id, Vertex(key, value.edges, value.type));
}


// Fill in dst global id and fire-and-forget insert
void Dst_(Handle & handle, const uint64_t & i, Vertex & dstV, uint64_t & ndx, Edge & edge, uint64_t & XEdgesOID) {
  auto XEdges = XEdgeType::GetPtr((XEdgeOID) XEdgesOID);

  edge.dst_glbid = dstV.id;
  XEdges->AsyncInsertAt(handle, ndx, edge);
};


// Move edges from Edges to XEdges
void moveEdges(Handle & handle, const uint64_t & src_id, std::vector<Edge> & edges,
     uint64_t & globalIDSOID, uint64_t & verticesOID, uint64_t & XEdgesOID) {
  auto Vertices  = VertexType::GetPtr((VertexOID) verticesOID);
  auto GlobalIDS = GlobalIDType::GetPtr((GlobalIDOID) globalIDSOID);

  Vertex srcVertex;
  GlobalIDS->Lookup(src_id, & srcVertex);                                    // lookup global id for src vertex
  uint64_t ndx = Vertices->At(srcVertex.id).start;                           // start index for src vertex edges

  for (auto & edge : edges) {                                                // for each edge of src vertex
    edge.src_glbid = srcVertex.id;
    GlobalIDS->AsyncApply(handle, edge.dst, Dst_, ndx, edge, XEdgesOID);     // ... send edge to dst vertex and forget
    ndx ++;                                                                  // ... increment edge index
} }



/********** CREATE COMPRESSED EDGE ARRAY AND VERTEX ARRAY **********/
void CSR(Graph_t graph, uint64_t num_vertices, uint64_t num_edges) {

// ***** convert local ids to global ids *****/
  Handle handle;
  uint64_t EdgesOID = graph["Edges"];
  uint64_t GlobalIDSOID = graph["GlobalIDS"];
  auto Edges = EdgeType::GetPtr((EdgeOID) EdgesOID);
  auto GlobalIDS = GlobalIDType::GetPtr((GlobalIDOID) GlobalIDSOID);

  Args_t my_args = {0, GlobalIDSOID};
  shad::rt::asyncExecuteAt(handle, shad::rt::Locality(0), updateIDS, my_args);

// ***** allocate space for Compressed Edges and Vertices *****/
  auto XEdges = XEdgeType::Create(num_edges, Edge());
  auto Vertices = VertexType::Create(num_vertices + 1, Vertex());
  graph["XEdges"] = (uint64_t) (XEdges->GetGlobalID());
  graph["Vertices"] = (uint64_t) (Vertices->GetGlobalID());

  XEdges->FillPtrs();
  Vertices->FillPtrs();
  shad::rt::waitForCompletion(handle);

// ***** copy vertices from GlobalIDS to Vertices *****/
  GlobalIDS->AsyncForEachEntry(handle, moveVertex, graph["Vertices"]);
  waitForCompletion(handle);

  exclusiveScanVertices<Vertex>(graph["Vertices"]);     // convert # edges to start location

// ***** move edges from Edges to XEdges *****/
  Edges->AsyncForEachEntry(handle, moveEdges, GlobalIDSOID, graph["Vertices"], graph["XEdges"]);
  waitForCompletion(handle);
}

}
