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

#include "shad/data_structures/hashmap.h"
#include "shad/data_structures/multimap.h"
#include "shad/core/algorithm.h"
#include "shad/core/numeric.h"
#include "agile/workflow1/main.h"


void createSquareMatrix(Graph_t &A, Graph_t &B, GraphL &L)
{
    Vector<int> Aneighbors;
    Vectoe<int> Bneighbors;
    
    L.edgePtr()->AsyncForEachInRange(
        handle, 0, L.edgeNumber,
        [](shad::rt::Handle &, size_t j, Graph_t &A, Graph_t &B, GraphL &L,nA,nB)
        {
            /// get the two end points (i, j)
            /// Get degree of i in A and resize nA, then get the neighbors in nA
            /// Get degree of j in B and resize nB, then get the neighbors in nB
            /// two serial for loops nA x nB, and check for (i', j') if it is in L.edge
            /// and update the L.edge weight of (i,j)  
        }
    A,B,L,Aneighbors,Bneighbors);
    
    void Array<T>::AsyncGetElements(rt::Handle& h, T* local_data,
                                const uint64_t idx, const uint64_t num_el)
    
}

void createBipartite(Graph_t &A, Graph_t &B, GraphL &L)
{
    
    shad::rt::Handle handle;
    int numLocality=shad::rt::numLocalities();
    
    L.vertexNumber=A->vertexNumber+B->vertexNumber;
    L.vertexOID=shad::Array<size_t>::Create(L.vertexNumber + 1, 0)->GetGlobalID();
    
    
    /**
    auto countID=shad::Aray<size_t>::Create[30*numLocality,0]->getGlobalID();
    
    for(int i=0;i<30;i++)
        GraphAType[i]=GraphBType[i]=0;
    
    A.vertexPtr()->AsyncForEachInRange(
        handle,0,A.vertexNumber,
        [](shad::rt::Handle&, size_t i, Array<size_t>::objectID)
        {
            auto type=A.vertexType()->At(i);
            ///compute the offset
            __sync_fetch_and_add(&count[i],1);
        }
        countID);
    
    shad::rt::waitForCompletion(handle);
    
    B.vertexType()->AsyncForEachInRange(
        handle,0,B.vertexNumber,
        [](shad::rt::Handle &, size_t i, size_t &val,int* count)
        {
            //auto type=B.vertexType()->At(i);
            __sync_fetch_and_add(&count[val],1);
        }
        GraphBType);
    
    shad::rt::waitForCompletion(handle);
    */
    
    ///// ADD ENUM count at the end of enum class to get the size
    int typeSize=TYPES:Count;
    count=0;
    for(int i=0;i<typeSize;i++)
        count+=A.typeFreq[i]*B.typeFreq[i];
    
    L.edgeNumber=2*count;
    L.edgeOID=shad::Array<size_t>::Create(L.edgeNumber, 0)->GetGlobalID();
    L.edgeWID=shad::Array<size_t>::Create(L.edgeNumber, 0.0)->GetGlobalID();
    
    L.vertexPtr()->AsyncForEachInRange(
        handle, 0, L.vertexNumber,
        [](shad::rt::Handle &, size_t i, int* GraphAType, int* GraphBType, int ns)
        {
            
            if(i<ns)
            {
                auto type=A.vertexType()->At(i);
                auto count=B.typeFreq[type];
                L.vertexPtr()->AsyncInsertAt(handle, i+1, count);
            }
            else
            {
                auto type=B.vertexType()->At(i-ns);
                auto count=B.typeFreq[type];
                L.vertexPtr()->AsyncInsertAt(handle, i+1, count);
            }
        }
        GraphAType, GraphBType, A.vertexNumber);
    
    shad::rt::waitForCompletion(handle);
    
    ////// Check this one
    shad::core::exclusive_scan(shad::distributed_parallel_tag{},L.vertexPtr()->begin(),L.vertexPtr()->end(),0)
    
    shad::rt::waitForCompletion(handle);
    
    Vector<int> Atype;
    Vector<int> Btype;
    ///// Get the neighbor and do a serial search
    for(int i=0;i<typeSize;i++)
    { 
        A.vertexPtr()->asyncForEachInRange(
            handle, 0, A.vertexNumber,
            [](shad::rt::Handle &, size_t j, Graph_t &A)
            {
                if(A.vertexType()->At(j) != j)
                    continue;
                
                Atype.push_back(j);
                
            },
        A);
        
        shad::rt::waitForCompletion(handle);
        
        B.vertexPtr()->asyncForEachInRange(
            handle, 0, B.vertexNumber,
            [](shad::rt::Handle &, size_t j, Graph_t &B, int ns)
            {
                if(B.vertexType()->At(j) != j)
                    continue;
                
                Btype.push_back(j+ns); // Updating the index for bipartite graph
                
            },
        B,A.vertexNumber);
        
        shad::rt::waitForCompletion(handle);
        
        //// Now that we got the edges indices ... just copy that to appropriate places
        
        L.vertexPtr()->asyncForEachInRange(
            handle,0,L.vertexNumber,
            [](shad::rt::Handle &, size_t j,Graph_t &L, int ns)
            {
                if(j<ns)
                {
                    if(L.vertexType()->At(j)!=i)
                        continue;
                    
                    int start=L.vertexPtr()->At(j);
                    L.edgePtr()->AsyncInsertAt(handle, start, Btype.data(), Btype.size());                   
                }
                else
                {
                    if(L.vertexType()->At(j)!=i)
                        continue;
                    
                    int start=L.vertexPtr()->At(j);
                    L.edgePtr()->AsyncInsertAt(handle, start, Atype.data(), Atype.size()); 
                }
            },
        L,A.vertexNumber);
        
        Atype.clear();
        Btype.clear();
    }
    
    
    shad::rt::waitForCompletion(handle);
}


