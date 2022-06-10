add_library(wk2_approx STATIC approxMatching.cc readFiles.cc)
target_link_libraries(wk2_approx CONAN_PKG::shad Threads::Threads MPI::MPI_CXX)
