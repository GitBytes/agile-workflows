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

namespace agile::workflow3 {

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
};

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

using KMapType    = shad::Hashmap<uint64_t, uint64_t, shad::MemCmp<uint64_t>, KMapInserter<uint64_t>>;
using KMapOID     = shad::ObjectIdentifier<KMapType>;
using MNMapType   = shad::Multimap<uint64_t, MacroNode>;
using MNMapOID    = shad::ObjectIdentifier<MNMapType>;
using WireMapType = shad::Multimap<uint64_t, WireNode>;
using WireMapOID  = shad::ObjectIdentifier<WireMapType>;

bool MN_comp(MacroNode &, MacroNode &);
void InitialMacroNodeWire(Handle &, const uint64_t &, std::vector<MacroNode> &, Args_t &);

} // namespace agile::workflow3

#endif  // GRAPH_H
