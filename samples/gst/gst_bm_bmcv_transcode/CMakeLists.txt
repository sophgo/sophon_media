cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0046 NEW)

PROJECT(test_gst_bmcv_transcode)

find_package(PkgConfig REQUIRED)
if("${PLATFORM}" STREQUAL "pcie")
    set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${CMAKE_CURRENT_SOURCE_DIR}/../../../prebuilt/x86_64/pkgconfig/gst")
elseif("${PLATFORM}" STREQUAL "soc" OR "${PLATFORM}" STREQUAL "pcie_arm64")
    if("${GCC_VERSION}" STREQUAL "930")
        set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${CMAKE_CURRENT_SOURCE_DIR}/../../../prebuilt/lib930/pkgconfig/gst")
    elseif("${GCC_VERSION}" STREQUAL "1131")
        set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${CMAKE_CURRENT_SOURCE_DIR}/../../../prebuilt/lib1131/pkgconfig/gst")
    else()
        set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${CMAKE_CURRENT_SOURCE_DIR}/../../../prebuilt/lib/pkgconfig/gst")
    endif()
endif()

pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(GMODULE REQUIRED gmodule-no-export-2.0)

pkg_check_modules(GST REQUIRED gstreamer-1.0)
pkg_check_modules(GST_BASE REQUIRED gstreamer-base-1.0)
pkg_check_modules(GST_VIDEO REQUIRED gstreamer-video-1.0)
pkg_check_modules(GST_ALLOCATORS REQUIRED gstreamer-allocators-1.0)
pkg_check_modules(GST_PBUTILS REQUIRED gstreamer-pbutils-1.0)

add_compile_options(${GST_VIDEO_CFLAGS} ${GST_PBUTILS_CFLAGS} ${GMODULE_CFLAGS} ${GST_APP_CFLAGS})
add_compile_options(${GST_VIDEO_LDFLAGS} ${GST_PBUTILS_LDFLAGS} ${GMODULE_LDFLAGS} ${GST_APP_LDFLAGS})

if("${PLATFORM}" STREQUAL "pcie")
    pkg_check_modules(FFI REQUIRED libffi)
    pkg_check_modules(GST_TAG REQUIRED gstreamer-tag-1.0)
    pkg_check_modules(ZLIB REQUIRED zlib)

    add_compile_options(${FFI_CFLAGS} ${GST_TAG_CFLAGS} ${ZLIB_CFLAGS})
    add_compile_options(${FFI_LDFLAGS} ${GST_TAG_LDFLAGS} ${ZLIB_CFLAGS})

    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../prebuilt/x86_64/gst)
elseif("${PLATFORM}" STREQUAL "soc" OR "${PLATFORM}" STREQUAL "pcie_arm64")
    if("${GCC_VERSION}" STREQUAL "930")
        pkg_check_modules(FFI REQUIRED libffi)
        pkg_check_modules(GST_TAG REQUIRED gstreamer-tag-1.0)

        add_compile_options(${FFI_CFLAGS} ${GST_TAG_CFLAGS})
        add_compile_options(${FFI_LDFLAGS} ${GST_TAG_LDFLAGS})

        link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../prebuilt/lib930/gst)
    elseif("${GCC_VERSION}" STREQUAL "1131")
        pkg_check_modules(FFI REQUIRED libffi)
        pkg_check_modules(GST_TAG REQUIRED gstreamer-tag-1.0)
        pkg_check_modules(ZLIB REQUIRED zlib)

        add_compile_options(${FFI_CFLAGS} ${GST_TAG_CFLAGS} ${ZLIB_CFLAGS})
        add_compile_options(${FFI_LDFLAGS} ${GST_TAG_LDFLAGS} ${ZLIB_CFLAGS})

        link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../prebuilt/lib1131/gst)
    else()
        link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../prebuilt/lib/gst)
    endif()
endif()

#libbmcv
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../libsophav/bmcv/include)
link_directories(${CMAKE_BINARY_DIR}/libsophav/bmcv)
# libbmlib
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../libsophav/3rdparty/libbmcv/include)
if("${PLATFORM}" STREQUAL "pcie" OR "${PLATFORM}" STREQUAL "pcie_arm64")
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../libsophav/3rdparty/libbmcv/lib/${PLATFORM})
    add_definitions(-DBM_PCIE_MODE)
elseif("${PLATFORM}" STREQUAL "soc")
    if("${GCC_VERSION}" STREQUAL "930")
        link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../libsophav/3rdparty/libbmcv/lib930/${PLATFORM})
    elseif("${GCC_VERSION}" STREQUAL "1131")
        link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../libsophav/3rdparty/libbmcv/lib1131/${PLATFORM})
    else()
        link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../libsophav/3rdparty/libbmcv/lib/${PLATFORM})
    endif()
endif()

#jpeg
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../libsophav/jpeg/3rdparty/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../libsophav/jpeg/inc)
link_directories(${CMAKE_BINARY_DIR}/libsophav/jpeg)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../libsophav/3rdparty/libjpeg-turbo/lib/${PLATFORM})

set(SYSTEM_LIB pthread stdc++ z)

SET(SRCS
  gst_bmcv_transcode.cpp
)

set(BMLIB    bmlib)
set(BMCV_LIB bmcv)
set(JPEG_LIB bmjpeg turbojpeg)

add_executable(test_gst_bmcv_transcode EXCLUDE_FROM_ALL ${SRCS})
if("${PLATFORM}" STREQUAL "pcie")
    target_link_libraries(test_gst_bmcv_transcode ${GST_VIDEO_LIBRARIES} ${GST_PBUTILS_LIBRARIES} ${GMODULE_LIBRARIES} ${GST_APP_LIBRARIES} ${SYSTEM_LIB} ${FFI_LIBRARIES} ${GST_TAG_LIBRARIES} ${ZLIB_CFLAGS} ${BMLIB} ${BMCV_LIB} ${JPEG_LIB})
elseif("${PLATFORM}" STREQUAL "soc" OR "${PLATFORM}" STREQUAL "pcie_arm64")
    if("${GCC_VERSION}" STREQUAL "930")
        target_link_libraries(test_gst_bmcv_transcode ${GST_VIDEO_LIBRARIES} ${GST_PBUTILS_LIBRARIES} ${GMODULE_LIBRARIES} ${GST_APP_LIBRARIES} ${SYSTEM_LIB} ${FFI_LIBRARIES} ${GST_TAG_LIBRARIES} ${BMLIB} ${BMCV_LIB} ${JPEG_LIB})
    elseif("${GCC_VERSION}" STREQUAL "1131")
        target_link_libraries(test_gst_bmcv_transcode ${GST_VIDEO_LIBRARIES} ${GST_PBUTILS_LIBRARIES} ${GMODULE_LIBRARIES} ${GST_APP_LIBRARIES} ${SYSTEM_LIB} ${FFI_LIBRARIES} ${GST_TAG_LIBRARIES} ${ZLIB_CFLAGS} ${BMLIB} ${BMCV_LIB} ${JPEG_LIB})
    else()
        target_link_libraries(test_gst_bmcv_transcode ${GST_VIDEO_LIBRARIES} ${GST_PBUTILS_LIBRARIES} ${GMODULE_LIBRARIES} ${GST_APP_LIBRARIES} ${SYSTEM_LIB} ${BMLIB} ${BMCV_LIB} ${JPEG_LIB})
    endif()
endif()
