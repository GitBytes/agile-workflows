#ifndef GRAPH_H_
#define GRAPH_H_

#include <cstdint>
#include <limits>
#include <vector>

#include "shad/data_structures/hashmap.h"
#include "shad/extensions/data_types/data_types.h"

#include "agile/workflow2/main.h"
#include "agile/workflow2/graphTypes.h"
#include "agile/workflow2/globalIDS.h"

#define UINT   shad::data_types::UINT
#define DOUBLE shad::data_types::DOUBLE
#define USDATE shad::data_types::USDATE
#define ENCODE shad::data_types::encode

namespace agile::workflow2 {

class VertexL {          // used by both GlobalIDS and Vertices
  public:
    uint64_t label;
    uint64_t id;        // GlobalIDS: global id ... Vertices: vertex id
    uint64_t edges;     // GlobalIDS: number of edges ... Vertices: start index in Edges
    TYPES    type;
    int mate;
    int indx;

    VertexL () {
      label = shad::data_types::kNullValue<uint64_t>;
      id    = shad::data_types::kNullValue<uint64_t>;
      edges = shad::data_types::kNullValue<uint64_t>;
      type  = TYPES::NONE;
      mate  = -1;
      indx  = -1;
    }

    VertexL (uint64_t label_, uint64_t id_, uint64_t edges_, TYPES type_, int mate_, int indx_) {
      label = label_;
      id    = id_;
      edges = edges_;
      type  = type_;
      mate  = mate_;
      indx  = indx_;
    }
};

class GraphL {
  public:
    shad::Array<VertexL>::ObjectID vertexOID;
    shad::Array<Edge>::ObjectID edgeOID;
    uint64_t vertexNumber;
    uint64_t a_num_vertices;
    uint64_t edgeNumber;

    GraphL()
      : vertexOID(shad::rt::Locality(), 0),
        edgeOID(shad::rt::Locality(), 0),
        vertexNumber(0),
        a_num_vertices(0),
        edgeNumber(0) {}

    shad::Array<VertexL>::ShadArrayPtr vertexPtr() const {
      return shad::Array<VertexL>::GetPtr(vertexOID);
    }

    shad::Array<Edge>::ShadArrayPtr edgePtr() const {
      return shad::Array<Edge>::GetPtr(edgeOID);
    }

};


class PersonVertex {
  public:
    uint64_t id;
    uint64_t glbid;

    PersonVertex () {
      id    = shad::data_types::kNullValue<uint64_t>;
      glbid = shad::data_types::kNullValue<uint64_t>;
    }

    PersonVertex (std::vector <std::string> & tokens) {
      id    = ENCODE<uint64_t, std::string, UINT>(tokens[1]);
      glbid = shad::data_types::kNullValue<uint64_t>;
    }

    uint64_t key() { return id; }
};

class ForumEventVertex {
  public:
    uint64_t id;
    uint64_t forum;
    time_t   date;
    uint64_t glbid;

    ForumEventVertex () {
      id    = shad::data_types::kNullValue<uint64_t>;
      forum = shad::data_types::kNullValue<uint64_t>;
      date  = shad::data_types::kNullValue<time_t>;
      glbid = shad::data_types::kNullValue<uint64_t>;
    }

    ForumEventVertex (std::vector <std::string> & tokens) {
      id    = ENCODE<uint64_t, std::string, UINT>  (tokens[4]);
      forum = ENCODE<uint64_t, std::string, UINT>  (tokens[3]);
      date  = ENCODE<time_t,   std::string, USDATE>(tokens[7]);
      glbid = shad::data_types::kNullValue<uint64_t>;
    }

    uint64_t key() { return id; }
};

class ForumVertex {
  public:
    uint64_t id;
    uint64_t glbid; 

    ForumVertex () {
      id    = shad::data_types::kNullValue<uint64_t>;
      glbid = shad::data_types::kNullValue<uint64_t>;
    }

    ForumVertex (std::vector <std::string> & tokens) {
      id   = ENCODE<uint64_t, std::string, UINT>(tokens[3]);
      glbid = shad::data_types::kNullValue<uint64_t>;
    }

    uint64_t key() { return id; }
};

class PublicationVertex {
  public:
    uint64_t id;
    time_t   date;
    uint64_t glbid;

    PublicationVertex () {
      id    = shad::data_types::kNullValue<uint64_t>;
      date  = shad::data_types::kNullValue<time_t>;
      glbid = shad::data_types::kNullValue<uint64_t>;
    }

    PublicationVertex (std::vector <std::string> & tokens) {
      id    = ENCODE<uint64_t, std::string, UINT>  (tokens[5]);
      date  = ENCODE<time_t,   std::string, USDATE>(tokens[7]);
      glbid = shad::data_types::kNullValue<uint64_t>;
    }

    uint64_t key() { return id; }
};

class TopicVertex {
  public:
    uint64_t id;
    double   lat;
    double   lon;
    uint64_t glbid;

