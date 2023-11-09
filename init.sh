#!/bin/bash

pushd circle
echo -e "PREFIX64 = aarch64-linux-gnu-\nAARCH = 64\nRASPPI = 3\n" > Config.mk
./makeall --nosample -j4
pushd boot
make
popd
pushd addon/fatfs
make -j4
popd
pushd addon/SDCard
make -j4
popd
popd
