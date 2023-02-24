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
