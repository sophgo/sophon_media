
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(CMAKE_IMPORT_LIBRARY_PREFIX lib)
    if (NOT DEFINED FFMPEG_INCLUDE_DIRS)
        set(FFMPEG_INCLUDE_DIRS
            "${OpenCV_SOURCE_DIR}/3rdparty/libbmcv/include"
            "${OpenCV_SOURCE_DIR}/../ffmpeg_install/include"
            "${OpenCV_SOURCE_DIR}/../bmvid/jpeg/driver/bmjpuapi/inc"
            "${OpenCV_SOURCE_DIR}/../bmvid/3rdparty/libyuv/include"
            "${OpenCV_SOURCE_DIR}/../bmvid/vpp/driver/include/bm1684"
            "${OpenCV_SOURCE_DIR}/../bmvid/bmcv/include"
        )
    else()
        string(REPLACE " " ";" FFMPEG_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIRS})
    endif()
    if (NOT DEFINED FFMPEG_LIBRARY_DIRS)
        set(FFMPEG_LIBRARY_DIRS
            "${OpenCV_SOURCE_DIR}/3rdparty/libbmcv/lib/${PRODUCTFORM}"
            "${OpenCV_SOURCE_DIR}/../ffmpeg_install/lib"
            "${OpenCV_SOURCE_DIR}/../ffmpeg_install/bin"
            "${OpenCV_SOURCE_DIR}/../bmvid/release/lib"
            "${OpenCV_SOURCE_DIR}/../prebuilt/windows/lib"
        )
    else()
        string(REPLACE " " ";" FFMPEG_LIBRARY_DIRS ${FFMPEG_LIBRARY_DIRS})
    endif()
    LINK_DIRECTORIES(${FFMPEG_LIBRARY_DIRS})
else() #"Linux"
    if (NOT DEFINED FFMPEG_INCLUDE_DIRS)
        set(FFMPEG_INCLUDE_DIRS
            "${OpenCV_SOURCE_DIR}/3rdparty/libbmcv/include"
            "${CMAKE_INSTALL_PREFIX}/../ffmpeg/usr/local/include"
            "${CMAKE_INSTALL_PREFIX}/../decode/include"
            "${CMAKE_INSTALL_PREFIX}/../vpp/include"
            "${CMAKE_INSTALL_PREFIX}/../bmcv/include"
        )
    else()
        string(REPLACE " " ";" FFMPEG_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIRS})
    endif()
    if (NOT DEFINED FFMPEG_LIBRARY_DIRS)
        set(FFMPEG_LIBRARY_DIRS
            "${OpenCV_SOURCE_DIR}/3rdparty/libbmcv/lib/${PRODUCTFORM}"
            "${CMAKE_INSTALL_PREFIX}/../ffmpeg/usr/local/lib"
            "${CMAKE_INSTALL_PREFIX}/../decode/lib"
            "${CMAKE_INSTALL_PREFIX}/../bmcv/lib"
        )
    else()
        string(REPLACE " " ";" FFMPEG_LIBRARY_DIRS ${FFMPEG_LIBRARY_DIRS})
    endif()
endif()
set(FFMPEG_LIBRARIES avcodec avformat avutil swscale swresample ${CMAKE_IMPORT_LIBRARY_PREFIX}bmcv ${CMAKE_IMPORT_LIBRARY_PREFIX}cmodel ${CMAKE_IMPORT_LIBRARY_PREFIX}bmlib ${CMAKE_IMPORT_LIBRARY_PREFIX}bmjpeg ${CMAKE_IMPORT_LIBRARY_PREFIX}yuv ${CMAKE_IMPORT_LIBRARY_PREFIX}bmvd ${CMAKE_IMPORT_LIBRARY_PREFIX}bmvenc)

# libisp for CVI_ISP_V4L2_Init/Exit static calls -- only when the ISP/V4L2
# pipeline is enabled (bm1688 soc). cv84x6 / pcie have no VI/ISP, so the
# libisp headers/libs must not be pulled into those builds.
if(NOT CMAKE_SYSTEM_NAME MATCHES "Windows" AND ENABLE_ISP)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../libsophav/3rdparty/libisp/include)
    if("${GCC_VERSION}" STREQUAL "930")
        list(APPEND FFMPEG_LIBRARY_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../libsophav/3rdparty/libisp/lib930/soc")
    elseif("${GCC_VERSION}" STREQUAL "1131")
        list(APPEND FFMPEG_LIBRARY_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../libsophav/3rdparty/libisp/lib1131/soc")
    else()
        list(APPEND FFMPEG_LIBRARY_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../libsophav/3rdparty/libisp/lib/soc")
    endif()
    # ISP libraries moved to videoio module PUBLIC link (see videoio/CMakeLists.txt)
    # list(APPEND FFMPEG_LIBRARIES ispv4l2_helper ae af awb cvi_bin cvi_bin_isp isp isp_algo ispv4l2_adapter sns_full)
endif()
#list(APPEND FFMPEG_LIBRARIES
#${CMAKE_IMPORT_LIBRARY_PREFIX}bmion
#)



add_definitions(-DVPP_BM1684)
add_definitions(-DBM1684_CHIP)
#add_definitions(-DUSING_SOC)
#add_definitions(-DHAVE_LIBYUV)
set(HAVE_BMCV ON)

if(${PRODUCTFORM} STREQUAL "soc")
  #set(USING_SOC ON)
  #set(HAVE_LIBYUV ON)
  add_definitions(-DUSING_SOC)
  add_definitions(-DHAVE_LIBYUV)
endif()
#
#if(${CHIP} STREQUAL "bm1684")
#    if(${BUILD_opencv_world})
#        set(HAVE_BMCV OFF)
#    else()
#        set(HAVE_BMCV ON)
#    endif()
#    if(${ENABLE_BMCPU})
#        add_definitions(-DENABLE_BMCPU)
#    endif()
#  add_definitions(-DBM1684_CHIP)
#endif()
#
#if(${CHIP} STREQUAL "bm1682")
#  add_definitions(-DVPP_BM1682)
#endif()
#if(${CHIP} STREQUAL "bm1684")
#  add_definitions(-DVPP_BM1684)
#endif()
#if(${CHIP} STREQUAL "bm1880")
#  add_definitions(-DVPP_BM1880)
#endif()
