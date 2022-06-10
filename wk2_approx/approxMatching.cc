//===------------------------------------------------------------*- C++ -*-===//
//
//                                     SHAD
//
//      The Scalable High-performance Algorithms and Data Structure Library
//
//===----------------------------------------------------------------------===//
//
// Copyright 2018 Battelle Memorial Institute
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy
// of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
//===----------------------------------------------------------------------===/

#include <mutex>
#include "agile/wk2_approx/graph.h"

using Edge = std::pair<uint64_t, double>;

struct V_struct {
  uint64_t id;
  agile::wk2_approx::TYPES type;
  agile::wk2_approx::Triples triples;
  int64_t mate  = -1;
  int64_t index = -1;
  int64_t taken =  0;
  std::vector<Edge> edges;
};

std::mutex lock_LHS;
std::mutex lock_RHS;
std::map<uint64_t, V_struct> local_LHS;
std::map<uint64_t, V_struct> local_RHS;

namespace agile::wk2_approx {

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

  SaleEdgeType::LookupResult sales;
  AuthorEdgeType::LookupResult authors;
  PurchaseEdgeType::LookupResult purchases;

  SaleEdgeType::GetPtr((SaleEdgeOID) Sales_OID)->Lookup(key, & sales);                     // get vertex's sales
  AuthorEdgeType::GetPtr((AuthorEdgeOID) Authors_OID)->Lookup(key, & authors);             // get vertex's authors
  PurchaseEdgeType::GetPtr((PurchaseEdgeOID) Purchases_OID)->Lookup(key, & purchases);     // get vertex's purchases

// Accumulte vertex's SPO histogram
  for (auto & S1 : sales.value) {
    if      (S1.product == 2869238) v.triples[(uint64_t) TRIPLES::PERSON_SALE_PERSON_BOMB_BATH] ++;
    else if (S1.product == 271997)  v.triples[(uint64_t) TRIPLES::PERSON_SALE_PERSON_PRESSURE_COOKER] ++;
    else if (S1.product == 185785)  v.triples[(uint64_t) TRIPLES::PERSON_SALE_PERSON_AMMUNITION] ++;
    else if (S1.product == 11650)   v.triples[(uint64_t) TRIPLES::PERSON_SALE_PERSON_ELECTRONICS] ++;
  }

  for (auto & P1 : purchases.value) {
    if      (P1.product == 2869238) v.triples[(uint64_t) TRIPLES::PERSON_PURCHASE_PERSON_BOMB_BATH] ++;
    else if (P1.product == 271997)  v.triples[(uint64_t) TRIPLES::PERSON_PURCHASE_PERSON_PRESSURE_COOKER] ++;
    else if (P1.product == 185785)  v.triples[(uint64_t) TRIPLES::PERSON_PURCHASE_PERSON_AMMUNITION] ++;
    else if (P1.product == 11650)   v.triples[(uint64_t) TRIPLES::PERSON_PURCHASE_PERSON_ELECTRONICS] ++;
  }

  for (auto & A1 : authors.value) {
    if      (A1.dst_type == TYPES::FORUMEVENT)  v.triples[(uint64_t) TRIPLES::PERSON_AUTHOR_FORUMEVENT] ++;
    else if (A1.dst_type == TYPES::PUBLICATION) v.triples[(uint64_t) TRIPLES::PERSON_AUTHOR_PUBLICATION] ++;
  }

  VertexType::GetPtr((VertexOID) V_OID)->BufferedAsyncInsert(handle, v.id, v);
}


// Create a ForumEvent vertex. ForumEvent vertices are src vertices of HasTopic edges.
void ForumEventVertex_(Handle & handle, const uint64_t & key,
     ForumEventVertex & vertex, uint64_t & HasTopic_OID, uint64_t & V_OID) {
  Vertex v = Vertex();
  v.id     = vertex.id;
  v.type   = TYPES::FORUMEVENT;

  HasTopicEdgeType::LookupResult topics;
  HasTopicEdgeType::GetPtr((HasTopicEdgeOID) HasTopic_OID)->Lookup(key, & topics);     // get vertex's topics

// Accumulte vertex's SPO histogram
  for (auto & T1 : topics.value) {
    if      (T1.topic == 127197)   v.triples[(uint64_t) TRIPLES::FORUMEVENT_HASTOPIC_TOPIC_BOMB] ++;
    else if (T1.topic == 179057)   v.triples[(uint64_t) TRIPLES::FORUMEVENT_HASTOPIC_TOPIC_EXPLOSION] ++;
    else if (T1.topic == 771572)   v.triples[(uint64_t) TRIPLES::FORUMEVENT_HASTOPIC_TOPIC_WILLIAMSBURG] ++;
    else if (T1.topic == 1049632)  v.triples[(uint64_t) TRIPLES::FORUMEVENT_HASTOPIC_TOPIC_PROSPECT_PARK] ++;
    else if (T1.topic == 69871376) v.triples[(uint64_t) TRIPLES::FORUMEVENT_HASTOPIC_TOPIC_OUTDOORS] ++;
    else if (T1.topic == 44311)    v.triples[(uint64_t) TRIPLES::FORUMEVENT_HASTOPIC_TOPIC_JIHAD] ++;
  }

  VertexType::GetPtr((VertexOID) V_OID)->BufferedAsyncInsert(handle, v.id, v);
}


