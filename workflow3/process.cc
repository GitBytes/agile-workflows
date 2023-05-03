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
// return leading base pairs in word; extract_pred_word(word, 2, 8), returns ______CT
uint64_t extract_pred_word(uint64_t word, uint64_t pred_size, uint64_t word_size) {
  uint64_t remove = word_size - pred_size;               // remove 2 base pairs from word
  uint64_t mask   = pred_mask(pred_size, word_size);     // mask = 11110000
  return (word & mask) >> (remove * SIZE_BP);            // (CCTA & 11110000) >> 4 = __CT
}


// word: __CTGTCA
//
// return trailing base pairs in word; extract_succ_word(word, 2, 8), returns ______CA
uint64_t extract_succ_word(uint64_t word, uint64_t suff_size, uint64_t word_size) {
  return word & succ_mask(suff_size);
}


MNInfo get_prefix_merge_info(uint64_t key, BasePairVector & affix, uint64_t mnLength) {
  uint64_t new_key;
  BasePairVector new_affix;
  uint64_t size = affix.size();

  if (size > mnLength) {
     // affix     = AACAGCAGGAAGGCACCGAAGATATACAGGATCCAGTCG
     // key       =                                        AACTGCGAAATTAGCCAGCTGCCAGTGAAGA
     // new key   = AACAGCAGGAAGGCACCGAAGATATACAGGA
     // new_affix =                                TCCAGTCGAACTGCGAAATTAGCCAGCTGCCAGTGAAGA
     uint64_t rem = size - mnLength;
     new_key = affix.vec_[0] >> ((BP_PER_WORD - mnLength) * SIZE_BP);
     new_affix.extract_succ2(affix, rem);
     new_affix.append(BasePairVector(key, mnLength));
  } else if (size == mnLength) {
     new_key   = affix.vec_[0];
     new_affix = BasePairVector(key, mnLength);

  } else {                                                         // affix : ___AAGT; key = _GGTCATA
     new_key = affix.vec_[0] << (SIZE_BP * (mnLength - size));     // __AAGT << (3 * 2) = _AAGT___
     new_key = new_key | (key >> (size * SIZE_BP));                // _AAGT___ | (_GGTCATA >> 4) = _AAGTGGT
     new_affix = BasePairVector(key & succ_mask(size), size);      // _GGTCATA & 00001111 = ____CATA
  }

  return MNInfo{new_key, new_affix};
}


MNInfo get_suffix_merge_info(uint64_t key, BasePairVector & affix, uint64_t mnLength) {
  uint64_t new_key;
  BasePairVector new_affix;
  uint64_t size = affix.size();

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
     new_key = key & succ_mask(rem);                                        // _GGTCATA & 00000111 = _____ATA
     for (uint64_t i = 0; i < size; ++ i)                                   // _____ATA + AAGT = _ATAAAGT
       new_key = (new_key << SIZE_BP) + affix[i];
     new_affix = BasePairVector(key >> (rem * SIZE_BP), size);              // _GGTCATA >> 3 = ____GGTC
  }

  return MNInfo{new_key, new_affix};
}


