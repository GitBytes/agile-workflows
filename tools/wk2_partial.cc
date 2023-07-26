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
  
  std::set<std::pair<uint64_t, uint64_t>> SubPattern8; //a forumevent with jihad topic - key:forumevent id, value: forumid  
  std::mutex SP8_mutex;
  
  void InsertSP8(shad::rt::Handle & handle, const std::pair<uint64_t, uint64_t>& input){
    SubPattern8.insert(input);
  }

  void F9(shad::rt::Handle & handle, const uint64_t& forumID, std::pair<bool, uint64_t>& value, const uint64_t& personID, const RF_args_t & args){
    if(value.first && value.second >1){
      auto time_diff = my_timer() - args.start_time;
      std::cout<<"FOUND THE PATTERN FOR PERSON:"<<personID<<std::endl;
      std::cout << "Time elapsed between the final edge of the pattern read and alert issued: " <<  time_diff <<"s" << std::endl;
    }
  }

  //Function called for a person on Authors multimap (key:person, value: vec<AuthorEdge>) - a person who satisfies SP12, SP5, SP6
  void F8(shad::rt::Handle & handle, const uint64_t& personID, std::vector<AuthorEdge>& value,const RF_args_t & args){
    
    std::sort(value.begin(), value.end(), [](AuthorEdge a, AuthorEdge b) {
      return a.item < b.item;
    });


    auto first1 = SubPattern8.begin();
    auto last1 = SubPattern8.end();
    auto first2 = value.begin();
    auto last2 = value.end();

    auto intersect = std::vector<std::pair<uint64_t, uint64_t>>(); 

    while(first1 != last1 && first2 != last2) {
      if ((*first1).first < (*first2).item) {
        ++first1;
      }
      else {
        if (!((*first2).item < (*first1).first)) {
          intersect.push_back(*first1++);
        }
        ++first2;
      }
    }

    //sort by forum id 
    std::sort(intersect.begin(), intersect.end(), [](std::pair<uint64_t, uint64_t> a, std::pair<uint64_t, uint64_t> b) {
      return a.second < b.second;
    });

    uint64_t curForum = -1;
    
    for(auto f : intersect){
      if(curForum == f.second){
        //at least two in the same forum
        //check if forum is in SP3 
        using SP3type = shad::Hashmap<uint64_t, std::pair<bool, uint64_t>>; 
        auto SubPattern3 = SP3type::GetPtr((shad::ObjectIdentifier<SP3type>)args.SubPattern3_OID);
        SubPattern3->AsyncApply(handle, f.second, F9, personID, args); 
      }
      else{
        curForum = f.second;
      }
    }
  }

  //Function called for a publication in SubPattern5 table (key:publication, value: pair<bool,bool>) - a publication published by a person who sold electronics to an SP6 & SP12 person
  void F7(shad::rt::Handle & handle, const uint64_t& publicationID, std::pair<bool,bool>& value,const uint64_t& personID,const  RF_args_t & args){
    if(value.first && value.second){
      std::cout<<"This person satisfies SP6, SP12, SP5 :"<<personID<<std::endl;
      auto Authors  = AuthorEdgeType::GetPtr((AuthorEdgeOID) args.Authors_OID);
      Authors->AsyncApply(handle, personID, F8, args);
    }
  }

  //Function called for a person on Authors multimap (key:person, value:vec<AuthorEdge>) - a person who sold electronics to an SP6 & SP12 person
  void F6(shad::rt::Handle & handle, const uint64_t& sellerID, std::vector<AuthorEdge>& value,const uint64_t& personID, const RF_args_t & args){
    for (auto & EV : value) {   
      if (EV.dst_type != TYPES::PUBLICATION) continue;
      using SP5type = shad::Hashmap<uint64_t, std::pair<bool, bool>>; 
      auto SubPattern5 = SP5type::GetPtr((shad::ObjectIdentifier<SP5type>)args.SubPattern5_OID);
      SubPattern5->AsyncApply(handle, EV.item, F7, personID, args);
    }
  }

  //Function called for a person on Purchases multimap (key: person, value: vec<PurchaseEdge>) - a person who stisfies SP6 and SP12
  void F5(shad::rt::Handle & handle, const uint64_t& personID, std::vector<PurchaseEdge>& value,const RF_args_t & args){
    for (auto & Prchs : value){
      if(Prchs.product == 11650 ){ 
        auto Authors  = AuthorEdgeType::GetPtr((AuthorEdgeOID) args.Authors_OID);
        Authors->AsyncApply(handle, Prchs.seller, F6, personID, args);
      }
    }
  }

  //Function called for a forum in SubPattern12 table(key: forum, value: pair<score, date>) -  a forum where an SP6 person attended a forumevent at 
  void F4(shad::rt::Handle & handle, const uint64_t& forumID, std::pair<uint64_t, time_t>& value, const uint64_t& personID, time_t& SP6date,const RF_args_t& args){
    if(value.first == 3 && value.second < SP6date){
      auto Purchases  = PurchaseEdgeType::GetPtr((PurchaseEdgeOID) args.Purchases_OID);
      Purchases->AsyncApply(handle, personID, F5, args);
    }
  }

  //Function called for a forumevent on ForumEvents table (key eventID, value:ForumEventVertex) - an event SP6 person attended 
  void F3(shad::rt::Handle & handle, const uint64_t& eventID, ForumEventVertex& value, const uint64_t& personID, time_t& SP6date,const RF_args_t& args){
    using SP12type = shad::Hashmap<uint64_t, std::pair<uint64_t, time_t>>; 
    auto SubPattern12 = SP12type::GetPtr((shad::ObjectIdentifier<SP12type>)args.SubPattern12_OID);
    SubPattern12->AsyncApply(handle, value.forum, F4, personID, SP6date, args);
  } 

  //Function called for a person on Authors multipmap (key:person, value:vec<AuthorEdge>)- a person who satisfies SP6
  void F2(shad::rt::Handle & handle, const uint64_t& personID, std::vector<AuthorEdge>& value, time_t& SP6date,const RF_args_t & args){
    for (auto & EV : value) {   
      if (EV.dst_type != TYPES::FORUMEVENT) continue;
      auto ForumEvents = ForumEventVertexType::GetPtr((ForumEventVertexType::ObjectID)args.ForumEvents_OID);
      ForumEvents->AsyncApply(handle, EV.item, F3, personID, SP6date, args);
    }
  }

  //Function called for each SubPattern6 entry (key:person, value:pair<score, date>)
  void F1(shad::rt::Handle & handle, const uint64_t& personID,std::pair<uint64_t, time_t>& value, const RF_args_t & args){
    if(value.first == 15){ // Person Satisfies SP6
      auto Authors  = AuthorEdgeType::GetPtr((AuthorEdgeOID) args.Authors_OID);
      Authors->AsyncApply(handle, personID, F2, value.second, args);
    }
  }

  // check if a pattern is formed for each person vertex in the graph
  void PatternCheck(shad::rt::Handle& handle,const RF_args_t & args)
  {
    //check each person who satisfy SubPattern6 and see if they also satisfy other SPs 
    using SP6type = shad::Hashmap<uint64_t, std::pair<uint64_t, time_t>>;
    auto SubPattern6 = SP6type::GetPtr((shad::ObjectIdentifier<SP6type>)args.SubPattern6_OID);
    //std::cout<<"PatternCheck called after a SubPattern"<<incoming<<" match"<<std::endl;
    if(SubPattern6->Size()){
      SubPattern6->AsyncForEachEntry(handle, F1, args);
    }
    else{
      //std::cout<<"no SP6 matches"<<std::endl;
    }
  }

  ////////

  class InsertSP12 
  {
    private:
      RF_args_t args;
      shad::rt::Handle patternHandle;
    public:
      //uint64_t key;
      //uint64_t topic;
    
    InsertSP12(){}
    InsertSP12(RF_args_t ar, shad::rt::Handle patternH){
      args =ar;
      patternHandle = patternH;
    }

    bool operator() (std::pair<uint64_t, time_t> *const lhs, const std::pair<uint64_t, time_t> &rhs, bool same_key) {
      if(!same_key){ //if the entry isn't in the table yet, initialize it
          (*lhs).first = rhs.first;
          (*lhs).second = rhs.second;
      }
      else {
        (*lhs).first |= rhs.first;
        (*lhs).second = std::min((*lhs).second, rhs.second);
      }
      

      if ((*lhs).first == 3){
	      auto time_diff = my_timer() - args.start_time;
	      std::cout<<"A SubPattern-12 match is detected and reported in "<< time_diff <<"s" << std::endl;
        shad::rt::asyncExecuteAt(patternHandle, shad::rt::thisLocality(), PatternCheck, args);
        //PatternCheck(12, args);
      }
      return true;
    }
    bool operator() (shad::rt::Handle& handle, std::pair<uint64_t, time_t> *const lhs, const std::pair<uint64_t, time_t> &rhs, bool same_key) {
      if(!same_key){ //if the entry isn't in the table yet, initialize it
          (*lhs).first = rhs.first;
          (*lhs).second = rhs.second;
      }
      else {
        (*lhs).first |= rhs.first;
        (*lhs).second = std::min((*lhs).second, rhs.second);
      }
      

      if ((*lhs).first == 3){
	      auto time_diff = my_timer() - args.start_time;
	      std::cout<<"A SubPattern-12 match is detected and reported in "<< time_diff <<"s" << std::endl;
        shad::rt::asyncExecuteAt(handle, shad::rt::thisLocality(), PatternCheck, args);
        //PatternCheck(12, args);
      }
      return true;
    }
  };

  class InsertSP1
  {
    private:
      RF_args_t args;
      shad::rt::Handle patternHandle;

    public:
      uint64_t topic;
      uint64_t key;
      InsertSP1() {  

      }

      InsertSP1(RF_args_t ar, uint64_t FEkey, uint64_t tpc, shad::rt::Handle patternH) {  
        args = ar;
        key = FEkey;
        topic = tpc;
        patternHandle = patternH;
      }

      bool operator() (uint64_t *const lhs, const uint64_t &rhs, bool same_key) {
        if(!same_key){ //if the entry isn't in the table yet initialize it as 0
          *lhs=0;
          //*lhs = std::move(rhs);
        }
        if (topic == 1049632)
          *lhs |= 2; // Prospect Park
        else if (topic == 69871376)
          *lhs |= 1; // Outdoors

        if(*lhs == 3){
          //here we call upper level check sp12
	        auto time_diff = my_timer() - args.start_time;
          std::cout<<"A SubPattern-1 match is detected and reported in "<< time_diff <<"s" << std::endl;
          using SP12type = shad::Hashmap<uint64_t, std::pair<uint64_t, time_t>>; 
          auto SubPattern12 = SP12type::GetPtr((shad::ObjectIdentifier<SP12type>)args.SubPattern12_OID);
          auto ForumEvents = ForumEventVertexType::GetPtr((ForumEventVertexType::ObjectID)args.ForumEvents_OID);
          ForumEventVertex FEV;
          ForumEvents->Lookup(key, &FEV);

          std::pair<uint64_t, time_t> sp12tmp(1, FEV.date);
          InsertSP12 inserter(args, patternHandle);
          // shad::rt::Handle nextHandle;
          // SubPattern12->AsyncInsert(nextHandle, inserter, FEV.forum, sp12tmp);
          SubPattern12->Insert(inserter, FEV.forum, sp12tmp);
        }
        return true;
      }
      
      bool operator() (shad::rt::Handle& handle, uint64_t *const lhs, const uint64_t &rhs, bool same_key) {
        if(!same_key){ //if the entry isn't in the table yet initialize it as 0
          *lhs=0;
          //*lhs = std::move(rhs);
        }
        if (topic == 1049632)
          *lhs |= 2; // Prospect Park
        else if (topic == 69871376)
          *lhs |= 1; // Outdoors

        if(*lhs == 3){
          //here we call upper level check sp12
	        auto time_diff = my_timer() - args.start_time;
          std::cout<<"A SubPattern-1 match is detected and reported in "<< time_diff <<"s" << std::endl;
          using SP12type = shad::Hashmap<uint64_t, std::pair<uint64_t, time_t>>; 
          auto SubPattern12 = SP12type::GetPtr((shad::ObjectIdentifier<SP12type>)args.SubPattern12_OID);
          auto ForumEvents = ForumEventVertexType::GetPtr((ForumEventVertexType::ObjectID)args.ForumEvents_OID);
          ForumEventVertex FEV;
          ForumEvents->Lookup(key, &FEV);

          std::pair<uint64_t, time_t> sp12tmp(1, FEV.date);
          InsertSP12 inserter(args, handle);
          // shad::rt::Handle nextHandle;
          SubPattern12->AsyncInsert(handle, inserter, FEV.forum, sp12tmp);
          //SubPattern12->Insert(inserter, FEV.forum, sp12tmp);
        }
        return true;
      }
  };

  class InsertSP2
  {
    private:
      RF_args_t args;
      shad::rt::Handle patternHandle;
    public:
      uint64_t key;
      uint64_t topic;

    InsertSP2(){}
    InsertSP2(RF_args_t ar, uint64_t FEkey, uint64_t tpc, shad::rt::Handle patternH){
      args =ar;
      key = FEkey;
      topic = tpc;
      patternHandle = patternH;
    }

    bool operator() (uint64_t *const lhs, const uint64_t &rhs, bool same_key) {
      if(!same_key){ //if the entry isn't in the table yet initialize it as 0
          *lhs=0;
      }
      
      if (topic == 127197)
        *lhs |= 4; // Bomb
      else if (topic == 179057)
        *lhs |= 2; // Explosion
      else if (topic == 771572)
        *lhs |= 1; // Williamsburg  

      if (*lhs == 7)
      {
	      auto time_diff = my_timer() - args.start_time;
	      std::cout<<"A SubPattern-2 match is detected and reported in "<< time_diff <<"s" << std::endl;
        using SP12type = shad::Hashmap<uint64_t, std::pair<uint64_t, time_t>>;
        auto SubPattern12 = SP12type::GetPtr( (shad::ObjectIdentifier<SP12type>) args.SubPattern12_OID);
        auto ForumEvents  = ForumEventVertexType::GetPtr( (ForumEventVertexOID) args.ForumEvents_OID);

        ForumEventVertex FEV;
        ForumEvents->Lookup(key, &FEV);
        std::pair<uint64_t, time_t> sp12tmp(2, FEV.date);
        InsertSP12 inserter(args, patternHandle);
        // shad::rt::Handle nextHandle;
        // SubPattern12->AsyncInsert(nextHandle, inserter, FEV.forum, sp12tmp);
        SubPattern12->Insert(inserter, FEV.forum, sp12tmp);
      }
      return true;
    }
    bool operator() (shad::rt::Handle& handle, uint64_t *const lhs, const uint64_t &rhs, bool same_key) {
      if(!same_key){ //if the entry isn't in the table yet initialize it as 0
          *lhs=0;
      }
      
      if (topic == 127197)
        *lhs |= 4; // Bomb
      else if (topic == 179057)
        *lhs |= 2; // Explosion
      else if (topic == 771572)
        *lhs |= 1; // Williamsburg  

      if (*lhs == 7)
      {
	      auto time_diff = my_timer() - args.start_time;
	      std::cout<<"A SubPattern-2 match is detected and reported in "<< time_diff <<"s" << std::endl;
        using SP12type = shad::Hashmap<uint64_t, std::pair<uint64_t, time_t>>;
        auto SubPattern12 = SP12type::GetPtr( (shad::ObjectIdentifier<SP12type>) args.SubPattern12_OID);
        auto ForumEvents  = ForumEventVertexType::GetPtr( (ForumEventVertexOID) args.ForumEvents_OID);

        ForumEventVertex FEV;
        ForumEvents->Lookup(key, &FEV);
        std::pair<uint64_t, time_t> sp12tmp(2, FEV.date);
        InsertSP12 inserter(args, handle);
        // shad::rt::Handle nextHandle;
        SubPattern12->AsyncInsert(handle, inserter, FEV.forum, sp12tmp);
        //SubPattern12->Insert(inserter, FEV.forum, sp12tmp);
      }
      return true;
    }
  };

  class InsertSP3
  {

    private:
      RF_args_t args;
      shad::rt::Handle patternHandle;
    public:
      uint64_t key;
      uint64_t topic;

    InsertSP3(){}
    InsertSP3(RF_args_t ar, uint64_t ForumKey, uint64_t tpc, shad::rt::Handle patternH){
      args =ar;
      key = ForumKey;
      topic = tpc;
      patternHandle = patternH;
    }

    bool operator() (std::pair<bool, uint64_t> *const lhs, const std::pair<bool, uint64_t> &rhs, bool same_key) {
      if(!same_key){ //if the entry isn't in the table yet initialize it as 0
          (*lhs).first = false;
          (*lhs).second = 0;
      }

      if(topic == 60){ //NYC
        (*lhs).first = true;
      }
      else if(topic == 44311) { //Jihad
        (*lhs).second ++;
      }
      
      if((*lhs).first && (*lhs).second >= 2){
	      auto time_diff = my_timer() - args.start_time;
	      std::cout<<"A SubPattern-3 match is detected and reported in "<< time_diff <<"s" << std::endl;
        shad::rt::asyncExecuteAt(patternHandle, shad::rt::thisLocality(), PatternCheck, args);
        //PatternCheck(3, args);
      }
      return true;
    }

    bool operator() (shad::rt::Handle& handle, std::pair<bool, uint64_t> *const lhs, const std::pair<bool, uint64_t> &rhs, bool same_key) {
      if(!same_key){ //if the entry isn't in the table yet initialize it as 0
          (*lhs).first = false;
          (*lhs).second = 0;
      }

      if(topic == 60){ //NYC
        (*lhs).first = true;
      }
      else if(topic == 44311) { //Jihad
        (*lhs).second ++;
      }
      
      if((*lhs).first && (*lhs).second >= 2){
	      auto time_diff = my_timer() - args.start_time;
	      std::cout<<"A SubPattern-3 match is detected and reported in "<< time_diff <<"s" << std::endl;
        shad::rt::asyncExecuteAt(handle, shad::rt::thisLocality(), PatternCheck, args);
        //PatternCheck(3, args);
      }
      return true;
    }
  };
  
  class InsertSP5
  {
    private:
      RF_args_t args;
      shad::rt::Handle patternHandle; 
    public:


    InsertSP5(){}
    InsertSP5(RF_args_t ar, shad::rt::Handle patternH){
      args =ar;
      patternHandle = patternH;
    }

    bool operator() (std::pair<bool, bool> *const lhs, const std::pair<bool, bool> &rhs, bool same_key) {
      if(!same_key){ //if the entry isn't in the table yet initialize it as 0
          (*lhs).first = false;
          (*lhs).second = false;
      }

      if( rhs.first ){
        (*lhs).first = true;
      }
      else if (rhs.second) {
        (*lhs).second = true;
      }

      if((*lhs).first && (*lhs).second){
	      auto time_diff = my_timer() - args.start_time;
	      std::cout<<"A SubPattern-5 match is detected and reported in "<< time_diff <<"s" << std::endl;
        shad::rt::asyncExecuteAt(patternHandle, shad::rt::thisLocality(), PatternCheck, args);
        //PatternCheck(5, args);
      }
      return true;
    }
    bool operator() (shad::rt::Handle& handle, std::pair<bool, bool> *const lhs, const std::pair<bool, bool> &rhs, bool same_key) {
      if(!same_key){ //if the entry isn't in the table yet initialize it as 0
          (*lhs).first = false;
          (*lhs).second = false;
      }

      if( rhs.first ){
        (*lhs).first = true;
      }
      else if (rhs.second) {
        (*lhs).second = true;
      }

      if((*lhs).first && (*lhs).second){
	      auto time_diff = my_timer() - args.start_time;
	      std::cout<<"A SubPattern-5 match is detected and reported in "<< time_diff <<"s" << std::endl;
        shad::rt::asyncExecuteAt(handle, shad::rt::thisLocality(), PatternCheck, args);
        //PatternCheck(5, args);
      }
      return true;
    }
  };

  class InsertSP6
  {
    private:
      RF_args_t args;
      shad::rt::Handle patternHandle;
    public:


    InsertSP6(){}
    InsertSP6(RF_args_t ar, shad::rt::Handle patternH){
      args =ar;
      patternHandle = patternH;
    }

    bool operator() (std::pair<uint64_t, time_t> *const lhs, const std::pair<uint64_t, time_t> &rhs, bool same_key) {
      if(!same_key){ //if the entry isn't in the table yet initialize it as 0
          (*lhs).first = 0;
          (*lhs).second = 0;
      }

      if (rhs.first == 2869238) 
        (*lhs).first |= 8;     // Bath Bomb
      else if (rhs.first == 271997)  
        (*lhs).first |= 4;     // Pressure Cooker
      else if (rhs.first == 11650)   
        (*lhs).first |= 2;     // Electronics
      else if (rhs.first == 185785)  
        (*lhs).first |= 1;     // Ammunition


      (*lhs).second = std::max((*lhs).second, rhs.second);
      if ((*lhs).first == 15){
	      auto time_diff = my_timer() - args.start_time;
	      std::cout<<"A SubPattern-6 match is detected and reported in "<< time_diff <<"s" << std::endl;
        shad::rt::asyncExecuteAt(patternHandle, shad::rt::thisLocality(), PatternCheck, args);
        //PatternCheck(6, args);
      }
      return true;
    }
    bool operator() (shad::rt::Handle& handle, std::pair<uint64_t, time_t> *const lhs, const std::pair<uint64_t, time_t> &rhs, bool same_key) {
      if(!same_key){ //if the entry isn't in the table yet initialize it as 0
          (*lhs).first = 0;
          (*lhs).second = 0;
      }

      if (rhs.first == 2869238) 
        (*lhs).first |= 8;     // Bath Bomb
      else if (rhs.first == 271997)  
        (*lhs).first |= 4;     // Pressure Cooker
      else if (rhs.first == 11650)   
        (*lhs).first |= 2;     // Electronics
      else if (rhs.first == 185785)  
        (*lhs).first |= 1;     // Ammunition


      (*lhs).second = std::max((*lhs).second, rhs.second);
      if ((*lhs).first == 15){
	      auto time_diff = my_timer() - args.start_time;
	      std::cout<<"A SubPattern-6 match is detected and reported in "<< time_diff <<"s" << std::endl;
        shad::rt::asyncExecuteAt(handle, shad::rt::thisLocality(), PatternCheck, args);
        //PatternCheck(6, args);
      }
      return true;
    }
  };

  class InsertSP7
  {
    private:
      RF_args_t args;
      shad::rt::Handle patternHandle;
    public:


    InsertSP7(){}
    InsertSP7(RF_args_t ar, shad::rt::Handle patternH){
      args =ar;
      patternHandle = patternH;
    }

    bool operator() (std::pair<int64_t, time_t> *const lhs, const std::pair<int64_t, time_t> &rhs, bool same_key) {
      if(!same_key){ //if the entry isn't in the table yet initialize it as 0
          (*lhs).first = shad::data_types::kNullValue<uint64_t>;
          (*lhs).second = shad::data_types::kNullValue<time_t>;
      }

      if ((*lhs).first == shad::data_types::kNullValue<uint64_t>) {   // first sell record for seller (first buyer)
        (*lhs).first = rhs.first;
        (*lhs).second = rhs.second;
      }

      else if((*lhs).first >= 0){ //not the first sell record , check if two separate buyers 
        if ((*lhs).first == rhs.first) {  //same buyer, we update the date if necessary  
          (*lhs).second = std::max((*lhs).second, rhs.second); //BURCU check this 
        }
        else{ //we have 2 separate buyers , seller is now a distributor

          using SP6type = shad::Hashmap<uint64_t, std::pair<uint64_t, time_t>>;
          auto SubPattern6 = SP6type::GetPtr( (shad::ObjectIdentifier<SP6type>) args.SubPattern6_OID);
          uint64_t ammunition = 185785;
          InsertSP6 inserter (args, patternHandle);

          //update SP6 for first buyer
          SubPattern6->Insert(inserter, (*lhs).first, std::pair<uint64_t, time_t>(ammunition,(*lhs).second));
          //update SP6 for second buyer
          SubPattern6->Insert(inserter, rhs.first, std::pair<uint64_t, time_t>(ammunition,rhs.second));

          (*lhs).first = -1 * (*lhs).first;
        }
      }
      else{ //seller is a known distributor
        using SP6type = shad::Hashmap<uint64_t, std::pair<uint64_t, time_t>>;
        auto SubPattern6 = SP6type::GetPtr( (shad::ObjectIdentifier<SP6type>) args.SubPattern6_OID);
        uint64_t ammunition = 185785;
        InsertSP6 inserter (args, patternHandle);
        SubPattern6->Insert(inserter, rhs.first, std::pair<uint64_t, time_t>(ammunition,rhs.second));
      }
      auto time_diff = my_timer() - args.start_time;
      std::cout<<"A SubPattern-7 match is detected and reported in "<< time_diff <<"s" << std::endl;
      return true;
    }
    bool operator() (shad::rt::Handle& handle, std::pair<int64_t, time_t> *const lhs, const std::pair<int64_t, time_t> &rhs, bool same_key) {
      if(!same_key){ //if the entry isn't in the table yet initialize it as 0
          (*lhs).first = shad::data_types::kNullValue<uint64_t>;
          (*lhs).second = shad::data_types::kNullValue<time_t>;
      }

      if ((*lhs).first == shad::data_types::kNullValue<uint64_t>) {   // first sell record for seller (first buyer)
        (*lhs).first = rhs.first;
        (*lhs).second = rhs.second;
      }

      else if((*lhs).first >= 0){ //not the first sell record , check if two separate buyers 
        if ((*lhs).first == rhs.first) {  //same buyer, we update the date if necessary  
          (*lhs).second = std::max((*lhs).second, rhs.second); //BURCU check this 
        }
        else{ //we have 2 separate buyers , seller is now a distributor

          using SP6type = shad::Hashmap<uint64_t, std::pair<uint64_t, time_t>>;
          auto SubPattern6 = SP6type::GetPtr( (shad::ObjectIdentifier<SP6type>) args.SubPattern6_OID);
          uint64_t ammunition = 185785;
          InsertSP6 inserter (args, handle);

          //update SP6 for first buyer
          //SubPattern6->Insert(inserter, (*lhs).first, std::pair<uint64_t, time_t>(ammunition,(*lhs).second));
          SubPattern6->AsyncInsert(handle, inserter, (*lhs).first, std::pair<uint64_t, time_t>(ammunition,(*lhs).second));
          //update SP6 for second buyer
          //SubPattern6->Insert(inserter, rhs.first, std::pair<uint64_t, time_t>(ammunition,rhs.second));
          SubPattern6->AsyncInsert(handle, inserter, rhs.first, std::pair<uint64_t, time_t>(ammunition,rhs.second));

          (*lhs).first = -1 * (*lhs).first;
        }
      }
      else{ //seller is a known distributor
        using SP6type = shad::Hashmap<uint64_t, std::pair<uint64_t, time_t>>;
        auto SubPattern6 = SP6type::GetPtr( (shad::ObjectIdentifier<SP6type>) args.SubPattern6_OID);
        uint64_t ammunition = 185785;
        InsertSP6 inserter (args, handle);
        //SubPattern6->Insert(inserter, rhs.first, std::pair<uint64_t, time_t>(ammunition,rhs.second));
        SubPattern6->AsyncInsert(handle, inserter, rhs.first, std::pair<uint64_t, time_t>(ammunition,rhs.second));
      
      }
      auto time_diff = my_timer() - args.start_time;
      std::cout<<"A SubPattern-7 match is detected and reported in "<< time_diff <<"s" << std::endl;
      return true;
    }
  };


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


  int main(int argc, char *argv[])
  {

    double time1 = my_timer();

    Graph_t graph;
    std::string dataFile = argv[1];
    uint64_t num_edges, num_vertices;

    auto Persons = PersonVertexType::Create(AGILE_MEDIUM);
    auto ForumEvents = ForumEventVertexType::Create(AGILE_MEDIUM);
    auto Forums = ForumVertexType::Create(AGILE_SMALL);
    auto Publications = PublicationVertexType::Create(AGILE_SMALL);
    auto Topics = TopicVertexType::Create(AGILE_SMALL);

    auto Purchases = PurchaseEdgeType::Create(AGILE_MEDIUM);
    auto Sales = SaleEdgeType::Create(AGILE_MEDIUM);
    auto Authors = AuthorEdgeType::Create(AGILE_LARGE);
    auto Includes = IncludesEdgeType::Create(AGILE_LARGE);
    auto HasTopic = HasTopicEdgeType::Create(AGILE_LARGE);
    auto HasOrg = HasOrgEdgeType::Create(AGILE_MEDIUM);
    
    // Partial match subpattern scoreboards
    // Level1
    // SubPattern1  : A FORUMEVENT with Prospect Park, Outdoors
    // SubPattern2  : A FORUMEVENT with Bomb, Explosion, Williamsburg
    // SubPattern3  : A FORUM that has NYC topic and 2 FEs with Jihad topic
    // SubPattern5  : A publication with Electrical Engineering as topic and organization close to NYC
    // SubPattern6  : A person who purchased bath bomb, pressure cooker, electronics, ammunition from distributor
    // SubPattern7  : A person who is an ammunition distributer (multiple buyers)
    // SubPattern8  : set of forumevents with jihad topic (set of pair<forumeventID, forumID>)
    
    // Level2
    // SubPattern12 : A FORUM that has forumevents that satisfy both SP1 and SP2
    auto SubPattern1 = shad::Hashmap<uint64_t, uint64_t>::Create(AGILE_TINY);
    auto SubPattern2 = shad::Hashmap<uint64_t, uint64_t>::Create(AGILE_TINY);
    auto SubPattern12 = shad::Hashmap<uint64_t, std::pair<uint64_t, time_t>>::Create(AGILE_TINY);
    auto SubPattern3 = shad::Hashmap<uint64_t, std::pair<bool, uint64_t>>::Create(AGILE_TINY);
    auto SubPattern5 = shad::Hashmap<uint64_t, std::pair<bool, bool>>::Create(AGILE_SMALL);
    auto SubPattern6 = shad::Hashmap<uint64_t, std::pair<uint64_t, time_t>>::Create(AGILE_SMALL);
    auto SubPattern7 = shad::Hashmap<uint64_t, std::pair<int64_t, time_t>>::Create(AGILE_SMALL);

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
    args.SubPattern5_OID = graph["SubPattern5"];
    args.SubPattern6_OID = graph["SubPattern6"];
    args.SubPattern7_OID = graph["SubPattern7"];
 
    shad::rt::Handle bufferhandle;
    shad::rt::Handle matchHandle;
    shad::rt::Handle patternHandle;


    std::ifstream file(dataFile);
    if (!file.is_open())
    {
      printf("cannot open file %s\n", dataFile.c_str());
      exit(-1);
    }
    std::string dataLine;
    uint64_t counter = 0;
    uint64_t rest=0;
    
    while (getline(file, dataLine))
    {
      if (dataLine[0] == '#')
        continue; // skip comments
      counter++;
      // if (counter == 33769810) 
      // if(! (counter % 10000)){
      //   std::cout<<counter<<" "<<rest<<std::endl;
      // }
      args.start_time = my_timer();
      std::vector<std::string> tokens = split(dataLine, ',', 10);
      if (tokens[0] == "HasTopic")
      {
        HasTopicEdge record(tokens);
        // FORUM EVENT with Prospect Park and Outdoors topics
        if ((tokens[4] != "") && ((tokens[6] == "1049632") || (tokens[6] == "69871376")))
        { 
          HasTopic->AsyncInsert(matchHandle, record.key(), record);
          InsertSP1 inserter(args, record.key(), record.topic, patternHandle);
          SubPattern1->AsyncInsert(matchHandle, inserter, record.key(), 0);
	}
        //FORUMEVENT with Bomb, Explosion, and Williamsburg topics 
        else if ((tokens[4] != "") && ((tokens[6] == "127197") || (tokens[6] == "179057") || (tokens[6] == "771572")))
        { 
          HasTopic->AsyncInsert(matchHandle, record.key(), record);
          InsertSP2 inserter(args, record.key(), record.topic, patternHandle);
          SubPattern2->AsyncInsert(matchHandle, inserter, record.key(), 0);
        }
        //FORUM with NYC topic
        else if ((tokens[3] != "") && (tokens[6] == "60")) {     
          HasTopic->AsyncInsert(matchHandle, record.key(), record);
          InsertSP3 inserter(args, record.key(), record.topic, patternHandle);
          SubPattern3->AsyncInsert(matchHandle, inserter, record.key(), std::pair<bool, uint64_t> (0,0));
        }
        //FORUMEVENT with Jihad topic
        else if ((tokens[4] != "") && (tokens[6] == "44311")) 
        {
          HasTopic->AsyncInsert(matchHandle, record.key(), record);
          //get forumevent's forum 
          ForumEventVertex FEV;
          ForumEvents->Lookup(record.key(), &FEV);
          InsertSP3 inserter(args, FEV.forum, record.topic, patternHandle);
          SubPattern3->AsyncInsert(matchHandle, inserter, FEV.forum, std::pair<bool, uint64_t> (0,0));
          shad::rt::asyncExecuteOnAll(matchHandle, InsertSP8, std::pair<uint64_t,uint64_t>(record.key(), FEV.forum));
        }
        //PUBLICATION with Electrical Engineering topic
        else if( (tokens[5] != "") && (tokens[6] == "43035"))
        {
          HasTopic->AsyncInsert(matchHandle, record.key(), record);
          InsertSP5 inserter (args, patternHandle);
          SubPattern5->AsyncInsert(matchHandle, inserter, record.key(), std::pair<bool, bool>(false, true));
        }
        else //Rest of the records doesn't matter for the pattern check
        {
          HasTopic->BufferedAsyncInsert(bufferhandle, record.key(), record);
        }
      }
      else if(tokens[0] == "HasOrg"){
        HasOrgEdge record(tokens);
        //PUBLICATION close to NYC

        //PUBLICATION close to NYC
        auto CheckProx = [](shad::rt::Handle &matchHandle, const uint64_t &org,
                            TopicVertex &value, RF_args_t& rfargs, uint64_t& rec_key, shad::rt::Handle& patternHandle) {
         
          double lon_miles = 0.91 * std::abs(-73.94 - value.lon);
          double lat_miles = 1.15 * std::abs(40.67 - value.lat);
          double distance = std::sqrt(lon_miles * lon_miles + lat_miles * lat_miles);
          if (distance <= 30.0) {
            InsertSP5 inserter (rfargs, patternHandle);
            using SP5type = shad::Hashmap<uint64_t, std::pair<bool, bool>>;
            auto SubPattern5 = SP5type::GetPtr((shad::ObjectIdentifier<SP5type>)rfargs.SubPattern5_OID);
            SubPattern5->AsyncInsert(matchHandle, inserter, rec_key, std::pair<bool, bool>(true, false));
	  }
        };
        if( tokens[5] != "")
        {
          HasOrg->AsyncInsert(matchHandle, record.key(), record);
          auto t = record.key();
          Topics->AsyncApply(matchHandle, record.organization, CheckProx, args, t, patternHandle);
        }
        else{//Rest of the records doesn't matter for the pattern check
          HasOrg->BufferedAsyncInsert(bufferhandle, record.key(), record);
          rest++;
	}
      }
      else if(tokens[0] == "Sale"){
        SaleEdge sale(tokens);
        PurchaseEdge purchase(tokens);

        if ( (tokens[6] == "2869238") || (tokens[6] == "271997") || (tokens[6] == "11650") ) {//bath bomb, pressure cooker, electronics
          Sales->AsyncInsert(matchHandle, sale.key(), sale);
          Purchases->AsyncInsert(matchHandle, purchase.key(), purchase);
          InsertSP6 inserter (args, patternHandle);
          SubPattern6->AsyncInsert(matchHandle, inserter, purchase.key(), std::pair<uint64_t, time_t>(purchase.product,purchase.date));
	  //waitForCompletion(matchHandle);
        }
        else if (tokens[6] == "185785") { //ammunition
          Sales->AsyncInsert(matchHandle, sale.key(), sale);
          Purchases->AsyncInsert(matchHandle, purchase.key(), purchase);
          InsertSP7 inserter (args, patternHandle);
          SubPattern7->AsyncInsert(matchHandle, inserter, sale.key(), std::pair<int64_t, time_t>(purchase.buyer, purchase.date));
	  //waitForCompletion(matchHandle);
        }
        else {//Rest of the records doesn't matter for the pattern check
          Sales->BufferedAsyncInsert(bufferhandle, sale.key(), sale);
          Purchases->BufferedAsyncInsert(bufferhandle, purchase.key(), purchase);
          rest++;
	}  
      }
      
      else//Rest of the records doesn't matter for the pattern check
      {
        insertToGraphBuffered(bufferhandle, dataLine, graph);
        rest++;
      }
#ifdef PRINT_STATS
      if(counter % 10000 == 0) {
	std::cout << counter << " " << counter - rest << " " << 
	  rest << " " << SubPattern1->Size() << " " <<
	  SubPattern2->Size() << " " <<
	  SubPattern12->Size() << " " <<
	  SubPattern3->Size() << " " <<
	  SubPattern5->Size() << " " <<
	  SubPattern6->Size() << " " <<
	  SubPattern7->Size() << " " << std::endl;
	
	// printf("Number of SP1 matches = %lu\n", SubPattern1->Size());
	// printf("Number of SP2 matches = %lu\n", SubPattern2->Size());
	// printf("Number of SP3 matches = %lu\n", SubPattern3->Size());
	// printf("Number of SP5 matches = %lu\n", SubPattern5->Size());
	// printf("Number of SP12 matches = %lu\n", SubPattern12->Size());
	// printf("Number of SP6 matches = %lu\n", SubPattern6->Size());
	// printf("Number of SP7 matches = %lu\n", SubPattern7->Size());
	// printf("Number of Jihad Events = %lu\n", SubPattern8.size());
      }
#endif
    }
    printf("Got out of the loop\n");
    printf("Time for ingestion = %lf\n", my_timer() - time1);

    waitForCompletion(patternHandle);
    waitForCompletion(matchHandle);

    printf("Insert handles returned %lf\n", my_timer() - time1);
 
    waitForCompletion(bufferhandle);
    Persons->AsyncWaitForBufferedInsert(bufferhandle);
    ForumEvents->AsyncWaitForBufferedInsert(bufferhandle);
    Forums->AsyncWaitForBufferedInsert(bufferhandle);
    Publications->AsyncWaitForBufferedInsert(bufferhandle);
    Topics->AsyncWaitForBufferedInsert(bufferhandle);

    Purchases->AsyncWaitForBufferedInsert(bufferhandle);
    Sales->AsyncWaitForBufferedInsert(bufferhandle);
    Authors->AsyncWaitForBufferedInsert(bufferhandle);
    Includes->AsyncWaitForBufferedInsert(bufferhandle);
    HasTopic->AsyncWaitForBufferedInsert(bufferhandle);
    HasOrg->AsyncWaitForBufferedInsert(bufferhandle);

    waitForCompletion(bufferhandle);

    printf("handle waits returned\n");
    // one last check after finishing all handles
    shad::rt::asyncExecuteAt(patternHandle, shad::rt::thisLocality(), PatternCheck, args);
    waitForCompletion(patternHandle);
    printf("After final patterncheck %lf\n", my_timer() - time1);

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
    printf("Number of SP5 matches = %lu\n", SubPattern5->Size());
    printf("Number of SP12 matches = %lu\n", SubPattern12->Size());
    printf("Number of SP6 matches = %lu\n", SubPattern6->Size());
    printf("Number of SP7 matches = %lu\n", SubPattern7->Size());
    printf("Number of Jihad Events = %lu\n", SubPattern8.size());
    printf("\n");
    //printf("Total number of edges    = %lu\n", num_edges);
    //printf("Total number of vertices = %lu\n\n", num_vertices);
    printf("Total number of records = %lu\n", counter);
    printf("Total number of pattern-related records = %lu (%f%)\n", counter-rest, (counter-rest)*100.0/counter);
    return 0;
  }

  } // namespace shad
