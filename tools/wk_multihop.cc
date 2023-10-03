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
#include "agile/wk_multihop/main.h"
#include "agile/wk_multihop/graph.h"

/** STEPS:
1. Read in the edgelist file, given as a triple per line (head, relation, tail)

2. Based on the "relation", insert the (head, tail) pair in the appropriate "relation" table. A total of 1387 relation tables will be created and populated with appropriate entries.

3. Read in the file containing the embeddding for all the entities (heads and tails). Approx 91M entities. Construct the entity embedding table.

4. Read in the file containing the embeddding for all the relations. Construct the relation embedding table.

5. Construct a table containing person entities. Currently we only extract the person entities by looking into the "AWARD WINNER" relation table. Other relevant relation tables can be peeked at if we want to expand the size of the person table with more entries for exercising sclability.

6. Similarly construct a table for universities/institutions. For now, we extract these only by looking into the "AFFILIATED WITH" relation table. Could be expanded similarly as step 5.

7. MHR loop: K is set to 50 by default
7a. For "Turing Award" (head) entity and "AWARD WINNER" relation, we compute a score for each of the person in the Person table to see their potential for winning the award. We use an embedding based model for prediction, specifically the simple TransE function (https://papers.nips.cc/paper_files/paper/2013/file/1cecc7a77928ca8133fa24680a88d2f9-Paper.pdf) or (https://github.com/snap-stanford/ogb/blob/master/examples/linkproppred/wikikg2/model.py#L163)

7b. We select the top k persons with highest score.

7c. Next, for each of these person, we compute the score for them to "work in" (relation) the "deep learning" (head) field.

7d. Sort them again based on the score.

7e. Finally, we find their potential affiliation by taking each of the entries in the university table and computing the score based on  person (head) -> affiliated with (relation) ->  university/institurion (tail). For each person we select the university/institute with highest score and print it out.  

Dataset reference: https://ogb.stanford.edu/docs/lsc/wikikg90mv2/
 **/

