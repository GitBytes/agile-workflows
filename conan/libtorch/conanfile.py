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
