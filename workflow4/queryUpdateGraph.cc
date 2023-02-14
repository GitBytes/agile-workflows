#include "agile/workflow4/main.h"
#include "agile/workflow4/graph.h"

namespace agile::workflow4 {

void CoffeeCancel(Handle & handle, const uint64_t & key, TraderVertex & value, RF_args_t & args) {
  auto CoffeeSales = SaleEdgeType::GetPtr((SaleEdgeType::ObjectID) args.CoffeeSales_OID);
  auto CoffeePurchases = PurchaseEdgeType::GetPtr((PurchaseEdgeType::ObjectID) args.CoffeePurchases_OID);

  SaleEdgeType::LookupResult sales;
  PurchaseEdgeType::LookupResult purchases;

  value.sold = 0.0;
  value.bought = 0.0;
  value.desired = 0.0;
  CoffeeSales->Lookup(key, & sales);              // get my coffee sales
  CoffeePurchases->Lookup(key, & purchases);      // get my coffee purchases
  CoffeeSales->AsyncErase(handle, key);           // delete my coffee sales
  CoffeePurchases->AsyncErase(handle, key);       // delete my coffee purchases

  for (auto sale : sales.value) { };             // alert each customer that I am not selling coffee
  for (auto purchase : purchases.value) { };     // alert each supplier that I am not buying coffee
}

void PrintWeightedSalesEdgesToFile(Handle & handle, const uint64_t& seller, std::vector<SaleEdge>& sales, RF_args_t & args){
  std::ofstream file_out;
    file_out.open(args.filename, std::ios_base::app);
    for (auto cse : sales)
    {
        file_out << cse.seller << "," << cse.buyer << "," << cse.weight << "\n";
    }
    file_out.close();
}

void CoffeeSalesWeight(Handle & handle, const uint64_t& seller, std::vector<SaleEdge>& sales, RF_args_t & args) {
  auto CoffeeTraders = TraderVertexType::GetPtr((TraderVertexType::ObjectID) args.CoffeeTraders_OID);
  // TraderVertexType::LookupResult trader;
  TraderVertex trader;
  CoffeeTraders->Lookup(seller, &trader);
  for (auto se : sales)
  {
    se.weight = se.amount/trader.bought;
  }
}

} // namespace
