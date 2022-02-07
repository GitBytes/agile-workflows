#include <iomanip>
#include <iostream>

#include "agile/pi.h"

namespace shad {

int main(int argc, char *argv[]) {
  const size_t numberOfPoints = 1e6;

  auto pi = agile::aSliceOfPi(numberOfPoints);
  std::cout << "Pi is roughly "
            << std::setprecision(20)
            << pi << std::endl;

  return 0;
}

}
