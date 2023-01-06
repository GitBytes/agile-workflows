
#include "agile/workflow4/extractGraph.h"

using intAtomic = shad::Atomic<int64_t>;
using intAtomicOID = shad::ObjectIdentifier<intAtomic>;

namespace agile::workflow4 {

void printCoffeeEdgesToFile(const uint64_t& seller, std::vector<SaleEdge>& sales, const RF_args_t & args)
{
    std::ofstream file_out;
    file_out.open(args.outfilename, std::ios_base::app);
    for (auto cse : sales)
    {
        file_out << cse.seller << "," << cse.buyer << "," << cse.weight << "\n";
    }
    file_out.close();
}

void getCoffeeSales(const uint64_t& seller, std::vector<SaleEdge>& sales, const RF_args_t & args)
{
    Handle handle;
    double total_sales = 0;
    for (auto se : sales)
    {
        if(se.product == 8486)
            total_sales+= se.amount;
    }
    // std::cout << " Total Coffee Sales: " << total_sales << "\n";
    auto CoffeeSales = SaleEdgeType::GetPtr((SaleEdgeOID) args.CoffeeSales_OID);

    for (auto se : sales)
    {
        if(se.product == 8486)
        {
            se.weight = se.amount/total_sales;
            CoffeeSales->BufferedAsyncInsert(handle, se.key(), se);
        }    
    }
    shad::rt::waitForCompletion(handle);
}

void getCoffeeSaleEdgeWeights(Graph_t & graph, const RF_args_t & args, uint64_t  product_id)
{
    Handle handle;
    // auto Sales  = SaleEdgeType::GetPtr( (SaleEdgeOID) salesEdge_OID);
    // Create Coffee Sales Edges
    // CoffeeSalesEdge_(handle, key_coffee, salesEdge_OID);

    auto G_sales = SaleEdgeType::GetPtr((SaleEdgeOID) graph["Sales"]);

    std::cout << " Sales Size: " << G_sales->Size() << " \n";
    G_sales->ForEachEntry(getCoffeeSales, args);
    
    auto G_coffeesales = SaleEdgeType::GetPtr((SaleEdgeOID) graph["CoffeeSales"]);
    std::cout << " Total Coffee Sales: " << G_coffeesales->Size() << "\n";
    std::ofstream file_out;
    G_coffeesales->ForEachEntry(printCoffeeEdgesToFile, args);

    // std::size_t count_keys = 0;
    // // auto itr_e = G_sales->cend(); 
    // // auto itr_b = G_sales->cbegin();
    // auto kitr_e = G_sales->key_end(); 
    // auto kitr_b = G_sales->key_begin();
    // // for(; itr_b != itr_e; ++itr_b)
    // for(auto itr = kitr_b; itr != kitr_e; ++itr)
    // {
    //     auto key = (*itr).first;
    //     std::cout << " Sales key: " << key << "\n";
    //     ++count_keys;
    // }
    // std::cout << " Sales Key Size: " << count_keys << " \n";

}

} // namespace