cmake_minimum_required(VERSION 3.16)
project(bmv4l2src)

# add gst compile flags
add_compile_options(${GST_VIDEO_CFLAGS} ${GST_PBUTILS_CFLAGS} ${GMODULE_CFLAGS})
add_compile_options(${GST_VIDEO_LDFLAGS} ${GST_PBUTILS_LDFLAGS} ${GMODULE_LDFLAGS})

if("${GCC_VERSION}" STREQUAL "930")
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../prebuilt/lib930/gst)
    link_directories(${LIBSOPHAV_TOP}/3rdparty/libisp/lib930/soc)
else()
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../prebuilt/lib/gst)
    link_directories(${LIBSOPHAV_TOP}/3rdparty/libisp/lib/soc)
endif()


# add header files path
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../bmv4l2src)
include_directories(${LIBSOPHAV_TOP}/3rdparty/libisp/include)

# add library files path
link_directories(${CMAKE_CURRENT_BINARY_DIR}/../bmv4l2src)

set(SRCS
    gstbmv4l2src.c
    gstv4l2allocator.c
    gstv4l2bufferpool.c
    gstv4l2object.c
    v4l2-utils.c
    v4l2_calls.c
    plugin.c)

set(HEADERS
    gstbmv4l2src.h
    gstv4l2allocator.h
    gstv4l2bufferpool.h
    gstv4l2object.h
    v4l2-utils.h
    ext/v4l2-common.h
    ext/v4l2-controls.h
    ext/types-compat.h
    ext/videodev2.h)

set(LIB_NAME gstbmv4l2src)

# build dynamic library
add_library(${LIB_NAME} SHARED ${SRCS})
target_link_libraries(${LIB_NAME}
    ${GST_VIDEO_LIBRARIES} ${GST_PBUTILS_LIBRARIES} ${GMODULE_LIBRARIES}
    gstbmallocator ispv4l2_helper ae af awb cvi_bin cvi_bin_isp cvi_ispd2 isp isp_algo ispv4l2_adapter json-c sns_full
    )

add_custom_target(bmv4l2src_all ALL DEPENDS ${LIB_NAME})

# install files to output path, or pack files into .deb
install(TARGETS ${LIB_NAME}
    LIBRARY DESTINATION ${GST_LOCATION_PREFIX}/lib
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    COMPONENT ${GST_COMPONENT})

install(FILES ${HEADERS}
    DESTINATION ${GST_LOCATION_PREFIX}/include
    COMPONENT ${GST_COMPONENT}-dev)

