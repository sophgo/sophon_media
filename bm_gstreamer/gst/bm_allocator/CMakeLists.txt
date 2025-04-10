cmake_minimum_required(VERSION 3.16)
project(bm_allocator)

# add gst compile flags
add_compile_options(${GST_BASE_CFLAGS} ${GST_VIDEO_CFLAGS} ${GST_ALLOCATORS_CFLAGS})
add_compile_options(${GST_BASE_LDFLAGS} ${GST_VIDEO_LDFLAGS} ${GST_ALLOCATORS_LDFLAGS})

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

set(SRCS gstbmallocator.c)
set(HEADERS gstbmallocator.h)
set(LIB_NAME gstbmallocator)

# build dynamic library
add_library(${LIB_NAME} SHARED ${SRCS})
target_link_libraries(${LIB_NAME}
    gstallocators-1.0
    bmlib bmvd bmvenc)

add_custom_target(bm_allocator_all ALL DEPENDS ${LIB_NAME})

# install files to output path, or pack files into .deb
install(TARGETS ${LIB_NAME}
    LIBRARY DESTINATION ${GST_LOCATION_PREFIX}/lib
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    COMPONENT ${GST_COMPONENT})

install(FILES ${HEADERS}
    DESTINATION ${GST_LOCATION_PREFIX}/include
    COMPONENT ${GST_COMPONENT}-dev)
