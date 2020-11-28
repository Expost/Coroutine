@echo off

if not exist xco32 mkdir xco32
cd xco32
cmake -G "Visual Studio 15 2017"  ..
cd ..

if not exist xco64 mkdir xco64
cd xco64
cmake -G "Visual Studio 15 2017 Win64"  ..
cd ..