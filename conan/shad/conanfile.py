from conans import ConanFile, CMake, tools


class ShadConan(ConanFile):
    name = "shad"
    version = "1.0.0"
    license = "APACHE 2.0"
    author = "<Put your name here> <And your email here>"
    url = "<Package recipe repository url here, for issues about the package>"
    description = "<Description of Shad here>"
    topics = ("<Put some tag here>", "<here>", "<and here>")
    settings = "os", "compiler", "build_type", "arch"
    options = {"runtime": ["GMT", "TBB"]}
    default_options = {"runtime": "GMT"}
    generators = "cmake"

    def requirements(self):
        if self.options.runtime == 'GMT':
            self.requires('gmt/2.0.0@user/stable')

    def source(self):
        self.run("git clone https://github.com/pnnl/SHAD.git")
        # This small hack might be useful to guarantee proper /MT /MD linkage
        # in MSVC if the packaged project doesn't have variables to set it
        # properly
        tools.replace_in_file("SHAD/CMakeLists.txt", "cmake_minimum_required(VERSION 3.1)",
                              '''cmake_minimum_required(VERSION 3.1)
project(SHAD LANGUAGES CXX)
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

set(GMT_ROOT ${CONAN_GMT_ROOT})
''')
        tools.replace_in_file("SHAD/CMakeLists.txt", 
'''if (SHAD_RUNTIME_SYSTEM STREQUAL "GMT")
  find_package(MPI REQUIRED)
  include_directories(${MPI_INCLUDE_PATH})
endif()''',
'''if (SHAD_RUNTIME_SYSTEM STREQUAL "GMT")
  find_package(MPI REQUIRED)
  include_directories(${MPI_INCLUDE_PATH})
  ##### HWLOC #####
  include(FindPkgConfig REQUIRED)
  pkg_check_modules(hwloc REQUIRED IMPORTED_TARGET hwloc)
  link_libraries(PkgConfig::hwloc)
  include_directories(${MPI_INCLUDE_PATH} PkgConfig::hwloc)
  #################
endif()
''')

    def build(self):
        cmake = CMake(self)
        cmake.definitions["SHAD_RUNTIME_SYSTEM"] = "GMT"
        cmake.definitions["SHAD_ENABLE_UNIT_TEST"] = "off"
        cmake.configure(source_folder="SHAD")
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libdirs = ["lib"] + self.deps_cpp_info['gmt'].lib_paths
        self.cpp_info.libs = ["gmt_runtime"] + self.deps_cpp_info['gmt'].libs + ["runtime"]
        self.cpp_info.defines = ["HAVE_GMT"]
