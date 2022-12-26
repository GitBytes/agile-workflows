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


void ProcessMacroNode(const uint64_t & key, std::vector<MacroNode> & value, Args_t & args) {
  uint64_t mnLength = args.mnLength;

  for (auto node : value) {
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

    if (kmer > key) return;             // node has    prefix/suffix > key, so don't process
  }

  // iterate_and_pack(key, node, args);    // node has no prefix/suffix > key, so process
}

} // namespace agile::workflow3

/*
void iterate_and_pack_mn (uint64_t & key, MacroNode & node, Args_t args) {
  auto MNMap = args.MNMap_OID;
  auto Contigs = args.Contigs_OID;
 
  for (uint64_t itr_p = 0; itr_p < node.prefix_begin_info.size(); ++ itr_p) {     // for each prefix
    if (node.prefix_begin_info[k].num_wires == 0) continue;     // no wires

    MnodeInfo search_pred_param;
    if ( (node.prefixes[itr_p].size() > 0) && (! node.prefixes_terminal[itr_p]) )
       search_pred_param = retrieve_mn_pinfo(node.prefixes[itr_p], node.k_1_mer);

    for (uint64_t t = 0; t < node.prefix_begin_info[k].num_wires; ++ t) {         // .. for each wire

      itr_s = node.wiring_info[node.prefix_begin_info[k].prefix_pos + t].suffix_id;
      int64_t count = node.wiring_info[node.prefix_begin_info[k].prefix_pos + t].count;

      if (node.prefixes_terminal[itr_p] && node.suffixes_terminal[itr_s]) {
         BasePairVector partial_contig = node.prefixes[itr_p];
         partial_contig.append(node.k_1_mer);
         partial_contig.append(node.suffixes[itr_s]);
         local_contig_list.push_back(partial_contig);

      } else {
         MnodeInfo search_succ_param;

         if ( (node.suffixes[itr_s].size() > 0) && (! node.suffixes_terminal[itr_s]) )
            search_succ_param = retrieve_mn_sinfo(node.suffixes[itr_s], node.k_1_mer);

         bool new_node_type;
         bool self_loop_0 = search_pred_param.search_mn == key;
         bool self_loop_1 = search_succ_param.search_mn == key;

         if ( (! node.prefixes_terminal[itr_p]) && (! self_loop_0) ) {     // new terminal value
            if      (node.suffixes_terminal[itr_s]) new_node_type = true;
            else if (self_loop_1                  ) new_node_type = true;
            else                                    new_node_type = false;

            mn_nodes_per_proc[retrieve_proc_id(search_pred_param.search_mn)].push_back (
                 TransferNode{
                     search_pred_param.search_mn,
                     search_pred_param.search_ext,
                     node.suffixes[itr_s],
                     std::make_pair( std::min(node.prefix_count[itr_p].first, node.suffix_count[itr_s].first), count ),
                     new_node_type,
                     (KdirType) P
                 }
            );
         }

         if ( (! node.suffixes_terminal[itr_s]) && (! self_loop_1) ) {     // new terminal value
            if      (node.prefixes_terminal[itr_p]) new_node_type = true;
            else if (self_loop_0                  ) new_node_type = true;
            else                                    new_node_type = false;

            mn_nodes_per_proc[retrieve_proc_id(search_succ_param.search_mn)].push_back (
                 TransferNode{
                     search_succ_param.search_mn,
                     search_succ_param.search_ext,
                     node.prefixes[itr_p],
                     std::make_pair( std::min(node.prefix_count[itr_p].first, node.suffix_count[itr_s].first), count),
                     new_node_type,
                     (KdirType) S
                 }
            );
} } } }  }
*/
