#!/bin/bash

# Compile the program
echo "Compiling program..."
g++-14 -fopenmp -std=c++17 taller-51.cpp -o taller-51

# Run the program
echo "Running program..."
./taller-51

echo "Program completed."
