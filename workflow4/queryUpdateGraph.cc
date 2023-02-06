
#include "agile/workflow4/queryUpdateGraph.h"

using intAtomic = shad::Atomic<int64_t>;
using intAtomicOID = shad::ObjectIdentifier<intAtomic>;

namespace agile::workflow4 {

bool traderExists(const uint64_t & id, const std::vector<uint64_t> & lost_traders)
{
  for(auto lost_id : lost_traders)
  {
    if(id == lost_id)
      return true;
  }
  return false;

}

void purchaseAmount(Handle& handle, const uint64_t & id, PersonVertex & person, uint64_t & product, uint64_t & PurchaseEdgeOID) {
  auto Purchases = PurchaseEdgeType::GetPtr((PurchaseEdgeType::ObjectID) PurchaseEdgeOID);
  PurchaseEdgeType::LookupResult purchases;
  Purchases->Lookup(id, & purchases);                                            // get my purchases

  for (auto purchase : purchases.value) {
    if (purchase.product == 8486) {
      person.coffee_purchased += purchase.amount;    // coffee purchase
      person.coffee_purchased_old += purchase.amount;
    }
  }
  waitForCompletion(handle);
}

void soldAmount(Handle & handle, const uint64_t & id, PersonVertex & person,
                uint64_t& product, uint64_t& SaleEdgeOID) {

  auto Sales = SaleEdgeType::GetPtr((SaleEdgeType::ObjectID) SaleEdgeOID);
  SaleEdgeType::LookupResult sales;
  Sales->Lookup(id, & sales);                                                      // get my sales

  for (auto sale : sales.value){
    if (sale.product == 8486) {
      person.coffee_sold      += sale.amount;                   // coffee purchase
      person.coffee_sold_old  += sale.amount;
    }
  }
  
  waitForCompletion(handle);
}

// Cancel purchase and search for deficit coffee
void CancelPurchase(Handle & handle, const uint64_t& id, PersonVertex & person, double & amount, const std::vector<uint64_t> & lost_traders,const RF_args_t & args) {
  person.coffee_purchased -= amount;
  // ... search coffee suppliers to make up amount ...
  
  // auto Sales = SaleEdgeType::GetPtr((SaleEdgeType::ObjectID) args.Sales_OID);
  auto CoffeeTraders    = PersonVertexType::GetPtr((PersonVertexType::ObjectID) args.CoffeeTraders_OID);
  auto CoffeePurchases  = PurchaseEdgeType::GetPtr((PurchaseEdgeType::ObjectID) args.CoffeePurchases_OID);
  auto CoffeeSales      = SaleEdgeType::GetPtr((SaleEdgeType::ObjectID) args.CoffeeSales_OID);

  // Search coffee from my previous sellers
  std::vector<uint64_t> my_sellers = lost_traders; 
  PurchaseEdgeType::LookupResult purchases;     // get my purchases (old)
  CoffeePurchases->Lookup(id, & purchases);
  double buyer_deficit = person.coffee_purchased_old - person.coffee_purchased;
  // TODO - refactor the loop below
  while(buyer_deficit > 0) {
    // Search coffee from my previous sellers first (only once)
    auto purchase_ref = purchases.value[0];
    for (auto purchase : purchases.value)
    {
      if(!(traderExists(purchase.seller, lost_traders)))
      {
        my_sellers.push_back(purchase.seller);
        PersonVertex seller;
        CoffeeTraders->Lookup(purchase.seller, &seller);
        double seller_surplus = seller.coffee_sold_old - seller.coffee_sold;
        if(seller_surplus > 0)
        {
          if(buyer_deficit <= seller_surplus)
          {
            seller.coffee_sold += buyer_deficit;
            person.coffee_purchased += buyer_deficit;

            PurchaseEdge new_purchase;
            {
              new_purchase.seller   = seller.key();
              new_purchase.buyer    = person.key();
              new_purchase.product  = purchase.product;
              new_purchase.date     = purchase.date;
              new_purchase.amount   = buyer_deficit;
              new_purchase.src_type = purchase.src_type;
              new_purchase.dst_type = purchase.dst_type;
            }
            CoffeePurchases->BufferedAsyncInsert(handle, new_purchase.key(), new_purchase);
            SaleEdge new_sale;
            {
              new_sale.seller   = seller.key();
              new_sale.buyer    = person.key();
              new_sale.product  = purchase.product;
              new_sale.date     = purchase.date;
              new_sale.amount   = buyer_deficit;
              new_sale.src_type = purchase.dst_type;
              new_sale.dst_type = purchase.src_type;
            }
            CoffeeSales->BufferedAsyncInsert(handle, new_sale.key(), new_sale);
            waitForCompletion(handle);
            buyer_deficit = 0;
            return;
          }
          else
          {
            seller.coffee_sold += seller_surplus;
            person.coffee_purchased += seller_surplus;
            PurchaseEdge new_purchase;
            {
              new_purchase.seller   = seller.key();
              new_purchase.buyer    = person.key();
              new_purchase.product  = purchase.product;
              new_purchase.date     = purchase.date;
              new_purchase.amount   = seller_surplus;
              new_purchase.src_type = purchase.src_type;
              new_purchase.dst_type = purchase.dst_type;
            }
            CoffeePurchases->BufferedAsyncInsert(handle, new_purchase.key(), new_purchase);
            SaleEdge new_sale;
            {
              new_sale.seller   = seller.key();
              new_sale.buyer    = person.key();
              new_sale.product  = purchase.product;
              new_sale.date     = purchase.date;
              new_sale.amount   = seller_surplus;
              new_sale.src_type = purchase.dst_type;
              new_sale.dst_type = purchase.src_type;
            }
            CoffeeSales->BufferedAsyncInsert(handle, new_sale.key(), new_sale);
            waitForCompletion(handle);
            buyer_deficit = buyer_deficit - seller_surplus;
          }
        }
      } 
    }
    
    // Search coffee from new sellers
    for (auto itr = CoffeeTraders->begin(); itr != CoffeeTraders->end(); ++itr)
    {
      std::pair<uint64_t, PersonVertex> entry = (* itr);
      if(!(traderExists(entry.first, my_sellers)))
      {
        PersonVertex seller = entry.second;
        double seller_surplus = seller.coffee_sold_old - seller.coffee_sold;
        if(seller_surplus > 0)
        {
          if(buyer_deficit <= seller_surplus)
          {
            seller.coffee_sold += buyer_deficit;
            person.coffee_purchased += buyer_deficit;

            PurchaseEdge new_purchase;
            {
              new_purchase.seller   = seller.key();
              new_purchase.buyer    = person.key();
              new_purchase.product  = purchase_ref.product;
              new_purchase.date     = purchase_ref.date;
              new_purchase.amount   = buyer_deficit;
              new_purchase.src_type = purchase_ref.src_type;
              new_purchase.dst_type = purchase_ref.dst_type;
            }
            CoffeePurchases->BufferedAsyncInsert(handle, new_purchase.key(), new_purchase);
            SaleEdge new_sale;
            {
              new_sale.seller   = seller.key();
              new_sale.buyer    = person.key();
              new_sale.product  = purchase_ref.product;
              new_sale.date     = purchase_ref.date;
              new_sale.amount   = buyer_deficit;
              new_sale.src_type = purchase_ref.dst_type;
              new_sale.dst_type = purchase_ref.src_type;
            }
            CoffeeSales->BufferedAsyncInsert(handle, new_sale.key(), new_sale);
            waitForCompletion(handle);
            buyer_deficit = 0;
            return;
          }
          else
          {
            seller.coffee_sold += seller_surplus;
            person.coffee_purchased += seller_surplus;
            PurchaseEdge new_purchase;
            {
              new_purchase.seller   = seller.key();
              new_purchase.buyer    = person.key();
              new_purchase.product  = purchase_ref.product;
              new_purchase.date     = purchase_ref.date;
              new_purchase.amount   = seller_surplus;
              new_purchase.src_type = purchase_ref.src_type;
              new_purchase.dst_type = purchase_ref.dst_type;
            }
            CoffeePurchases->BufferedAsyncInsert(handle, new_purchase.key(), new_purchase);
            SaleEdge new_sale;
            {
              new_sale.seller   = seller.key();
              new_sale.buyer    = person.key();
              new_sale.product  = purchase_ref.product;
              new_sale.date     = purchase_ref.date;
              new_sale.amount   = seller_surplus;
              new_sale.src_type = purchase_ref.dst_type;
              new_sale.dst_type = purchase_ref.src_type;
            }
            CoffeeSales->BufferedAsyncInsert(handle, new_sale.key(), new_sale);
            waitForCompletion(handle);
            buyer_deficit = buyer_deficit - seller_surplus;
          }
        }
      }
    }
  };
}

void CancelSale(Handle & handle, const uint64_t& id, PersonVertex & person, double & amount, const RF_args_t & args) {
  person.coffee_sold -= amount;
  // ... search coffee suppliers (by buyers) to make up amount ...
  
}

void CancelCoffee(Handle & handle, const uint64_t& id, PersonVertex & person, const std::vector<uint64_t>& lost_traders,
      const RF_args_t & args ) {

  Handle handleS;
  Handle handleP;
  auto CoffeeTraders    = PersonVertexType::GetPtr((PersonVertexType::ObjectID) args.CoffeeTraders_OID);
  auto CoffeeSales      = SaleEdgeType::GetPtr((SaleEdgeType::ObjectID) args.CoffeeSales_OID);
  auto CoffeePurchases  = PurchaseEdgeType::GetPtr((PurchaseEdgeType::ObjectID) args.CoffeePurchases_OID);

  // cancel purchases first
  if(traderExists(person.id, lost_traders))
  {
    PurchaseEdgeType::LookupResult purchases;     // get my purchases
    CoffeePurchases->Lookup(id, & purchases);
    person.coffee_sold = 0;                       // coffee sales canceled
    person.coffee_purchased = 0;                  // coffee purchases canceled

    // Cancel id's purchases.
    for (auto purchase : purchases.value){
      CoffeeTraders->AsyncApply(handleP, purchase.seller, CancelSale, purchase.amount, args);
      purchase.amount = 0; // null the purchase amount in purchase edge
    }
  }
  waitForCompletion(handleP);

  // cancel sales
  if(traderExists(person.id, lost_traders))
  {
    SaleEdgeType::LookupResult sales;             // get my sales
    CoffeeSales->Lookup(id, & sales);                   
    
    // Cancel id's sales.
    for (auto sale : sales.value){
      CoffeeTraders->AsyncApply(handleS, sale.buyer, CancelPurchase, sale.amount, lost_traders, args);
      sale.amount = 0; // null the sale amount in sale edge
    }
  }
  waitForCompletion(handleS);
}

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

