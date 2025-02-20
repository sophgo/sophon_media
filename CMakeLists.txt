cmake_minimum_required(VERSION 3.16)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)
if("${PLATFORM}" STREQUAL "soc" OR "${PLATFORM}" STREQUAL "pcie_arm64")
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_MODULE_PATH}/aarch64-gnu.toolchain.cmake)
    if("${GCC_VERSION}" STREQUAL "930")
        set(CMAKE_TOOLCHAIN_FILE ${CMAKE_MODULE_PATH}/aarch64-gnu.toolchain-930.cmake)
        #set(CMAKE_SKIP_RPATH TRUE)
        set(CMAKE_SKIP_INSTALL_RPATH TRUE)
    elseif("${GCC_VERSION}" STREQUAL "1131")
        set(CMAKE_TOOLCHAIN_FILE ${CMAKE_MODULE_PATH}/aarch64-gnu.toolchain-1131.cmake)
        #set(CMAKE_SKIP_RPATH TRUE)
        set(CMAKE_SKIP_INSTALL_RPATH TRUE)
    endif()
elseif("${PLATFORM}" STREQUAL "pcie")
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_MODULE_PATH}/x86_64-gnu.toolchain.cmake)
    #include(cmake/x86_64-gnu.toolchain.cmake)
endif()

set(GIT_VERSION "1.9.0")
STRING(REGEX REPLACE "[\n\t\r]" "" GIT_VERSION ${GIT_VERSION})
STRING(REGEX REPLACE "([0-9][0-9]*)\\..*" "\\1" MAJOR_VERSION ${GIT_VERSION})
STRING(REGEX REPLACE ".*\\.([0-9][0-9]*)\\..*" "\\1" MINOR_VERSION ${GIT_VERSION})
STRING(REGEX REPLACE ".*\\.([0-9][0-9]*).*" "\\1" PATCH_VERSION ${GIT_VERSION})
project("sophon-media" VERSION ${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION})
# subdirectories may depend on former settings

# options
set(CHIP_LIST bm1686)
set(CHIP_NAME bm1686 CACHE STRING "chip: bm1686")
set_property(CACHE CHIP_NAME PROPERTY STRINGS ${CHIP_LIST})

set(PLATFORM_LIST soc pcie pcie_arm64 pcie_loongarch64)
set(PLATFORM soc CACHE STRING "platform: soc pcie pcie_arm64 pcie_loongarch64")
set_property(CACHE PLATFORM PROPERTY STRINGS ${PLATFORM_LIST})

set(SUBTYPE_LIST asic palladium fpga)
set(SUBTYPE asic CACHE STRING "subtype: asic palladium fpga")
set_property(CACHE SUBTYPE PROPERTY STRINGS ${SUBTYPE_LIST})

set(DEBUG_LIST off on)
set(DEBUG off CACHE STRING "debug: off on")
set_property(CACHE DEBUG PROPERTY STRINGS ${DEBUG_LIST})

if("${DEBUG}" STREQUAL "on")
    add_compile_options(-O0 -Wall -g -ggdb)
    set(CMAKE_BUILD_TYPE Debug)
else()
    add_compile_options(-O3 -Wall)
endif()

set(LIBSOPHAV_OUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/libsophav CACHE STRING "bmvid compile output location")
set(LIBSOPHAV_TOP ${CMAKE_CURRENT_SOURCE_DIR}/libsophav CACHE STRING "bmvid compile output location")

