cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0046 NEW)

PROJECT(ocv_video_xcode)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../bm_ffmpeg)

# libbmcv
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../libsophav/3rdparty/libbmcv/include)
if("${PLATFORM}" STREQUAL "pcie"  OR "${PLATFORM}" STREQUAL "pcie_arm64")
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
link_directories(${CMAKE_BINARY_DIR}/release/opencv/lib)
include_directories(${CMAKE_BINARY_DIR}/release/opencv/include/opencv4)

# libyuv
link_directories(${CMAKE_BINARY_DIR}/libsophav/libyuv/lib)
include_directories(${CMAKE_BINARY_DIR}/libsophav/libyuv/include)


set(SYSTEM_LIB pthread stdc++)

SET(SRCS
    ocv_video_xcode.cpp
)

add_executable(test_ocv_video_xcode EXCLUDE_FROM_ALL ${SRCS})
if("${PLATFORM}" STREQUAL "soc")
    target_compile_definitions(test_ocv_video_xcode PRIVATE USING_SOC=1)
endif()
if("${GCC_VERSION}" STREQUAL "930")
    set(EXTRA_LIB yuv bmlib bmjpeg bmvd bmvenc bmcv avutil avcodec avformat swscale swresample opencv_imgcodecs opencv_imgproc)
    target_link_libraries(test_ocv_video_xcode opencv_core opencv_videoio opencv_highgui ${SYSTEM_LIB} ${EXTRA_LIB})
elseif("${GCC_VERSION}" STREQUAL "1131")
    set(EXTRA_LIB yuv bmlib bmjpeg bmvd bmvenc bmcv avutil avcodec avformat swscale swresample opencv_imgcodecs opencv_imgproc)
    target_link_libraries(test_ocv_video_xcode opencv_core opencv_videoio opencv_highgui ${SYSTEM_LIB} ${EXTRA_LIB})
else()
    target_link_libraries(test_ocv_video_xcode opencv_core opencv_videoio opencv_highgui ${SYSTEM_LIB})
endif()
add_dependencies(test_ocv_video_xcode bm_opencv)
