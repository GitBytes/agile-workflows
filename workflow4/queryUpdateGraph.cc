#include "agile/workflow4/main.h"
#include "agile/workflow4/graph.h"

namespace agile::workflow4 {

void EraseSaleEdge(Handle & handle, const uint64_t & seller,
     std::vector<SaleEdge> & sales, uint64_t & buyer, double & amount, time_t & date) {

  for (auto itr = sales.begin(); itr != sales.end(); ++ itr) {
    if ( ((* itr).buyer != buyer) || ((* itr).amount != amount) || ((* itr).date != date) ) continue;
    sales.erase(itr);
    break;
} }


void ErasePurchaseEdge(Handle & handle, const uint64_t & buyer,
     std::vector<PurchaseEdge> & purchases, uint64_t & seller, double & amount, time_t & date) { 

  for (auto itr = purchases.begin(); itr != purchases.end(); ++ itr) {
    if ( ((* itr).seller != seller) || ((* itr).amount != amount) || ((* itr).date != date) ) continue;
    purchases.erase(itr);
    break;
} }


// Initiated by the seller at the site of the buyer, this routine adjusts the buyer's purchase amount,
// removes the purchase edge from the buyer to the seller and searches for one or more suppliers that
// can replace the lost purchase.
void CancelCoffeePurchase(Handle & handle,
     const uint64_t & id, TraderVertex & buyer, SaleEdge & sale, RF_args_t & args) {
  auto CoffeePurchases = PurchaseEdgeType::GetPtr((PurchaseEdgeType::ObjectID) args.CoffeePurchases_OID);

  if (buyer.bought > 0) {     // if buyer has not been canceled
     buyer.bought -= sale.amount;
     CoffeePurchases->AsyncApply(handle, id, ErasePurchaseEdge, sale.seller, sale.amount, sale.date);
     // ... TODO search for supplier to replace amount ...
} }


// Initiated by the purchaser at the site of the seller, this routine adjusts the seller's sold amount
// and removes the sale edge from the seller to the purchaser.
void CancelCoffeeSale(Handle & handle,
     const uint64_t & id, TraderVertex & seller, PurchaseEdge & purchase, RF_args_t & args) {
  auto CoffeeSales = SaleEdgeType::GetPtr((SaleEdgeType::ObjectID) args.CoffeeSales_OID);
  
  if (seller.sold > 0)  {      // if seller has not been canceled
     seller.sold -= purchase.amount;
     CoffeeSales->AsyncApply(handle, id, EraseSaleEdge, purchase.buyer, purchase.amount, purchase.date);
} }


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
      CoffeeTraders->AsyncApply(handle, sale.buyer, CancelCoffeePurchase, sale, args);
  for (auto purchase : purchases.value)         // alert my suppliers
      CoffeeTraders->AsyncApply(handle, purchase.seller, CancelCoffeeSale, purchase, args);
}


void PrintWeightedSalesEdgesToFile(const RF_args_t & args) {
  std::ofstream file;
  auto CoffeeSales = SaleEdgeType::GetPtr((SaleEdgeType::ObjectID) args.CoffeeSales_OID)->GetLocalMultimap();

  if (shad::rt::thisLocality() == (shad::rt::Locality) 0)
     file.open(args.filename);
  else
     file.open(args.filename, std::ios_base::app);

  for (auto itr = CoffeeSales->begin(); itr != CoffeeSales->end(); ++ itr)
    file << (* itr).second.seller << "," << (* itr).second.buyer << "," << (* itr).second.weight << "\n";

  file.close();
};


void CoffeeSalesWeight(Handle & handle, const uint64_t & seller, std::vector<SaleEdge> & sales, RF_args_t & args) {
  auto CoffeeTraders = TraderVertexType::GetPtr((TraderVertexType::ObjectID) args.CoffeeTraders_OID);

  TraderVertex trader;
  CoffeeTraders->Lookup(seller, & trader);
  for (auto & sale : sales) {
    sale.weight = sale.amount / trader.sold;
} }

} // namespace
