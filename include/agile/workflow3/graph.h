//===------------------------------------------------------------*- C++ -*-===//
//
//                            The AGILE Workflows
//
//===----------------------------------------------------------------------===//
// ** Pre-Copyright Notice
//
// This computer software was prepared by Battelle Memorial Institute,
// hereinafter the Contractor, under Contract No. DE-AC05-76RL01830 with the
// Department of Energy (DOE). All rights in the computer software are reserved
// by DOE on behalf of the United States Government and the Contractor as
// provided in the Contract. You are authorized to use this computer software
// for Governmental purposes but it is not to be released or distributed to the
// public. NEITHER THE GOVERNMENT NOR THE CONTRACTOR MAKES ANY WARRANTY, EXPRESS
// OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE. This
// notice including this sentence must appear on any copies of this computer
// software.
//
// ** Disclaimer Notice
//
// This material was prepared as an account of work sponsored by an agency of
// the United States Government. Neither the United States Government nor the
// United States Department of Energy, nor Battelle, nor any of their employees,
// nor any jurisdiction or organization that has cooperated in the development
// of these materials, makes any warranty, express or implied, or assumes any
// legal liability or responsibility for the accuracy, completeness, or
// usefulness or any information, apparatus, product, software, or process
// disclosed, or represents that its use would not infringe privately owned
// rights. Reference herein to any specific commercial product, process, or
// service by trade name, trademark, manufacturer, or otherwise does not
// necessarily constitute or imply its endorsement, recommendation, or favoring
// by the United States Government or any agency thereof, or Battelle Memorial
// Institute. The views and opinions of authors expressed herein do not
// necessarily state or reflect those of the United States Government or any
// agency thereof.
//
//                    PACIFIC NORTHWEST NATIONAL LABORATORY
//                                 operated by
//                                   BATTELLE
//                                   for the
//                      UNITED STATES DEPARTMENT OF ENERGY
//                       under Contract DE-AC05-76RL01830
//===----------------------------------------------------------------------===//

#ifndef GRAPH_H_
#define GRAPH_H_

// pred_mask(3, 5)
// shift_right = (32 - 3) * 2 = 58
// shift_left  = (5 - 3) * 2 = 4
//
// 11111...11111 >> 58 = 000...00000111111
// 00...00111111 <<  4 = 000...01111110000
inline uint64_t pred_mask(uint64_t pred_size, uint64_t word_size) {
  uint64_t shift_right = (BP_PER_WORD - pred_size) * SIZE_BP;
  uint64_t shift_left  = (word_size - pred_size) * SIZE_BP;
  return ((~0UL) >> shift_right) << shift_left;
}

namespace agile::workflow3 {

class BasePairVector {
 public:
  uint64_t size_;
  uint64_t vec_[SIZE_BPV];
  
