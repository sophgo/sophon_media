cmake_minimum_required(VERSION 3.13)

if(CMAKE_HOST_WIN32)
    set(WINDOWS 1)
elseif(CMAKE_HOST_APPLE)
    set(MACOS 1)
elseif(CMAKE_HOST_UNIX)
    set(LINUX 1)
endif()
cmake_policy(SET CMP0046 NEW)
########################
#  ISP libraries needed transitively by libopencv_videoio.so
# link_libraries() injects these into ALL targets in this directory scope.
# ISP libraries are only available when the ISP/V4L2 pipeline is enabled
# (bm1688 soc). cv84x6 (even on soc) and all pcie modes have no VI/ISP.
if(NOT CMAKE_SYSTEM_NAME MATCHES "Windows")
  if(ENABLE_ISP)
    if("${GCC_VERSION}" STREQUAL "930")
      link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../libsophav/3rdparty/libisp/lib930/soc)
    elseif("${GCC_VERSION}" STREQUAL "1131")
      link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../libsophav/3rdparty/libisp/lib1131/soc)
    else()
      link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../libsophav/3rdparty/libisp/lib/soc)
    endif()
    link_libraries(ispv4l2_helper ae af awb cvi_bin cvi_bin_isp isp isp_algo ispv4l2_adapter sns_full)
  endif()
endif()
########################
#  Target
set(CHIP_LIST bm1684 bm1686)
set(CHIP_NAME bm1684 CACHE STRING "chip: bm1686, bm1684")
set_property(CACHE CHIP_NAME PROPERTY STRINGS ${CHIP_LIST})

set(HOST_ARCH_LIST aarch64 x86)
set(HOST_ARCH x86 CACHE STRING "host arch: aarch64 x86")
set_property(CACHE HOST_ARCH PROPERTY STRINGS ${HOST_ARCH_LIST})

set(PLATFORM_LIST pcie soc pice_arm64)
set(PLATFORM pcie CACHE STRING "platform: pcie soc pice_arm64")
set_property(CACHE PLATFORM PROPERTY STRINGS ${PLATFORM_LIST})
set(MIDDELWARE_SOC_PLATFORM ${PLATFORM})

set(FIRMWARE_CORE_SRCS_C "" CACHE INTERNAL "FIRMWARE_CORE_SRCS_C" )
set(SAMPLE_TARGETS test_bm_restart test_ff_video_encode
                test_bm_api_transcode
                test_ff_bmcv_transcode
                test_ff_hw_bmcv_transcode
                test_ff_scale_transcode
                test_ff_overlay_transcode
                test_ff_video_xcode
                test_ff_resize_transcode
                test_ff_bmjpeg
                test_ff_bmjpeg_dec_recycle
                test_ocv_jpubasic
                test_ocv_jpumulti
                test_ocv_vidbasic
                test_ocv_video_xcode
                test_ocv_vidmulti
                test_gst_vcmulti
                test_gst_transcode
                test_gst_xcode
                test_gst_bmcv_transcode
                )
# test_ocv_vidsensor / test_ff_sns_xcode use the ISP/sensor capture backend;
# only build when the ISP/V4L2 pipeline is enabled (bm1688 soc).
if(ENABLE_ISP)
    list(APPEND SAMPLE_TARGETS test_ff_sns_xcode test_ocv_vidsensor)
endif()

# add sub-modules
add_subdirectory(ff_video_decode)
add_subdirectory(ff_video_encode)
add_subdirectory(ff_video_xcode)
add_subdirectory(bm_api_transcode)
add_subdirectory(ff_bmcv_transcode)
add_subdirectory(ff_hw_bmcv_transcode)
add_subdirectory(ff_scale_transcode)
add_subdirectory(ff_overlay_transcode)
add_subdirectory(ff_resize_transcode)
add_subdirectory(ff_bmjpeg)
add_subdirectory(ff_bmjpeg_dec_recycle)
add_subdirectory(ocv_jpubasic)
add_subdirectory(ocv_jpumulti)
add_subdirectory(ocv_vidbasic)
add_subdirectory(ocv_video_xcode)
add_subdirectory(ocv_vidmulti)
add_subdirectory(gst/gst_bm_vcmulti)
add_subdirectory(gst/gst_bm_transcode)
add_subdirectory(gst/gst_bm_v4l2xcode)
add_subdirectory(gst/gst_bm_bmcv_transcode)
if(ENABLE_ISP)
    add_subdirectory(ff_sns_xcode)
    add_subdirectory(ocv_vidsensor)
