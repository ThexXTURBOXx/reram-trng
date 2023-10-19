#!/bin/bash

# Check if all parameters are provided
if [ "$#" -ne 4 ]; then
    echo "Error: This script requires exactly 4 parameters: RPI_VERSION, RPI_BPLUS, MEM_TYPE, SPI_CLOCK_DIV"
    exit 1
fi

# Assign parameters to variables
RPI_VERSION="$1"
RPI_BPLUS="$2"
MEM_TYPE="$3"
SPI_CLOCK_DIV="$4"

# Run the make command with the parameters
make RPI_VERSION=$RPI_VERSION RPI_BPLUS=$RPI_BPLUS MEM_TYPE=$MEM_TYPE SPI_CLOCK_DIV=$SPI_CLOCK_DIV -j4 -B
