@echo off
set BUILD_DIR=build
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cmake -S . -B "%BUILD_DIR%" -DCMAKE_BUILD_TYPE=Release %*
cmake --build "%BUILD_DIR%" --config Release -j %NUMBER_OF_PROCESSORS%
