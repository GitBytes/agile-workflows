#include "agile/workflow1/main.h"
#include "agile/workflow1/graph.h"
#include "agile/workflow1/csr.h"

namespace agile::workflow1 {

struct args_t {
  uint64_t size;
  uint64_t delta;
  uint64_t arrayOID;
};


struct ME_args_t {
  uint64_t purchasesOID;
  uint64_t salesOID;
  uint64_t authorsOID;
  uint64_t includesOID;
  uint64_t hasOrgOID;
  uint64_t hasTopicOID;
  uint64_t edgesOID;
};


// Exclusive scan for vertex class array
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
  auto localMap  = GlobalIDS->GetLocalHashmap();

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
void moveVertex(shad::rt::Handle & handle, const uint64_t & key, Vertex & value, args_t & args) {
  uint64_t ndx = value.id;
  auto Vertices = VertexType::GetPtr((VertexOID) args.arrayOID);
  Vertices->AsyncInsertAt(handle, ndx, Vertex(key, value.edges, value.type));
}


// Move edges to Edges
void moveEdges(uint64_t pos, Vertex & value, ME_args_t & args) {
  uint64_t NE = 0;
  uint32_t retSize;
  uint64_t id = value.id;
  shad::rt::Handle handle;

  auto Purchases = PurchaseEdgeType::GetPtr((PurchaseEdgeOID) args.purchasesOID);
  auto Sales     = SaleEdgeType::GetPtr((SaleEdgeOID) args.salesOID);
  auto Authors   = AuthorEdgeType::GetPtr((AuthorEdgeOID) args.authorsOID);
  auto Includes  = IncludesEdgeType::GetPtr((IncludesEdgeOID) args.includesOID);
  auto HasOrg    = HasOrgEdgeType::GetPtr((HasOrgEdgeOID) args.hasOrgOID);
  auto HasTopic  = HasTopicEdgeType::GetPtr((HasTopicEdgeOID) args.hasTopicOID);

  MTE_args_t my_args;
  my_args.start    = value.edges;
  my_args.edgesOID = args.edgesOID;

  if (value.type == TYPES::PERSON) {     // Person has purchase, sale, and author edges
     my_args.type = TYPES::PURCHASE;     // ... PURCHASE edges
     Purchases->AsyncApplyWithRetBuff(handle, id, MoveTableEdges<PurchaseEdge>, (uint8_t *) & NE, & retSize, my_args);

     waitForCompletion(handle);
     my_args.start += NE;
     NE = 0;

     my_args.type = TYPES::SALE;         // ... SALE edges
     Sales->AsyncApplyWithRetBuff(handle, id, MoveTableEdges<SaleEdge>, (uint8_t *) & NE, & retSize, my_args);

     waitForCompletion(handle);
     my_args.start += NE;
     NE = 0;

     my_args.type = TYPES::AUTHOR;       // ... AUTHOR edges
     Authors->AsyncApplyWithRetBuff(handle, id, MoveTableEdges<AuthorEdge>, (uint8_t *) & NE, & retSize, my_args);
     waitForCompletion(handle);

  } else if (value.type == TYPES::FORUMEVENT) {     // ForumEvent has has_topic edges
     my_args.type = TYPES::HASTOPIC;                // ... HASTOPIC edges
     HasTopic->AsyncApplyWithRetBuff(handle, id, MoveTableEdges<HasTopicEdge>, (uint8_t *) & NE, & retSize, my_args);
     waitForCompletion(handle);

  } else if (value.type == TYPES::FORUM) {     // Forum has includes and has_topic edges
     my_args.type = TYPES::INCLUDES;           // ... INLCUDES edges
     Includes->AsyncApplyWithRetBuff(handle, id, MoveTableEdges<IncludesEdge>, (uint8_t *) & NE, & retSize, my_args);

     waitForCompletion(handle);
     my_args.start += NE;
     NE = 0;

     my_args.type = TYPES::HASTOPIC;           // ... HASTOPIC edges
     HasTopic->AsyncApplyWithRetBuff(handle, id, MoveTableEdges<HasTopicEdge>, (uint8_t *) & NE, & retSize, my_args);
     waitForCompletion(handle);

  } else if (value.type == TYPES::PUBLICATION) {     // Publication has has_org and has_topic edges
     my_args.type = TYPES::HASORG;                   // ... HASORG edges
     HasOrg->AsyncApplyWithRetBuff(handle, id, MoveTableEdges<HasOrgEdge>, (uint8_t *) & NE, & retSize, my_args);

     waitForCompletion(handle);
     my_args.start += NE;
     NE = 0;

     my_args.type = TYPES::HASTOPIC;                 // ... HASTOPIC edges
     HasTopic->AsyncApplyWithRetBuff(handle, id, MoveTableEdges<HasTopicEdge>, (uint8_t *) & NE, & retSize, my_args);
     waitForCompletion(handle);
} }


/********** CREATE COMPRESSED EDGE ARRAY AND VERTEX ARRAY **********/
void CSR(uint64_t & num_edges, uint64_t & num_vertices, Graph_t & graph) {
  shad::rt::Handle handle;
  auto GlobalIDS = GlobalIDType::GetPtr((GlobalIDOID) graph["GlobalIDS"]);

// ***** allocate space for Vertices, fill pointers, and add to graph *****/
  num_vertices  = GlobalIDS->Size();
  auto Vertices = VertexType::Create(num_vertices + 1, Vertex());

  Vertices->FillPtrs();
  graph["Vertices"] = (uint64_t) (Vertices->GetGlobalID());

// ***** convert local ids to global ids *****/
  args_t args = {ULLONG_MAX, 0, graph["GlobalIDS"]};     // size not needed
  shad::rt::asyncExecuteAt(handle, shad::rt::Locality(0), updateIDS_, args);

  waitForCompletion(handle);

// ***** allocate space for Vertices and copy vertex classes from GlobalIDS *****/
  args.arrayOID = graph["Vertices"];
  GlobalIDS->AsyncForEachEntry(handle, moveVertex, args);

  waitForCompletion(handle);
  exclusiveScanVertices(graph["Vertices"]);     // exclusive scan of edges to convert # edges to start location

// ***** allocate space for Edges, fill pointers, and add to graph *****/
  num_edges  = (Vertices->At(num_vertices)).edges;
  auto Edges = EdgeType::Create(num_edges, Edge());

  Edges->FillPtrs();
  graph["Edges"] = (uint64_t) (Edges->GetGlobalID());

// ***** move edges from edge tables to Edges *****/
  ME_args_t me_args;
  me_args.purchasesOID = graph["Purchases"];
  me_args.salesOID     = graph["Sales"];
  me_args.authorsOID   = graph["Authors"];
  me_args.includesOID  = graph["Includes"];
  me_args.hasOrgOID    = graph["HasOrg"];
  me_args.hasTopicOID  = graph["HasTopic"];
  me_args.edgesOID     = graph["Edges"];

  Vertices->ForEach(moveEdges, me_args);
  Edges->WaitForBufferedInsert();
}

} // namespace agile::workflow1
