#include <random>

#include "shad/core/algorithm.h"
#include "shad/core/array.h"
#include "shad/core/numeric.h"

namespace agile {
double aSliceOfPi(size_t samplingEffort) {
  shad::array<uint64_t, 128> counters;

  const size_t numberOfPointsPerSim = samplingEffort / counters.size();

  shad::generate(shad::distributed_parallel_tag{}, counters.begin(),
                 counters.end(), [=]() -> uint64_t {
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

  uint64_t count = shad::reduce(shad::distributed_parallel_tag{},
                                counters.begin(), counters.end());

  return (4.0 * count) / samplingEffort;
}
}
