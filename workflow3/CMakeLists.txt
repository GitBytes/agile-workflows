add_library(workflow3 STATIC readFASTA.cc macroNode.cc processMacroNode.cc utilities.cc)
target_link_libraries(workflow3 CONAN_PKG::shad Threads::Threads MPI::MPI_CXX)
