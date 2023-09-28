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

#include <mutex>
#include <algorithm>
#include "shad/data_structures/atomic.h"
#include "agile/wk2_approx/graph.h"

using intAtomic = shad::Atomic<int64_t>;
using intAtomicOID = shad::ObjectIdentifier<intAtomic>;

struct V_struct {
  bool taken;
  uint64_t id;
  agile::wk2_approx::TYPES type;
  agile::wk2_approx::Triples triples;
  agile::wk2_approx::Edge mate;
  std::vector<agile::wk2_approx::Edge> edges;
};

uint64_t null;
std::mutex lock_LHS;
std::mutex lock_RHS;
std::map<uint64_t, V_struct> local_LHS;
std::map<uint64_t, V_struct> local_RHS;

namespace agile::wk2_approx {

struct args_t {
  uint64_t LHS_OID;
  uint64_t Matched_OID;
};


bool compEdge(Edge & A, Edge & B) {
    return ( (A.second > B.second) || ( (A.second == B.second) && (A.first > B.first) ) );
}


bool proximity(TopicVertex & A, TopicVertex & B) {
  double lon_miles = 0.91 * std::abs(A.lon - B.lon);
  double lat_miles = 1.15 * std::abs(A.lat - B.lat);
  double distance = std::sqrt( lon_miles * lon_miles + lat_miles * lat_miles );
  return distance <= 30.0;
}


// Create a Person vertex. Person vertices are src vertices of Sale, Purchase, and Author edges.
void PersonVertex_(Handle & handle, const uint64_t & key, PersonVertex & vertex,
     uint64_t & Sales_OID, uint64_t & Purchases_OID, uint64_t & Authors_OID, uint64_t & V_OID) {
  Vertex v = Vertex();
  v.id     = vertex.id;
  v.type   = TYPES::PERSON;

  auto Sales_ = [] (const uint64_t & key, std::vector<SaleEdge> & value, Vertex & v) {
    for (auto & edge : value) {
      if      (edge.product == 2869238) v.triples[(uint64_t) TRIPLES::PERSON_SALE_PERSON_BOMB_BATH] ++;
      else if (edge.product == 271997)  v.triples[(uint64_t) TRIPLES::PERSON_SALE_PERSON_PRESSURE_COOKER] ++;
      else if (edge.product == 185785)  v.triples[(uint64_t) TRIPLES::PERSON_SALE_PERSON_AMMUNITION] ++;
      else if (edge.product == 11650)   v.triples[(uint64_t) TRIPLES::PERSON_SALE_PERSON_ELECTRONICS] ++;
    }
  };

  auto Purchases_ = [] (const uint64_t & key, std::vector<PurchaseEdge> & value, Vertex & v) {
    for (auto & edge : value) {
      if      (edge.product == 2869238) v.triples[(uint64_t) TRIPLES::PERSON_PURCHASE_PERSON_BOMB_BATH] ++;
      else if (edge.product == 271997)  v.triples[(uint64_t) TRIPLES::PERSON_PURCHASE_PERSON_PRESSURE_COOKER] ++;
      else if (edge.product == 185785)  v.triples[(uint64_t) TRIPLES::PERSON_PURCHASE_PERSON_AMMUNITION] ++;
      else if (edge.product == 11650)   v.triples[(uint64_t) TRIPLES::PERSON_PURCHASE_PERSON_ELECTRONICS] ++;
    }
  };

  auto Authors_ = [] (const uint64_t & key, std::vector<AuthorEdge> & value, Vertex & v) {
    for (auto & edge : value) {
      if      (edge.dst_type == TYPES::FORUMEVENT)  v.triples[(uint64_t) TRIPLES::PERSON_AUTHOR_FORUMEVENT] ++;
      else if (edge.dst_type == TYPES::PUBLICATION) v.triples[(uint64_t) TRIPLES::PERSON_AUTHOR_PUBLICATION] ++;
    }
  };

  SaleEdgeType::GetPtr((SaleEdgeOID) Sales_OID)->Apply(key, Sales_, v);
  PurchaseEdgeType::GetPtr((PurchaseEdgeOID) Purchases_OID)->Apply(key, Purchases_, v);
  AuthorEdgeType::GetPtr((AuthorEdgeOID) Authors_OID)->Apply(key, Authors_, v);
  VertexType::GetPtr((VertexOID) V_OID)->BufferedAsyncInsert(handle, v.id, v);
}


// Create a ForumEvent vertex. ForumEvent vertices are src vertices of HasTopic edges.
void ForumEventVertex_(Handle & handle, const uint64_t & key,
     ForumEventVertex & vertex, uint64_t & HasTopic_OID, uint64_t & V_OID) {
  Vertex v = Vertex();
  v.id     = vertex.id;
  v.type   = TYPES::FORUMEVENT;

  auto Topics_ = [] (const uint64_t & key, std::vector<HasTopicEdge> & value, Vertex & v) {
    for (auto & edge : value) {
      if      (edge.topic == 127197)   v.triples[(uint64_t) TRIPLES::FORUMEVENT_HASTOPIC_TOPIC_BOMB] ++;
      else if (edge.topic == 179057)   v.triples[(uint64_t) TRIPLES::FORUMEVENT_HASTOPIC_TOPIC_EXPLOSION] ++;
      else if (edge.topic == 771572)   v.triples[(uint64_t) TRIPLES::FORUMEVENT_HASTOPIC_TOPIC_WILLIAMSBURG] ++;
      else if (edge.topic == 1049632)  v.triples[(uint64_t) TRIPLES::FORUMEVENT_HASTOPIC_TOPIC_PROSPECT_PARK] ++;
      else if (edge.topic == 69871376) v.triples[(uint64_t) TRIPLES::FORUMEVENT_HASTOPIC_TOPIC_OUTDOORS] ++;
      else if (edge.topic == 44311)    v.triples[(uint64_t) TRIPLES::FORUMEVENT_HASTOPIC_TOPIC_JIHAD] ++;
    }
  };

  HasTopicEdgeType::GetPtr((HasTopicEdgeOID) HasTopic_OID)->Apply(key, Topics_, v);
  VertexType::GetPtr((VertexOID) V_OID)->BufferedAsyncInsert(handle, v.id, v);
}


// Create a Forum vertex. Forum vertices are src vertices of Includes and HasTopic edges.
void ForumVertex_(Handle & handle, const uint64_t & key, ForumVertex & vertex,
     uint64_t & Includes_OID, uint64_t & HasTopic_OID, uint64_t & V_OID) {
  Vertex v = Vertex();
  v.id     = vertex.id;
  v.type   = TYPES::FORUM;

  auto Events_ = [] (const uint64_t & key, std::vector<IncludesEdge> & value, Vertex & v) {
    v.triples[(uint64_t) TRIPLES::FORUM_INCLUDES_FORUMEVENT] += value.size();
  };

  auto Topics_ = [] (const uint64_t & key, std::vector<HasTopicEdge> & value, Vertex & v) {
    for (auto & edge : value)
      if (edge.topic == 60) v.triples[(uint64_t) TRIPLES::FORUM_HASTOPIC_TOPIC_NYC] ++;
  };

  IncludesEdgeType::GetPtr((IncludesEdgeOID) Includes_OID)->Apply(key, Events_, v);
  HasTopicEdgeType::GetPtr((HasTopicEdgeOID) HasTopic_OID)->Apply(key, Topics_, v);
  VertexType::GetPtr((VertexOID) V_OID)->BufferedAsyncInsert(handle, v.id, v);
}


// Create a Publication vertex. Publication vertices are src vertices of HasOrg and HasTopic edges.
void PublicationVertex_(Handle & handle, const uint64_t & key, PublicationVertex & vertex,
     TopicVertex & NYC, uint64_t & Topics_OID, uint64_t & HasOrg_OID, uint64_t & HasTopic_OID, uint64_t & V_OID) {
  Vertex v = Vertex();
  v.id     = vertex.id;
  v.type   = TYPES::PUBLICATION;

// Accumulte vertex's SPO histogram
  auto OrgsProximity_ = [] (const uint64_t & key,
       std::vector<HasOrgEdge> & value, TopicVertex & NYC, uint64_t & Topics_OID, Vertex & v) {
    for (auto & edge : value) {
      TopicVertex org;
      TopicVertexType::GetPtr((TopicVertexOID) Topics_OID)->Lookup(edge.organization, & org);
      v.triples[(uint64_t) TRIPLES::PUBLICATION_HASORG_TOPIC_NEAR_NYC] += proximity(org, NYC);
    }
  };

  auto Topics_ = [] (const uint64_t & key, std::vector<HasTopicEdge> & value, Vertex & v) {
    for (auto & edge : value)
      if (edge.topic == 43035) v.triples[(uint64_t) TRIPLES::PUBLICATION_HASTOPIC_TOPIC_ELECTRICAL_ENG] ++;
  };

  HasOrgEdgeType::GetPtr((HasOrgEdgeOID) HasOrg_OID)->Apply(key, OrgsProximity_, NYC, Topics_OID, v);
  HasTopicEdgeType::GetPtr((HasTopicEdgeOID) HasTopic_OID)->Apply(key, Topics_, v);
  VertexType::GetPtr((VertexOID) V_OID)->BufferedAsyncInsert(handle, v.id, v);
}


// Copy LHS shad hashmap to local map on each locale.
void copy_LHS(const uint64_t & LHS_OID) {
  auto LHS = VertexType::GetPtr((VertexOID) LHS_OID);

  for (auto itr = LHS->begin(); itr != LHS->end(); ++ itr) {
    std::pair<uint64_t, Vertex> entry = (* itr);
    auto itr_local = local_LHS.find(entry.first);

    if (itr_local == local_LHS.end()) {     // entry is not in local map, create entry
       V_struct my_entry;
       my_entry.id      = entry.second.id;
       my_entry.type    = entry.second.type;
       my_entry.triples = entry.second.triples;
       my_entry.mate    = entry.second.mate;
       my_entry.taken   = entry.second.taken;
       local_LHS[entry.first] = my_entry;
    } else {
       (* itr_local).second.mate  = entry.second.mate;
       (* itr_local).second.taken = entry.second.taken;
} } }


// Update mate of local LHS copies on each locale.
void update_LHS(const uint64_t & LHS_OID) {
  Vertex vertex;
  auto LHS = VertexType::GetPtr((VertexOID) LHS_OID);

  for (auto itr = local_LHS.begin(); itr != local_LHS.end(); ++ itr) {
    if ((* itr).second.taken) continue;       // vertex already matched, so mate has not changed
    LHS->Lookup((* itr).first, & vertex);     // get global copy
    (* itr).second.mate  = vertex.mate;
    (* itr).second.taken = vertex.taken;
} }


// Sort edges in local copy of LHS
void sort_LHS(const uint64_t & null) {
  for (auto itr = local_LHS.begin(); itr != local_LHS.end(); ++ itr)
    std::sort((* itr).second.edges.begin(), (* itr).second.edges.end(), compEdge);
}


// Create an local copy of the RHS vertex and the bipartite edges from it to all LHS vertices.
void biPartiteEdges(const uint64_t & key, Vertex & vertex, uint64_t & null) {
  V_struct vB;     // local copy of vertex
  vB.id      = vertex.id;
  vB.type    = vertex.type;
  vB.triples = vertex.triples;
  vB.mate    = vertex.mate;
  vB.taken   =  vertex.taken;

  for (auto itr = local_LHS.begin(); itr != local_LHS.end(); ++ itr) {     // for each LHS vertex
    V_struct & vA = (* itr).second;
    if (vA.type != vB.type) continue;               // an edge exists only for vertices of the same type

    double similarity = 0.0;                        // compute the cosine similarity of the SPO vectors
    double adj = 0.0, dot = 0.0, lenVA = 0.0, lenVB = 0.0;

    for (uint64_t i = 0; i < NUMTRIPLES; ++ i) {
      double minTriple = std::min(vA.triples[i], vB.triples[i]);
      adj   += minTriple * minTriple;
      dot   += vA.triples[i] * vB.triples[i];
      lenVA += vA.triples[i] * vA.triples[i];
      lenVB += vB.triples[i] * vB.triples[i];
    }

    if ((lenVA != 0.0) && (lenVB != 0.0))
       similarity = (sqrt(adj) * dot) / (sqrt(lenVA) * sqrt(lenVB));

    vB.edges.push_back( Edge(vA.id, similarity) );

    lock_LHS.lock();
       vA.edges.push_back( Edge(vB.id, similarity) );
    lock_LHS.unlock();
  }

  std::sort(vB.edges.begin(), vB.edges.end(), compEdge);

  lock_RHS.lock();
     local_RHS[vB.id] = vB;
  lock_RHS.unlock();
}


static void updateGlobalMate(const uint64_t & id, Vertex & vertex, Edge & mate) {
  if (compEdge(mate, vertex.mate)) {
     vertex.mate.first  = mate.first;
     vertex.mate.second = mate.second;
} }


static void setMate(Handle & handle, const uint64_t & id, Vertex & vertex, bool & taken) {
  if (taken) vertex.taken = true;
  else       vertex.mate  = {shad::data_types::kNullValue<uint64_t>, 0.0};
}


static void initializeMate(const uint64_t & id, Vertex & vertex, uint64_t & null) {
  vertex.taken = false;
  vertex.mate  = {shad::data_types::kNullValue<uint64_t>, 0.0};
}


static void resetMate(const uint64_t & id, Vertex & vertex, uint64_t & null) {
  if (! vertex.taken) vertex.mate = {shad::data_types::kNullValue<uint64_t>, 0.0};
}


void mate_LHS(const uint64_t & LHS_OID) {
  auto LHS = VertexType::GetPtr((VertexOID) LHS_OID);

  for (auto itr = local_LHS.begin(); itr != local_LHS.end(); ++ itr) {     // for each LHS vertex
    if ((* itr).second.taken) continue;                     // ... vertex is already matched

    for (Edge & mate : (* itr).second.edges) {              // ... for each RHS mate
      auto itr_B = local_RHS.find(mate.first);              // ... ... find first local RHS mate that is not taken
      if (itr_B == local_RHS.end()) continue;
      if ((* itr_B).second.taken)   continue;

      LHS->Apply((* itr).first, updateGlobalMate, mate);    // ... ... update global mate for this vertex
      break;
} } }


void check_mate(Handle & handle, const args_t & args) {
  auto LHS = VertexType::GetPtr((VertexOID) args.LHS_OID);

  for (auto itr = local_LHS.begin(); itr != local_LHS.end(); ++ itr) {     // for each LHS vertex
    uint64_t A = (* itr).first;
    V_struct & vA = (* itr).second;

    if (vA.taken) continue;                      // ... A is already matched, so continue
    uint64_t B = vA.mate.first;                  // ... A wants to mate with B
    auto itr_vB = local_RHS.find(B);

    if (itr_vB == local_RHS.end()) continue;     // ... B is not local, so another locale will check this A
    V_struct & vB = (* itr_vB).second;
    if (vB.taken) continue;                      // ... B is taken, note on next iteration A will mate with another B

    for (Edge edge : vB.edges) {                 // ... for each edge B --> LHS vertex
      uint64_t V = edge.first;                   // ... ... B wants to mate with V
      if (local_LHS[V].taken) continue;          // ... ... V is already matched, continue to find best choice

      if (V == A) {                              // ... ... match found 
         vB.mate  = edge;
         vA.taken = vB.taken = true;
         intAtomic::GetPtr((intAtomicOID) args.Matched_OID)->AsyncFetchAdd(handle, 1);
      }

      LHS->AsyncApply(handle, A, setMate, vA.taken);
      break;
} } }


void createBipartite(Graph_t & A, Graph_t & B, uint64_t & LHS_OID, uint64_t & RHS_OID) {
  Handle handle;
  auto A_Persons      = PersonVertexType::GetPtr((PersonVertexOID) A["Persons"]);
  auto A_ForumEvents  = ForumEventVertexType::GetPtr((ForumEventVertexOID) A["ForumEvents"]);
  auto A_Forums       = ForumVertexType::GetPtr((ForumVertexOID) A["Forums"]);
  auto A_Publications = PublicationVertexType::GetPtr((PublicationVertexOID) A["Publications"]);
  auto A_Topics       = TopicVertexType::GetPtr((TopicVertexOID) A["Topics"]);

  auto B_Persons      = PersonVertexType::GetPtr((PersonVertexOID) B["Persons"]);
  auto B_ForumEvents  = ForumEventVertexType::GetPtr((ForumEventVertexOID) B["ForumEvents"]);
  auto B_Forums       = ForumVertexType::GetPtr((ForumVertexOID) B["Forums"]);
  auto B_Publications = PublicationVertexType::GetPtr((PublicationVertexOID) B["Publications"]);
  auto B_Topics       = TopicVertexType::GetPtr((TopicVertexOID) B["Topics"]);

  TopicVertex NYC;
  B_Topics->Lookup(60, & NYC);

// Create a vertex on the LHS of the bipartite graph for each vertex in the Patttern Graph
  A_Persons->AsyncForEachEntry(handle, PersonVertex_, A["Sales"], A["Purchases"], A["Authors"], LHS_OID);
  A_ForumEvents->AsyncForEachEntry(handle, ForumEventVertex_, A["HasTopic"], LHS_OID);
  A_Forums->AsyncForEachEntry(handle, ForumVertex_, A["Includes"], A["HasTopic"], LHS_OID);
  A_Publications->AsyncForEachEntry(handle, PublicationVertex_, NYC, B["Topics"], A["HasOrg"], A["HasTopic"], LHS_OID);

// Create a vertex on the RHS of the bipartite graph for each vertex in the Data Graph
  B_Persons->AsyncForEachEntry(handle, PersonVertex_, B["Sales"], B["Purchases"], B["Authors"], RHS_OID);
  B_ForumEvents->AsyncForEachEntry(handle, ForumEventVertex_, B["HasTopic"], RHS_OID);
  B_Forums->AsyncForEachEntry(handle, ForumVertex_, B["Includes"], B["HasTopic"], RHS_OID);
  B_Publications->AsyncForEachEntry(handle, PublicationVertex_, NYC, B["Topics"], B["HasOrg"], B["HasTopic"], RHS_OID);

  waitForCompletion(handle);
  VertexType::GetPtr((VertexOID) LHS_OID)->WaitForBufferedInsert();
  VertexType::GetPtr((VertexOID) RHS_OID)->WaitForBufferedInsert();

  shad::rt::executeOnAll(copy_LHS, LHS_OID);     // copy LHS to each locale

  VertexType::GetPtr((VertexOID) RHS_OID)->ForEachEntry(biPartiteEdges, null);
  shad::rt::executeOnAll(sort_LHS, null);        // sort edges in local copy of LHS
};


void ApproxMatching(uint64_t & LHS_OID, uint64_t & RHS_OID) {
  Handle handle;
  auto Matched = intAtomic::Create(0);                           // number of matched Pattern vertices
  uint64_t Matched_OID = (uint64_t) (Matched->GetGlobalID());

  uint64_t prev_matched = 0;
  args_t args = {LHS_OID, Matched_OID};
  uint64_t num_pattern_vertices = VertexType::GetPtr((VertexOID) LHS_OID)->Size();

  VertexType::GetPtr((VertexOID) LHS_OID)->ForEachEntry(initializeMate, null);
  shad::rt::executeOnAll(copy_LHS, LHS_OID);                   // copy LHS to each locale
  
  while (prev_matched < num_pattern_vertices) {  
    shad::rt::executeOnAll(mate_LHS, LHS_OID);                 // set mate for each LHS vertex
    shad::rt::executeOnAll(update_LHS, LHS_OID);               // update local LHS on each locale
    shad::rt::asyncExecuteOnAll(handle, check_mate, args);     // check if LHS and RHS mates match

    waitForCompletion(handle);
    uint64_t matched_now = Matched->Load();

    if (prev_matched < matched_now) {     // still matching vertices
       prev_matched = matched_now;
       VertexType::GetPtr((VertexOID) LHS_OID)->ForEachEntry(resetMate, null);
    } else {                              // no more vertices to match
       break;
  } }

// print match
  printf("\n ********** Match ********** \n");

  for (auto itr = local_LHS.begin(); itr != local_LHS.end(); ++ itr)
    if (! ((* itr).second.taken))
       printf("Pattern vertex %2lu matched to Data vertex **********\n", (* itr).first);
    else
       printf("Pattern vertex %2lu matched to Data vertex %lu\n", (* itr).first, (* itr).second.mate.first);
}

} // namespace agile::wk2_approx
