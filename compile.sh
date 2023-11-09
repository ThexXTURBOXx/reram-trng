#!/bin/bash

# Check if all parameters are provided

if [ "$#" -eq 2 ]; then
  FINAL_NAME="kernel8"
elif [ "$#" -eq 3 ]; then
  FINAL_NAME="$3"
else
  echo "Error: This script requires 2 parameters: MEM_TYPE, SPI_FREQ"
  exit 1
fi

MEM_TYPE="$1"
SPI_FREQ="$2"

# Run the make command with the parameters
make MEM_TYPE=$MEM_TYPE SPI_FREQ=$SPI_FREQ -j4 -B boot

if [ $FINAL_NAME != "kernel8" ]; then
  mv boot/kernel8.img boot/$FINAL_NAME.img
fi
