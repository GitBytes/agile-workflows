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

#include "agile/workflow4/main.h"
#include "agile/workflow4/graph.h"
#include "agile/workflow4/queryUpdateGraph.h"

namespace shad {
  using namespace agile::workflow4;

int main(int argc, char *argv[]) {
  double time1 = my_timer();

/********** KERNEL 1 - Graph Construction **********/

  Handle handle1, handle2, handle3, handle4, handle5;
  Graph_t graph;
  std::string dataFile  = argv[1];
  std::string dataFile2 = argv[2];
  std::string dataFile3 = argv[3];
  std::string dataFile4 = argv[4];
  std::string dataFile5 = argv[5];
  std::string outFile   = argv[6];

  std::vector<std::string> dataFiles{dataFile, dataFile2, dataFile3, dataFile4, dataFile5};

  auto Persons          = PersonVertexType::Create(MEDIUM);
  auto Topics           = TopicVertexType::Create(SMALL);
  auto Purchases        = PurchaseEdgeType::Create(MEDIUM);
  auto Sales            = SaleEdgeType::Create(MEDIUM);
  auto CoffeeSales      = SaleEdgeType::Create(MEDIUM);
  auto CoffeePurchases  = SaleEdgeType::Create(MEDIUM);
  auto Friends          = FriendOfEdgeType::Create(MEDIUM);
  auto Servers          = ServerVertexType::Create(MEDIUM);
  auto Sends            = SendsEdgeType::Create(MEDIUM);
  auto Uses             = UsesEdgeType::Create(MEDIUM);

  graph["Persons"]          = (uint64_t) (Persons->GetGlobalID());
  graph["Topics"]           = (uint64_t) (Topics->GetGlobalID());
  graph["Purchases"]        = (uint64_t) (Purchases->GetGlobalID());
  graph["Sales"]            = (uint64_t) (Sales->GetGlobalID());
  graph["CoffeeSales"]      = (uint64_t) (CoffeeSales->GetGlobalID());
  graph["CoffeePurchases"]  = (uint64_t) (CoffeePurchases->GetGlobalID());
  graph["Friends"]          = (uint64_t) (Friends->GetGlobalID());
  graph["Servers"]          = (uint64_t) (Servers->GetGlobalID());
  graph["Sends"]            = (uint64_t) (Sends->GetGlobalID());
  graph["Uses"]             = (uint64_t) (Uses->GetGlobalID());

  RF_args_t args;
  args.Persons_OID          = graph["Persons"];
  args.Topics_OID           = graph["Topics"];
  args.Purchases_OID        = graph["Purchases"];
  args.Sales_OID            = graph["Sales"];
  args.CoffeeSales_OID      = graph["CoffeeSales"];
  args.CoffeePurchases_OID  = graph["CoffeePurchases"];
  args.Friends_OID          = graph["Friends"];
  args.Servers_OID          = graph["Servers"];
  args.Sends_OID            = graph["Sends"];
  args.Uses_OID             = graph["Uses"];
  memcpy(args.outfilename, outFile.c_str(), outFile.size() + 1);
  // args.outfilename      = outFile;

  int count = 1;
  for(auto itr=dataFiles.begin(); itr!=dataFiles.end(); itr++) {
    memcpy(args.filename, itr->c_str(), itr->size() + 1);
    printf("Reading data file %s\n",  itr->c_str());    // read file, create tables
    switch (count) {
      case 1:
        shad::rt::asyncExecuteOnAll(handle1, readFileCoffee, args);
        shad::rt::waitForCompletion(handle1);
        break;
      case 2:
        shad::rt::asyncExecuteOnAll(handle2, readFileSocial, args);
        shad::rt::waitForCompletion(handle2);
        break;
      case 3:
        shad::rt::asyncExecuteOnAll(handle3, readFileCyber, args);
        shad::rt::waitForCompletion(handle3);
        break;
      case 4:
        shad::rt::asyncExecuteOnAll(handle4, readFileUses, args);
        shad::rt::waitForCompletion(handle4);
        break;
      case 5:
        shad::rt::asyncExecuteOnAll(handle5, readFileCommercial, args);
        shad::rt::waitForCompletion(handle5);
        break;
    }
    ++count;
    Persons->WaitForBufferedInsert();
    Topics->WaitForBufferedInsert();
    Purchases->WaitForBufferedInsert();
    Sales->WaitForBufferedInsert();
    Friends->WaitForBufferedInsert();
    Servers->WaitForBufferedInsert();
    Sends->WaitForBufferedInsert();
    Uses->WaitForBufferedInsert();
  }

  printf("Time for Kernel 1 - Graph Construction = %lf\n\n", my_timer() - time1);

  printf("Number of persons      = %lu\n", Persons->Size());
  printf("Number of topics       = %lu\n", Topics->Size());
  printf("Number of servers        = %lu\n", Servers->Size());

  printf("\n");
  printf("Number of purchase edges = %lu\n", Purchases->Size());
  printf("Number of sale edges     = %lu\n", Sales->Size());
  printf("Number of friends edges  = %lu\n", Friends->Size());
  printf("Number of sends edges    = %lu\n", Sends->Size());
  printf("Number of uses edges     = %lu\n", Uses->Size());

  printf("\n");
  printf("Total number of vertices = %lu\n",
       Persons->Size() + Servers->Size() + Topics->Size());
  printf("Total number of edges    = %lu\n", 
       Purchases->Size() + Sales->Size() + Friends->Size() + Sends->Size() + Uses->Size());

  getCoffeeSaleEdgeWeights(graph, args, 8486);
  std::vector<uint64_t> influencers = {20858,20859,41718,83435,166870,333741,667481};
  return 0;
}

}
