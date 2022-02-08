#include <cstdint>
#include <limits>
#include <vector>

#include "agile/workflow2/main.h"

#include "shad/data_structures/hashmap.h"

namespace agile::workflow2 {

class PersonVertex {
  public:
    uint64_t id;

    PersonVertex () {
      id = std::numeric_limits<uint64_t>::max();
    }

    PersonVertex (std::vector <std::string> & tokens) {
      id = String_to_Uint(tokens[1]);
    }

    uint64_t key() { return id; }
};

class ForumEventVertex {
  public:
    uint64_t id;
    double date;

    ForumEventVertex () {
      id   = std::numeric_limits<uint64_t>::max();
      date = std::numeric_limits<uint64_t>::max();
    }

    ForumEventVertex (std::vector <std::string> & tokens) {
      id   = String_to_Uint(tokens[4]);
      date = String_to_Date(tokens[7]);
    }

    uint64_t key() { return id; }
};

class ForumVertex {
  public:
    uint64_t id;

    ForumVertex () {
      id   = std::numeric_limits<uint64_t>::max();
    }

    ForumVertex (std::vector <std::string> & tokens) {
      id = String_to_Uint(tokens[3]);
    }

    uint64_t key() { return id; }
};

class PublicationVertex {
  public:
    uint64_t id;
    double date;

    PublicationVertex () {
      id   = std::numeric_limits<uint64_t>::max();
      date = std::numeric_limits<uint64_t>::max();
    }

    PublicationVertex (std::vector <std::string> & tokens) {
      id   = String_to_Uint(tokens[5]);
      date = String_to_Date(tokens[7]);
    }

    uint64_t key() { return id; }
};

class TopicVertex {
  public:
    uint64_t id;
    double lat;
    double lon;

    TopicVertex () {
      id  = std::numeric_limits<uint64_t>::max();
      lat = std::numeric_limits<uint64_t>::max();
      lon = std::numeric_limits<uint64_t>::max();
    }

    TopicVertex (std::vector <std::string> & tokens) {
      id  = String_to_Uint(tokens[6]);
      lat = String_to_Double(tokens[8]);
      lon = String_to_Double(tokens[9]);
    }

    uint64_t key() { return id; }
};

class PurchaseEdge {
  public:
    uint64_t buyer;
    uint64_t seller;
    uint64_t product;
    double date;

    PurchaseEdge () {
       buyer   = std::numeric_limits<uint64_t>::max();
       seller  = std::numeric_limits<uint64_t>::max();
       product = std::numeric_limits<uint64_t>::max();
       date    = std::numeric_limits<uint64_t>::max();
    }

    PurchaseEdge (std::vector <std::string> & tokens) {
      buyer   = String_to_Uint(tokens[1]);
      seller  = String_to_Uint(tokens[2]);
      product = String_to_Uint(tokens[6]);
      date    = String_to_Date(tokens[7]);
    }

    uint64_t key() { return buyer; }
    uint64_t src() { return buyer; }
    uint64_t dst() { return seller; }
};

class SaleEdge {
  public:
    uint64_t seller;
    uint64_t buyer;
    uint64_t product;
    double date;

    SaleEdge () {
       seller  = std::numeric_limits<uint64_t>::max();
       buyer   = std::numeric_limits<uint64_t>::max();
       product = std::numeric_limits<uint64_t>::max();
       date    = std::numeric_limits<uint64_t>::max();
    }

    SaleEdge (std::vector <std::string> & tokens) {
      seller  = String_to_Uint(tokens[1]);
      buyer   = String_to_Uint(tokens[2]);
      product = String_to_Uint(tokens[6]);
      date    = String_to_Date(tokens[7]);
    }

    uint64_t key() { return seller; }
    uint64_t src() { return seller; }
    uint64_t dst() { return buyer; }
};

class AuthorEdge {
  public:
    uint64_t author;
    uint64_t item;

    AuthorEdge () {
       author = std::numeric_limits<uint64_t>::max();
       item   = std::numeric_limits<uint64_t>::max();
    }

    AuthorEdge (std::vector <std::string> & tokens) {
      author = String_to_Uint(tokens[1]);
      if (tokens[4] != "") item = String_to_Uint(tokens[4]);
      else                 item = String_to_Uint(tokens[5]);
    }

    uint64_t key() { return author; }
    uint64_t src() { return author; }
    uint64_t dst() { return item; }
};

class OccursAtEdge {
  public:
    uint64_t forum;
    uint64_t forum_event;

    OccursAtEdge () {
       forum_event = std::numeric_limits<uint64_t>::max();
       forum       = std::numeric_limits<uint64_t>::max();
    }

    OccursAtEdge (std::vector <std::string> & tokens) {
      forum_event = String_to_Uint(tokens[4]);
      forum       = String_to_Uint(tokens[3]);
    }

    uint64_t key() { return forum_event; }
    uint64_t src() { return forum_event; }
    uint64_t dst() { return forum; }
};

class HasTopicEdge {
  public:
    uint64_t item;
    uint64_t topic;

    HasTopicEdge () {
       item  = std::numeric_limits<uint64_t>::max();
       topic = std::numeric_limits<uint64_t>::max();
    }

    HasTopicEdge (std::vector <std::string> & tokens) {
      if      (tokens[3] != "") item = String_to_Uint(tokens[3]);
      else if (tokens[4] != "") item = String_to_Uint(tokens[4]);
      else                      item = String_to_Uint(tokens[5]);
      topic = String_to_Uint(tokens[6]);
    }

    uint64_t key() { return item; }
    uint64_t src() { return item; }
    uint64_t dst() { return topic; }
};

class HasOrgEdge {
  public:
    uint64_t publication;
    uint64_t organization;

    HasOrgEdge () {
       publication  = std::numeric_limits<uint64_t>::max();
       organization = std::numeric_limits<uint64_t>::max();
    }

    HasOrgEdge (std::vector <std::string> & tokens) {
      publication  = String_to_Uint(tokens[5]);
      organization = String_to_Uint(tokens[6]);
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

using OccursAtEdgeType = shad::Multimap<uint64_t, OccursAtEdge>;
using OccursAtEdgeOID  = shad::ObjectIdentifier<OccursAtEdgeType>;

using HasTopicEdgeType = shad::Multimap<uint64_t, HasTopicEdge>;
using HasTopicEdgeOID  = shad::ObjectIdentifier<HasTopicEdgeType>;

using HasOrgEdgeType = shad::Multimap<uint64_t, HasOrgEdge>;
using HasOrgEdgeOID  = shad::ObjectIdentifier<HasOrgEdgeType>;

} // namespace agile::workflow2