message(STATUS "PROJECT..................... " ${PROJECT_NAME})
message(STATUS "VERSION..................... " ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})
message(STATUS "CMAKE_VERSION............... " ${CMAKE_VERSION})
message(STATUS "CMAKE_BINARY_DIR............ " ${CMAKE_BINARY_DIR})
message(STATUS "CMAKE_SOURCE_DIR............ " ${CMAKE_SOURCE_DIR})
message(STATUS "CMAKE_GENERATOR............. " ${CMAKE_GENERATOR})
message(STATUS "CMAKE_TOOLCHAIN_FILE ....... " ${CMAKE_TOOLCHAIN_FILE})
message(STATUS "CMAKE_C_COMPILER............ " ${CMAKE_C_COMPILER})
message(STATUS "CMAKE_CXX_COMPILER.......... " ${CMAKE_CXX_COMPILER})
message(STATUS "CMAKE_BUILD_TYPE............ " ${CMAKE_BUILD_TYPE})
message(STATUS "CMAKE_TOOLCHAIN_FILE........ " ${CMAKE_TOOLCHAIN_FILE})
message(STATUS "CMAKE_SYSTEM_PROCESSOR...... " ${CMAKE_SYSTEM_PROCESSOR})
message(STATUS "CHIP NAME................... " ${CHIP_NAME})
message(STATUS "PLATFORM.................... " ${PLATFORM})
message(STATUS "SUBTYPE..................... " ${SUBTYPE})
message(STATUS "CMAKE_INSTALL_PREFIX........ " ${CMAKE_INSTALL_PREFIX})
message(STATUS "LIBSOPHAV_OUT_PATH.......... " ${LIBSOPHAV_OUT_PATH})
message(STATUS "LIBSOPHAV_TOP............... " ${LIBSOPHAV_TOP})

add_subdirectory(libsophav)

find_package(FFmpeg)
ADD_TARGET_FFMPEG(bm_ffmpeg ${CHIP_NAME} ${PLATFORM} OFF ${LIBSOPHAV_OUT_PATH}/jpeg ${LIBSOPHAV_OUT_PATH}/video ${LIBSOPHAV_OUT_PATH}/bmcv sophon-ffmpeg ${CMAKE_INSTALL_PREFIX}/ffmpeg)
add_dependencies(bm_ffmpeg jpeg_all  vdec_all venc_all bmcv_all yuv_lib)

option(BMCPU_USED "Use CPU on board" OFF)
set_property(CACHE BMCPU_USED PROPERTY TYPE BOOL)
option(ENABLE_OCV_CONTRIB "Compile opencv contrib modules" OFF)
set_property(CACHE ENABLE_OCV_CONTRIB PROPERTY TYPE BOOL)
if (ENABLE_ABI0)
    ADD_TARGET_OPENCV_LIB(bm_opencv_abi0 ${CHIP_NAME} ${SUBTYPE} ${PLATFORM} ON ${ENABLE_OCV_CONTRIB} ${BMCPU_USED} ${LIBSOPHAV_OUT_PATH}
        ${LIBSOPHAV_OUT_PATH} ${LIBSOPHAV_OUT_PATH} ${LIBSOPHAV_OUT_PATH} ${LIBSOPHAV_OUT_PATH} ${LIBSOPHAV_OUT_PATH}
        ${CMAKE_CURRENT_BINARY_DIR}/release/ffmpeg sophon-opencv-abi0 ${CMAKE_CURRENT_BINARY_DIR}/release/opencv-abi0)
    add_dependencies(bm_opencv_abi0 bm_ffmpeg)
else()
    ADD_TARGET_OPENCV_LIB(bm_opencv ${CHIP_NAME} ${SUBTYPE} ${PLATFORM} OFF ${ENABLE_OCV_CONTRIB} ${BMCPU_USED} ${LIBSOPHAV_OUT_PATH} ${LIBSOPHAV_OUT_PATH}
        ${LIBSOPHAV_OUT_PATH} ${LIBSOPHAV_OUT_PATH} ${LIBSOPHAV_OUT_PATH} ${LIBSOPHAV_OUT_PATH} ${CMAKE_CURRENT_BINARY_DIR}/../install/ffmpeg sophon-opencv
        ${CMAKE_CURRENT_BINARY_DIR}/release/opencv)
    add_dependencies(bm_opencv jpeg_all  vdec_all venc_all bmcv_all yuv_lib bm_ffmpeg)
    add_subdirectory(samples)
    add_dependencies(sophon_sample bm_ffmpeg bm_opencv)
endif()

add_subdirectory(bm_gstreamer)

include(cmake/MW_linux_packages.cmake)

