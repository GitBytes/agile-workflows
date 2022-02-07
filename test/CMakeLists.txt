add_library(test_main OBJECT gtest_main.cc)
target_link_libraries(test_main CONAN_PKG::rapidcheck CONAN_PKG::gtest CONAN_PKG::shad)

add_executable(test-gtest_remote_failure gtest_test.cc)
target_link_libraries(test-gtest_remote_failure CONAN_PKG::rapidcheck CONAN_PKG::gtest test_main CONAN_PKG::shad Threads::Threads MPI::MPI_CXX)

add_executable(test-pi pi.cc)
target_link_libraries(test-pi CONAN_PKG::rapidcheck CONAN_PKG::gtest test_main CONAN_PKG::shad Threads::Threads MPI::MPI_CXX)
