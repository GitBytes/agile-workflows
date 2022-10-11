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

#include "agile/wk2_partial/graph.h"
#include "agile/wk2_partial/main.h"

namespace shad
{
  using namespace agile::wk2_partial;
  
  shad::rt::Handle patternHandle;

  std::vector<std::string> split(std::string &line, char delim, uint64_t size = 0)
  {
    uint64_t ndx = 0, start = 0, end = 0;
    std::vector<std::string> tokens(size);

    for (; end < line.length(); end++)
    {
      if ((line[end] == delim) || (line[end] == '\n'))
      {
        tokens[ndx] = line.substr(start, end - start);
        start = end + 1;
        ndx++;
      }
    }

    tokens[size - 1] = line.substr(start, end - start); // flush last token
    return tokens;
  }

  bool proximity(TopicVertex &A, TopicVertex &B){
    double lon_miles = 0.91 * std::abs(A.lon - B.lon);
    double lat_miles = 1.15 * std::abs(A.lat - B.lat);
    double distance = std::sqrt(lon_miles * lon_miles + lat_miles * lat_miles);
    return distance <= 30.0;
  }

  bool check_proximity(uint64_t A, uint64_t B, Graph_t &graph){
    auto Topics = TopicVertexType::GetPtr((TopicVertexOID)graph["Topics"]);
    TopicVertex A_tpc, B_tpc; 
    Topics->Lookup(A, &A_tpc);
    Topics->Lookup(B, &B_tpc);

    double lon_miles = 0.91 * std::abs(A_tpc.lon - B_tpc.lon);
    double lat_miles = 1.15 * std::abs(A_tpc.lat - B_tpc.lat);
    double distance = std::sqrt(lon_miles * lon_miles + lat_miles * lat_miles);
    return distance <= 30.0;
  }



  //////
    template <typename T>
  struct CustomInsert { //insert if no record, do nothing if there is already a record 
    bool operator()(T *const lhs, const T &rhs, bool) {
      if(lhs){
        std::cout<<"inserter 1 :"<<std::endl;
        return false;
      }
      else{
        *lhs = std::move(rhs);
        std::cout<<"inserter 2 :"<<std::endl;
        return true;
      }
    }
    static bool Insert(T *const lhs, const T &rhs, bool) {
      if(lhs){
        std::cout<<"inserter 3 :"<<std::endl;
        return false;
      }
      else{
        *lhs = std::move(rhs);
        std::cout<<"inserter 4 :"<<std::endl;
        return true;
      }
    }
  };

      template <typename T>
  struct CustomInsert2 { //insert if no record, do nothing if there is already a record 
    bool operator()(T *const lhs, const T &rhs, bool same_key) {
      if(same_key){
        //key is already present 
        return false;
      }
      else{
        //new key
        *lhs = std::move(rhs);
        return true;
      }
    }
    static bool Insert(T *const lhs, const T &rhs, bool same_key) {
      if(same_key){
        return false;
      }
      else{
        *lhs = std::move(rhs);
        return true;
      }
    }
  };
  ////////

  void SP3Check( const uint64_t &Fid, std::pair<bool, uint64_t>& value, std::vector<AuthorEdge>& evs, const uint64_t &person,RF_args_t & args)
  {
    //for each forum F in sp3
          //if forum is valid
          //get all jihad FEs of F from SP4 
          //check if 2 FEs also in authorevent list  

      std::cout<<"in sp3check"<<std::endl;
      if(value.first && value.second >= 2){
        auto SubPattern4 = shad::Multimap<uint64_t, uint64_t>::GetPtr((shad::ObjectIdentifier<shad::Multimap<uint64_t, uint64_t>>) args.SubPattern4_OID);

        shad::Multimap<uint64_t, uint64_t>::LookupResult FElist;
        SubPattern4->Lookup(Fid, &FElist);

        int matches=0;
        for (auto & FE : FElist.value){

          for (auto & element : evs) {
            if (element.item == FE){
              matches++;
              if(matches >= 2){
                std::cout<<"PATTERN FOUND FOR PERSON "<<person<<std::endl;
                return;
              }
              break;
            }
          }
        }
      }
    }

