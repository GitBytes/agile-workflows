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

#include <iomanip>
#include <iostream>
#include <climits>
#include <random>

#include "shad/data_structures/hashmap.h"
#include "shad/data_structures/multimap.h"
#include "shad/core/algorithm.h"
#include "shad/core/numeric.h"

using intMap = std::map<uint64_t, uint64_t>;
using Graph_t = std::map<std::string, uint64_t>;

using PersonVertexType       = shad::Hashmap<uint64_t, shad::PersonVertex>;
using ForumEventVertexType   = shad::Hashmap<uint64_t, shad::ForumEventVertex>;
using ForumVertexType        = shad::Hashmap<uint64_t, shad::ForumVertex>;
using PublicationVertexType  = shad::Hashmap<uint64_t, shad::PublicationVertex>;
using EventVertexType        = shad::Hashmap<uint64_t, shad::EventVertex>;

using PurchaseEdgeType = shad::Multimap<uint64_t, shad::PurchaseEdge>;
using SaleEdgeType     = shad::Multimap<uint64_t, shad::PurchaseEdge>;
using AuthorEdgeType   = shad::Multimap<uint64_t, shad::AuthorEdge>;
using AttendEdgeType   = shad::Multimap<uint64_t, shad::AttendEdge>;
using IncludeEdgeType  = shad::Multimap<uint64_t, shad::IncludeEdge>;
using HasTopicEdgeType = shad::Multimap<uint64_t, shad::HasTopicEdge>;
using HasOrgEdgeType   = shad::Multimap<uint64_t, shad::HasOrgEdge>;

// check to see if person attended a forum event at a forum that discusses Brooklyn
// AND has an event that discusses outdoors and Prospect Park
// AND an event that discusses Williamsburg, Explosion, and Bomb
bool event_pattern_1(uint64_t max_forum_date, shad::AttendEdge & edge, Graph_t & graph) {
  auto Forums = ForumVertexType::GetPtr( (ForumVertexType::ObjectID) graph["Forums"] );
  auto Includes= IncludeEdgeType::GetPtr( (IncludeEdgeType::ObjectID) graph["Includes"] );
  auto HasTopics = HasTopicEdgeType::GetPtr( (HasTopicEdgeType::ObjectID) graph["HasTopics"] );

  ForumVertex forum;                                        // get the forum record of the attended event
  Forums->Lookup(edge.forum, & forum);
  HasTopicEdgeType::LookupResult forum_topic_edges;         // get the forum's topics
  HasTopics->Lookup(edge.forum, & forum_topic_edges);

  bool forum_topics = false, FE_topics_0 = false, FE_topics_1 = false;

  for (auto & FT : forum_topic_edges.value) {                  // for each forum topic
    if (FT.topic == 18419) { forum_topics = true; break; }     // ... topic == Brooklyn
  }

  if (! forum_topics) return false;

  IncludeEdgeType::LookupResult forum_events_included;      // get the forum's events
  Includes->Lookup(forum.id, & forum_events_included);

  for (auto & FE : forum_events_included.value) {            // for each event included in this forum
    bool topics_0_0 = false;                                 // ... does forum event discuss outdoors
    bool topics_0_1 = false;                                 // ................... and Prospect Park
    bool topics_1_0 = false;                                 // ... does forum event discuss Williamsburg 
    bool topics_1_1 = false;                                 // ........................... and Explosion
    bool topics_1_2 = false;                                 // ................................ and Bomb

    HasTopicEdgeType::LookupResult FE_topic_edges;
    HasTopics->Lookup(FE.forum_event, & FE_topic_edges);

    for (auto & FET : FE_topic_edges.value) {                // ... for each topic of this forum event
      if      (FET.topic == 69871376) topics_0_0 |= true;
      else if (FET.topic == 1049632)  topics_0_1 |= true;
      else if (FET.topic == 771572)   topics_1_0 |= true;
      else if (FET.topic == 179057)   topics_1_1 |= true;
      else if (FET.topic == 127197)   topics_1_2 |= true;
    }

  FE_topics_0 |= (topics_0_0 && topics_0_1);
  FE_topics_1 |= (topics_1_0 && topics_1_1 && topics_1_2);

  if (FE_topics_0 && FE_topics_1) return true;
  }

  return false;     // did not find 2 events at this forum that discuss the right topics
}


bool event_pattern_2(intMap & jihad_nyc_forums, shad::AttendEdge & attends, Graph_t & graph) {
  auto HasTopics = HasTopicEdgeType::GetPtr( (HasTopicEdgeType::ObjectID) graph["HasTopic"] );

  HasTopicEdgeType::LookupResult FE_topic_edges;          // get topics linked to this forum event
  HasTopics->Lookup(attends.forum_event, & FE_topic_edges);

  for (auto & FET : FE_topic_edges.value) {               // for each forum event topic
    if (FET.topic != 44311) continue;                     // ... this event did not discuss Jihad

    HasTopicEdgeType::LookupResult forum_topic_edges;     // ... get topics linked to this forum
    HasTopics->Lookup(attends.forum, & forum_topic_edges);

    for (auto & FT : forum_topic_edges.value) {           // ... for each forum topic
      if (FT.topic != 60) continue;                       // ... ... this forum did not discuss NYC

      uint64_t count = jihad_nyc_forums[attends.forum] ++;
      if (count == 1) return true;     // found a forum with that discussed NYC with two events that discussed Jihad
  } }

  return false;                        // did not find a forum that discussed NYC with two events that discussed Jihad
}


