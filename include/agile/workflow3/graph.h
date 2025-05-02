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

inline uint64_t succ_mask(uint64_t succ_size) {
  return ((~0UL) >> (SIZE_BP * (BP_PER_WORD - succ_size)));
}

namespace agile::workflow3 {

class BasePairVector {
 public:
  uint64_t size_;
  uint64_t vec_[SIZE_BPV];
  
  BasePairVector() {size_ = 0; vec_[0] = 0; vec_[1] = 0;}
  BasePairVector(uint64_t word, uint64_t size) {size_ = size; vec_[0] = word; vec_[1] = 0;}

// base pairs: AAGTCCTACG
// stored    : AAGT CCTA __CG
// word      :  0    1    2
//
// return leading base pairs in vector; extract(3), returns _AAG
  uint64_t extract_pred(uint64_t pred_size) {
    assert (pred_size < BP_PER_WORD);

    uint64_t word_size = (size_ < BP_PER_WORD) ? size_ : BP_PER_WORD;
    uint64_t remove    = word_size - pred_size;               // remove 2 base pairs from word
    uint64_t mask      = pred_mask(pred_size, word_size);     // mask = 11110000
    return (vec_[0] & mask) >> (remove * SIZE_BP);            // (CCTA & 11110000) >> 4 = __CC
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

    if (last_word_full || (suff_size <= mod_size)) {                   // suffix is all in last word
       return vec_[num_words - 1] & succ_mask(suff_size);

    } else {                                                           // suffix is partially in previous word
       uint64_t last_word = mod_size;                                  // ... last word has 2 bases
       uint64_t prev_word = suff_size - mod_size;                      // ... prevous word has 1 base
       uint64_t kmer = vec_[num_words - 2] & succ_mask(prev_word);     // ... suffix in previous word = __A

       // shift kmer left by # base pairs in previous word and OR in last word; __A << (1 * 2) | __CG
       kmer = (kmer << (last_word * SIZE_BP)) | (vec_[num_words - 1] & succ_mask(last_word));
       return kmer;
  } }

// base pairs: AAGTCCTACG
// stored    : AAGT CCTA __CG
// word      :  0    1    2
//
// return trailing base pairs in vector; extract_succ(3), returns _ACG
  void extract_succ2(BasePairVector & affix, uint64_t suff_size) {
    size_ = 0;
    uint64_t start = affix.size() - suff_size;
    for (uint64_t i = start; i < affix.size(); ++ i) this->push_back(affix[i]);
  }

  void push_back(uint64_t val) {                    // val is a single base pair
    if (size_ == SIZE_BPV * BP_PER_WORD) {
       printf("push back ERROR %lu\n", size_);
    } else {
       size_ ++;
       uint64_t word = (size_ - 1) / BP_PER_WORD;      // new base pair is in word
       vec_[word] = (vec_[word] << SIZE_BP) | val;     // shift word to left and OR in val
  } }

  void append(const BasePairVector & bpv) {
    for (uint64_t i = 0; i < bpv.size(); ++ i) push_back(bpv[i]);
  }

  uint64_t size() const { return size_; }
  
  void print(FILE * ff) {
    for (uint64_t i = 0; i < size_; ++ i)
      fprintf(ff, "%c", EL_TO_CHAR((* this)[i]));
  }


  std::string to_string() {
    std::string str(size_, ' ');
    for (uint64_t i = 0; i < size_; ++ i)
       str[i] = EL_TO_CHAR((* this)[i]);
    return str;
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
    BasePairVector affix;                  // leading or trailing bases
    bool isPrefix;                         // true if prefix; false if suffix
    bool isTerminal;                       // true if terminal (size == 0); otherwise, false (size > 0)
    int64_t  num_wires;                    // number of wires (prefixInfo.num_wires in original code)
    uint64_t wire_index;                   // index in this key's WireMap value vector
    std::pair<int64_t, int64_t> count;     // prefix_count or suffix_count

    MacroNode () {
      affix = BasePairVector();
      isPrefix   = false;
      isTerminal = false;
      num_wires  = 0;
      wire_index = 0;
      count      = {-1, -1};
    }
};     // MacroNode

class WireNode {
  public:
    uint64_t sid;     // suffix id in MacroNode value
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

class ModifiedNode {
  public:
    BasePairVector old_affix;              // macro node's old affix (==> node to be replaced)
    BasePairVector new_affix;              // macro node's new affix
    bool isPrefix;
    bool isTerminal;
    uint64_t num_wires;
    uint64_t wire_index;
    std::pair<int64_t, int64_t> count;     // macro node's new count

