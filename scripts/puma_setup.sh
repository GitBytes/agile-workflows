#!/bin/bash

echo 'Running the first time setup script'

pip install --user conan

conan profile new default --detect &> /dev/null
conan profile update settings.compiler.libcxx=libstdc++11 default

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
