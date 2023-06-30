from conans import ConanFile, CMake, tools


class PyTorchSparseConan(ConanFile):
    name = "pytorch_sparse"
    version = "0.6.11"
    license = "MIT"
    author = "<Put your name here> <And your email here>"
    url = "<Package recipe repository url here, for issues about the package>"
    description = "<Description of Gmt here>"
    topics = ("<Put some tag here>", "<here>", "<and here>")
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}
    generators = "cmake"
    scm = {
        "type": "git",
        "subfolder": "pytorch_sparse",
        "url": "https://github.com/rusty1s/pytorch_sparse.git",
        "revision": "0.6.11"
    }

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def requirements(self):
        self.requires('libtorch/1.13.1@user/stable')

    def source(self):
        tools.replace_in_file("pytorch_sparse/CMakeLists.txt", "set(TORCHSPARSE_VERSION 0.6.11)",
                              '''set(TORCHSPARSE_VERSION 0.6.11)
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()''')

    def build(self):
        cmake = CMake(self)
        cmake.configure(source_folder="pytorch_sparse")
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libdirs = ["lib"]
        self.cpp_info.libs = ["torchsparse"]
