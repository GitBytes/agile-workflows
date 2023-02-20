#!/bin/bash

echo 'Running setup script for `ripples`'

export agile_WF=$PWD
git clone https://github.com/pnnl/ripples.git $HOME/ripples
cd $HOME/ripples

pip install --user pipenv
export PATH=$HOME/.local/bin:$PATH
pipenv --three
pipenv install
pipenv shell

cd $HOME/ripples
conan create conan/waf-generator user/stable
conan profile new --detect puma-gcc-10
conan profile update settings.compiler.libcxx=libstdc++11 puma-gcc-10
conan profile update env.CXX=$(which g++) puma-gcc-10
conan profile update env.CC=$(which gcc) puma-gcc-10
conan create conan/trng 4.22@user/stable -pr puma-gcc-10
conan install --install-folder build . --build -pr puma-gcc-10

./waf configure build_release

echo $PWD
cd $agile_WF
