/*===------------------------------------------------------------*- C++ -*-===
 *
 *                            The AGILE Workflows
 *
 *===----------------------------------------------------------------------===
 *
 * Copyright (c) 2025 Battelle Memorial Institute
 *
 * Battelle Memorial Institute (hereinafter Battelle) hereby grants permission
 * to any person or entity lawfully obtaining a copy of this software and
 * associated documentation files (hereinafter “the Software”) to redistribute
 * and use the Software in source and binary forms, with or without
 * modification. Such person or entity may use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and may permit
 * others to do so, subject to the following conditions:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimers.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Other than as used herein, neither the name Battelle Memorial Institute or
 *    Battelle may be used in any form whatsoever without the express written
 *    consent of Battelle.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL BATTELLE OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *===----------------------------------------------------------------------===*/
#include "agile/wk2_exact/main.h"
#include "agile/wk2_exact/graph.h"
#include "agile/wk2_exact/pattern.h"

namespace agile::wk2_exact {

bool proximity(TopicVertex & A, TopicVertex & B) {
  double lon_miles = 0.91 * std::abs(A.lon - B.lon);
  double lat_miles = 1.15 * std::abs(A.lat - B.lat);
  double distance = std::sqrt( lon_miles * lon_miles + lat_miles * lat_miles );
  return distance <= 30.0;
}


// Check if person authored two forum events satisfying forum 1 SP and a forum event satisfying forum 2 SP
bool forumEvent(uint64_t person, time_t trans_date, RF_args_t & args) {
  bool forum_1 = false, forum_2 = false;
  auto Authors = AuthorEdgeType::GetPtr((AuthorEdgeOID) args.Authors_OID);
  auto TopicsMap = TopicMap::GetPtr((TopicMapOID) args.TopicsMap_OID);
  auto ForumEvents = ForumEventVertexType::GetPtr((ForumEventVertexType::ObjectID) args.ForumEvents_OID);

  AuthorEdgeType::LookupResult authoredItems;                // get person's authored items
  Authors->Lookup(person, & authoredItems);

  for (auto & AI: authoredItems.value) {                     // for each authored item
    if (AI.dst_type != TYPES::FORUMEVENT) continue;          // ... authored item is not a forum event

    TopicMapVertex TMV;
    ForumEventVertex FEV;
    ForumEvents->Lookup(AI.item, & FEV);                     // ... get forum event vertex
    if (! TopicsMap->Lookup(FEV.forum, & TMV)) continue;     // ... get forum event's forum map entry

    // forum event's forum discusses NYC and includes 2 forum events that discuss jihad
    if (! forum_1) forum_1 = (TMV.NYC && (TMV.jihad > 1));

    // forum event's forum roots subpattern FE4 and FE5 and date is earlier than transaction date
    if (! forum_2) forum_2 = (TMV.FE4 && TMV.FE5 && (TMV.date < trans_date));

    if (forum_1 && forum_2) return true;
  }

  return false;
}


// Check if person bought an electronic product from a seller who published an item
// on electronic engineering associated with an organization near NYC
bool electronic_subpattern(uint64_t seller, TopicVertex & NYC, RF_args_t & args) {
  auto Authors   = AuthorEdgeType::GetPtr((AuthorEdgeOID) args.Authors_OID);
  auto Topics    = TopicVertexType::GetPtr((TopicVertexOID) args.Topics_OID);
  auto HasOrg    = HasOrgEdgeType::GetPtr((HasOrgEdgeOID) args.HasOrg_OID);
  auto TopicsMap = TopicMap::GetPtr((TopicMapOID) args.TopicsMap_OID);

  AuthorEdgeType::LookupResult documents;                        // get seller's documents
  Authors->Lookup(seller, & documents);

  for (auto & document : documents.value) {                      // for each document
    if (document.dst_type != TYPES::PUBLICATION) continue;       // ... document is not a publication

    TopicMapVertex TMV;                                          // ... get publication's topics
    if (! TopicsMap->Lookup(document.item, & TMV)) continue;     // ... publication topic is not EE
    if (! TMV.ELE) continue;                                     // ... publication topic is not EE

    HasOrgEdgeType::LookupResult organizations;                  // ... get publication's organizations
    HasOrg->Lookup(document.item, & organizations);

    for (auto & PO : organizations.value) {                      // ... for each organization
      TopicVertex org;
      Topics->Lookup(PO.organization, & org);
      if (proximity(org, NYC)) return true;                      // ... ... organization is close to NYC
  } }

  return false;
}


// Check if person bought ammunition from a distributor (... defined as a seller of ammunition with
// at least two different customers
bool ammunition_subpattern(uint64_t buyer, uint64_t seller, RF_args_t & args) {
  SaleEdgeType::LookupResult sales;                 // get seller's sales
  SaleEdgeType::GetPtr((SaleEdgeOID) args.Sales_OID)->Lookup(seller, & sales);

  for (auto & S1 : sales.value)     // if sale is ammunition and buyer is not person, return true
    if ( (S1.product == 185785) && (S1.buyer != buyer) ) return true;

  return false;
}


void PersonPattern(const uint64_t & person, std::vector<PurchaseEdge> & purchases, RF_args_t & args) {
  std::vector<PurchaseEdge> ammunitionSales;
  std::vector<PurchaseEdge> electronicSales;
  time_t latest_BB = 0, latest_PC = 0, latest_AMO = 0;

// ***** TRANSACTION SUBPATTERN ***** //
  for (auto & PO : purchases) {                     // for each purchase
    if (PO.product == 2869238) {                    // ... product is a bath bomb
       latest_BB = std::max(latest_BB, PO.date);

    } else if (PO.product == 271997) {              // ... product is a pressure cooker
       latest_PC = std::max(latest_PC, PO.date);

    } else if (PO.product == 185785) {              // ... save sale to check for ammunition distributor
       ammunitionSales.push_back(PO);

    } else if (PO.product == 11650) {               // ... save sale to check electronic subpattern
       electronicSales.push_back(PO);
  } }

  // no bomb bath, pressure cooker, ammunition, or electronic purchase, so return
  if (latest_BB == 0 || latest_PC == 0 || ammunitionSales.size() == 0 || electronicSales.size() == 0) return;

  for (auto & PO : ammunitionSales)
    if (PO.date > latest_AMO)
       if (ammunition_subpattern(PO.buyer, PO.seller, args)) latest_AMO = PO.date;

  if (latest_AMO == 0) return;                             // no ammunition sales by distributor, so return

  TopicVertex NYC;
  TopicVertexType::GetPtr((TopicVertexOID) args.Topics_OID)->Lookup(60, & NYC);
  time_t trans_date = std::min( std::min(latest_BB, latest_PC), latest_AMO );

  for (auto & PO : electronicSales)
    if (electronic_subpattern(PO.seller, NYC, args)) {     // publication subpattern found, check forum events
       if (forumEvent(person, trans_date, args)) printf("pattern found for person %lu\n", person);
       break;
}   };


// Find each topic pattern and save it by its root forum or publication id
// ... TopicMap vertex{id, FE4, FE5, NYC, ELE, jihad, date}
void topicPatterns(Handle & handle, const uint64_t & item, std::vector<HasTopicEdge> & edges, RF_args_t & args) {
  time_t maxTime = shad::data_types::kNullValue<time_t>;
  auto TopicsMap = TopicMap::GetPtr((TopicMapOID) args.TopicsMap_OID);
  auto ForumEvents = ForumEventVertexType::GetPtr((ForumEventVertexOID) args.ForumEvents_OID);

  if (edges[0].src_type == TYPES::FORUM)              {    // this item is a forum, look for NYC topic
     for (auto & edge : edges) {                           // ... for each edge
       if (edge.topic == 60)   {                           // ... ... topic is NYC
          TopicsMap->AsyncInsert(handle, item, TopicMapVertex(item, false, false, true, false, 0, maxTime));
          return;
     } }

  } else if (edges[0].src_type == TYPES::FORUMEVENT)  {    // this item is a forum event
     uint64_t FE4 = 0, FE5 = 0, jihad = 0;                 // ... look for FE4, FE5, or jihad topic

     for (auto & edge : edges) {                           // ... for each edge
       if      (edge.topic == 771572)   FE4 |= 1;          // ... ... topic is Williamsburg
       else if (edge.topic == 179057)   FE4 |= 2;          // ... ... topic is Explosion
       else if (edge.topic == 127197)   FE4 |= 4;          // ... ... topic is Bomb
       else if (edge.topic == 69871376) FE5 |= 1;          // ... ... topic is Outdoors
       else if (edge.topic == 1049632)  FE5 |= 2;          // ... ... topic is Prospect Park
       else if (edge.topic == 44311)    jihad = 1;         // ... ... topic is Jihad
     }

     if (FE4 == 7 || FE5 == 3 || jihad == 1) {             // ... forum event is FE4 or FE5 or has topic jihad
        ForumEventVertex FEV;                              // ... ... get this forum event's forum id
        ForumEvents->Lookup(item, & FEV);

        if (FE4 == 7) {
           TopicMapVertex tmp(FEV.forum, true, false, false, false, 0, FEV.date);
           TopicsMap->AsyncInsert(handle, FEV.forum, tmp);
        }
        if (FE5 == 3) {
           TopicMapVertex tmp(FEV.forum, false, true, false, false, 0, maxTime);
           TopicsMap->AsyncInsert(handle, FEV.forum, tmp);
        }
        if (jihad == 1) {
           TopicMapVertex tmp(FEV.forum, false, false, false, false, 1, maxTime);
           TopicsMap->AsyncInsert(handle, FEV.forum, tmp);
     }  }

  } else if (edges[0].src_type == TYPES::PUBLICATION) {    // this item is a publication
     for (auto & edge : edges)  {                          // ... for each edge
       if (edge.topic == 43035) {                          // ... ... topic is electrical engineering
          TopicsMap->AsyncInsert(handle, item, TopicMapVertex(item, false, false, false, true, 0, maxTime));
          return;
} }  } }


void WMD_pattern(RF_args_t & args) {
  Handle handle;
  auto HasTopic = HasTopicEdgeType::GetPtr((HasTopicEdgeOID) args.HasTopic_OID);
  auto Purchases = PurchaseEdgeType::GetPtr((PurchaseEdgeOID) args.Purchases_OID);

  auto TopicsMap = TopicMap::Create(AGILE_TINY);                // forums that include FE4 and FE5
  args.TopicsMap_OID = (uint64_t) (TopicsMap->GetGlobalID());
  HasTopic->AsyncForEachEntry(handle, topicPatterns, args);     // for each forum, check for FE4 and FE5

  waitForCompletion(handle);
  if (TopicsMap->Size() == 0) return;                           // no forum includes both FE4 and FE5

// find all persons with the right financial transaction and forum event attendence
  Purchases->ForEachEntry(PersonPattern, args);
}

} // namespace agile::wk2_exact
