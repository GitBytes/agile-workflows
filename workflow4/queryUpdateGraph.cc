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

} // namespace
