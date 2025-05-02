/*===------------------------------------------------------------*- C++ -*-===
 *
 *                            The AGILE Workflows
 *
 *===----------------------------------------------------------------------===
 *
 * Copyright (c) 2025 Battelle Memorial Institute
 *
 * Battelle Memorial Institute (hereinafter Battelle) hereby grants permission
 * to any person or entity lawfully obtaining a copy of this software and
 * associated documentation files (hereinafter “the Software”) to redistribute
 * and use the Software in source and binary forms, with or without
 * modification. Such person or entity may use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and may permit
 * others to do so, subject to the following conditions:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimers.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Other than as used herein, neither the name Battelle Memorial Institute or
 *    Battelle may be used in any form whatsoever without the express written
 *    consent of Battelle.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL BATTELLE OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *===----------------------------------------------------------------------===*/
#include <iomanip>
#include <iostream>
#include <string>

#include "agile/wk2_approx/main.h"
#include "agile/wk2_approx/graph.h"

namespace shad {
  using namespace agile::wk2_approx;
  using Weight = std::pair<uint64_t, double>;
  
int main(int argc, char *argv[]) {
  double time1;
  Handle handle;
  std::string patternFile = argv[1];
  std::string dataFile    = argv[2];
  uint64_t Top_K = std::stod(argv[3]);

// CONSTRUCT PATTERN GRAPH
  Graph_t A;
  auto Persons      = PersonVertexType::Create(AGILE_MEDIUM);
  auto ForumEvents  = ForumEventVertexType::Create(AGILE_MEDIUM);
  auto Forums       = ForumVertexType::Create(AGILE_SMALL);
  auto Publications = PublicationVertexType::Create(AGILE_SMALL);
  auto Topics       = TopicVertexType::Create(AGILE_SMALL);

  auto Purchases    = PurchaseEdgeType::Create(AGILE_MEDIUM);
  auto Sales        = SaleEdgeType::Create(AGILE_MEDIUM);
  auto Authors      = AuthorEdgeType::Create(AGILE_LARGE);
  auto Includes     = IncludesEdgeType::Create(AGILE_LARGE);
  auto HasTopic     = HasTopicEdgeType::Create(AGILE_LARGE);
  auto HasOrg       = HasOrgEdgeType::Create(AGILE_MEDIUM);

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

  RF_args_t args;
  args.Persons_OID = A["Persons"];
  args.ForumEvents_OID = A["ForumEvents"];
  args.Forums_OID = A["Forums"];
  args.Publications_OID = A["Publications"];
  args.Topics_OID = A["Topics"];
  args.Purchases_OID = A["Purchases"];
  args.Sales_OID = A["Sales"];
  args.Authors_OID = A["Authors"];
  args.Includes_OID = A["Includes"];
  args.HasTopic_OID = A["HasTopic"];
  args.HasOrg_OID = A["HasOrg"];
  memcpy(args.filename, patternFile.c_str(), patternFile.size() + 1);

  printf("Reading pattern file %s\n",  patternFile.c_str());    // read file, create tables
  shad::rt::asyncExecuteOnAll(handle, readFile, args);
  shad::rt::waitForCompletion(handle);

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

  uint64_t A_num_vertices = 
     Persons->Size() + ForumEvents->Size() + Forums->Size() + Publications->Size() + Topics->Size();
  uint64_t A_num_edges = 
     Purchases->Size() + Sales->Size() + Authors->Size() + Includes->Size() + HasTopic->Size() + HasOrg->Size();

  printf("Pattern Graph has %lu vertices and %lu edges\n\n", A_num_vertices, A_num_edges);

/********** Kernel 1 - Graph Construction **********/

  time1 = my_timer();

// CONSTRUCT DATA GRAPH
  Graph_t B;
  Persons      = PersonVertexType::Create(AGILE_MEDIUM);
  ForumEvents  = ForumEventVertexType::Create(AGILE_MEDIUM);
  Forums       = ForumVertexType::Create(AGILE_SMALL);
  Publications = PublicationVertexType::Create(AGILE_SMALL);
  Topics       = TopicVertexType::Create(AGILE_SMALL);

  Purchases    = PurchaseEdgeType::Create(AGILE_MEDIUM);
  Sales        = SaleEdgeType::Create(AGILE_MEDIUM);
  Authors      = AuthorEdgeType::Create(AGILE_LARGE);
  Includes     = IncludesEdgeType::Create(AGILE_LARGE);
  HasTopic     = HasTopicEdgeType::Create(AGILE_LARGE);
  HasOrg       = HasOrgEdgeType::Create(AGILE_MEDIUM);

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

  args.Persons_OID = B["Persons"];
  args.ForumEvents_OID = B["ForumEvents"];
  args.Forums_OID = B["Forums"];
  args.Publications_OID = B["Publications"];
  args.Topics_OID = B["Topics"];
  args.Purchases_OID = B["Purchases"];
  args.Sales_OID = B["Sales"];
  args.Authors_OID = B["Authors"];
  args.Includes_OID = B["Includes"];
  args.HasTopic_OID = B["HasTopic"];
  args.HasOrg_OID = B["HasOrg"];
  memcpy(args.filename, dataFile.c_str(), dataFile.size() + 1);

  printf("Reading data file %s\n",  dataFile.c_str());    // read file, create tables
  shad::rt::asyncExecuteOnAll(handle, readFile, args);
  shad::rt::waitForCompletion(handle);
  
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

  uint64_t B_num_vertices = 
     Persons->Size() + ForumEvents->Size() + Forums->Size() + Publications->Size() + Topics->Size();
  uint64_t B_num_edges = 
     Purchases->Size() + Sales->Size() + Authors->Size() + Includes->Size() + HasTopic->Size() + HasOrg->Size();

  printf("Time for Kernel 1 - Graph Construction = %lf\n", my_timer() - time1);
  printf("Data Graph has %lu vertices and %lu edges\n\n", B_num_vertices, B_num_edges);

/********** Kernel 4a - BiPartite Graph Construction **********/

  time1 = my_timer();

// CONSTRUCT BIPARTITE VERTICES WITH EDGES
  auto LHS = VertexType::Create(AGILE_TINY);
  auto RHS = VertexType::Create(AGILE_LARGE);
  uint64_t LHS_OID = (uint64_t) (LHS->GetGlobalID());
  uint64_t RHS_OID = (uint64_t) (RHS->GetGlobalID());
  createBipartite(A, B, LHS_OID, RHS_OID);
  printf("Time for Kernel 4a - BiPartite Graph Construction = %lf\n", my_timer() - time1);
  
/********** Kernel 4b - Approximate Match **********/

  time1 = my_timer();

  for (uint64_t i = 0; i < Top_K; ++ i) ApproxMatching(LHS_OID, RHS_OID);
  printf("Time for Kernel 4b - Approximate Match = %lf\n", my_timer() - time1);
  return 0;

}

} /// namespace
