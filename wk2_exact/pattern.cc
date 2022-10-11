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

struct Pattern_args_t {
  uint64_t ForumEventsOID;
  uint64_t TopicsOID;
  uint64_t SalesOID;
  uint64_t PurchasesOID;
  uint64_t AuthorsOID;
  uint64_t IncludesOID;
  uint64_t HasOrgOID;
  uint64_t HasTopicOID;
  uint64_t Forums_2_OID;
};


bool proximity(TopicVertex & A, TopicVertex & B) {
  double lon_miles = 0.91 * std::abs(A.lon - B.lon);
  double lat_miles = 1.15 * std::abs(A.lat - B.lat);
  double distance = std::sqrt( lon_miles * lon_miles + lat_miles * lat_miles );
  return distance <= 30.0;
}


// Check to see forum event has topic Jihad and occurred at a forum with topic NYC
// if yes, insert the forum id in jihadForums
//    if insertion fails, then second such forum event found for that forum, so RETURN TRUE
bool forum_1_subpattern(std::set<uint64_t> & jihadForums, uint64_t forum_event, Pattern_args_t & args) {
  auto HasTopic = HasTopicEdgeType::GetPtr((HasTopicEdgeType::ObjectID) args.HasTopicOID);
  auto ForumEvents = ForumEventVertexType::GetPtr((ForumEventVertexType::ObjectID) args.ForumEventsOID);

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
bool forum_2_subpattern(time_t trans_date, uint64_t forum_event, Pattern_args_t args) {
  auto Forums_2    = intTimeMap::GetPtr((intTimeMapOID) args.Forums_2_OID);
  auto ForumEvents = ForumEventVertexType::GetPtr((ForumEventVertexType::ObjectID) args.ForumEventsOID);

  ForumEventVertex FEV;                                // get forum event vertex
  ForumEvents->Lookup(forum_event, & FEV);

  time_t forum_event_date;         
  Forums_2->Lookup(FEV.forum, & forum_event_date);     // forum's value stored in Forums_2

return (trans_date > forum_event_date);
}


// Check if person authored two forum events satisfying forum 1 SP and a forum event satisfying forum 2 SP
bool forumEvent_subpattern(uint64_t person, time_t date, Pattern_args_t & args) {
  std::set<uint64_t> jihadForums;
  bool forum_1 = false, forum_2 = false;
  auto Authors  = AuthorEdgeType::GetPtr((AuthorEdgeOID) args.AuthorsOID);

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
bool electronic_subpattern(uint64_t seller, Pattern_args_t & args) {
  auto Topics   = TopicVertexType::GetPtr((TopicVertexOID) args.TopicsOID);
  auto Authors  = AuthorEdgeType::GetPtr((AuthorEdgeOID) args.AuthorsOID);
  auto HasTopic = HasTopicEdgeType::GetPtr((HasTopicEdgeOID) args.HasTopicOID);
  auto HasOrg   = HasOrgEdgeType::GetPtr((HasOrgEdgeOID) args.HasOrgOID);

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
        TopicVertex NYC, org;
        Topics->Lookup(60, & NYC);
        Topics->Lookup(PO.organization, & org);

        if (proximity(org, NYC)) return true;         // ... ... ... ... organization is close to NYC
  } } }

  return false;
}


// Check if person bought ammunition from a distributor (... defined as a seller of ammunition with
// at least two different customers
bool ammunition_subpattern(uint64_t buyer, uint64_t seller, Pattern_args_t & args) {
  SaleEdgeType::LookupResult sales;                 // get seller's sales
  SaleEdgeType::GetPtr((SaleEdgeOID) args.SalesOID)->Lookup(seller, & sales);

  for (auto & S1 : sales.value)     // if sale is ammunition and buyer is not person, return true
    if ( (S1.product == 185785) && (S1.buyer != buyer) ) return true;

  return false;
}


void transEvents(const uint64_t & key, PersonVertex & person, Pattern_args_t & args) {
  bool ESP = false;
  time_t latest_BB = 0, latest_PC = 0, latest_AMO = 0;
  auto Purchases = PurchaseEdgeType::GetPtr((PurchaseEdgeOID) args.PurchasesOID);

  PurchaseEdgeType::LookupResult purchases;         // get person's purchases
  Purchases->Lookup(person.id, & purchases);

// ***** TRANSACTION SUBPATTERN ***** //
  for (auto & PO : purchases.value) {               // for each purchase

    if (PO.product == 2869238) {                    // ... product is a bath bomb
       latest_BB = std::max(latest_BB, PO.date);

    } else if (PO.product == 271997) {              // ... product is a pressure cooker
       latest_PC = std::max(latest_PC, PO.date);

    } else if (PO.product == 185785) {              // ... product is a ammunition 
       if (PO.date > latest_AMO)
          if (ammunition_subpattern(PO.buyer, PO.seller, args)) latest_AMO = PO.date;

    } else if (PO.product == 11650) {               // ... product is a electronics
       if (! ESP) ESP = electronic_subpattern(PO.seller, args);
  } }

  // earliest of the lastest individual transaction dates
  time_t trans_date = std::min( std::min(latest_BB, latest_PC), latest_AMO );
  if (( trans_date == 0) || (! ESP)) return;        // person failed the transaction or electronic subpattern

// ***** FORUM SUBPATTERN ***** //
  if (forumEvent_subpattern(person.id, trans_date, args)) {
     printf("pattern found for person %lu\n", person.id);
     return;
} };


