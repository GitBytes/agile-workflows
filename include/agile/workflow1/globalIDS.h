#ifndef GLOBALIDS_H_
#define GLOBALIDS_H_

#include "shad/data_structures/hashmap.h"
#include "shad/extensions/data_types/data_types.h"

namespace agile::workflow1 {

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

using VertexType = shad::Array<Vertex>;                // index == vertex glbid
using VertexOID  = shad::ObjectIdentifier<VertexType>;

using GlobalIDType = shad::Hashmap<uint64_t, Vertex, shad::MemCmp<uint64_t>, globalIdInserter<Vertex> >;
using GlobalIDOID  = shad::ObjectIdentifier<GlobalIDType>;

} // namespace agile::workflow1

#endif // GLOBALIDS_H
