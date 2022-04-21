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
#include "agile/workflow2/csr.h"

namespace agile::workflow2{


static int localCounter(0);

struct args_t {
  uint64_t size;
  uint64_t delta;
  uint64_t arrayOID;
};


// Exclusive scan for vertex class array
static void exclusiveRecursiveScan(shad::rt::Handle & handle, uint64_t pos, VertexL & elem, args_t & args) {
    //auto arrayPtr = VertexType::GetPtr((VertexOID) args.arrayOID);
    auto arrayPtr = shad::Array<VertexL>::GetPtr((shad::Array<VertexL>::ObjectID) args.arrayOID);

    uint64_t delta  = args.delta;
    uint64_t nelems = arrayPtr->getNElems();
    std::vector<VertexL> * data = arrayPtr->getData();

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


void exclusiveScanVerticesL(uint64_t arrayOID) {
  //auto arrayPtr = VertexType::GetPtr((VertexOID) arrayOID);
  auto arrayPtr = shad::Array<VertexL>::GetPtr((shad::Array<VertexL>::ObjectID) arrayOID);

  auto localInclusiveScan = [](shad::rt::Handle & handle, const uint64_t & arrayOID) {
    auto arrayPtr = shad::Array<VertexL>::GetPtr((shad::Array<VertexL>::ObjectID) arrayOID);
    uint64_t nelems = arrayPtr->getNElems();
    std::vector<VertexL> * data = arrayPtr->getData();

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
        edges.push_back(Edge((uint64_t)i,(uint64_t)args.values[j],(double)0.0, TYPES::NONE,TYPES::NONE,TYPES::NONE,0,0));
    }
    if(type == args.target)
    {
        ///copy the colIndices values in the edge pointer 
        args.L.edgePtr()->AsyncInsertAt(handle, val.edges, edges.data(), edges.size());
        shad::rt::waitForCompletion(handle);
    }

    shad::rt::waitForCompletion(handle);    
}

struct args_S_t {
    uint64_t vertexOID_A;
    uint64_t edgeOID_A;
    uint64_t vertexOID_B;
    uint64_t edgeOID_B;
    uint64_t a_num_vertices;
};

void createSquareMatrix(size_t k, Edge &edge, args_S_t &args)
{
    
    shad::rt::Handle handle;
    auto A_Vertices =VertexType::GetPtr((VertexOID) args.vertexOID_A);
    auto B_Vertices =VertexType::GetPtr((VertexOID) args.vertexOID_B);

    auto A_Edges =EdgeType::GetPtr((EdgeOID) args.edgeOID_A);
    auto B_Edges =EdgeType::GetPtr((EdgeOID) args.edgeOID_B);

    uint64_t start,end,start1,end1;
    
    /// get the two end points (i, j)
    uint64_t i=edge.src;
    uint64_t j=edge.dst;
    
    edge.weight+=1;  /// Initial weight
    
    //std::cout<<k<<": "<<i<<" , "<<j<<std::endl;
    /// Get degree of i in A and resize nA, then get the neighbors in nA
    //std::cout<<i<<" "<<std::endl;
    if(i<args.a_num_vertices)
    {
        start=(A_Vertices->At(i)).edges;
        end=(A_Vertices->At(i+1)).edges;

        start1=(B_Vertices->At(j-args.a_num_vertices)).edges;
        end1=(B_Vertices->At(j+1-args.a_num_vertices)).edges;

        if((end-start)>0 && (end1-start1)>0)
        {    
            std::vector<Edge> Aneighbors(end-start); 
            A_Edges->AsyncGetElements(handle,Aneighbors.data(),start,end-start);
            shad::rt::waitForCompletion(handle);

            /// Get degree of j in B and resize nB, then get the neighbors in nB
            std::vector<Edge> Bneighbors(end1-start1);
            B_Edges->AsyncGetElements(handle,Bneighbors.data(),start1,end1-start1);
            shad::rt::waitForCompletion(handle);

            for(int indx1=0;indx1<Aneighbors.size();indx1++)
            {    
                for(int indx2=indx1;indx2<Bneighbors.size();indx2++)
                {    
                    //std::cout<<Bneighbors[indx2].src<<" || "<<Bneighbors[indx2].dst<<" "<<(int)Bneighbors[indx2].type<<std::endl;
                    if((Aneighbors[indx1].dst_type==Bneighbors[indx2].dst_type) && (Aneighbors[indx1].type==Bneighbors[indx2].type)) ///Edge
                    {    
                        edge.weight++;
                    }
                }
            }
        }
    }
    else
    {
        start=(A_Vertices->At(j)).edges;
        end=(A_Vertices->At(j+1)).edges;

        start1=(B_Vertices->At(i-args.a_num_vertices)).edges;
        end1=(B_Vertices->At(i+1-args.a_num_vertices)).edges;

        if((end-start)>0 && (end1-start1)>0)
        {    
            std::vector<Edge> Aneighbors(end-start); 
            A_Edges->AsyncGetElements(handle,Aneighbors.data(),start,end-start);
            shad::rt::waitForCompletion(handle);
        
            /// Get degree of j in B and resize nB, then get the neighbors in nB
            std::vector<Edge> Bneighbors(end1-start1);
            B_Edges->AsyncGetElements(handle,Bneighbors.data(),start1,end1-start1);
            shad::rt::waitForCompletion(handle);

            for(int indx1=0;indx1<Aneighbors.size();indx1++)
                for(int indx2=indx1;indx2<Bneighbors.size();indx2++)
                    if((Aneighbors[indx1].dst_type==Bneighbors[indx2].dst_type) && (Aneighbors[indx1].type==Bneighbors[indx2].type)) ///Edge
                        edge.weight++;
        }
        
    }

    //shad::rt::waitForCompletion(handle); 
   
}

struct args_L_t {
    uint64_t GlobalOID_A;
    uint64_t GlobalOID_B;
    uint64_t edgesOID_L;
    
    uint64_t side;
    uint64_t start;
    uint64_t src;
    TYPES type;
    uint64_t a_num_vertices;
    uint64_t num_vertices;
    uint64_t num_edges;
    uint64_t offset;
    
};

void addIndices_L(shad::rt::Handle & handle, const args_L_t & args)
{
    uint64_t locale = (uint32_t) shad::rt::thisLocality();
    uint64_t dst=0;
    args_L_t my_args={args.GlobalOID_A,args.GlobalOID_B,args.edgesOID_L,args.side,
        args.start,args.src,args.type,args.a_num_vertices,args.num_vertices,args.num_edges,args.offset};
    uint64_t my_start=args.start;
    
    auto edgePtr=shad::Array<Edge>::GetPtr((EdgeOID)args.edgesOID_L);
    
    if(args.side==1)
    {
        auto GlobalIDS = GlobalIDType::GetPtr((GlobalIDOID) args.GlobalOID_B);
        auto localMap  = GlobalIDS->GetLocalHashmap();

        for(auto it = GlobalIDS->local_begin(); it != GlobalIDS->local_end(); it++ )
        {
            if((*it).second.type==args.type)
            {
                uint64_t dst=(*it).second.id+args.offset;
                edgePtr->AsyncInsertAt(handle, my_start, Edge(args.src,dst,0.0,TYPES::NONE,args.type,args.type,0,0));
                //edgePtr->BufferedAsyncInsertAt(handle, my_start, Edge(args.src,dst,0.0,TYPES::NONE,args.type,args.type));
                my_start++; 
            }
        }
        my_args.start=my_start;
    }
    else
    {
        auto GlobalIDS = GlobalIDType::GetPtr((GlobalIDOID) args.GlobalOID_A);
        auto localMap  = GlobalIDS->GetLocalHashmap();

        for(auto it = GlobalIDS->local_begin(); it != GlobalIDS->local_end(); it++ )
        {
            if((*it).second.type==args.type)
            {
                uint64_t dst=(*it).second.id+args.offset;
                edgePtr->AsyncInsertAt(handle, my_start, Edge(args.src,dst,0.0,TYPES::NONE,args.type,args.type,0,0));
                //edgePtr->BufferedAsyncInsertAt(handle, my_start, Edge(args.src,dst,0.0,TYPES::NONE,args.type,args.type));
                my_start++; 
            }
        }
        my_args.start=my_start;
    }
   
    if(locale < shad::rt::numLocalities() - 1)
        shad::rt::asyncExecuteAt(handle, shad::rt::Locality(locale + 1), addIndices_L, my_args);

}


void create_LEdges(uint64_t i, VertexL &vertex, args_L_t &args)
{
    shad::rt::Handle handle;
    args_L_t my_args={args.GlobalOID_A,args.GlobalOID_B,args.edgesOID_L,args.side,
        args.start,args.src,args.type,args.a_num_vertices,args.num_vertices,args.num_edges,args.offset};
    my_args.start=vertex.edges;
    my_args.src=i;
    my_args.type=vertex.type;
      
    
    if(i < args.a_num_vertices)
    {
        my_args.side=1;
        my_args.offset=args.a_num_vertices;
    }
    else
    {
        if(i < args.num_vertices)
        {    
            my_args.side=0;
            my_args.offset=0;
        }
    }
    
    //std::cout<<"Start "<<i<<std::endl;
    
    shad::rt::asyncExecuteAt(handle, shad::rt::Locality(0), addIndices_L, my_args);
    
    //auto edgePtr=shad::Array<Edge>::GetPtr((EdgeOID)args.edgesOID_L);
    //edgePtr->WaitForBufferedInsert();

    shad::rt::waitForCompletion(handle);
    
    //std::cout<<"end "<<i<<std::endl;
}

void printing(size_t i,Vertex& vertex)
{
    std::cout<<"Vertex: "<<vertex.id<<" "<<(uint64_t)vertex.type<<std::endl;
}

struct args_t_t {
    uint64_t arrayOID;
    uint64_t edgeOID;
    uint64_t offset;
    
};


void create_edges(uint64_t i, VertexL& vertex, args_t_t &args)
{

    shad::rt::Handle handle;
    auto vertexPtr=shad::Array<VertexL>::GetPtr((VertexLOID)args.arrayOID);
    auto edgePtr =EdgeType::GetPtr((EdgeOID) args.edgeOID);
    uint64_t start,end,index;
    if (i<args.offset)
    {
        start=args.offset;
        end=vertexPtr->Size();
    }
    else
    {
        start=0;
        end=args.offset;
    }
    
    //vertexPtr->ForEach(innerLoop,my_args);
    index=vertex.edges;

}
void createBipartite(Graph_t &A, Graph_t &B, GraphL &L)
{
    
    
    auto numLocality=shad::rt::numLocalities();
    
    auto GlobalIDS_A = GlobalIDType::GetPtr((GlobalIDOID) A["GlobalIDS"]);
    int a_num_vertices = (int)GlobalIDS_A->Size();

    auto GlobalIDS_B = GlobalIDType::GetPtr((GlobalIDOID) B["GlobalIDS"]);
    int b_num_vertices = (int)GlobalIDS_B->Size();

    L.vertexNumber=a_num_vertices+b_num_vertices;
    L.a_num_vertices=a_num_vertices;
    L.vertexOID=shad::Array<VertexL>::Create(L.vertexNumber + 1, VertexL(0,0,0,TYPES::NONE,-1, -1, 0))->GetGlobalID();
    
    L.vertexPtr()->FillPtrs();
    //std::cout<<"L Vertices: "<<L.vertexNumber<<" "<<L.a_num_vertices<<std::endl;
    
    int typeSize=5; /// Tells the number of vertex type
    
    Freq AtypeFreq, BtypeFreq;
    
    ///// THIS PART IS HAND CODED
    AtypeFreq.counter[0] = (uint64_t)PersonVertexType::GetPtr( (PersonVertexOID) A["Persons"] )->Size();
    AtypeFreq.counter[1] = (uint64_t)ForumEventVertexType::GetPtr( (ForumEventVertexOID) A["ForumEvents"] )->Size();
    AtypeFreq.counter[2] = (uint64_t)ForumVertexType::GetPtr( (ForumVertexOID) A["Forums"] )->Size();
    AtypeFreq.counter[3] = (uint64_t)PublicationVertexType::GetPtr( (PublicationVertexOID) A["Publications"] )->Size();
    AtypeFreq.counter[4] = (uint64_t)TopicVertexType::GetPtr( (TopicVertexOID) A["Topics"] )->Size();

    BtypeFreq.counter[0] = (uint64_t)PersonVertexType::GetPtr( (PersonVertexOID) B["Persons"] )->Size();
    BtypeFreq.counter[1] = (uint64_t)ForumEventVertexType::GetPtr( (ForumEventVertexOID) B["ForumEvents"] )->Size();
    BtypeFreq.counter[2] = (uint64_t)ForumVertexType::GetPtr( (ForumVertexOID) B["Forums"] )->Size();
    BtypeFreq.counter[3] = (uint64_t)PublicationVertexType::GetPtr( (PublicationVertexOID) B["Publications"] )->Size();
    BtypeFreq.counter[4] = (uint64_t)TopicVertexType::GetPtr( (TopicVertexOID) B["Topics"] )->Size();



    uint64_t count=0;
    for(int i=0;i<typeSize;i++)
    {    
        count=count+(AtypeFreq.counter[i]*BtypeFreq.counter[i]);
        //std::cout<<AtypeFreq.counter[i]<<" "<<BtypeFreq.counter[i]<<std::endl;
    }
    L.edgeNumber=2*count;
    //std::cout<<"L Edges: "<<L.edgeNumber<<std::endl;
    //L.edgeOID=shad::Array<uint64_t>::Create(L.edgeNumber, 0)->GetGlobalID();
    L.edgeOID=shad::Array<Edge>::Create(L.edgeNumber,Edge())->GetGlobalID();
    L.edgePtr()->FillPtrs();
    //L.edgeWID=shad::Array<float>::Create(L.edgeNumber, 0.0)->GetGlobalID();
    
    ///// For each vertex in the pattern ... .
    
    shad::rt::Handle handle;
    auto A_Vertices =VertexType::GetPtr((VertexOID) A["Vertices"]);
    //A_Vertices->ForEach(printing);
        
    A_Vertices->AsyncForEach(handle,
        [](shad::rt::Handle &handle, size_t i, Vertex &vertex, GraphL &L, Freq &fB)
        {
            
            size_t pos=i;
            if(pos < L.a_num_vertices)
            {
                //shad::rt::Handle handle1;
                
                auto count=fB.counter[(uint64_t)vertex.type];
                L.vertexPtr()->AsyncInsertAt(handle, pos, VertexL(vertex.id, pos, count, vertex.type,-1,-1, 0));
                //shad::rt::waitForCompletion(handle1);
            }
        },
    L,BtypeFreq);
    
    shad::rt::waitForCompletion(handle);
    //shad::rt::waitForCompletion(handle1);

    auto B_Vertices =VertexType::GetPtr((VertexOID) B["Vertices"]);
    B_Vertices->AsyncForEach(handle,
        [](shad::rt::Handle &handle, size_t i, Vertex &vertex, GraphL &L, Freq &fA)
        {
            size_t pos=i+L.a_num_vertices;
            if(pos<L.vertexNumber)
            {
                auto count=fA.counter[(uint64_t)vertex.type];

                L.vertexPtr()->AsyncInsertAt(handle, pos, VertexL(vertex.id, pos, count, vertex.type,-1,-1,0));
                //shad::rt::waitForCompletion(handle1);
            }
        },
    L,AtypeFreq);
    //shad::rt::waitForCompletion(handle1);
    shad::rt::waitForCompletion(handle);
    
     
    //exclusiveScanVertices(L.vertexOID);     // exclusive scan for the vertex pointer array of L
    exclusiveScanVerticesL((uint64_t)L.vertexOID);
    /// Now get the all vertices (global ids) of a vertex type

   
    ///// Adding the  edge pointer array
    args_L_t args={A["GlobalIDS"],B["GlobalIDS"],(uint64_t)L.edgeOID,0,0,0,TYPES::NONE,0,0,0,0};
    args.a_num_vertices=L.a_num_vertices;
    args.num_vertices=L.vertexNumber;
    args.num_edges=L.edgeNumber;
    L.vertexPtr()->ForEach(create_LEdges, args);

    //args_t_t args ={(uint64_t)L.vertexOID,(uint64_t)L.edgeOID,L.a_num_vertices};
    //L.vertexPtr()->ForEach(create_edges, args);
    /*
    std::cout<<L.edgePtr()->At(1).src<<" "<<L.edgePtr()->At(1).dst<<std::endl;
    std::cout<<L.edgePtr()->At(53289).src<<" "<<L.edgePtr()->At(53289).dst<<std::endl;
    std::cout<<L.edgePtr()->At(8313760).src<<" "<<L.edgePtr()->At(8313760).dst<<std::endl;
    std::cout<<L.edgePtr()->At(8313780).src<<" "<<L.edgePtr()->At(8313780).dst<<std::endl;
    */
}

struct args_M_t {
    uint64_t arrayOID;
    uint64_t edgeOID;
    uint64_t offset;
    uint64_t num_vertex;
    
};

//void setMate(uint64_t i, VertexL &vertex, args_M_t &args)
void setMate(shad::rt::Handle & handle,uint64_t i, VertexL &vertex, args_M_t &args)
{
    if(i<args.num_vertex)
    {
        auto edgePtr=shad::Array<Edge>::GetPtr((EdgeOID)args.edgeOID);
        auto vertexPtr=shad::Array<VertexL>::GetPtr((VertexLOID)args.arrayOID);
        if(vertex.mate < 0)
        {    
            uint64_t start=vertexPtr->At(i).edges;
            uint64_t end=vertexPtr->At(i+1).edges;
            
            if((start-end)>0)
            {
                int64_t partner=-1;
                int64_t id=-1;
                int64_t heavyIndx=-1;
                double heaviest=0.0;
                double weight=0.0;

                if(start>=0 && end<=edgePtr->Size())
                {
                    shad::rt::Handle handle1;
                    std::vector<Edge> neighbors(end-start); 
                    //std::cout<<"B Trouble: "<<i<<" "<<start<<" "<<end<<" "<<edgePtr->Size()<<std::endl;
                    edgePtr->AsyncGetElements(handle1,neighbors.data(),start,end-start);
                    shad::rt::waitForCompletion(handle1);
                    //std::cout<<"A Trouble: "<<i<<" "<<start<<" "<<end<<" "<<edgePtr->Size()<<std::endl;

                    //// NOTE: SHOULD I DO Async on L->edgePtr() ??
                    
                    for(int indx=0;indx<neighbors.size();indx++)
                    {
                        
                        weight=neighbors[indx].weight;
                        id=neighbors[indx].dst;
                        //std::cout<<weight<<" | ("<<i<<","<<id<<")"<<std::endl;
                        int taken=vertexPtr->At(id).taken;
                        if((taken == 0) && (weight > 0.0) && (( weight > heaviest) || (weight == heaviest && id > partner )))
                        {
                            partner=id;
                            heaviest=weight;
                            heavyIndx=start+indx;
                        }
                            
                    }
                    neighbors.clear(); 
                    
                    vertex.mate=partner;
                    vertex.indx=heavyIndx;
                    //std::cout<<"P: "<<i<<"->"<<vertex.mate<<" "<<heaviest<<" "<<vertex.indx<<std::endl;
                    
                }
                else std::cout<<"Trouble: "<<i<<" "<<start<<" "<<end<<std::endl;
            }
        }
    }
    

}

void getApproxMatching(GraphL &L)
{
    
    shad::rt::Handle handle;

    auto my_rank=shad::rt::thisLocality();
    auto total_ranks =shad::rt::numLocalities();
    auto Counter = shad::Array<int>::Create(total_ranks, 0);
    auto counterID = Counter->GetGlobalID();
    uint64_t arrayOID=(uint64_t)L.edgeOID;
    uint64_t vertexOID=(uint64_t)L.vertexOID;
    uint32_t my_pos = (uint32_t)my_rank;
    
    /// Matching algorithm is two loops over the vertices

    /// First loop for each vertex u, find  the heaviest edge (u,v) and set the partner of u -> v
    uint64_t iter=0;
    while(true)
    {
        iter+=1;
        //std::cout<<"Iteration: "<<iter<<std::endl;
        args_M_t args={vertexOID,arrayOID,L.a_num_vertices,L.vertexNumber};

        L.vertexPtr()->AsyncForEach(handle,setMate,args);
        shad::rt::waitForCompletion(handle);

        //std::cout<<"Mate"<<std::endl;
        //// Second loop, check whether u -> v and v -> u. If yes then match it. If no then iterate.
        //// DO NOT FORGET TO RESET THE EDGE WEIGHTS AND MATE
        L.vertexPtr()->AsyncForEachInRange(handle,0,L.vertexNumber,
            [](shad::rt::Handle &handle, size_t i, VertexL &vertex, uint64_t &arrayOID, uint64_t &vertexOID, shad::Array<int>::ObjectID &counterID)
            {
                auto edgePtr=shad::Array<Edge>::GetPtr((EdgeOID)arrayOID);
                auto vertexPtr=shad::Array<VertexL>::GetPtr((VertexLOID)vertexOID);
                auto Counter=shad::Array<int>::GetPtr(counterID);
                int64_t my_id=i; /// 5
                int64_t indx = vertex.indx;
                int64_t mate = vertex.mate;
                int taken = vertex.taken;
                if(taken == 0 && mate!=-1 && my_id == vertexPtr->At(mate).mate)
                {
                    //// We got a match, RESET edges we need this for multiple matches
                    ///L.edgePtr()->At(vertex.indx).weight=-1.0;
                    //// NOTE: L.edgePtr()->AsyncApply(indx,function(), args);
                    //shad::rt::Handle handle1;
                    vertex.taken=1;
                    double val=-1.00;
                    edgePtr->AsyncApply(handle, indx,
                        [](shad::rt::Handle &, size_t i, Edge &edge, double &val)
                        {
                            edge.weight=-1.00;
                        },
                    val);
                    
                    Counter->AsyncInsertAt(handle,0,1);
                    //std::cout<<"Got mate: "<<my_id<<" "<<mate<<std::endl;
                }
                else
                    if(taken==0)
                        vertex.mate=-1;
            },
        arrayOID, vertexOID, counterID);

        shad::rt::waitForCompletion(handle);
        
        int flag=Counter->At(0);
        //std::cout<<flag<<std::endl;
        if(iter==10)
            break;
        if(flag==1)
            Counter->InsertAt(0,0);
        else
            break;
    }
}

void check_graph(GraphL &L)
{
    shad::rt::Handle handle;
    uint64_t arrayOID=(uint64_t)L.edgeOID;
    
    L.edgePtr()->AsyncForEachInRange(handle, 0, 8313761,
        [](shad::rt::Handle &handle, size_t i, Edge &edge, uint64_t &arrayOID, uint64_t &ns, uint64_t &ts)
        {
            auto edgePtr =EdgeType::GetPtr((EdgeOID) arrayOID);
            if(edge.src<0 || edge.src >= ns)
                std::cout<<"id Problem(src): "<<i<<" "<<edge.src<<" "<<edge.dst<<std::endl;
            if(edge.dst<ns || edge.dst >= ts)
                std::cout<<"id Problem (dst): "<<i<<" "<<edge.src<<" "<<edge.dst<<std::endl;
            
        },
        arrayOID, L.a_num_vertices,L.vertexNumber);

    L.edgePtr()->AsyncForEachInRange(handle, 8313761, L.edgeNumber,
        [](shad::rt::Handle &handle, size_t i, Edge &edge, uint64_t &arrayOID, uint64_t &ns, uint64_t &ts)
        {
            auto edgePtr =EdgeType::GetPtr((EdgeOID) arrayOID);
            if(edge.dst<0 || edge.dst >= ns)
                std::cout<<"id Problem(src): "<<i<<" "<<edge.src<<" "<<edge.dst<<std::endl;
            if(edge.src<ns || edge.src >= ts)
                std::cout<<"id Problem (dst): "<<i<<" "<<edge.src<<" "<<edge.dst<<std::endl;
            
        },
        arrayOID, L.a_num_vertices,L.vertexNumber);
}

void print_graph(GraphL &L)
{

    for(int i=0;i<L.vertexNumber;i++)
    {
        std::cout<<L.vertexPtr()->At(i).id<<" "<<L.vertexPtr()->At(i).edges<<" "<<(int)L.vertexPtr()->At(i).type<<std::endl;
    }

    for(int i=0;i<L.edgeNumber;i++)
    {
        std::cout<<L.edgePtr()->At(i).src<<" "<<L.edgePtr()->At(i).dst<<" "<<L.edgePtr()->At(i).weight<<std::endl;
    }
}

void print_graph_(uint64_t arrayOID, uint64_t edgeOID, uint64_t m, uint64_t n)
{
    auto Vertices =VertexType::GetPtr((VertexOID) arrayOID);

    auto Edges =EdgeType::GetPtr((EdgeOID) edgeOID);


    for(int i=0;i<n;i++)
    {
        std::cout<<(int)Vertices->At(i).id<<" "<<(int)Vertices->At(i).edges<<" "<<(int)Vertices->At(i).type<<std::endl;
    }

    std::cout<<"$$$$$"<<std::endl;

    for(int i=0;i<m;i++)
    {
        std::cout<<(int)Edges->At(i).type<<" "<<(int)Edges->At(i).src_type<<" "<<(int)Edges->At(i).dst_type<<std::endl;
    }
}

void reset_weight(GraphL &L)
{

    shad::rt::Handle handle;
    L.vertexPtr()->AsyncForEachInRange(handle,0,L.vertexNumber,
        [](shad::rt::Handle &handle, size_t i, VertexL &vertex, uint64_t &ns)
        {
            vertex.taken=0;
            vertex.mate=-1;
        },
    L.a_num_vertices);
    shad::rt::waitForCompletion(handle);
}

std::map<uint64_t, uint64_t> get_matching(GraphL &L, int verbose)
{
    std::map<uint64_t, uint64_t> match;
    std::vector<VertexL> vertex(L.vertexNumber);
    shad::rt::Handle handle;

    if(verbose==1)
        std::cout<<std::endl<<"....Matching Extraction...."<<std::endl<<std::endl;
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
                std::cout<<id<<" "<<mate<<std::endl;
        }
    }
    reset_weight(L);
    return match;
}


