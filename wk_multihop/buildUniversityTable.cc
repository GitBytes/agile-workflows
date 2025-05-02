#include <limits>
#include <string>
#include <cstdlib>
#include <sys/stat.h>

#include "agile/wk_multihop/main.h"
#include "agile/wk_multihop/graph.h"

namespace agile::wk_multihop {

void Find_Insert_Universities(Handle & handle, const uint64_t & src, 
			       std::vector <WikiDataEdge> & affiliations,
			       uint64_t &universityTableGID) {
  auto UniversityTable = UniversityVertexType::GetPtr( (UniversityVertexOID)  
						       universityTableGID); 

  for (auto affiliation : affiliations) { // TODO: optimize since only one unique winner
    /*Filter out some ambiguous affiliations*/
    if (affiliation.dst_v == 22174494 /*generic affiliation*/ 
	|| affiliation.dst_v == 78111271 
	|| affiliation.dst_v == 51562303 
	|| affiliation.dst_v == 344618 /*self-employment*/
	|| affiliation.dst_v == 4209802
	|| affiliation.dst_v == 9294723 /*generic description*/
	|| affiliation.dst_v == 35693055 /*another generic entity*/) {continue;}
    UniversityVertex record(affiliation.dst_v);
    UniversityTable->BufferedAsyncInsert(handle, record.key(), record);
  }
}


void buildUniversityTable(uint64_t universityTableGID) {
  Handle handle;
  auto UniversityTable = UniversityVertexType::GetPtr( (UniversityVertexOID)  
						       universityTableGID); 

  /*Extract university entities from affiliatedWithEdgeTable */ 
  auto AffiliatedWithEdgeTable = WikiDataEdgeType::GetPtr( (WikiDataEdgeOID) 
  						  graph[AFFILIATED_WITH_TABLE_IDX]); 
  AffiliatedWithEdgeTable->AsyncForEachEntry(handle, Find_Insert_Universities, 
					     universityTableGID);
  
  waitForCompletion(handle);

  UniversityTable->WaitForBufferedInsert();

}
}
