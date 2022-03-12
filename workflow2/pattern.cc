#include "agile/workflow2/main.h"
#include "agile/workflow2/graph.h"

using intMap = std::map<uint64_t, uint64_t>;

namespace agile::workflow2 {

struct Pattern_args_t {
  uint64_t personsOID;
  uint64_t forumEventsOID;
  uint64_t forumsOID;
  uint64_t publicationsOID;
  uint64_t topicsOID;
  uint64_t purchasesOID;
  uint64_t salesOID;
  uint64_t authorsOID;
  uint64_t includesOID;
  uint64_t hasOrgOID;
  uint64_t hasTopicOID;
};


bool proximity(TopicVertex & A, TopicVertex & B) {
  double lon_miles = 0.91 * std::abs(A.lon - B.lon);
  double lat_miles = 1.15 * std::abs(A.lat - B.lat);
  double distance = std::sqrt( lon_miles * lon_miles + lat_miles * lat_miles );
  return distance <= 30.0;
}


/*
// Check to see if person authored a forum event that occurred at a forum that included:
//     1) a forum event with {topics outdoors and Prospect Park} and
//     2) a forum event with topics {Williamsburg, Explosion, and Bomb} that occured before PD
bool event_pattern_1(uint64_t PD, uint64_t & forum_event, Patterns_args_t args) {
  auto Forums = ForumVertexType::GetPtr((ForumVertexType::ObjectID) args.forumsOID);
  auto HasTopic = HasTopicEdgeType::GetPtr((HasTopicEdgeType::ObjectID) args.hasTopicOID);

  OccursAtEdgeType::LookupResult forum;            // get forum event's forum
  OccursAt->Lookup(forum_event, & forum);
  bool FE_topics_0 = false, FE_topics_1 = false;

  OccursAtEdgeType::LookupResult forum_events;     // get the forum's events
  OccursAt->Lookup(forum.id, & forum_events);

  for (auto & FE : forum_events.value) {           // for each event included in this forum
    bool topics_0_0 = false;                       // ... does forum event discuss outdoors
    bool topics_0_1 = false;                       // ................... and Prospect Park
    bool topics_1_0 = false;                       // ... does forum event discuss Williamsburg 
    bool topics_1_1 = false;                       // ........................... and Explosion
    bool topics_1_2 = false;                       // ................................ and Bomb

    HasTopicEdgeType::LookupResult FE_topic_edges;
    HasTopic->Lookup(FE.item, & FE_topic_edges);

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


// Check to see this forum event had topic Jihad and occurred at a forum with topic NYC
bool event_pattern_1(intMap & forum_map, uint64_t & forum_event, Pattern_args_t & args) {
  auto HasTopic = HasTopicEdgeType::GetPtr((HasTopicEdgeType::ObjectID) args.hasTopicOID);
  auto ForumEvents = ForumEventVertexType::GetPtr((ForumEventVertexType::ObjectID) args.forumEventsOID);

  ForumEventVertex FEV;                              // get this forum event's vertex
  ForumEvents->Lookup(forum_event, & FEV);
  HasTopicEdgeType::LookupResult FE_topics;          // get topics linked to this forum event
  HasTopic->Lookup(forum_event, & FE_topics);
  HasTopicEdgeType::LookupResult forum_topics;       // get topics linked to forum event's forum
  HasTopic->Lookup(FEV.forum, & forum_topics);

  for (auto & FET : FE_topics.value) {               // for each forum event topic
    if (FET.topic != 44311) continue;                // ... this topic is not Jihad


    for (auto & FT : forum_topics.value) {           // ... for each forum topic
      if (FT.topic != 60) continue;                  // ... ... this forum did not discuss NYC

      uint64_t count = forum_map[forum.value[0].forum] ++;
      if (count == 1) return true;     // found a forum with topic NYC with two events that discussed Jihad
  } }

  return false;                        // did not find a forum that discussed NYC with two events that discussed Jihad
}


bool event_pattern(uint64_t person, time_t PD, Pattern_args_t args) {
  bool event_1 = false;
  bool event_2 = false;
  intMap jihad_nyc_forums;                      // map of jihad events at NYC forums <forum, # of events>
  auto Authors = AuthorsEdgeType::GetPtr((AuthorsEdgeType::ObjectID) args.authorsOID);

  AttendEdgeType::LookupResult authored;
  Authors->Lookup(person, & authored);
  if (authored.size < 2) return false;            // this person can not satissfy the forum event filters

  for (auto & forum_event : authored.value) {             // for each forum event authored by this person
    if (forum_event.document_type != TYPES::forum_event) continue;     // this entry is not a forum event
    event_1 |= event_pattern_1(PD, forum_event.document, args);
    event_2 |= event_pattern_2(forum_map, forum_event.document, args);
  }

  return event_1 && event_2;     // found forum event subpattern
}
*/


// Check if person bought ammunition from a distributor (... defined as a seller of ammunition with
// at least two different customers
bool ammunition_subpattern(uint64_t buyer, uint64_t seller, Pattern_args_t & args) {
  SaleEdgeType::LookupResult sales;                 // get seller's sales
  SaleEdgeType::GetPtr((SaleEdgeOID) args.salesOID)->Lookup(seller, & sales);

  for (auto & sale : sales.value)     // if sale is ammunition and buyer is not person, return true
    if ( (sale.product == 185785) && (sale.buyer != buyer) ) return true;

  return false;
}


// Check if person bought an electronic product from a seller who published an item
// on electronic engineering associated with an organization near NYC
bool electronic_subpattern(uint64_t seller, Pattern_args_t & args) {
  auto Topics   = TopicVertexType::GetPtr((TopicVertexOID) args.topicsOID);
  auto Authors  = AuthorEdgeType::GetPtr((AuthorEdgeOID) args.authorsOID);
  auto HasTopic = HasTopicEdgeType::GetPtr((HasTopicEdgeOID) args.hasTopicOID);
  auto HasOrg   = HasOrgEdgeType::GetPtr((HasOrgEdgeOID) args.hasOrgOID);

  AuthorEdgeType::LookupResult authored;              // get seller's authored items
  Authors->Lookup(seller, & authored);

  for (auto & publication : authored.value) {         // for each authored item by seller
    uint64_t pub_id = publication.item;
    HasTopicEdgeType::LookupResult topics;            // ... get topics linked to publication
    HasTopic->Lookup(pub_id, & topics);

    for (auto & PT : topics.value) {                  // ... for each publication topic
      if (PT.topic != 43035) continue;                // ... ... if topic is not electrical engineering, continue

      HasOrgEdgeType::LookupResult organizations;     // ... ... get organizations linked to publication
      HasOrg->Lookup(pub_id, & organizations);

      for (auto & PO : organizations.value) {         // ... ... ... for each organization
        TopicVertex NYC, org;
        Topics->Lookup(60, & NYC);
        Topics->Lookup(PO.organization, & org);

        if (proximity(org, NYC)) return true;         // ... ... ... ... if organization close to NYC return true
  } } }

  return false;
}


void transEvents(const uint64_t & key, PersonVertex & person, Pattern_args_t & args) {
  bool ESP = false;
  time_t latest_0 = 0, latest_1 = 0, latest_2 = 0;
  auto Purchases = PurchaseEdgeType::GetPtr((PurchaseEdgeOID) args.purchasesOID);

  PurchaseEdgeType::LookupResult purchases;
  Purchases->Lookup(person.id, & purchases);

  for (auto & purchase : purchases.value) {                      // for each purchase

    if (purchase.product == 2869238) {                           // ... product = bath bomb
       latest_0 = std::max(latest_0, purchase.date);

    } else if (purchase.product == 271997) {                     // ... product = pressure cooker
       latest_1 = std::max(latest_1, purchase.date);

    } else if (purchase.product == 185785) {                     // ... product = ammunication, 
       if (purchase.date <= latest_2) continue;                                 // ... this purchase is covered  
       if (! ammunition_subpattern(purchase.buyer, purchase.seller, args));     // ... seller is not a distributor
       latest_2 = std::max(latest_2, purchase.date);

    } else if (purchase.product == 11650) {                      // ... product = electronics
       ESP |= electronic_subpattern(purchase.seller, args);

  } }

  if (! electronic_subpattern) return;                           // person failed the electronic subpattern
  time_t PD = std::min( std::min(latest_0, latest_1), latest_2 );

  // if (event_pattern(purchase.buyer, PD, args))  printf("pattern found\n");
}


void WMD_pattern(Graph_t & graph) {
  Pattern_args_t args = {
       graph["Persons"],  graph["ForumEvents"],
       graph["Forums"],   graph["Publications"],
       graph["Topics"],   graph["Purchases"],
       graph["Sales"],    graph["Authors"],
       graph["Includes"], graph["HasTopic"],
       graph["HasOrg"]
  };

// find all persons with the right financial transaction and forum event attendence
  auto Persons = PersonVertexType::GetPtr((PersonVertexOID) graph["Persons"]);
  Persons->ForEachEntry(transEvents, args);
}

} // namespace agile::workflow2
