#include <iomanip>
#include <iostream>
#include <random>

#include "shad/core/algorithm.h"
#include "shad/core/array.h"
#include "shad/core/numeric.h"

namespace shad {

int main(int argc, char *argv[]) {
  shad::array<uint64_t, 128> counters;

  const size_t numberOfPoints = 1e10;
  const size_t numberOfPointsPerSim = numberOfPoints / counters.size();

  shad::generate(
      shad::distributed_parallel_tag{},
      counters.begin(), counters.end(),
      [=]() -> uint64_t {
        size_t counter = 0;

        std::random_device rd;
        std::default_random_engine G(rd());
        std::uniform_real_distribution<double> dist(0.0, 1.0);

        for (size_t i = 0; i < numberOfPointsPerSim; ++i) {
          double x = dist(G);
          double y = dist(G);
          if ((x * x + y * y) < 1) {
            ++counter;
          }
        }
        return counter;
      });

  uint64_t count = shad::reduce(
      shad::distributed_parallel_tag{}, counters.begin(), counters.end());

  std::cout << "Pi is roughly "
            << std::setprecision(20)
            << (4.0 * count) / numberOfPoints << std::endl;

  return 0;
}

}