    // // update Person vertex as well
    // auto Persons = PersonVertexType::GetPtr((PersonVertexType::ObjectID) args.Persons_OID);
    // for 

    for (auto se : sales)
    {
        if(se.product == 8486)
            total_sales+= se.amount;
    }
    // std::cout << " Total Coffee Sales: " << total_sales << "\n";
    auto CoffeeSales     = SaleEdgeType::GetPtr((SaleEdgeOID) args.CoffeeSales_OID);
    auto CoffeePurchases = PurchaseEdgeType::GetPtr((PurchaseEdgeOID) args.CoffeePurchases_OID);

    for (auto se : sales)
    {
        if(se.product == 8486)
        {
            se.weight = se.amount/total_sales;
            CoffeeSales->BufferedAsyncInsert(handle, se.key(), se);
            PurchaseEdge purchase;
            {
              purchase.seller   = se.buyer;
              purchase.buyer    = se.seller;
              purchase.product  = se.product;
              purchase.date     = se.date;
              purchase.amount   = se.amount;
              purchase.src_type = se.dst_type;
              purchase.dst_type = se.src_type;
            }
            CoffeePurchases->BufferedAsyncInsert(handle, purchase.key(), purchase);
        }    
    }
    shad::rt::waitForCompletion(handle);
}

