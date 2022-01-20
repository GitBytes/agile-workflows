#!/bin/bash

module purge
module load cmake/3.18.2
module load git/2.17.0
module load gcc/8.2.0
module load openmpi/3.1.3

module load python &> /dev/null
source /share/apps/python/miniconda3.9/etc/profile.d/conda.sh

export PATH=$PATH:$HOME/.local/bin

