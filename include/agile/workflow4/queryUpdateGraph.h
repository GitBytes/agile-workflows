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

#ifndef QUERYUPDATEGRAPH_H_
#define QUERYUPDATEGRAPH_H_

#include <mutex>
#include <algorithm>
#include <fstream>
#include "shad/data_structures/atomic.h"
#include "agile/workflow4/graph.h"

#define UINT   shad::data_types::UINT
#define DOUBLE shad::data_types::DOUBLE
#define USDATE shad::data_types::USDATE
#define ENCODE shad::data_types::encode

using intAtomic = shad::Atomic<int64_t>;
using intAtomicOID = shad::ObjectIdentifier<intAtomic>;

namespace agile::workflow4 { 

struct SellerVertex {
    uint64_t id;
    uint64_t glbid;
    double sales;           // total sales by this seller
    uint64_t sales_count;   // for verification?
    TYPES type;

    SellerVertex () {
        id          = shad::data_types::kNullValue<uint64_t>;
        glbid       = shad::data_types::kNullValue<uint64_t>;
        sales       = shad::data_types::kNullValue<double>;
        sales_count = shad::data_types::kNullValue<uint64_t>;
        type        = TYPES::NONE;
    }

    SellerVertex (uint64_t seller, double sale=0.0) {
        id      = seller;
        sales   = sale;
        type   = TYPES::PERSON;
    }

    void add_sale_amount(double amount) {
        sales += amount;
        if (amount > 0.0)
            ++sales_count;
    }

    void add_sale_amount(double amount, uint64_t count) {
        sales       = amount;
        sales_count = count;
    }

    uint64_t key() { return id;}
};

struct CoffeeSaleEdge {
    uint64_t seller;
    uint64_t buyer;
    double sale_weight;
    TYPES src_type;
    TYPES dst_type;

    CoffeeSaleEdge () {
        seller      = shad::data_types::kNullValue<uint64_t>;
        buyer       = shad::data_types::kNullValue<uint64_t>;
        sale_weight = shad::data_types::kNullValue<double>;
        src_type    = TYPES::NONE;
        dst_type    = TYPES::NONE;
    }

    CoffeeSaleEdge (uint64_t s_id, uint64_t b_id, double sale_wt) {
        seller      = s_id;
        buyer       = b_id;
        sale_weight = sale_wt;
        src_type    = TYPES::PERSON;
        dst_type    = TYPES::PERSON;
    }
};

// void getCoffeeSalesEdge(Handle & handle, const uint64_t & topic_key, uint64_t & salesEdge_OID);

// void printAllEntries(uint64_t & salesEdge_OID);

// void CoffeeSalesWeight(Handle & handle, const uint64_t& seller, std::vector<SaleEdge>& sales, const RF_args_t & args);

// void getCoffeeSaleEdgeWeights(Graph_t & graph, const RF_args_t & args, uint64_t  product_id);
// void getCoffeeSaleEdgeWeights(Graph_t & graph, const RF_args_t & args);
// void getCoffeeTraders(Graph_t & graph, const RF_args_t & args);
// void reconfigureGraph(Graph_t & graph, const std::vector<uint64_t>& influencers, const RF_args_t & args);
} // namespace agile::workflow4

#endif // QUERYUPDATEGRAPH_H_