    TopicVertex () {
      id    = shad::data_types::kNullValue<uint64_t>;
      lat   = shad::data_types::kNullValue<double>;
      lon   = shad::data_types::kNullValue<double>;
      glbid = shad::data_types::kNullValue<uint64_t>;
    }

    TopicVertex (std::vector <std::string> & tokens) {
      id    = ENCODE<uint64_t, std::string, UINT>  (tokens[6]);
      lat   = ENCODE<double,   std::string, DOUBLE>(tokens[8]);
      lon   = ENCODE<double,   std::string, DOUBLE>(tokens[9]);
      glbid = shad::data_types::kNullValue<uint64_t>;
    }

    uint64_t key() { return id; }
};

class PurchaseEdge {
  public:
    uint64_t buyer;            // vertex id
    uint64_t seller;           // vertex id
    uint64_t product;
    time_t   date;
    TYPES    src_type;
    TYPES    dst_type;

    PurchaseEdge () {
      buyer   = shad::data_types::kNullValue<uint64_t>;
      seller  = shad::data_types::kNullValue<uint64_t>;
      product = shad::data_types::kNullValue<uint64_t>;
      date    = shad::data_types::kNullValue<time_t>;
      src_type = TYPES::NONE;
      dst_type = TYPES::NONE;
    }

    PurchaseEdge (std::vector <std::string> & tokens) {
      buyer    = ENCODE<uint64_t, std::string, UINT>  (tokens[2]);
      seller   = ENCODE<uint64_t, std::string, UINT>  (tokens[1]);
      product  = ENCODE<uint64_t, std::string, UINT>  (tokens[6]);
      date     = ENCODE<time_t,   std::string, USDATE>(tokens[7]);
      src_type = TYPES::PERSON;
      dst_type = TYPES::PERSON;
    }

    uint64_t key() { return buyer; }
    uint64_t src() { return buyer; }
    uint64_t dst() { return seller; }
};

class SaleEdge {
  public:
    uint64_t seller;           // vertex id
    uint64_t buyer;            // vertex id
    uint64_t product;
    time_t   date;
    TYPES    src_type;
    TYPES    dst_type;

    SaleEdge () {
      seller   = shad::data_types::kNullValue<uint64_t>;
      buyer    = shad::data_types::kNullValue<uint64_t>;
      product  = shad::data_types::kNullValue<uint64_t>;
      date     = shad::data_types::kNullValue<time_t>;
      src_type = TYPES::NONE;
      dst_type = TYPES::NONE;
    }

    SaleEdge (std::vector <std::string> & tokens) {
      seller   = ENCODE<uint64_t, std::string, UINT>  (tokens[1]);
      buyer    = ENCODE<uint64_t, std::string, UINT>  (tokens[2]);
      product  = ENCODE<uint64_t, std::string, UINT>  (tokens[6]);
      date     = ENCODE<time_t,   std::string, USDATE>(tokens[7]);
      src_type = TYPES::PERSON;
      dst_type = TYPES::PERSON;
    }

    uint64_t key() { return seller; }
    uint64_t src() { return seller; }
    uint64_t dst() { return buyer; }
};

class AuthorEdge {
  public:
    uint64_t author;     // vertex id
    uint64_t item;       // vertex id
    TYPES    src_type;
    TYPES    dst_type;

    AuthorEdge () {
      author   = shad::data_types::kNullValue<uint64_t>;
      item     = shad::data_types::kNullValue<uint64_t>;
      src_type = TYPES::NONE;
      dst_type = TYPES::NONE;
    }

    AuthorEdge (std::vector <std::string> & tokens) {
      if (tokens[4] != "") {
         author   = ENCODE<uint64_t, std::string, UINT>(tokens[1]);
         item     = ENCODE<uint64_t, std::string, UINT>(tokens[4]);
         src_type = TYPES::PERSON;
         dst_type = TYPES::FORUMEVENT;
      } else {
         author   = ENCODE<uint64_t, std::string, UINT>(tokens[1]);
         item     = ENCODE<uint64_t, std::string, UINT>(tokens[5]);
         src_type = TYPES::PERSON;
         dst_type = TYPES::PUBLICATION;
    } }

    uint64_t key() { return author; }
    uint64_t src() { return author; }
    uint64_t dst() { return item; }
};

class IncludesEdge {
  public:
    uint64_t forum;            // vertex id
    uint64_t forum_event;      // vertex id
    TYPES    src_type;
    TYPES    dst_type;

    IncludesEdge () {
      forum       = shad::data_types::kNullValue<uint64_t>;
      forum_event = shad::data_types::kNullValue<uint64_t>;
      src_type    = TYPES::NONE;
      dst_type    = TYPES::NONE;
    }

