#ifndef GRAPHTYPES_H_
#define GRAPHTYPES_H_

namespace agile::workflow4 {

enum class TYPES {
  PERSON,
  SERVER,
  TOPIC,
  RETAIL,         // retail customer, buys only
  DISTRIBUTOR,    // middle man, buys and sells goods
  PRODUCER,       // produces goods, quantity represented as self-edge, sells only
  SALE,
  PURCHASE,
  USES,
  FRIENDOF,
  SENDS,
  NONE
};

constexpr uint64_t NUMTYPES = (uint64_t) TYPES::NONE;

} // namespace agile::workflow4

#endif // GRAPHTYPES_H
