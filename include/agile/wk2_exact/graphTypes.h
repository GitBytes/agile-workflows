#ifndef GRAPHTYPES_H_
#define GRAPHTYPES_H_

namespace agile::wk2_exact {

enum class TYPES {
  PERSON,
  FORUMEVENT,
  FORUM,
  PUBLICATION,
  TOPIC,
  PURCHASE,
  SALE,
  AUTHOR,
  INCLUDES,
  HASTOPIC,
  HASORG,
  NONE
};

constexpr uint64_t NUMTYPES = (uint64_t) TYPES::NONE;

} // namespace agile::wk2_exact

#endif // GRAPHTYPES_H
