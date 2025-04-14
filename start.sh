#!/bin/bash

# Compile the program
echo "Compiling program..."
g++ -std=c++17 -pthread taller-5.cpp -o taller-5

# Check if compilation was successful
if [ $? -eq 0 ]; then
    echo "Compilation successful!"
    echo "Running program..."
    echo "----------------------------------------"
    ./taller-5
else
    echo "Compilation failed!"
    exit 1
fi
