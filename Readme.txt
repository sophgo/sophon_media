soc CI

1: mkdir buildit ; cd buildit

2:
    asic
    cmake -DPLATFORM=soc -DSUBTYPE=asic -DCMAKE_INSTALL_PREFIX=../install -DDEBUG=on ..
    cmake --build . --target all -- -j`nproc`
    cmake --build . --target sophon_sample
    cmake --build . --target package

    fpga
    cmake -DPLATFORM=soc -DSUBTYPE=fpga -DCMAKE_INSTALL_PREFIX=../install -DDEBUG=on ..
    cmake --build . --target all -- -j`nproc`
    cmake --build . --target sophon_sample
    cmake --build . --target package





