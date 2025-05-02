#!/bin/bash

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

echo 'Running the first time setup script'

pip install --user "conan==1.59"

if [ ! -f ~/.conan/settings.yml ]; then
    conan config init
fi
conan profile new default --detect &> /dev/null
conan profile update settings.compiler.libcxx=libstdc++11 default
conan profile update settings.compiler.version=11.2 default
# Cray Machine have compiler wrappers providing MPI
conan profile update env.CC=cc default
conan profile update env.CXX=CC default

if grep riscv $HOME/.conan/settings.yml; then
    echo RISCV support already added. Skipping.
else
    echo RISCV support added.
    sed --in-place=.bkp 's/x86/x86, riscv/' $HOME/.conan/settings.yml
fi


for i in hwloc gmt shad libtorch pytorch-scatter pytorch-sparse; do
    if [ -d $HOME/.conan/data/$i ]; then
        rm -rf $HOME/.conan/data/$i
    fi

    conan create conan/$i user/stable
done
