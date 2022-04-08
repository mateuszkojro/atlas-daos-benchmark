#!/bin/bash
COMMON_FLAGS="-DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DBENCHMARK_ENABLE_TESTING=false -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang -B build ."
if [ -f /.dockerenv ]; then
    cmake -DDAOS_DIR=/daos/install $COMMON_FLAGS && cmake --build build/ -j3
else
    cmake $COMMON_FLAGS && cmake --build build/ -j3
fi


