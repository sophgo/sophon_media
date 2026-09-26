cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0046 NEW)

PROJECT(ocv_vidsensor)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../bm_ffmpeg)

# libbmcv
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

#libisp — only available on soc (VI/ISP pipeline); not on pcie/pcie_arm64
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../libsophav/3rdparty/libisp/include)
if("${PLATFORM}" STREQUAL "soc")
    if("${GCC_VERSION}" STREQUAL "930")
        link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../libsophav/3rdparty/libisp/lib930/${PLATFORM})
    elseif("${GCC_VERSION}" STREQUAL "1131")
        link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../libsophav/3rdparty/libisp/lib1131/${PLATFORM})
    else()
        link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../libsophav/3rdparty/libisp/lib/${PLATFORM})
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
    ../../bm_opencv/apps/vidsensor/vidsensor.cpp
    # vidsensor.cpp
)

add_executable(test_ocv_vidsensor EXCLUDE_FROM_ALL ${SRCS})
if("${PLATFORM}" STREQUAL "soc")
    target_compile_definitions(test_ocv_vidsensor PRIVATE USING_SOC=1)
endif()
# ISP libs only available on soc (VI/ISP pipeline); not on pcie/pcie_arm64
if("${PLATFORM}" STREQUAL "soc")
    set(ISP_LIB ispv4l2_helper ae af awb cvi_bin cvi_bin_isp isp isp_algo ispv4l2_adapter sns_full)
else()
    set(ISP_LIB "")
endif()
if("${GCC_VERSION}" STREQUAL "930")
    set(EXTRA_LIB yuv bmlib bmjpeg bmvd bmvenc bmcv avutil avcodec avformat swscale swresample opencv_imgproc ${ISP_LIB})
    target_link_libraries(test_ocv_vidsensor opencv_core opencv_imgcodecs opencv_videoio ${SYSTEM_LIB} ${EXTRA_LIB})
elseif("${GCC_VERSION}" STREQUAL "1131")
    set(EXTRA_LIB yuv bmlib bmjpeg bmvd bmvenc bmcv avutil avcodec avformat swscale swresample opencv_imgproc ${ISP_LIB})
    target_link_libraries(test_ocv_vidsensor opencv_core opencv_imgcodecs opencv_videoio ${SYSTEM_LIB} ${EXTRA_LIB})
else()
    target_link_libraries(test_ocv_vidsensor opencv_core opencv_imgcodecs opencv_videoio ${SYSTEM_LIB} ${ISP_LIB})
endif()
add_dependencies(test_ocv_vidsensor bm_opencv)