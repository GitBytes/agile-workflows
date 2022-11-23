#!/bin/bash

module load cmake/3.22.0
module load PrgEnv-gnu/8.3.3
module load python3/3.9-anaconda-2021.11
module load gcc/11.2.0

export PATH=$PATH:$HOME/.local/bin
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/global/common/software/nersc/pm-2022q3/sw/python/3.9-anaconda-2021.11/lib
