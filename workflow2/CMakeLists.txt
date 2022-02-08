add_library(workflow2 STATIC readFiles.cc)
target_link_libraries(workflow2 CONAN_PKG::shad Threads::Threads MPI::MPI_CXX)
