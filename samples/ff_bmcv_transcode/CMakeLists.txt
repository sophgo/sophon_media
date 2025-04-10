cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0046 NEW)

PROJECT(ff_video_decode)
#libbmlib
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../libsophav/3rdparty/libbmcv/include)
if("${PLATFORM}" STREQUAL "pcie" OR "${PLATFORM}" STREQUAL "pcie_arm64")
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../libsophav/3rdparty/libbmcv/lib/${PLATFORM})
elseif("${PLATFORM}" STREQUAL "soc")
    if("${GCC_VERSION}" STREQUAL "930")
        link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../libsophav/3rdparty/libbmcv/lib930/${PLATFORM})
    elseif("${GCC_VERSION}" STREQUAL "1131")
        link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../libsophav/3rdparty/libbmcv/lib1131/${PLATFORM})
    else()
        link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../libsophav/3rdparty/libbmcv/lib/${PLATFORM})
    endif()
endif()
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
  ff_avframe_convert.cpp
  ff_video_decode.cpp
  ff_video_encode.cpp
  test_ff_video_transcode.cpp
)


add_executable(test_ff_bmcv_transcode EXCLUDE_FROM_ALL ${SRCS})
target_compile_options(test_ff_bmcv_transcode PRIVATE -Wno-deprecated-declarations)
target_link_libraries(test_ff_bmcv_transcode ${FFMPEG_LIB} ${BMLIB} ${VPU_LIB} ${BMCV_LIB} ${JPEG_LIB} ${SYSTEM_LIB})
add_dependencies(test_ff_bmcv_transcode bm_ffmpeg)

