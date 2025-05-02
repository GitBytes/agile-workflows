#include <algorithm>
#include <gtest/gtest.h>
#include <iterator>
#include <rapidcheck/gtest.h>
#include <string>

#include <iostream>

#include "agile/pi.h"

RC_GTEST_PROP(PieceOfPi, firstThreeDigitsAreCorrect, ()) {
  const auto x = *rc::gen::inRange(10000000, 20000000);
  double piEstimate = agile::aSliceOfPi(x);

  std::string reference{"3.14"};
  RC_ASSERT(std::equal(reference.begin(), reference.end(),
                       std::to_string(piEstimate).begin()));
}
