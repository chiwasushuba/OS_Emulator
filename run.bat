@echo off
cmake --build build

cd build/src

os_emulator.exe

cd ../../
