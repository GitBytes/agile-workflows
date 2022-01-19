add_executable(hello-shad hello.cc)
target_link_libraries(hello-shad CONAN_PKG::shad Threads::Threads MPI::MPI_CXX)
