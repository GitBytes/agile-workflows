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

#include <cstdint>
#include <limits>
#include <vector>
#include <atomic>

#include "shad/data_structures/hashmap.h"
#include "shad/extensions/data_types/data_types.h"

#include "agile/workflow4/main.h"
#include "agile/workflow4/graphTypes.h"

#define UINT   shad::data_types::UINT
#define DOUBLE shad::data_types::DOUBLE
#define USDATE shad::data_types::USDATE
#define ENCODE shad::data_types::encode

namespace agile::workflow4 {

class PersonVertex {
  public:
    uint64_t id;
    uint64_t glbid;
    // std::atomic<uint64_t> coffee_sold = {0};
    // std::atomic<uint64_t> coffee_purchased = {0};
    double coffee_sold = {0};                       // original sell qty
    double coffee_purchased = {0};                  // original buy qty  //FIFO..distirbuted control.
    // Wholeseller.. deficit/surplus
    // Distributor.. deficit/surplus 

    // double coffee_surplus = {0};                    // seller
    // double coffee_deficit = {0};                    // buyer
    
    double coffee_sold_old = {0};                    // seller
    double coffee_purchased_old = {0};                    // buyer

    PersonVertex ()
    {
      id    = shad::data_types::kNullValue<uint64_t>;
      glbid = shad::data_types::kNullValue<uint64_t>;
    }

    PersonVertex(std::string id_)
    {
      id    = ENCODE<uint64_t, std::string, UINT>(id_);
      glbid = shad::data_types::kNullValue<uint64_t>;
    }

    PersonVertex (std::vector <std::string> & tokens)
    {
      id    = ENCODE<uint64_t, std::string, UINT>(tokens[1]);
      glbid = shad::data_types::kNullValue<uint64_t>;
    }

    uint64_t key() { return id; }
};

class ServerVertex {
  public:
    uint64_t id;
    uint64_t glbid;

    ServerVertex () {
      id    = shad::data_types::kNullValue<uint64_t>;
      glbid = shad::data_types::kNullValue<uint64_t>;
    }

    ServerVertex(std::string id_)
    {
      id    = ENCODE<uint64_t, std::string, UINT>(id_);
      glbid = shad::data_types::kNullValue<uint64_t>;
    }

    ServerVertex (std::vector <std::string> & tokens) {
      id    = ENCODE<uint64_t, std::string, UINT>  (tokens[1]);
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
    TYPES    type;

    TopicVertex () {
      id    = shad::data_types::kNullValue<uint64_t>;
      lat   = shad::data_types::kNullValue<double>;
      lon   = shad::data_types::kNullValue<double>;
      glbid = shad::data_types::kNullValue<uint64_t>;
      type  = TYPES::NONE;
    }

    TopicVertex (std::vector <std::string> & tokens) {
      id    = ENCODE<uint64_t, std::string, UINT>  (tokens[3]);
      lat   = ENCODE<double,   std::string, DOUBLE>(tokens[5]);
      lon   = ENCODE<double,   std::string, DOUBLE>(tokens[6]);
      glbid = shad::data_types::kNullValue<uint64_t>;
      type  = TYPES::TOPIC;
    }

    uint64_t key() { return id; }
};

class PurchaseEdge {
  public:
    uint64_t buyer;            // vertex id
    uint64_t seller;           // vertex id
    uint64_t product;
    time_t   date;
    double   amount;
    double   weight;
    TYPES    src_type;
    TYPES    dst_type;

    PurchaseEdge () {
      buyer   = shad::data_types::kNullValue<uint64_t>;
      seller  = shad::data_types::kNullValue<uint64_t>;
      product = shad::data_types::kNullValue<uint64_t>;
      date    = shad::data_types::kNullValue<time_t>;
      amount  = shad::data_types::kNullValue<double>;
      weight  = shad::data_types::kNullValue<double>;
      src_type = TYPES::NONE;
      dst_type = TYPES::NONE;
    }

