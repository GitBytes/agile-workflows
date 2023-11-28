#!/bin/bash

module purge
module load cmake/3.27.7
module load git/2.17.0
module load gcc/11.2.0
module load openmpi/4.1.5

module load python &> /dev/null
source /share/apps/python/miniconda3.9/etc/profile.d/conda.sh

export PATH=$PATH:$HOME/.local/bin

