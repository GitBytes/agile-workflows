#ifndef AGILE_WORKFLOW1_GRAPH_H
#define AGILE_WORKFLOW1_GRAPH_H

#include "shad/core/algorithm.h"
#include "shad/core/numeric.h"
#include "shad/data_structures/array.h"
#include "shad/data_structures/hashmap.h"
#include "shad/data_structures/multimap.h"
#include "shad/extensions/data_types/data_types.h"

namespace agile::workflow1 {

enum class TYPES {
  PERSON,
  FORUMEVENT,
  FORUM,
  PUBLICATION,
  TOPIC,
  PURCHASE,
  SALE,
  AUTHOR,
  OCCURSAT,
  HASTOPIC,
  HASORG,
  VERTEX,
  EDGE,
  NONE
};

class PersonVertex {
  public:
    uint64_t id;

    PersonVertex () {
      id = shad::data_types::kNullValue<uint64_t>;
    }

    PersonVertex (std::vector <std::string> & tokens) {
      id = shad::data_types::encode<uint64_t, std::string>(tokens[1], shad::data_types::UINT);
    }

    uint64_t get_key() { return id; }
};

class ForumEventVertex {
  public:
    uint64_t id;
    time_t date;

    ForumEventVertex () {
      id   = shad::data_types::kNullValue<uint64_t>;
      date = shad::data_types::kNullValue<time_t>;
    }

    ForumEventVertex (std::vector <std::string> & tokens) {
      id   = shad::data_types::encode<uint64_t, std::string>(tokens[4], shad::data_types::UINT);
      date = shad::data_types::encode<time_t, std::string>(tokens[7], shad::data_types::USDATE);
    }

    uint64_t get_key() { return id; }
};

class ForumVertex {
  public:
    uint64_t id;

    ForumVertex () {
      id = shad::data_types::kNullValue<uint64_t>;
    }

    ForumVertex (std::vector <std::string> & tokens) {
      id = shad::data_types::encode<uint64_t, std::string>(tokens[3], shad::data_types::UINT);
    }

    uint64_t get_key() { return id; }
};

class PublicationVertex {
  public:
    uint64_t id;
    time_t date;

    PublicationVertex () {
      id   = shad::data_types::kNullValue<uint64_t>;
      date = shad::data_types::kNullValue<time_t>;
    }

    PublicationVertex (std::vector <std::string> & tokens) {
      id   = shad::data_types::encode<uint64_t, std::string>(tokens[5], shad::data_types::UINT);
      date = shad::data_types::encode<time_t, std::string>(tokens[7], shad::data_types::USDATE);
    }

    uint64_t get_key() { return id; }
};

class TopicVertex {
  public:
    uint64_t id;
    double lat;
    double lon;

    TopicVertex () {
      id  = shad::data_types::kNullValue<uint64_t>;
      lat = shad::data_types::kNullValue<double>;
      lon = shad::data_types::kNullValue<double>;
    }

    TopicVertex (std::vector <std::string> & tokens) {
      id  = shad::data_types::encode<uint64_t, std::string>(tokens[6], shad::data_types::UINT);
      lat = shad::data_types::encode<double, std::string>(tokens[8], shad::data_types::DOUBLE);
      lon = shad::data_types::encode<double, std::string>(tokens[9], shad::data_types::DOUBLE);
    }

    uint64_t get_key() { return id; }
};

class PurchaseEdge {
  public:
    uint64_t buyer;
    uint64_t seller;
    uint64_t product;
    time_t date;

    PurchaseEdge () {
       buyer   = shad::data_types::kNullValue<uint64_t>;
       seller  = shad::data_types::kNullValue<uint64_t>;
       product = shad::data_types::kNullValue<uint64_t>;
       date    = shad::data_types::kNullValue<time_t>;
    }

    PurchaseEdge (std::vector <std::string> & tokens) {
      buyer   = shad::data_types::encode<uint64_t, std::string>(tokens[1], shad::data_types::UINT);
      seller  = shad::data_types::encode<uint64_t, std::string>(tokens[2], shad::data_types::UINT);
      product = shad::data_types::encode<uint64_t, std::string>(tokens[6], shad::data_types::UINT);
      date    = shad::data_types::encode<time_t, std::string>(tokens[7], shad::data_types::USDATE);
    }

    uint64_t get_key() { return buyer; }
    uint64_t get_src() { return buyer; }
    uint64_t get_dst() { return seller; }
};

class SaleEdge {
  public:
    uint64_t seller;
    uint64_t buyer;
    uint64_t product;
    time_t date;

    SaleEdge () {
       seller  = shad::data_types::kNullValue<uint64_t>;
       buyer   = shad::data_types::kNullValue<uint64_t>;
       product = shad::data_types::kNullValue<uint64_t>;
       date    = shad::data_types::kNullValue<time_t>;
    }

    SaleEdge (std::vector <std::string> & tokens) {
      seller  = shad::data_types::encode<uint64_t, std::string>(tokens[1], shad::data_types::UINT);
      buyer   = shad::data_types::encode<uint64_t, std::string>(tokens[2], shad::data_types::UINT);
      product = shad::data_types::encode<uint64_t, std::string>(tokens[6], shad::data_types::UINT);
      date    = shad::data_types::encode<time_t, std::string>(tokens[7], shad::data_types::USDATE);
    }

