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


void BuyProduct(const uint64_t & id, TraderVertex & seller, uint8_t * result, uint32_t * resSize, RF_args_t & args) {
  double sale = std::min(seller.bought - seller.sold, args.to_buy);
  * ((double *) result) = sale;
  * resSize = sizeof(double);
  seller.sold -= sale;
}


// Initiated by the purchaser at the site of the seller, this routine adjusts the seller's sold amount
// and removes the sale edge from the seller to the purchaser.
void CancelCoffeeSale(Handle & handle,
     const uint64_t & seller, TraderVertex & sellerVertex, PurchaseEdge & purchase, RF_args_t & args) {
  auto CoffeeSales = SaleEdgeType::GetPtr((SaleEdgeType::ObjectID) args.CoffeeSales_OID);
  
  if (sellerVertex.sold > 0)  {      // if seller has not been canceled
     sellerVertex.sold -= purchase.amount;
     CoffeeSales->AsyncBlockingApply(handle, seller, EraseSaleEdge, purchase.buyer, purchase.amount, purchase.date);
} }


// Initiated by the seller at the site of the buyer, this routine adjusts the buyer's purchase amount,
// removes the purchase edge from the buyer to the seller and searches for one or more suppliers that
// can replace the lost purchase.
void CancelCoffeePurchase(Handle & handle,
     const uint64_t & buyer, TraderVertex & buyerVertex, SaleEdge & sale, RF_args_t & args) {
  auto CoffeeTraders = TraderVertexType::GetPtr((TraderVertexType::ObjectID) args.CoffeeTraders_OID);
  auto CoffeePurchases = PurchaseEdgeType::GetPtr((PurchaseEdgeType::ObjectID) args.CoffeePurchases_OID);

  if (buyerVertex.bought > 0) {                                  // if buyer has not been canceled
     buyerVertex.bought -= sale.amount;                          // ... reduce amount of coffee being purchased
     CoffeePurchases->AsyncBlockingApply(handle, buyer, ErasePurchaseEdge, sale.seller, sale.amount, sale.date);

     PurchaseEdgeType::LookupResult purchases;                   // ... lookup coffee suppliers for this buyer
     CoffeePurchases->Lookup(buyer, & purchases);
     if (! purchases.found) return;                              // ... this buyer has no other suppliers

     args.buyer  = buyer;
     args.handle = handle;
     args.to_buy = buyerVertex.desired - buyerVertex.bought;     // ... amount of coffee to be bought

     PurchaseEdge edge;
     edge.buyer    = buyer;
     edge.product  = 8486;
     edge.date     = shad::data_types::kNullValue<time_t>;
     edge.weight   = shad::data_types::kNullValue<double>;
     edge.src_type = TYPES::NONE;
     edge.dst_type = TYPES::NONE;

     for (auto itr = purchases.value.begin(); itr != purchases.value.end(); ++ itr) {
       double   result;
       uint32_t resultSize;
       uint64_t seller = (* itr).seller;

       CoffeeTraders->TryBlockingApplyWithRetBuff(seller, BuyProduct, (uint8_t *) (& result), & resultSize, args);

       if (result > 0.0) {                                       // ... if seller had coffee to sell
          edge.seller  = seller;
          edge.amount  = result;
          args.to_buy -= result;
          CoffeePurchases->BufferedAsyncInsert(handle, buyer, edge);
       }

       if (args.to_buy == 0.0) break;                            // ... no more coffee to buy
} } }


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
  CoffeePurchases->Lookup(id, & purchases);     // get my coffee purchases
  CoffeePurchases->Erase(id);                   // erase my purchase edges from the graph

  for (auto & purchase : purchases.value)         // alert my suppliers
      CoffeeTraders->AsyncApply(handle, purchase.seller, CancelCoffeeSale, purchase, args);
  for (auto & sale : sales.value)                 // alert my customers
      CoffeeTraders->AsyncApply(handle, sale.buyer, CancelCoffeePurchase, sale, args);
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
