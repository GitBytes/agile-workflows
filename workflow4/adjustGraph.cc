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


void BuyProduct(const uint64_t & seller, TraderVertex & sellerVertex,
     uint8_t * result, uint32_t * resSize, RF_args_t & args) {

  if (sellerVertex.bought > sellerVertex.sold) {     // seller has something to sell
     double sale = std::min(sellerVertex.bought - sellerVertex.sold, args.to_buy);
     sellerVertex.sold += sale;
     * ((double *) result) = sale;
     * resSize = sizeof(double);

  } else {                                           // seller has nothing to sell
     * ((double *) result) = 0.0;
     * resSize = sizeof(double);
} }


// Initiated by the purchaser at the site of the seller, this routine adjusts the seller's sold amount
// and removes the sale edge from the seller to the purchaser.
void CancelCoffeeSale(const uint64_t & seller, TraderVertex & sellerVertex, PurchaseEdge & edge, RF_args_t & args) {
  Handle handle = args.handle;
  auto CoffeeSales = SaleEdgeType::GetPtr((SaleEdgeType::ObjectID) args.Sales_OID);
  
  if (sellerVertex.sold > 0)  {      // if seller has not been canceled
     sellerVertex.sold -= edge.amount;
     CoffeeSales->AsyncBlockingApply(handle, seller, EraseSaleEdge, edge.buyer, edge.amount, edge.date);
} }


// Initiated by the seller at the site of the buyer, this routine adjusts the buyer's purchase amount,
// removes the purchase edge from the buyer to the seller and searches for one or more suppliers that
// can replace the lost purchase.
void CancelCoffeePurchase(const uint64_t & buyer, TraderVertex & buyerVertex, SaleEdge & edge, RF_args_t & args) {
  Handle handle = args.handle;
  auto CoffeeTraders   = TraderVertexType::GetPtr((TraderVertexType::ObjectID) args.Persons_OID);
  auto CoffeeSales     = SaleEdgeType::GetPtr((SaleEdgeType::ObjectID) args.Sales_OID);
  auto CoffeePurchases = PurchaseEdgeType::GetPtr((PurchaseEdgeType::ObjectID) args.Purchases_OID);

  if (buyerVertex.bought > 0) {                                  // if buyer has not been canceled
     buyerVertex.bought -= edge.amount;                          // ... reduce amount of coffee being purchased
     CoffeePurchases->AsyncBlockingApply(handle, buyer, ErasePurchaseEdge, edge.seller, edge.amount, edge.date);

     PurchaseEdgeType::LookupResult purchases;                   // ... lookup coffee suppliers for this buyer
     CoffeePurchases->Lookup(buyer, & purchases);
     if (! purchases.found) return;                              // ... this buyer has no other suppliers

     args.buyer  = buyer;
     args.to_buy = buyerVertex.desired - buyerVertex.bought;     // ... amount of coffee to be bought

     SaleEdge sale;
     sale.buyer    = buyer;
     sale.product  = 8486;
     sale.date     = shad::data_types::kNullValue<time_t>;
     sale.weight   = shad::data_types::kNullValue<double>;
     sale.src_type = TYPES::PERSON;
     sale.dst_type = TYPES::PERSON;

     for (auto itr = purchases.value.begin(); itr != purchases.value.end(); ++ itr) {
       double   result;
       uint32_t resultSize;
       uint64_t seller = (* itr).seller;

       CoffeeTraders->TryBlockingApplyWithRetBuff(seller, BuyProduct, (uint8_t *) (& result), & resultSize, args);

       if (result > 0.0) {                                                   // ... if seller had coffee to sell
          sale.seller = seller;
          sale.amount = result;
          CoffeeSales->BufferedAsyncInsert(handle, seller, sale);            // ... ... add sale edge

          PurchaseEdge purchase(sale);
          CoffeePurchases->BufferedAsyncInsert(handle, buyer, purchase);     // ... ... add purchase edge

          buyerVertex.bought += result;                                      // ... ... increment amount bought
          args.to_buy -= result;                                             // ... ... decrement amount to buy
       }

       if (args.to_buy == 0.0) break;                                    // ... all done
} } }


void CancelCoffeeTrader(Handle & handle, const uint64_t & id, TraderVertex & trader, RF_args_t & args) {
  SaleEdgeType::LookupResult sales;
  PurchaseEdgeType::LookupResult purchases;
  auto CoffeeTraders   = TraderVertexType::GetPtr((TraderVertexType::ObjectID) args.Persons_OID);
  auto CoffeeSales     = SaleEdgeType::GetPtr((SaleEdgeType::ObjectID) args.Sales_OID);
  auto CoffeePurchases = PurchaseEdgeType::GetPtr((PurchaseEdgeType::ObjectID) args.Purchases_OID);

  trader.sold = 0.0;                            // zero out my coffee sales
  trader.bought = 0.0;                          // zero out my coffee purchases
  trader.desired = 0.0;                         // zero out my desired coffee purchases

  CoffeeSales->Lookup(id, & sales);             // get my coffee sales
  CoffeeSales->Erase(id);                       // erase my sale edges from the graph
  CoffeePurchases->Lookup(id, & purchases);     // get my coffee purchases
  CoffeePurchases->Erase(id);                   // erase my purchase edges from the graph

  for (auto & purchase : purchases.value)       // alert my suppliers
      CoffeeTraders->TryBlockingApply(purchase.seller, CancelCoffeeSale, purchase, args);
  for (auto & sale : sales.value)               // alert my customers
      CoffeeTraders->TryBlockingApply(sale.buyer, CancelCoffeePurchase, sale, args);
}

} // namespace
