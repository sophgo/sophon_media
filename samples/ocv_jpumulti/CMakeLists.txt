cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0046 NEW)

PROJECT(ocv_jpumulti)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../bm_ffmpeg)

# libmbcv
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../libsophav/3rdparty/libbmcv/include)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../libsophav/3rdparty/libbmcv/lib/${PLATFORM})

#jpeg
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../libsophav/jpeg/inc)
link_directories(${CMAKE_BINARY_DIR}/libsophav/jpeg)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../libsophav/3rdparty/libjpeg-turbo/lib/${PLATFORM})

#video dec
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../libsophav/video/dec/inc)
link_directories(${CMAKE_BINARY_DIR}/libsophav/video/dec)

#video enc
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../libsophav/video/enc/inc)
link_directories(${CMAKE_BINARY_DIR}/libsophav/video/enc)

#bmcv
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../libsophav/bmcv/include)
link_directories(${CMAKE_BINARY_DIR}/libsophav/bmcv)

#ffmpeg
include_directories(${CMAKE_INSTALL_PREFIX}/ffmpeg/usr/local/include)
link_directories(${CMAKE_INSTALL_PREFIX}/ffmpeg/usr/local/lib)

# opencv
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../buildit/release/opencv/lib)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../buildit/release/opencv/include/opencv4)

# libyuv
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../buildit/libsophav/libyuv/lib)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../buildit/libsophav/libyuv/include)


set(SYSTEM_LIB pthread stdc++)

SET(SRCS
    jpumulti.cpp
)

add_executable(test_ocv_jpumulti EXCLUDE_FROM_ALL ${SRCS})
target_link_libraries(test_ocv_jpumulti opencv_core opencv_imgcodecs ${SYSTEM_LIB})

