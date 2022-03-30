#ifndef GLOBALIDS_H_
#define GLOBALIDS_H_

#include "shad/data_structures/hashmap.h"
#include "shad/extensions/data_types/data_types.h"

namespace agile::workflow2 {

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
    uint64_t src;     // global id of src
    uint64_t dst;     // global id of dst
    double   weight;
    TYPES    type;

    Edge () {
      src    = shad::data_types::kNullValue<uint64_t>;
      dst    = shad::data_types::kNullValue<uint64_t>;
      weight = shad::data_types::kNullValue<double>;
      type   = TYPES::NONE;
    }

    Edge (uint64_t src_, uint64_t dst_, double weight_, TYPES type_) {
      src    = src_;
      dst    = dst_;
      weight = weight_;
      type   = type_;
    }
};


using EdgeType = shad::Array<Edge>;
using EdgeOID  = shad::ObjectIdentifier<EdgeType>;

using VertexType = shad::Array<Vertex>;                // index == vertex GLBID
using VertexOID  = shad::ObjectIdentifier<VertexType>;

using GlobalIDType = shad::Hashmap<uint64_t, Vertex, shad::MemCmp<uint64_t>, globalIdInserter<Vertex> >;
using GlobalIDOID  = shad::ObjectIdentifier<GlobalIDType>;

} // namespace agile::workflow2

#endif // GLOBALIDS_H
