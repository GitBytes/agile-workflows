add_executable(pi pi.cc)
target_link_libraries(pi CONAN_PKG::shad Threads::Threads MPI::MPI_CXX)

add_executable(do-workflow1 workflow1.cc)
target_link_libraries(do-workflow1 workflow1 CONAN_PKG::shad Threads::Threads MPI::MPI_CXX)

add_executable(do-workflow2 workflow2.cc)
target_link_libraries(do-workflow2 workflow2 CONAN_PKG::shad Threads::Threads MPI::MPI_CXX)

add_executable(do-workflow2B workflow2B.cc)
target_link_libraries(do-workflow2B workflow2 CONAN_PKG::shad Threads::Threads MPI::MPI_CXX)

add_executable(vc-simple vc-simple.cc)
target_link_libraries(vc-simple gnn_utils CONAN_PKG::pytorch_sparse CONAN_PKG::pytorch_scatter CONAN_PKG::libtorch)

add_executable(vc-shad vc-shad.cc)
target_link_libraries(vc-shad gnn_utils CONAN_PKG::pytorch_sparse CONAN_PKG::pytorch_scatter CONAN_PKG::libtorch CONAN_PKG::shad Threads::Threads MPI::MPI_CXX)