void netAlign(uint64_t argc, char* argv[])
{

    netalign_parameters opts;
    if (!opts.parse(argc, argv)) {
        return;
    }
    
    string basename = opts.problemname;
    string Afilename = basename + "-A.mtx";
    string Bfilename = basename + "-B.mtx";
    
    
    double time1,time2,time3,time4,temp,timet;
    double objective=-1000000,zero=0.0;
   
    cout<<"Initialization Starts"<<endl;
    time1=my_timer(); 
    
    Graph_t A;
    Graph_t B;
    
    readFile((char*)Afilename.c_str(), &A);
    cout<<"File A reading done..!!"<<endl;
    readFile((char*)Bfilename.c_str(), &B);
    cout<<"File B reading done..!!"<<endl;

    GraphL L;
    
    ///// TODO: Add the graph weight by adding some similarity
    createBipartite(&A,&B,&L);
    cout<<"L construction done..!!"<<endl;
    
    time4=my_timer();
    
    createSquareMatrix(&A,&B,&L);
    
    
    // Graph data Structures
    //gmt_data_t buffer=gmt_alloc(nvectors*sizeof(gmt_data_t),GMT_ALLOC_REPLICATE);
    /gmt_data_t nMate=gmt_alloc(nvectors*sizeof(gmt_data_t),GMT_ALLOC_REPLICATE);
    /gmt_data_t nDuals=gmt_alloc(nvectors*sizeof(gmt_data_t),GMT_ALLOC_REPLICATE);
    /gmt_data_t nQ1=gmt_alloc(nvectors*sizeof(gmt_data_t),GMT_ALLOC_REPLICATE);
    /gmt_data_t nQ2=gmt_alloc(nvectors*sizeof(gmt_data_t),GMT_ALLOC_REPLICATE);
    /gmt_data_t nLock=gmt_alloc(nvectors*sizeof(gmt_data_t),GMT_ALLOC_REPLICATE);
    /gmt_data_t nCur=gmt_alloc(nvectors*sizeof(gmt_data_t),GMT_ALLOC_REPLICATE);
    
    
    /*************** Creating parameter structure *************/
    
    args_t args;

    args.size=size;
    args.snz=snz;
    /args.ns=ns;
    /args.nt=nt;
    /args.nz=nz;
    
    args.verbose=opts.verbose;
    

    args.dt=dt;
    /args.yt=yt; /// This will get the weight of L
    
    
    //args.buffer=buffer;
    /args.nMate=nMate;
    /args.nDuals=nDuals;
    
    /args.nQ1=nQ1;
    /args.nQ2=nQ2;
    /args.nLock=nLock;
    /args.nCur=nCur;
    
    /args.ic1=L.ic;
    /args.jc1=L.rjc;
   

    args.objT=objT;
    args.curr=curr;
    args.overlaps=overlaps;
    
    
    
    
    
    cout<<"Initialization Ends"<<endl;

    /*************** Its Solution Time..!! ************************/
    double* ind;
    double* mbest;

    ///// Call Half Aprox Method
    
    time2=my_timer();
    mbest=gmt_netAlignMP(&args,&objective);
    time3=my_timer();
    cout<<"Set Up Time: "<<time2-time1<<" "<<time4-time1<<endl;
    cout<<"Solve Time: "<<time3-time2<<endl;
    
/************** Local arrays clean up ************/
    //delete[] s;
    //delete[] t;
    //delete[] w;
    delete[] mbest;
    delete[] ind;
/************* Global arrays clean up ***********/
    gmt_free(dt);
    gmt_free(yt);
    gmt_free(zt);
    gmt_free(dt1);
    gmt_free(yt1);
    gmt_free(zt1);
    gmt_free(Fvc);
    gmt_free(vvc);
    //gmt_free(p);
    //gmt_free(aw);
    gmt_free(stvc);
    gmt_free(st1vc);
    //gmt_free(ic);
    //gmt_free(jc);
    //gmt_free(perm);
    gmt_free(roli);
    gmt_free(rolj);
    gmt_free(roiperm);
    gmt_free(rojperm);

    //gmt_free(buffer);
    gmt_free(nMate);
    gmt_free(nDuals);
    gmt_free(x);
    gmt_free(nQ1);
    gmt_free(nQ2);
    gmt_free(nLock);
    gmt_free(nCur);
    //gmt_free(ic1);
    //gmt_free(jc1);
    
    gmt_free(objT);
    gmt_free(curr);
    gmt_free(overlaps);
    gmt_waitCommands();
}