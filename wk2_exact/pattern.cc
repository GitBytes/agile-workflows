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


// Check to see forum event has topic Jihad and occurred at a forum with topic NYC
// if yes, insert the forum id in jihadForums
//    if insertion fails, then second such forum event found for that forum, so RETURN TRUE
bool forum_1_subpattern(std::set<uint64_t> & jihadForums, uint64_t forum_event, RF_args_t & args) {
  auto HasTopic = HasTopicEdgeType::GetPtr((HasTopicEdgeType::ObjectID) args.HasTopic_OID);
  auto ForumEvents = ForumEventVertexType::GetPtr((ForumEventVertexType::ObjectID) args.ForumEvents_OID);

  // check if forum event has topic Jihad
  HasTopicEdgeType::LookupResult topics;
  HasTopic->Lookup(forum_event, & topics);          // get forum event's topics

  for (auto & T1 : topics.value) {                  // for each forum event topic
    if (T1.topic != 44311) continue;                // ... topic is not Jihad

    // check if forum has topic NYC
    ForumEventVertex FEV;                           // ... get forum event vertex
    ForumEvents->Lookup(forum_event, & FEV);

    HasTopicEdgeType::LookupResult forum_topics;    // ... get forum's topics
    HasTopic->Lookup(FEV.forum, & forum_topics);

    for (auto & FT : forum_topics.value) {          // ... for each forum topic
      if (FT.topic != 60) continue; ;               // ... ... topic is not NYC

      auto insert = jihadForums.insert(FEV.forum);
      return (insert.second == false);              // false -> second insertion of forum id
  } }

  return false;                                     // forum event does not have topic Jihad
}


// Check if forum event is in a forum that satisfies forum 2 subpattern
bool forum_2_subpattern(time_t trans_date, uint64_t forum_event, RF_args_t args) {
  auto ForumsMap    = ForumMap::GetPtr((ForumMapOID) args.ForumsMap_OID);
  auto ForumEvents = ForumEventVertexType::GetPtr((ForumEventVertexType::ObjectID) args.ForumEvents_OID);

  ForumEventVertex FEV;                       // get forum event vertex
  ForumEvents->Lookup(forum_event, & FEV);

  ForumMapVertex FMV;         
  bool found = ForumsMap->Lookup(FEV.forum, & FMV);
  return ( found && FMV.FE4 && FMV.FE5 && (FMV.date < trans_date) );
}


// Check if person authored two forum events satisfying forum 1 SP and a forum event satisfying forum 2 SP
bool forumEvent(uint64_t person, time_t date, RF_args_t & args) {
  std::set<uint64_t> jihadForums;
  bool forum_1 = false, forum_2 = false;
  auto Authors  = AuthorEdgeType::GetPtr((AuthorEdgeOID) args.Authors_OID);

  AuthorEdgeType::LookupResult events;             // get person's events
  Authors->Lookup(person, & events);

  for (auto & EV : events.value) {                 // for each P -> FE
    if (EV.dst_type != TYPES::FORUMEVENT) continue;

    if (! forum_1) forum_1 = forum_1_subpattern(jihadForums, EV.item, args);
    if (! forum_2) forum_2 = forum_2_subpattern(date, EV.item, args);
    if (forum_1 && forum_2) return true;            // ... forum event subpattern satisfied
  }

  return false;                                     // person failed forum subpattern

}


