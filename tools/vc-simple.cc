#include <iostream>

#include <torch/torch.h>
#include <torch/script.h>

int main(int argc, const char* argv[]) {
  if (argc != 2) {
    std::cerr << "usage: torch-test <path-to-exported-script-module>\n";
    return EXIT_FAILURE;
  }

  torch::jit::script::Module module;
  try {
    module = torch::jit::load(argv[1]);
  } catch (const c10::Error & e) {
    std::cerr << "Error loading the module: " << e.msg() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
