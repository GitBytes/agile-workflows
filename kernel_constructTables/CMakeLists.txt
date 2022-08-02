add_library(kernel_constructTables STATIC main.cc)
target_link_libraries(kernel_constructTables CONAN_PKG::shad Threads::Threads MPI::MPI_CXX)
