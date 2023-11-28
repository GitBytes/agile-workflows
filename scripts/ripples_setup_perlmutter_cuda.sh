#!/bin/bash

echo 'Running setup script for `ripples`'

export agile_WF=$PWD
if [ ! -d $HOME/ripples ]; then
    git clone https://github.com/pnnl/ripples.git $HOME/ripples
fi

cd $HOME/ripples
if [ ! -d $HOME/ripples/.venv ]; then
    python -m venv --prompt ripples .venv
    source $HOME/ripples/.venv/bin/activate
    pip install conan
    conan profile detect
    cat << EOF >> $(conan profile path default)
[buildenv]
*:CC=$(which gcc)
*:CXX=$(which g++)
EOF

    deactivate
fi
source $HOME/ripples/.venv/bin/activate

conan create conan/trng
conan install --build missing . -o gpu=nvidia
conan build . -o gpu=nvidia
deactivate

echo $PWD
cd $agile_WF