void walk(std::string & cstring, int64_t freq, int64_t offset_in_prefix,
          uint64_t key, std::vector<MacroNode> & macroNodes, MacroNode & node, const Args_t & args) {
  auto MNMap   = MNMapType::GetPtr((MNMapOID) args.MNMap_OID);
  auto WireMap = WireMapType::GetPtr((WireMapOID) args.WireMap_OID);

  uint64_t offset = 0;
  WireMapType::LookupResult wireEntry;     // get wireNodes
  WireMap->Lookup(key, & wireEntry);
  std::vector<WireNode> & wireNodes = wireEntry.value;

  for (uint64_t i = 0; i < node.num_wires; offset += wireNodes[node.wire_index + i].count, ++ i) {
    uint64_t sid             = wireNodes[node.wire_index + i].sid;
    int64_t  count           = wireNodes[node.wire_index + i].count;
    int64_t  offset_in_suffix = wireNodes[node.wire_index + i].offset;
   
    if ( (offset + count <= offset_in_prefix) || (offset > offset_in_prefix + freq) ) continue;
    int64_t offset_in_wire = (offset_in_prefix <= offset) ? 0 : offset_in_prefix - offset;

    int64_t next_offset  = offset_in_suffix + offset_in_wire;
    int64_t freq_in_wire = std::min(freq, (count - offset_in_wire));

    std::string my_cstring = cstring;
    my_cstring.append( macroNodes[sid].affix.to_string() );

    if (my_cstring.size() > 20000) {     // ... ... output partial contig to avoid recursion limit
       std::string name = ">contig_l_" + std::to_string(my_cstring.size());
       printf("%s\n%s\n", name.c_str(), my_cstring.c_str());

    } else if (macroNodes[sid].isTerminal) {                         // ... all done

       if (my_cstring.size() > CONTIG_LENGTH_THRESHOLD) {     // ... ... output contig if longer than threshold
          std::string name = ">contig_l_" + std::to_string(my_cstring.size());
          printf("%s\n%s\n", name.c_str(), my_cstring.c_str());
       }

    } else {                                                  // ... continue walk

       MNInfo next_macro_node_info;                           // ... ... get key and affix for next macro node
       next_macro_node_info = get_suffix_merge_info(key, macroNodes[sid].affix, args.mnLength);

       uint64_t next_key = next_macro_node_info.key;
       BasePairVector & next_affix = next_macro_node_info.affix;

       MNMapType::LookupResult next_macroNodeEntry;           // ... ... get next macro node
       MNMap->Lookup(next_key, & next_macroNodeEntry);
       std::vector<MacroNode> & next_macroNodes = next_macroNodeEntry.value;

       MacroNode next_node;
       bool found = false;
       for (auto & node : next_macroNodes)
           if (next_affix == node.affix) {next_node = node; found = true; break;}

       // recursively go to next macro node in this walk
         walk(my_cstring, freq_in_wire, next_offset, next_key, next_macroNodes, next_node, args);
    }

    freq -= freq_in_wire;
} }


