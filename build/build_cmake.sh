#!/bin/bash

# example (run in sophon_media root dir)
DEBUG="off"
if [ "$DEBUG" = "on" ]; then
    CMAKE_BUILD_TYPE="Debug"
else
    CMAKE_BUILD_TYPE="Release"
fi

GCC_V="630"
if [ $# -ge 1 ]; then
    GCC_V=$1
fi


rm -rf buildit install
mkdir buildit
pushd buildit
cmake -DPLATFORM=soc -DGCC_VERSION=$GCC_V -DSUBTYPE=asic -DCMAKE_INSTALL_PREFIX=../install -DDEBUG=$DEBUG -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE ..
cmake --build . --target all -- -j`nproc`
cmake --build . --target sophon_sample
cmake --build . --target package
popd
