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
  auto CoffeeSales = SaleEdgeType::GetPtr((SaleEdgeType::ObjectID) args.CoffeeSales_OID);
  
  if (sellerVertex.sold > 0)  {      // if seller has not been canceled
     sellerVertex.sold -= edge.amount;
     CoffeeSales->AsyncBlockingApply(handle, seller, EraseSaleEdge, edge.buyer, edge.amount, edge.date);
} }


// Initiated by the seller at the site of the buyer, this routine adjusts the buyer's purchase amount,
// removes the purchase edge from the buyer to the seller and searches for one or more suppliers that
// can replace the lost purchase.
void CancelCoffeePurchase(const uint64_t & buyer, TraderVertex & buyerVertex, SaleEdge & edge, RF_args_t & args) {
  Handle handle = args.handle;
  auto CoffeeTraders   = TraderVertexType::GetPtr((TraderVertexType::ObjectID) args.CoffeeTraders_OID);
  auto CoffeeSales     = SaleEdgeType::GetPtr((SaleEdgeType::ObjectID) args.CoffeeSales_OID);
  auto CoffeePurchases = PurchaseEdgeType::GetPtr((PurchaseEdgeType::ObjectID) args.CoffeePurchases_OID);

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

  for (auto & purchase : purchases.value)       // alert my suppliers
      CoffeeTraders->TryBlockingApply(purchase.seller, CancelCoffeeSale, purchase, args);
  for (auto & sale : sales.value)               // alert my customers
      CoffeeTraders->TryBlockingApply(sale.buyer, CancelCoffeePurchase, sale, args);
}


void PrintWeightedSaleEdgesSimple(const RF_args_t & args) {
  std::ofstream file;
  file.open(args.filename, std::ios_base::app);
  auto CoffeeSales = SaleEdgeType::GetPtr((SaleEdgeType::ObjectID) args.CoffeeSales_OID)->GetLocalMultimap();

  for (auto itr = CoffeeSales->begin(); itr != CoffeeSales->end(); ++ itr)
    file << (* itr).second.seller << " " << (* itr).second.buyer << " " << (* itr).second.weight << "\n";
};


void PrintWeightedSaleEdgesComplex(const RF_args_t & args) {
  std::ofstream file;
  file.open(args.filename, std::ios_base::app);
  auto CoffeeTraders = TraderVertexType::GetPtr((TraderVertexType::ObjectID) args.CoffeeTraders_OID);
  auto CoffeeSales = SaleEdgeType::GetPtr((SaleEdgeType::ObjectID) args.CoffeeSales_OID)->GetLocalMultimap();

  std::map<uint64_t, uint64_t> traders;

  for (auto itr = CoffeeSales->begin(); itr != CoffeeSales->end(); ++ itr) {
    uint64_t buyer  = (* itr).second.buyer;
    uint64_t seller = (* itr).second.seller;
    double   weight = (* itr).second.weight;

    auto bV = traders.find(buyer);
    auto sV = traders.find(seller);
    uint64_t buyerType, sellerType;

    if (bV == traders.end()) {
       TraderVertex tmp;
       CoffeeTraders->Lookup(buyer, & tmp);
       traders.insert(std::make_pair(buyer, tmp.type));
       buyerType = tmp.type;
    } else {
       buyerType = (* bV).second;
    }

    if (sV == traders.end()) {
       TraderVertex tmp;
       CoffeeTraders->Lookup(seller, & tmp);
       traders.insert(std::make_pair(seller, tmp.type));
       sellerType = tmp.type;
    } else {
       sellerType = (* sV).second;
    }

    file << seller << " " << sellerType << " " << buyer << " " << buyerType << " " << weight << "\n";
} };


void PrintWeightedFriendEdges(const RF_args_t & args) {
  std::ofstream file;
  file.open(args.filename, std::ios_base::app);
  auto Friends = FriendOfEdgeType::GetPtr((FriendOfEdgeType::ObjectID) args.Friends_OID)->GetLocalMultimap();

  for (auto itr = Friends->begin(); itr != Friends->end(); ++ itr) {
    uint64_t person1 = (* itr).second.person1;
    uint64_t person2 = (* itr).second.person2;
    double   weight  = (* itr).second.weight;
    uint64_t type   = (uint64_t) TYPES::PERSON;
    file << person1 << " " << type << " " << person2 << " " << type << " " << weight << "\n";
} };


void PrintWeightedUsesEdges(const RF_args_t & args) {
  std::ofstream file;
  file.open(args.filename, std::ios_base::app);
  auto Uses = UsesEdgeType::GetPtr((UsesEdgeType::ObjectID) args.Uses_OID)->GetLocalMultimap();

  for (auto itr = Uses->begin(); itr != Uses->end(); ++ itr) {
    uint64_t person = (* itr).second.person;
    uint64_t server = (* itr).second.server;
    double   weight = (* itr).second.weight;
    uint64_t type1  = (uint64_t) TYPES::PERSON;
    uint64_t type2  = (uint64_t) TYPES::SERVER;
    file << person << " " << type1 << " " << server << " " << type2 << " " << weight << "\n";
} };


void PrintWeightedServerToServerEdges(const RF_args_t & args) {
  std::ofstream file;
  file.open(args.filename, std::ios_base::app);
  auto ServerToServer = ServerToServerEdgeType::GetPtr((ServerToServerEdgeType::ObjectID)
       args.ServerToServer_OID)->GetLocalMultimap();

  for (auto itr = ServerToServer->begin(); itr != ServerToServer->end(); ++ itr) {
    uint64_t src_device = (* itr).second.src_device;
    uint64_t dst_device = (* itr).second.dst_device;
    double   weight = (* itr).second.weight;
    uint64_t type   = (uint64_t) TYPES::SERVER;
    file << src_device << " " << type << " " << dst_device << " " << type << " " << weight << "\n";
} };


void CoffeeSalesWeight(Handle & handle, const uint64_t & seller, std::vector<SaleEdge> & sales, RF_args_t & args) {
  auto CoffeeTraders = TraderVertexType::GetPtr((TraderVertexType::ObjectID) args.CoffeeTraders_OID);

  TraderVertex trader;
  CoffeeTraders->Lookup(seller, & trader);
  for (auto & sale : sales) sale.weight = sale.amount / trader.sold;
}


void FriendsEdgeWeights(Handle & handle, const uint64_t & id, std::vector<FriendOfEdge> & edges, RF_args_t & args) {
  double denom = 1.0 / edges.size();
  for (auto & edge : edges) edge.weight = denom;
}


void UsesEdgeWeights(Handle & handle, const uint64_t & id, std::vector<UsesEdge> & edges, RF_args_t & args) {
  double denom = 1.0 / edges.size();
  for (auto & edge : edges) edge.weight = denom;
}


void SendsEdgeWeights(Handle & handle, const uint64_t & id, std::vector<SendsEdge> & edges, RF_args_t & args) {
  auto ServerToServer = ServerToServerEdgeType::GetPtr((ServerToServerEdgeType::ObjectID) args.ServerToServer_OID);

  double denom = 1.0 / edges.size();
  std::map<uint64_t, uint64_t> servers;
  for (auto edge : edges) servers[edge.dst_device] ++;           // count edges to each destination device

  for (auto server : servers) {     // weight is (number of edges to destination device) / number of edges
    uint64_t dst_device = server.first;
    double weight = server.second * denom;
    ServerToServer->BufferedAsyncInsert(handle, id, ServerToServerEdge(id, dst_device, weight));
} }

} // namespace
