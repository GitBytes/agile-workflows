//===------------------------------------------------------------*- C++ -*-===//
//
//                            The AGILE Workflows
//
//===----------------------------------------------------------------------===//
// ** Pre-Copyright Notice
//
// This computer software was prepared by Battelle Memorial Institute,
// hereinafter the Contractor, under Contract No. DE-AC05-76RL01830 with the
// Department of Energy (DOE). All rights in the computer software are reserved
// by DOE on behalf of the United States Government and the Contractor as
// provided in the Contract. You are authorized to use this computer software
// for Governmental purposes but it is not to be released or distributed to the
// public. NEITHER THE GOVERNMENT NOR THE CONTRACTOR MAKES ANY WARRANTY, EXPRESS
// OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE. This
// notice including this sentence must appear on any copies of this computer
// software.
//
// ** Disclaimer Notice
//
// This material was prepared as an account of work sponsored by an agency of
// the United States Government. Neither the United States Government nor the
// United States Department of Energy, nor Battelle, nor any of their employees,
// nor any jurisdiction or organization that has cooperated in the development
// of these materials, makes any warranty, express or implied, or assumes any
// legal liability or responsibility for the accuracy, completeness, or
// usefulness or any information, apparatus, product, software, or process
// disclosed, or represents that its use would not infringe privately owned
// rights. Reference herein to any specific commercial product, process, or
// service by trade name, trademark, manufacturer, or otherwise does not
// necessarily constitute or imply its endorsement, recommendation, or favoring
// by the United States Government or any agency thereof, or Battelle Memorial
// Institute. The views and opinions of authors expressed herein do not
// necessarily state or reflect those of the United States Government or any
// agency thereof.
//
//                    PACIFIC NORTHWEST NATIONAL LABORATORY
//                                 operated by
//                                   BATTELLE
//                                   for the
//                      UNITED STATES DEPARTMENT OF ENERGY
//                       under Contract DE-AC05-76RL01830
//===----------------------------------------------------------------------===//

#include "gtest/gtest.h"

#include "shad/config/config.h"
#include "shad/runtime/runtime.h"

namespace shad {

namespace test {

// Report failing asserts on locality 0 to make test fails when
// failing asserts triggers or remote nodes.
class DistributedSystemCheck : public testing::EmptyTestEventListener {
 private:
  void OnTestPartResult(const ::testing::TestPartResult &result) override {
    // When the result is passed() or the locality is 0 we don't need
    // to do anything.
    if (result.passed() || rt::thisLocality() == rt::Locality(0)) {
      return;
    }

    shad::rt::executeAt(rt::Locality(0),
                        [](const bool &fatal) {
                          if (fatal) {
                            FAIL() << "Remote failure";
                          }

                          ADD_FAILURE() << "Remote non-fatal failure";
                        },
                        result.fatally_failed());
  }
};

}  // namespace test

int main(int argc, char *argv[]) {
  ::testing::InitGoogleTest(&argc, argv);

  if (rt::numLocalities() > 1) {
    // Add the handler that deals with remote asserts when
    // we have more than one locality.
    shad::rt::executeOnAll(
        [](const size_t &) {
          testing::TestEventListeners &listeners =
              testing::UnitTest::GetInstance()->listeners();
          listeners.Append(new test::DistributedSystemCheck);
        },
        size_t(0));
  }
  return RUN_ALL_TESTS();
}

} // namespace shad
