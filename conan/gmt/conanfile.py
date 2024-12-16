from conans import ConanFile, CMake, tools


class GmtConan(ConanFile):
    name = "gmt"
    version = "2.0.0"
    license = "BSD 3"
    author = "<Put your name here> <And your email here>"
    url = "<Package recipe repository url here, for issues about the package>"
    description = "<Description of Gmt here>"
    topics = ("<Put some tag here>", "<here>", "<and here>")
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}
    generators = "cmake"
    requires = "hwloc/2.10@user/stable"

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def source(self):
        self.run("git clone https://github.com/pnnl/gmt.git")
        tools.replace_in_file("gmt/CMakeLists.txt", "cmake_minimum_required(VERSION 3.3.0)",
                              '''cmake_minimum_required(VERSION 3.3.0)
project(GMT LANGUAGES CXX)
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()''')

    def build(self):
        cmake = CMake(self)
        if self.settings.arch == "riscv":
            cmake.definitions['GMT_TARGET_ARCH'] = "RISCV"
            cmake.definitions['GMT_ENABLE_UCONTEXT'] = False
        cmake.configure(source_folder="gmt")
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libdirs = ["lib"]
        self.cpp_info.libs = ["gmt", "rt", "pthread"]
