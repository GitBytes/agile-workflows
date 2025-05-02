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
#include "agile/workflow3/main.h"
#include "agile/workflow3/graph.h"

namespace agile::workflow3 {

uint64_t CHAR_TO_EL(char x) {
  uint64_t result;
  switch (x) {
    case 'A': result =  0; break;
    case 'C': result =  1; break;
    case 'T': result =  2; break;
    case 'G': result =  3; break;
    case '*': result = 42; break;
    default : result = ULLONG_MAX; break;
  }

  return result;
}

char EL_TO_CHAR(uint64_t x) {
  uint64_t result;
  switch (x) {
    case  0: result = 'A'; break;
    case  1: result = 'C'; break;
    case  2: result = 'T'; break;
    case  3: result = 'G'; break;
    case 42: result = '*'; break;
    default: result = '*'; break;
  }

  return result;
}

void int_fetch_add(Handle & handle, uint64_t pos, int64_t & elem, int64_t & incr) {
  __sync_fetch_and_add(& elem, incr);
}

bool MN_comp(MacroNode & A, MacroNode & B) {
  if (A.isPrefix != B.isPrefix) return A.isPrefix;     // prefixes stored before suffixes

  return ( A.count.second >  B.count.second) ||        // store in coverage - count order
         ((A.count.second == B.count.second) && (A.count.first > B.count.first));
}

std::string kmer_string(uint64_t kmer, uint64_t length) {
  std::string str;
  str.resize(length);

  for (uint64_t i = 0; i < length; ++ i) {
    str[length - i - 1] = EL_TO_CHAR(kmer & 3);
    kmer = kmer >> 2;
  }

  return str;
}

} // namespace agile::workflow3
