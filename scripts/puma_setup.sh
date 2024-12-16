#!/bin/bash

echo 'Running the first time setup script'

pip install --user "conan==1.59"
pip install --user torch torchvision torchaudio --extra-index-url https://download.pytorch.org/whl/cpu
pip install --user torch-scatter torch-sparse torch-cluster torch-spline-conv torch-geometric -f https://data.pyg.org/whl/torch-1.11.0+cpu.html
if [ ! -f ~/.conan/settings.yml ]; then
    conan config init
fi
conan profile new default --detect &> /dev/null
conan profile update settings.compiler.libcxx=libstdc++ default

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
