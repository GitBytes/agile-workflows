#!/bin/bash

echo 'Running setup script for `ripples` - MPI version'

export agile_WF=$PWD
git clone https://github.com/pnnl/ripples.git $HOME/ripples
cd $HOME/ripples
git checkout v2.2

pip install --user pipenv
export PATH=$HOME/.local/bin:$PATH
pipenv --three
pipenv install
pipenv shell

module purge
module load cmake/3.15.3
module load gcc/10.2
module load cuda/11.4
module load openmpi/4.1.4

cd $HOME/ripples
conan create conan/waf-generator user/stable
conan profile new --detect marianas-gcc-10-openmpi-4.1.4-cuda-11.4
conan profile update settings.compiler.libcxx=libstdc++11 marianas-gcc-10-openmpi-4.1.4-cuda-11.4
conan profile update env.CXX=$(which mpicxx) marianas-gcc-10-openmpi-4.1.4-cuda-11.4
conan profile update env.CC=$(which mpicc) marianas-gcc-10-openmpi-4.1.4-cuda-11.4
conan create conan/trng 4.22@user/stable -pr marianas-gcc-10-openmpi-4.1.4-cuda-11.4
conan install --install-folder build . --build -pr marianas-gcc-10-openmpi-4.1.4-cuda-11.4

./waf configure --enable-mpi build_release

echo $PWD
cd $agile_WF