  BasePairVector() {size_ = 0;}

// base pairs: AAGTCCTACG
// stored    : AAGT CCTA __CG
// word      :  0    1    2
//
// return leading base pairs in word; extract_pred(1, 2, 4), returns __CC
  uint64_t extract_pred(uint64_t word, uint64_t pred_size, uint64_t word_size) {
    assert(pred_size <= word_size);                        // # BP in pred <= # BP in word;

    uint64_t remove = word_size - pred_size;               // remove 2 base pairs from word
    uint64_t mask   = pred_mask(pred_size, word_size);     // mask = 11110000
    return (word & mask) >> (remove * SIZE_BP);            // (CCTA & 11110000) >> 4 = __CC
  }

// base pairs: AAGTCCTACG
// stored    : AAGT CCTA __CG
// word      :  0    1    2
//
// return leading base pairs in vector; extract(3), returns _AAG
  uint64_t extract(uint64_t pred_size) {
    assert (pred_size < BP_PER_WORD);

    uint64_t word_size = (size_ < BP_PER_WORD) ? size_ : BP_PER_WORD;
    return extract_pred(vec_[0], pred_size, word_size);
  } 

// base pairs: AAGTCCTACG
// stored    : AAGT CCTA __CG
// word      :  0    1    2
//
// return trailing base pairs in vector; extract_succ(3), returns _ACG
  uint64_t extract_succ(uint64_t suff_size) {
    assert (suff_size < BP_PER_WORD);

    uint64_t mod_size = (size_ % BP_PER_WORD);
    uint64_t last_word_full = (mod_size == 0);
    uint64_t num_words = (size_ / BP_PER_WORD) + ((last_word_full) ? 0 : 1);

    if (last_word_full || (suff_size <= mod_size)) {                // suffix is all in last word
       uint64_t mask = ((1UL) << (suff_size * SIZE_BP)) - 1;        // ... 1 << (3 * 2) = 1000000 - 1 = 0111111
       return vec_[num_words - 1] & mask;

    } else {                                                        // suffix is partially in previous word
       uint64_t last_word = mod_size;                               // ... last word has 2 bases
       uint64_t prev_word = suff_size - mod_size;                   // ... prevous word has 1 base
       uint64_t last_mask = ((1UL) << last_word * SIZE_BP) - 1;     // ... 1 << (1 * 2) = 100 - 1 = 00011
       uint64_t prev_mask = ((1UL) << prev_word * SIZE_BP) - 1;     // ... 1 << (1 * 2) = 100 - 1 = 00011
       uint64_t kmer = vec_[num_words - 2] & prev_mask;;            // ... suffix in previous word = __A

       // shift kmer left by # base pairs in previous word and OR in last word; __A << (1 * 2) | __CG
       kmer = (kmer << (last_word * SIZE_BP)) | (vec_[num_words - 1] & last_mask);
       return kmer;
  } }

  void push_back(uint64_t val) {
    assert(size_ < SIZE_BPV);

    size_ ++;
    uint64_t word = (size_ - 1) / BP_PER_WORD;      // new base pair is in word
    vec_[word] = (vec_[word] << SIZE_BP) | val;     // shift word to left and OR in val
  }

  void append(const BasePairVector & bpv) {
    for (uint64_t i = 0; i < bpv.size(); ++ i) push_back(bpv[i]);
  }

  uint64_t size() const { return size_; }
  
// base pairs: AAGTCCTACG
// stored    : AAGT CCTA __CG
// word      :  0    1    2
// offset    : 0123 0123   01
//
// shrink number of base pairs to new_size; resize(6), leaves vec_ = AAGT __CC, size_ = 6
  void resize(uint64_t new_size) {
    assert(new_size < size_);

    uint64_t word   = (new_size - 1) / BP_PER_WORD;                            // last base pair is in word 1
    uint64_t offset = (new_size - 1) % BP_PER_WORD;                            // last base pair has offset 1
    uint64_t last_word = size_ / BP_PER_WORD;                                  // last word is word 2
    uint64_t base_pairs_in_word = (word < last_word) ? BP_PER_WORD : size_ % BP_PER_WORD;

    vec_[word] = extract_pred(vec_[word], offset + 1, base_pairs_in_word);     // CCTA ==> __CC
    size_ = new_size;
  }

