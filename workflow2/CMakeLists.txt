add_library(workflow2 STATIC approxMatching.cc readFiles.cc csr.cc pattern.cc)
target_link_libraries(workflow2 CONAN_PKG::shad Threads::Threads MPI::MPI_CXX)
