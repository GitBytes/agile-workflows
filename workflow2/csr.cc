#include "agile/workflow2/main.h"
#include "agile/workflow2/graph.h"
#include "agile/workflow2/globalIDS.h"
#include "agile/workflow2/csr.h"

namespace agile::workflow2 {

struct Args_t { uint64_t delta; uint64_t oid; };

// Update the global ids on this local and spawn updateIDS_ on next local.
void updateIDS_(Handle & handle, const Args_t & args) {
  auto GlobalIDS = GlobalIDType::GetPtr((GlobalIDOID) args.oid);
  auto locale = (uint32_t) shad::rt::thisLocality();
  auto my_map = GlobalIDS->GetLocalHashmap();

  auto updateLambda = [] (const uint64_t & key, Vertex & value, const uint64_t & delta) {
    value.id += delta;
  };

  if (locale < shad::rt::numLocalities() - 1) {
     Args_t my_args = {args.delta + my_map->Size(), args.oid};
     shad::rt::asyncExecuteAt(handle, shad::rt::Locality(locale + 1), updateIDS_, my_args);
  }

  my_map->ForEachEntry(updateLambda, args.delta);
}


// Fill in Vertices and update global ids in vertex tables
// ... store entry {key, value.edges, value.type} at index value.id
// ... replace vertex's global id in its vertex table with value.id
void moveVertex(Handle & handle, const uint64_t & key, Vertex & value,
     uint64_t & personsOID, uint64_t & forumEventsOID, uint64_t & forumsOID,
     uint64_t & publicationsOID, uint64_t & topicsOID, uint64_t & verticesOID) {

  VertexType::GetPtr((VertexOID) verticesOID)->
       AsyncInsertAt(handle, value.id, Vertex(key, value.edges, value.type));

  if (value.type == TYPES::PERSON) {
     PersonVertexType::GetPtr((PersonVertexOID) personsOID)->
          AsyncApply(handle, key, updateGLBID<PersonVertex>, value.id);

  } else if (value.type == TYPES::FORUMEVENT) {
     ForumEventVertexType::GetPtr((ForumEventVertexOID) forumEventsOID)->
          AsyncApply(handle, key, updateGLBID<ForumEventVertex>, value.id);

  } else if (value.type == TYPES::FORUM) {
     ForumVertexType::GetPtr((ForumVertexOID) forumsOID)->
          AsyncApply(handle, key, updateGLBID<ForumVertex>, value.id);

  } else if (value.type == TYPES::PUBLICATION) {
     PublicationVertexType::GetPtr((PublicationVertexOID) publicationsOID)->
          AsyncApply(handle, key, updateGLBID<PublicationVertex>, value.id);

  } else if (value.type == TYPES::TOPIC) {
     TopicVertexType::GetPtr((TopicVertexOID) topicsOID)->
          AsyncApply(handle, key, updateGLBID<TopicVertex>, value.id);
} }


// Move edges to Edges
void moveEdges(uint64_t pos, Vertex & value,
  uint64_t & purchasesOID, uint64_t & salesOID, uint64_t & authorsOID, uint64_t & includesOID,
  uint64_t & hasTopicOID,  uint64_t & hasOrgOID, uint64_t & globalIDSOID, uint64_t & edgesOID) {

  Handle handle;
  uint32_t retSize;
  uint64_t ndx = value.edges, NE = 0;
  auto Purchases = PurchaseEdgeType::GetPtr((PurchaseEdgeOID) purchasesOID);
  auto Sales     = SaleEdgeType::GetPtr((SaleEdgeOID) salesOID);
  auto Authors   = AuthorEdgeType::GetPtr((AuthorEdgeOID) authorsOID);
  auto Includes  = IncludesEdgeType::GetPtr((IncludesEdgeOID) includesOID);
  auto HasTopic  = HasTopicEdgeType::GetPtr((HasTopicEdgeOID) hasTopicOID);
  auto HasOrg    = HasOrgEdgeType::GetPtr((HasOrgEdgeOID) hasOrgOID);

  if (value.type == TYPES::PERSON) {     // Person has purchase, sale, and author edges
     Purchases->AsyncApplyWithRetBuff(handle, value.id, MoveTableEdges<PurchaseEdge>,
          (uint8_t *) & NE, & retSize, ndx, globalIDSOID, edgesOID);

     waitForCompletion(handle);
     ndx += NE; NE = 0;

     Sales->AsyncApplyWithRetBuff(handle, value.id, MoveTableEdges<SaleEdge>,
          (uint8_t *) & NE, & retSize, ndx, globalIDSOID, edgesOID);

     waitForCompletion(handle);
     ndx += NE; NE = 0;

     Authors->AsyncApplyWithRetBuff(handle, value.id, MoveTableEdges<AuthorEdge>,
          (uint8_t *) & NE, & retSize, ndx, globalIDSOID, edgesOID);

  } else if (value.type == TYPES::FORUMEVENT) {     // ForumEvent has has_topic edges
     HasTopic->AsyncApplyWithRetBuff(handle, value.id, MoveTableEdges<HasTopicEdge>,
          (uint8_t *) & NE, & retSize, ndx, globalIDSOID, edgesOID);

  } else if (value.type == TYPES::FORUM) {     // Forum has includes and has_topic edges
     Includes->AsyncApplyWithRetBuff(handle, value.id, MoveTableEdges<IncludesEdge>,
          (uint8_t *) & NE, & retSize, ndx, globalIDSOID, edgesOID);

     waitForCompletion(handle);
     ndx += NE; NE = 0;

     HasTopic->AsyncApplyWithRetBuff(handle, value.id, MoveTableEdges<HasTopicEdge>,
          (uint8_t *) & NE, & retSize, ndx, globalIDSOID, edgesOID);

  } else if (value.type == TYPES::PUBLICATION) {     // Publication has has_org and has_topic edges
     HasOrg->AsyncApplyWithRetBuff(handle, value.id, MoveTableEdges<HasOrgEdge>,
          (uint8_t *) & NE, & retSize, ndx, globalIDSOID, edgesOID);

     waitForCompletion(handle);
     ndx += NE; NE = 0;

     HasTopic->AsyncApplyWithRetBuff(handle, value.id, MoveTableEdges<HasTopicEdge>,
          (uint8_t *) & NE, & retSize, ndx, globalIDSOID, edgesOID);

  } else return;

  waitForCompletion(handle);
}


/********** CREATE COMPRESSED EDGE ARRAY AND VERTEX ARRAY **********/
void CSR(uint64_t & num_edges, uint64_t & num_vertices, Graph_t & graph) {
  Handle handle;
  auto GlobalIDS = GlobalIDType::GetPtr((GlobalIDOID) graph["GlobalIDS"]);

// ***** allocate space for Vertices, fill pointers, and add to graph *****/
  num_vertices  = GlobalIDS->Size();
  auto Vertices = VertexType::Create(num_vertices + 1, Vertex());

  Vertices->FillPtrs();
  graph["Vertices"] = (uint64_t) (Vertices->GetGlobalID());

// ***** convert local ids to global ids *****/
  Args_t my_args = {0, graph["GlobalIDS"]};
  shad::rt::asyncExecuteAt(handle, shad::rt::Locality(0), updateIDS_, my_args);

  waitForCompletion(handle);

// ***** allocate space for Vertices and copy entries from GlobalIDS to Vertices *****/
  GlobalIDS->AsyncForEachEntry(handle, moveVertex, graph["Persons"], graph["ForumEvents"],
       graph["Forums"], graph["Publications"], graph["Topics"], graph["Vertices"]);

  waitForCompletion(handle);
  exclusiveScanVertices<Vertex>(graph["Vertices"]);     // convert # edges to start location

// ***** allocate space for Edges, fill pointers, and add to graph *****/
  num_edges  = (Vertices->At(num_vertices)).edges;
  auto Edges = EdgeType::Create(num_edges, Edge());

  Edges->FillPtrs();
  graph["Edges"] = (uint64_t) (Edges->GetGlobalID());

// ***** move edges from edge tables to Edges *****/
  Vertices->ForEach(moveEdges, graph["Purchases"], graph["Sales"], graph["Authors"],
       graph["Includes"], graph["HasTopic"], graph["HasOrg"], graph["GlobalIDS"], graph["Edges"]);

  Edges->WaitForBufferedInsert();
}

} // namespace agile::workflow2
