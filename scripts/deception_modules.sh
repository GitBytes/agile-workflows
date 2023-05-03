#!/bin/bash

module purge
module load cmake/3.26.0
module load git
module load gcc/11.2.0
module load openmpi/4.1.4

module load python/miniconda3.9 &> /dev/null
source /share/apps/python/miniconda3.9/etc/profile.d/conda.sh

export PATH=$PATH:$HOME/.local/bin

