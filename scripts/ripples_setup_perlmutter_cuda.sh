#!/bin/bash

echo 'Running setup script for `ripples`'

export agile_WF=$PWD
if [ ! -d $HOME/ripples ]; then 
    git clone https://github.com/pnnl/ripples.git $HOME/ripples
fi
cd $HOME/ripples

cd $HOME/ripples
conan create conan/waf-generator user/stable
conan create conan/trng 4.22@user/stable
conan create conan/nvidia-cub user/stable
conan install --install-folder build . -onvidia_cub=True --build 

env CXX=CC CC=cc ./waf configure --enable-mpi --enable-cuda build_release

echo $PWD
cd $agile_WF
