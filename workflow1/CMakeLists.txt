add_library(workflow1 STATIC readFiles.cc csr.cc)
target_link_libraries(workflow1 CONAN_PKG::shad Threads::Threads MPI::MPI_CXX)

add_library(gnn_utils STATIC cora.cc wmd.cc)
target_link_libraries(gnn_utils CONAN_PKG::libtorch CONAN_PKG::shad)
