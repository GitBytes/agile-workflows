#include "agile/workflow2/graph.h"
#include "agile/workflow2/main.h"
/*
 * Types of edges
 *  Sale edge (&purchase edge)[buyer - seller - product - date] -- BLUE
 *  Includes Edge [forum - forumevent] -- GRAY
 *  HasTopic Edge -- ORANGE
 *     [forum- topic]
 *     [forumevent - topic]
 *     [publication - topic]
 *  HasOrg Edge [publication - organization] -- YELLOW
 *  Author Edge -- BLACK
 *     [Person - Forumevent]
 *     [Person - Publication]
 *
 */
namespace shad {
using namespace agile::workflow2;
std::queue<std::string> dataQueue;
std::mutex queueMutex;
std::condition_variable dataCV;
shad::rt::Handle patternHandle;

std::vector<std::string> split(std::string &line, char delim,
                               uint64_t size = 0) {
  uint64_t ndx = 0, start = 0;
  std::vector<std::string> tokens(size);

  for (uint64_t end = 0; end < line.length(); end++) {

    if ((line[end] == delim) || (line[end] == '\n')) {
      tokens[ndx] = line.substr(start, end - start);
      start = end + 1;
      ndx++;
    }
  }

  return tokens;
}

bool proximity(TopicVertex &A, TopicVertex &B) {
  double lon_miles = 0.91 * std::abs(A.lon - B.lon);
  double lat_miles = 1.15 * std::abs(A.lat - B.lat);
  double distance = std::sqrt(lon_miles * lon_miles + lat_miles * lat_miles);
  return distance <= 30.0;
}

void StreamDataFile(shad::rt::Handle &, const std::string &dataFile) {
  double time_start = my_timer(); // temp to finish experiment
  double max_time = 600;
  int waitMillisec = 5;
  std::ifstream file(dataFile);
  if (file.is_open()) {
    printf("reading file %s\n", dataFile.c_str());
  } else {
    printf("Cannot open file %s\n", dataFile.c_str());
    exit(-1);
  }
  std::string line;
  while (getline(file, line)) {
    if (line[0] == '#')
      continue;
    std::this_thread::sleep_for(std::chrono::milliseconds(waitMillisec));
    std::unique_lock<std::mutex> ul(queueMutex);
    dataQueue.push(line);
    ul.unlock();
    // printf("added another line\n");
    dataCV.notify_one();
    if (my_timer() - time_start > max_time) {
      std::unique_lock<std::mutex> ul(queueMutex);
      dataQueue.push("###END OF TIME###");
      printf("Stream timed out\n");
      ul.unlock();
      break;
    }
  }
  std::unique_lock<std::mutex> ul(queueMutex);
  dataQueue.push("###END OF FILE###");
  printf("Stream file ended\n");
  ul.unlock();
  printf("stream stopped\n");
}

bool SubPattern1Check(std::vector<std::string> &tokens, Graph_t &graph) {
  bool ret = false;

  uint64_t FE = ENCODE<uint64_t, std::string, UINT>(tokens[4]);
  // TODO: check FE is valid, in case of bad data?
  auto HasTopic =
      HasTopicEdgeType::GetPtr((HasTopicEdgeType::ObjectID)graph["HasTopic"]);
  auto SubPattern1 = shad::Set<uint64_t>::GetPtr(
      (shad::ObjectIdentifier<shad::Set<uint64_t>>)graph["SubPattern1"]);
  HasTopicEdgeType::LookupResult FE_topics;
  HasTopic->Lookup(FE, &FE_topics);

  // check if forumevent has 2 events outdoors and prospectpark
  bool topic_1 = false;
  bool topic_2 = false;
  for (auto &FET : FE_topics.value) {
    if (FET.topic == 69871376) {
      topic_1 = true; // ... topic is Outdoors
    } else if (FET.topic == 1049632) {
      topic_2 = true; // ... topic is Prospect Park
    }
    if (topic_1 && topic_2) {
      SubPattern1->AsyncInsert(patternHandle, FE);
      ret = true;
      break;
    }
  }
  return ret;
}

bool SubPattern2Check(std::vector<std::string> &tokens, Graph_t &graph) {

  bool ret = false;
  uint64_t FE = ENCODE<uint64_t, std::string, UINT>(tokens[4]);
  // TODO: check FE is valid, in case of bad data?
  auto HasTopic =
      HasTopicEdgeType::GetPtr((HasTopicEdgeType::ObjectID)graph["HasTopic"]);
  auto SubPattern2 = shad::Hashmap<uint64_t, time_t>::GetPtr(
      (shad::ObjectIdentifier<shad::Hashmap<uint64_t, time_t>>)
          graph["SubPattern2"]);

  HasTopicEdgeType::LookupResult FE_topics;
  HasTopic->Lookup(FE, &FE_topics);
  // check if forumevent has 3 events Bomb, Explosion, Williamsburg
  bool topic_1 = false;
  bool topic_2 = false;
  bool topic_3 = false;
  for (auto &FET : FE_topics.value) {
    if (FET.topic == 771572) {
      topic_1 = true; // ... topic is Williamsburg
    } else if (FET.topic == 179057) {
      topic_2 = true; // ... topic is Explosion
    } else if (FET.topic == 127197) {
      topic_3 = true;
    }
    if (topic_1 && topic_2 && topic_3) {
      // get the date of the forumevent
      auto ForumEvents = ForumEventVertexType::GetPtr(
          (ForumEventVertexType::ObjectID)graph["ForumEvents"]);
      ForumEventVertex FEvertex;
      ForumEvents->Lookup(FE, &FEvertex);

      SubPattern2->AsyncInsert(patternHandle, FE, FEvertex.date);
      ret = true;
      break;
    }
  }
  return ret;
}

bool SubPattern12Check(std::vector<std::string> &tokens, Graph_t &graph) {
  bool ret = false;
  uint64_t FE = ENCODE<uint64_t, std::string, UINT>(tokens[4]);
  auto ForumEvents = ForumEventVertexType::GetPtr(
      (ForumEventVertexType::ObjectID)graph["ForumEvents"]);
  auto Includes = IncludesEdgeType::GetPtr((IncludesEdgeOID)graph["Includes"]);
  auto SubPattern1 = shad::Set<uint64_t>::GetPtr(
      (shad::ObjectIdentifier<shad::Set<uint64_t>>)graph["SubPattern1"]);
  auto SubPattern2 = shad::Hashmap<uint64_t, time_t>::GetPtr(
      (shad::ObjectIdentifier<shad::Hashmap<uint64_t, time_t>>)
          graph["SubPattern2"]);
  auto SubPattern12 = shad::Hashmap<uint64_t, time_t>::GetPtr(
      (shad::ObjectIdentifier<shad::Hashmap<uint64_t, time_t>>)
          graph["SubPattern12"]);

  ForumEventVertex FEV;
  ForumEvents->Lookup(FE, &FEV);
  uint64_t ForumID = FEV.forum;
  IncludesEdgeType::LookupResult FEs;
  Includes->Lookup(ForumID, &FEs); // this might come back as null, if we didn't
                                   // get the includes info for forum yet

  bool SP1 = false;
  bool SP2 = false;
  time_t date = time_t(0); // init to today
  time_t tempDate;

  for (auto &FE_id : FEs.value) {
    if (SubPattern2->Lookup(FE_id.forum_event, &tempDate)) {
      SP2 = true;
      if (tempDate < date) {
        date = tempDate;
      }
    }
    if (SubPattern1->Find(FE_id.forum_event)) {
      SP1 = true;
    }
  }

  if (SP1 && SP2) {
    SubPattern12->AsyncInsert(patternHandle, ForumID, date);
    return true;
  }
  return false;
}

bool SubPattern7Check(std::vector<std::string> &tokens, Graph_t &graph) {
  bool ret = false;
  using SP7Type = shad::Hashmap<uint64_t, int64_t>;
  auto SubPattern7 =
      SP7Type::GetPtr((shad::ObjectIdentifier<SP7Type>)graph["SubPattern7"]);

  uint64_t seller = ENCODE<uint64_t, std::string, UINT>(tokens[1]);
  int64_t buyer = ENCODE<int64_t, std::string, UINT>(tokens[2]);

  int64_t ammo_buyer;
  SubPattern7->Lookup(seller, &ammo_buyer);

  if (&ammo_buyer == NULL) {
    // first time seeing this ammunition seller
    SubPattern7->AsyncInsert(patternHandle, seller, buyer);
  } else if (ammo_buyer == -1) {
    // we already know seller is a distributor
    ret = true;
  } else if (ammo_buyer != buyer) {
    // there are 2 separate buyers of ammo
    SubPattern7->AsyncInsert(patternHandle, seller,
                             -1); // hashmap insertion policy is overwrite
    ret = true;
  }
  return ret;
}

bool SubPattern3Check(std::vector<std::string> &tokens, Graph_t &graph) {
  auto Includes = IncludesEdgeType::GetPtr((IncludesEdgeOID)graph["Includes"]);
  auto SubPattern4 = shad::Set<uint64_t>::GetPtr(
      (shad::ObjectIdentifier<shad::Set<uint64_t>>)graph["SubPattern4"]);

  using SP3Type = shad::Hashmap<uint64_t, uint64_t>;
  auto SubPattern3 =
      SP3Type::GetPtr((shad::ObjectIdentifier<SP3Type>)graph["SubPattern3"]);
  auto HasTopic =
      HasTopicEdgeType::GetPtr((HasTopicEdgeType::ObjectID)graph["HasTopic"]);
  uint64_t ForumID = ENCODE<uint64_t, std::string, UINT>(tokens[3]);

  // check for NYC forum
  bool NYC = false;
  HasTopicEdgeType::LookupResult forum_topics; // ... get forum's topics
  HasTopic->Lookup(ForumID, &forum_topics);
  for (auto &FT : forum_topics.value) { // ... for each forum topic
    if (FT.topic == 60) {
      NYC = true;
      break;
    }
  }

  if (NYC) {
    IncludesEdgeType::LookupResult FEs;
    Includes->Lookup(ForumID,
                     &FEs); // this might come back as null, if we didn't
                            // get the includes info for forum yet

    std::set<uint64_t> events;
    for (auto FE : FEs.value) {
      if (SubPattern4->Find(FE.forum_event)) {
        events.insert(FE.forum_event);
      }
    }

    if (events.size() > 1) { // at least 2 forum events with Jihad info
      for (auto e : events) {
        SubPattern3->AsyncInsert(patternHandle, e, ForumID);
      }
      return true;
    }
  }
  return false;
}

bool SubPattern5Check(std::vector<std::string> &tokens, Graph_t &graph) {
  uint64_t PubID = ENCODE<uint64_t, std::string, UINT>(tokens[5]);

  // if topic is electrical engineering
  if (tokens[6] == "43035") {
    // check for an organization close to NYC
    auto HasOrg = HasOrgEdgeType::GetPtr((HasOrgEdgeType::ObjectID)graph["HasOrg"]);
    auto Topics = TopicVertexType::GetPtr((TopicVertexOID)graph["Topics"]);
    auto SubPattern5 = shad::Set<uint64_t>::GetPtr(
        (shad::ObjectIdentifier<shad::Set<uint64_t>>)graph["SubPattern5"]);
    TopicVertex NYC, org;
    Topics->Lookup(60, &NYC);

    HasOrgEdgeType::LookupResult organizations;
    HasOrg->Lookup(PubID, &organizations);

    for (auto PubOrg : organizations.value) {
      Topics->Lookup(PubOrg.organization, &org);
      if (proximity(org, NYC)) {
        SubPattern5->AsyncInsert(patternHandle, PubID);
        return true; // one org close to NY satisfies the subpattern
      }
    }
  }
  return false;
}

bool SubPattern5CheckForOrg(std::vector<std::string> &tokens, Graph_t &graph){
  uint64_t PubID = ENCODE<uint64_t, std::string, UINT>(tokens[5]);
  uint64_t PubOrg = ENCODE<uint64_t, std::string, UINT>(tokens[6]);
  auto Topics   = TopicVertexType::GetPtr((TopicVertexType::ObjectID) graph["Topics"]);
  auto HasTopic = HasTopicEdgeType::GetPtr((HasTopicEdgeType::ObjectID)graph["HasTopic"]);
  auto SubPattern5 = shad::Set<uint64_t>::GetPtr((shad::ObjectIdentifier<shad::Set<uint64_t>>)graph["SubPattern5"]);
  TopicVertex NYC, org;
  Topics->Lookup(60, &NYC);
  Topics->Lookup(PubOrg, &org);

  if (proximity(org, NYC)) {
    HasTopicEdgeType::LookupResult topics;            // ... get publication's topics
    HasTopic->Lookup(PubID, &topics);
    for(auto top : topics.value){
      if(top.topic == 43035){//electrical engineerin
        SubPattern5->AsyncInsert(patternHandle, PubID);
        return true;
      }
    }
  }
  return false;
}

bool SubPattern6Check(std::vector<std::string> &tokens, Graph_t &graph) {
  auto Purchases =
      PurchaseEdgeType::GetPtr((PurchaseEdgeOID)graph["Purchases"]);
  using SP7Type = shad::Hashmap<uint64_t, int64_t>;
  auto SubPattern7 =
      SP7Type::GetPtr((shad::ObjectIdentifier<SP7Type>)graph["SubPattern7"]);

  auto SubPattern6 = shad::Hashmap<uint64_t, time_t>::GetPtr(
      (shad::ObjectIdentifier<shad::Hashmap<uint64_t, time_t>>)
          graph["SubPattern6"]);
  uint64_t buyer = ENCODE<uint64_t, std::string, UINT>(tokens[2]);
  uint64_t seller = ENCODE<uint64_t, std::string, UINT>(tokens[1]);
  PurchaseEdgeType::LookupResult purchaseList;
  Purchases->Lookup(buyer, &purchaseList);

  time_t latest_BB = 0, latest_PC = 0, latest_AMO = 0;

  for (auto p : purchaseList.value) {
    if (p.product == 2869238) { // ... product is a bath bomb
      latest_BB = std::max(latest_BB, p.date);
    } else if (p.product == 271997) { // ... product is a pressure cooker
      latest_PC = std::max(latest_PC, p.date);
    } else if (p.product == 185785) { // ... product is a ammunition
      if (p.date > latest_AMO) {
        int64_t ammo_buyer;
        SubPattern7->Lookup(seller, &ammo_buyer);
        if (ammo_buyer == -1) { // if seller is a distributor
          latest_AMO = p.date;
        }
      }
    }
  }
  time_t trans_date = std::min(std::min(latest_BB, latest_PC), latest_AMO);

  if (trans_date == 0) { // no pattern found
    return false;
  } else {
    SubPattern6->AsyncInsert(patternHandle, buyer, trans_date);
    return true;
  }
}

// We are looking for a person with
// SP3 - 2 jihad forumeventss at a nyc forum
// SP12 - attended a forumevent which is in a forum from SP12
// SP5 - 1 purchase from a person with publication S5
// SP6- 3 purchases (bath bomb, pressure cooker, ammo from distributor
void PatternCheckForPerson(shad::rt::Handle &, const uint64_t &key,
                           PersonVertex &person, Graph_t &graph) {
  bool SP3 = false;
  bool SP12 = false;
  bool SP5 = false;
  bool SP6 = false;

  using SP3Type = shad::Hashmap<uint64_t, uint64_t>;
  auto Authors = AuthorEdgeType::GetPtr((AuthorEdgeOID)graph["Authors"]);
  auto ForumEvents = ForumEventVertexType::GetPtr((ForumEventVertexType::ObjectID)graph["ForumEvents"]);
  auto Purchases = PurchaseEdgeType::GetPtr((PurchaseEdgeOID)graph["Purchases"]);
  auto SubPattern3 = SP3Type::GetPtr((shad::ObjectIdentifier<SP3Type>)graph["SubPattern3"]);
  auto SubPattern12 = shad::Hashmap<uint64_t, time_t>::GetPtr((shad::ObjectIdentifier<shad::Hashmap<uint64_t, time_t>>)graph["SubPattern12"]);
  auto SubPattern6 = shad::Hashmap<uint64_t, time_t>::GetPtr((shad::ObjectIdentifier<shad::Hashmap<uint64_t, time_t>>)graph["SubPattern6"]);
  auto SubPattern5 = shad::Set<uint64_t>::GetPtr((shad::ObjectIdentifier<shad::Set<uint64_t>>)graph["SubPattern5"]);

  AuthorEdgeType::LookupResult events;
  Authors->Lookup(key, &events);

  // SP3 check

  // we store FE --> Forum SP3
  // for each FE of a person,
  // check if in SP3 , if so add forum to set
  // if addition returns false (second insertion, we found the pattern)
  std::set<uint64_t> ForumSet;
  for (auto ev : events.value) { // for each Forum Event of a person
    if (ev.dst_type != TYPES::FORUMEVENT)
      continue;

    uint64_t ForumID;
    bool find = SubPattern3->Lookup(ev.item, &ForumID);
    if (find) {
      auto result = ForumSet.insert(ForumID);
      if (!result.second) {
        SP3 = true;
        break;
      }
    }
  }

  if (SP3) {
    // SP12 check
    time_t SP12date = time_t(0);
    for (auto ev : events.value) { // for each Forum Event of a person
      if (ev.dst_type != TYPES::FORUMEVENT)
        continue;

      // get the forum of the forumevent
      ForumEventVertex FEV;
      ForumEvents->Lookup(ev.item, &FEV);
      uint64_t ForumID = FEV.forum;
      // if forum satisfies SP12, check the date
      time_t date;
      if (SubPattern12->Lookup(ForumID, &date)) {
        SP12 = true;
        if (date < SP12date) {
          SP12date = date;
        }
      }
    }

    if (SP12) {

      // SP6 check that happened after SP1
      // check SP6 only if we have a date from SP1 to compare against
      time_t SP6date;
      if (SubPattern6->Lookup(key, &SP6date)) {
        if (SP6date >
            SP12date) { // purchase happened after the SP1 forum attendance
          SP6 = true;
        }
      }

      if (SP6) {
        // SP5 check
        PurchaseEdgeType::LookupResult purchaseList;
        Purchases->Lookup(key, &purchaseList);

        for (auto purchase : purchaseList.value) {
          if (purchase.product == 11650) {
            auto seller = purchase.seller;
            // check if seller authored a SP5 publication
            AuthorEdgeType::LookupResult pubs;
            Authors->Lookup(seller, &pubs);
            for (auto pub : pubs.value) {
              // do we need to do a if publication check? if it isn't it won't
              // be in the graph, how expensive is the set lookup?
              if (SubPattern5->Find(pub.item)) {
                SP5 = true;
                break;
              }
            }
          }
        }

        if (SP5) {
          // we found the pattern!!
          printf("Pattern found for person %ld\n", key);
        }
      }
    }
  }
}

// check if a pattern is formed for each person vertex in the graph
void PatternCheck(Graph_t &graph) {
  auto Persons = PersonVertexType::GetPtr((PersonVertexOID)graph["Persons"]);
  Persons->AsyncForEachEntry(patternHandle, PatternCheckForPerson, graph);
}

struct Pattern_args_t {
  Graph_t *graph;
  std::string dataLine;
  TYPES t;
};

void CheckPartialMatch(shad::rt::Handle &, const Pattern_args_t &args) {

  Graph_t graph = *(args.graph);
  TYPES t = args.t;
  std::string dataLine = args.dataLine;
  // edges
  if (t == TYPES::HASTOPIC) {
    // 3 types of hastopic, for publication, for forum, for forumevent
    // we can differentiate by looking at which field is filled (tokens[3]->
    // forumID , tokens[4]->forumeventID, tokens[5]->publicationID)
    std::vector<std::string> tokens = split(dataLine, ',', 10);

    // if this is a FORUMEVENT with TOPIC info
    if (tokens[4] != "") {

      // trigger to set if we need to do an upper level partial match check
      bool SP1 = SubPattern1Check(tokens, graph);
      bool SP2 = SubPattern2Check(tokens, graph);

      // check for forums that has a FE in SP1 and SP2
      if (SP1 || SP2) { // if we added this forumevent to either SubPattern
                        // board, we do an upper level check
        bool SP12 = SubPattern12Check(tokens, graph);

        // check if this addition resulted in a full pattern match
        if (SP12) {
          PatternCheck(graph);
        }
      }

      // check if topic is Jihad
      if (tokens[6] == "44311") {
        uint64_t FE = ENCODE<uint64_t, std::string, UINT>(tokens[4]);
        auto SubPattern4 = shad::Set<uint64_t>::GetPtr(
            (shad::ObjectIdentifier<shad::Set<uint64_t>>)graph["SubPattern4"]);
        // I THINK WE NEED THIS HANDLE TO COMPLETE BEFORE NEXT LEVEL CHECK, SO
        // NOT ASYNC
        SubPattern4->Insert(FE);
        bool SP3 = SubPattern3Check(tokens, graph);
        // check if this addition resulted in a full pattern match
        if (SP3) {
          PatternCheck(graph);
        }
      }
    }
    // This is a Forum with Topic Info
    if (tokens[3] != "") {
      uint64_t ForumID = ENCODE<uint64_t, std::string, UINT>(tokens[3]);
      // if topic info is NYC
      if (tokens[6] == "60") {
        // check for 2 FEs in SP4
        if (SubPattern3Check(tokens, graph)) {
          // check if this addition resulted in a full pattern match
          PatternCheck(graph);
        }
      }
    }

    // This is a publication with topic info
    if (tokens[5] != "") {
      bool SP5 = SubPattern5Check(tokens, graph);
      // check if this addition resulted in a full pattern match
      if (SP5) {
        PatternCheck(graph);
      }
    }
  } else if (t == TYPES::SALE) {
    std::vector<std::string> tokens = split(dataLine, ',', 10);
    // check for electrical pattern
    bool SP5 = false;
    bool distributor = false;
    if (tokens[6] == "11650") {
      // check if SELLER (tokens[1]) has a publication in publications with
      // ElEng and nyc org--> (SP5)

      uint64_t seller = ENCODE<uint64_t, std::string, UINT>(tokens[1]);
      auto Authors = AuthorEdgeType::GetPtr((AuthorEdgeOID)graph["Authors"]);
      auto SubPattern5 = shad::Set<uint64_t>::GetPtr(
          (shad::ObjectIdentifier<shad::Set<uint64_t>>)graph["SubPattern5"]);
      AuthorEdgeType::LookupResult pubList;
      Authors->Lookup(seller, &pubList);
      for (auto pub : pubList.value) {
        if (SubPattern5->Find(pub.item)) {
          SP5 = true;
          break;
        }
      }
    }

    // check for ammo pattern
    else if (tokens[6] == "185785") {
      distributor = SubPattern7Check(tokens, graph);
    }

    // Check for blue pattern (person bought bathbomb, pressure cooker, ammo
    // from distributor) SP6
    bool SP6 = SubPattern6Check(tokens, graph);

    if (distributor) { // if we found a new distributor, we might have
                       // completed someone's pattern, check every person
      PatternCheck(graph);
    }
    // this buyer satisfies blue pattern or publication pattern. check if we
    // see whole pattern for this buyer
    else if (SP6 || SP5) {
      uint64_t buyer = ENCODE<uint64_t, std::string, UINT>(tokens[2]);
      auto Persons =
          PersonVertexType::GetPtr((PersonVertexOID)graph["Persons"]);
      PersonVertex person;
      Persons->Lookup(buyer, &person);
      PatternCheckForPerson(patternHandle, buyer, person, graph);
    }
  } else if (t == TYPES::INCLUDES) {
    std::vector<std::string> tokens = split(dataLine, ',', 10);
    // check SP12
    bool SP12 = SubPattern12Check(tokens, graph);

    // check Jihad subpattern
    bool SP3 = SubPattern3Check(tokens, graph);

    // check for overall pattern (forum-fe3)
    PatternCheck(graph);
  } else if (t == TYPES::HASORG) {
    std::vector<std::string> tokens = split(dataLine, ',', 10);
    // check for SP5
    bool SP5 = SubPattern5CheckForOrg(tokens, graph);
    // if added SP5, check for overall pattern
    if(SP5){
      PatternCheck(graph);
    }
  } else if (t == TYPES::AUTHOR) {
    // check for overall pattern
    //OPTIMIZATION
    // patterncheckforperson for author
    // check if person satisfies a SP5
      // if so; check for all all people 
    PatternCheck(graph);
  }
  // vertices !!
  else if (t == TYPES::PERSON) {
    //check for SubPattern7 (ammo distributor)
    //check for SubPattern6 (bath bomb, pressure cooker, ammo)
    //check for person authored SP5
    //check for full pattern match
  } else if (t == TYPES::FORUM) {
    //check for subpattern12
    //check for subpattern3
    //if any new matches check for full pattern
  } else if (t == TYPES::FORUMEVENT) {
    //check for SubPattern1 & Subpattern2
      //if one matches, check for subpattern12 
    //check for SubPattern3 
    //if match to SP3 or SP12, check for full pattern
  } else if (t == TYPES::PUBLICATION) {
    //check for SP5
    //if new match, check for full pattern
  } else if (t == TYPES::TOPIC) {
    //we might need to check for every topic related subpattern (1, 2, 12, 3, ,4, 5)
    //then full pattern
  }
}

int main(int argc, char *argv[]) {
  double time1 = my_timer();

  Graph_t graph;
  std::string dataFile = argv[1];
  uint64_t num_edges, num_vertices;

  auto Persons = PersonVertexType::Create(MEDIUM);
  auto ForumEvents = ForumEventVertexType::Create(MEDIUM);
  auto Forums = ForumVertexType::Create(SMALL);
  auto Publications = PublicationVertexType::Create(SMALL);
  auto Topics = TopicVertexType::Create(SMALL);

  auto Purchases = PurchaseEdgeType::Create(MEDIUM);
  auto Sales = SaleEdgeType::Create(MEDIUM);
  auto Authors = AuthorEdgeType::Create(LARGE);
  auto Includes = IncludesEdgeType::Create(LARGE);
  auto HasTopic = HasTopicEdgeType::Create(LARGE);
  auto HasOrg = HasOrgEdgeType::Create(MEDIUM);
  auto GlobalIDS = GlobalIDType::Create(LARGE);

  graph["Persons"] = (uint64_t)(Persons->GetGlobalID());
  graph["ForumEvents"] = (uint64_t)(ForumEvents->GetGlobalID());
  graph["Forums"] = (uint64_t)(Forums->GetGlobalID());
  graph["Publications"] = (uint64_t)(Publications->GetGlobalID());
  graph["Topics"] = (uint64_t)(Topics->GetGlobalID());

  graph["Purchases"] = (uint64_t)(Purchases->GetGlobalID());
  graph["Sales"] = (uint64_t)(Sales->GetGlobalID());
  graph["Authors"] = (uint64_t)(Authors->GetGlobalID());
  graph["Includes"] = (uint64_t)(Includes->GetGlobalID());
  graph["HasTopic"] = (uint64_t)(HasTopic->GetGlobalID());
  graph["HasOrg"] = (uint64_t)(HasOrg->GetGlobalID());
  graph["GlobalIDS"] = (uint64_t)(GlobalIDS->GetGlobalID());

  // Partial match subpattern scoreboards
  // Level1
  // SubPattern1  : A FORUMEVENT with Prospect Park, Outdoors
  // SubPattern2  : A FORUMEVENT with Bomb, Explosion, Williamsburg
  // SubPattern3  : A FORUM that has NYC topic and 2 FEs with Jihad topic
  // SubPattern4  : A FORUMEVENT that has Jihad as topic
  // SubPattern5  : A publication with Electrical Engineering as topic and organization close to NYC 
  // SubPattern6  : A person who purchased bath bomb, pressure cooker, ammo from distributor 
  // SubPattern7  : A person who is an ammo distributer (multiple buyers)

  // Level2
  // SubPattern12 : A FORUM that has forumevents that satisfy both SP1 and SP2

  auto SubPattern1 = shad::Set<uint64_t>::Create(TINY);
  auto SubPattern2 = shad::Hashmap<uint64_t, time_t>::Create(TINY);
  auto SubPattern12 = shad::Hashmap<uint64_t, time_t>::Create(TINY);
  auto SubPattern3 = shad::Hashmap<uint64_t, uint64_t>::Create(TINY);
  auto SubPattern4 = shad::Set<uint64_t>::Create(TINY);
  auto SubPattern5 = shad::Set<uint64_t>::Create(TINY);
  auto SubPattern6 = shad::Hashmap<uint64_t, time_t>::Create(TINY);
  auto SubPattern7 = shad::Hashmap<uint64_t, int64_t>::Create(TINY);

  graph["SubPattern1"] = (uint64_t)(SubPattern1->GetGlobalID());
  graph["SubPattern2"] = (uint64_t)(SubPattern2->GetGlobalID());
  graph["SubPattern12"] = (uint64_t)(SubPattern12->GetGlobalID());
  graph["SubPattern3"] = (uint64_t)(SubPattern3->GetGlobalID());
  graph["SubPattern4"] = (uint64_t)(SubPattern4->GetGlobalID());
  graph["SubPattern5"] = (uint64_t)(SubPattern5->GetGlobalID());
  graph["SubPattern6"] = (uint64_t)(SubPattern6->GetGlobalID());
  graph["SubPattern7"] = (uint64_t)(SubPattern7->GetGlobalID());

  shad::rt::Handle streamReadHandle;
  shad::rt::Handle partialHandle;
  shad::rt::asyncExecuteAt(streamReadHandle, shad::rt::thisLocality(),
                           StreamDataFile, dataFile);
  // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  // this loop listens to streaming thread until it sees a streming stopped
  // flag
  while (true) {
    std::unique_lock<std::mutex> ul(queueMutex);
    if (dataQueue.empty()) {
      dataCV.wait(ul, []() { return !dataQueue.empty(); });
    }
    ul.unlock();
    std::string dataLine = dataQueue.front();
    dataQueue.pop();

    // stream stopped signal is "#"
    if (dataLine[0] == '#') {
      printf("Stream stopped\n");
      break;
    }

    TYPES t = insertToGraph(dataLine, graph);
    //printf("after insertion\n");
    Pattern_args_t args;
    args.graph = &graph;
    args.dataLine = dataLine;
    args.t = t;
    shad::rt::asyncExecuteAt(partialHandle, shad::rt::thisLocality(), CheckPartialMatch, args);
    //printf("checking for subpatterns \n");
  }

  printf("Got out of the loop\n");

  waitForCompletion(streamReadHandle);
  waitForCompletion(patternHandle);
  waitForCompletion(partialHandle);

  printf("handle waits returned\n");
  // one last check after finishing all handles
  // PatternCheck(graph);
  // waitForCompletion(patternHandle);

  printf("Time for graph construction = %lf\n", my_timer() - time1);

  printf("\n");
  printf("Number of persons      = %lu\n", Persons->Size());
  printf("Number of forum_events = %lu\n", ForumEvents->Size());
  printf("Number of forums       = %lu\n", Forums->Size());
  printf("Number of publications = %lu\n", Publications->Size());
  printf("Number of topics       = %lu\n", Topics->Size());

  printf("\n");
  printf("Number of purchase edges = %lu\n", Purchases->Size());
  printf("Number of sale edges     = %lu\n", Sales->Size());
  printf("Number of author edges   = %lu\n", Authors->Size());
  printf("Number of include edges  = %lu\n", Includes->Size());
  printf("Number of hasTopic edges = %lu\n", HasTopic->Size());
  printf("Number of hasOrg edges   = %lu\n", HasOrg->Size());

  printf("\n");
  printf("Number of SP1 matches = %lu\n", SubPattern1->Size());
  printf("Number of SP2 matches = %lu\n", SubPattern2->Size());
  printf("Number of SP3 matches = %lu\n", SubPattern3->Size());
  printf("Number of SP4 matches = %lu\n", SubPattern4->Size());
  printf("Number of SP5 matches = %lu\n", SubPattern5->Size());
  printf("Number of SP12 matches = %lu\n", SubPattern12->Size());
  printf("Number of SP6 matches = %lu\n", SubPattern6->Size());
  printf("Number of SP7 matches = %lu\n", SubPattern7->Size());
  printf("\n");
  printf("Total number of edges    = %lu\n", num_edges);
  printf("Total number of vertices = %lu\n\n", num_vertices);

  time1 = my_timer();

  //   WMD_pattern(graph);
  //   printf("Time for exact pattern matching = %lf\n", my_timer() - time1);
  return 0;
}

} // namespace shad
