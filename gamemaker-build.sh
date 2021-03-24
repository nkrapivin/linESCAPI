#!/bin/sh

# clean up the build directory
rm -rf build/*

# set your compiler here, clang++ or g++ I don't care.
CC=g++

# build the api first, and then the demo.
$CC -c -fPIC src/linescapi.cpp -o build/linescapi.o
$CC -c -fPIC src/gamemaker.cpp -o build/gamemaker.o

# link it all together!
$CC -shared -fPIC build/linescapi.o build/gamemaker.o -o build/libgmescapi_linux.so -lv4l2
