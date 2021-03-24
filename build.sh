#!/bin/sh

# clean up the build directory
rm -rf build/*

# set your compiler here, clang++ or g++ I don't care.
CC=g++

# build the api first, and then the demo.
$CC -c src/linescapi.cpp -o build/linescapi.o
$CC -c src/main.cpp -o build/main.o

# link it all together!
$CC build/linescapi.o build/main.o -o build/linescapi.demo -lv4l2