  // We are looking for a person with
  // SP3 - 2 jihad forumeventss at a nyc forum
  // SP12 - attended a forumevent which is in a forum from SP12
  // SP5 - 1 purchase from a person with publication S5
  // SP6- 3 purchases (bath bomb, pressure cooker, ammo from distributor
  void PatternCheckForPerson(shad::rt::Handle & handle, const uint64_t &person, std::pair<uint64_t, time_t>& value, RF_args_t & args)
  {
      time_t SP12_date = NULL;  
      //check if person fulfills SP6
      if(value.first == 15){ // person purchased bath bomb, pressure cooker, ammunition from distributor, electronics 
        
        //check if person attended an event from a SP12 forum 
        using SP12type = shad::Hashmap<uint64_t, std::pair<uint64_t, time_t>, shad::MemCmp<uint64_t>,CustomInsert2<std::pair<uint64_t, time_t>>>; 
        auto SubPattern12 = SP12type::GetPtr((shad::ObjectIdentifier<SP12type>)args.SubPattern12_OID);
        auto Authors  = AuthorEdgeType::GetPtr((AuthorEdgeOID) args.Authors_OID);
        
        //make sure there are entries in SP12 
        if(!SubPattern12->Size())
          return; //person cannot fulfill the pattern if SP12 is empty
        
        AuthorEdgeType::LookupResult events;             // get person's events
        Authors->Lookup(person, & events);
        
        for (auto & EV : events.value) {   
          if (EV.dst_type != TYPES::FORUMEVENT) continue;
          
          //get the forum of FE 
          auto ForumEvents = ForumEventVertexType::GetPtr((ForumEventVertexType::ObjectID)args.ForumEvents_OID);
          ForumEventVertex FEV;
          ForumEvents->Lookup(EV.item, &FEV);
          std::pair<uint64_t, time_t> sp12_entry(NULL,NULL);
          SubPattern12->Lookup(FEV.forum, &sp12_entry);
          if( sp12_entry.first == NULL || sp12_entry.first < 3 )
          {//no match in SP12 table 
            continue;
          }
          else {
            //This person attended a FE in a Forum that is in SP12, we want to record the earliest attendance to such forum event
            if(SP12_date){
              std::min(SP12_date, sp12_entry.second);
            }
            else{
              SP12_date = sp12_entry.second;
            }
          }
        }

        if(!SP12_date){
          std::cout<<"no SP12 match for person "<<person<<std::endl;
          return; //we couldn't find a SP12 match for the person 
        }
        else{
          //here we know this person satisfies SP6 and SP12, next we check if date requirement btw SP7 and SP12 is satisfied 
          if( SP12_date >= value.second){
            std::cout<<"no SP6-SP12 date match for person "<<person<<std::endl;
            return; //date requirement didn't work 
          }
        }

        std::cout<<"this person satisfies sp12 and sp7 :"<<person<<std::endl;
        
        //next we check SP5 matches 
        //this person must purchase an electronic from a person who published in SP5 
        bool SP5match = false; 
        using SP5type = shad::Hashmap<uint64_t, std::pair<bool, bool>, shad::MemCmp<uint64_t>, CustomInsert2<std::pair<bool, bool>>>; 
        auto SubPattern5 = SP5type::GetPtr((shad::ObjectIdentifier<SP5type>)args.SubPattern5_OID);

        if(!SubPattern5->Size()){
          std::cout<<"no SP5 match for person "<<person<<std::endl;
          return; //no entries in SP5 table, we can't have a pattern
        }

        //get all people who This person purchased an electronic from
        auto Purchases = PurchaseEdgeType::GetPtr((PurchaseEdgeOID) args.Purchases_OID);
        PurchaseEdgeType::LookupResult purchases;         // get person's purchases
        Purchases->Lookup(person, & purchases);

        for (auto & PO : purchases.value) { 
          if(SP5match)
            break;
          if(PO.product == 11650 ){ 
            auto electronic_seller = PO.seller;
            //get publications of this seller 
            AuthorEdgeType::LookupResult pubs;             // get person's events
            Authors->Lookup(electronic_seller, &pubs);

            for (auto & EV : pubs.value) {   
              if (EV.dst_type != TYPES::PUBLICATION) continue;

              std::pair<bool, bool> sp5value(false,false);
              SubPattern5->Lookup(EV.item, &sp5value);

              if(sp5value.first && sp5value.second){
                //we found a match in SP5 
                SP5match = true;
                break;
              }
            }
          }
        }

        if(!SP5match){
          std::cout<<"no SP5 matches for person "<<person<<std::endl;
          return; //no match for SP5 subpattern
        }

        std::cout<<"This person satisfies sp12 and sp7 and sp5 :"<<person<<std::endl;
        
        //Next we check for SP3 
        //we ned to see if this person attended 2 jihad events in a SP3 forum 

        using SP3type = shad::Hashmap<uint64_t, std::pair<bool, uint64_t>,shad::MemCmp<uint64_t>, CustomInsert2<std::pair<bool, uint64_t>>>;
        auto SubPattern3 = SP3type::GetPtr((shad::ObjectIdentifier<SP3type>)args.SubPattern3_OID);

        //SubPattern3->ForEachEntry(SP3Check, evs, person, args);

        //all the events person attended
        std::vector<AuthorEdge> evs = events.value;
        auto ForumEvents = ForumEventVertexType::GetPtr((ForumEventVertexType::ObjectID)args.ForumEvents_OID);
      
        for(auto eventEdge : evs){
          if( eventEdge.dst_type != TYPES::FORUMEVENT) continue;

          //each forumevent person attended
          //get forum to see if forum satisfies in SP3 and FE is Jihad 
          ForumEventVertex FEV;
          ForumEvents->Lookup(eventEdge.item, &FEV);

          //if(FEV.topic != 44311) continue; //not jihad

          std::pair<bool, uint64_t> SP3Obj;
          SubPattern3->Lookup(FEV.forum, &SP3Obj); 

          if(SP3Obj.first && SP3Obj.second >=2){
            std::cout<<"PERSON ("<<person<<")SATISFIES THE PATTERN! "<<std::endl;
          }
        }


            //for each forum F in sp3
          //if forum is valid
          //get all jihad FEs of F from SP4 
          //check if 2 FEs also in authorevent list  
          /**/
          //each forumevent this person attended 
          //if it's jihad, 
          //check if forum of FE in SP3, 
          //if so check if any other FE in SP4 in events 




      }
      else{
        //person doesn't fulfill the pattern
        std::cout<<"no SP6 match for person "<<person<<std::endl;
      }
  }

