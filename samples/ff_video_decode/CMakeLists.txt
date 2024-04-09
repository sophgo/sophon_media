cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0046 NEW)

PROJECT(ff_video_decode)
#libbmlib
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../libsophav/3rdparty/libbmcv/include)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../libsophav/3rdparty/libbmcv/lib/${PLATFORM})
#jpeg
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../libsophav/jpeg/3rdparty/include)
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


set(FFMPEG_LIB avfilter avformat avcodec swscale avutil swresample)
set(BMLIB    bmlib)
set(VPU_LIB  bmvd bmvenc)
set(BMCV_LIB bmcv)
set(JPEG_LIB bmjpeg turbojpeg)
set(SYSTEM_LIB pthread stdc++)

SET(SRCS
  ff_video_decode.cpp
  test_ff_video_decode.cpp
)


add_executable(test_bm_restart EXCLUDE_FROM_ALL ${SRCS})
target_link_libraries(test_bm_restart ${FFMPEG_LIB} ${BMLIB} ${VPU_LIB}	${BMCV_LIB} ${JPEG_LIB} ${SYSTEM_LIB})
