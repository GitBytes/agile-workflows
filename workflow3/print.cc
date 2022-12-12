#include "agile/workflow3/main.h"
#include "agile/workflow3/graph.h"

namespace agile::workflow3 {

void printMNMap(uint64_t count, uint64_t MNMap_OID) {
  auto KNMap = KMapType::GetPtr((KMapOID) MNMap_OID);

  for (auto itr = MNMap->begin(); itr != MNMap->end() + count; ++ itr) {
    MacroNode tmp = (* itr).second;
    std::string key = kmer_string( (* itr).first, kmer_length - 1 );

    printf("%s, %lu %lu %c\n", key.c_str(), tmp.count.first, tmp.count.second, tmp.baseAcid);
    printf("     %lu %lu %d %lu\n", (uint64_t) tmp.isPrefix, (uint64_t) tmp.terminal, tmp.prefix_begin, tmp.num_wires);
} }


void printWireMap(uint64_t count, uint64_t WireMap_OID) {
  auto WireMap = WireMapType::GetPtr((WireMapOID) WireMap_OID);

  for (auto itr = WireMap->begin(); itr != WireMap->end() + count; ++ itr) {
    WireNode tmp = (* itr).second;
    std::string key = kmer_string( (* itr).first, kmer_length - 1 );

    printf("%s %lu %d %d\n", key.c_str(), tmp.sid, tmp.offset, tmp.count);
} }

}
