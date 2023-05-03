#!/bin/bash

echo 'Running setup script for `ripples`'

export agile_WF=$PWD
if [ ! -d $HOME/ripples ]; then
    git clone https://github.com/pnnl/ripples.git $HOME/ripples
fi
cd $HOME/ripples

cd $HOME/ripples
conan profile new --detect deception-gcc-11
conan profile update settings.compiler.libcxx=libstdc++11 deception-gcc-11
conan profile update env.CXX=$(which g++) deception-gcc-11
conan profile update env.CC=$(which gcc) deception-gcc-11
conan create conan/waf-generator user/stable
conan create conan/trng 4.22@user/stable -pr deception-gcc-11
conan install --install-folder build . --build -pr deception-gcc-11

./waf configure --enable-mpi build_release

echo $PWD
cd $agile_WF
