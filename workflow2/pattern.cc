#include "agile/workflow2/main.h"
#include "agile/workflow2/graph.h"
#include "agile/workflow2/pattern.h"

using intMap = std::map<uint64_t, uint64_t>;

namespace agile::workflow2 {

struct Pattern_args_t {
  uint64_t PersonsOID;
  uint64_t ForumsOID;
  uint64_t ForumEventsOID;
  uint64_t PublicationsOID;
  uint64_t TopicsOID;
  uint64_t SalesOID;
  uint64_t PurchasesOID;
  uint64_t AuthorsOID;
  uint64_t IncludesOID;
  uint64_t HasOrgOID;
  uint64_t HasTopicOID;
  uint64_t Forums_2A_OID;
  uint64_t Forums_2B_OID;
};


bool proximity(TopicVertex & A, TopicVertex & B) {
  double lon_miles = 0.91 * std::abs(A.lon - B.lon);
  double lat_miles = 1.15 * std::abs(A.lat - B.lat);
  double distance = std::sqrt( lon_miles * lon_miles + lat_miles * lat_miles );
  return distance <= 30.0;
}


// Check to see forum event FE has topic Jihad and occurred at a forum with topic NYC
// if yes and it is the second such forum event to be found for the forum, then RETURN TRUE
// if yes and it is the first  such forum event to be found for the forum, then set forum count to 1 and RETURN FALSE
// if no, then RETURN FALSE
bool forum_pattern_1(intMap & jihadForums, uint64_t FE, Pattern_args_t & args) {
  auto HasTopic = HasTopicEdgeType::GetPtr((HasTopicEdgeType::ObjectID) args.HasTopicOID);
  auto ForumEvents = ForumEventVertexType::GetPtr((ForumEventVertexType::ObjectID) args.ForumEventsOID);

// check if forum event has topic Jihad
  HasTopicEdgeType::LookupResult FE_topics;
  HasTopic->Lookup(FE, & FE_topics);           // get topics linked to this forum event

  for (auto & FET : FE_topics.value) {         // for each forum event topic
    if (FET.topic != 44311) continue;          // ... this topic is not Jihad

// check if forum has topic NYC
    ForumEventVertex FEV;                      // ... get forum event vertex
    ForumEvents->Lookup(FE, & FEV);
    auto entry = jihadForums.insert( std::make_pair(FEV.forum, 0) );

    if (entry.second == false) {               // ... forum in map, so we already know if forum has topic NYC
       return (* entry.first).second == 1;     // ... ... 1: forum has topic NYC & FE is second with topic Jihad
                                               // ... ... 0: forum does not have topic NYC
    } else {                                   // ... forum not in map, so check if it has topic NYC
       HasTopicEdgeType::LookupResult forum_topics;          // ... ... get forum's topics
       HasTopic->Lookup(FEV.forum, & forum_topics);

       for (auto & FT : forum_topics.value)                  // ... ... for each forum topic
         if (FT.topic == 60) jihadForums[FEV.forum] = 1;     // ... ... ... forum has topic NYC

       return false;                            // ... ... FE has topic Jihad, but first encountered for forum 
  } }

  return false;                                 // forum event does not have topic Jihad
}


// Check to see if forum is in Forums_2A and Forums_2B.
// if yes, then check if PD is after forum's 2B value; else return false
bool check_forum_pattern_2(time_t PD, uint64_t FE, Pattern_args_t args) {
  auto Forums_2A   = intSet::GetPtr((intSetOID) args.Forums_2A_OID);
  auto Forums_2B   = intTimeMap::GetPtr((intTimeMapOID) args.Forums_2B_OID);
  auto ForumEvents = ForumEventVertexType::GetPtr((ForumEventVertexType::ObjectID) args.ForumEventsOID);

  time_t time;
  ForumEventVertex FEV;                      // ... get forum event vertex
  ForumEvents->Lookup(FE, & FEV);

  if (Forums_2A->Find(FEV.forum) && Forums_2B->Lookup(FEV.forum, & time)) return (PD > time);
  return false;
}


// Check if forum event discusses topics Outdoors and Prospect Park
void patternForum_2A_(shad::rt::Handle & handle, const uint64_t & FE,
   std::vector<HasTopicEdge> & FET, Pattern_args_t & args) {

   bool topic_1 = false;                               // does forum event discuss outdoors
   bool topic_2 = false;                               // ................ and Prospect Park
   auto Forums_2A   = intSet::GetPtr((intSetOID) args.Forums_2A_OID);
   auto ForumEvents = ForumEventVertexType::GetPtr((ForumEventVertexOID) args.ForumEventsOID);

   for (auto & T : FET) {                              // for each topic
     if      (T.topic == 69871376) topic_1 = true;     // ... topic is Outdoors
     else if (T.topic == 1049632)  topic_2 = true;     // ... topic is Prospect Park
   }

   if (topic_1 && topic_2) {                           // FE discusses both topics
      ForumEventVertex FEV;
      ForumEvents->Lookup(FE, & FEV);
      Forums_2A->AsyncInsert(handle, FEV.forum);
} }


// Check if forum event discusses topics Williamsbug, Explosion, and Bomb
void patternForum_2B_(shad::rt::Handle & handle, const uint64_t & FE,
     std::vector<HasTopicEdge> & FET, Pattern_args_t & args) {

  bool topic_1 = false;                              // does forum event discuss Williamsburg
  bool topic_2 = false;                              // ................ and Explosion
  bool topic_3 = false;                              // ................ and Bomb
  auto Forums_2B   = intTimeMap::GetPtr((intTimeMapOID) args.Forums_2B_OID);
  auto ForumEvents = ForumEventVertexType::GetPtr((ForumEventVertexOID) args.ForumEventsOID);

  for (auto & T : FET) {                             // for each topic
    if      (T.topic == 771572) topic_1 = true;      // ... topic is Williamsburg
    else if (T.topic == 179057) topic_2 = true;      // ... topic is Explosion
    else if (T.topic == 127197) topic_3 = true;      // ... topic is Bomb
  }

  if (topic_1 && topic_2 && topic_3) {               // FE discusses all topics
     ForumEventVertex FEV;
     ForumEvents->Lookup(FE, & FEV);
     Forums_2B->AsyncInsert(handle, FEV.forum, FEV.date);
} }


// Check if forum includes a forum event with topics Outdoors and Prospect Park
void patternForum_2A(shad::rt::Handle & handle, const uint64_t & forum,
     std::vector<IncludesEdge> & includes, Pattern_args_t & args) {

  auto HasTopic = HasTopicEdgeType::GetPtr((HasTopicEdgeOID) args.HasTopicOID);
  for (auto FFE : includes) HasTopic->AsyncApply(handle, FFE.forum_event, patternForum_2A_, args);
}


// Check if forum includes a forum event with topics Williamsburg, Explosion, and Bomb
void patternForum_2B(shad::rt::Handle & handle, const uint64_t & forum,
     std::vector<IncludesEdge> & includes, Pattern_args_t & args) {

  auto HasTopic = HasTopicEdgeType::GetPtr((HasTopicEdgeOID) args.HasTopicOID);
  for (auto FFE : includes) HasTopic->AsyncApply(handle, FFE.forum_event, patternForum_2B_, args);
}


// Check if person bought ammunition from a distributor (... defined as a seller of ammunition with
// at least two different customers
bool ammunition_subpattern(uint64_t buyer, uint64_t seller, Pattern_args_t & args) {
  SaleEdgeType::LookupResult sales;                 // get seller's sales
  SaleEdgeType::GetPtr((SaleEdgeOID) args.SalesOID)->Lookup(seller, & sales);

  for (auto & sale : sales.value)     // if sale is ammunition and buyer is not person, return true
    if ( (sale.product == 185785) && (sale.buyer != buyer) ) return true;

  return false;
}


// Check if person bought an electronic product from a seller who published an item
// on electronic engineering associated with an organization near NYC
bool electronic_subpattern(uint64_t seller, Pattern_args_t & args) {
  auto Topics   = TopicVertexType::GetPtr((TopicVertexOID) args.TopicsOID);
  auto Authors  = AuthorEdgeType::GetPtr((AuthorEdgeOID) args.AuthorsOID);
  auto HasTopic = HasTopicEdgeType::GetPtr((HasTopicEdgeOID) args.HasTopicOID);
  auto HasOrg   = HasOrgEdgeType::GetPtr((HasOrgEdgeOID) args.HasOrgOID);

  AuthorEdgeType::LookupResult documents;                  // get seller's documents
  Authors->Lookup(seller, & documents);

  for (auto & document : documents.value) {                // for each document
    if (document.type != TYPES::PUBLICATION) continue;     // ... this document is not a publication

    HasTopicEdgeType::LookupResult topics;                 // ... get document's topics
    HasTopic->Lookup(document.item, & topics);

    for (auto & PT : topics.value) {                       // ... for each doucment topic
      if (PT.topic != 43035) continue;                     // ... ... topic is not electrical engineering

      HasOrgEdgeType::LookupResult organizations;          // ... ... get document's organizations
      HasOrg->Lookup(document.item, & organizations);

      for (auto & PO : organizations.value) {              // ... ... ... for each organization
        TopicVertex NYC, org;
        Topics->Lookup(60, & NYC);
        Topics->Lookup(PO.organization, & org);

        if (proximity(org, NYC)) return true;              // ... ... ... ... organization is close to NYC
  } } }

  return false;
}


void transEvents(const uint64_t & key, PersonVertex & person, Pattern_args_t & args) {
  intMap jihadForums;                                     // map of jihad events at NYC forums
  time_t latest_0 = 0, latest_1 = 0, latest_2 = 0;
  bool ESP = false, forum_1 = false, forum_2 = false;
  auto Purchases = PurchaseEdgeType::GetPtr((PurchaseEdgeOID) args.PurchasesOID);
  auto Authors = AuthorEdgeType::GetPtr((AuthorEdgeType::ObjectID) args.AuthorsOID);

  AuthorEdgeType::LookupResult authored;                  // get person's authored documents
  Authors->Lookup(person.id, & authored);
  if (authored.value.size() < 2) return;                  // need at least 2 documents to satisfy pattern

  PurchaseEdgeType::LookupResult purchases;               // get person's purchases
  Purchases->Lookup(person.id, & purchases);

// ***** TRANSACTION SUBPATTERN ***** //
  for (auto & purchase : purchases.value) {               // for each purchase

    if (purchase.product == 2869238) {                    // ... product is a bath bomb
       latest_0 = std::max(latest_0, purchase.date);

    } else if (purchase.product == 271997) {              // ... product is a pressure cooker
       latest_1 = std::max(latest_1, purchase.date);

    } else if (purchase.product == 185785) {              // ... product is a ammunition 
                                                          // ... ... date is not covered or seller is a distributor
       if (purchase.date > latest_2 || ammunition_subpattern(purchase.buyer, purchase.seller, args))
          latest_2 = std::max(latest_2, purchase.date);

    } else if (purchase.product == 11650) {               // ... product is a electronics
       if (! ESP) ESP = electronic_subpattern(purchase.seller, args);
  } }

  time_t PD = std::min( std::min(latest_0, latest_1), latest_2 );
  if (( PD == 0) || (! ESP)) return;                      // person failed the transaction or electronic subpattern

// ***** FORUM SUBPATTERN ***** //
  for (auto & document : authored.value) {                // for each document
    if (document.type != TYPES::FORUMEVENT) continue;     // ... document is not a forum event
    if (! forum_1) forum_1 = forum_pattern_1(jihadForums, document.item, args);
    if (! forum_2) forum_2 = check_forum_pattern_2(PD, document.item, args);
    if (forum_1 && forum_2) break;
  }

  if ( ! (forum_1 && forum_2) ) return;                   // person failed forum subpattern
  printf("pattern found for person %lu\n", person.id);
}


void WMD_pattern(Graph_t & graph) {
  Pattern_args_t args;
  shad::rt::Handle handle;
  auto Forums_2A = intSet::Create(TINY);
  auto Forums_2B = intTimeMap::Create(TINY);
  auto Persons   = PersonVertexType::GetPtr((PersonVertexOID) graph["Persons"]);
  auto Includes  = IncludesEdgeType::GetPtr((IncludesEdgeOID) graph["Includes"]);

  args.PersonsOID      = graph["Persons"];
  args.ForumsOID       = graph["Forums"];
  args.ForumEventsOID  = graph["ForumEvents"];
  args.PublicationsOID = graph["Publications"];
  args.TopicsOID       = graph["Topics"];
  args.SalesOID        = graph["Sales"];
  args.PurchasesOID    = graph["Purchases"];
  args.AuthorsOID      = graph["Authors"];
  args.IncludesOID     = graph["Includes"];
  args.HasOrgOID       = graph["HasOrg"];
  args.HasTopicOID     = graph["HasTopic"];
  args.Forums_2A_OID   = (uint64_t) (Forums_2A->GetGlobalID());
  args.Forums_2B_OID   = (uint64_t) (Forums_2B->GetGlobalID());

  Includes->AsyncForEachEntry(handle, patternForum_2A, args);     // F -> FE {Prospect Park, Outdoors}
  Includes->AsyncForEachEntry(handle, patternForum_2B, args);     // F -> FE {Bomb, Explosion, Williamsburg}
  waitForCompletion(handle);

  if ( (Forums_2A->Size() == 0) || (Forums_2B->Size() == 0) ) return;

// find all persons with the right financial transaction and forum event attendence
  Persons->ForEachEntry(transEvents, args);
}

} // namespace agile::workflow2
