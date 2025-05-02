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
