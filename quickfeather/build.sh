#!/bin/bash

pwd

PATH="/usr/share/gcc-arm-none-eabi-9-2020-q2-update/bin/:$PATH"

cd source/app/GCC_Project

make "$@"