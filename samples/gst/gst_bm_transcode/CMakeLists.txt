cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0046 NEW)

PROJECT(gst_bm_transcode)

find_package(PkgConfig REQUIRED)
set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${CMAKE_CURRENT_SOURCE_DIR}/../../../prebuilt/lib/pkgconfig/gst")

pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(GMODULE REQUIRED gmodule-no-export-2.0)

pkg_check_modules(GST REQUIRED gstreamer-1.0)
pkg_check_modules(GST_BASE REQUIRED gstreamer-base-1.0)
pkg_check_modules(GST_VIDEO REQUIRED gstreamer-video-1.0)
pkg_check_modules(GST_ALLOCATORS REQUIRED gstreamer-allocators-1.0)
pkg_check_modules(GST_PBUTILS REQUIRED gstreamer-pbutils-1.0)

add_compile_options(${GST_VIDEO_CFLAGS} ${GST_PBUTILS_CFLAGS} ${GMODULE_CFLAGS})
add_compile_options(${GST_VIDEO_LDFLAGS} ${GST_PBUTILS_LDFLAGS} ${GMODULE_LDFLAGS})

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../prebuilt/lib/gst)


set(SYSTEM_LIB pthread stdc++ z)

SET(SRCS
  gst_transcode.cpp
)


add_executable(test_gst_transcode EXCLUDE_FROM_ALL ${SRCS})
target_link_libraries(test_gst_transcode ${GST_VIDEO_LIBRARIES} ${GST_PBUTILS_LIBRARIES} ${GMODULE_LIBRARIES} ${SYSTEM_LIB})
