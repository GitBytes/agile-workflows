add_executable(pi pi.cc)
target_link_libraries(pi CONAN_PKG::shad Threads::Threads MPI::MPI_CXX)

add_executable(do-workflow1 workflow1.cc)
target_link_libraries(do-workflow1 workflow1 CONAN_PKG::shad Threads::Threads MPI::MPI_CXX)

add_executable(do-workflow2 workflow2.cc)
target_link_libraries(do-workflow2 workflow2 CONAN_PKG::shad Threads::Threads MPI::MPI_CXX)
