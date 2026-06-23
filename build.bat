@echo off
if exist build rmdir /s /q build

cmake -G "MinGW Makefiles" -B build -S .

cmake --build build

cd build/src

os_emulator.exe

cd ../../
