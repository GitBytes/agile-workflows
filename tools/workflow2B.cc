#include "agile/workflow2/main.h"
#include "agile/workflow2/graph.h"

namespace shad {
  using namespace agile::workflow2;
  std::queue<std::string> dataQueue;
  std::mutex queueMutex;
  std::condition_variable dataCV;
  shad::rt::Handle patternHandle;

  std::vector <std::string> split(std::string & line, char delim, uint64_t size = 0) {
    uint64_t ndx = 0, start = 0;
    std::vector <std::string> tokens(size);

    for (uint64_t end = 0; end < line.length(); end ++) {

      if ( (line[end] == delim) || (line[end] == '\n') ) {
        tokens[ndx] = line.substr(start, end - start);
        start = end + 1;
        ndx ++;
    } }

    return tokens;
  }

  void StreamDataFile(shad::rt::Handle & , const std::string& dataFile){
    double time_start = my_timer(); //temp to finish experiment
    double max_time = 300;
    int waitMillisec=50;
    std::ifstream file(dataFile);
    if (file.is_open()) {
      printf("reading file %s\n", dataFile.c_str());
    } else {
      printf("Cannot open file %s\n", dataFile.c_str());
      exit(-1);
    }
    std::string line;
    while (getline(file, line) ) {
      if (line[0] == '#') continue; 
      std::this_thread::sleep_for(std::chrono::milliseconds(waitMillisec)); 
      std::unique_lock<std::mutex> ul(queueMutex);
      dataQueue.push(line);
      ul.unlock();
      //printf("added another line\n");
      dataCV.notify_one();
      if(my_timer()-time_start > max_time){
        std::unique_lock<std::mutex> ul(queueMutex);
        dataQueue.push("###END OF TIME###");
        printf("Stream timed out\n");
        ul.unlock();
        break;
      }
    }
    std::unique_lock<std::mutex> ul(queueMutex);
    dataQueue.push("###END OF FILE###");
    printf("Stream file ended");
    ul.unlock();
    printf("stream stopped\n");
  }

  bool SubPattern1Check(std::vector <std::string>& tokens, Graph_t& graph){
    bool ret = false;

    uint64_t FE = ENCODE<uint64_t, std::string, UINT>  (tokens[4]);
    //TODO: check FE is valid, in case of bad data?
    auto HasTopic = HasTopicEdgeType::GetPtr((HasTopicEdgeType::ObjectID) graph["HasTopic"]);
    auto SubPattern1 = shad::Set<uint64_t>::GetPtr((shad::ObjectIdentifier<shad::Set<uint64_t>>) graph["SubPattern1"]);
    HasTopicEdgeType::LookupResult FE_topics;
    HasTopic->Lookup(FE, &FE_topics);    

    //check if forumevent has 2 events outdoors and prospectpark
    bool topic_1 = false;                     
    bool topic_2 = false; 
    for (auto &FET : FE_topics.value) { 
      if (FET.topic == 69871376) {
        topic_1 = true;     // ... topic is Outdoors
      }
      else if (FET.topic == 1049632) {
        topic_2 = true;     // ... topic is Prospect Park
      } 
      if(topic_1 && topic_2){
        SubPattern1->AsyncInsert(patternHandle, FE);
        ret=true;
        break;
      }
    }
    return ret;
  }

  bool SubPattern2Check(std::vector <std::string>& tokens, Graph_t& graph){

    bool ret = false;
    uint64_t FE = ENCODE<uint64_t, std::string, UINT>  (tokens[4]);
    //TODO: check FE is valid, in case of bad data?   
    auto HasTopic = HasTopicEdgeType::GetPtr((HasTopicEdgeType::ObjectID) graph["HasTopic"]);
    auto SubPattern2 = shad::Set<uint64_t>::GetPtr((shad::ObjectIdentifier<shad::Set<uint64_t>>) graph["SubPattern2"]);


    HasTopicEdgeType::LookupResult FE_topics;
    HasTopic->Lookup(FE, &FE_topics);    
    //check if forumevent has 3 events Bomb, Explosion, Williamsburg
    bool topic_1 = false;                     
    bool topic_2 = false;
    bool topic_3 = false; 
    for (auto &FET : FE_topics.value) { 
      if (FET.topic == 771572) {
        topic_1 = true;      // ... topic is Williamsburg
      }
      else if (FET.topic == 179057) {
        topic_2 = true;      // ... topic is Explosion
      }
      else if (FET.topic == 127197) {
        topic_3 = true;  
      }
      if(topic_1 && topic_2 && topic_3){
        SubPattern2->AsyncInsert(patternHandle, FE);
        ret=true;
        break;
      }
    }
    return ret;
  }

