add_library(wk2_exact STATIC readFiles.cc pattern.cc)
target_link_libraries(wk2_exact CONAN_PKG::shad Threads::Threads MPI::MPI_CXX)