    IncludesEdge (std::vector <std::string> & tokens) {
      forum       = ENCODE<uint64_t, std::string, UINT>(tokens[3]);
      forum_event = ENCODE<uint64_t, std::string, UINT>(tokens[4]);
      src_type    = TYPES::FORUM;
      dst_type    = TYPES::FORUMEVENT;
    }

    uint64_t key() { return forum; }
    uint64_t src() { return forum; }
    uint64_t dst() { return forum_event; }
};

class HasTopicEdge {
  public:
    uint64_t item;      // vertex id
    uint64_t topic;     // vertex id
    TYPES    src_type;
    TYPES    dst_type;
 
    HasTopicEdge () {
      item     = shad::data_types::kNullValue<uint64_t>;
      topic    = shad::data_types::kNullValue<uint64_t>;
      src_type = TYPES::NONE;
      dst_type = TYPES::NONE;
    }

    HasTopicEdge (std::vector <std::string> & tokens) {
      if (tokens[3] != "") {
         item     = ENCODE<uint64_t, std::string, UINT>(tokens[3]);
         topic    = ENCODE<uint64_t, std::string, UINT>(tokens[6]);
         src_type = TYPES::FORUM;
         dst_type = TYPES::TOPIC;
      } else if (tokens[4] != "") {
         item     = ENCODE<uint64_t, std::string, UINT>(tokens[4]);
         topic    = ENCODE<uint64_t, std::string, UINT>(tokens[6]);
         src_type = TYPES::FORUMEVENT;
         dst_type = TYPES::TOPIC;
      } else {
         item     = ENCODE<uint64_t, std::string, UINT>(tokens[5]);
         topic    = ENCODE<uint64_t, std::string, UINT>(tokens[6]);
         src_type = TYPES::PUBLICATION;
         dst_type = TYPES::TOPIC;
    } }

    uint64_t key() { return item; }
    uint64_t src() { return item; }
    uint64_t dst() { return topic; }
};

class HasOrgEdge {
  public:
    uint64_t publication;      // vertex id
    uint64_t organization;     // vertex id
    TYPES    src_type;
    TYPES    dst_type;

  public:
    HasOrgEdge () {
      publication  = shad::data_types::kNullValue<uint64_t>;
      organization = shad::data_types::kNullValue<uint64_t>;
      src_type     = TYPES::NONE;
      dst_type     = TYPES::NONE;
    }

    HasOrgEdge (std::vector <std::string> & tokens) {
      publication  = ENCODE<uint64_t, std::string, UINT>(tokens[5]);
      organization = ENCODE<uint64_t, std::string, UINT>(tokens[6]);
      src_type     = TYPES::PUBLICATION;
      dst_type     = TYPES::TOPIC;
    }

    uint64_t key() { return publication; }
    uint64_t src() { return publication; }
    uint64_t dst() { return organization; }
};

using PersonVertexType = shad::Hashmap<uint64_t, PersonVertex>;
using PersonVertexOID  = shad::ObjectIdentifier<PersonVertexType>;

using ForumEventVertexType = shad::Hashmap<uint64_t, ForumEventVertex>;
using ForumEventVertexOID  = shad::ObjectIdentifier<ForumEventVertexType>;

using ForumVertexType = shad::Hashmap<uint64_t, ForumVertex>;
using ForumVertexOID  = shad::ObjectIdentifier<ForumVertexType>;

using PublicationVertexType = shad::Hashmap<uint64_t, PublicationVertex>;
using PublicationVertexOID  = shad::ObjectIdentifier<PublicationVertexType>;

using TopicVertexType = shad::Hashmap<uint64_t, TopicVertex>;
using TopicVertexOID  = shad::ObjectIdentifier<TopicVertexType>;

using PurchaseEdgeType = shad::Multimap<uint64_t, PurchaseEdge>;
using PurchaseEdgeOID  = shad::ObjectIdentifier<PurchaseEdgeType>;

using SaleEdgeType = shad::Multimap<uint64_t, SaleEdge>;
using SaleEdgeOID  = shad::ObjectIdentifier<SaleEdgeType>;

using AuthorEdgeType = shad::Multimap<uint64_t, AuthorEdge>;
using AuthorEdgeOID  = shad::ObjectIdentifier<AuthorEdgeType>;

using IncludesEdgeType = shad::Multimap<uint64_t, IncludesEdge>;
using IncludesEdgeOID  = shad::ObjectIdentifier<IncludesEdgeType>;

using HasTopicEdgeType = shad::Multimap<uint64_t, HasTopicEdge>;
using HasTopicEdgeOID  = shad::ObjectIdentifier<HasTopicEdgeType>;

using HasOrgEdgeType = shad::Multimap<uint64_t, HasOrgEdge>;
using HasOrgEdgeOID  = shad::ObjectIdentifier<HasOrgEdgeType>;

using VertexLType = shad::Array<VertexL>;                // index == vertex glbid
using VertexLOID  = shad::ObjectIdentifier<VertexLType>;

} // namespace agile::workflow2

#endif // GRAPH_H
