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

#include <string>
#include <numeric>
#include <math.h>
#include <limits.h>

#include "shad/data_structures/set.h"
#include "shad/data_structures/array.h"
#include "shad/data_structures/atomic.h"
#include "shad/data_structures/vector.h"
#include "shad/data_structures/hashmap.h"
#include "shad/data_structures/multimap.h"
#include "shad/extensions/data_types/data_types.h"

#define AGILE_TINY   5000
#define AGILE_SMALL  500000
#define AGILE_MEDIUM 5000000
#define AGILE_LARGE  50000000

#define UINT_BITS 64
#define SIZE_BP 2                       // bits per base pair
#define SIZE_BPV 6                      // size of base pair vector in 64 bit words
#define BP_PER_WORD 32                  // number of base pairs per word = 64 / 2
#define CONTIG_LENGTH_THRESHOLD 400     // output contig length threshold

namespace agile::workflow3 {

using Handle       = shad::rt::Handle;
using IntSet       = shad::Set<uint64_t>;
using IntArray     = shad::Array<int64_t>;
using IntSetOID    = shad::ObjectIdentifier<IntSet>;
using IntArrayOID  = shad::ObjectIdentifier<IntArray>;

struct Args_t {
  uint64_t KMap_OID;
  uint64_t MNMap_OID;
  uint64_t WireMap_OID;
  uint64_t BucketCounts_OID;
  uint64_t ModifiedNodes_OID;
  uint64_t ProcessedNodes_OID;
  uint64_t mnLength;
  uint64_t coverage;
  uint64_t min_index;
  uint64_t min_counts;
  char filename [120];
};

uint64_t CHAR_TO_EL(char x);
char EL_TO_CHAR(uint64_t x);
std::string kmer_string(uint64_t, uint64_t);
void int_fetch_add(Handle &, uint64_t, int64_t &, int64_t &);

void readFASTA(Handle &, const Args_t &);
void BucketCounts_(Handle &, const Args_t &);
void ConstructMacroNodes(Handle &, const Args_t &);
void DeleteMacroNode(Handle &, const uint64_t &, Args_t &);
void RewireMacroNode(Handle &, const uint64_t &, Args_t &);

} // namespace agile::workflow3

#endif  // MAIN_H
