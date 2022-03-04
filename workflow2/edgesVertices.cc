#include <limits>
#include <string>

#include "agile/workflow2/main.h"
#include "agile/workflow2/graph.h"

namespace agile::workflow2 {

struct args_t {
  uint64_t size;
  uint64_t delta;
  uint64_t arrayOID;
};

static void exclusiveRecursiveScan(shad::rt::Handle & handle, uint64_t pos, Vertex & elem, args_t & args) {
    auto arrayPtr = VertexType::GetPtr((VertexOID) args.arrayOID);

    uint64_t delta  = args.delta;
    uint64_t nelems = arrayPtr->getNElems();
    std::vector<Vertex> * data = arrayPtr->getData();

    // if not the last set, spawn next scan
    // ... next delta is this delta + # edges of last vertex in set 
    if (pos + nelems < args.size) {
       args_t next_args = args;
       next_args.delta += (* data)[nelems - 1].edges;
       arrayPtr->AsyncApply(handle, pos + nelems, exclusiveRecursiveScan, next_args);
    }

    for (uint64_t i = nelems - 1; i > 0; i --)
      (* data)[i].edges = (* data)[i - 1].edges + delta;

    (* data)[0].edges = delta;
}


// Exclusive scan for vertex class array
void exclusiveScanVertices(uint64_t arrayOID) {
  auto arrayPtr = VertexType::GetPtr((VertexOID) arrayOID);

  auto localInclusiveScan = [](shad::rt::Handle & handle, const uint64_t & arrayOID) {
    auto arrayPtr = VertexType::GetPtr((VertexOID) arrayOID);
    uint64_t nelems = arrayPtr->getNElems();
    std::vector<Vertex> * data = arrayPtr->getData();

    for (uint64_t i = 1; i < nelems; i ++) (* data)[i].edges += (* data)[i - 1].edges;
  };

  shad::rt::Handle handle;
  shad::rt::asyncExecuteOnAll(handle, localInclusiveScan, arrayOID);
  waitForCompletion(handle);

  args_t args = {arrayPtr->Size(), 0, arrayOID};
  arrayPtr->AsyncApply(handle, 0, exclusiveRecursiveScan, args);
  waitForCompletion(handle);
}


// Update the global ids on this local and spawn updateIDS_ on next local.
void updateIDS_(shad::rt::Handle & handle, const args_t & args) {
  auto GlobalIDS = GlobalIDType::GetPtr((GlobalIDOID) args.arrayOID);
  uint64_t local = (uint32_t) shad::rt::thisLocality();
  auto localMap  = GlobalIDS->getLocalMap();

  auto updateLambda = [] (const uint64_t & key, Vertex & value, const uint64_t & delta) {
    value.id += delta;
  };

  if (local < shad::rt::numLocalities() - 1) {
     uint64_t next_delta = args.delta + localMap->Size();
     args_t next_args = {ULLONG_MAX, next_delta, args.arrayOID};     // size not needed
     shad::rt::asyncExecuteAt(handle, shad::rt::Locality(local + 1), updateIDS_, next_args);
  }

  localMap->ForEachEntry(updateLambda, args.delta);
}


// Move entry to Vertices ... store entry at index value.id ... replace value.id with key
void moveVertex_(shad::rt::Handle & handle, const uint64_t & key, Vertex & value, args_t & args) {
  uint64_t ndx = value.id;
  auto Vertices = VertexType::GetPtr((VertexOID) args.arrayOID);
  Vertices->AsyncInsertAt(handle, ndx, Vertex(key, value.edges, value.type));
}


/********** CREATE COMPRESSED EDGE ARRAY AND VERTEX ARRAY **********/
void edgesVertices(uint64_t & num_edges, uint64_t & num_vertices, Graph_t & graph) {
  shad::rt::Handle handle;
  auto GlobalIDS = GlobalIDType::GetPtr((GlobalIDOID) graph["GlobalIDS"]);

// ***** allocate space for Vertices, fill pointers, and add to graph*****/
  num_vertices = GlobalIDS->Size();
  auto Vertices = VertexType::Create(num_vertices + 1, Vertex());

  Vertices->FillPtrs();
  graph["Vertices"] = (uint64_t) (Vertices->GetGlobalID());

// ***** convert local ids to global ids *****/
  args_t args = {ULLONG_MAX, 0, graph["GlobalIDS"]};     // size not needed
  shad::rt::asyncExecuteAt(handle, shad::rt::Locality(0), updateIDS_, args);

  waitForCompletion(handle);

// ***** allocate space for Vertices and copy vertex classes from GlobalIDS *****/
  args.arrayOID = graph["Vertices"];
  GlobalIDS->AsyncForEachEntry(handle, moveVertex_, args);

  waitForCompletion(handle);
  exclusiveScanVertices(graph["Vertices"]);     // exclusive scan of edges to convert # edges to start location

  num_edges = ( Vertices->At(num_vertices) ).edges;
  // auto Edges = EdgeType::Create(num_edges, Edge());
  // graph["Edges"] = (uint64_t) (Edges->GetGlobalID());

}

} // namespace agile::workflow2
