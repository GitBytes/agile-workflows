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