// Redundant
void getCoffeePurchases(const uint64_t& buyer, std::vector<PurchaseEdge>& purchases, const RF_args_t & args)
{
    Handle handle;
    double total_purchases = 0;

    // // update Person vertex as well
    // auto Persons = PersonVertexType::GetPtr((PersonVertexType::ObjectID) args.Persons_OID);
    // for 

    for (auto pe : purchases)
    {
        if(pe.product == 8486)
            total_purchases+= pe.amount;
    }
    // std::cout << " Total Coffee Sales: " << total_sales << "\n";
    // auto CoffeeSales     = SaleEdgeType::GetPtr((SaleEdgeOID) args.CoffeeSales_OID);
    auto CoffeePurchases = PurchaseEdgeType::GetPtr((PurchaseEdgeOID) args.CoffeePurchases_OID);

    for (auto pe : purchases)
    {
        if(pe.product == 8486)
        {
            pe.weight = pe.amount/total_purchases;
            CoffeePurchases->BufferedAsyncInsert(handle, pe.key(), pe);
        }    
    }
    shad::rt::waitForCompletion(handle);
}

void coffeeTraders(Handle & handle, const uint64_t& id, PersonVertex & person, const RF_args_t & args)
{
  auto CoffeeTraders = PersonVertexType::GetPtr((PersonVertexOID)args.Persons_OID);
  if((person.coffee_sold >0) || (person.coffee_purchased > 0))
  {
    CoffeeTraders->BufferedAsyncInsert(handle, person.key(), person);
  }
}

