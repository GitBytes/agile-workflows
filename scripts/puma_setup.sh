#!/bin/bash

echo 'Running the first time setup script'

pip install --user conan

conan profile new default --detect &> /dev/null
conan profile update settings.compiler.libcxx=libstdc++11 default

conan create conan/gmt user/stable
conan create conan/shad user/stable
