//===------------------------------------------------------------*- C++ -*-===//
////
////                            The AGILE Workflows
////
////===----------------------------------------------------------------------===//
//// ** Pre-Copyright Notice
////
//// This computer software was prepared by Battelle Memorial Institute,
//// hereinafter the Contractor, under Contract No. DE-AC05-76RL01830 with the
//// Department of Energy (DOE). All rights in the computer software are reserved
//// by DOE on behalf of the United States Government and the Contractor as
//// provided in the Contract. You are authorized to use this computer software
//// for Governmental purposes but it is not to be released or distributed to the
//// public. NEITHER THE GOVERNMENT NOR THE CONTRACTOR MAKES ANY WARRANTY, EXPRESS
//// OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE. This
//// notice including this sentence must appear on any copies of this computer
//// software.
////
//// ** Disclaimer Notice
////
//// This material was prepared as an account of work sponsored by an agency of
//// the United States Government. Neither the United States Government nor the
//// United States Department of Energy, nor Battelle, nor any of their employees,
//// nor any jurisdiction or organization that has cooperated in the development
//// of these materials, makes any warranty, express or implied, or assumes any
//// legal liability or responsibility for the accuracy, completeness, or
//// usefulness or any information, apparatus, product, software, or process
//// disclosed, or represents that its use would not infringe privately owned
//// rights. Reference herein to any specific commercial product, process, or
//// service by trade name, trademark, manufacturer, or otherwise does not
//// necessarily constitute or imply its endorsement, recommendation, or favoring
//// by the United States Government or any agency thereof, or Battelle Memorial
//// Institute. The views and opinions of authors expressed herein do not
//// necessarily state or reflect those of the United States Government or any
//// agency thereof.
////
////                    PACIFIC NORTHWEST NATIONAL LABORATORY
////                                 operated by
////                                   BATTELLE
////                                   for the
////                      UNITED STATES DEPARTMENT OF ENERGY
////                       under Contract DE-AC05-76RL01830
////===----------------------------------------------------------------------===//

#include "agile/workflow3/main.h"
#include "agile/workflow3/graph.h"

namespace agile::workflow3 {

uint64_t visit_value (uint64_t count, uint64_t coverage) {
  double ceil_val = (double) count / (double) coverage;
  return (uint64_t) ceil(ceil_val);
}


void BucketCounts(Handle & handle, const Args_t & args) {
  auto KMap = KMapType::GetPtr((KMapOID) args.KMap_OID);
  auto bucketCounts = IntArray::GetPtr((IntArrayOID) args.bucketCounts_OID);

  uint64_t min_counts = args.min_counts;
  std::vector<int64_t> counts(min_counts, 0);

  for (auto itr = KMap->local_begin(); itr != KMap->local_end(); ++ itr) {
    uint64_t count = (* itr).second;
    if (count < min_counts) counts[count] ++;
  }

  for (uint64_t i = 0; i < min_counts; ++ i) {
    bucketCounts->AsyncApply(handle, i, int_fetch_add, counts[i]);
} }


void RemoveKmers(Handle & handle, const Args_t & args) {
  auto KMap = KMapType::GetPtr((KMapOID) args.KMap_OID);
  auto KVMap = KMapType::GetPtr((KMapOID) args.KVMap_OID);

  for (auto itr = KMap->local_begin(); itr != KMap->local_end(); ++ itr) {
    uint64_t kmer  = (* itr).first;
    uint64_t count = (* itr).second;
    if (count >= args.min_index) KVMap->BufferedAsyncInsert(handle, kmer, count);
} }


void ConstructMacroNodes(Handle & handle, const uint64_t & key, uint64_t & value, Args_t & args) {
  MacroNode prefix_mn = MacroNode();
  MacroNode suffix_mn = MacroNode();
  auto MNMap = MNMapType::GetPtr((MNMapOID) args.MNMap_OID);
  uint64_t suffix_mask = ~0UL >> (UINT_BITS - (2 * (args.kmer_length - 1)));

  prefix_mn.mnode    = key & suffix_mask;                                   // last  KMER_LENGTH - 1 proteins
  suffix_mn.mnode    = key >> 2;                                            // first KMER_LENGTH - 1 proteins
  prefix_mn.baseAcid = EL_TO_CHAR(key >> (2 * (args.kmer_length - 1)));     // first protein of key
  suffix_mn.baseAcid = EL_TO_CHAR(key & 3);                                 // last protein of key
  prefix_mn.isPrefix = true;
  suffix_mn.isPrefix = false;
  prefix_mn.count    = {value, visit_value(value, args.coverage)};
  suffix_mn.count    = {value, visit_value(value, args.coverage)};

  MNMap->BufferedAsyncInsert(handle, prefix_mn.mnode, prefix_mn);
  MNMap->BufferedAsyncInsert(handle, suffix_mn.mnode, suffix_mn);
}


void InitialMacroNodeWire(Handle & handle, const uint64_t & key, std::vector<MacroNode> & value, Args_t & args) {
  MacroNode prefix_mn = MacroNode();
  MacroNode suffix_mn = MacroNode();
  auto WireMap = WireMapType::GetPtr((WireMapOID) args.WireMap_OID);

  // push null prefix and suffix onto value
  prefix_mn.isPrefix = true;
  suffix_mn.isPrefix = false;
  prefix_mn.terminal = true;
  suffix_mn.terminal = true;
  value.push_back(prefix_mn);
  value.push_back(suffix_mn);

  int64_t  pc = 0, sc = 0;
  uint64_t null_prefix_id;
  uint64_t null_suffix_id;
  std::sort(value.begin(), value.end(), MN_comp);                  // sort value ... prefixes stored before suffixes

  for (uint64_t i = 0; i < value.size(); ++ i) {
    if (value[i].isPrefix) {     // node is prefix
       if (value[i].baseAcid != '*') pc += value[i].count.second; else null_prefix_id = i;
    } else {                     // node is suffix
       if (value[i].baseAcid != '*') sc += value[i].count.second; else null_suffix_id = i;
  } }

  value[null_prefix_id].count = {1, std::max(sc - pc, 0L)};     // count and coverage for null prefix
  value[null_suffix_id].count = {1, std::max(pc - sc, 0L)};     // count and coverage for null suffix

  uint64_t last_p = -1, prefix_begin = -1;
  uint64_t top_p = 0, top_s = 0, wire_idx = 0, p_size = 0;
  int64_t  var_p = 0, var_s = 0, offset_in_suffix = 0;
  int64_t  leftover = sc + value[null_suffix_id].count.second;

  while (leftover > 0) {
    int64_t count = std::min( (value[top_p].count.second - var_p), (value[top_s].count.second - var_s) );
    WireMap->BufferedAsyncInsert(handle, key, WireNode(top_s, offset_in_suffix, count));
    if (last_p != top_p) { prefix_begin = wire_idx; last_p = top_p; }

    p_size ++;
    wire_idx ++;
    var_p    += count;
    var_s    += count;
    leftover -= count;
    offset_in_suffix += count;

    if (var_p == value[top_p].count.second) {
       value[top_p].num_wires = p_size;
       value[top_p].prefix_begin = prefix_begin;
       p_size = 0;
       var_p  = 0;
       top_p ++;
    }

    if (var_s == value[top_s].count.second) {
       offset_in_suffix = 0;
       var_s = 0;
       top_s ++;
  } }

  for ( ; wire_idx < null_suffix_id + 2; ++ wire_idx)     // # wire nodes = # macro nodes + 1 
    WireMap->BufferedAsyncInsert(handle, key, WireNode());
}

} // namespace agile::workflow3