// Check if forum includes a forum event with topics Williamsburg, Explosion, and Bomb
void forumPattern_2B(const uint64_t & forum, time_t & date, Pattern_args_t & args) {
  Handle handle;
  auto Includes = IncludesEdgeType::GetPtr((IncludesEdgeOID) args.IncludesOID);

  auto Lambda2B = []             // for each forum -> FE
  (Handle & handle, const uint64_t & forum, std::vector<IncludesEdge> & includes, Pattern_args_t & args) {

    auto lambdaLambda2B = []     // for each FE -> topic
    (Handle & handle, const uint64_t & FE, std::vector<HasTopicEdge> & FET, Pattern_args_t & args) {
      bool topic_1  = false;                             // does forum event discuss Williamsburg
      bool topic_2  = false;                             // ................ and Explosion
      bool topic_3  = false;                             // ................ and Bomb
      auto Forums_2 = intTimeMap::GetPtr((intTimeMapOID) args.Forums_2_OID);
      auto ForumEvents = ForumEventVertexType::GetPtr((ForumEventVertexOID) args.ForumEventsOID);

      for (auto & T : FET) {                             // for each topic
        if      (T.topic == 771572) topic_1 = true;      // ... topic is Williamsburg
        else if (T.topic == 179057) topic_2 = true;      // ... topic is Explosion
        else if (T.topic == 127197) topic_3 = true;      // ... topic is Bomb
      }

      if (topic_1 && topic_2 && topic_3) {               // FE discusses all topics
         ForumEventVertex FEV;
         ForumEvents->Lookup(FE, & FEV);
         Forums_2->AsyncInsert(handle, FEV.forum, FEV.date);
    } };     // lambdaLambda_2B ... FE -> topic

    auto HasTopic = HasTopicEdgeType::GetPtr((HasTopicEdgeOID) args.HasTopicOID);
    for (auto FFE : includes) HasTopic->AsyncApply(handle, FFE.forum_event, lambdaLambda2B, args);
  };         // Lambda_2B ... forum -> FE

  Includes->AsyncApply(handle, forum, Lambda2B, args);
  waitForCompletion(handle);
}


// Check if forum includes a forum event with topics Outdoors and Prospect Park
void forumPattern_2A(const uint64_t & forum, std::vector<IncludesEdge> & includes, Pattern_args_t & args) {
  Handle handle;
  auto HasTopic = HasTopicEdgeType::GetPtr((HasTopicEdgeOID) args.HasTopicOID);

  auto Lambda2A = []     // for each FE -> topic
  (Handle & handle, const uint64_t & FE, std::vector<HasTopicEdge> & FET, Pattern_args_t & args) {
    bool topic_1 = false;                               // does forum event discuss outdoors
    bool topic_2 = false;                               // ................ and Prospect Park
    auto Forums_2 = intTimeMap::GetPtr((intTimeMapOID) args.Forums_2_OID);
    auto ForumEvents = ForumEventVertexType::GetPtr((ForumEventVertexOID) args.ForumEventsOID);

    for (auto & T : FET) {                              // for each topic
      if      (T.topic == 69871376) topic_1 = true;     // ... topic is Outdoors
      else if (T.topic == 1049632)  topic_2 = true;     // ... topic is Prospect Park
    }

    if (topic_1 && topic_2) {     // FE discusses both topics, so forum satisfies forum 2A subpattern
       ForumEventVertex FEV;
       ForumEvents->Lookup(FE, & FEV);
       Forums_2->AsyncInsert(handle, FEV.forum, shad::data_types::kNullValue<time_t>);
  } };      // Lambda2A ... FE -> topic

  for (auto FFE : includes) HasTopic->AsyncApply(handle, FFE.forum_event, Lambda2A, args);
  waitForCompletion(handle);
}


void WMD_pattern(Graph_t & graph) {
  Pattern_args_t args;
  auto Forums_2 = intTimeMap::Create(TINY);     // for each forum, the earliest date of a included FE4 vertex
  auto Persons  = PersonVertexType::GetPtr((PersonVertexOID) graph["Persons"]);
  auto Includes = IncludesEdgeType::GetPtr((IncludesEdgeOID) graph["Includes"]);

  args.ForumEventsOID  = graph["ForumEvents"];
  args.TopicsOID       = graph["Topics"];
  args.SalesOID        = graph["Sales"];
  args.PurchasesOID    = graph["Purchases"];
  args.AuthorsOID      = graph["Authors"];
  args.IncludesOID     = graph["Includes"];
  args.HasOrgOID       = graph["HasOrg"];
  args.HasTopicOID     = graph["HasTopic"];
  args.Forums_2_OID    = (uint64_t) (Forums_2->GetGlobalID());

  Includes->ForEachEntry(forumPattern_2A, args);     // F -> FE {Prospect Park, Outdoors}
  Forums_2->ForEachEntry(forumPattern_2B, args);     // F -> FE {Bomb, Explosion, Williamsburg}
  if (Forums_2->Size() == 0) return;

// find all persons with the right financial transaction and forum event attendence
  Persons->ForEachEntry(transEvents, args);
}

} // namespace agile::wk2_exact
