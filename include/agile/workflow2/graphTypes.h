#ifndef GRAPHTYPES_H_
#define GRAPHTYPES_H_

namespace agile::workflow2 {

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
  VERTEX,
  EDGE,
  NONE
};

class Freq{
public:
  uint64_t counter[100];
  Freq()
  {
    for(int i=0; i<100;i++)
      counter[i]=0;
  }
};

} // namespace agile::workflow2

#endif // GRAPHTYPES_H
