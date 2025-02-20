set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(GCC_COMPILER_VERSION "" CACHE STRING "GCC Compiler version")
set(GNU_MACHINE "aarch64-none-linux-gnu" CACHE STRING "GNU compiler triple")

if(NOT DEFINED CMAKE_C_COMPILER)
    find_program(CMAKE_C_COMPILER NAMES ${GNU_MACHINE}-gcc)
endif()
message(STATUS "CMAKE_C_COMPILER=${CMAKE_C_COMPILER} is defined")

if(NOT DEFINED CMAKE_CXX_COMPILER)
    find_program(CMAKE_CXX_COMPILER NAMES ${GNU_MACHINE}-g++)
endif()
message(STATUS "CMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} is defined")

if(NOT DEFINED CMAKE_LINKER)
    find_program(CMAKE_LINKER NAMES ${GNU_MACHINE}-ld)
endif()
message(STATUS "CMAKE_LINKER=${CMAKE_LINKER} is defined")

if(NOT DEFINED CMAKE_AR)
    find_program(CMAKE_AR NAMES ${GNU_MACHINE}-ar)
endif()
message(STATUS "CMAKE_AR=${CMAKE_AR} is defined")