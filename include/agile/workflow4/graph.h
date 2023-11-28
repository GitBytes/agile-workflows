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

inline void atomic_double_add(double * lhs, double rhs) {
  while (true) {
    double old_value = * lhs;
    double new_value = old_value + rhs;
    int64_t * old_value_ptr = (int64_t *) lhs;
    int64_t * new_value_ptr = (int64_t *) & new_value;
    if (__sync_bool_compare_and_swap((uint64_t *) lhs, * old_value_ptr, * new_value_ptr)) break;
} }

inline void atomic_uint64_max(uint64_t * lhs, uint64_t rhs) {
  while (true) {
    uint64_t old_value = * lhs;
    uint64_t new_value = std::max(old_value, rhs);
    int64_t * old_value_ptr = (int64_t *) lhs;
    int64_t * new_value_ptr = (int64_t *) & new_value;
    if (__sync_bool_compare_and_swap(lhs, * old_value_ptr, * new_value_ptr)) break;
} }

template <typename T>
struct TraderInserter {
  
  bool operator()(T *const lhs, const T &rhs, bool same_key) {
    if (same_key) {     // entry in hashmap, increment edges
       atomic_double_add(& lhs->sold, rhs.sold);
       atomic_double_add(& lhs->bought, rhs.bought);
       atomic_double_add(& lhs->desired, rhs.desired);
       atomic_uint64_max(& lhs->type, rhs.type);
    } else {            // entry not in hashmap, assign next local id
       T temp = rhs;
       * lhs = std::move(temp);
    }  

    return true;   
  }    

  bool Insert(T *const lhs, const T &rhs, bool same_key) {
    if (same_key) {     // entry in hashmap, increment edges
       atomic_double_add(& lhs->sold, rhs.sold);
       atomic_double_add(& lhs->bought, rhs.bought);
       atomic_double_add(& lhs->desired, rhs.desired);
    } else {            // entry not in hashmap, assign next local id
       T temp = rhs;                              
       * lhs = std::move(temp);                              
    }  

    return true;                                                   
  }    
};     

class PersonVertex {
  public:
    uint64_t id;

    PersonVertex () {
      id    = shad::data_types::kNullValue<uint64_t>;
    }

    PersonVertex(std::string id_) {
      id    = ENCODE<uint64_t, std::string, UINT>(id_);
    }

    PersonVertex (std::vector <std::string> & tokens) {
      id    = ENCODE<uint64_t, std::string, UINT>(tokens[1]);
    }

    uint64_t key() { return id; }
};

class TraderVertex {
  public:
    uint64_t id;
    double sold;        // amount of coffee sold
    double bought;      // amount of coffee bought  (>= coffee sold)
    double desired;     // amount of coffee desired (>= coffee bought)
    uint64_t type;      // 0: retail customer; 1: distributor/wholesaler; 2: grower

    TraderVertex () {
      id = shad::data_types::kNullValue<uint64_t>;
      sold = 0.0;
      bought = 0.0;
      desired = 0.0;
      type = shad::data_types::kNullValue<uint64_t>;
    }

    TraderVertex (uint64_t id_, double sold_, double bought_, double desired_, uint64_t type_) {
      id = id_;
      sold = sold_;
      bought = bought_;
      desired = desired_;
      type = type_;
    }

    uint64_t key() { return id; }
};

class ServerVertex {
  public:
    uint64_t id;

    ServerVertex () {
      id    = shad::data_types::kNullValue<uint64_t>;
    }

    ServerVertex(std::string id_) {
      id    = ENCODE<uint64_t, std::string, UINT>(id_);
    }

    ServerVertex (std::vector <std::string> & tokens) {
      id    = ENCODE<uint64_t, std::string, UINT>  (tokens[1]);
    }

    uint64_t key() { return id; }
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

    uint64_t key() { return seller; }
    uint64_t src() { return seller; }
    uint64_t dst() { return buyer; }
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