    PurchaseEdge (std::vector <std::string> & tokens) {
      buyer    = ENCODE<uint64_t, std::string, UINT>  (tokens[1]);
      seller   = ENCODE<uint64_t, std::string, UINT>  (tokens[2]);
      product  = ENCODE<uint64_t, std::string, UINT>  (tokens[3]);
      date     = ENCODE<time_t,   std::string, USDATE>(tokens[4]);
      amount   = ENCODE<time_t,   std::string, USDATE>(tokens[7]);
      weight   = shad::data_types::kNullValue<double>;
      src_type = TYPES::PERSON;
      dst_type = TYPES::PERSON;
    }

    // PurchaseEdge (PurchaseEdge & purchase) {
    //   buyer    = purchase.buyer;
    //   seller   = purchase.seller;
    //   product  = purchase.product;
    //   date     = purchase.date;
    //   amount   = purchase.amount;
    //   src_type = purchase.src_type;
    //   dst_type = purchase.dst_type;
    // }

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
    double   amount;
    double   weight;
    TYPES    src_type;
    TYPES    dst_type;

    SaleEdge () {
      seller   = shad::data_types::kNullValue<uint64_t>;
      buyer    = shad::data_types::kNullValue<uint64_t>;
      product  = shad::data_types::kNullValue<uint64_t>;
      date     = shad::data_types::kNullValue<time_t>;
      amount   = shad::data_types::kNullValue<double>;
      weight   = shad::data_types::kNullValue<double>;
      src_type = TYPES::NONE;
      dst_type = TYPES::NONE;
    }

    SaleEdge (std::vector <std::string> & tokens) {
      seller   = ENCODE<uint64_t, std::string, UINT>  (tokens[1]);
      buyer    = ENCODE<uint64_t, std::string, UINT>  (tokens[2]);
      product  = ENCODE<uint64_t, std::string, UINT>  (tokens[3]);
      date     = ENCODE<time_t,   std::string, USDATE>(tokens[4]);
      amount   = ENCODE<double,   std::string, DOUBLE>(tokens[7]);
      weight   = shad::data_types::kNullValue<double>;
      src_type = TYPES::PERSON;
      dst_type = TYPES::PERSON;
    }

    // SaleEdge (SaleEdge & sale) {
    //   seller   = sale.seller;
    //   buyer    = sale.buyer;
    //   product  = sale.product;
    //   date     = sale.date;
    //   amount   = sale.amount;
    //   src_type = sale.src_type;
    //   dst_type = sale.dst_type;
    // }

    uint64_t key() { return seller; }
    uint64_t src() { return seller; }
    uint64_t dst() { return buyer; }
};

class FriendOfEdge {
  public:
    uint64_t person1;     // vertex id
    uint64_t person2;     // vertex id
    TYPES    src_type;
    TYPES    dst_type;

    FriendOfEdge () {
      person1  = shad::data_types::kNullValue<uint64_t>;
      person2  = shad::data_types::kNullValue<uint64_t>;
      src_type = TYPES::NONE;
      dst_type = TYPES::NONE;
    }

    FriendOfEdge (std::vector <std::string> & tokens) {
      person1  = ENCODE<uint64_t, std::string, UINT>(tokens[0]);
      person2  = ENCODE<uint64_t, std::string, UINT>(tokens[1]);
      src_type = TYPES::PERSON;
      dst_type = TYPES::PERSON;
    }

    uint64_t key() { return person1; }
    uint64_t src() { return person1; }
    uint64_t dst() { return person2; }
};

class UsesEdge {
  public:
    uint64_t person;      // vertex id
    uint64_t server;      // vertex id
    TYPES    src_type;
    TYPES    dst_type;

    UsesEdge () {
      person   = shad::data_types::kNullValue<uint64_t>;
      server   = shad::data_types::kNullValue<uint64_t>;
      src_type = TYPES::NONE;
      dst_type = TYPES::NONE;
    }

    UsesEdge (std::vector <std::string> & tokens) {
      person  = ENCODE<uint64_t, std::string, UINT>(tokens[0]);
      server   = ENCODE<uint64_t, std::string, UINT>(tokens[1]);
      src_type = TYPES::PERSON;
      dst_type = TYPES::SERVER;
    }

