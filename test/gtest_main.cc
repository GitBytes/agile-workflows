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