namespace shad {
  using namespace agile::wk_multihop;


int main(int argc, char *argv[]) {
// /**********  Graph/Edgelist, Tables, Embeddings  **********/

  Handle handle;

  auto EntityEmbeddingTable = EntityEmbeddingType::Create(AGILE_LARGE);
  auto RelationEmbeddingTable = RelationEmbeddingType::Create(AGILE_SMALL);
  auto PersonTable = PersonVertexType::Create(AGILE_LARGE);
  auto UniversityTable = UniversityVertexType::Create(AGILE_SMALL);
  
  std::string edgeDataFile = argv[1];
  std::string entityEmbeddingDataFile = argv[2];
  std::string relationEmbeddingDataFile = argv[3];

  /* Create 1387 (relation) tables, each for one type of relations in 
    "head, relation, tail" format (edge). */

  for (auto i = 0; i < 1387; i++) {
    graph[i] = (uint64_t) (WikiDataEdgeType::Create(AGILE_MEDIUM))->GetGlobalID();
  }

  /*Since passing around 1387 global ids in the arglist is not feasible due to some runtime 
    restriction, to make these ids available on all locales, "broadcast" these global ids 
    for the relation table to each locale */

  auto current_begin = graph.begin();
  auto current_end = graph.begin() + 100; // graph.end();
  auto i = 0;
  for (i = 0; i < 1300; i += 100) {
    uint64_t begin_idx = i;
    uint64_t end_idx = i + 100;
    std::array<uint64_t, 100> tmp;
    std::copy(current_begin, current_end, tmp.begin());
    auto indices_payload = std::make_tuple(begin_idx, end_idx, tmp);
    shad::rt::asyncExecuteOnAll(handle, [](shad::rt::Handle &, 
					   const std::tuple<uint64_t, uint64_t, 
					   std::array<uint64_t, 100> > &
					   indices_payload) {
				  std::copy(std::get<2>(indices_payload).begin(), 
					    std::get<2>(indices_payload).end(), 
					    graph.begin() + std::get<0>(indices_payload));
				}, indices_payload);
    current_begin = current_end;
    current_end += 100;
  }

  shad::rt::waitForCompletion(handle);

  // insert the rest
  current_end = current_begin + 87;
  std::array<uint64_t, 87> tmp;
  std::copy(current_begin, current_end, tmp.begin());
  uint64_t begin_idx = i;
  uint64_t end_idx = i + 87;
  auto indices_payload = std::make_tuple(begin_idx, end_idx, tmp);
  shad::rt::asyncExecuteOnAll(handle, [](shad::rt::Handle &, 
					 const std::tuple<uint64_t, uint64_t, 
					 std::array<uint64_t, 87> > &
					 indices_payload) {
				std::copy(std::get<2>(indices_payload).begin(), 
					  std::get<2>(indices_payload).end(), 
					  graph.begin() + std::get<0>(indices_payload));
			      }, indices_payload);

  shad::rt::waitForCompletion(handle);

  RF_args_t args;
  args.entity_embedding_table_OID = (uint64_t) (EntityEmbeddingTable->GetGlobalID()); 
  args.relation_embedding_table_OID = (uint64_t) (RelationEmbeddingTable->GetGlobalID()); 
  
  memcpy(args.edge_data_filename, edgeDataFile.c_str(), edgeDataFile.size() + 1);
  memcpy(args.entity_embedding_filename, entityEmbeddingDataFile.c_str(), 
	 entityEmbeddingDataFile.size() + 1);
  memcpy(args.relation_embedding_filename, relationEmbeddingDataFile.c_str(), 
	 relationEmbeddingDataFile.size() + 1);


  std::cout << "Reading edge (triples) data file " <<  edgeDataFile.c_str() << std::endl;
  shad::rt::asyncExecuteOnAll(handle, readEdgeFile, args);
  shad::rt::waitForCompletion(handle);


  for (auto i = 0; i< 1387; i++) {
    auto CurrentEdgeTable = WikiDataEdgeType::GetPtr( (WikiDataEdgeOID) 
    						      graph[i]);
    CurrentEdgeTable->WaitForBufferedInsert(); //AsyncWaitForBufferedInsert(handle);
  }

  // TODO: replace with the async variant
  //  shad::rt::waitForCompletion(handle);
#ifdef DEBUG
  for (auto i = 0; i< 1387; i++){
    auto CurrentEdgeTable = WikiDataEdgeType::GetPtr( (WikiDataEdgeOID) 
    						      graph[i]);
    auto table_size = CurrentEdgeTable->Size();
    if (table_size > 0) {
      std::cout << "Table " << i << " size " << table_size << std::endl;
    }
  }
#endif

  auto time1 = my_timer();

  std::cout << "Reading embedding data file for all entities "  
	    << entityEmbeddingDataFile.c_str() << std::endl;
  shad::rt::asyncExecuteOnAll(handle, readEntityEmbeddingFile, args);
  shad::rt::waitForCompletion(handle);
  EntityEmbeddingTable->WaitForBufferedInsert();

  std::cout << "Size of the entity embedding  table: " 
	    << EntityEmbeddingTable->Size() << std::endl;


  std::cout << "Reading embedding data file for all relations "  
	    << relationEmbeddingDataFile.c_str() << std::endl;
  shad::rt::asyncExecuteOnAll(handle, readRelationEmbeddingFile, args);
  shad::rt::waitForCompletion(handle);
  RelationEmbeddingTable->WaitForBufferedInsert();

  std::cout << "Size of the relation embedding table: " 
	    << RelationEmbeddingTable->Size() << std::endl;

  /*Create person table from a handful of relation tables*/
  auto personTableGID = (uint64_t) (PersonTable->GetGlobalID());
  buildPersonTable(personTableGID);
  std::cout << "Number of entries in the Person table " << PersonTable->Size() << std::endl;

  /*Create university table from a handful of relation tables*/
  auto universityTableGID = (uint64_t) (UniversityTable->GetGlobalID());
  buildUniversityTable(universityTableGID);
  std::cout << "Number of entries in the university table " << 
    UniversityTable->Size() << std::endl;

  auto time2 = my_timer();
  std::cout << "Construction time: " << time2 - time1 << std::endl;


  auto EntityEmbeddingTableGID = (uint64_t) EntityEmbeddingTable->GetGlobalID();
  auto RelationEmbeddingTableGID = (uint64_t) RelationEmbeddingTable->GetGlobalID(); 
  multiHopReasoning(EntityEmbeddingTableGID, RelationEmbeddingTableGID, personTableGID,
		    universityTableGID);

  std::cout << "Multihop execution time: " << my_timer() - time2 << std::endl;


  return 0;
}

}
