#!/bin/bash

pwd

PATH="/usr/share/gcc-arm-none-eabi-10-2020-q4-major/bin/:$PATH"

cd source/app/GCC_Project

make "$@"