endif()



# exclude from all, build alone after bm_ffmpeg and bm_opencv
add_custom_target(sophon_sample DEPENDS ${SAMPLE_TARGETS})

set(LOCATION_PREFIX sophon-sample_${PROJECT_VERSION})
install(TARGETS ${SAMPLE_TARGETS}
    RUNTIME DESTINATION ${LOCATION_PREFIX}/bin
    COMPONENT sophon-sample)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/debian_script/sophon-sample-autoconf.sh
    DESTINATION ${LOCATION_PREFIX}/data
    COMPONENT sophon-sample)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/ff_bmcv_transcode/water.bin
    DESTINATION ${LOCATION_PREFIX}/bin
    COMPONENT sophon-sample)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/ff_bmcv_transcode/water.bin
    DESTINATION ${LOCATION_PREFIX}/samples/ff_bmcv_transcode
    COMPONENT sophon-sample)

set(BMMW_SAMPLES_SOURCE
    ocv_vidmulti
    ocv_jpumulti
    ocv_vidbasic
    ocv_jpubasic
    ocv_video_xcode
    ff_video_decode
    ff_video_encode
    bm_api_transcode
    ff_bmcv_transcode
    ff_video_xcode
    ff_scale_transcode
    ff_overlay_transcode
    gst/gst_bm_vcmulti
    gst/gst_bm_transcode
    gst/gst_bm_v4l2xcode
    gst/gst_bm_bmcv_transcode
    )
if(ENABLE_ISP)
    list(APPEND BMMW_SAMPLES_SOURCE ff_sns_xcode ocv_vidsensor)
endif()

set(BMMW_SAMPLES_SOURCE_DIR ${BMMW_SAMPLES_SOURCE})
list(TRANSFORM BMMW_SAMPLES_SOURCE_DIR PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/)

list(LENGTH BMMW_SAMPLES_SOURCE_DIR size)
math(EXPR size_minus_1 "${size} - 1")

foreach(index RANGE ${size_minus_1})
    list(GET BMMW_SAMPLES_SOURCE ${index} src_dir_name)
    list(GET BMMW_SAMPLES_SOURCE_DIR ${index} abs_dir_name)
    FILE(GLOB srcs ${abs_dir_name}/*.cpp ${abs_dir_name}/*.c ${abs_dir_name}/*.h ${abs_dir_name}/*.hpp ${abs_dir_name}/Makefile)
    UNSET(INSTALL_SRCS)
    foreach(f ${srcs})
        get_filename_component(real_f ${f} REALPATH)
        list(APPEND INSTALL_SRCS ${real_f})
    endforeach()
    install(FILES ${INSTALL_SRCS}
        DESTINATION ${LOCATION_PREFIX}/samples/${src_dir_name}
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ GROUP_WRITE WORLD_READ WORLD_WRITE
        COMPONENT sophon-sample)

    set(template_file ${abs_dir_name}/CMakeLists.template)
    install(FILES ${template_file}
        DESTINATION ${LOCATION_PREFIX}/samples/${src_dir_name}
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ GROUP_WRITE WORLD_READ WORLD_WRITE
        COMPONENT sophon-sample
        RENAME CMakeLists.txt)
endforeach()


########################
#  message
message(STATUS "INSTALL PATH: " ${CMAKE_INSTALL_PREFIX})
message(STATUS "USING_CMODEL " ${USING_CMODEL})
