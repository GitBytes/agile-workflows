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
#include <string>

#include "shad/data_structures/hashmap.h"
#include "shad/data_structures/multimap.h"
#include "agile/workflow2/main.h"
#include "agile/workflow2/graph.h"
#include "agile/workflow2/globalIDS.h"

namespace agile::workflow2{
using FreqArr = std::array<uint64_t, 5>;

void createSquareMatrix(uint64_t k, Edge & edge, uint64_t & AV_OID,
     uint64_t & AE_OID, uint64_t & BV_OID, uint64_t & BE_OID, uint64_t & a_num_vertices) {

  uint64_t i = edge.src;
  uint64_t j = edge.dst;
  auto A_Edges = EdgeType::GetPtr((EdgeOID) AE_OID);
  auto B_Edges = EdgeType::GetPtr((EdgeOID) BE_OID);
  auto A_Vertices = VertexType::GetPtr((VertexOID) AV_OID);
  auto B_Vertices = VertexType::GetPtr((VertexOID) BV_OID);

  edge.weight += 1;     /// Initial weight

  /// Get degree of i in A and resize nA, then get the neighbors in nA
  if (i >= a_num_vertices) std::swap(i, j);
  uint64_t start  = (A_Vertices->At(i)).edges;
  uint64_t end    = (A_Vertices->At(i + 1)).edges;
  uint64_t start1 = (B_Vertices->At(j - a_num_vertices)).edges;
  uint64_t end1   = (B_Vertices->At(j + 1 - a_num_vertices)).edges;

  if ((end - start) <= 0 || (end1 - start1) <= 0) return;

  Handle handle;
  std::vector<Edge> Aneighbors(end - start);
  A_Edges->AsyncGetElements(handle, Aneighbors.data(), start, end - start);
  
  /// Get degree of j in B and resize nB, then get the neighbors in nB
  std::vector<Edge> Bneighbors(end1 - start1);
  B_Edges->AsyncGetElements(handle, Bneighbors.data(), start1, end1 - start1);
  shad::rt::waitForCompletion(handle);

  for (int index1 = 0;      index1 < Aneighbors.size(); index1 ++) {
  for (int index2 = index1; index2 < Bneighbors.size(); index2 ++) {
      if ( (Aneighbors[index1].type     == Bneighbors[index2].type) &&
           (Aneighbors[index1].dst_type == Bneighbors[index2].dst_type) ) edge.weight++;
} } }


struct args_L_t {
  TYPES type;
  uint64_t src;
  uint64_t glbid;
  uint64_t edges;
  uint64_t num_edges;
  uint64_t edgeOID;
  VertexLType::iterator A_begin;
  VertexLType::iterator A_end;
  VertexLType::iterator B_begin;
  VertexLType::iterator B_end;
};


void copy_BEdges(Handle & handle, const args_L_t & args)  {
  auto Edges = EdgeType::GetPtr((EdgeOID) args.edgeOID);
  uint64_t locale = (uint32_t) shad::rt::thisLocality();

  if (locale < shad::rt::numLocalities() - 1)     // check next locale for additional vertices of type
     asyncExecuteAt(handle, shad::rt::Locality(locale + 1), copy_BEdges, args);

  Handle my_handle;
  args_L_t my_args = args;
  std::vector<Edge> edges(args.num_edges);
  auto itr_A = VertexLType::iterator::local_range(my_args.A_begin, my_args.A_end);

  Edges->AsyncGetElements(my_handle, edges.data(), args.edges, args.num_edges);    // get edges to copy
  waitForCompletion(my_handle);

  for (auto itr = itr_A.begin(); itr != itr_A.end(); itr ++) {                     // for each vertex A on this locale
    if ((* itr).type != args.type) continue;                                       // ... A vertex has wrong type

    for (uint64_t i = 0; i < args.num_edges; i ++) {                               // ... for each edge
      edges[i].src = (* itr).label;                                                // ... ... update src label
      edges[i].src_glbid = (* itr).id;                                             // ... ... update global id
    }

    Edges->AsyncInsertAt(handle, (* itr).edges, edges.data(), args.num_edges);     // ... copy edges
} }


void create_BEdges(Handle & handle, const args_L_t & args) {
  args_L_t my_args = args;
  uint64_t locale = (uint32_t) shad::rt::thisLocality();
  auto Edges = EdgeType::GetPtr((EdgeOID) args.edgeOID);
  auto itr_B = VertexLType::iterator::local_range(my_args.B_begin, my_args.B_end);

  for (auto itr = itr_B.begin(); itr != itr_B.end(); itr ++) {     // for each vertex B on this locale
    if ((* itr).type != args.type) continue;                       // ... B vertex has wrong type

    Edge edge(args.src, (* itr).label, 0.0, args.type, args.type, args.type, args.glbid, (* itr).id);
    Edges->AsyncInsertAt(handle, my_args.edges, edge);
    my_args.edges ++;
  }

  if (locale < shad::rt::numLocalities() - 1)     // check next locale for additional vertices of type
     shad::rt::asyncExecuteAt(handle, shad::rt::Locality(locale + 1), create_BEdges, my_args);
}


void create_LVertex(Handle & handle, size_t i, Vertex & vertex, uint64_t & offset, GraphL & L, FreqArr & freq) {
  VertexL vtemp(i + offset, vertex.id, freq[(uint64_t) vertex.type], vertex.type, -1, -1, 0);
  L.vertexPtr()->AsyncInsertAt(handle, i + offset, vtemp);
}


void create_LEdges(Handle & handle, const args_L_t & args)  {
  args_L_t my_args = args;
  auto itr_A = VertexLType::iterator::local_range(my_args.A_begin, my_args.A_end);

  for (auto itr = itr_A.begin(); itr != itr_A.end(); itr ++) {     // for each vertex A on this locale
    if ((* itr).type != args.type) continue;                       // ... A vertex has wrong type

    Handle my_handle;                                              // ... found first A vertex of type
    my_args.src = (* itr).label;                                   // ... ... create L edges
    my_args.glbid = (* itr).id;                                    // ... ... copy edges to other A's of type
    my_args.edges = (* itr).edges;

    shad::rt::asyncExecuteAt(my_handle, shad::rt::Locality(0), create_BEdges, my_args);
    waitForCompletion(my_handle);

    my_args.A_begin = VertexLType::iterator::iterator_from_local(my_args.A_begin, my_args.A_end, itr) + 1;
    if (my_args.A_begin == my_args.A_end) return;     // no A vertices left to scan

    VertexL next = * my_args.A_begin;
    my_args.num_edges = next.edges - my_args.edges;
    shad::rt::asyncExecuteAt(handle, shad::rt::Locality(0), copy_BEdges, my_args);
    return;
  }

  uint64_t locale = (uint32_t) shad::rt::thisLocality();           // vertex of type not found, check next locale
  shad::rt::asyncExecuteAt(handle, shad::rt::Locality(locale + 1), create_LEdges, args);
}


void createBipartite(Graph_t &A, Graph_t &B, GraphL &L) {
  L.a_num_vertices = GlobalIDType::GetPtr((GlobalIDOID) A["GlobalIDS"])->Size();
  L.b_num_vertices = GlobalIDType::GetPtr((GlobalIDOID) B["GlobalIDS"])->Size();
  L.vertexNumber = L.a_num_vertices + L.b_num_vertices;

  VertexL vtemp(0, 0, 0, TYPES::NONE, -1,  -1, 0);
  L.vertexOID = shad::Array<VertexL>::Create(L.vertexNumber + 1, vtemp)->GetGlobalID();
  L.vertexPtr()->FillPtrs();

  ///// THIS PART IS HAND CODED
  FreqArr AtypeFreq, BtypeFreq;
  AtypeFreq[0] = (uint64_t)PersonVertexType::GetPtr( (PersonVertexOID) A["Persons"] )->Size();
  AtypeFreq[1] = (uint64_t)ForumEventVertexType::GetPtr( (ForumEventVertexOID) A["ForumEvents"] )->Size();
  AtypeFreq[2] = (uint64_t)ForumVertexType::GetPtr( (ForumVertexOID) A["Forums"] )->Size();
  AtypeFreq[3] = (uint64_t)PublicationVertexType::GetPtr( (PublicationVertexOID) A["Publications"] )->Size();
  AtypeFreq[4] = (uint64_t)TopicVertexType::GetPtr( (TopicVertexOID) A["Topics"] )->Size();

  BtypeFreq[0] = (uint64_t)PersonVertexType::GetPtr( (PersonVertexOID) B["Persons"] )->Size();
  BtypeFreq[1] = (uint64_t)ForumEventVertexType::GetPtr( (ForumEventVertexOID) B["ForumEvents"] )->Size();
  BtypeFreq[2] = (uint64_t)ForumVertexType::GetPtr( (ForumVertexOID) B["Forums"] )->Size();
  BtypeFreq[3] = (uint64_t)PublicationVertexType::GetPtr( (PublicationVertexOID) B["Publications"] )->Size();
  BtypeFreq[4] = (uint64_t)TopicVertexType::GetPtr( (TopicVertexOID) B["Topics"] )->Size();

  L.edgeNumber = 0;
  for (int i = 0; i < 5; i++)     // HAND CODED, five vertex types
    L.edgeNumber += (2 * AtypeFreq[i] * BtypeFreq[i]);

  L.edgeOID = shad::Array<Edge>::Create(L.edgeNumber,Edge())->GetGlobalID();
  L.edgePtr()->FillPtrs();

  Handle handle;
  auto A_Vertices = VertexType::GetPtr((VertexOID) A["Vertices"]);
  auto B_Vertices = VertexType::GetPtr((VertexOID) B["Vertices"]);

  ///// Construct Vertex array for the L Graph
  ///// ... for each vertex in the pattern
  uint64_t offset = 0;
  A_Vertices->AsyncForEachInRange(handle, 0, L.a_num_vertices, create_LVertex, offset, L, BtypeFreq);

  ///// ... for each vertex in the pattern
  offset = L.a_num_vertices;
  B_Vertices->AsyncForEachInRange(handle, 0, L.b_num_vertices, create_LVertex, offset, L, AtypeFreq);

  shad::rt::waitForCompletion(handle);
  exclusiveScanVertices<VertexL>((uint64_t) L.vertexOID);
   
  ///// Construct Edge array for the L Graph
  args_L_t args;
  args.edgeOID = (uint64_t) L.edgeOID;
  args.A_begin = L.vertexPtr()->begin();
  args.A_end   = args.A_begin + L.a_num_vertices;
  args.B_begin = args.A_end;
  args.B_end   = args.B_begin + L.b_num_vertices;

  for (uint64_t i = 0; i < 5; i ++) {     // HAND CODED, five vertex types
    args.type = (TYPES) i;
    shad::rt::asyncExecuteAt(handle, shad::rt::Locality(0), create_LEdges, args);

    std::swap(args.A_begin, args.B_begin);
    std::swap(args.A_end,   args.B_end  );
    shad::rt::asyncExecuteAt(handle, shad::rt::Locality(0), create_LEdges, args);
  }

  waitForCompletion(handle);
}


void setMate(Handle &handle, uint64_t i, VertexL &vertex, uint64_t &vertexOID, uint64_t &edgeOID, uint64_t &offset) {
  auto edgePtr = shad::Array<Edge>::GetPtr((EdgeOID) edgeOID);
  auto vertexPtr = shad::Array<VertexL>::GetPtr((VertexLOID) vertexOID);
  if (vertex.mate >= 0) return;

  uint64_t start = vertexPtr->At(i).edges;
  uint64_t end   = vertexPtr->At(i + 1).edges;
  if ((start - end) <= 0) return;

  int64_t id        = -1;
  int64_t partner   = -1;
  int64_t heavyIndx = -1;
  double weight     = 0.0;
  double heaviest   = 0.0;

  if (start >= 0 && end <= edgePtr->Size()) {
     Handle my_handle;
     std::vector<Edge> neighbors(end-start);

     edgePtr->AsyncGetElements(my_handle, neighbors.data(),start,end-start);
     waitForCompletion(my_handle);

     for (int index = 0; index < neighbors.size(); index++) {
         weight = neighbors[index].weight;
         id = neighbors[index].dst;
         int taken = vertexPtr->At(id).taken;

         if ( (taken == 0) && (weight > 0.0) &&
              ((weight > heaviest) || (weight == heaviest && id > partner )) ) {
              partner   = id;
              heaviest  = weight;
              heavyIndx = start + index;
      }   }

      neighbors.clear();
      vertex.mate = partner;
      vertex.index = heavyIndx;
  } else {
      std::cout << "Trouble: " << i << " " << start << " " << end << std::endl;
} }


void getApproxMatching(GraphL &L) {
  Handle handle;
  auto my_rank=shad::rt::thisLocality();
  auto total_ranks =shad::rt::numLocalities();
  auto Counter = shad::Array<int>::Create(total_ranks, 0);
  auto counterID = Counter->GetGlobalID();
  uint64_t arrayOID=(uint64_t)L.edgeOID;
  uint64_t vertexOID=(uint64_t)L.vertexOID;
  uint32_t my_pos = (uint32_t)my_rank;
    
  /// Matching algorithm is two loops over the vertices

  /// First loop for each vertex u, find  the heaviest edge (u,v) and set the partner of u -> v
  uint64_t iter = 0;

  while (true) {
    iter += 1;

    L.vertexPtr()->AsyncForEachInRange(handle, 0, L.vertexNumber, setMate, vertexOID, arrayOID, L.a_num_vertices);
    waitForCompletion(handle);

    //// Second loop, check whether u -> v and v -> u. If yes then match it. If no then iterate.
    //// DO NOT FORGET TO RESET THE EDGE WEIGHTS AND MATE
    L.vertexPtr()->AsyncForEachInRange(handle, 0, L.vertexNumber,
         [](Handle & handle, size_t i, VertexL & vertex,
            uint64_t &arrayOID, uint64_t &vertexOID, shad::Array<int>::ObjectID &counterID) {
            auto edgePtr=shad::Array<Edge>::GetPtr((EdgeOID)arrayOID);
            auto vertexPtr=shad::Array<VertexL>::GetPtr((VertexLOID)vertexOID);
            auto Counter=shad::Array<int>::GetPtr(counterID);
            int64_t my_id=i; /// 5
            int64_t mate = vertex.mate;
            int64_t index = vertex.index;
            int taken = vertex.taken;
            if (taken == 0 && mate!=-1 && my_id == vertexPtr->At(mate).mate) {
               //// We got a match, RESET edges we need this for multiple matches
               ///L.edgePtr()->At(vertex.index).weight=-1.0;
               //// NOTE: L.edgePtr()->AsyncApply(index,function(), args);
               vertex.taken = 1;
               double val = -1.00;

               edgePtr->AsyncApply(handle, index,
                    [](Handle &, size_t i, Edge &edge, double &val) { edge.weight =- 1.00; }, val);
                    
               Counter->AsyncInsertAt(handle,0,1);

            } else if (taken == 0) vertex.mate = -1;
         },
         arrayOID, vertexOID, counterID
    );

    shad::rt::waitForCompletion(handle);
    int flag = Counter->At(0);

    if (iter == 10) break;
    if (flag == 1) Counter->InsertAt(0,0); else break;
} }


void print_graph(GraphL &L) {
  for (int i = 0; i < L.vertexNumber; i++) {
    VertexL v = L.vertexPtr()->At(i);
    printf("%lu %lu %lu %lu %d %d %d\n", v.id, v.label, v.edges, (uint64_t) v.type, v.mate, v.index, v.taken);
  }

  for (int i = 0; i < L.edgeNumber; i++) {
    Edge e = L.edgePtr()->At(i);
    printf("%lu %lu %lf %lu %lu %lu %lu %lu\n", e.src, e.dst, e.weight,
         (uint64_t) e.type, (uint64_t) e.src_type, (uint64_t) e.dst_type, e.src_glbid, e.dst_glbid);
} }


void reset_weight(GraphL &L) {
  Handle handle;
  L.vertexPtr()->AsyncForEachInRange(handle, 0, L.vertexNumber,
       [](Handle &handle, size_t i, VertexL &vertex, uint64_t &ns) {vertex.taken = 0; vertex.mate = -1;},
       L.a_num_vertices
  );

  shad::rt::waitForCompletion(handle);
}


std::map<uint64_t, uint64_t> get_matching(GraphL &L, int verbose) {
  Handle handle;
  std::map<uint64_t, uint64_t> match;
  std::vector<VertexL> vertex(L.vertexNumber);

    if(verbose==1)
        std::cout << std::endl << "....Matching Extraction...." << std::endl << std::endl;
    L.vertexPtr()->AsyncGetElements(handle,vertex.data(),0,L.vertexNumber);
    shad::rt::waitForCompletion(handle);
    for(int i=0;i<L.a_num_vertices;i++)
    {    
        uint64_t id=vertex[i].label;
        if(vertex[i].taken==1)
        {    
            uint64_t mate=vertex[vertex[i].mate].label;
            match.insert( std::pair<int,int>(id,mate));
            if(verbose ==1)
                std::cout << id << " " << mate << std::endl;
        }
    }
    reset_weight(L);
    return match;
}


void netAlign(std::string & patternFile,std::string & dataFile) {
  std::cout << "Initialization Starts" << std::endl;
  double time1 = my_timer();

  Graph_t A;
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

  A["Persons"]      = (uint64_t) (Persons->GetGlobalID());
  A["ForumEvents"]  = (uint64_t) (ForumEvents->GetGlobalID());
  A["Forums"]       = (uint64_t) (Forums->GetGlobalID());
  A["Publications"] = (uint64_t) (Publications->GetGlobalID());
  A["Topics"]       = (uint64_t) (Topics->GetGlobalID());

  A["Purchases"]    = (uint64_t) (Purchases->GetGlobalID());
  A["Sales"]        = (uint64_t) (Sales->GetGlobalID());
  A["Authors"]      = (uint64_t) (Authors->GetGlobalID());
  A["Includes"]     = (uint64_t) (Includes->GetGlobalID());
  A["HasTopic"]     = (uint64_t) (HasTopic->GetGlobalID());
  A["HasOrg"]       = (uint64_t) (HasOrg->GetGlobalID());
  A["GlobalIDS"]    = (uint64_t) (GlobalIDS->GetGlobalID());

  Graph_t B;
  Persons      = PersonVertexType::Create(MEDIUM);
  ForumEvents  = ForumEventVertexType::Create(MEDIUM);
  Forums       = ForumVertexType::Create(SMALL);
  Publications = PublicationVertexType::Create(SMALL);
  Topics       = TopicVertexType::Create(SMALL);

  Purchases    = PurchaseEdgeType::Create(MEDIUM);
  Sales        = SaleEdgeType::Create(MEDIUM);
  Authors      = AuthorEdgeType::Create(LARGE);
  Includes     = IncludesEdgeType::Create(LARGE);
  HasTopic     = HasTopicEdgeType::Create(LARGE);
  HasOrg       = HasOrgEdgeType::Create(MEDIUM);
  GlobalIDS    = GlobalIDType::Create(LARGE);

  B["Persons"]      = (uint64_t) (Persons->GetGlobalID());
  B["ForumEvents"]  = (uint64_t) (ForumEvents->GetGlobalID());
  B["Forums"]       = (uint64_t) (Forums->GetGlobalID());
  B["Publications"] = (uint64_t) (Publications->GetGlobalID());
  B["Topics"]       = (uint64_t) (Topics->GetGlobalID());

  B["Purchases"]    = (uint64_t) (Purchases->GetGlobalID());
  B["Sales"]        = (uint64_t) (Sales->GetGlobalID());
  B["Authors"]      = (uint64_t) (Authors->GetGlobalID());
  B["Includes"]     = (uint64_t) (Includes->GetGlobalID());
  B["HasTopic"]     = (uint64_t) (HasTopic->GetGlobalID());
  B["HasOrg"]       = (uint64_t) (HasOrg->GetGlobalID());
  B["GlobalIDS"]    = (uint64_t) (GlobalIDS->GetGlobalID());

  uint64_t m, n;
  readFile(patternFile, A);
  CSR(m, n, A);

  std::cout << "File A reading done in " << my_timer() - time1 << " seconds" << std::endl;
  double time2 = my_timer();
  
  readFile(dataFile, B);
  CSR(m, n, B);

  std::cout << "File B reading done in " << my_timer() - time2 << " seconds" << std::endl;
  time2 = my_timer();
    
  GraphL L;
  createBipartite(A,B,L);

  std::cout << "L construction done in " << my_timer() - time2 << " seconds" << std::endl;
  time2 = my_timer();
  // print_graph(L);
  exit(0);
    
///// BP LOGIC
  L.edgePtr()->ForEach(createSquareMatrix, A["Vertices"], A["Edges"], B["Vertices"], B["Edges"], L.a_num_vertices);
  std::cout << "BP done in " << my_timer() - time2 << " seconds" << std::endl;
  time2 = my_timer();

  int top_k = 3;
  std::vector<std::map<uint64_t,uint64_t> >result(top_k);

  for (int i = 0; i < top_k; i++) {
      getApproxMatching(L);
      std::map<uint64_t,uint64_t> match=get_matching(L,1);
      result.push_back(match);
  }

  std::cout << "Matching done in " << my_timer() - time2 << " seconds" << std::endl;
  std::cout << "Total done in " << my_timer() - time1 << " seconds" << std::endl; 
    
} /// NetAlign
} /// namespace