    uint64_t get_key() { return seller; }
    uint64_t get_src() { return seller; }
    uint64_t get_dst() { return buyer; }
};

class AuthorEdge {
  public:
    uint64_t author;
    uint64_t item;

    AuthorEdge () {
       author = shad::data_types::kNullValue<uint64_t>;
       item   = shad::data_types::kNullValue<uint64_t>;
    }

    AuthorEdge (std::vector <std::string> & tokens) {
      author = shad::data_types::encode<uint64_t, std::string>(tokens[1], shad::data_types::UINT);
      if (tokens[4] != "") item = shad::data_types::encode<uint64_t, std::string>(tokens[4], shad::data_types::UINT);
      else                 item = shad::data_types::encode<uint64_t, std::string>(tokens[5], shad::data_types::UINT);
    }

    uint64_t get_key() { return author; }
    uint64_t get_src() { return author; }
    uint64_t get_dst() { return item; }
};

class OccursAtEdge {
  public:
    uint64_t forum;
    uint64_t forum_event;

    OccursAtEdge () {
       forum_event = shad::data_types::kNullValue<uint64_t>;
       forum       = shad::data_types::kNullValue<uint64_t>;
    }

    OccursAtEdge (std::vector <std::string> & tokens) {
      forum_event = shad::data_types::encode<uint64_t, std::string>(tokens[4], shad::data_types::UINT);
      forum       = shad::data_types::encode<uint64_t, std::string>(tokens[3], shad::data_types::UINT);
    }

    uint64_t get_key() { return forum_event; }
    uint64_t get_src() { return forum_event; }
    uint64_t get_dst() { return forum; }
};

class HasTopicEdge {
  public:
    uint64_t item;
    uint64_t topic;

    HasTopicEdge () {
       item  = shad::data_types::kNullValue<uint64_t>;
       topic = shad::data_types::kNullValue<uint64_t>;
    }

    HasTopicEdge (std::vector <std::string> & tokens) {
      if      (tokens[3] != "") item = shad::data_types::encode<uint64_t, std::string>(tokens[3], shad::data_types::UINT);
      else if (tokens[4] != "") item = shad::data_types::encode<uint64_t, std::string>(tokens[4], shad::data_types::UINT);
      else                      item = shad::data_types::encode<uint64_t, std::string>(tokens[5], shad::data_types::UINT);
      topic = shad::data_types::encode<uint64_t, std::string>(tokens[6], shad::data_types::UINT);
    }

    uint64_t get_key() { return item; }
    uint64_t get_src() { return item; }
    uint64_t get_dst() { return topic; }
};

class HasOrgEdge {
  public:
    uint64_t publication;
    uint64_t organization;

    HasOrgEdge () {
       publication  = shad::data_types::kNullValue<uint64_t>;
       organization = shad::data_types::kNullValue<uint64_t>;
    }

    HasOrgEdge (std::vector <std::string> & tokens) {
      publication  = shad::data_types::encode<uint64_t, std::string>(tokens[5], shad::data_types::UINT);
      organization = shad::data_types::encode<uint64_t, std::string>(tokens[6], shad::data_types::UINT);
    }

    uint64_t get_key() { return publication; }
    uint64_t get_src() { return publication; }
    uint64_t get_dst() { return organization; }
};

class Vertex {
  public:
    TYPES type;

    Vertex () {
      type = TYPES::NONE;
    }

    Vertex (TYPES edgeType) {
      type = edgeType;
    }

    TYPES get_type () { return type; }
};

class Edge {
  public:
    uint64_t src;
    uint64_t dst;
    TYPES type;

    Edge () {
      src = shad::data_types::kNullValue<uint64_t>;
      dst = shad::data_types::kNullValue<uint64_t>;
      type = TYPES::NONE;
    }

    Edge (uint64_t srcID, uint64_t dstID, TYPES edgeType) {
      src = srcID;
      dst = dstID;
      type = edgeType;
    }

    uint64_t get_src() { return src; }
    uint64_t get_dst() { return dst; }
    TYPES get_type () { return type; }

};


using Graph_t = std::map<TYPES, uint64_t>;

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

using OccursAtEdgeType = shad::Multimap<uint64_t, OccursAtEdge>;
using OccursAtEdgeOID  = shad::ObjectIdentifier<OccursAtEdgeType>;

using HasTopicEdgeType = shad::Multimap<uint64_t, HasTopicEdge>;
using HasTopicEdgeOID  = shad::ObjectIdentifier<HasTopicEdgeType>;

using HasOrgEdgeType = shad::Multimap<uint64_t, HasOrgEdge>;
using HasOrgEdgeOID  = shad::ObjectIdentifier<HasOrgEdgeType>;

using EdgeType = shad::Multimap<uint64_t, Edge>;
using EdgeOID  = shad::ObjectIdentifier<EdgeType>;

using VertexType = shad::Hashmap<uint64_t, Vertex>;
using VertexOID  = shad::ObjectIdentifier<VertexType>;

} // namespace agile::workflow1

#endif
