#!/bin/sh

rm -f pe2
rm -f datagen
rm -f a
rm -f b
rm -f x
rm -f stdans
rm -f ao
clear
echo ----------------------------------------
echo Compiling pe2.cpp
g++ -pthread -std=c++11 -Ofast -o pe2 pe2.cpp
echo Compiling datagen
g++ -std=c++11 -Wno-unused-result -Ofast -o datagen datagen.cpp
echo ----------------------------------------
echo Finished
