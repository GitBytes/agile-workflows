#include <limits>
#include <string>
#include <cstdlib>
#include <sys/stat.h>

#include "agile/wk_multihop/main.h"
#include "agile/wk_multihop/graph.h"

namespace agile::wk_multihop {

void Find_Insert_Award_Winners(Handle & handle, const uint64_t & src, 
			       std::vector <WikiDataEdge> & award_winners,
			       uint64_t &personTableGID) {
  auto PersonTable = PersonVertexType::GetPtr( (PersonVertexOID)  personTableGID); 

  for (auto award_winner : award_winners) { // TODO: optimize since only one unique winner
    PersonVertex record(award_winner.src_v);
    PersonTable->BufferedAsyncInsert(handle, record.key(), record);
  }
}


void buildPersonTable(uint64_t personTableGID) {
  Handle handle;
  auto PersonTable = PersonVertexType::GetPtr( (PersonVertexOID)  personTableGID); 

  /*Extract person entities from awardWinnerEdgeTable, total 539603 */ 
  auto AwardWinnerEdgeTable = WikiDataEdgeType::GetPtr( (WikiDataEdgeOID) 
  						  graph[AWARD_WINNER_TABLE_IDX]); 
  AwardWinnerEdgeTable->AsyncForEachEntry(handle, Find_Insert_Award_Winners, personTableGID);
  
  waitForCompletion(handle);

  PersonTable->WaitForBufferedInsert(); 

}
}