    ModifiedNode () {
      old_affix  = BasePairVector();
      new_affix  = BasePairVector();
      isPrefix   = false;
      isTerminal = false;
      num_wires  = 0;
      wire_index = 0;
      count      = {-1, -1};
    }
};

inline bool operator==(const BasePairVector & k1, const BasePairVector & k2) {
    if (k1.size() != k2.size()) return false;

    for (uint64_t i = 0; i < k1.size(); ++ i)
      if (k1[i] != k2[i]) return false;

    return true;
}

inline bool operator!=(const BasePairVector & k1, const BasePairVector & k2) { return ! (k1 == k2); }

inline bool operator>(const BasePairVector & k1, const BasePairVector & k2) {
    if (k1.size() != k2.size()) return k1.size() > k2.size();

    for (uint64_t i = 0; i < k1.size(); ++ i)
      if (k1[i] != k2[i]) return k1[i] > k2[i];

    return false;
}

class Comp_rev {
  const std::vector<MacroNode> & _v;

  public:
  Comp_rev(const std::vector<MacroNode> & v) : _v(v) {}

  bool operator()(size_t i, size_t j) {
    if (_v[i].isPrefix     != _v[j].isPrefix)     return _v[i].isPrefix;     // prefixes stored before suffixes
    if (_v[i].count.second != _v[j].count.second) return (_v[i].count.second > _v[j].count.second);
    if (_v[i].count.first  != _v[j].count.first)  return (_v[i].count.first  > _v[j].count.first);
    return _v[i].affix > _v[j].affix;
  }

};

struct MNInfo {
  uint64_t key;
  BasePairVector affix;
};

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

using KMapType        = shad::Hashmap<uint64_t, uint64_t, shad::MemCmp<uint64_t>, KMapInserter<uint64_t>>;
using KMapOID         = shad::ObjectIdentifier<KMapType>;
using MNMapType       = shad::Multimap<uint64_t, MacroNode>;
using MNMapOID        = shad::ObjectIdentifier<MNMapType>;
using WireMapType     = shad::Multimap<uint64_t, WireNode>;
using WireMapOID      = shad::ObjectIdentifier<WireMapType>;
using ModifiedMapType = shad::Multimap<uint64_t, ModifiedNode>;
using ModifiedMapOID  = shad::ObjectIdentifier<ModifiedMapType>;
using ContigSetType   = shad::Set<BasePairVector>;
using ContigSetOID    = shad::ObjectIdentifier<ContigSetType>;
using ContigMapType   = shad::Hashmap<uint64_t, BasePairVector>;
using ContigMapOID    = shad::ObjectIdentifier<ContigMapType>;

bool MN_comp(MacroNode &, MacroNode &);
MNInfo get_suffix_merge_info(uint64_t, BasePairVector &, uint64_t);
MNInfo get_prefix_merge_info(uint64_t, BasePairVector &, uint64_t);
void ProcessContigs(const uint64_t &, std::vector<MacroNode> &, Args_t &);
void WireMacroNodes(Handle &, const uint64_t &, std::vector<MacroNode> &, Args_t &);
void ProcessMacroNode(Handle &, const uint64_t &, std::vector<MacroNode> &, Args_t &);
void ModifyMacroNode(Handle &, const uint64_t &, std::vector<ModifiedNode> &, Args_t &);
void Finish_MN_WireMaps(Handle &, const uint64_t &, std::vector<MacroNode> &, Args_t &);

} // namespace agile::workflow3

#endif  // GRAPH_H