// Create a Forum vertex. Forum vertices are src vertices of Includes and HasTopic edges.
void ForumVertex_(Handle & handle, const uint64_t & key, ForumVertex & vertex,
     uint64_t & Includes_OID, uint64_t & HasTopic_OID, uint64_t & V_OID) {
  Vertex v = Vertex();
  v.id     = vertex.id;
  v.type   = TYPES::FORUM;

  IncludesEdgeType::LookupResult events;
  HasTopicEdgeType::LookupResult topics;

  IncludesEdgeType::GetPtr((IncludesEdgeOID) Includes_OID)->Lookup(key, & events);     // get vertex's events
  HasTopicEdgeType::GetPtr((HasTopicEdgeOID) HasTopic_OID)->Lookup(key, & topics);     // get vertex's topics

// Accumulte vertex's SPO histogram
  for (auto & T1 : topics.value)
    if (T1.topic == 60) v.triples[(uint64_t) TRIPLES::FORUM_HASTOPIC_TOPIC_NYC] ++;

  v.triples[(uint64_t) TRIPLES::FORUM_INCLUDES_FORUMEVENT] += events.value.size();
  VertexType::GetPtr((VertexOID) V_OID)->BufferedAsyncInsert(handle, v.id, v);
}


// Create a Publication vertex. Publication vertices are src vertices of HasOrg and HasTopic edges.
void PublicationVertex_(Handle & handle, const uint64_t & key, PublicationVertex & vertex,
     TopicVertex & NYC, uint64_t & Topics_OID, uint64_t & HasOrg_OID, uint64_t & HasTopic_OID, uint64_t & V_OID) {
  Vertex v = Vertex();
  v.id     = vertex.id;
  v.type   = TYPES::PUBLICATION;

  HasOrgEdgeType::LookupResult orgs;
  HasTopicEdgeType::LookupResult topics;

  HasOrgEdgeType::GetPtr((HasOrgEdgeOID) HasOrg_OID)->Lookup(key, & orgs);             // get vertex's orgs
  HasTopicEdgeType::GetPtr((HasTopicEdgeOID) HasTopic_OID)->Lookup(key, & topics);     // get vertex's topics

// Accumulte vertex's SPO histogram
  for (auto & PO : orgs.value) {
    TopicVertex org;
    TopicVertexType::GetPtr((TopicVertexOID) Topics_OID)->Lookup(PO.organization, & org);
    v.triples[(uint64_t) TRIPLES::PUBLICATION_HASORG_TOPIC_NEAR_NYC] += proximity(org, NYC);
  }

  for (auto & T1 : topics.value)
    if (T1.topic == 43035) v.triples[(uint64_t) TRIPLES::PUBLICATION_HASTOPIC_TOPIC_ELECTRICAL_ENG] ++;

  VertexType::GetPtr((VertexOID) V_OID)->BufferedAsyncInsert(handle, v.id, v);
}


// Make a local copy of the LHS shad hashmap on each locale.
void copy_LHS(const uint64_t & LHS_OID) {
  auto LHS = VertexType::GetPtr((VertexOID) LHS_OID);

  for (auto itr = LHS->begin(); itr != LHS->end(); ++ itr) {
    std::pair<uint64_t, Vertex> entry = (* itr);

    V_struct my_entry;
    my_entry.id      = entry.second.id;
    my_entry.type    = entry.second.type;
    my_entry.triples = entry.second.triples;
    my_entry.mate    = -1;
    my_entry.index   = -1;
    my_entry.taken   =  0;

    local_LHS[my_entry.id] = my_entry;
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
  vB.mate    = -1;
  vB.index   = -1;
  vB.taken   =  0;

  for (auto itr = local_LHS.begin(); itr != local_LHS.end(); ++ itr) {     // for each LHS vertex
    V_struct & vA = (* itr).second;
    if (vA.type != vB.type) continue;        // an edge exists only for vertices of the same type

    double weight = 1.0;                     // compute the weight of the edge
    for (uint64_t i = 0; i < NUMTRIPLES; ++ i) weight += vA.triples[i] * vB.triples[i];

    vB.edges.push_back( Edge(vA.id, weight) );

    lock_LHS.lock();
       vA.edges.push_back( Edge(vB.id, weight) );
    lock_LHS.unlock();
  }

  std::sort(vB.edges.begin(), vB.edges.end(), compEdge);

  lock_RHS.lock();
     local_RHS[vB.id] = vB;
  lock_RHS.unlock();
}


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

  shad::rt::executeOnAll(copy_LHS, LHS_OID);     // make a local copy of the LHS

  uint64_t null;
  VertexType::GetPtr((VertexOID) RHS_OID)->ForEachEntry(biPartiteEdges, null);
  shad::rt::executeOnAll(sort_LHS, null);        // sort edges in local copy of LHS

  for (auto itr = local_LHS.begin(); itr != local_LHS.end(); ++ itr) {
    V_struct entry = (* itr).second;
    printf("%lu %lu\n", entry.id, (uint64_t) entry.type);
    printf("  ");
    for (uint64_t j = 0; j < NUMTRIPLES; ++ j) printf(" %lu", entry.triples[j]);
    printf("\n");
    for (auto edge : entry.edges) printf("  %lu %lf\n", edge.first, edge.second);
  }

  printf("\n\n ********************* \n\n");

  for (auto itr = local_RHS.begin(); itr != local_RHS.end(); ++ itr) {
    V_struct entry = (* itr).second;
    printf("%lu %lu\n", entry.id, (uint64_t) entry.type);
    printf("  ");
    for (uint64_t j = 0; j < NUMTRIPLES; ++ j) printf(" %lu", entry.triples[j]);
    printf("\n");
    for (auto edge : entry.edges) printf("  %lu %lf\n", edge.first, edge.second);
  }

};

} // namespace agile::wk2_approx
