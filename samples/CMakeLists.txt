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
                )
if("${PLATFORM}" STREQUAL "soc")
    list(APPEND SAMPLE_TARGETS test_ff_sns_xcode)
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
if("${PLATFORM}" STREQUAL "soc")
    add_subdirectory(ff_sns_xcode)
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
    )
if("${PLATFORM}" STREQUAL "soc")
    list(APPEND BMMW_SAMPLES_SOURCE ff_sns_xcode)
endif()

set(BMMW_SAMPLES_SOURCE_DIR ${BMMW_SAMPLES_SOURCE})
list(TRANSFORM BMMW_SAMPLES_SOURCE_DIR PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/)

list(LENGTH BMMW_SAMPLES_SOURCE_DIR size)
math(EXPR size_minus_1 "${size} - 1")

foreach(index RANGE ${size_minus_1})
    list(GET BMMW_SAMPLES_SOURCE ${index} src_dir_name)
    list(GET BMMW_SAMPLES_SOURCE_DIR ${index} abs_dir_name)
    FILE(GLOB srcs ${abs_dir_name}/*.cpp ${abs_dir_name}/*.c ${abs_dir_name}/*.h ${abs_dir_name}/*.hpp ${abs_dir_name}/CMakeLists.txt ${abs_dir_name}/Makefile)
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
