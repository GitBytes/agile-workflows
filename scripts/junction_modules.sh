#!/bin/bash

module purge
module load cmake/3.27.7
module load gcc/11.2.0
module load openmpi/4.1.2

module load python/miniconda3.9 &> /dev/null
source /share/apps/miniconda/3.9/etc/profile.d/conda.sh

export PATH=$PATH:$HOME/.local/bin

# HWLOCROOT=$(pwd)/hwloc/install
# echo $HWLOCROOT
# 
# if [ ! -d "hwloc" ]; then
#     git clone https://github.com/open-mpi/hwloc.git
#     cd hwloc
#     git checkout c80630218a1c11e1ccc3fa89155647c737385f59 # version 2.10
#     ./autogen.sh
#     ./configure --enable-plugins -prefix=$HWLOCROOT
#     make -j && make install
# fi
# 
# 
# export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$HWLOCROOT/lib/pkgconfig
