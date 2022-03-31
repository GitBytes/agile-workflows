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
#include <climits>
#include <random>
#include <string>

#include "shad/data_structures/hashmap.h"
#include "shad/data_structures/multimap.h"
#include "shad/core/algorithm.h"
#include "shad/core/numeric.h"
#include "agile/workflow2/main.h"
#include "agile/workflow2/graph.h"

namespace agile::workflow2{


static int localCounter(0);
/*
struct args_t {
  uint64_t size;
  uint64_t delta;
  shad::Array<uint64_t>::ObjectID arrayOID;
};

// Exclusive scan for SHAD::array
static void exclusiveRecursiveScan(shad::rt::Handle & handle, uint64_t pos, uint64_t & elem, args_t & args) {
    auto arrayPtr = shad::Array<uint64_t>::GetPtr((shad::Array<uint64_t>::ObjectID) args.arrayOID);

    uint64_t delta  = args.delta;
    uint64_t nelems = arrayPtr->getNElems();
    std::vector<uint64_t> * data = arrayPtr->getData();

    // if not the last set, spawn next scan
    // ... next delta is this delta + # edges of last vertex in set 
    if (pos + nelems < args.size) {
       args_t next_args = args;
       next_args.delta += (* data)[nelems - 1];
       arrayPtr->AsyncApply(handle, pos + nelems, exclusiveRecursiveScan, next_args);
    }

    for (uint64_t i = nelems - 1; i > 0; i --)
      (* data)[i] = (* data)[i - 1] + delta;

    (* data)[0] = delta;
}


void exclusiveScanVertices(shad::Array<uint64_t>::ObjectID arrayOID) {
  auto arrayPtr = shad::Array<uint64_t>::GetPtr((shad::Array<uint64_t>::ObjectID)arrayOID);

  auto localInclusiveScan = [](shad::rt::Handle & handle, const shad::Array<uint64_t>::ObjectID & arrayOID) {
    auto arrayPtr = shad::Array<uint64_t>::GetPtr((shad::Array<uint64_t>::ObjectID)arrayOID);
    uint64_t nelems = arrayPtr->getNElems();
    std::vector<uint64_t> * data = arrayPtr->getData();

    for (uint64_t i = 1; i < nelems; i ++) (* data)[i] += (* data)[i - 1];
  };

  shad::rt::Handle handle;
  shad::rt::asyncExecuteOnAll(handle, localInclusiveScan, arrayOID);
  waitForCompletion(handle);

  args_t args = {arrayPtr->Size(), 0, arrayOID};
  arrayPtr->AsyncApply(handle, 0, exclusiveRecursiveScan, args);
  waitForCompletion(handle);
}
*/

struct args_t {
  uint64_t size;
  uint64_t delta;
  uint64_t arrayOID;
};


// Exclusive scan for vertex class array
static void exclusiveRecursiveScan(shad::rt::Handle & handle, uint64_t pos, Vertex & elem, args_t & args) {
    auto arrayPtr = VertexType::GetPtr((VertexOID) args.arrayOID);

    uint64_t delta  = args.delta;
    uint64_t nelems = arrayPtr->getNElems();
    std::vector<Vertex> * data = arrayPtr->getData();

    // if not the last set, spawn next scan
    // ... next delta is this delta + # edges of last vertex in set 
    if (pos + nelems < args.size) {
       args_t next_args = args;
       next_args.delta += (* data)[nelems - 1].edges;
       arrayPtr->AsyncApply(handle, pos + nelems, exclusiveRecursiveScan, next_args);
    }

    for (uint64_t i = nelems - 1; i > 0; i --)
      (* data)[i].edges = (* data)[i - 1].edges + delta;

    (* data)[0].edges = delta;

}


void exclusiveScanVertices(uint64_t arrayOID) {
  auto arrayPtr = VertexType::GetPtr((VertexOID) arrayOID);

  auto localInclusiveScan = [](shad::rt::Handle & handle, const uint64_t & arrayOID) {
    auto arrayPtr = VertexType::GetPtr((VertexOID) arrayOID);
    uint64_t nelems = arrayPtr->getNElems();
    std::vector<Vertex> * data = arrayPtr->getData();

    for (uint64_t i = 1; i < nelems; i ++) (* data)[i].edges += (* data)[i - 1].edges;
  };

  shad::rt::Handle handle;
  shad::rt::asyncExecuteOnAll(handle, localInclusiveScan, arrayOID);
  shad::rt::waitForCompletion(handle);

  args_t args = {arrayPtr->Size(), 0, arrayOID};
  arrayPtr->AsyncApply(handle, 0, exclusiveRecursiveScan, args);
  shad::rt::waitForCompletion(handle);
}


struct indices_args_t {
    Graph_t &A;
    Graph_t &B;
    GraphL &L;
    std::vector<uint64_t> & values;
    int &target;
    int &a_num_vertices;
};
/*
void addIndices(shad::rt::Handle & handle, size_t i, VertexL &val, indices_args_t & args)
{
    int type;
    if(i<args.a_num_vertices)
        type=(int)(VertexType::GetPtr((VertexOID) args.A["Vertices"])->At(i).type);
    else
        type=(int)(VertexType::GetPtr((VertexOID) args.B["Vertices"])->At(i-args.a_num_vertices).type);

    std::vector<Edge> edges;
    for(int j=0;j<args.values.size();j++)
    {
        edges.push_back(Edge((uint64_t)i,(uint64_t)args.values[j],(double)0.0, TYPES::NONE));
    }
    if(type == args.target)
    {
        ///copy the colIndices values in the edge pointer 
        args.L.edgePtr()->AsyncInsertAt(handle, val.edges, edges.data(), edges.size());
        shad::rt::waitForCompletion(handle);
    }

    shad::rt::waitForCompletion(handle);    
}
*/

void createSquareMatrix(Graph_t &A, Graph_t &B, GraphL &L)
{
    shad::rt::Handle handle;
    L.edgePtr()->AsyncForEachInRange(
        handle, 0, L.edgeNumber,
        [](shad::rt::Handle &handle, size_t k, Edge & edge,Graph_t &A, Graph_t &B, GraphL &L)
        {
            
            std::vector<Edge> Aneighbors;
            std::vector<Edge> Bneighbors;
            
            shad::rt::Handle handle1;
            /// get the two end points (i, j)
            uint64_t i=edge.src;
            uint64_t j=edge.dst;
            /// Get degree of i in A and resize nA, then get the neighbors in nA
            uint64_t start=(uint64_t)(VertexType::GetPtr((VertexOID) A["Vertices"])->At(i).edges);
            uint64_t end=(uint64_t)(VertexType::GetPtr((VertexOID) A["Vertices"])->At(i+1).edges);
            
            //// NOTE: SHOULD I DO Async on A->edgePtr() ??
            //// Buffered Get: getElements()...
            ///// NOTE: create another handle
            
            Aneighbors.resize(end-start+1);
            EdgeType::GetPtr((EdgeOID) A["Edges"])->AsyncGetElements(handle1,&Aneighbors[0],start,end-start+1);
            shad::rt::waitForCompletion(handle1);

            
            /// Get degree of j in B and resize nB, then get the neighbors in nB
            start=(uint64_t)(VertexType::GetPtr((VertexOID) B["Vertices"])->At(j).edges);
            end=(uint64_t)(VertexType::GetPtr((VertexOID) B["Vertices"])->At(j+1).edges);
            
            Bneighbors.resize(end-start+1);
            EdgeType::GetPtr((EdgeOID) B["Edges"])->AsyncGetElements(handle1,&Bneighbors[0],start,end-start+1);
            shad::rt::waitForCompletion(handle1);
            

            /*
            for(uint64_t indx=start;indx<end;indx++)
            {
                uint64_t nb=(uint64_t)(EdgeType::GetPtr((EdgeOID) B["Edges"])->At(indx).dst);
                

                nb=(uint64_t)(EdgeType::GetPtr((EdgeOID) B["Edges"])->At(indx).type);
                BneighborsType.push_back((uint64_t)nb);
            }*/
            
            /// two serial for loops nA x nB, and check for (i', j') if it is in L.edge
            /// and update the L.edge weight of (i,j)  

            for(int indx1=0;indx1<Aneighbors.size();indx1++)
                for(int indx2=indx1;indx2<Bneighbors.size();indx2++)
                    if((Aneighbors[indx1].type==Bneighbors[indx2].type) && (Aneighbors[indx1].type==Bneighbors[indx2].type)) ///Edge
                        edge.weight++;
            
        },
    A,B,L);   
    shad::rt::waitForCompletion(handle); 
}


struct args_L_t {
    uint64_t start;
    uint64_t src;
    TYPES type;
    uint64_t typeOID;
    uint64_t a_num_vertices;
    Graph_t A;
    Graph_t B;
    Graph_t G;
    GraphL L;
};

void addIndices(shad::rt::Handle & handle, args_L_t & args)
{
    uint64_t locale = (uint32_t) shad::rt::thisLocality();
    if(locale != shad::rt::numLocalities())
    {
        args_L_t my_args=args;
        

        auto typeID   = PersonVertexType::GetPtr((PersonVertexOID) args.typeOID);
        auto localMap  = typeID->GetLocalHashmap();
        my_args.start = args.start + localMap->Size();;
        

        shad::rt::asyncExecuteAt(handle, shad::rt::Locality(locale + 1), addIndices, my_args);
        
        //// For loop

    }


}

void create_LEdges(size_t i, VertexL &vertex, args_L_t & args)
{
    shad::rt::Handle handle;
    args.start=vertex.edges;
    args.src=i;
    args.type=vertex.type;
    
    args.typeOID=0;
    
    
    if(i < args.a_num_vertices)
        args.G=args.B;
    else
        args.G=args.A;

    shad::rt::asyncExecuteAt(handle, shad::rt::Locality(0), addIndices, args);
}

void createBipartite(Graph_t &A, Graph_t &B, GraphL &L)
{
    
    shad::rt::Handle handle;
    auto numLocality=shad::rt::numLocalities();
    
    auto GlobalIDS_A = GlobalIDType::GetPtr((GlobalIDOID) A["GlobalIDS"]);
    int a_num_vertices = (int)GlobalIDS_A->Size();

    auto GlobalIDS_B = GlobalIDType::GetPtr((GlobalIDOID) B["GlobalIDS"]);
    int b_num_vertices = (int)GlobalIDS_B->Size();

    L.vertexNumber=a_num_vertices+b_num_vertices;
    L.a_num_vertices=a_num_vertices;
    L.vertexOID=shad::Array<VertexL>::Create(L.vertexNumber + 1, VertexL(0,0,0,TYPES::NONE,-1, -1))->GetGlobalID();
    
    
    int typeSize=5; /// Tells the number of vertex type
    
    Freq AtypeFreq, BtypeFreq;
    
    ///// THIS PART IS HAND CODED
    AtypeFreq.counter[0] = (int)PersonVertexType::GetPtr( (PersonVertexOID) A["Persons"] )->Size();
    AtypeFreq.counter[1] = (int)ForumEventVertexType::GetPtr( (ForumEventVertexOID) A["ForumEvents"] )->Size();
    AtypeFreq.counter[2] = (int)ForumVertexType::GetPtr( (ForumVertexOID) A["Forums"] )->Size();
    AtypeFreq.counter[3] = (int)PublicationVertexType::GetPtr( (PublicationVertexOID) A["Publications"] )->Size();
    AtypeFreq.counter[4] = (int)TopicVertexType::GetPtr( (TopicVertexOID) A["Topics"] )->Size();

    BtypeFreq.counter[0] = (int)PersonVertexType::GetPtr( (PersonVertexOID) B["Persons"] )->Size();
    BtypeFreq.counter[1] = (int)ForumEventVertexType::GetPtr( (ForumEventVertexOID) B["ForumEvents"] )->Size();
    BtypeFreq.counter[2] = (int)ForumVertexType::GetPtr( (ForumVertexOID) B["Forums"] )->Size();
    BtypeFreq.counter[3] = (int)PublicationVertexType::GetPtr( (PublicationVertexOID) B["Publications"] )->Size();
    BtypeFreq.counter[4] = (int)TopicVertexType::GetPtr( (TopicVertexOID) B["Topics"] )->Size();


    int count=0;
    for(int i=0;i<typeSize;i++)
        count+=AtypeFreq.counter[i]*BtypeFreq.counter[i];
    
    L.edgeNumber=2*count;
    //L.edgeOID=shad::Array<uint64_t>::Create(L.edgeNumber, 0)->GetGlobalID();
    L.edgeOID=shad::Array<Edge>::Create(L.edgeNumber,Edge())->GetGlobalID();
    //L.edgeWID=shad::Array<float>::Create(L.edgeNumber, 0.0)->GetGlobalID();
    
    ///// For each vertex in the pattern ... 3...

    auto A_Vertices =VertexType::GetPtr((VertexOID) A["Vertices"]);
    A_Vertices->AsyncForEach(handle,
        [](shad::rt::Handle &, size_t i, Vertex &vertex, GraphL &L, Freq &fB)
        {
            shad::rt::Handle handle1;
            size_t pos=i;
            auto count=fB.counter[(uint64_t)vertex.type];
            L.vertexPtr()->AsyncInsertAt(handle1, pos, VertexL(vertex.id, pos, count, vertex.type,-1,-1));
        },
    L,BtypeFreq);

    auto B_Vertices =VertexType::GetPtr((VertexOID) B["Vertices"]);
    B_Vertices->AsyncForEach(handle,
        [](shad::rt::Handle &, size_t i, Vertex &vertex, GraphL &L, Freq &fA)
        {
            shad::rt::Handle handle1;
            auto count=fA.counter[(uint64_t)vertex.type];
            size_t pos=i+L.a_num_vertices;
            L.vertexPtr()->AsyncInsertAt(handle1, pos, VertexL(vertex.id, pos, count, vertex.type,-1,-1));
        },
    L,AtypeFreq);
    
    shad::rt::waitForCompletion(handle);
    
     
    //exclusiveScanVertices(L.vertexOID);     // exclusive scan for the vertex pointer array of L
    exclusiveScanVertices((uint64_t)L.vertexOID);
    /// Now get the all vertices (global ids) of a vertex type


    ///// Adding the  edge pointer array
    args_L_t args;
    args.A=A;
    args.B=B;
    args.L=L;
    args.a_num_vertices=L.a_num_vertices;
    L.vertexPtr()->ForEach(create_LEdges, args);



    
    std::vector<uint64_t> colIndices; /// SHAD ARRAY <GLBID> #Number of Persons in DATA 
    
    ///// Get the neighbor and do a serial search
    auto Persons      = PersonVertexType::GetPtr( (PersonVertexOID) B["Persons"] );
    auto ForumEvents  = ForumEventVertexType::GetPtr( (ForumEventVertexOID) B["ForumEvents"] );
    auto Forums       = ForumVertexType::GetPtr( (ForumVertexOID) B["Forums"] );
    auto Publications = PublicationVertexType::GetPtr( (PublicationVertexOID) B["Publications"] );
    auto Topics       = TopicVertexType::GetPtr( (TopicVertexOID) B["Topics"] );


    ////////////////// Persons
    int target=0;
    Persons->AsyncForEachEntry(handle, 
        [](shad::rt::Handle & handle, const uint64_t & key, PersonVertex & vertex, std::vector<uint64_t >& vec) 
        {
            vec.push_back(vertex.GLBID);
        },
        colIndices); /// I have global ids of all person vertices
    indices_args_t ind_args={A,B,L,colIndices,target,a_num_vertices};
    L.vertexPtr()->AsyncForEachInRange(handle, 0, a_num_vertices, addIndices,ind_args);
    shad::rt::waitForCompletion(handle);
    colIndices.clear();
    ////////////////////////////////

    ////////////////// Forum Event
    //col_args = {colIndices};
    target=1;
    ForumEvents->AsyncForEachEntry(handle, 
        [](shad::rt::Handle & handle, const uint64_t & key, ForumEventVertex & vertex, std::vector<uint64_t >& vec) 
        {
            vec.push_back(vertex.GLBID);
        },
        colIndices); /// I have global ids of all ForumEvent vertices
    ind_args.target=target;
    L.vertexPtr()->AsyncForEachInRange(handle, 0, a_num_vertices, addIndices,ind_args);
    shad::rt::waitForCompletion(handle);
    colIndices.clear();
    ////////////////////////////////

    ////////////////// Forums
    //col_args = {colIndices};
    target=2;
    Forums->AsyncForEachEntry(handle, 
        [](shad::rt::Handle & handle, const uint64_t & key, ForumVertex & vertex, std::vector<uint64_t >& vec) 
        {
            vec.push_back(vertex.GLBID);
        },
        colIndices); /// I have global ids of all ForumEvent vertices
    ind_args.target=target;
    L.vertexPtr()->AsyncForEachInRange(handle, 0, a_num_vertices, addIndices,ind_args);
    shad::rt::waitForCompletion(handle);
    colIndices.clear();
    ////////////////////////////////

    ////////////////// Publication 
    //col_args = {colIndices};
    target=3;
    Publications->AsyncForEachEntry(handle, 
        [](shad::rt::Handle & handle, const uint64_t & key, PublicationVertex & vertex, std::vector<uint64_t >& vec) 
        {
            vec.push_back(vertex.GLBID);
        },
        colIndices); /// I have global ids of all ForumEvent vertices
    ind_args.target=target;
    L.vertexPtr()->AsyncForEachInRange(handle, 0, a_num_vertices, addIndices,ind_args);
    shad::rt::waitForCompletion(handle);
    colIndices.clear();
    ////////////////////////////////

    ////////////////// Topic 
    //col_args = {colIndices};
    target=4;
    Topics->AsyncForEachEntry(handle, 
        [](shad::rt::Handle & handle, const uint64_t & key, TopicVertex & vertex, std::vector<uint64_t >& vec) 
        {
            vec.push_back(vertex.GLBID);
        },
        colIndices); /// I have global ids of all ForumEvent vertices
    ind_args.target=target;
    L.vertexPtr()->AsyncForEachInRange(handle, 0, a_num_vertices, addIndices,ind_args);
    shad::rt::waitForCompletion(handle);
    colIndices.clear();
    ////////////////////////////////

    /************************* NOW DO IT FROM B to A *****************/
    ///// Get the neighbor and do a serial search
    Persons      = PersonVertexType::GetPtr( (PersonVertexOID) A["Persons"] );
    ForumEvents  = ForumEventVertexType::GetPtr( (ForumEventVertexOID) A["ForumEvents"] );
    Forums       = ForumVertexType::GetPtr( (ForumVertexOID) A["Forums"] );
    Publications = PublicationVertexType::GetPtr( (PublicationVertexOID) A["Publications"] );
    Topics       = TopicVertexType::GetPtr( (TopicVertexOID) A["Topics"] );


    ////////////////// Persons
    target=0;
    Persons->AsyncForEachEntry(handle, 
        [](shad::rt::Handle & handle, const uint64_t & key, PersonVertex & vertex, std::vector<uint64_t >& vec) 
        {
            vec.push_back(vertex.GLBID);
        },
        colIndices); /// I have global ids of all person vertices
    ind_args.target=target;
    L.vertexPtr()->AsyncForEachInRange(handle, a_num_vertices, b_num_vertices, addIndices,ind_args);
    shad::rt::waitForCompletion(handle);
    colIndices.clear();
    ////////////////////////////////

    ////////////////// Forum Event
    //col_args = {colIndices};
    target=1;
    ForumEvents->AsyncForEachEntry(handle, 
        [](shad::rt::Handle & handle, const uint64_t & key, ForumEventVertex & vertex, std::vector<uint64_t >& vec) 
        {
            vec.push_back(vertex.GLBID);
        },
        colIndices); /// I have global ids of all ForumEvent vertices
    ind_args.target=target;
    L.vertexPtr()->AsyncForEachInRange(handle, a_num_vertices, b_num_vertices, addIndices,ind_args);
    shad::rt::waitForCompletion(handle);
    colIndices.clear();
    ////////////////////////////////

    ////////////////// Forums
    //col_args = {colIndices};
    target=2;
    Forums->AsyncForEachEntry(handle, 
        [](shad::rt::Handle & handle, const uint64_t & key, ForumVertex & vertex, std::vector<uint64_t >& vec) 
        {
            vec.push_back(vertex.GLBID);
        },
        colIndices); /// I have global ids of all ForumEvent vertices
    ind_args.target=target;
    L.vertexPtr()->AsyncForEachInRange(handle, a_num_vertices, b_num_vertices, addIndices,ind_args);
    shad::rt::waitForCompletion(handle);
    colIndices.clear();
    ////////////////////////////////

    ////////////////// Publication 
    //col_args = {colIndices};
    target=3;
    Publications->AsyncForEachEntry(handle, 
        [](shad::rt::Handle & handle, const uint64_t & key, PublicationVertex & vertex, std::vector<uint64_t >& vec) 
        {
            vec.push_back(vertex.GLBID);
        },
        colIndices); /// I have global ids of all ForumEvent vertices
    ind_args.target=target;
    L.vertexPtr()->AsyncForEachInRange(handle, a_num_vertices, b_num_vertices, addIndices,ind_args);
    shad::rt::waitForCompletion(handle);
    colIndices.clear();
    ////////////////////////////////

    ////////////////// Topic 
    //col_args = {colIndices};
    target=4;
    Topics->AsyncForEachEntry(handle, 
        [](shad::rt::Handle & handle, const uint64_t & key, TopicVertex & vertex, std::vector<uint64_t >& vec) 
        {
            vec.push_back(vertex.GLBID);
        },
        colIndices); /// I have global ids of all ForumEvent vertices
    ind_args.target=target;
    L.vertexPtr()->AsyncForEachInRange(handle, a_num_vertices, b_num_vertices, addIndices,ind_args);
    shad::rt::waitForCompletion(handle);
    colIndices.clear();
    ////////////////////////////////
}

/*

*/


void getApproxMatching(GraphL &L,shad::Array<int>::ObjectID &mateID)
{
    
    shad::rt::Handle handle;

    auto my_rank=shad::rt::thisLocality();
    auto total_ranks =shad::rt::numLocalities();
    auto counter = shad::Array<int>::Create(total_ranks, 0);
    auto counterID = counter->GetGlobalID();
    
    ///// I need a shared array on uint64 of size num_locality
    
    /// Matching algorithm is two loops over the vertices

    /// First loop for each vertex u, find  the heaviest edge (u,v) and set the partner of u -> v
    while(true)
    {
        
        L.vertexPtr()->AsyncForEach(handle,
            [](shad::rt::Handle &, size_t i, VertexL &vertex, GraphL &L)
            {
                if(vertex.mate > 0)
                {    
                    uint64_t start=(uint64_t)L.vertexPtr()->At(i).edges;
                    uint64_t end=(uint64_t)L.vertexPtr()->At(i+1).edges;
                    
                    uint64_t partner=-1;
                    uint64_t id=-1;
                    uint64_t heavyIndx=-1;
                    double heaviest=0.0;
                    double weight=0.0;
                    Edge edge;

                    //// NOTE: SHOULD I DO Async on L->edgePtr() ??
                    for(int indx=start;indx<end;indx++)
                    {
                        edge=L.edgePtr()->At(indx);
                        weight=edge.weight;
                        //weight=L.edgePtr()->At(indx).weight;
                        id=edge.dst;
                        //id=L.edgePtr()->At(indx).dst;
                        if((weight > 0.0) &&( weight > heaviest) || (weight == heaviest && id > partner ))
                        {
                            partner=id;
                            heaviest=weight;
                            heavyIndx=indx;
                        }
                            
                    }
                    vertex.mate=partner;
                    vertex.indx=heavyIndx;
                }
            },
        L);

        shad::rt::waitForCompletion(handle);

        //// Second loop, check whether u -> v and v -> u. If yes then match it. If no then iterate.
        //// DO NOT FORGET TO RESET THE EDGE WEIGHTS AND MATE
        L.vertexPtr()->AsyncForEach(handle,
            [](shad::rt::Handle &handle, size_t i, VertexL &vertex, GraphL &L, int &localCounter)
            {
                
                uint64_t my_id=vertex.id; /// 5
                uint64_t indx = vertex.indx;
                if(my_id == L.vertexPtr()->At(vertex.mate).mate)
                {
                    //// We got a match, RESET edges we need this for multiple matches
                    ///L.edgePtr()->At(vertex.indx).weight=-1.0;
                    //// NOTE: L.edgePtr()->AsyncApply(indx,function(), args);
                    shad::rt::Handle handle1;
                    double val=-1.00;
                    L.edgePtr()->AsyncApply(handle1, indx,
                        [](shad::rt::Handle &, size_t i, Edge &edge, double &val)
                        {
                            edge.weight=val;
                        },
                    val);
                    shad::rt::waitForCompletion(handle1);
                    
                    localCounter++;
                }
                else
                    vertex.mate=-1;
            },
        L, localCounter);

        shad::rt::waitForCompletion(handle);

        
        /////// This reducer is copied from the SHAD PNNL GIT examples
        // This performs a reduction into a single counter.
        std::vector<double> reducer(shad::rt::numLocalities());

        for (auto &locality : shad::rt::allLocalities()) {
        shad::rt::asyncExecuteAtWithRet(
            handle, locality,
            [](shad::rt::Handle &, const size_t &, double *value) {
                *value = localCounter;
            },
            size_t(0), &reducer[static_cast<uint32_t>(locality)]);
        }
        shad::rt::waitForCompletion(handle);

        for (double i : reducer) localCounter += i;

        if (localCounter == 0) break;  ///// Break

        shad::rt::asyncExecuteOnAll(
            handle, [](shad::rt::Handle &, const size_t &) { localCounter = 0; },
            size_t(0));

        shad::rt::waitForCompletion(handle);


    }

    //// Now get the matching and reset the mates
    ///// NOTE: SHOULD we do HANDLE 1 ??
    L.vertexPtr()->AsyncForEach(handle,
        [](shad::rt::Handle &handle, size_t i, VertexL &vertex, shad::Array<int>::ObjectID &mateID, uint64_t &ns)
        {
            auto mate = shad::Array<int>::GetPtr(mateID);
            shad::rt::Handle handle1;
            if(vertex.mate != -1 && i < ns)
                mate->AsyncInsertAt(handle1,i, vertex.mate);
            
            shad::rt::waitForCompletion(handle1);
            vertex.mate=-1;
        },
    mateID,L.a_num_vertices);
    shad::rt::waitForCompletion(handle);
}

void netAlign(uint64_t argc, char* argv[])
{

    /*
    netalign_parameters opts;
    if (!opts.parse(argc, argv)) {
        return;
    }
    
    */

    auto my_rank=shad::rt::thisLocality();
    auto total_ranks =shad::rt::numLocalities();

    std::string basename = "test";
    std::string Afilename = basename + "-A.mtx";
    std::string Bfilename = basename + "-B.mtx";
    
    
    double time1,time2,time3,time4,time5,time6,time7,temp,timet;
    double objective=-1000000,zero=0.0;
   
    std::cout<<"Initialization Starts"<<std::endl;
    time1=my_timer(); 
    
    Graph_t A;
    Graph_t B;
    uint64_t  m,n;
    readFile( Afilename, A);
    CSR(m, n, A);
    std::cout<<"File A reading done..!!"<<std::endl;
    time2=my_timer();

    readFile( Bfilename, B);
    CSR(m, n, B);
    std::cout<<"File B reading done..!!"<<std::endl;
    time3=my_timer();

    GraphL L;
    createBipartite(A,B,L);
    std::cout<<"L construction done..!!"<<std::endl;
    time4=my_timer();
    
    ///// BP LOGIC
    createSquareMatrix(A,B,L);
    time5=my_timer();

    ///// Matching
    auto mate = shad::Array<int>::Create(L.a_num_vertices, -1);
    auto mateID = mate->GetGlobalID();
    
    getApproxMatching(L,mateID);
    
    //// I need to get the matching 
    std::vector<int> match;
    /*
    if(my_rank==0)
        for(int i=0;i<L.a_num_vertices;i++)
            match.push_back(mate->At(i));
    */
    time6=my_timer();
    
    
} /// NetAlign
} /// namespace