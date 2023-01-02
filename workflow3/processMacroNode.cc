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

// word: __CTGTCA
//
// return leading base pairs in word; extract_pred(word, 2, 8), returns ______CT
uint64_t extract_pred_word(uint64_t word, uint64_t pred_size, uint64_t word_size) {
  assert(pred_size <= word_size);                        // # BP in pred <= # BP in word;

  uint64_t remove = word_size - pred_size;               // remove 2 base pairs from word
  uint64_t mask   = pred_mask(pred_size, word_size);     // mask = 11110000
  return (word & mask) >> (remove * SIZE_BP);            // (CCTA & 11110000) >> 4 = __CT
}


// word: __CTGTCA
//
// return trailing base pairs in word; extract_succ(word, 2, 8), returns ______CA
uint64_t extract_succ_word(uint64_t word, uint64_t suff_size, uint64_t word_size) {
  assert (suff_size < word_size);

  uint64_t mask = ((1UL) << (suff_size * SIZE_BP)) - 1;        // ... 1 << (3 * 2) = 1000000 - 1 = 0111111
  return word & mask;
}


// return the new key and prefix for the macro node to be merged with this affix and key
MNInfo get_prefix_merge_info(uint64_t key, BasePairVector & affix, uint64_t mnLength) {
  assert(affix.size() < mnLength);
  BasePairVector new_affix;
  uint64_t size = affix.size();
  uint64_t rem = mnLength - size;                          // remainder = 7 - 4 = 3
  uint64_t mask = ((1UL) << (size * SIZE_BP)) - 1;         // mask = 00001111

// affix : ___AAGT; key = _GGTCATA
  uint64_t new_key = affix.vec_[0] << (rem * SIZE_BP);     // __AAGT << (3 * 2) = _AAGT___
  new_key = new_key | (key >> (size * SIZE_BP));           // _AAGT___ | (_GGTCATA >> 4) = _AAGTGGT
  new_affix = BasePairVector(key & mask, size);            // _GGTCATA & 00001111 = ____CATA

  return MNInfo{new_key, new_affix};
}


// return the new key and suffix for the macro node to be merged with this key and affix
MNInfo get_suffix_merge_info(uint64_t key, BasePairVector & affix, uint64_t mnLength) {
  BasePairVector new_affix;
  uint64_t new_key, size = affix.size();

// affix : AAGTCCTA ______CG; key = _GGTCATA
  if (size > mnLength) {
     uint64_t rem = size - mnLength;                                        // remainder = 10 - 7 = 3
     new_key = affix.extract_succ(mnLength);                                // _TCCTACG
     new_affix = BasePairVector(key, mnLength);                             // _GGTCATA
     for (uint64_t i = 0; i < rem; ++ i) new_affix.push_back(affix[i]);     // _GGTCAT + AAG = GGTCATA ______AG

// affix : _AAGTCCT; key = _GGTCATA
  } else if (size == mnLength) {
     new_key   = affix.vec_[0];                                             // _AAGTCCT
     new_affix = BasePairVector(key, size);                                 // _GGTCATA

// affix : ____AAGT; key = _GGTCATA
  } else {
     uint64_t rem = mnLength - size;                                        // remainder = 7 - 4 = 3
     new_key = key & (((1UL) << (rem * SIZE_BP)) - 1);                      // _GGTCATA & 00000111 = _____ATA
     for (uint64_t i = 0; i < size; ++ i)                                   // _____ATA + AAGT = _ATAAAGT
       new_key = (new_key << SIZE_BP) + affix[i];
     new_affix = BasePairVector(key >> (rem * SIZE_BP), size);              // _GGTCATA >> 3 = ____GGTC
  }

  return MNInfo{new_key, new_affix};
}


void ProcessMacroNode(const uint64_t & key, std::vector<MacroNode> & macroNodes, Args_t & args) {
  uint64_t mnLength = args.mnLength;
  auto WireMap = WireMapType::GetPtr((WireMapOID) args.WireMap_OID);
  auto ContigVector = ContigVectorType::GetPtr((ContigVectorOID) args.ContigVector_OID);


  for (auto node : macroNodes) {
    if (node.isTerminal) continue;     // skip terminals
    uint64_t kmer, size = node.affix.size();

    if (size < mnLength) {
       uint64_t rem  = mnLength - size;
       uint64_t extract = node.affix.extract(size);
       if (node.isPrefix) kmer =  extract_pred_word(key, rem, mnLength) | (extract << (rem * 2));
       else               kmer = (extract_succ_word(key, rem, mnLength) << (size * 2)) | extract;
    } else {
       if (node.isPrefix) kmer = node.affix.extract(mnLength);
       else               kmer = node.affix.extract_succ(mnLength);
    }

// node has prefix/suffix > key, so don't process
    if (kmer > key) return;
  }

// node has no prefix/suffix > key, so process
  WireMapType::LookupResult wireEntry;     // get wireNodes
  WireMap->Lookup(key, & wireEntry);
  std::vector<WireNode> & wireNodes = wireEntry.value;

  for (auto node : macroNodes) {           // node has no prefix/suffix > key, so process
    if (! node.isPrefix) break;            // ... process only prefixes, prefixes listed first
    if (node.num_wires == 0) continue;     // ... skip prefixes with no wires

    MNInfo prefix_merge_info;;
    if ( (node.affix.size() > 0) && (! node.isTerminal) )
       prefix_merge_info = get_prefix_merge_info(key, node.affix, mnLength);

    for (int t = 0; t < node.num_wires; ++ t) {
      uint64_t sid  = wireNodes[node.wire_index + t].sid;
      int64_t count = wireNodes[node.wire_index + t].count;
      MacroNode & suffix = macroNodes[sid];

      if (node.isTerminal && suffix.isTerminal) {
         BasePairVector contig = node.affix;
         BasePairVector key_bvp(key, mnLength);

         contig.append(key_bvp);
         contig.append(suffix.affix);
         ContigVector->PushBack(contig);
         printf("saved contig\n");

      } else {
         MNInfo suffix_merge_info;
         if ( (suffix.affix.size() > 0) && (! suffix.isTerminal) )
            suffix_merge_info = get_suffix_merge_info(key, suffix.affix, mnLength);

         bool self_loop_0 = (prefix_merge_info.key == key);
         bool self_loop_1 = (suffix_merge_info.key == key);

         if ( (! node.isTerminal) && (! self_loop_0) ) {
            printf("transferred pred\n");
            // mn_nodes_per_proc[retrieve_proc_id(prefix_merge_info.key)].push_back(
                // TransferNode { prefix_merge_info.key,
                               // prefix_merge_info.affix,
                               // suffix.affix,
                               // std::make_pair( std::min(node.count.first, suffix.count.first), count ),
                               // (suffix.isTerminal || self_loop_1)     // isTerminal
                               // true                                   // isPrefix
                             // }
                // );
         }

         if ( (! suffix.isTerminal) && (! self_loop_1) ) {
            printf("transferred succ\n");
            // mn_nodes_per_proc[retrieve_proc_id(suffix_merge_info.key)].push_back(
                // TransferNode { suffix_merge_info.key,
                               // suffix_merge_info.affix,
                               // node.affix,
                               // std::make_pair( std::min(node.count.first, suffix.count.first), count ),
                               // (node.isTerminal || self_loop_0)     // isTerminal
                               // false                                // isPrefix
                             // }
                // );
 }  }

 printf("deleted macro node\n");
} } }

} // namespace agile::workflow3
