@echo off
cmake --build build

cd build/src/Debug

os_emulator.exe

cd ../../

pause