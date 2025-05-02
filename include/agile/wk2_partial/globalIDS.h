#ifndef GLOBALIDS_H_
#define GLOBALIDS_H_

#include "shad/data_structures/array.h"
#include "shad/data_structures/hashmap.h"
#include "shad/extensions/data_types/data_types.h"

#include "agile/wk2_partial/graphTypes.h"

namespace agile::wk2_partial {

template <typename T>
struct globalIdInserter {
  globalIdInserter() : counter(0lu) { }

  bool operator()(T *const lhs, const T &rhs, bool same_key) {
    if (same_key) {     // entry in hashmap, increment edges
       lhs->edges += rhs.edges;
    } else {            // entry not in hashmap, assign next local id
       T temp = rhs;
       temp.id = counter ++;
       * lhs = std::move(temp);
    }

    return true;
  }

  bool Insert(T *const lhs, const T &rhs, bool same_key) {
    if (same_key) {     // entry in hashmap, increment edges
       lhs->edges += rhs.edges;
    } else {            // entry not in hashmap, assign next local id
       T temp = rhs;
       temp.id = counter ++;
       * lhs = std::move(temp);
    }

    return true;
  }

  std::atomic<uint64_t> counter;
};


// Exclusive scan for vertex class array
template <typename VTYPE>
static void exclusiveRecursiveScan(Handle & handle, uint64_t pos, VTYPE & elem, uint64_t & ndx, uint64_t & oid) {
  using arrayOID = shad::ObjectIdentifier<shad::Array<VTYPE>>;
  auto arrayPtr  = shad::Array<VTYPE>::GetPtr((arrayOID) oid);

  uint64_t size = arrayPtr->Size();
  uint64_t nelems = arrayPtr->getNElems();
  std::vector<VTYPE> * data = arrayPtr->getData();

  // if not the last set, spawn next scan
  // ... next ndx is this ndx + # edges of last vertex in set 
  if (pos + nelems < size) {
     uint64_t my_ndx = ndx + (* data)[nelems - 1].edges;
     arrayPtr->AsyncApply(handle, pos + nelems, exclusiveRecursiveScan<VTYPE>, my_ndx, oid);
  }

  for (uint64_t i = nelems - 1; i > 0; i --) (* data)[i].edges = (* data)[i - 1].edges + ndx;
  (* data)[0].edges = ndx;
}


template <typename VTYPE>
void exclusiveScanVertices(uint64_t oid) {
  using arrayOID = shad::ObjectIdentifier<shad::Array<VTYPE>>;
  auto arrayPtr  = shad::Array<VTYPE>::GetPtr((arrayOID) oid);

  auto localInclusiveScan = [](Handle & handle, const uint64_t & oid) {
    auto arrayPtr = shad::Array<VTYPE>::GetPtr((arrayOID) oid);
    std::vector<VTYPE> * data = arrayPtr->getData();

    uint64_t nelems = arrayPtr->getNElems();
    for (uint64_t i = 1; i < nelems; i ++) (* data)[i].edges += (* data)[i - 1].edges;
  };

  Handle handle;
  shad::rt::asyncExecuteOnAll(handle, localInclusiveScan, oid);
  shad::rt::waitForCompletion(handle);

  uint64_t ndx = 0;
  arrayPtr->AsyncApply(handle, 0, exclusiveRecursiveScan<VTYPE>, ndx, oid);
  shad::rt::waitForCompletion(handle);
}


class Vertex {          // used by both GlobalIDS and Vertices
  public:
    uint64_t id;        // GlobalIDS: global id ... Vertices: vertex id
    uint64_t edges;     // GlobalIDS: number of edges ... Vertices: start index in Edges
    TYPES    type;

    Vertex () {
      id    = shad::data_types::kNullValue<uint64_t>;
      edges = shad::data_types::kNullValue<uint64_t>;
      type  = TYPES::NONE;
    }

    Vertex (uint64_t id_, uint64_t edges_, TYPES type_) {
      id    = id_;
      edges = edges_;
      type  = type_;
    }
};

class Edge {
  public:
    uint64_t src;     // vertex id of src
    uint64_t dst;     // vertex id of dst
    double   weight;
    TYPES    type;
    TYPES    src_type;
    TYPES    dst_type;
    uint64_t src_glbid;
    uint64_t dst_glbid;

    Edge () {
      src       = shad::data_types::kNullValue<uint64_t>;
      dst       = shad::data_types::kNullValue<uint64_t>;
      weight    = shad::data_types::kNullValue<double>;
      type      = TYPES::NONE;
      src_type  = TYPES::NONE;
      dst_type  = TYPES::NONE;
      src_glbid = shad::data_types::kNullValue<uint64_t>;
      dst_glbid = shad::data_types::kNullValue<uint64_t>;
    }

    Edge (uint64_t src_, uint64_t dst_, double weight_, TYPES type_,
         TYPES src_type_, TYPES dst_type_, uint64_t src_glbid_, uint64_t dst_glbid_) {
      src       = src_;
      dst       = dst_;
      weight    = weight_;
      type      = type_;
      src_type  = src_type_;
      dst_type  = dst_type_;
      src_glbid = src_glbid_;
      dst_glbid = dst_glbid_;
    }
};

using EdgeType = shad::Array<Edge>;
using EdgeOID  = shad::ObjectIdentifier<EdgeType>;

using VertexType = shad::Array<Vertex>;                              // index == vertex glbid
using VertexOID  = shad::ObjectIdentifier<VertexType>;

using GlobalIDType = shad::Hashmap<uint64_t, Vertex, shad::MemCmp<uint64_t>, globalIdInserter<Vertex> >;
using GlobalIDOID  = shad::ObjectIdentifier<GlobalIDType>;

} // namespace agile::wk2_partial

#endif // GLOBALIDS_H
