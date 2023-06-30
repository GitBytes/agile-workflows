from conans import ConanFile, tools
import tempfile
import os


class LibTorchConan(ConanFile):
    name = "libtorch"
    version = "1.13.1"
    license = "BSD 3"
    author = "<Put your name here> <And your email here>"
    url = "<Package recipe repository url here, for issues about the package>"
    description = "<Description of Gmt here>"
    topics = ("<Put some tag here>", "<here>", "<and here>")
    settings = "os", "compiler", "build_type", "arch"
    default_options = {}
    default_options = {}

    def source(self):
        if self.settings.os != "Linux":
            raise Exception("Operating System not supported by the recipe.")

        if self.settings.compiler.libcxx == 'libstdc++11':
            url = 'https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-1.13.1%2Bcpu.zip'
        else:
            url = 'https://download.pytorch.org/libtorch/cpu/libtorch-shared-with-deps-1.13.1%2Bcpu.zip'

        tools.download(url, 'libtorch-linux-cpu')
        tools.unzip('libtorch-linux-cpu')

    def package(self):
        self.copy('*', src='libtorch/')

    def package_info(self):
        self.cpp_info.libs = ['torch', 'c10', 'kineto', 'torch_cpu']
        self.cpp_info.includedirs = ['include', 'include/torch/csrc/api/include']
        self.cpp_info.bindirs = ['bin']
        self.cpp_info.libdirs = ['lib']
        self.cpp_info.defines = ['USE_C10D_GLOO', 'USE_RPC', 'USE_DISTRIBUTED', 'USE_TENSORPIPE']
        self.cpp_info.system_libs = ['pthread']
