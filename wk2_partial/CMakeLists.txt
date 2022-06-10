add_library(wk2_partial STATIC readFiles.cc csr.cc pattern.cc)
target_link_libraries(wk2_partial CONAN_PKG::shad Threads::Threads MPI::MPI_CXX)