// Check if person bought an electronic product from a seller who published an item
// on electronic engineering associated with an organization near NYC
bool electronic_subpattern(uint64_t seller, TopicVertex & NYC, RF_args_t & args) {
  auto Topics   = TopicVertexType::GetPtr((TopicVertexOID) args.Topics_OID);
  auto Authors  = AuthorEdgeType::GetPtr((AuthorEdgeOID) args.Authors_OID);
  auto HasTopic = HasTopicEdgeType::GetPtr((HasTopicEdgeOID) args.HasTopic_OID);
  auto HasOrg   = HasOrgEdgeType::GetPtr((HasOrgEdgeOID) args.HasOrg_OID);

  AuthorEdgeType::LookupResult documents;             // get seller's documents
  Authors->Lookup(seller, & documents);

  for (auto & PUB : documents.value) {                // for each publication
    if (PUB.dst_type != TYPES::PUBLICATION) continue;

    HasTopicEdgeType::LookupResult topics;            // ... get publication's topics
    HasTopic->Lookup(PUB.item, & topics);

    for (auto & PT : topics.value) {                  // ... for each topic
      if (PT.topic != 43035) continue;                // ... ... topic is not electrical engineering

      HasOrgEdgeType::LookupResult organizations;     // ... ... get publication's organizations
      HasOrg->Lookup(PUB.item, & organizations);

      for (auto & PO : organizations.value) {         // ... ... ... for each organization
        TopicVertex org;
        Topics->Lookup(PO.organization, & org);

        if (proximity(org, NYC)) return true;         // ... ... ... ... organization is close to NYC
  } } }

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

    } else if (PO.product == 185785) {              // ... safe sale to check for ammunition distributor
       ammunitionSales.push_back(PO);

    } else if (PO.product == 11650) {               // ... safe sale to check electronic subpattern
       electronicSales.push_back(PO);
  } }

  // no bomb bath, pressure cooker, ammunition, or electronic purchase, so return
  if (latest_BB == 0 || latest_PC == 0 || ammunitionSales.size() == 0 || electronicSales.size() == 0) return;

  for (auto & PO : ammunitionSales)
    if (PO.date > latest_AMO)
       if (ammunition_subpattern(PO.buyer, PO.seller, args)) latest_AMO = PO.date;

  if (latest_AMO == 0) return;                           // no ammunition sales by distributor, so return

  TopicVertex NYC;
  TopicVertexType::GetPtr((TopicVertexOID) args.Topics_OID)->Lookup(60, & NYC);
  time_t trans_date = std::min( std::min(latest_BB, latest_PC), latest_AMO );

  for (auto & PO : electronicSales)
    if (electronic_subpattern(PO.seller, NYC, args))     // electronic publication subpattern found
       if (forumEvent(person, trans_date, args)) {printf("pattern found for person %lu\n", person); break;}
};


// Check if forum includes a FE4 and FE5 subpattern
void forumPattern(Handle & handle, const uint64_t & FE, std::vector<HasTopicEdge> & edges, RF_args_t & args) {
  uint64_t FE4 = 0;
  uint64_t FE5 = 0;
  auto ForumsMap = ForumMap::GetPtr((ForumMapOID) args.ForumsMap_OID);
  auto ForumEvents = ForumEventVertexType::GetPtr((ForumEventVertexOID) args.ForumEvents_OID);

  if (edges[0].src_type != TYPES::FORUMEVENT) return;     // src is not a forum event

  for (auto & edge : edges) {                             // for each edge
    if      (edge.topic == 69871376) FE4 |= 1;            // ... topic is Outdoors
    else if (edge.topic == 1049632)  FE4 |= 2;            // ... topic is Prospect Park
    else if (edge.topic == 771572)   FE5 |= 1;            // ... topic is Williamsburg
    else if (edge.topic == 179057)   FE5 |= 2;            // ... topic is Explosion
    else if (edge.topic == 127197)   FE5 |= 4;            // ... topic is Bomb
  }

  if (FE4 == 3) {                                         // forum includes FE4
     ForumEventVertex FEV;
     ForumEvents->Lookup(FE, & FEV);
     time_t maxTime = shad::data_types::kNullValue<time_t>;
     ForumsMap->AsyncInsert(handle, FEV.forum, ForumMapVertex(FEV.forum, true, false, maxTime));
  }
  if (FE5 == 7) {                                         // forum includes FE5
     ForumEventVertex FEV;
     ForumEvents->Lookup(FE, & FEV);
     ForumsMap->AsyncInsert(handle, FEV.forum, ForumMapVertex(FEV.forum, false, true, FEV.date));
} }


void WMD_pattern(RF_args_t & args) {
  Handle handle;
  auto HasTopic = HasTopicEdgeType::GetPtr((HasTopicEdgeOID) args.HasTopic_OID);
  auto Purchases = PurchaseEdgeType::GetPtr((PurchaseEdgeOID) args.Purchases_OID);

  auto ForumsMap = ForumMap::Create(AGILE_TINY);               // forums that include FE4 and FE5
  args.ForumsMap_OID = (uint64_t) (ForumsMap->GetGlobalID());
  HasTopic->AsyncForEachEntry(handle, forumPattern, args);     // for each forum, check for FE4 and FE5

  waitForCompletion(handle);
  if (ForumsMap->Size() == 0) return;                          // no forum includes both FE4 and FE5

// find all persons with the right financial transaction and forum event attendence
  Purchases->ForEachEntry(PersonPattern, args);
}

} // namespace agile::wk2_exact
