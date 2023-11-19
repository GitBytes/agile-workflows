#!/bin/bash

echo 'Running setup script for `ripples`'

export agile_WF=$PWD
if [ ! -d $HOME/ripples ]; then 
    git clone https://github.com/pnnl/ripples.git $HOME/ripples
fi
cd $HOME/ripples
git checkout v2.2

conan create conan/waf-generator user/stable
conan create conan/trng 4.22@user/stable
conan install --install-folder build . --build 

env CXX=CC CC=cc ./waf configure --enable-mpi build_release

echo $PWD
cd $agile_WF
