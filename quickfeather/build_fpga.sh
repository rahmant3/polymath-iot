#!/bin/bash

pwd

PROJ_DIR="$PWD/source/fpga"

export INSTALL_DIR="$PWD/source/qorc-sdk/quicklogic-fpga-toolchain/install"
export PATH="$INSTALL_DIR/quicklogic-arch-defs/bin:$INSTALL_DIR/quicklogic-arch-defs/bin/python:$PATH"
source "$INSTALL_DIR/conda/etc/profile.d/conda.sh"
conda activate

cd $PROJ_DIR
make "$@"


