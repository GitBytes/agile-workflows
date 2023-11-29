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

  printf("\n");
  printf("Total number of vertices = %lu\n",
       Persons->Size() + ForumEvents->Size() + Forums->Size() + Publications->Size() + Topics->Size());
  printf("Total number of edges    = %lu\n", 
       Purchases->Size() + Sales->Size() + Authors->Size() + Includes->Size() + HasTopic->Size() + HasOrg->Size());

/********** Kernel 5 - Exact Match **********/

  time1 = my_timer();

  WMD_pattern(args);
  printf("Time for Kernel 5 - Exact Pattern Matching = %lf\n", my_timer() - time1);

  return 0;
}

}
