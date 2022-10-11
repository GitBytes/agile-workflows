#!/bin/bash

PKG_LIST=""
for p in build-essential libmpich-dev git wget cmake python3-pip; do
    if ! dpkg -l $p > /dev/null ; then
        PKG_LIST="$p $PKG_LIST"
    fi
done

echo $PKG_LIST

if [ -x $PKG_LIST ]; then
    sudo apt update -y
    sudo apt ugrade -y
    sudo apt install -y $PKG_LIST
fi

source scripts/docker_setup.sh
