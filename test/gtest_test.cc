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
#include "gtest/gtest-spi.h"

#include "shad/runtime/runtime.h"

namespace testing {
namespace internal {

// Check for at least a failure of the expected type matching substr.
AssertionResult HasRemoteFailure(const char * /*result_expr */,
                                 const char * /* type_expr */,
                                 const char * /* substr_expr*/,
                                 const TestPartResultArray &results,
                                 TestPartResult::Type type,
                                 const std::string &substr) {
  const std::string expected(type == TestPartResult::kFatalFailure
                                 ? "a fatal failure"
                                 : "a non-fatal failure");
  Message msg;
  for (int i = 0; i < results.size(); ++i) {
    const TestPartResult &r = results.GetTestPartResult(i);
    if (r.type() != type) {
      continue;
    }

    if (strstr(r.message(), substr.c_str()) != nullptr) {
      return AssertionSuccess();
    }
  }

  return AssertionFailure()
         << "Expected: " << expected << " containing \"" << substr << "\"\n"
         << "  None found\n";
}

// Run the HasRemoteFailure check.
class RemoteFailureChecker {
 public:
  RemoteFailureChecker(const TestPartResultArray *results,
                       TestPartResult::Type type, const std::string &substr)
      : results_(results), type_(type), substr_(substr) {}

  ~RemoteFailureChecker() {
    EXPECT_PRED_FORMAT3(HasRemoteFailure, *results_, type_, substr_);
  }

  RemoteFailureChecker(RemoteFailureChecker &&) = delete;
  RemoteFailureChecker operator=(RemoteFailureChecker &&) = delete;

 private:
  const TestPartResultArray *const results_;
  const TestPartResult::Type type_;
  const std::string substr_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(RemoteFailureChecker);
};

}  // namespace internal
}  // namespace testing

// Implement an equivalent version of EXPECT_FATAL_FAILURE_ON_ALL_THREADS
// to check that remote failures have been propagate in the rank 0 node.
#define EXPECT_REMOTE_FATAL_FAILURE_ON_ALL_THREADS(statement, substr)         \
  do {                                                                        \
    class GTestExpectFatalFailureHelper {                                     \
     public:                                                                  \
      static void Execute() { statement; }                                    \
    };                                                                        \
    ::testing::TestPartResultArray gtest_failures;                            \
    ::testing::internal::RemoteFailureChecker gtest_checker(                  \
        &gtest_failures, ::testing::TestPartResult::kFatalFailure, (substr)); \
    {                                                                         \
      ::testing::ScopedFakeTestPartResultReporter gtest_reporter(             \
          ::testing::ScopedFakeTestPartResultReporter::INTERCEPT_ALL_THREADS, \
          &gtest_failures);                                                   \
      GTestExpectFatalFailureHelper::Execute();                               \
    }                                                                         \
  } while (::testing::internal::AlwaysFalse())

static void test1(const size_t & /* unused */) {
  if (shad::rt::thisLocality() == shad::rt::Locality(0)) {
    SUCCEED();
  } else {
    FAIL() << "Failing on remote locality : " << shad::rt::thisLocality();
  }
}

auto test1Lambda = []() {
  for (auto &locality : shad::rt::allLocalities()) {
    shad::rt::executeAt(locality, test1, size_t(0));
  }
};

TEST(TestingRemoteFailure, remoteFailsLocalSuccedes) {
  if (shad::rt::numLocalities() == 1) {
    SUCCEED();
    return;
  }

  EXPECT_REMOTE_FATAL_FAILURE_ON_ALL_THREADS(test1Lambda(), "Remote failure");
}

static void test2(const size_t & /* unused */) {
  if (shad::rt::thisLocality() != shad::rt::Locality(0)) {
    SUCCEED();
  } else {
    FAIL() << "Failing on locality 0";
  }
}

auto test2Lambda = []() {
  for (auto &locality : shad::rt::allLocalities()) {
    shad::rt::executeAt(locality, test2, size_t(0));
  }
};

TEST(TestingRemoteFailure, remoteSuccedesLocalFails) {
  if (shad::rt::numLocalities() == 1) {
    SUCCEED();
    return;
  }

  EXPECT_FATAL_FAILURE_ON_ALL_THREADS(test2Lambda(), "Failing on locality 0");
}

static void test3(const size_t & /* unused */) {
  if (shad::rt::thisLocality() != shad::rt::Locality(0)) {
    SUCCEED();
  } else {
    ADD_FAILURE() << "Failing on locality 0";
  }
}

auto test3Lambda = []() {
  for (auto &locality : shad::rt::allLocalities()) {
    shad::rt::executeAt(locality, test3, size_t(0));
  }
};

TEST(TestingRemoteNonFatalFailure, remoteSuccedesLocalFails) {
  if (shad::rt::numLocalities() == 1) {
    SUCCEED();
    return;
  }

  EXPECT_NONFATAL_FAILURE_ON_ALL_THREADS(test3Lambda(),
                                         "Failing on locality 0");
}

// Implement an equivalent version of EXPECT_NONFATAL_FAILURE_ON_ALL_THREADS
// to check that remote failures have been propagate in the rank 0 node.
#define EXPECT_REMOTE_NONFATAL_FAILURE_ON_ALL_THREADS(statement, substr)      \
  do {                                                                        \
    class GTestExpectFatalFailureHelper {                                     \
     public:                                                                  \
      static void Execute() { statement; }                                    \
    };                                                                        \
    ::testing::TestPartResultArray gtest_failures;                            \
    ::testing::internal::RemoteFailureChecker gtest_checker(                  \
        &gtest_failures, ::testing::TestPartResult::kNonFatalFailure,         \
        (substr));                                                            \
    {                                                                         \
      ::testing::ScopedFakeTestPartResultReporter gtest_reporter(             \
          ::testing::ScopedFakeTestPartResultReporter::INTERCEPT_ALL_THREADS, \
          &gtest_failures);                                                   \
      GTestExpectFatalFailureHelper::Execute();                               \
    }                                                                         \
  } while (::testing::internal::AlwaysFalse())

static void test4(const size_t & /* unused */) {
  if (shad::rt::numLocalities() == 1) {
    SUCCEED();
    return;
  }

  if (shad::rt::thisLocality() == shad::rt::Locality(0)) {
    SUCCEED();
  } else {
    ADD_FAILURE() << "Failing on locality : " << shad::rt::thisLocality();
  }
}

auto test4Lambda = []() {
  for (auto &locality : shad::rt::allLocalities()) {
    shad::rt::executeAt(locality, test4, size_t(0));
  }
};

TEST(TestingRemoteNonFatalFailure, remoteFailsLocalSucceedes) {
  if (shad::rt::numLocalities() == 1) {
    SUCCEED();
    return;
  }

  EXPECT_REMOTE_NONFATAL_FAILURE_ON_ALL_THREADS(test4Lambda(),
                                                "Remote non-fatal failure");
}
