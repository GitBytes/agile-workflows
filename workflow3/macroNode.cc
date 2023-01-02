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


void UpdateWireMap(Handle & handle, const uint64_t & key, std::vector<WireNode> & value,
        uint64_t & idx, uint64_t & sid, int64_t & offset, int64_t & count) {
  value[idx].sid    = sid;
  value[idx].offset = offset;
  value[idx].count  = count;
}


void BucketCounts_(Handle & handle, const Args_t & args) {
  auto KMap = KMapType::GetPtr((KMapOID) args.KMap_OID);
  auto BucketCounts = IntArray::GetPtr((IntArrayOID) args.BucketCounts_OID);

  uint64_t min_counts = args.min_counts;
  std::vector<int64_t> counts(min_counts, 0);

  for (auto itr = KMap->local_begin(); itr != KMap->local_end(); ++ itr) {
    uint64_t count = (* itr).second;
    if (count < min_counts) counts[count] ++;
  }

  for (uint64_t i = 0; i < min_counts; ++ i) {
    BucketCounts->AsyncApply(handle, i, int_fetch_add, counts[i]);
} }


void ConstructMacroNodes(Handle & handle, const Args_t & args) {
  auto KMap  = KMapType::GetPtr((KMapOID) args.KMap_OID);
  auto MNMap = MNMapType::GetPtr((MNMapOID) args.MNMap_OID);
  uint64_t suffix_mask = ~0UL >> (UINT_BITS - (2 * (args.mnLength)));

  for (auto itr = KMap->local_begin(); itr != KMap->local_end(); ++ itr) {
    uint64_t kmer  = (* itr).first;
    uint64_t count = (* itr).second;

    if (count >= args.min_index) {
       MacroNode suffix_mn = MacroNode();
       MacroNode prefix_mn = MacroNode();
       uint64_t suffix_key = kmer >> 2;                            // first KMER_LENGTH - 1 proteins
       uint64_t prefix_key = kmer & suffix_mask;                   // last  KMER_LENGTH - 1 proteins
       suffix_mn.affix.push_back(kmer & 3);                        // push back last  protein of kmer
       prefix_mn.affix.push_back(kmer >> (2 * args.mnLength));     // push back first protein of kmer
       suffix_mn.isPrefix = false;
       prefix_mn.isPrefix = true;
       suffix_mn.count    = {count, visit_value(count, args.coverage)};
       prefix_mn.count    = {count, visit_value(count, args.coverage)};

       MNMap->BufferedAsyncInsert(handle, suffix_key, suffix_mn);
       MNMap->BufferedAsyncInsert(handle, prefix_key, prefix_mn);
} } }


void PushTerminals_Defaults(Handle & handle, const uint64_t & key, std::vector<MacroNode> & value, Args_t & args) {
  uint64_t size = value.size();
  auto MNMap = MNMapType::GetPtr((MNMapOID) args.MNMap_OID);
  auto WireMap = WireMapType::GetPtr((WireMapOID) args.WireMap_OID);

  // push null prefix and suffix onto value
  MacroNode prefix_mn = MacroNode();
  MacroNode suffix_mn = MacroNode();
  prefix_mn.isPrefix = true;
  suffix_mn.isPrefix = false;
  prefix_mn.isTerminal = true;
  suffix_mn.isTerminal = true;
  MNMap->BufferedAsyncInsert(handle, key, prefix_mn);
  MNMap->BufferedAsyncInsert(handle, key, suffix_mn);

  for (uint64_t i = 0; i < size + 3; ++ i)
    WireMap->BufferedAsyncInsert(handle, key, WireNode());
}


void InitialMacroNodeWire(Handle & handle, const uint64_t & key, std::vector<MacroNode> & value, Args_t & args) {
  auto WireMap = WireMapType::GetPtr((WireMapOID) args.WireMap_OID);
  // std::string my_kmer = kmer_string(key, args.mnLength);
  // if (strcmp(my_kmer.c_str(), "AAAATTGCCTGATGCGCTACGCTTATCAGGC") != 0) return;
  // printf("processing macro node AAAATTGCCTGATGCGCTACGCTTATCAGGC, key = %lu\n", key);

  int64_t  pc = 0, sc = 0;
  uint64_t null_prefix_id;
  uint64_t null_suffix_id;
  std::sort(value.begin(), value.end(), MN_comp);               // sort value ... prefixes stored before suffixes

  for (uint64_t i = 0; i < value.size(); ++ i) {
    if (value[i].isPrefix) {     // node is prefix
       if (value[i].affix.size() > 0) pc += value[i].count.second; else null_prefix_id = i;
    } else {                     // node is suffix
       if (value[i].affix.size() > 0) sc += value[i].count.second; else null_suffix_id = i;
  } }

  value[null_prefix_id].count = {1, std::max(sc - pc, 0L)};     // count and coverage for null prefix
  value[null_suffix_id].count = {1, std::max(pc - sc, 0L)};     // count and coverage for null suffix

  std::vector<uint64_t> indices(value.size());
  std::iota(indices.begin(), indices.end(), 0);
  std::sort(indices.begin(), indices.end(), Comp_rev(value));

  uint64_t wire_idx = 0, num_wires = 0;
  uint64_t top_prefix = 0, top_suffix = null_prefix_id + 1;
  uint64_t last_prefix_id = ULLONG_MAX, prefix_begin = ULLONG_MAX;

  int64_t  var_p = 0, var_s = 0, offset_in_suffix = 0;
  int64_t  leftover = sc + value[null_suffix_id].count.second;

  while (leftover > 0) {
    uint64_t prefix_id = indices[top_prefix];
    uint64_t suffix_id = indices[top_suffix];
    int64_t count = std::min( (value[prefix_id].count.second - var_p), (value[suffix_id].count.second - var_s) );
    WireMap->AsyncApply(handle, key, UpdateWireMap, wire_idx, suffix_id, offset_in_suffix, count);

    if (last_prefix_id != prefix_id) {prefix_begin = wire_idx; last_prefix_id = prefix_id;}

    wire_idx  ++;
    num_wires ++;
    var_p    += count;
    var_s    += count;
    leftover -= count;
    offset_in_suffix += count;

    if (var_p == value[prefix_id].count.second) {
       value[prefix_id].num_wires  = num_wires;
       value[prefix_id].wire_index = prefix_begin;
       var_p      = 0;
       num_wires  = 0;
       top_prefix ++;
    }

    if (var_s == value[suffix_id].count.second) {
       offset_in_suffix = 0;
       var_s      = 0;
       top_suffix ++;
} } }

} // namespace agile::workflow3
