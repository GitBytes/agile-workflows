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

uint64_t visit_value (uint64_t count, uint64_t coverage) {
  double ceil_val = (double) count / (double) coverage;
  return (uint64_t) ceil(ceil_val);
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


void Finish_MN_WireMaps(Handle & handle, const uint64_t & key, std::vector<MacroNode> & value, Args_t & args) {
  uint64_t size = value.size();
  auto MNMap = MNMapType::GetPtr((MNMapOID) args.MNMap_OID);
  auto WireMap = WireMapType::GetPtr((WireMapOID) args.WireMap_OID);

  MacroNode prefix_mn = MacroNode();                           // push null prefix and suffix onto MNMap
  MacroNode suffix_mn = MacroNode();
  prefix_mn.isPrefix = true;
  suffix_mn.isPrefix = false;
  prefix_mn.isTerminal = true;
  suffix_mn.isTerminal = true;
  MNMap->BufferedAsyncInsert(handle, key, prefix_mn);
  MNMap->BufferedAsyncInsert(handle, key, suffix_mn);

  for (uint64_t i = 0; i < size + 3; ++ i)                     // push default wire node onto WireMap
    WireMap->BufferedAsyncInsert(handle, key, WireNode());
}


void ModifyMN_(Handle & handle, const uint64_t & key,
     std::vector<MacroNode> & macroNodes, std::vector<ModifiedNode> * (& modifiedNodes), Args_t & args) {
  auto WireMap = WireMapType::GetPtr((WireMapOID) args.WireMap_OID);

  for (auto & mod : (* modifiedNodes))  {                           // for each modification
    bool found = false;

    for (uint64_t i = 0; i < macroNodes.size(); ++ i) {             // ... for each macro node
      if (mod.isPrefix != macroNodes[i].isPrefix) continue;         // ... ... affix types are not the same
      if (mod.old_affix != macroNodes[i].affix) continue;           // ... ... affixes are not the same
      found = true;                                                 // ... ... found affix in list

      if (macroNodes[i].isTerminal) {                               // ... ... affix is a terminal 
         MacroNode tmp;                                             // ... ... ... push new macro node
         tmp.affix      = mod.new_affix;
         tmp.isPrefix   = mod.isPrefix;
         tmp.isTerminal = mod.isTerminal;
         tmp.num_wires  = mod.num_wires;
         tmp.wire_index = mod.wire_index;
         tmp.count      = mod.count;
         macroNodes.push_back(tmp);                                 // ... append new macro node
         WireMap->BufferedAsyncInsert(handle, key, WireNode());     // ... extend wire map for key

      } else {                                                      // ... ... affix is not a terminal 
         macroNodes[i].affix      = mod.new_affix;                  // ... ... ... replace macro node
         macroNodes[i].isPrefix   = mod.isPrefix;
         macroNodes[i].isTerminal = mod.isTerminal;
         macroNodes[i].num_wires  = mod.num_wires;
         macroNodes[i].wire_index = mod.wire_index;
         macroNodes[i].count      = mod.count;
      }

      break;
    }

    if (! found) {                                                // ... affix is not in list
       MacroNode tmp;                                             // ... ... push new macro node
       tmp.affix      = mod.new_affix;
       tmp.isPrefix   = mod.isPrefix;
       tmp.isTerminal = mod.isTerminal;
       tmp.num_wires  = mod.num_wires;
       tmp.wire_index = mod.wire_index;
       tmp.count      = mod.count;
       macroNodes.push_back(tmp);                                 // ... append new macro node
       WireMap->BufferedAsyncInsert(handle, key, WireNode());     // ... extend wire map for key
} } }


void RewireMN_(Handle & handle, const uint64_t & key,
     std::vector<WireNode> & wireNodes, std::vector<MacroNode> * (& macroNodes), Args_t & args) {
  int64_t  pc = 0, sc = 0, index = -1;
  uint64_t top_prefix = 0, top_suffix = 0;
  uint64_t null_prefix_id, null_suffix_id;

  for (auto & node : (* macroNodes)) {
    index ++;
    if (node.isPrefix) {                            // node is prefix
       if (node.affix.size() == 0) null_prefix_id = index; else pc += node.count.second;
    } else {                                        // node is suffix
       if (top_suffix == 0) top_suffix = index;     // ... first suffix
       if (node.affix.size() == 0) null_suffix_id = index; else sc += node.count.second;
  } }

  (* macroNodes)[null_prefix_id].count = {1, std::max(sc - pc, 0L)};     // count and coverage for null prefix
  (* macroNodes)[null_suffix_id].count = {1, std::max(pc - sc, 0L)};     // count and coverage for null suffix

  std::vector<uint64_t> indices((* macroNodes).size());
  std::iota(indices.begin(), indices.end(), 0);
  std::sort(indices.begin(), indices.end(), Comp_rev(* macroNodes));

  uint64_t wire_idx = 0, num_wires = 0;
  uint64_t last_prefix_id = ULLONG_MAX, prefix_begin = ULLONG_MAX;

  int64_t  var_p = 0, var_s = 0, offset_in_suffix = 0;
  int64_t  leftover = sc + (* macroNodes)[null_suffix_id].count.second;

  while (leftover > 0) {
    uint64_t prefix_id = indices[top_prefix];
    uint64_t suffix_id = indices[top_suffix];
    int64_t  prefix_count = (* macroNodes)[prefix_id].count.second - var_p;
    int64_t  suffix_count = (* macroNodes)[suffix_id].count.second - var_s;
    int64_t  count = std::min(prefix_count, suffix_count);

    wireNodes[wire_idx].sid    =  suffix_id;
    wireNodes[wire_idx].offset = offset_in_suffix;
    wireNodes[wire_idx].count  = count;
    if (last_prefix_id != prefix_id) {prefix_begin = wire_idx; last_prefix_id = prefix_id;}

    wire_idx  ++;
    num_wires ++;
    var_p    += count;
    var_s    += count;
    leftover -= count;
    offset_in_suffix += count;

    if (var_p == (* macroNodes)[prefix_id].count.second) {
       (* macroNodes)[prefix_id].num_wires  = num_wires;
       (* macroNodes)[prefix_id].wire_index = prefix_begin;
       var_p      = 0;
       num_wires  = 0;
       top_prefix ++;
    }

    if (var_s == (* macroNodes)[suffix_id].count.second) {
       offset_in_suffix = 0;
       var_s      = 0;
       top_suffix ++;
} } }


void WireMacroNodes(Handle & handle, const uint64_t & key, std::vector<MacroNode> & value, Args_t & args) {
  std::vector<MacroNode> * tmp = & value;
  auto WireMap = WireMapType::GetPtr((WireMapOID) args.WireMap_OID);

  std::sort(value.begin(), value.end(), MN_comp);     // sort macro nodes
  WireMap->AsyncApply(handle, key, RewireMN_, tmp, args);
}


void DeleteMacroNode(Handle & handle, const uint64_t & key, Args_t & args) {
  auto MNMap = MNMapType::GetPtr((MNMapOID) args.MNMap_OID);
  auto WireMap = WireMapType::GetPtr((WireMapOID) args.WireMap_OID);

  MNMap->AsyncErase(handle, key);
  WireMap->AsyncErase(handle, key);
}


void ModifyMacroNode(Handle & handle, const uint64_t & key, std::vector<ModifiedNode> & value, Args_t & args) {
  auto MNMap = MNMapType::GetPtr((MNMapOID) args.MNMap_OID);

  std::vector<ModifiedNode> * tmp = & value;
  MNMap->AsyncApply(handle, key, ModifyMN_, tmp, args);
}


void RewireMacroNode(Handle & handle, const uint64_t & key, Args_t & args) {
  auto MNMap = MNMapType::GetPtr((MNMapOID) args.MNMap_OID);

  auto RewireLambda = [] (Handle & handle, const uint64_t & key, std::vector<MacroNode> & value, Args_t & args) {
    std::vector<MacroNode> * tmp = & value;
    auto WireMap = WireMapType::GetPtr((WireMapOID) args.WireMap_OID);

    std::sort(value.begin(), value.end(), MN_comp);
    WireMap->AsyncApply(handle, key, RewireMN_, tmp, args);
  };

  MNMap->AsyncApply(handle, key, RewireLambda, args);
}

} // namespace agile::workflow3