bool event_pattern(uint64_t max_forum_date, uint64_t person, Graph_t & graph) {
  auto Attends = AttendEdgeType::GetPtr( (AttendEdgeType::ObjectID) graph["Attends"] );

  bool event_1 = false;
  bool event_2 = false;
  intMap jihad_nyc_forums;                 // map of jihad events at NYC forums <forum, # of events>

  AttendEdgeType::LookupResult attends;
  Attends->Lookup(person, & attends);

  if (attends.size < 2) return false;      // this person can not satissfy the forum event filters

  for (auto & attend : attends.value) {    // for each forum event attended by this person
    event_1 |= event_pattern_1(max_forum_date, attend, graph);
    event_2 |= event_pattern_2(jihad_nyc_forums, attend, graph);
  }

  return event_1 && event_2;     // found forum event subpattern
}


uint64_t trans_pattern(uint64_t person, Graph_t & graph) {
  uint64_t latest_0 = 0, latest_1 = 0, latest_2 = 0, latest_3 = 0;
  auto Sales = SaleEdgeType::GetPtr( (SaleEdgeType::ObjectID) graph["Sales"] );
  auto Authors = AuthorEdgeType::GetPtr( (AuthorEdgeType::ObjectID) graph["Authors"] );
  auto HasOrgs = HasOrgEdgeType::GetPtr( (HasOrgEdgeType::ObjectID) graph["HasOrgs"] );
  auto HasTopics = HasTopicEdgeType::GetPtr( (HasTopicEdgeType::ObjectID) graph["HasTopics"] );
  auto Purchases = PurchaseEdgeType::GetPtr( (PurchaseEdgeType::ObjectID) graph["Purchases"] );
  auto Organizations = OrganizationVertexType::GetPtr( (OrganizationVertexType::ObjectID) graph["Organizations"] );

  PurchaseEdgeType::LookupResult purchases;
  Purchases->Lookup(person, & purchases);

  for (auto & purchase : purchases.value) {                     // for each purchase

    if (purchase.product == 2869238) {                          // ... product = bath bomb
       latest_0 = std::max(latest_0, purchase.date);

    } else if (purchase.product == 271997) {                    // ... product = pressure cooker
       latest_1 = std::max(latest_1, purchase.date);

    } else if (purchase.product == 185785) {                    // ... product = ammunication, 
       SaleEdgeType::LookupResult sales;                        // ... get seller's sales
       Sales->Lookup(purchase.seller, & sales);

       for (auto & sale : sales.value) {                        // ... ... is seller a distributor of ammuninations
         if ( (sale.product == 185785) && (sale.buyer != person) ) {
            latest_2 = std::max(latest_2, purchase.date);
            break;                                              // ... ... only need to find one other buyer
       } }

    } else if (purchase.product == 11650) {                     // ... product = electronics
       AuthorEdgeType::LookupResult sellers_pubs;               // ... get seller's publications
       Authors->Lookup(purchase.seller, & sellers_pubs);

       for (auto & pub : sellers_pubs.value) {                  // ... did seller publish on electrical engineering
         HasTopicEdgeType::LookupResult pub_topic_edges;        // ... ... get topics linked to publication
         HasTopics->Lookup(pub.publication, & pub_topic_edges);

         for (auto & PT : pub_topic_edges.value) {              // ... ... for each topic
           if (PT.topic != 43035) continue;                     // ... ... ... topic is not electrical engineering

           HasOrgEdgeType::LookupResult pub_org_edges;          // ... ... ... get orgs linked to publication
           HasOrgs->Lookup(pub.publication, & pub_org_edges);

           for (auto & PO : pub_org_edges.value) {              // ... ... ... is an author's org close to NYC
              OrganizationVertex org;
              Organizations->Lookup(PO.organization, & org);

              // is organization within 60 miles of NYC
              if (org.location == 60) continue;
              latest_3 = std::max(latest_3, purchase.date);
              break;                                            // ... ... ... ...  only need to find one org
           }
           break;                                               // ... ... ... only need to find topic once
  } }  } }

  if (latest_3 == 0) return 0; else return std::min( std::min(latest_0, latest_1), latest_2 );
}


void WMD_pattern(Graph_t & graph) {
  std::atomic<uint64_t> event_lock = 0;
  auto Events = EventVertexType::GetPtr( (EventVertexType::ObjectID) graph["Events"] );
  auto Persons = PersonVertexType::GetPtr( (PersonVertexType::ObjectID) graph["Persons"] );

// check for acetone chemical reaction event located in Williamsburg
  for (auto event_itr = Events->begin(); event_itr != Events->end(); event_itr ++) {
    if ((* event_itr).second.description.find("acetone") == std::string::npos) continue;
    if ((* event_itr).second.description.find("chemical reaction") == std::string::npos) continue;
    if ((* event_itr).second.location == 771572) continue;

    if (event_lock.fetch_add(1) != 0) return;               // some other thread found a confirming event;

    // find all persons with the right financial transaction and forum event attendence
    for (auto person_itr = Persons->begin(); person_itr != Persons->end(); person_itr ++) {
      uint64_t person = (* person_itr).second.id;
      uint64_t latest_date = trans_pattern(person, graph);
      if (event_pattern(latest_date, person, graph)) printf("pattern found\n");
    }

    break;     // found event and all persons satisfying query
} }            // did not find event