void ProcessMacroNode(Handle & handle, const uint64_t & key, std::vector<MacroNode> & macroNodes, Args_t & args) {
  uint64_t mnLength   = args.mnLength;
  auto WireMap        = WireMapType::GetPtr((WireMapOID) args.WireMap_OID);
  auto ModifiedNodes  = ModifiedMapType::GetPtr((ModifiedMapOID) args.ModifiedNodes_OID);
  auto ProcessedNodes = IntSet::GetPtr((IntSetOID) args.ProcessedNodes_OID);

  for (auto node : macroNodes) {
    if (node.isTerminal) continue;     // skip terminals
    uint64_t kmer, size = node.affix.size();

    if (size < mnLength) {
       uint64_t rem  = mnLength - size;
       uint64_t extract = node.affix.extract_pred(size);
       if (node.isPrefix) kmer =  extract_pred_word(key, rem, mnLength) | (extract << (rem * 2));
       else               kmer = (extract_succ_word(key, rem, mnLength) << (size * 2)) | extract;
    } else {
       if (node.isPrefix) kmer = node.affix.extract_pred(mnLength);
       else               kmer = node.affix.extract_succ(mnLength);
    }

// node has prefix/suffix > key, so don't process
    if (kmer > key) return;
  }

// *** node has no prefix/suffix > key, so process *** //
  ProcessedNodes->AsyncInsert(handle, key);

  WireMapType::LookupResult wireEntry;     // get wireNodes
  WireMap->Lookup(key, & wireEntry);
  std::vector<WireNode> & wireNodes = wireEntry.value;

  for (auto node : macroNodes) {           // node has no prefix/suffix > key, so process
    if (! node.isPrefix) break;            // ... process only prefixes, prefixes listed first
    if (node.num_wires == 0) continue;     // ... skip prefixes with no wires

    MNInfo prefix_merge_info;              // ... get key and affix for node that prefix modifies
    if ( (node.affix.size() > 0) && (! node.isTerminal) ) {
       prefix_merge_info = get_prefix_merge_info(key, node.affix, mnLength);
    } else {
       prefix_merge_info.key = 0;
       prefix_merge_info.affix = BasePairVector();
    }

    for (int t = 0; t < node.num_wires; ++ t) {                // ... for each wire attached to the node
      uint64_t sid  = wireNodes[node.wire_index + t].sid;
      int64_t count = wireNodes[node.wire_index + t].count;
      MacroNode & suffix = macroNodes[sid];                    // ... ... suffix attached

      if (node.isTerminal && suffix.isTerminal) {              // ... ... string is terminated on both sides
         uint64_t size = node.affix.size() + mnLength + suffix.affix.size();

         if (size > CONTIG_LENGTH_THRESHOLD) {          // ... ... ... output contig
            std::string name = ">contig_l_" + std::to_string(size);

            BasePairVector contig = node.affix;
            contig.append( BasePairVector(key, mnLength) );
            contig.append(suffix.affix);
            printf("%s\n%s\n", name.c_str(), contig.to_string().c_str());
         }

      } else {                                                 // ... ... string is open on at least one side

         MNInfo suffix_merge_info;                             // ... ... ... get key and affix for node that
         if ( (suffix.affix.size() > 0) && (! suffix.isTerminal) ) {                    // ... suffix modifies
            suffix_merge_info = get_suffix_merge_info(key, suffix.affix, mnLength);
         } else {
            suffix_merge_info.key = 0;
            suffix_merge_info.affix = BasePairVector();
         }

         bool self_loop_0 = (prefix_merge_info.key == key);
         bool self_loop_1 = (suffix_merge_info.key == key);

         if ( (! node.isTerminal) && (! self_loop_0) ) {       // ... ... ... prefix is not a terminal or a self_loop
            ModifiedNode tmp;                                  // ... ... ... ... prefix modifies suffix node
            tmp.old_affix = prefix_merge_info.affix;
            tmp.new_affix = prefix_merge_info.affix;

            (tmp.new_affix).append(suffix.affix);
            tmp.isPrefix   = false;
            tmp.isTerminal = suffix.isTerminal || self_loop_1;
            tmp.num_wires  = 0;
            tmp.wire_index = 0;
            tmp.count      = {std::min(node.count.first, suffix.count.first), count };
            ModifiedNodes->AsyncInsert(handle, prefix_merge_info.key, tmp);
         }

         if ( (! suffix.isTerminal) && (! self_loop_1) ) {     // ... ... ... suffix is not a terminal or a self_loop
            ModifiedNode tmp;                                  // ... ... ... ... suffix modifies prefix node
            tmp.old_affix = suffix_merge_info.affix;
            tmp.new_affix = node.affix;

            (tmp.new_affix).append(suffix_merge_info.affix);
            tmp.isPrefix   = true;
            tmp.isTerminal = node.isTerminal || self_loop_0;
            tmp.num_wires  = 0;
            tmp.wire_index = 0;
            tmp.count      = {std::min(node.count.first, suffix.count.first), count };
            ModifiedNodes->AsyncInsert(handle, suffix_merge_info.key, tmp);
} } } }  }


void ProcessContigs(const uint64_t & key, std::vector<MacroNode> & value, Args_t & args) {

  for (auto node : value) {
    // node is NOT a begin kmer ==> node is NOT the prefix terminal with frequency > 0
    if (! (node.isPrefix && node.isTerminal && node.count.second > 0)) continue;

    std::string cstring = node.affix.to_string();
    cstring.append( BasePairVector(key, args.mnLength).to_string() );
    walk(cstring, node.count.second, 0, key, value, node, args);
} }

} // namespace agile::workflow3
