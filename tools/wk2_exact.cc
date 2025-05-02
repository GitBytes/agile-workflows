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
#include "agile/wk2_exact/main.h"
#include "agile/wk2_exact/graph.h"

namespace shad {
  using namespace agile::wk2_exact;

int main(int argc, char *argv[]) {
  double time1 = my_timer();

/********** KERNEL 1 - Graph Construction **********/

  Handle handle;
  Graph_t graph;
  std::string dataFile = argv[1];

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
  memcpy(args.filename, dataFile.c_str(), dataFile.size() + 1);

  printf("Reading data file %s\n",  dataFile.c_str());    // read file, create tables
  shad::rt::asyncExecuteOnAll(handle, readFile, args);
  shad::rt::waitForCompletion(handle);

  Persons->AsyncWaitForBufferedInsert(handle);
  ForumEvents->AsyncWaitForBufferedInsert(handle);
  Forums->AsyncWaitForBufferedInsert(handle);
  Publications->AsyncWaitForBufferedInsert(handle);
  Topics->AsyncWaitForBufferedInsert(handle);

  Purchases->AsyncWaitForBufferedInsert(handle);
  Sales->AsyncWaitForBufferedInsert(handle);
  Authors->AsyncWaitForBufferedInsert(handle);
  Includes->AsyncWaitForBufferedInsert(handle);
  HasTopic->AsyncWaitForBufferedInsert(handle);
  HasOrg->AsyncWaitForBufferedInsert(handle);
  shad::rt::waitForCompletion(handle);

  printf("Time for Kernel 1 - Graph Construction = %lf\n\n", my_timer() - time1);

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

/********** Kernel 5 - Exact Match **********/

  time1 = my_timer();
  WMD_pattern(args);
  printf("Time for Kernel 5 - Exact Pattern Matching = %lf\n", my_timer() - time1);

  return 0;
}

}
