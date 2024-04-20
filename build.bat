@echo off

echo Cleaning up
rd /s /q build

echo Creating build directory
if not exist build mkdir build

echo Compiling galaxy_visualization_raylib


:: Set number of cores to max cores / 2
set CORES=%NUMBER_OF_PROCESSORS%
set /a CORES=%CORES% / 2
echo Using %CORES% cores for compilation

cmake -S ./ -B build -DCMAKE_BUILD_TYPE=Release  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_C_COMPILER=%CC% -DCMAKE_CXX_COMPILER=%CXX%
cmake --build ./build --config Release --target ALL_BUILD  -- /maxcpucount:%CORES%


echo 
echo Running galaxy_visualization_raylib
echo

:: Run the program on Windows
build\Release\galaxy_visualization_raylib.exe