  // check if a pattern is formed for each person vertex in the graph
  void PatternCheck(int incoming, RF_args_t & args)
  {
    //check each person who satisfy SubPattern6 and see if they also satisfy other SPs 
    using SP6type = shad::Hashmap<uint64_t, std::pair<uint64_t, time_t>, shad::MemCmp<uint64_t>, CustomInsert2<std::pair<uint64_t, time_t>>>;
    auto SubPattern6 = SP6type::GetPtr((shad::ObjectIdentifier<SP6type>)args.SubPattern6_OID);
    std::cout<<"PatternCheck called from Update_SP"<<incoming<<std::endl;
    if(SubPattern6->Size()){
      shad::rt::Handle foreachHandle;
      std::cout<<SubPattern6->Size()<<std::endl;
      SubPattern6->AsyncForEachEntry(foreachHandle, PatternCheckForPerson, args);
      waitForCompletion(foreachHandle);
    }
  }

  void Update_SP12(const uint64_t &forum, std::pair<uint64_t, time_t> &value, uint64_t &SP, time_t &date, RF_args_t & args)
  {
    value.first |= SP;
    value.second = std::min(value.second, date);
    if (value.first == 3)
      PatternCheck(12, args);
  }

  void Update_SP1(Handle &handle, const uint64_t &FE, uint64_t &value, uint64_t &topic, RF_args_t & args)
  {
    if (topic == 1049632)
      value |= 2; // Prospect Park
    else if (topic == 69871376)
      value |= 1; // Outdoors

    if (value == 3)
    { 
      using SP12type = shad::Hashmap<uint64_t, std::pair<uint64_t, time_t>, shad::MemCmp<uint64_t>,CustomInsert2<std::pair<uint64_t, time_t>>>; 
      auto SubPattern12 = SP12type::GetPtr((shad::ObjectIdentifier<SP12type>)args.SubPattern12_OID);
      auto ForumEvents = ForumEventVertexType::GetPtr((ForumEventVertexType::ObjectID)args.ForumEvents_OID);
      ForumEventVertex FEV;
      ForumEvents->Lookup(FE, &FEV);
      uint64_t tmp = 1;
      std::pair<uint64_t, time_t> sp12tmp(0, 0);
      SubPattern12->Insert(FEV.forum, sp12tmp);
      SubPattern12->Apply(FEV.forum, Update_SP12, tmp, FEV.date, args);
    }
  }

