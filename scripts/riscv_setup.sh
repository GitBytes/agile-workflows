#!/bin/bash

echo 'Running the first time setup script'

pip install --user conan

conan profile new default --detect &> /dev/null
conan profile update settings.compiler.libcxx=libstdc++11 default
conan profile update settings.arch=riscv default
conan profile update settings.arch_build=riscv default
conan profile update env.CC=$(which mpicc) default
conan profile update env.CXX=$(which mpicxx) default

if [ -d $HOME/.conan/data ]; then
    rm -rf $HOME/.conan/data
fi

if grep riscv $HOME/.conan/settings.yml; then
    echo RISCV support already added. Skipping.
else
    echo RISCV support added.
    sed -i .bkp s/x86/x86, riscv/ $HOME/.conan/settings.yml
fi

conan create conan/gmt user/stable
conan create conan/shad user/stable
