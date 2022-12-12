#define SIZE_BP 2          // bits per base pair
#define SIZE_BPV 256       // size of base pair vector in 64 bit words
#define BP_PER_WORD 32     // number of base pairs per word = 64 / 2

// for BP_PER_WORD = 5, pred_mask(3) = (11 11 11 11 11) << ((5 - 3) * 2) = 11 11 11 00 00
inline const pred_mask(uint64_t pred_size) {
  return (~0UL) << ((BP_PER_WORD - pred_size) * SIZE_BP);
}


class BasePairVector {
 public:
  uint64_t size_;
  uint64_t[MAX_BPV] vec_;
  
  BasePairVector() {size_ = 0;}
  BasePairVector(const BasePairVector & bv) {vec_ = bv.vec_; size_ = bv.size_;}

  bool operator == (const BasePairVector & k1, const BasePairVector & k2) {
    if (k1.size_ != k2.size_) return false;
    uint64_t num_words = (k1.size_ / BP_PER_WORD) + ((k1.size_ % BP_PER_WORD) ? 1 : 0);

    for (uint64_t i = 0; i < num_words; ++ i)
      if (k1.vec_[i] != k2.vec_[i]) return false;

    return true;
  }

// base pairs: AAGTCCTACG
// stored    : AAGT CCTA __CG          (assume 4 base pairs per word)
// position  : 0123 4567   89
// word      :  0    1    2
// offset    : 0123 0123   01
//
// return base pair at position; [2], returns G
  BasePair operator [] (uint64_t pos) const {
    assert(pos < size_);                               // position must be less than size_

    uint64_t word   = pos / BP_PER_WORD;               // position 2 is in word 0
    uint64_t offset = pos % BP_PER_WORD;               // position 2 has offset 2
    uint64_t last_full_word = size_ / BP_PER_WORD;     // last full word is word 1, so word 0 has 4 base pairs
    uint64_t base_pairs_in_word = (word <= last_full_word) ? BP_PER_WORD : size_ % BP_PER_WORD;

    uint64_t shift = (base_pairs_in_word - offset - 1) * SIZE_BP;     // (4 - 2 - 1) * 2 = 2
    return (vec_[word] >> shift) & (0x3);                             // (AAGT >> 2) & 00000011 = ___G
  };

  void push_back(BasePair val) {
    assert(size_ < MAX_BPV)                         // # of base pairs limited to MAX_BPV

    size_ ++;
    uint64_t word = (size_ - 1) / BP_PER_WORD;      // new base pair is in word
    vec_[word] = (vec_[word] << SIZE_BP) | val;     // shift word to left and OR in val
  }

  void append(const BasePairVector & bpv) {
    for (uint64_t i = 0; i < bpv.size(); ++ i) push_back(bpv[i]);
  }

// base pairs: AAGTCCTACG
// stored    : AAGT CCTA __CG
// word      :  0    1    2
//
// return leading base pairs in word; extract_pred(1, 2, 4), returns __CC
  uint64_t extract_pred(uint64_t word, uint64_t pred_size, uint64_t word_size) {
    assert(pred_size <= word_size)                 // # BP in pred <= # BP in word

    uint64_t remove = word_size - pred_size;       // remove 2 base pairs from word
    uint64_t mask   = pred_mask(pred_size);        // mask = 11110000
    return (word & mask) >> (remove * SIZEBP);     // (CCTA & 11110000) >> 4 = __CC
  }


  uint64_t size() const { return vec_.size(); }
  
// base pairs: AAGTCCTACG
// stored    : AAGT CCTA __CG
// word      :  0    1    2
// offset    : 0123 0123   01
//
// shrink number of base pairs to new_size; resize(6), leaves vec_ = AAGT __CC, size_ = 6
  void resize(uint64_t new_size) {
    assert(new_size < size_);

    uint64_t word   = new_size / BP_PER_WORD;          // last base pair is in word 1
    uint64_t offset = new_size % BP_PER_WORD;          // last base pair has offset 1

    uint64_t pred_size = offset + 1;                   // retain leading (offset + 1) base pairs of word
    uint64_t last_full_word = size_ / BP_PER_WORD;     // last full word is word 1, so word 1 has 4 base pairs
    uint64_t word_size = (word <= last_full_word) ? BP_PER_WORD : size_ % BP_PER_WORD;

    vec_[word] = extract_pred(vec_[word], pred_size, word_size);     // CCTA ==> __CC
    size_ = new_size;
  }

// base pairs: AAGTCCTACG
// stored    : AAGT CCTA __CG
// word      :  0    1    2
//
// return leading base pairs in vector; extract(3), returns _AAG
  uint64_t extract(size_t pred_size) {
    assert (pred_size < nels_per_value);
    uint64_t word_size = std::min((size_, BP_PER_WORD);
    return extract_pred(vec_[0], pred_size, word_size);
  } 

  uint64_t extract_succ(size_t dist) {
    assert (dist < nels_per_value);
    if (size_ < nels_per_value) return mask(dist) & vec_[0];

    uint64_t idx_size = size_/nels_per_value;
    uint64_t idx_off  = size_%nels_per_value;
    int64_t  rem      = dist - idx_off;

      if (rem < 0) {
         return mask(dist) & vec_[idx_size];
      
      } else if (rem > 0) {
          uint64_t partial_kmer = mask(rem) & vec_[idx_size - 1];
          if (idx_off) return ( (partial_kmer << (idx_off * bsize) ) | vec_[idx_size]);
          else         return partial_kmer;

      } else {
          return vec_[idx_size];
  }   }

};  // BasePairVector
