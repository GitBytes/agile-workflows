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