void netAlign(std::string & patternFile,std::string & dataFile)
{


    auto my_rank=shad::rt::thisLocality();
    auto total_ranks =shad::rt::numLocalities();
    
    //std::string basename = "test";
    //std::string Afilename = basename + "-A.mtx";
    //std::string Bfilename = basename + "-B.mtx";
    
    
    double time1,time2,time3,time4,time5,time6,time7,temp,timet;
    double objective=-1000000,zero=0.0;
   
    std::cout<<"Initialization Starts"<<std::endl;
    time1=my_timer(); 
    
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

    uint64_t  m,n,m1,n1;
    readFile( patternFile, A);
    CSR(m, n, A);
    time2=my_timer();
    //std::cout<<m<<" "<<n<<std::endl;
    std::cout<<"File A reading done in "<<time2-time1<<" seconds"<<std::endl;
    
    readFile( dataFile, B);
    CSR(m1, n1, B);
    time3=my_timer();
    //std::cout<<m<<" "<<n<<std::endl;
    std::cout<<"File B reading done in "<<time3-time2<<" seconds"<<std::endl;
    
    GraphL L;
    createBipartite(A,A,L);
    time4=my_timer();
    std::cout<<"L construction done in "<<time4-time3<<" seconds"<<std::endl;
    
    
    
    ///// BP LOGIC
    //args_S_t args={A["Vertices"],A["Edges"],B["Vertices"],B["Edges"],L.a_num_vertices};
    args_S_t args={A["Vertices"],A["Edges"],A["Vertices"],A["Edges"],L.a_num_vertices};
    //std::cout<<"Edges "<<L.edgePtr()->Size()<<std::endl;
    L.edgePtr()->ForEach(createSquareMatrix, args);
    //check_graph(L);
    //shad::rt::waitForCompletion(handle);
    time5=my_timer();
    std::cout<<"BP done in "<<time5-time4<<" seconds"<<std::endl;
    //print_graph(L);
    //std::cout<<"-------------"<<std::endl;
    //print_graph_(A["Vertices"],A["Edges"],m,n);
    //std::cout<<"-------------"<<std::endl;
    //print_graph_(B["Vertices"],B["Edges"],m1,n1);
    
    ///// Matching

    int top_k=3;
    std::vector<std::map<uint64_t,uint64_t> >result(top_k);
    for(int i=0;i<top_k;i++)
    {
        getApproxMatching(L);
         //// Output Processing
        std::map<uint64_t,uint64_t> match=get_matching(L,1);
        result.push_back(match);
        time7=my_timer(); 
    }
    time6=my_timer();
    std::cout<<"Matching done in "<<time6-time5<<" seconds"<<std::endl;
    std::cout<<"Total done in "<<time6-time1<<" seconds"<<std::endl; 
    
} /// NetAlign
} /// namespace