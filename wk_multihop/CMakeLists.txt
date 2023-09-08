add_library(wk_multihop STATIC readFiles.cc buildPersonTable.cc buildUniversityTable.cc pattern.cc)
target_link_libraries(wk_multihop CONAN_PKG::shad Threads::Threads MPI::MPI_CXX)



