#!/bin/sh

rm -rf build/*

gcc -c src/linescapi.cpp -o build/linescapi.o
gcc -c src/main.cpp -o build/main.o
gcc build/linescapi.o build/main.o -o build/linescapi.demo -lstdc++ -lv4l2