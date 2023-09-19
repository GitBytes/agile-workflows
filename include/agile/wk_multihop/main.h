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

#ifndef MAIN_H_
#define MAIN_H_

// #include <map>
#include <array>
#include <math.h>
#include <limits.h>

#include "shad/data_structures/array.h"
#include "shad/data_structures/hashmap.h"
#include "shad/data_structures/multimap.h"

#define AGILE_TINY   5000
#define AGILE_SMALL  500000
#define AGILE_MEDIUM 5000000
#define AGILE_LARGE  50000000

#define AWARD_WINNER_TABLE_IDX 207
#define WORKS_IN_TABLE_IDX 3
#define AFFILIATED_WITH_TABLE_IDX 40

#define EMBEDDING_DIMENSION 450

namespace agile::wk_multihop {

using Handle = shad::rt::Handle;
using Graph_t = std::array<uint64_t, 1386>;
extern Graph_t graph;

struct RF_args_t {
  uint64_t entity_embedding_table_OID;
  uint64_t relation_embedding_table_OID;
  char edge_data_filename [120];
  char entity_embedding_filename [120];
  char relation_embedding_filename [120];
};

void readEdgeFile(Handle & handle, const RF_args_t & args);
void readEntityEmbeddingFile(Handle & handle, const RF_args_t & args);
void readEntityEmbeddingFileSingleLocale(Handle & handle, const RF_args_t & args);
void readRelationEmbeddingFile(Handle & handle, const RF_args_t & args);
void buildPersonTable(uint64_t personTableGID);
void buildUniversityTable(uint64_t universityTableGID);
void WikiData_pattern();
void multiHopReasoning(uint64_t entity_embedding_table_oid, 
		       uint64_t relation_embedding_table_id,
		       uint64_t person_table_oid,
		       uint64_t university_table_oid);
} // namespace agile::wk_multihop

#endif  // MAIN_H