  bool SubPattern12Check(std::vector <std::string>& tokens, Graph_t& graph, bool SP1=false, bool SP2=false){
    bool ret = false;
    uint64_t FE = ENCODE<uint64_t, std::string, UINT>  (tokens[4]);
    auto ForumEvents = ForumEventVertexType::GetPtr((ForumEventVertexType::ObjectID) graph["ForumEvents"]);
    auto Includes  = IncludesEdgeType::GetPtr((IncludesEdgeOID) graph["Includes"]);
    auto SubPattern1 = shad::Set<uint64_t>::GetPtr((shad::ObjectIdentifier<shad::Set<uint64_t>>) graph["SubPattern1"]);
    auto SubPattern2 = shad::Set<uint64_t>::GetPtr((shad::ObjectIdentifier<shad::Set<uint64_t>>) graph["SubPattern2"]);
    auto SubPattern12 = shad::Set<uint64_t>::GetPtr((shad::ObjectIdentifier<shad::Set<uint64_t>>) graph["SubPattern12"]);

    ForumEventVertex FEV;                   
    ForumEvents->Lookup(FE, & FEV);
    uint64_t ForumID = FEV.forum;
    bool addToBoard=true;
    if(!SP1){
      bool found = false;
      IncludesEdgeType::LookupResult FEs;
      Includes->Lookup(ForumID, &FEs);

      for (auto &FE_id : FEs.value) {
        if(SubPattern2->Find(FE_id.forum_event))
        {
          found = true;
          break;
        }
      }
      if(!found){
        addToBoard=false;
      }
    }
    if(!SP2){
      bool found = false;
      IncludesEdgeType::LookupResult FEs;
      Includes->Lookup(ForumID, &FEs);

      for (auto & FE_id : FEs.value) {
        if(SubPattern1->Find(FE_id.forum_event))
        {
          found = true;
          break;
        }
      }
      if(!found){
        addToBoard=false;
      }
    }
    if(addToBoard){
      SubPattern12->AsyncInsert(patternHandle,ForumID);
      //next level checks can be triggered here
    }
    return addToBoard;
  }


int main(int argc, char *argv[]) {
  double time1 = my_timer();

  Graph_t graph;
  std::string dataFile = argv[1];
  uint64_t num_edges, num_vertices;

  auto Persons      = PersonVertexType::Create(MEDIUM);
  auto ForumEvents  = ForumEventVertexType::Create(MEDIUM);
  auto Forums       = ForumVertexType::Create(SMALL);
  auto Publications = PublicationVertexType::Create(SMALL);
  auto Topics       = TopicVertexType::Create(SMALL);

  auto Purchases    = PurchaseEdgeType::Create(MEDIUM);
  auto Sales        = SaleEdgeType::Create(MEDIUM);
  auto Authors      = AuthorEdgeType::Create(LARGE);
  auto Includes     = IncludesEdgeType::Create(LARGE);
  auto HasTopic     = HasTopicEdgeType::Create(LARGE);
  auto HasOrg       = HasOrgEdgeType::Create(MEDIUM);
  auto GlobalIDS    = GlobalIDType::Create(LARGE);

  graph["Persons"]      = (uint64_t) (Persons->GetGlobalID());
  graph["ForumEvents"]  = (uint64_t) (ForumEvents->GetGlobalID());
  graph["Forums"]       = (uint64_t) (Forums->GetGlobalID());
  graph["Publications"] = (uint64_t) (Publications->GetGlobalID());
  graph["Topics"]       = (uint64_t) (Topics->GetGlobalID());

  graph["Purchases"]    = (uint64_t) (Purchases->GetGlobalID());
  graph["Sales"]        = (uint64_t) (Sales->GetGlobalID());
  graph["Authors"]      = (uint64_t) (Authors->GetGlobalID());
  graph["Includes"]     = (uint64_t) (Includes->GetGlobalID());
  graph["HasTopic"]     = (uint64_t) (HasTopic->GetGlobalID());
  graph["HasOrg"]       = (uint64_t) (HasOrg->GetGlobalID());
  graph["GlobalIDS"]    = (uint64_t) (GlobalIDS->GetGlobalID());

  //Partial match subpattern scoreboards
  //Level1
  //SubPattern1  : A FORUMEVENT with Prospect Park, Outdoors
  //SubPattern2  : A FORUMEVENT with Bomb, Explosion, Williamsburg
  //SubPattern3  : A FORUM that has NYC topic
  //SubPattern4  : A FORUMEVENT that has Jihad as topic

  //Level2
  //SubPattern12 : A FORUM that has forumevents that satisfy both SP1 and SP2 

  //Level3
  //SubPattern123: A FORUM that satisfies subpatterns 1, 2, and 3

  auto SubPattern1 = shad::Set<uint64_t>::Create(TINY);
  auto SubPattern2 = shad::Set<uint64_t>::Create(TINY);
  auto SubPattern12 = shad::Set<uint64_t>::Create(TINY);
  auto SubPattern3 = shad::Set<uint64_t>::Create(TINY);
  auto SubPattern123 = shad::Set<uint64_t>::Create(TINY);
  auto SubPattern4 = shad::Set<uint64_t>::Create(TINY);

  graph["SubPattern1"]  = (uint64_t) (SubPattern1->GetGlobalID());
  graph["SubPattern2"]  = (uint64_t) (SubPattern2->GetGlobalID());
  graph["SubPattern12"]  = (uint64_t) (SubPattern12->GetGlobalID());
  graph["SubPattern3"]  = (uint64_t) (SubPattern3->GetGlobalID());
  graph["SubPattern123"]  = (uint64_t) (SubPattern123->GetGlobalID());
  graph["SubPattern4"] = (uint64_t) (SubPattern4->GetGlobalID());



  shad::rt::Handle streamReadHandle;
  //shad::rt::Handle patternHandle;
  shad::rt::asyncExecuteAt(streamReadHandle, shad::rt::thisLocality(), StreamDataFile, dataFile);
std::this_thread::sleep_for(std::chrono::milliseconds(1000)); 
  //this loop listens to streaming thread until it sees a streming stopped flag
  while (true){
    std::unique_lock<std::mutex> ul(queueMutex);
    if (dataQueue.empty())
		{
			dataCV.wait(ul, []() {return !dataQueue.empty(); }); 
		}
    ul.unlock();
    std::string dataLine = dataQueue.front();
    dataQueue.pop();

    //stream stopped signal is "#"
    if(dataLine[0] == '#'){
      printf("Stream stopped\n");
      break;
    }

    TYPES t = insertToGraph(dataLine, graph);
    //printf("added to the graph\n");
    // if (t == TYPES::PERSON ){
    //   printf("\t It's a person!\n");
    // }

    if (t == TYPES::HASTOPIC) {
      //3 types of hastopic, for publication, for forum, for forumevent 
      //we can differentiate by looking at which field is filled (tokens[3]-> forumID , tokens[4]->forumeventID, tokens[5]->publicationID)
      std::vector <std::string> tokens = split(dataLine, ',', 10);

      //if this is a FORUMEVENT with TOPIC info 
      if(tokens[4] != "") {

        //trigger to set if we need to do an upper level partial match check 
        bool SP1 = SubPattern1Check(tokens, graph);
        bool SP2 = SubPattern2Check(tokens, graph);

        // check for forums that has a FE in SP1 and SP2 
        if(SP1 || SP2){ //if we added this forumevent to either SubPattern board, we do an upper level check
          bool SP12 = SubPattern12Check(tokens, graph, SP1, SP2);
          //do we trigger upper level checks here? or call it from sp12check?
        }

        //check if topic is Jihad
        if (tokens[6] == "44311"){
          uint64_t FE = ENCODE<uint64_t, std::string, UINT>  (tokens[4]);
          SubPattern4->AsyncInsert(patternHandle, FE);
          //NEXT: check for person affiliated with 2 jihad forumevents  
        }
      }


      //This is a Forum with Topic Info
      if(tokens[3] != "") {
        uint64_t ForumID = ENCODE<uint64_t, std::string, UINT>  (tokens[3]);
        //if topic info is NYC
        if(tokens[6] == "60"){
          SubPattern3->AsyncInsert(patternHandle, ForumID);
          //CHECK if FORUM is in SubPattern12 
          if(SubPattern12->Find(ForumID)){
            SubPattern123->AsyncInsert(patternHandle, ForumID);
            //NEXT:upper level check
          }
        } 
      }
    }


    //if person selling ammo, 
      //check person has 2 ammo buyers 
        //add person to SP4 hashmap

    //if Publication has topic elec eng
      //add person to SP5
    
    //if publication has org close to NYC
      //add publication to SP6 (or mark it multiple orgs)



  ///level 2 check
    //if person -- forum event data comes
      //check FE is in SP7
      //check if person has another FE in SP7
        //add person to SP77



    //temp: stop after some time
    // if(my_timer() - time1 > 200){
    //   break;
    // }
  }
  printf("Got out of the loop\n");

  waitForCompletion(streamReadHandle);
  waitForCompletion(patternHandle);



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
  printf("Number of SP12 matches = %lu\n", SubPattern12->Size());
  printf("Number of SP123 matches = %lu\n", SubPattern123->Size());
  printf("\n");
  printf("Total number of edges    = %lu\n", num_edges);
  printf("Total number of vertices = %lu\n\n", num_vertices);

  time1 = my_timer();

//   WMD_pattern(graph);
//   printf("Time for exact pattern matching = %lf\n", my_timer() - time1);
  return 0;
}

}