  void print() {
    uint64_t num_full_words   = size_ / BP_PER_WORD;
    uint64_t extra_base_pairs = size_ % BP_PER_WORD;
    uint64_t last_mask = (extra_base_pairs) ? pred_mask(1, extra_base_pairs) : 0;

    for (uint64_t i = 0; i < num_full_words; ++ i) {
      uint64_t mask = pred_mask(1, BP_PER_WORD);

      for (uint64_t j = BP_PER_WORD; j > 0; -- j) {
        uint64_t BP = (vec_[i] & mask) >> ((j - 1) * SIZE_BP);
        printf("%c", EL_TO_CHAR(BP));
        mask >>= SIZE_BP;
    } }

    for (uint64_t j = extra_base_pairs; j > 0; -- j) {
      uint64_t BP = (vec_[num_full_words] & last_mask) >> ((j - 1) * SIZE_BP);
      printf("%c", EL_TO_CHAR(BP));
      last_mask >>= SIZE_BP;
    }

    printf("\n");
  }

// base pairs: AAGTCCTACG
// stored    : AAGT CCTA __CG          (assume 4 base pairs per word)
// position  : 0123 4567   89
// word      :  0    1    2
// offset    : 0123 0123   01
//
// return base pair at position; [2], returns G
  uint64_t operator [] (uint64_t pos) const {
    assert(pos < size_);                          // position must be less than size_

    uint64_t word = pos / BP_PER_WORD;            // position 2 is in word 0
    uint64_t offset = pos % BP_PER_WORD;          // position 2 has offset 2
    uint64_t last_word = size_ / BP_PER_WORD;     // last word is 2
    uint64_t base_pairs_in_word = (word < last_word) ? BP_PER_WORD : size_ % BP_PER_WORD;

    uint64_t shift = (base_pairs_in_word - offset - 1) * SIZE_BP;     // (4 - 2 - 1) * 2 = 2
    return (vec_[word] >> shift) & (0x3);                             // (AAGT >> 2) & 00000011 = ___G
  }
};     // BasePairVector

class MacroNode {
  public:
    uint64_t mnode;          // key, KMER_LENGTH - 1 bases
    char baseAcid;           // leading or trailing base
    bool isPrefix;           // prefix - true, suffix - false
    bool terminal;
    int64_t  num_wires;
    uint64_t prefix_begin;
    std::pair<int64_t, int64_t> count;

    MacroNode () {
      mnode    = ULLONG_MAX;
      baseAcid = '*';
      isPrefix = false;
      terminal = false;
      prefix_begin = 0;
      num_wires = 0;
      count = {-1, -1};
    }
};     // MacroNode

class WireNode {
  public:
    uint64_t sid;          // suffix id in MacroNode value
    int64_t  offset;
    int64_t  count;

    WireNode () {
      sid    = 0;
      offset = 0;
      count  = 0;
    }

    WireNode (uint64_t sid_, int64_t offset_, int64_t count_) {
      sid    = sid_;
      offset = offset_;
      count  = count_;
    }
};     // WireNode

template <typename T>
struct KMapInserter {

  bool operator()(T * const lhs, const T & rhs, bool same_key) {
    if (same_key) {     // entry in hashmap, increment value
       * lhs += rhs;
    } else {            // entry not in hashmap, set value
       * lhs = std::move(rhs);
    }

    return true;
  }

  bool Insert(T * const lhs, const T & rhs, bool same_key) {
    if (same_key) {     // entry in hashmap, increment value
       * lhs += rhs;
    } else {            // entry not in hashmap, set value
       * lhs = std::move(rhs);
    }

    return true;
  }
};

using KMapType    = shad::Hashmap<uint64_t, uint64_t, shad::MemCmp<uint64_t>, KMapInserter<uint64_t>>;
using KMapOID     = shad::ObjectIdentifier<KMapType>;
using MNMapType   = shad::Multimap<uint64_t, MacroNode>;
using MNMapOID    = shad::ObjectIdentifier<MNMapType>;
using WireMapType = shad::Multimap<uint64_t, WireNode>;
using WireMapOID  = shad::ObjectIdentifier<WireMapType>;

bool MN_comp(MacroNode &, MacroNode &);
void InitialMacroNodeWire(Handle &, const uint64_t &, std::vector<MacroNode> &, Args_t &);

bool inline operator == (const BasePairVector & k1, const BasePairVector & k2) {
  if (k1.size_ != k2.size_) return false;
  uint64_t num_words = (k1.size_ / BP_PER_WORD) + ((k1.size_ % BP_PER_WORD) ? 1 : 0);

  for (uint64_t i = 0; i < num_words; ++ i)
    if (k1.vec_[i] != k2.vec_[i]) return false;

  return true;
}

} // namespace agile::workflow3

#endif  // GRAPH_H