    uint64_t key() { return person; }
    uint64_t src() { return person; }
    uint64_t dst() { return server; }
};

class SendsEdge {
  public:
    uint64_t src_device;     // vertex id
    uint64_t dst_device;     // vertex id
    uint64_t epoch_time;
    uint64_t duration;
    uint64_t protocol;
    uint64_t src_port;
    uint64_t dst_port;
    uint64_t src_packets;
    uint64_t dst_packets;
    uint64_t src_bytes;
    uint64_t dst_bytes;
    TYPES    src_type;
    TYPES    dst_type;
 
    SendsEdge () {
      src_device  = shad::data_types::kNullValue<uint64_t>;
      dst_device  = shad::data_types::kNullValue<uint64_t>;
      epoch_time  = shad::data_types::kNullValue<uint64_t>;
      duration    = shad::data_types::kNullValue<uint64_t>;
      protocol    = shad::data_types::kNullValue<uint64_t>;
      src_port    = shad::data_types::kNullValue<uint64_t>;
      dst_port    = shad::data_types::kNullValue<uint64_t>;
      src_packets = shad::data_types::kNullValue<uint64_t>;
      dst_packets = shad::data_types::kNullValue<uint64_t>;
      src_bytes   = shad::data_types::kNullValue<uint64_t>;
      dst_bytes   = shad::data_types::kNullValue<uint64_t>;
      src_type    = TYPES::NONE;
      dst_type    = TYPES::NONE;
    }

    SendsEdge (std::vector <std::string> & tokens) {
      src_device  = ENCODE<uint64_t, std::string, UINT>(tokens[0]);
      dst_device  = ENCODE<uint64_t, std::string, UINT>(tokens[1]);
      epoch_time  = ENCODE<uint64_t, std::string, UINT>(tokens[2]);
      duration    = ENCODE<uint64_t, std::string, UINT>(tokens[3]);
      protocol    = ENCODE<uint64_t, std::string, UINT>(tokens[4]);
      src_port    = ENCODE<uint64_t, std::string, UINT>(tokens[5]);
      dst_port    = ENCODE<uint64_t, std::string, UINT>(tokens[6]);
      src_packets = ENCODE<uint64_t, std::string, UINT>(tokens[7]);
      dst_packets = ENCODE<uint64_t, std::string, UINT>(tokens[8]);
      src_bytes   = ENCODE<uint64_t, std::string, UINT>(tokens[9]);
      dst_bytes   = ENCODE<uint64_t, std::string, UINT>(tokens[10]);
      src_type = TYPES::SERVER;
      dst_type = TYPES::SERVER;
    }

    uint64_t key() { return src_device; }
    uint64_t src() { return src_device; }
    uint64_t dst() { return dst_device; }
};

// using GlobalIDType = shad::Hashmap<uint64_t, Vertex, shad::MemCmp<uint64_t>, globalIdInserter<Vertex> >;
// using GlobalIDOID  = shad::ObjectIdentifier<GlobalIDType>;

using PersonVertexType = shad::Hashmap<uint64_t, PersonVertex>;
using PersonVertexOID  = shad::ObjectIdentifier<PersonVertexType>;

using ServerVertexType = shad::Hashmap<uint64_t, ServerVertex>;
using ServerVertexOID  = shad::ObjectIdentifier<ServerVertexType>;

using TopicVertexType = shad::Hashmap<uint64_t, TopicVertex>;
using TopicVertexOID  = shad::ObjectIdentifier<TopicVertexType>;

using PurchaseEdgeType = shad::Multimap<uint64_t, PurchaseEdge>;
using PurchaseEdgeOID  = shad::ObjectIdentifier<PurchaseEdgeType>;

using SaleEdgeType = shad::Multimap<uint64_t, SaleEdge>;
using SaleEdgeOID  = shad::ObjectIdentifier<SaleEdgeType>;

using FriendOfEdgeType = shad::Multimap<uint64_t, FriendOfEdge>;
using FriendOfEdgeOID  = shad::ObjectIdentifier<FriendOfEdgeType>;

using UsesEdgeType = shad::Multimap<uint64_t, UsesEdge>;
using UsesEdgeOID  = shad::ObjectIdentifier<UsesEdgeType>;

using SendsEdgeType = shad::Multimap<uint64_t, SendsEdge>;
using SendsEdgeOID  = shad::ObjectIdentifier<SendsEdgeType>;

} // namespace agile::workflow4

#endif // GRAPH_H
