#!/bin/bash

export DEBUG=${DEBUG:-off}
export CHIP=${CHIP:-bm1688}
export SUBTYPE=${SUBTYPE:-asic}
export PLATFORM=${PLATFORM:-soc}
export GCC_V=${GCC_V:-1131}

# example (run in sophon_media root dir)
if [ "$DEBUG" = "on" ]; then
    CMAKE_BUILD_TYPE="Debug"
else
    CMAKE_BUILD_TYPE="Release"
fi

if [ $# -ge 1 ]; then
    GCC_V=$1
fi

if [ $# -ge 2 ]; then
    PLATFORM=$2
fi

echo "SOPHON_MEDIA BULLD PARAMETER"
echo "CHIP = $CHIP, SUBTYPE = $SUBTYPE, PLATFORM = $PLATFORM, DEBUG = $DEBUG, GCC_V = $GCC_V"
echo

rm -rf buildit install
mkdir buildit
pushd buildit
cmake -DPLATFORM=${PLATFORM} -DCHIP_NAME=${CHIP} -DGCC_VERSION=$GCC_V -DSUBTYPE=asic -DCMAKE_INSTALL_PREFIX=../install -DDEBUG=$DEBUG -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE ..
cmake --build . --target all -- -j`nproc`
cmake --build . --target sophon_sample
cmake --build . --target package
popd