  void Update_SP2(Handle &handle, const uint64_t &FE, uint64_t &value, uint64_t &topic, RF_args_t & args)
  { 
    if (topic == 127197)
      value |= 4; // Bomb
    else if (topic == 179057)
      value |= 2; // Explosion
    else if (topic == 771572)
      value |= 1; // Williamsburg  

    if (value == 7)
    {
        using SP12type = shad::Hashmap<uint64_t, std::pair<uint64_t, time_t>, shad::MemCmp<uint64_t>,CustomInsert2<std::pair<uint64_t, time_t>>>;
        auto SubPattern12 = SP12type::GetPtr( (shad::ObjectIdentifier<SP12type>) args.SubPattern12_OID);
        auto ForumEvents  = ForumEventVertexType::GetPtr( (ForumEventVertexOID) args.ForumEvents_OID);

        ForumEventVertex FEV;
        ForumEvents->Lookup(FE, &FEV);
        uint64_t tmp = 2;
        std::pair<uint64_t, time_t> sp12tmp(0, shad::data_types::kNullValue<time_t>);
        SubPattern12->Insert(FEV.forum, sp12tmp);
        SubPattern12->Apply(FEV.forum, Update_SP12, tmp, FEV.date, args);
    }
  }

  void Update_SP3(Handle &handle, const uint64_t &Fid, std::pair<bool, uint64_t> &value, uint64_t &topic, RF_args_t & args)
  {
    if(topic == 60){ //NYC
      value.first = true;
    }
    else if( topic == 44311){ //Jihad
      value.second ++;
    }

    if(value.first && value.second >= 2){
      PatternCheck(3, args);
    }
  }

  void Update_SP5(Handle &handle, const uint64_t &Fid, std::pair<bool, bool> &value, uint64_t &topic, RF_args_t & args)
  {
    if(topic == 60){
      value.first = true;
    }
    if(topic == 43035){
      value.second = true;
    }
    if(value.first && value.second){
      PatternCheck(5, args);
    }
  }

  void Update_SP6(Handle &handle, const uint64_t & buyer, std::pair<uint64_t, time_t> & value, uint64_t & product, time_t & date, RF_args_t & args) 
  {
    if (product == 2869238) 
      value.first |= 8;     // Bath Bomb
    else if (product == 271997)  
      value.first |= 4;     // Pressure Cooker
    else if (product == 11650)   
      value.first |= 2;     // Electronics
    else if (product == 185785)  
      value.first |= 1;     // Ammunition

    value.second = std::max(value.second, date);

    if (value.first == 15)
      PatternCheck(6, args);
  }

