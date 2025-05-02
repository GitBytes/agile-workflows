# The AGILE Workflows
#
# Copyright (c) 2025 Battelle Memorial Institute
#
# Battelle Memorial Institute (hereinafter Battelle) hereby grants permission to
# any person or entity lawfully obtaining a copy of this software and associated
# documentation files (hereinafter “the Software”) to redistribute and use the
# Software in source and binary forms, with or without modification.  Such person
# or entity may use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and may permit others to do so, subject to the
# following conditions:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimers.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# 3. Other than as used herein, neither the name Battelle Memorial Institute or
#    Battelle may be used in any form whatsoever without the express written
#    consent of Battelle.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL BATTELLE OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
