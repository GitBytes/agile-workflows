add_library(workflow1 STATIC readFiles.cc gnn.cc)
target_link_libraries(workflow1 CONAN_PKG::shad Threads::Threads MPI::MPI_CXX)
