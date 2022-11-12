add_library(workflow4 STATIC readFiles.cc)
target_link_libraries(workflow4 CONAN_PKG::shad Threads::Threads MPI::MPI_CXX)