    PurchaseEdge (SaleEdge & sale) {
      buyer    = sale.buyer;
      seller   = sale.seller;
      product  = sale.product;
      date     = sale.date;
      amount   = sale.amount;
      weight   = sale.weight;
      src_type = sale.src_type;
      dst_type = sale.src_type;
    }

    uint64_t key() { return buyer; }
    uint64_t src() { return buyer; }
    uint64_t dst() { return seller; }
};

class FriendOfEdge {
  public:
    uint64_t person1;     // vertex id
    uint64_t person2;     // vertex id
    double   weight;
    TYPES    src_type;
    TYPES    dst_type;

    FriendOfEdge () {
      person1  = shad::data_types::kNullValue<uint64_t>;
      person2  = shad::data_types::kNullValue<uint64_t>;
      weight   = shad::data_types::kNullValue<double>;
      src_type = TYPES::NONE;
      dst_type = TYPES::NONE;
    }

    FriendOfEdge (std::vector <std::string> & tokens) {
      person1  = ENCODE<uint64_t, std::string, UINT>(tokens[0]);
      person2  = ENCODE<uint64_t, std::string, UINT>(tokens[1]);
      weight   = shad::data_types::kNullValue<double>;
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
    double   weight;
    TYPES    src_type;
    TYPES    dst_type;

    UsesEdge () {
      person   = shad::data_types::kNullValue<uint64_t>;
      server   = shad::data_types::kNullValue<uint64_t>;
      weight   = shad::data_types::kNullValue<double>;
      src_type = TYPES::NONE;
      dst_type = TYPES::NONE;
    }

    UsesEdge (std::vector <std::string> & tokens) {
      person   = ENCODE<uint64_t, std::string, UINT>(tokens[0]);
      server   = ENCODE<uint64_t, std::string, UINT>(tokens[1]);
      weight   = shad::data_types::kNullValue<double>;
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

class ServerToServerEdge {
  public:
    uint64_t src_device;     // vertex id
    uint64_t dst_device;     // vertex id
    double   weight;
 
    ServerToServerEdge () {
      src_device = shad::data_types::kNullValue<uint64_t>;
      dst_device = shad::data_types::kNullValue<uint64_t>;
      weight     = shad::data_types::kNullValue<double>;
    }

    ServerToServerEdge (uint64_t src_, uint64_t dst_, double weight_) {
      src_device  = src_;
      dst_device  = dst_;
      weight      = weight_;
    }

    uint64_t key() { return src_device; }
    uint64_t src() { return src_device; }
    uint64_t dst() { return dst_device; }
};

using PersonVertexType = shad::Hashmap<uint64_t, PersonVertex>;
using PersonVertexOID  = shad::ObjectIdentifier<PersonVertexType>;

using ServerVertexType = shad::Hashmap<uint64_t, ServerVertex>;
using ServerVertexOID  = shad::ObjectIdentifier<ServerVertexType>;

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

using TraderVertexType = shad::Hashmap<uint64_t, TraderVertex, shad::MemCmp<uint64_t>, TraderInserter<TraderVertex>>;
using TraderVertexOID = shad::ObjectIdentifier<TraderVertexType>;

using ServerToServerEdgeType = shad::Multimap<uint64_t, ServerToServerEdge>;
using ServerToServerEdgeOID  = shad::ObjectIdentifier<ServerToServerEdgeType>;

void CancelCoffeeTrader(Handle &, const uint64_t &, TraderVertex &, RF_args_t &);
void CoffeeSalesWeight(Handle &, const uint64_t &, std::vector<SaleEdge> &, RF_args_t &);
void SelectSalesMarket(Handle &, const uint64_t &, std::vector<SaleEdge> &, uint64_t &, RF_args_t &);
void UsesEdgeWeights(Handle &, const uint64_t &, std::vector<UsesEdge> &, RF_args_t &);
void SendsEdgeWeights(Handle &, const uint64_t &, std::vector<SendsEdge> &, RF_args_t &);
void FriendsEdgeWeights(Handle &, const uint64_t &, std::vector<FriendOfEdge> &, RF_args_t &);
} // namespace agile::workflow4

#endif // GRAPH_H
