#!/bin/bash

module load cmake/3.22.2
module swap PrgEnv-intel/6.0.10 PrgEnv-gnu/6.0.10
module load python3/3.9-anaconda-2021.11

export PATH=$PATH:$HOME/.local/bin
