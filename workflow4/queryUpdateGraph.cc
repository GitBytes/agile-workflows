#include "agile/workflow4/main.h"
#include "agile/workflow4/graph.h"

namespace agile::workflow4 {

void EraseSaleEdge(Handle & handle, const uint64_t & seller,
     std::vector<SaleEdge> & sales, uint64_t & buyer, double & amount, time_t & date) {

  for (auto itr = sales.begin(); itr != sales.end(); ++ itr) {
    if ( ((* itr).buyer != buyer) || ((* itr).amount != amount) || ((* itr).date  != date) ) continue;
    sales.erase(itr);
    break;
} }


void ErasePurchaseEdge(Handle & handle, const uint64_t & buyer,
     std::vector<PurchaseEdge> & purchases, uint64_t & seller, double & amount, time_t & date) { 

  for (auto itr = purchases.begin(); itr != purchases.end(); ++ itr) {
    if ( ((* itr).seller != seller) || ((* itr).amount != amount) || ((* itr).date  != date) ) continue;
    purchases.erase(itr);
    break;
} }


// Initiated by the seller at the site of the buyer, this routine adjusts the buyer's (trader's) purchase
// amount, removes the purchase edge from the buyer to the seller, and searches for a new supplier to re-
// place the buyer's lost purchase.
void CancelCoffeeSale(Handle & handle,
     const uint64_t & buyer, TraderVertex & trader, SaleEdge & sale, RF_args_t & args) {
  auto CoffeePurchases = PurchaseEdgeType::GetPtr((PurchaseEdgeType::ObjectID) args.CoffeePurchases_OID);

  if (trader.bought > 0)  trader.bought  -= sale.amount;     // if buyer has been canceled, bought will be 0
  if (trader.desired > 0) trader.desired -= sale.amount;     // if buyer has been canceled, desired will be 0
  CoffeePurchases->AsyncApply(handle, buyer, ErasePurchaseEdge, sale.seller, sale.amount, sale.date);
}


// Initiated by the buyer at the site of the seller, this routine adjusts the seller's (trader's) sold
// amount and removes the sale edge from the seller to the buyer.
void CancelCoffeePurchase(Handle & handle,
     const uint64_t & seller, TraderVertex & trader, PurchaseEdge & purchase, RF_args_t & args) {
  auto CoffeeSales = SaleEdgeType::GetPtr((SaleEdgeType::ObjectID) args.CoffeeSales_OID);
  
  if (trader.sold > 0)  trader.sold -= purchase.amount;     // if seller has been canceled, sold will be 0
  CoffeeSales->AsyncApply(handle, seller, EraseSaleEdge, purchase.buyer, purchase.amount, purchase.date);
}


void CancelCoffeeTrader(Handle & handle, const uint64_t & id, TraderVertex & trader, RF_args_t & args) {
  SaleEdgeType::LookupResult sales;
  PurchaseEdgeType::LookupResult purchases;
  auto CoffeeTraders   = TraderVertexType::GetPtr((TraderVertexType::ObjectID) args.CoffeeTraders_OID);
  auto CoffeeSales     = SaleEdgeType::GetPtr((SaleEdgeType::ObjectID) args.CoffeeSales_OID);
  auto CoffeePurchases = PurchaseEdgeType::GetPtr((PurchaseEdgeType::ObjectID) args.CoffeePurchases_OID);

  trader.sold = 0.0;                            // zero out my coffee sales
  trader.bought = 0.0;                          // zero out my coffee purchases
  trader.desired = 0.0;                         // zero out my desired coffee purchases

  CoffeeSales->Lookup(id, & sales);             // get my coffee sales
  CoffeeSales->Erase(id);                       // erase my sale edges from the graph
  CoffeePurchases->Lookup(id, & purchases);     // get my coffee sales
  CoffeePurchases->Erase(id);                   // erase my purchase edges from the graph

  for (auto sale : sales.value)                 // alert my customers
      CoffeeTraders->AsyncApply(handle, sale.buyer, CancelCoffeeSale, sale, args);
  for (auto purchase : purchases.value)         // alert my suppliers
      CoffeeTraders->AsyncApply(handle, purchase.seller, CancelCoffeePurchase, purchase, args);
}

void PrintWeightedSalesEdgesToFile(Handle & handle, RF_args_t & args) {

  auto CoffeeSales = SaleEdgeType::GetPtr((SaleEdgeType::ObjectID) args.CoffeeSales_OID);

  auto printLambda = [](const uint8_t *argsBuffer, const uint32_t) {  
    const RF_args_t argsL = *reinterpret_cast<const RF_args_t *>(argsBuffer);
    auto mapPtr = SaleEdgeType::GetPtr((SaleEdgeType::ObjectID) argsL.CoffeeSales_OID);
    auto localMapPtr = mapPtr->GetLocalMultimap();
    std::ofstream file_out;
    file_out.open(argsL.outfilename, std::ios_base::app);
    for (auto itr = localMapPtr->begin(); itr != localMapPtr->end(); ++itr)
    {
      auto se = (*itr).second;
      file_out << se.seller << "," << se.buyer << "," << se.weight << "\n";
    }
    file_out.close();
  };

  std::shared_ptr<uint8_t> args_buffer(new uint8_t[sizeof(RF_args_t)], std::default_delete<uint8_t[]>());
  std::memcpy(args_buffer.get(), &args, sizeof(RF_args_t));

  for(auto loc : shad::rt::allLocalities()) {
    shad::rt::executeAt(loc, printLambda, args_buffer, sizeof(args));
  }
}

void CoffeeSalesWeight(Handle & handle, const uint64_t & seller, std::vector<SaleEdge> & sales, RF_args_t & args) {
  auto CoffeeTraders = TraderVertexType::GetPtr((TraderVertexType::ObjectID) args.CoffeeTraders_OID);

  TraderVertex trader;
  CoffeeTraders->Lookup(seller, &trader);

  for (auto se : sales)
  {
    se.weight = se.amount/trader.sold;
  }
}

} // namespace
