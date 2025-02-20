cmake_minimum_required(VERSION 3.16)

project(bm_gstreamer
    LANGUAGES C CXX
    VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})
if(POLICY CMP0046)
    cmake_policy(SET CMP0046 NEW)
endif()

# set gstreamer package configs
set(VERSION "1.14.4")
set(PACKAGE "gst-bm")
set(GST_PACKAGE_NAME "GStreamer Sophgo Plug-ins")
set(GST_PACKAGE_ORIGIN "https://sophon.ai")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/config.h)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_definitions(-DHAVE_CONFIG_H)

# set debug compile options
if("${DEBUG}" STREQUAL "on")
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the build type: Release Debug RelWithDebInfo MinSizeRel.")
    add_compile_options(-O0 -Wall -g -ggdb)
else()
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the build type: Release Debug RelWithDebInfo MinSizeRel.")
    add_compile_options(-O3 -Wall)
endif()

# check gstreamer framework and core plugins
find_package(PkgConfig REQUIRED)
if("${GCC_VERSION}" STREQUAL "930")
    set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${CMAKE_CURRENT_SOURCE_DIR}/../prebuilt/lib930/pkgconfig/gst")
elseif("${GCC_VERSION}" STREQUAL "1131")
    set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${CMAKE_CURRENT_SOURCE_DIR}/../prebuilt/lib1131/pkgconfig/gst")
else()
    set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${CMAKE_CURRENT_SOURCE_DIR}/../prebuilt/lib/pkgconfig/gst")
endif()

pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(GMODULE REQUIRED gmodule-no-export-2.0)

pkg_check_modules(GST REQUIRED gstreamer-1.0)
pkg_check_modules(GST_BASE REQUIRED gstreamer-base-1.0)
pkg_check_modules(GST_VIDEO REQUIRED gstreamer-video-1.0)
pkg_check_modules(GST_ALLOCATORS REQUIRED gstreamer-allocators-1.0)
pkg_check_modules(GST_PBUTILS REQUIRED gstreamer-pbutils-1.0)

# add bmlib headers and libs
include_directories(${LIBSOPHAV_TOP}/3rdparty/libbmcv/include)
link_directories(${LIBSOPHAV_TOP}/3rdparty/libbmcv/lib/${PLATFORM})

# add vc headers and libs
include_directories(
    ${LIBSOPHAV_TOP}/video/dec/inc
    ${LIBSOPHAV_TOP}/video/enc/inc
    ${LIBSOPHAV_TOP}/jpeg/inc
    ${LIBSOPHAV_TOP}/bmcv/include
)

link_directories(
    ${LIBSOPHAV_OUT_PATH}/video/dec
    ${LIBSOPHAV_OUT_PATH}/video/enc
    ${LIBSOPHAV_OUT_PATH}/jpeg
    ${LIBSOPHAV_OUT_PATH}/bmcv
)

set(GST_COMPONENT sophon-gstreamer)  # use to make deb package
set(GST_LOCATION_PREFIX ${GST_COMPONENT}_${PROJECT_VERSION})

# add sub-modules
add_subdirectory(gst/bm_allocator)
add_subdirectory(gst/bmcodec)
add_subdirectory(gst/bmvpss)
add_subdirectory(gst/bmv4l2src)
add_subdirectory(gst/bmive)
add_subdirectory(gst/bmdpu)
add_subdirectory(gst/bmdwa)
add_subdirectory(gst/bmstitch2way)
add_subdirectory(gst/bmstitch4way)

add_custom_target(bm_gstreamer_all ALL DEPENDS bm_allocator_all bmcodec_all bmvpss_all bmive_all)

# install runtime config files
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/debian_script/03_sophon-gstreamer.conf
    DESTINATION ${GST_LOCATION_PREFIX}/data
    COMPONENT ${GST_COMPONENT})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/debian_script/sophon-gstreamer-autoconf.sh
    DESTINATION ${GST_LOCATION_PREFIX}/data
    COMPONENT ${GST_COMPONENT})
