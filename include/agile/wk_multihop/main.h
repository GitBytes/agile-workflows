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
using Graph_t = std::array<uint64_t, 1387>;
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
