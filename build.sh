#!/bin/bash
set -e

clang-format -i include/zkint.h include/zkint/*.h tests/*.c tests/*.cpp

mkdir -p build
cd build
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..

clang-tidy -p . ../tests/*.c ../tests/*.cpp

cmake --build .

cp compile_commands.json ..
