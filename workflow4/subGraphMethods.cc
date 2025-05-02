/*===------------------------------------------------------------*- C++ -*-===
 *
 *                            The AGILE Workflows
 *
 *===----------------------------------------------------------------------===
 *
 * Copyright (c) 2025 Battelle Memorial Institute
 *
 * Battelle Memorial Institute (hereinafter Battelle) hereby grants permission
 * to any person or entity lawfully obtaining a copy of this software and
 * associated documentation files (hereinafter “the Software”) to redistribute
 * and use the Software in source and binary forms, with or without
 * modification. Such person or entity may use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and may permit
 * others to do so, subject to the following conditions:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimers.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Other than as used herein, neither the name Battelle Memorial Institute or
 *    Battelle may be used in any form whatsoever without the express written
 *    consent of Battelle.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL BATTELLE OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *===----------------------------------------------------------------------===*/
#include "agile/workflow4/main.h"
#include "agile/workflow4/graph.h"

namespace agile::workflow4 {

void PrintSaleEdgesSimple(const RF_args_t & args) {
  std::ofstream file;
  file.open(args.filename, std::ios_base::app);
  auto CoffeeSales = SaleEdgeType::GetPtr((SaleEdgeType::ObjectID) args.Sales_OID)->GetLocalMultimap();

  for (auto itr = CoffeeSales->begin(); itr != CoffeeSales->end(); ++ itr)
    file << (* itr).second.seller << " " << (* itr).second.buyer << " " << (* itr).second.weight << "\n";
};


void PrintSaleEdgesComplex(const RF_args_t & args) {
  std::ofstream file;
  file.open(args.filename, std::ios_base::app);
  auto CoffeeTraders = TraderVertexType::GetPtr((TraderVertexType::ObjectID) args.Persons_OID);
  auto CoffeeSales = SaleEdgeType::GetPtr((SaleEdgeType::ObjectID) args.Sales_OID)->GetLocalMultimap();

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


void PrintFriendEdges(const RF_args_t & args) {
  std::ofstream file;
  file.open(args.filename, std::ios_base::app);
  auto Friends = FriendEdgeType::GetPtr((FriendEdgeType::ObjectID) args.Friends_OID)->GetLocalMultimap();

  for (auto itr = Friends->begin(); itr != Friends->end(); ++ itr) {
    uint64_t person1 = (* itr).second.person1;
    uint64_t person2 = (* itr).second.person2;
    double   weight  = (* itr).second.weight;
    uint64_t type   = (uint64_t) TYPES::PERSON;
    file << person1 << " " << type << " " << person2 << " " << type << " " << weight << "\n";
} };


void PrintUsesEdges(const RF_args_t & args) {
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


void PrintServerSendEdges(const RF_args_t & args) {
  auto ServerSends = ServerSendEdgeType::GetPtr((ServerSendEdgeType::ObjectID) args.ServerSends_OID);

  std::ofstream file;
  file.open(args.filename, std::ios_base::app);
  auto LocalServerSends = ServerSends->GetLocalMultimap();

  for (auto itr = LocalServerSends->begin(); itr != LocalServerSends->end(); ++ itr) {
    uint64_t src_device = (* itr).second.src_device;
    uint64_t dst_device = (* itr).second.dst_device;
    double   weight = (* itr).second.weight;
    uint64_t type   = (uint64_t) TYPES::SERVER;
    file << src_device << " " << type << " " << dst_device << " " << type << " " << weight << "\n";
} };


void SaleWeights(Handle & handle, const uint64_t & seller, std::vector<SaleEdge> & sales, RF_args_t & args) {
  auto CoffeeTraders = TraderVertexType::GetPtr((TraderVertexType::ObjectID) args.Persons_OID);

  TraderVertex trader;
  CoffeeTraders->Lookup(seller, & trader);
  for (auto & sale : sales) sale.weight = sale.amount / trader.sold;
}


void FriendWeights(Handle & handle, const uint64_t & id, std::vector<FriendEdge> & edges, RF_args_t & args) {
  double denom = 1.0 / edges.size();
  for (auto & edge : edges) edge.weight = denom;
}


void UseWeights(Handle & handle, const uint64_t & id, std::vector<UsesEdge> & edges, RF_args_t & args) {
  double denom = 1.0 / edges.size();
  for (auto & edge : edges) edge.weight = denom;
}


void ServerSendWeights(Handle & handle, const uint64_t & id, std::vector<SendEdge> & edges, RF_args_t & args) {
  auto ServerSends = ServerSendEdgeType::GetPtr((ServerSendEdgeType::ObjectID) args.ServerSends_OID);

  double denom = 1.0 / edges.size();
  std::map<uint64_t, uint64_t> servers;
  for (auto edge : edges) servers[edge.dst_device] ++;           // count edges to each destination device

  for (auto server : servers) {     // weight is (number of edges to destination device) / number of edges
    uint64_t dst_device = server.first;
    double weight = server.second * denom;
    ServerSends->BufferedAsyncInsert(handle, id, ServerSendEdge(id, dst_device, weight));
} }


void Sends_(Handle & handle, const uint64_t & key, std::vector<SendEdge> & edges, RF_args_t & args) {
  ServerVertex result;
  auto CoffeeSends   = SendEdgeType::GetPtr((SendEdgeType::ObjectID) args.Sends_OID);
  auto CoffeeServers = ServerVertexType::GetPtr((ServerVertexType::ObjectID) args.Servers_OID);

  for (auto & edge : edges)
    if (CoffeeServers->Lookup(edge.dst_device, & result))
       CoffeeSends->BufferedAsyncInsert(handle, key, edge);
}


void SendsSubgraph(Handle & handle, const uint64_t & key,
     ServerVertex & person, RF_args_t & coffeeArgs, RF_args_t & args) {
  auto Sends = SendEdgeType::GetPtr((SendEdgeType::ObjectID) args.Sends_OID);

  Sends->AsyncApply(handle, key, Sends_, coffeeArgs);
}


void Friends_(Handle & handle, const uint64_t & key, std::vector<FriendEdge> & edges, RF_args_t & args) {
  TraderVertex result;
  auto CoffeeFriends = FriendEdgeType::GetPtr((FriendEdgeType::ObjectID) args.Friends_OID);
  auto CoffeeTraders = TraderVertexType::GetPtr((TraderVertexType::ObjectID) args.Persons_OID);

  for (auto & edge : edges)
    if (CoffeeTraders->Lookup(edge.person2, & result))
       CoffeeFriends->BufferedAsyncInsert(handle, key, edge);
}


void FriendsSubgraph(Handle & handle, const uint64_t & key,
     TraderVertex & person, RF_args_t & coffeeArgs, RF_args_t & args) {
  auto Friends = FriendEdgeType::GetPtr((FriendEdgeType::ObjectID) args.Friends_OID);

  Friends->AsyncApply(handle, key, Friends_, coffeeArgs);
}


void Servers_(Handle & handle, const uint64_t & key, std::vector<UsesEdge> & edges, RF_args_t & args) {
  auto CoffeeUses = UsesEdgeType::GetPtr((UsesEdgeType::ObjectID) args.Uses_OID);
  auto CoffeeServers = ServerVertexType::GetPtr((ServerVertexType::ObjectID) args.Servers_OID);

  for (auto & edge : edges) {
      CoffeeUses->BufferedAsyncInsert(handle, key, edge);
      CoffeeServers->BufferedAsyncInsert(handle, key, ServerVertex(edge.server));
} }


void ServersSubgraph(Handle & handle, const uint64_t & key,
     TraderVertex & person, RF_args_t & coffeeArgs, RF_args_t & args) {
  auto Uses = UsesEdgeType::GetPtr((UsesEdgeType::ObjectID) args.Uses_OID);

  Uses->AsyncApply(handle, key, Servers_, coffeeArgs);
}

} // namespace
