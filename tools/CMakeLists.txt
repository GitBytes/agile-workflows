add_executable(pi pi.cc)
target_link_libraries(pi CONAN_PKG::shad Threads::Threads MPI::MPI_CXX)
