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

#ifndef MAIN_H_
#define MAIN_H_

#include <string>
#include <numeric>
#include <math.h>
#include <limits.h>

#include "shad/data_structures/set.h"
#include "shad/data_structures/array.h"
#include "shad/data_structures/vector.h"
#include "shad/data_structures/hashmap.h"
#include "shad/data_structures/multimap.h"
#include "shad/extensions/data_types/data_types.h"

#define TINY   5000
#define SMALL  500000
#define MEDIUM 5000000
#define LARGE  50000000

#define UINT_BITS 64
#define SIZE_BP 2          // bits per base pair
#define SIZE_BPV 2         // size of base pair vector in 64 bit words
#define BP_PER_WORD 32     // number of base pairs per word = 64 / 2

namespace agile::workflow3 {

using Handle      = shad::rt::Handle;
using IntSet      = shad::Set<uint64_t>;
using IntArray    = shad::Array<int64_t>;
using IntSetOID   = shad::ObjectIdentifier<IntSet>;
using IntArrayOID = shad::ObjectIdentifier<IntArray>;

struct Args_t {
  uint64_t KMap_OID;
  uint64_t MNMap_OID;
  uint64_t WireMap_OID;
  uint64_t BucketCounts_OID;
  uint64_t ModifiedNodes_OID;
  uint64_t ProcessedNodes_OID;
  uint64_t ContigMap_OID;
  uint64_t PartialContigs_OID;
  uint64_t mnLength;
  uint64_t coverage;
  uint64_t min_index;
  uint64_t min_counts;
  char filename [120];
};

uint64_t CHAR_TO_EL(char x);
char EL_TO_CHAR(uint64_t x);
std::string kmer_string(uint64_t, uint64_t);
void int_fetch_add(Handle &, uint64_t, int64_t &, int64_t &);

void readFASTA(Handle &, const Args_t &);
void BucketCounts_(Handle &, const Args_t &);
void ConstructMacroNodes(Handle &, const Args_t &);
void DeleteMacroNode(Handle &, const uint64_t &, Args_t &);
void RewireMacroNode(Handle &, const uint64_t &, Args_t &);

} // namespace agile::workflow3

#endif  // MAIN_H
