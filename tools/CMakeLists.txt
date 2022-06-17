add_executable(pi pi.cc)
target_link_libraries(pi CONAN_PKG::shad Threads::Threads MPI::MPI_CXX)

add_executable(do-workflow1 workflow1.cc)
target_link_libraries(do-workflow1 gnn_utils workflow1 CONAN_PKG::shad CONAN_PKG::pytorch_sparse CONAN_PKG::pytorch_scatter CONAN_PKG::libtorch Threads::Threads MPI::MPI_CXX)

add_executable(do-wk2_exact wk2_exact.cc)
target_link_libraries(do-wk2_exact wk2_exact CONAN_PKG::shad Threads::Threads MPI::MPI_CXX)

add_executable(do-wk2_approx wk2_approx.cc)
target_link_libraries(do-wk2_approx wk2_approx CONAN_PKG::shad Threads::Threads MPI::MPI_CXX)

add_executable(do-wk2_partial wk2_partial.cc)
target_link_libraries(do-wk2_partial wk2_partial CONAN_PKG::shad Threads::Threads MPI::MPI_CXX)

add_executable(vc-simple vc-simple.cc)
target_link_libraries(vc-simple gnn_utils CONAN_PKG::pytorch_sparse CONAN_PKG::pytorch_scatter CONAN_PKG::libtorch)

add_executable(vc-shad vc-shad.cc)
target_link_libraries(vc-shad gnn_utils CONAN_PKG::pytorch_sparse CONAN_PKG::pytorch_scatter CONAN_PKG::libtorch CONAN_PKG::shad Threads::Threads MPI::MPI_CXX)
