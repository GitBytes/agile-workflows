from conans import ConanFile, CMake, tools


class PyTorchScatterConan(ConanFile):
    name = "pytorch_scatter"
    version = "2.0.9"
    license = "MIT"
    author = "<Put your name here> <And your email here>"
    url = "<Package recipe repository url here, for issues about the package>"
    description = "<Description of Gmt here>"
    topics = ("<Put some tag here>", "<here>", "<and here>")
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}
    generators = "cmake"

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def requirements(self):
        self.requires('libtorch/1.11.0@user/stable')

    def source(self):
        self.run("git clone https://github.com/rusty1s/pytorch_scatter.git")
        tools.replace_in_file("pytorch_scatter/CMakeLists.txt", "set(TORCHSCATTER_VERSION 2.0.9)",
                              '''set(TORCHSCATTER_VERSION 2.0.9)
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()''')

    def build(self):
        cmake = CMake(self)
        cmake.configure(source_folder="pytorch_scatter")
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libdirs = ["lib"]
        self.cpp_info.libs = ["torchscatter"]