void getCoffeeSaleEdgeWeights(Graph_t & graph, const RF_args_t & args, uint64_t  product_id)
{
    Handle handle, handle2;
    // auto Sales  = SaleEdgeType::GetPtr( (SaleEdgeOID) salesEdge_OID);
    // Create Coffee Sales Edges
    // CoffeeSalesEdge_(handle, key_coffee, salesEdge_OID);

    uint64_t product = 8486; // coffee
    
    auto Persons = PersonVertexType::GetPtr((PersonVertexOID) graph["Persons"]);
    // Update Person Vertex - Sold Amount (original)
    Persons->AsyncForEachEntry(handle, soldAmount, product, graph["Sales"]);
    // Update Person Vertex - Purchases (original)
    Persons->AsyncForEachEntry(handle2, purchaseAmount, product, graph["Purchases"]);

    auto G_sales = SaleEdgeType::GetPtr((SaleEdgeOID) graph["Sales"]);

    std::cout << " Sales Size: " << G_sales->Size() << " \n";
    G_sales->ForEachEntry(getCoffeeSales, args);
    
    auto G_coffeesales = SaleEdgeType::GetPtr((SaleEdgeOID) graph["CoffeeSales"]);
    std::cout << " Total Coffee Sales: " << G_coffeesales->Size() << "\n";
    std::ofstream file_out;
    G_coffeesales->ForEachEntry(printCoffeeEdgesToFile, args);
}

void getCoffeeTraders(Graph_t & graph, const RF_args_t & args)
{
  Handle handle;
  auto Persons = PersonVertexType::GetPtr((PersonVertexType::ObjectID) graph["Persons"]);
  Persons->AsyncForEachEntry(handle, coffeeTraders, args);

  shad::rt::waitForCompletion(handle);
}

void reconfigureGraph(Graph_t & graph, const std::vector<uint64_t>& influencers,const RF_args_t & args)
{
  Handle handle;
  // auto Persons = PersonVertexType::GetPtr((PersonVertexType::ObjectID) graph["Persons"]);
  auto CoffeeTraders = PersonVertexType::GetPtr((PersonVertexType::ObjectID) graph["CoffeeTraders"]);

  CoffeeTraders->AsyncForEachEntry(handle, CancelCoffee, influencers, args);

  waitForCompletion(handle);
}

} // namespace