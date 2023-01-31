
#include "agile/workflow4/queryUpdateGraph.h"

using intAtomic = shad::Atomic<int64_t>;
using intAtomicOID = shad::ObjectIdentifier<intAtomic>;

namespace agile::workflow4 {

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

// void replacePurchases(Handle & handle, )

void CancelPurchase(Handle & handle, const uint64_t& id, PersonVertex & person, double & amount, const RF_args_t & args) {
  person.coffee_purchased -= amount;
  // ... search coffee suppliers to make up amount ...
  
  // auto Sales = SaleEdgeType::GetPtr((SaleEdgeType::ObjectID) args.Sales_OID);
  auto Persons = PersonVertexType::GetPtr((PersonVertexType::ObjectID) args.Persons_OID);
  // auto Purchases = PurchaseEdgeType::GetPtr((PurchaseEdgeType::ObjectID) args.Purchases_OID);
  
  // double purchased_amount = Persons->AsyncForEachEntry(handle, id, )
  
  // NOTE: Search firsth from my previous and existing sellers (except the lost sellers).
  // for (auto itr = Persons->begin(); itr != Persons->end(); ++itr)
  // {
    
  // }
  // std::cout << "Cancelled Purchase Person(Buyer) Vertex Key: " << person.key() << std::endl;
}

void CancelSale(Handle & handle, const uint64_t& id, PersonVertex & person, double & amount, const RF_args_t & args) {
  person.coffee_sold -= amount;
  // ... search coffee suppliers (by buyers) to make up amount ...
  
  
}

void Cancel(Handle & handle, const uint64_t& id, PersonVertex & person, const std::vector<uint64_t>& influencers,
    //  uint64_t SaleEdgeOID, uint64_t PurchaseEdgeOID, uint64_t PersonVertexOID) {
      const RF_args_t & args ) {

  Handle handleS;
  Handle handleP;
  auto Sales = SaleEdgeType::GetPtr((SaleEdgeType::ObjectID) args.Sales_OID);
  auto Persons = PersonVertexType::GetPtr((PersonVertexType::ObjectID) args.Persons_OID);
  auto Purchases = PurchaseEdgeType::GetPtr((PurchaseEdgeType::ObjectID) args.Purchases_OID);

  for (auto inflncr_id : influencers)
  {
    if(person.id == inflncr_id)
    {
      SaleEdgeType::LookupResult sales;             // get my sales
      PurchaseEdgeType::LookupResult purchases;     // get my purchases
      Sales->Lookup(id, & sales);                   
      Purchases->Lookup(id, & purchases);
      std::cout << "Cancelled Purchase/Sale Person(Influencer) Vertex Key: " << person.key() << std::endl;
      for (auto purchase : purchases.value){
        Persons->AsyncApply(handleP, purchase.seller, CancelSale, purchase.amount, args);
        purchase.amount = 0; // null the purchase amount in purchase edge
      }

      for (auto sale : sales.value){
        Persons->AsyncApply(handleS, sale.buyer, CancelPurchase, sale.amount, args);
        sale.amount = 0; // null the sale amount in sale edge
      }

      person.coffee_sold = 0;                       // coffee sales canceled
      person.coffee_purchased = 0;                  // coffee purchases canceled
    }
  }

  waitForCompletion(handleS);
  waitForCompletion(handleP);
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

void reconfigureGraph(Graph_t & graph, const RF_args_t & args, const std::vector<uint64_t>& influencers)
{
  Handle handle;
  auto Persons = PersonVertexType::GetPtr((PersonVertexType::ObjectID) graph["Persons"]);
  Persons->AsyncForEachEntry(handle, Cancel, influencers, args);

  waitForCompletion(handle);
}

} // namespace