  void Update_SP7(Handle &handle, const uint64_t & seller, std::pair<int64_t, time_t> & value, uint64_t & buyer, time_t & date, RF_args_t & args) {
    if (value.first == shad::data_types::kNullValue<uint64_t>) {                              // first buyer
       value.first = buyer;
       value.second = date;
    }
    else if( value.first >= 0){
      if (value.first == buyer) {  //same buyer, we update the date if necessary
        value.second = std::max(value.second, date);
      }
      else {  //we have 2 separate buyers , seller is now a distributor
        using SP6type = shad::Hashmap<uint64_t, std::pair<uint64_t, time_t>, shad::MemCmp<uint64_t>, CustomInsert2<std::pair<uint64_t, time_t>>>;
        auto SubPattern6 = SP6type::GetPtr((shad::ObjectIdentifier<SP6type>)args.SubPattern6_OID);
        uint64_t ammunition = 185785;
        std::pair<uint64_t, time_t> tmp(0,shad::data_types::kNullValue<time_t>);
        SubPattern6->Insert(value.first, tmp);
        SubPattern6->Insert(buyer, tmp);
        SubPattern6->AsyncApply(handle,value.first, Update_SP6, ammunition, value.second, args);
        SubPattern6->AsyncApply(handle, buyer, Update_SP6, ammunition, date, args);
        value.first = -1 * value.first;
      }
    }
    else {
      using SP6type = shad::Hashmap<uint64_t, std::pair<uint64_t, time_t>, shad::MemCmp<uint64_t>, CustomInsert2<std::pair<uint64_t, time_t>>>;
      auto SubPattern6 = SP6type::GetPtr((shad::ObjectIdentifier<SP6type>)args.SubPattern6_OID);
      uint64_t ammunition = 185785;
      SubPattern6->AsyncApply(handle, buyer, Update_SP6, ammunition, date, args);
    }
  }


