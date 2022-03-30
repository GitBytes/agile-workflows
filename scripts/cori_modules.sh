#!/bin/bash

module purge
module load cmake/3.21.3
module load git/2.21.0
module load gcc/8.2.0
module load openmpi/3.1.3
module load python3/3.9-anaconda-2021.11

export PATH=$PATH:$HOME/.local/bin
