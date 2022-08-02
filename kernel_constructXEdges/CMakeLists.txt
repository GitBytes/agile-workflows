add_library(kernel_constructXEdges STATIC main.cc)
target_link_libraries(kernel_constructXEdges CONAN_PKG::shad Threads::Threads MPI::MPI_CXX)