  int main(int argc, char *argv[])
  {
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
    
    // Partial match subpattern scoreboards
    // Level1
    // SubPattern1  : A FORUMEVENT with Prospect Park, Outdoors
    // SubPattern2  : A FORUMEVENT with Bomb, Explosion, Williamsburg
    // SubPattern3  : A FORUM that has NYC topic and 2 FEs with Jihad topic
    // SubPattern4  : A FORUM that has Jihad Forumevents (key forum, value: list of FEs with jihad)
    // SubPattern5  : A publication with Electrical Engineering as topic and organization close to NYC
    // SubPattern6  : A person who purchased bath bomb, pressure cooker, ammo from distributor
    // SubPattern7  : A person who is an ammo distributer (multiple buyers)

    // Level2
    // SubPattern12 : A FORUM that has forumevents that satisfy both SP1 and SP2
    auto SubPattern1 = shad::Hashmap<uint64_t, uint64_t, shad::MemCmp<uint64_t>, CustomInsert2<uint64_t>>::Create(TINY);
    auto SubPattern2 = shad::Hashmap<uint64_t, uint64_t, shad::MemCmp<uint64_t>, CustomInsert2<uint64_t>>::Create(TINY);
    auto SubPattern12 = shad::Hashmap<uint64_t, std::pair<uint64_t, time_t>, shad::MemCmp<uint64_t>,CustomInsert2<std::pair<uint64_t, time_t>>>::Create(TINY);
    auto SubPattern3 = shad::Hashmap<uint64_t, std::pair<bool, uint64_t>,shad::MemCmp<uint64_t>, CustomInsert2<std::pair<bool, uint64_t>>>::Create(TINY);
    auto SubPattern4 = shad::Multimap<uint64_t, uint64_t>::Create(SMALL);
    auto SubPattern5 = shad::Hashmap<uint64_t, std::pair<bool, bool>, shad::MemCmp<uint64_t>, CustomInsert2<std::pair<bool, bool>>>::Create(SMALL);
    auto SubPattern6 = shad::Hashmap<uint64_t, std::pair<uint64_t, time_t>, shad::MemCmp<uint64_t>, CustomInsert2<std::pair<uint64_t, time_t>>>::Create(SMALL);
    auto SubPattern7 = shad::Hashmap<uint64_t, std::pair<int64_t, time_t>, shad::MemCmp<uint64_t>, CustomInsert2<std::pair<int64_t, time_t>>>::Create(SMALL);

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

    graph["SubPattern1"] = (uint64_t)(SubPattern1->GetGlobalID());
    graph["SubPattern2"] = (uint64_t)(SubPattern2->GetGlobalID());
    graph["SubPattern12"] = (uint64_t)(SubPattern12->GetGlobalID());
    graph["SubPattern3"] = (uint64_t)(SubPattern3->GetGlobalID());
    graph["SubPattern4"] = (uint64_t)(SubPattern4->GetGlobalID());
    graph["SubPattern5"] = (uint64_t)(SubPattern5->GetGlobalID());
    graph["SubPattern6"] = (uint64_t)(SubPattern6->GetGlobalID());
    graph["SubPattern7"] = (uint64_t)(SubPattern7->GetGlobalID());

    RF_args_t args;
    args.Persons_OID = graph["Persons"];
    args.ForumEvents_OID = graph["ForumEvents"];
    args.Forums_OID = graph["Forums"];
    args.Publications_OID = graph["Publications"];
    args.Topics_OID = graph["Topics"];
    args.Purchases_OID = graph["Purchases"];
    args.Sales_OID = graph["Sales"];
    args.Authors_OID = graph["Authors"];
    args.Includes_OID = graph["Includes"];
    args.HasTopic_OID = graph["HasTopic"];
    args.HasOrg_OID = graph["HasOrg"];
    args.SubPattern1_OID = graph["SubPattern1"];
    args.SubPattern2_OID = graph["SubPattern2"];
    args.SubPattern12_OID = graph["SubPattern12"];
    args.SubPattern3_OID = graph["SubPattern3"];
    args.SubPattern4_OID = graph["SubPattern4"];
    args.SubPattern5_OID = graph["SubPattern5"];
    args.SubPattern6_OID = graph["SubPattern6"];
    args.SubPattern7_OID = graph["SubPattern7"];
 
    shad::rt::Handle bufferhandle;
    shad::rt::Handle matchHandle;

    std::ifstream file(dataFile);
    if (!file.is_open())
    {
      printf("cannot open file %s\n", dataFile.c_str());
      exit(-1);
    }
    std::string dataLine;
    while (getline(file, dataLine))
    {
      if (dataLine[0] == '#')
        continue; // skip comments

      std::vector<std::string> tokens = split(dataLine, ',', 10);
      
      if (tokens[0] == "HasTopic")
      {
        HasTopicEdge record(tokens);
        // FORUM EVENT with Prospect Park and Outdoors topics
        if ((tokens[4] != "") && ((tokens[6] == "1049632") || (tokens[6] == "69871376")))
        { 
          HasTopic->AsyncInsert(matchHandle, record.key(), record);
          SubPattern1->Insert(record.key(), 0);
          SubPattern1->AsyncApply(matchHandle, record.key(), Update_SP1, record.topic, args);
        }
        //FORUMEVENT with Bomb, Explosion, and Williamsburg topics 
        else if ((tokens[4] != "") && ((tokens[6] == "127197") || (tokens[6] == "179057") || (tokens[6] == "771572")))
        { 
          HasTopic->AsyncInsert(matchHandle, record.key(), record);
          SubPattern2->Insert(record.key(), 0);
          SubPattern2->AsyncApply(matchHandle, record.key(), Update_SP2, record.topic, args);
        }
        //FORUM with NYC topic
        else if ((tokens[3] != "") && (tokens[6] == "60")) {     
          //std::cout<<"Forum in nyc "<<record.key()<<std::endl;                                           
          HasTopic->AsyncInsert(matchHandle, record.key(), record);
          std::pair<bool, uint64_t> tmp(0,0);
      
          SubPattern3->Insert(record.key(), tmp);
          SubPattern3->AsyncApply(matchHandle, record.key(), Update_SP3, record.topic, args);
          waitForCompletion(matchHandle);
        }
        //FORUMEVENT with Jihad topic
        else if ((tokens[4] != "") && (tokens[6] == "44311")) 
        {
          HasTopic->AsyncInsert(matchHandle, record.key(), record);
          //get forumevent's forum 
          ForumEventVertex FEV;
          ForumEvents->Lookup(record.key(), &FEV);
          //std::cout<<"Forumevent about jihad "<<record.key()<<" forum: "<<FEV.forum<<std::endl;
          std::pair<bool, uint64_t> tmp(0,0);
          SubPattern3->Insert(FEV.forum, tmp);
          SubPattern4->Insert(FEV.forum, record.key());
          SubPattern3->AsyncApply(matchHandle, FEV.forum, Update_SP3, record.topic, args);
          waitForCompletion(matchHandle);
        }
        else if( (tokens[5] != "") && (tokens[6] == "43035"))
        {
          HasTopic->AsyncInsert(matchHandle, record.key(), record);
          std::pair<bool, bool> tmp(0,0);
          SubPattern5->Insert(record.key(), tmp);
          SubPattern5->AsyncApply(matchHandle, record.key(), Update_SP5, record.topic, args);
          waitForCompletion(matchHandle);
        }
        else
        {
          HasTopic->BufferedAsyncInsert(bufferhandle, record.key(), record);
        }
      }
      else if(tokens[0] == "HasOrg"){
        HasOrgEdge record(tokens);
        //HasOrg,,,,,836683043222137391,1093910,,,
        if( tokens[5] != "" && (check_proximity(60, record.organization, graph)))
        {
          HasOrg->AsyncInsert(matchHandle, record.key(), record);
          std::pair<bool, bool> tmp(0,0);
          SubPattern5->Insert(record.key(), tmp);
          uint64_t topic = 60;
          SubPattern5->AsyncApply(matchHandle, record.key(),Update_SP5, topic, args);
          waitForCompletion(matchHandle);
        }
        else{
          HasOrg->BufferedAsyncInsert(bufferhandle, record.key(), record);
        }
      }
      else if(tokens[0] == "Sale"){
        SaleEdge sale(tokens);
        PurchaseEdge purchase(tokens);

        if ( (tokens[6] == "2869238") || (tokens[6] == "271997") || (tokens[6] == "11650") ) {
          Sales->AsyncInsert(matchHandle, sale.key(), sale);
          Purchases->AsyncInsert(matchHandle, purchase.key(), purchase);
          std::pair<uint64_t, time_t> tmp(0, shad::data_types::kNullValue<time_t>);
          SubPattern6->Insert(purchase.key(), tmp);
          SubPattern6->AsyncApply(matchHandle, purchase.key(), Update_SP6, purchase.product, purchase.date, args);      // THIS MAY NOT WORK
          waitForCompletion(matchHandle);
        }
        else if (tokens[6] == "185785") {
          Sales->AsyncInsert(matchHandle, sale.key(), sale);
          Purchases->AsyncInsert(matchHandle, purchase.key(), purchase);

          std::pair<uint64_t, time_t> tmp(shad::data_types::kNullValue<uint64_t>, shad::data_types::kNullValue<time_t>);
          SubPattern7->Insert(sale.key(), tmp);
          SubPattern7->AsyncApply(matchHandle, sale.key(), Update_SP7, purchase.buyer, purchase.date, args);            // THIS MAY NOT WORK
          waitForCompletion(matchHandle);
        }
        else {
          Sales->BufferedAsyncInsert(bufferhandle, sale.key(), sale);
          Purchases->BufferedAsyncInsert(bufferhandle, purchase.key(), purchase);
  
        }  
      }
      
      else
      {
        insertToGraphBuffered(bufferhandle, dataLine, graph);
      }
    }

    printf("Got out of the loop\n");


    PatternCheck(0,args);
    // one last check after finishing all handles
    // PatternCheck(graph);
    // waitForCompletion(patternHandle);
    waitForCompletion(patternHandle);
    waitForCompletion(matchHandle);

    Persons->WaitForBufferedInsert();
    ForumEvents->WaitForBufferedInsert();
    Forums->WaitForBufferedInsert();
    Publications->WaitForBufferedInsert();
    Topics->WaitForBufferedInsert();

    Purchases->WaitForBufferedInsert();
    Sales->WaitForBufferedInsert();
    Authors->WaitForBufferedInsert();
    Includes->WaitForBufferedInsert();
    HasTopic->WaitForBufferedInsert();
    HasOrg->WaitForBufferedInsert();
    waitForCompletion(bufferhandle);
    printf("handle waits returned\n");

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
