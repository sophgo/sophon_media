cmake_minimum_required(VERSION 3.16)
project(bmive)

# add gst compile flags
add_compile_options(${GST_VIDEO_CFLAGS} ${GST_PBUTILS_CFLAGS} ${GMODULE_CFLAGS})
add_compile_options(${GST_VIDEO_LDFLAGS} ${GST_PBUTILS_LDFLAGS} ${GMODULE_LDFLAGS})

if("${PLATFORM}" STREQUAL "pcie")
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../prebuilt/x86_64/gst)
elseif("${PLATFORM}" STREQUAL "soc" OR "${PLATFORM}" STREQUAL "pcie_arm64")
    if("${GCC_VERSION}" STREQUAL "930")
        link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../prebuilt/lib930/gst)
    elseif("${GCC_VERSION}" STREQUAL "1131")
        link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../prebuilt/lib1131/gst)
    else()
        link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../prebuilt/lib/gst)
    endif()
endif()

link_directories(${LIBSOPHAV_TOP}/3rdparty/libjpeg-turbo/lib/${PLATFORM})

# add header files path
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../bm_allocator)

# add library files path
link_directories(${CMAKE_CURRENT_BINARY_DIR}/../bm_allocator)

set(SRCS gstbmive.c plugin.c)
set(HEADERS gstbmive.h)
set(LIB_NAME gstbmive)

# build dynamic library
add_library(${LIB_NAME} SHARED ${SRCS})
target_link_libraries(${LIB_NAME}
    ${GST_VIDEO_LIBRARIES} ${GST_PBUTILS_LIBRARIES} ${GMODULE_LIBRARIES}
    gstbmallocator bmcv turbojpeg)

add_custom_target(bmive_all ALL DEPENDS ${LIB_NAME})

# install files to output path, or pack files into .deb
install(TARGETS ${LIB_NAME}
    LIBRARY DESTINATION ${GST_LOCATION_PREFIX}/lib
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    COMPONENT ${GST_COMPONENT})

install(FILES ${HEADERS}
    DESTINATION ${GST_LOCATION_PREFIX}/include
    COMPONENT ${GST_COMPONENT}-dev)
