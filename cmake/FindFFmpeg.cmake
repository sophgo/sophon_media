function(ADD_TARGET_FFMPEG target_name chip_name platform enable_sdl jpeg_abs_path video_abs_path bmcv_abs_path component out_abs_path)
    set(FFMPEG_LIB_TARGET ${out_abs_path}/usr/local/lib)
    set(FFMPEG_HEADER_TARGET ${out_abs_path}/usr/local/include)
    set(FFMPEG_SHARE_TARGET ${out_abs_path}/usr/local/share)
    set(FFMPEG_BIN_TARGET ${out_abs_path}/usr/local/bin)
    set(FFMPEG_VERSION ${PROJECT_VERSION})
    set(LOCATION_PREFIX sophon-ffmpeg_${FFMPEG_VERSION})

    #    if("${platform}" STREQUAL "pcie")
    #        set(PKG_CONFIG_PATH ${CMAKE_SOURCE_DIR}/prebuilt/x86_64/lib/pkgconfig)
    #        set(CROSS_COMPILE_OPTION --enable-cross-compile --cross-prefix=  --target-os=linux --arch=x86_64)
    #        set(EXTRA_LDFLAGS -L${CMAKE_SOURCE_DIR}/prebuilt/x86_64/lib -Wl,-rpath=${CMAKE_SOURCE_DIR}/prebuilt/x86_64/lib)
    #    elseif("${platform}" STREQUAL "pcie_arm64")
    #        set(PKG_CONFIG_PATH ${CMAKE_SOURCE_DIR}/prebuilt/lib/pkgconfig)
    #        set(CROSS_COMPILE_OPTION --enable-cross-compile --cross-prefix=aarch64-linux-gnu- --target-os=linux --arch=aarch64)
    #        set(EXTRA_LDFLAGS -L${CMAKE_SOURCE_DIR}/prebuilt/lib -Wl,-rpath=${CMAKE_SOURCE_DIR}/prebuilt/lib)
    #    elseif("${platform}" STREQUAL "pcie_mips64")
    #        set(PKG_CONFIG_PATH ${CMAKE_SOURCE_DIR}/prebuilt/mips_64/lib/pkgconfig)
    #        set(CROSS_COMPILE_OPTION --enable-cross-compile --cross-prefix=mips-linux-gnu- --target-os=linux --arch=mips64el --cpu=loongson3)
    #        set(OPTFLAGS -O3 -g -march=loongson3a -mips64r2 -mabi=64 -D_GLIBCXX_USE_CXX11_ABI=0)
    #        set(EXTRA_LDFLAGS -L${CMAKE_SOURCE_DIR}/prebuilt/mips_64/lib -Wl,-rpath=${CMAKE_SOURCE_DIR}/prebuilt/mips_64/lib)
    #        set(EXTRA_LIBS -lz)
    #    elseif("${platform}" STREQUAL "pcie_loongarch64")
    #        set(PKG_CONFIG_PATH ${CMAKE_SOURCE_DIR}/prebuilt/loongarch64/lib/pkgconfig)
    #        set(CROSS_COMPILE_OPTION --enable-cross-compile --cross-prefix=loongarch64-linux-gnu- --target-os=linux --arch=loongarch64)
    #        set(OPTFLAGS -O3 -g -D_GLIBCXX_USE_CXX11_ABI=0)
    #        set(EXTRA_LDFLAGS -L${CMAKE_SOURCE_DIR}/prebuilt/loongarch64/lib -Wl,-rpath=${CMAKE_SOURCE_DIR}/prebuilt/loongarch64/lib)
    #        set(EXTRA_LIBS -lz -lpthread -lharfbuzz -lbz2 -lpng16 -lfreetype)
    #    elseif("${platform}" STREQUAL "pcie_sw64")
    #        set(PKG_CONFIG_PATH ${CMAKE_SOURCE_DIR}/prebuilt/sw_64/lib/pkgconfig)
    #        set(CROSS_COMPILE_OPTION --enable-cross-compile --cross-prefix=sw_64-sunway-linux-gnu- --target-os=linux --arch=sw64)
    #        set(OPTFLAGS -O3 -g -D_GLIBCXX_USE_CXX11_ABI=0)
    #        set(EXTRA_LDFLAGS -L${CMAKE_SOURCE_DIR}/prebuilt/sw_64/lib -Wl,-rpath=${CMAKE_SOURCE_DIR}/prebuilt/sw_64/lib)
    #        set(EXTRA_LIBS -lz)
    #    elseif("${platform}" STREQUAL "pcie_riscv64")
    #        set(PKG_CONFIG_PATH ${CMAKE_SOURCE_DIR}/prebuilt/riscv64/lib/pkgconfig)
    #        set(CROSS_COMPILE_OPTION --enable-cross-compile --cross-prefix=riscv64-linux-gnu- --target-os=linux --arch=riscv64)
    #        set(OPTFLAGS -O3 -g -D_GLIBCXX_USE_CXX11_ABI=0)
    #        set(EXTRA_LDFLAGS -L${CMAKE_SOURCE_DIR}/prebuilt/riscv64/lib -Wl,-rpath=${CMAKE_SOURCE_DIR}/prebuilt/riscv64/lib)
    #        set(EXTRA_LIBS -lz -lpthread -lharfbuzz -lbz2 -lpng16 -lfreetype -lssl)
    #    else()
    set(PKG_CONFIG_PATH ${CMAKE_SOURCE_DIR}/prebuilt/lib/pkgconfig)
    if("${GCC_VERSION}" STREQUAL "930")
        set(CROSS_COMPILE_OPTION --enable-cross-compile --cross-prefix=aarch64-linux- --target-os=linux --arch=aarch64 --cpu=cortex-a53)
    elseif("${GCC_VERSION}" STREQUAL "1131")
        set(CROSS_COMPILE_OPTION --enable-cross-compile --cross-prefix=aarch64-none-linux-gnu- --target-os=linux --arch=aarch64 --cpu=cortex-a53)
    else()
        set(CROSS_COMPILE_OPTION --enable-cross-compile --cross-prefix=aarch64-linux-gnu- --target-os=linux --arch=aarch64 --cpu=cortex-a53)
    endif()

    set(EXTRA_LDFLAGS -L${CMAKE_SOURCE_DIR}/prebuilt/lib -Wl,-rpath=${CMAKE_SOURCE_DIR}/prebuilt/lib)
    #    endif()
    if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        set(DEBUG_OPTION --enable-debug --disable-optimizations --disable-stripping)
    endif()
    # TODO: add other modules headers
    set(EXTRA_CFLAGS    ${EXTRA_CFLAGS}
                        -I${LIBSOPHAV_TOP}/jpeg/inc
                        -I${LIBSOPHAV_TOP}/video/dec/inc
                        -I${LIBSOPHAV_TOP}/video/enc/inc
                        -I${LIBSOPHAV_TOP}/bmcv/include
                        -I${LIBSOPHAV_TOP}/3rdparty/osdrv
                        -I${LIBSOPHAV_TOP}/3rdparty/libisp/include
                        -I${LIBSOPHAV_TOP}/3rdparty/libdrm/include
                        -I${LIBSOPHAV_TOP}/3rdparty/libdrm/include/libdrm
                        -I${LIBSOPHAV_TOP}/3rdparty/libdrm/include/libkms
                        -I${CMAKE_SOURCE_DIR}/prebuilt/include
                        -I${CMAKE_SOURCE_DIR}/prebuilt/include/gbclient)

    set(EXTRA_CFLAGS ${EXTRA_CFLAGS} -I${LIBSOPHAV_TOP}/3rdparty/libbmcv/include)
    # TODO: add other modules libraries
    set(EXTRA_LDFLAGS   ${EXTRA_LDFLAGS}
                        -L${CMAKE_SOURCE_DIR}/bmcv )
    #TODO change bmlib dir
    set(EXTRA_LDFLAGS   ${EXTRA_LDFLAGS}
                        -L${LIBSOPHAV_OUT_PATH}/video/dec
                        -L${LIBSOPHAV_OUT_PATH}/video/enc
                        -L${LIBSOPHAV_OUT_PATH}/jpeg
                        -L${LIBSOPHAV_OUT_PATH}/bmcv
                        )

    if("${GCC_VERSION}" STREQUAL "930")
        set(EXTRA_LDFLAGS   ${EXTRA_LDFLAGS}
                            -L${LIBSOPHAV_TOP}/3rdparty/libbmcv/lib930/${platform}
                            -L${LIBSOPHAV_TOP}/3rdparty/libisp/lib930/soc
                            -Wl,-rpath=${CMAKE_SOURCE_DIR}/3rdparty/libbmcv/lib930/${platform}
                            )
    elseif("${GCC_VERSION}" STREQUAL "1131")
        set(EXTRA_LDFLAGS   ${EXTRA_LDFLAGS}
                            -L${LIBSOPHAV_TOP}/3rdparty/libbmcv/lib1131/${platform}
                            -L${LIBSOPHAV_TOP}/3rdparty/libisp/lib1131/soc
                            -Wl,-rpath=${CMAKE_SOURCE_DIR}/3rdparty/libbmcv/lib1131/${platform}
                            )
    else()
        set(EXTRA_LDFLAGS   ${EXTRA_LDFLAGS}
                            -L${LIBSOPHAV_TOP}/3rdparty/libbmcv/lib/${platform}
                            -L${LIBSOPHAV_TOP}/3rdparty/libisp/lib/soc
                            )
    endif()

    if(CMAKE_SYSTEM_NAME MATCHES "Linux")
        set(EXTRA_LIBS      ${EXTRA_LIBS} -lpthread -lrt -lssl -lcrypto -ldl -lresolv -lstdc++ -lgb28181_sip -lm)
    elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
        set(EXTRA_LIBS      ${EXTRA_LIBS} -lrt -lssl -lcrypto -ldl -lresolv -lstdc++ -lgb28181_sip -lm)
    endif()

    set(EXTRA_OPTIONS   --disable-decoder=h264_v4l2m2m
                        --disable-vaapi
                        --disable-hwaccel=h263_vaapi  --disable-hwaccel=h264_vaapi  --disable-hwaccel=hevc_vaapi
                        --disable-hwaccel=mjpeg_vaapi --disable-hwaccel=mpeg2_vaapi --disable-hwaccel=mpeg4_vaapi
                        --disable-hwaccel=vc1_vaapi   --disable-hwaccel=vp8_vaapi   --disable-hwaccel=wmv3_vaapi
            --enable-encoder=h264_bm      --enable-encoder=h265_bm      --enable-bmcodec)

    # basic config for different chips
#if("${chip_name}" STREQUAL "bm1686")
        #set(EXTRA_LDFLAGS ${EXTRA_LDFLAGS} -L${LIBSOPHAV_OUT_PATH}/bmcv -Wl,-rpath=${CMAKE_SOURCE_DIR}/3rdparty/libbmcv/lib/${platform})
    set(EXTRA_LIBS ${EXTRA_LIBS} -lispv4l2_helper -lae -laf -lawb -lcvi_bin -lcvi_bin_isp -lisp -lisp_algo -lispv4l2_adapter -lsns_full)#set mw lib
    set(EXTRA_LIBS ${EXTRA_LIBS} -lbmlib -lbmjpeg -lbmvd -lbmvenc -lbmcv -lbo -ldrm -lkms)

    # TODO: depend on chip name?
    set(EXTRA_CFLAGS ${EXTRA_CFLAGS} -DBM1684)
    set(EXTRA_CFLAGS ${EXTRA_CFLAGS} -DBM1686)
    set(EXTRA_CFLAGS ${EXTRA_CFLAGS} -DBM1688)

    set(EXTRA_OPTIONS   ${EXTRA_OPTIONS}
                        --enable-encoder=jpeg_bm
                        --enable-decoder=jpeg_bm
                        --enable-decoder=h264_bm
                        --enable-decoder=hevc_bm
                        --disable-decoder=h263_bm
                        --disable-decoder=vc1_bm
                        --disable-decoder=wmv1_bm --disable-decoder=wmv2_bm --disable-decoder=wmv3_bm
                        --disable-decoder=mpeg1_bm --disable-decoder=mpeg2_bm --disable-decoder=mpeg4_bm --disable-decoder=mpeg4v3_bm
                        --disable-decoder=flv1_bm
                        --disable-decoder=cavs_bm --disable-decoder=avs_bm
                        --disable-decoder=vp3_bm --disable-decoder=vp8_bm
                        --enable-zlib --enable-openssl)

    if(NOT "${platform}" STREQUAL "soc")
        set(EXTRA_OPTIONS ${EXTRA_OPTIONS} --enable-encoder=bmx264)
    else()
        set(EXTRA_OPTIONS ${EXTRA_OPTIONS} --disable-encoder=bmx264)
    endif()
#    else()
#        set(EXTRA_OPTIONS ${EXTRA_OPTIONS} --disable-bmcodec --disable-encoder=h264_bm --disable-encoder=h265_bm --disable-filter=scale_bm --disable-postproc --disable-autodetect --enable-zlib)
#
#        if("${platform}" STREQUAL "soc")
#            set(EXTRA_OPTIONS ${EXTRA_OPTIONS} --enable-libx264 --enable-gpl --enable-encoder=jpeg_bm --enable-decoder=jpeg_bm)
#            set(EXTRA_LIBS ${EXTRA_LIBS} -lbmjpeg)
#        else()
#            set(EXTRA_OPTIONS ${EXTRA_OPTIONS} --disable-encoder=jpeg_bm --disable-decoder=jpeg_bm)
#        endif()
#    endif()

    # config depending on platform
    if(NOT "${platform}" STREQUAL "soc")
        set(EXTRA_CFLAGS ${EXTRA_CFLAGS} -DBM_PCIE_MODE=1)
    endif()

    if("${SUBTYPE}" STREQUAL "fpga")
        set(EXTRA_CFLAGS ${EXTRA_CFLAGS} -DBOARD_FPGA=1)
    endif()

    set(EXTRA_OPTIONS ${EXTRA_OPTIONS} --enable-sdl2 --enable-ffplay)

    set(FFMPEG_BUILD_TARGETS all)

    set(EXTRA_LIBS -Wl,--start-group ${EXTRA_LIBS} -Wl,--end-group)

    set(PKG_OPTION  ${CROSS_COMPILE_OPTION}
                    --pkg-config=pkg-config
                    --optflags="${OPTFLAGS}"
                    --enable-static
                    --enable-shared
                    --enable-pic
                    --enable-swscale
                    --enable-libfreetype
                    --enable-libmp3lame
                    ${EXTRA_OPTIONS}
                    --extra-cflags="${EXTRA_CFLAGS}"
                    --extra-ldflags="${EXTRA_LDFLAGS}"
                    --extra-libs="${EXTRA_LIBS}"
                    --extra-version="sophon-${FFMPEG_VERSION}"
                    ${DEBUG_OPTION})

    add_custom_command(OUTPUT ${FFMPEG_LIB_TARGET} ${FFMPEG_HEADER_TARGET} ${FFMPEG_BIN_TARGET} ${FFMPEG_SHARE_TARGET}
        COMMAND ${CMAKE_COMMAND} -E env PKG_CONFIG_PATH=${PKG_CONFIG_PATH} ./configure ${PKG_OPTION}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${out_abs_path}
        COMMAND make -j`nproc` ${FFMPEG_BUILD_TARGETS} VERBOSE=1
        COMMAND make install DESTDIR=${out_abs_path}
        #COMMAND make distclean
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bm_ffmpeg)

    add_custom_target(${target_name} ALL
        DEPENDS ${FFMPEG_LIB_TARGET} ${FFMPEG_HEADER_TARGET} ${FFMPEG_BIN_TARGET} ${FFMPEG_SHARE_TARGET})
    # support x264
    #if((NOT ("${platform}" STREQUAL "soc")) AND ("${chip_name}" STREQUAL "bm1686"))
    #    add_custom_command(TARGET ${target_name}
    #        PRE_BUILD
    #        COMMAND ${CMAKE_COMMAND} -E make_directoy ${FFMPEG_LIB_TARGET}/firmware
    #        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/prebuilt/firmware/libx264.so ${FFMPEG_LIB_TARGET}/firmware
    #        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    #endif()
    install(DIRECTORY ${FFMPEG_LIB_TARGET}/
        DESTINATION ${LOCATION_PREFIX}/lib
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        COMPONENT ${component}
        PATTERN "pkgconfig" EXCLUDE
        PATTERN "*.a" EXCLUDE)
    install(DIRECTORY ${FFMPEG_BIN_TARGET}/
        DESTINATION ${LOCATION_PREFIX}/bin
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        COMPONENT ${component})
    install(DIRECTORY ${FFMPEG_LIB_TARGET}/
        DESTINATION ${LOCATION_PREFIX}/lib
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        COMPONENT ${component}-dev
        FILES_MATCHING PATTERN "*.a"
        PATTERN "pkgconfig" EXCLUDE)
    install(DIRECTORY ${FFMPEG_HEADER_TARGET}/
        DESTINATION ${LOCATION_PREFIX}/include
        COMPONENT ${component}-dev)
    install(DIRECTORY ${FFMPEG_SHARE_TARGET}/
        DESTINATION ${LOCATION_PREFIX}/share
        COMPONENT ${component}-dev)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian_script/01_sophon-ffmpeg.conf
        DESTINATION ${LOCATION_PREFIX}/data
        COMPONENT ${component})
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian_script/sophon-ffmpeg-autoconf.sh
        DESTINATION ${LOCATION_PREFIX}/data
        COMPONENT ${component})
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian_script/FFMPEGConfig.cmake
        DESTINATION ${LOCATION_PREFIX}/lib/cmake
        COMPONENT ${component}-dev)
    install(DIRECTORY ${FFMPEG_LIB_TARGET}/pkgconfig
        DESTINATION ${LOCATION_PREFIX}/lib
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        COMPONENT ${component}-dev)

endfunction(ADD_TARGET_FFMPEG)


macro(SET_OPENCV_ENV chip_name subtype platform enable_abi0 enable_ocv_contrib vpu_abs_path
        jpu_abs_path yuv_abs_path bmcv_abs_path vpp_abs_path ion_abs_path ffmpeg_abs_path out_abs_path)
    set(CHIP                        ${chip_name})
    set(SUBTYPE                     ${subtype})
    set(PRODUCTFORM                 ${platform})
    set(WITH_FFMPEG                 ON)
    set(WITH_GSTREAMER              OFF)
    set(WITH_GTK                    OFF)
    set(WITH_1394                   OFF)
    set(WITH_V4L                    ON)
    set(WITH_CUDA                   OFF)
    set(WITH_OPENCL                 OFF)
    set(WITH_LAPACK                 OFF)
    set(WITH_TBB                    ON)
    set(BUILD_TBB                   ON)
    set(WITH_TIFF                   ON)
    set(BUILD_TIFF                  ON)
    set(WITH_JPEG                   ON)
    set(OPENCV_GENERATE_PKGCONFIG   ON)
    set(FFMPEG_INCLUDE_DIRS         "${ffmpeg_abs_path}/usr/local/include"
                                    "${LIBSOPHAV_TOP}/jpeg/inc"
                                    "${LIBSOPHAV_TOP}/video/dec/inc"
                                    "${LIBSOPHAV_TOP}/video/enc/inc"
                                    "${yuv_abs_path}/libyuv/include"
                                    "${LIBSOPHAV_TOP}/bmcv/include"
                                    "${LIBSOPHAV_TOP}/3rdparty/libbmcv/include")
    set(FFMPEG_LIBRARY_DIRS         "${ffmpeg_abs_path}/usr/local/lib"
                                    "${jpu_abs_path}/jpeg"
                                    "${vpu_abs_path}/video/dec"
                                    "${vpu_abs_path}/video/enc"
                                    "${yuv_abs_path}/libyuv/lib"
                                    "${bmcv_abs_path}/bmcv")

    if("${GCC_VERSION}" STREQUAL "930")
        set(FFMPEG_LIBRARY_DIRS     ${FFMPEG_LIBRARY_DIRS}
                                    "${LIBSOPHAV_TOP}/3rdparty/libbmcv/lib930/${platform}")
    elseif("${GCC_VERSION}" STREQUAL "1131")
        set(FFMPEG_LIBRARY_DIRS     ${FFMPEG_LIBRARY_DIRS}
                                    "${LIBSOPHAV_TOP}/3rdparty/libbmcv/lib1131/${platform}")
    else()
        set(FFMPEG_LIBRARY_DIRS     ${FFMPEG_LIBRARY_DIRS}
                                    "${LIBSOPHAV_TOP}/3rdparty/libbmcv/lib/${platform}")
    endif()

    if (${platform} STREQUAL "pcie_sw64" OR ${platform} STREQUAL "pcie_loongarch64"  OR ${platform} STREQUAL "pcie_riscv64")
        set(BUILD_JPEG                  ON)
    else()
        set(BUILD_JPEG                  OFF)
    endif()

    if (${enable_abi0})
        set(ABI_FLAG    0)
    else()
        set(ABI_FLAG    1)
    endif()

    if (${enable_ocv_contrib})
        set(OPENCV_EXTRA_MODULES_PATH     ${CMAKE_CURRENT_SOURCE_DIR}/opencv_contrib-4.1.0/modules)
        set(OPENCV_ENABLE_NONFREE   ON)
    else()
        set(OPENCV_ENABLE_NONFREE   OFF)
    endif()

    set(CMAKE_MAKE_PROGRAM          make)

    if (${platform} STREQUAL "soc")

        set(HAVE_opencv_python3         ON)
        set(BUILD_opencv_python3        ON)
        set(PYTHON3_INCLUDE_PATH        ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/include/python3.5)
        set(PYTHON3_LIBRARIES           ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/lib/libpython3.5m.so)
        set(PYTHON3_EXECUTABLE          ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/bin/python3)
        set(PYTHON3_NUMPY_INCLUDE_DIRS  ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/lib/local/python3.5/dist-packages/numpy/core/include)
        set(PYTHON3_PACKAGES_PATH       ${out_abs_path}/opencv-python)
        set(HAVE_opencv_python2         ON)
        set(BUILD_opencv_python2        ON)
        set(PYTHON2_INCLUDE_PATH        ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/include/python2.7)
        set(PYTHON2_LIBRARIES           ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/lib/libpython2.7.so)
        set(PYTHON2_NUMPY_INCLUDE_DIRS  ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/lib/local/python2.7/dist-packages/numpy/core/include)
        set(PYTHON2_EXECUTABLE          ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/bin/python2.7)
        set(PYTHON2_PACKAGES_PATH       ${out_abs_path}/opencv-python/python2)
        set(PYTHON_DEFAULT_EXECUTABLE   /usr/bin/python)
        set(OPENCV_SKIP_PYTHON_LOADER   ON)
        set(ENABLE_NEON                 ON)

        set(OPENCV_FORCE_3RDPARTY_BUILD OFF)
        set(JPEG_LIBRARY                ${CMAKE_CURRENT_SOURCE_DIR}/bm_opencv/3rdparty/libjpeg-turbo/binary/lib/soc/libturbojpeg.a)
        set(JPEG_INCLUDE_DIR            ${CMAKE_CURRENT_SOURCE_DIR}/bm_opencv/3rdparty/libjpeg-turbo/binary/include)
        set(FREETYPE_LIBRARY            ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/lib/libfreetype.a)
        set(FREETYPE_INCLUDE_DIRS       ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/include/freetype2)
        set(HARFBUZZ_INCLUDE_DIRS       ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/include/harfbuzz)
        set(CMAKE_TOOLCHAIN_FILE        ../platforms/linux/aarch64-gnu.toolchain.cmake)
        if("${GCC_VERSION}" STREQUAL "930")
            set(CMAKE_TOOLCHAIN_FILE        ../platforms/linux/aarch64-gnu.toolchain-930.cmake)
        elseif("${GCC_VERSION}" STREQUAL "1131")
            set(CMAKE_TOOLCHAIN_FILE        ../platforms/linux/aarch64-gnu.toolchain-1131.cmake)
        endif()

    elseif(${platform} STREQUAL "pcie")

        #        set(HAVE_opencv_python3         ON)
        #        set(BUILD_opencv_python3        ON)
        #        set(PYTHON3_INCLUDE_PATH        ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/x86_64/python3.5/include/python3.5m)
        #        set(PYTHON3_LIBRARIES           ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/x86_64/python3.5/lib/libpython3.5m.so)
        #        set(PYTHON3_EXECUTABLE          ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/x86_64/python3.5/bin)
        #        set(PYTHON3_NUMPY_INCLUDE_DIRS  ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/x86_64/python3.5/dist-packages/numpy/core/include)
        #        set(PYTHON3_PACKAGES_PATH       ${out_abs_path}/opencv-python)
        #        set(HAVE_opencv_python2         ON)
        #        set(BUILD_opencv_python2        ON)
        #        set(PYTHON2_INCLUDE_PATH        ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/x86_64/python2.7/include/python2.7)
        #        set(PYTHON2_LIBRARIES           ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/x86_64/python2.7/lib/libpython2.7.so)
        #        set(PYTHON2_NUMPY_INCLUDE_DIRS  ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/x86_64/python2.7/dist-packages/numpy/core/include)
        #        set(PYTHON2_EXECUTABLE          ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/x86_64/python2.7/bin)
        #        set(PYTHON2_PACKAGES_PATH       ${out_abs_path}/opencv-python/python2)
        #        set(PYTHON_DEFAULT_EXECUTABLE   /usr/bin/python)
        #        set(OPENCV_SKIP_PYTHON_LOADER   ON)
        #        set(ENABLE_NEON                 OFF)
        #
        #        set(OPENCV_FORCE_3RDPARTY_BUILD ON)
        #        set(JPEG_LIBRARY                ${CMAKE_CURRENT_SOURCE_DIR}/bm_opencv/3rdparty/libjpeg-turbo/binary/lib/pcie/libturbojpeg.a)
        #        set(JPEG_INCLUDE_DIR            ${CMAKE_CURRENT_SOURCE_DIR}/bm_opencv/3rdparty/libjpeg-turbo/binary/include)
        #        set(FREETYPE_LIBRARY            ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/x86_64/lib/libfreetype.a)
        #        set(FREETYPE_INCLUDE_DIRS       ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/include/freetype2)
        #        set(HARFBUZZ_INCLUDE_DIRS       ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/include/harfbuzz)
        #        set(CMAKE_TOOLCHAIN_FILE        ../platforms/linux/x86_64-gnu.toolchain.cmake)
        #
        #    elseif(${platform} STREQUAL "pcie_arm64")
        #
        #        set(HAVE_opencv_python3         ON)
        #        set(BUILD_opencv_python3        ON)
        #        set(PYTHON3_INCLUDE_PATH        ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/include/python3.5)
        #        set(PYTHON3_LIBRARIES           ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/lib/libpython3.5m.so)
        #        set(PYTHON3_EXECUTABLE          ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/bin/python3)
        #        set(PYTHON3_NUMPY_INCLUDE_DIRS  ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/lib/local/python3.5/dist-packages/numpy/core/include)
        #        set(PYTHON3_PACKAGES_PATH       ${out_abs_path}/opencv-python)
        #        set(HAVE_opencv_python2         ON)
        #        set(BUILD_opencv_python2        ON)
        #        set(PYTHON2_INCLUDE_PATH        ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/include/python2.7)
        #        set(PYTHON2_LIBRARIES           ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/lib/libpython2.7.so)
        #        set(PYTHON2_NUMPY_INCLUDE_DIRS  ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/lib/local/python2.7/dist-packages/numpy/core/include)
        #        set(PYTHON2_EXECUTABLE          ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/bin/python2.7)
        #        set(PYTHON2_PACKAGES_PATH       ${out_abs_path}/opencv-python/python2)
        #        set(PYTHON_DEFAULT_EXECUTABLE   /usr/bin/python)
        #        set(OPENCV_SKIP_PYTHON_LOADER   ON)
        #        set(WITH_ITT                    OFF)
        #        set(BUILD_ITT                   OFF)
        #        set(WITH_IPP                    OFF)
        #        set(ENABLE_NEON                 ON)
        #
        #        set(OPENCV_FORCE_3RDPARTY_BUILD ON)
        #        set(JPEG_LIBRARY                ${CMAKE_CURRENT_SOURCE_DIR}/bm_opencv/3rdparty/libjpeg-turbo/binary/lib/soc/libturbojpeg.a)
        #        set(JPEG_INCLUDE_DIR            ${CMAKE_CURRENT_SOURCE_DIR}/bm_opencv/3rdparty/libjpeg-turbo/binary/include)
        #        set(FREETYPE_LIBRARY            ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/lib/libfreetype.a)
        #        set(FREETYPE_INCLUDE_DIRS       ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/include/freetype2)
        #        set(HARFBUZZ_INCLUDE_DIRS       ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/include/harfbuzz)
        #        set(CMAKE_TOOLCHAIN_FILE        ../platforms/linux/aarch64-gnu.toolchain.cmake)
        #
        #    elseif(${platform} STREQUAL "pcie_mips64")
        #
        #        set(HAVE_opencv_python3         OFF)
        #        set(BUILD_opencv_python3        OFF)
        #        set(HAVE_opencv_python2         OFF)
        #        set(BUILD_opencv_python2        OFF)
        #        set(ENABLE_NEON                 OFF)
        #
        #        set(OPENCV_FORCE_3RDPARTY_BUILD ON)
        #        set(JPEG_LIBRARY                ${CMAKE_CURRENT_SOURCE_DIR}/bm_opencv/3rdparty/libjpeg-turbo/binary/lib/mips/libturbojpeg.a)
        #        set(JPEG_INCLUDE_DIR            ${CMAKE_CURRENT_SOURCE_DIR}/bm_opencv/3rdparty/libjpeg-turbo/binary/include)
        #        set(FREETYPE_LIBRARY            ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/mip_64/lib/libfreetype.a)
        #        set(FREETYPE_INCLUDE_DIRS       ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/include/freetype2)
        #        set(HARFBUZZ_INCLUDE_DIRS       ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/include/harfbuzz)
        #        set(CMAKE_TOOLCHAIN_FILE        ../platforms/linux/mips_64-gnu.toolchain.cmake)
        #
        #    elseif(${platform} STREQUAL "pcie_sw64")
        #        set(HAVE_opencv_python3         ON)
        #        set(BUILD_opencv_python3        ON)
        #        set(PYTHON3_INCLUDE_PATH        ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/sw_64/python3.7/include/python3.7m)
        #        set(PYTHON3_LIBRARIES           ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/sw_64/python3.7/lib/libpython3.7m.so)
        #        set(PYTHON3_EXECUTABLE          ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/sw_64/python3.7/bin)
        #        set(PYTHON3_NUMPY_INCLUDE_DIRS  ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/sw_64/python3.7/dist-packages/numpy/core/include)
        #        set(PYTHON3_PACKAGES_PATH       ${out_abs_path}/opencv-python)
        #    set(HAVE_opencv_python2         ON)
        #    set(BUILD_opencv_python2        ON)
        #        set(PYTHON2_INCLUDE_PATH        ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/sw_64/python2.7/include/python2.7)
        #        set(PYTHON2_LIBRARIES           ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/sw_64/python2.7/lib/libpython2.7.so)
        #        set(PYTHON2_NUMPY_INCLUDE_DIRS  ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/sw_64/python2.7/dist-packages/numpy/core/include)
        #        set(PYTHON2_EXECUTABLE          ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/sw_64/python2.7/bin)
        #        set(PYTHON2_PACKAGES_PATH       ${out_abs_path}/opencv-python/python2)
        #        set(PYTHON_DEFAULT_EXECUTABLE   /usr/bin/python)
        #        set(OPENCV_SKIP_PYTHON_LOADER   ON)
        #        set(ENABLE_NEON                 OFF)
        #
        #        set(OPENCV_FORCE_3RDPARTY_BUILD ON)
        #        set(FREETYPE_LIBRARY            ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/sw_64/lib/libfreetype.a)
        #        set(FREETYPE_INCLUDE_DIRS       ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/include/freetype2)
        #        set(HARFBUZZ_INCLUDE_DIRS       ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/include/harfbuzz)
        #        set(CMAKE_TOOLCHAIN_FILE        ../platforms/linux/sw_64.toolchain.cmake)
        #
        #    elseif(${platform} STREQUAL "pcie_loongarch64")
        #
        #        set(HAVE_opencv_python3         OFF)
        #        set(BUILD_opencv_python3        OFF)
        #        set(HAVE_opencv_python2         OFF)
        #        set(BUILD_opencv_python2        OFF)
        #        set(ENABLE_NEON                 OFF)
        #
        #        set(OPENCV_FORCE_3RDPARTY_BUILD ON)
        #        set(FREETYPE_LIBRARY            ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/loongarch64/lib/libfreetype.a)
        #        set(FREETYPE_INCLUDE_DIRS       ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/include/freetype2)
        #        set(HARFBUZZ_INCLUDE_DIRS       ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/include/harfbuzz)
        #        set(CMAKE_TOOLCHAIN_FILE        ../platforms/linux/loongarch64.toolchain.cmake)
        #   elseif(${platform} STREQUAL "pcie_riscv64")
        #
        #        set(HAVE_opencv_python3         OFF)
        #        set(BUILD_opencv_python3        OFF)
        #        set(HAVE_opencv_python2         OFF)
        #        set(BUILD_opencv_python2        OFF)
        #        set(ENABLE_NEON                 OFF)
        #
        #        set(OPENCV_FORCE_3RDPARTY_BUILD ON)
        #    set(FREETYPE_LIBRARY            ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/riscv64/lib/libfreetype.a)
        #    set(FREETYPE_INCLUDE_DIRS       ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/include/freetype2)
        #        set(HARFBUZZ_INCLUDE_DIRS       ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/include/harfbuzz)
        #        set(CMAKE_TOOLCHAIN_FILE        ../platforms/linux/riscv64.toolchain.cmake)
    endif()

    set(OPTION_LIST WITH_FFMPEG WITH_GSTREAMER WITH_GTK WITH_1394 WITH_V4L WITH_OPENCL WITH_CUDA WITH_LAPACK WITH_TBB
            BUILD_TBB WITH_TIFF BUILD_TIFF WITH_IPP ENABLE_NEON WITH_JPEG BUILD_JPEG CMAKE_MAKE_PROGRAMa OPENCV_GENERATE_PKGCONFIG
            HAVE_opencv_python3 CHIP SUBTYPE PRODUCTFORM ABI_FLAG OPENCV_EXTRA_MODULES_PATH OPENCV_ENABLE_NONFREE
            BUILD_opencv_python3 PYTHON3_INCLUDE_PATH PYTHON3_LIBRARIES PYTHON3_EXECUTABLE PYTHON3_NUMPY_INCLUDE_DIRS
            PYTHON3_PACKAGES_PATH HAVE_opencv_python2 BUILD_opencv_python2 PYTHON2_INCLUDE_PATH PYTHON2_LIBRARIES
            PYTHON2_NUMPY_INCLUDE_DIRS PYTHON2_EXECUTABLE PYTHON2_PACKAGES_PATH PYTHON_DEFAULT_EXECUTABLE
            OPENCV_SKIP_PYTHON_LOADER JPEG_LIBRARY JPEG_INCLUDE_DIR FREETYPE_LIBRARY FREETYPE_INCLUDE_DIRS
            HARFBUZZ_INCLUDE_DIRS CMAKE_TOOLCHAIN_FILE OPENCV_FORCE_3RDPARTY_BUILD CMAKE_BUILD_TYPE)

    foreach(opt ${OPTION_LIST})
        if (DEFINED ${opt})
            list(APPEND OPENCV_CMAKE_OPTION -D${opt}=${${opt}})
        endif()
    endforeach()

    list(APPEND OPENCV_CMAKE_OPTION -DFFMPEG_INCLUDE_DIRS="${FFMPEG_INCLUDE_DIRS}")
    list(APPEND OPENCV_CMAKE_OPTION -DFFMPEG_LIBRARY_DIRS="${FFMPEG_LIBRARY_DIRS}")

endmacro(SET_OPENCV_ENV)

function(ADD_TARGET_OPENCV_LIB target_name chip_name subtype platform enable_abi0 enable_ocv_contrib
        enable_bmcpu vpu_abs_path jpu_abs_path yuv_abs_path
        bmcv_abs_path vpp_abs_path ion_abs_path ffmpeg_abs_path component out_abs_path)

    set(OPENCV_VERSION  ${PROJECT_VERSION})
    set(OPENCV_OUT_REL_PATH ${out_abs_path})
    set(OPENCV_LIB_TARGET ${OPENCV_OUT_REL_PATH}/lib)
    set(OPENCV_BIN_TARGET ${OPENCV_OUT_REL_PATH}/bin)
    set(OPENCV_TEST_TARGET ${OPENCV_OUT_REL_PATH}/test)
    set(OPENCV_PYTHON_TARGET ${OPENCV_OUT_REL_PATH}/opencv-python)
    set(OPENCV_HEADER_TARGET ${OPENCV_OUT_REL_PATH}/include)
    set(OPENCV_SHARE_TARGET ${OPENCV_OUT_REL_PATH}/share)

    SET_OPENCV_ENV(${chip_name} ${subtype} ${platform} ${enable_abi0} ${enable_ocv_contrib} ${vpu_abs_path}
        ${jpu_abs_path} ${yuv_abs_path} ${bmcv_abs_path} ${vpp_abs_path} ${ion_abs_path} ${ffmpeg_abs_path} ${out_abs_path})

    #if (${enable_abi0})
    #    set(LOCATION_PREFIX sophon-opencv_${OPENCV_VERSION})  # in centos, put on same directory to ubuntu
    #else()
    set(LOCATION_PREFIX sophon-opencv_${OPENCV_VERSION})
    #endif()

    if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        set(MAKE_TARGET     install)
    else()
        set(MAKE_TARGET     install/strip)
    endif()

    #if (${enable_bmcpu})
    #    set(ENABLE_BMCPU 1)
    #else()
    set(ENABLE_BMCPU 0)
    #endif()

    add_custom_command(OUTPUT ${OPENCV_LIB_TARGET} ${OPENCV_TEST_TARGET} ${OPENCV_HEADER_TARGET} ${OPENCV_PYTHON_TARGET}
            ${OPENCV_BIN_TARGET} ${OPENCV_SHARE_TARGET}
        COMMAND ${CMAKE_COMMAND} -E make_directory ./build
        COMMAND rm -rf ./build/*
        COMMAND ${CMAKE_COMMAND} -E chdir ./build ${CMAKE_COMMAND} -DCMAKE_INSTALL_PREFIX=${OPENCV_OUT_REL_PATH} -DENABLE_BMCPU=${ENABLE_BMCPU} ${OPENCV_CMAKE_OPTION} ..
        COMMAND ${CMAKE_COMMAND} -E chdir ./build make -j`nproc` ${MAKE_TARGET}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${OPENCV_OUT_REL_PATH}/test/
        COMMAND ${CMAKE_COMMAND} -E copy ./build/bin/opencv_test_* ${OPENCV_OUT_REL_PATH}/test/
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bm_opencv
    )

    add_custom_target (${target_name} ALL DEPENDS ${OPENCV_LIB_TARGET})
    set_property(TARGET ${target_name}
                APPEND
                PROPERTY ADDITIONAL_CLEAN_FILES ${CMAKE_CURRENT_SOURCE_DIR}/bm_opencv/build)
    install(DIRECTORY ${OPENCV_LIB_TARGET}/
        DESTINATION ${LOCATION_PREFIX}/lib
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        COMPONENT ${component}
        PATTERN "cmake" EXCLUDE
        PATTERN "pkgconfig" EXCLUDE
        PATTERN "*.a" EXCLUDE)
    install(DIRECTORY ${OPENCV_BIN_TARGET}/
        DESTINATION ${LOCATION_PREFIX}/bin
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        COMPONENT ${component})
    install(DIRECTORY ${OPENCV_TEST_TARGET}/
        DESTINATION ${LOCATION_PREFIX}/test
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        COMPONENT ${component})


    if ((${HAVE_opencv_python3} STREQUAL "ON") OR (${HAVE_opencv_python2} STREQUAL "ON"))
       install(DIRECTORY ${OPENCV_PYTHON_TARGET}/
         DESTINATION ${LOCATION_PREFIX}/opencv-python
         FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
         COMPONENT ${component})
    endif()

    install(DIRECTORY ${OPENCV_LIB_TARGET}/
        DESTINATION ${LOCATION_PREFIX}/lib
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        COMPONENT ${component}-dev
        FILES_MATCHING PATTERN "*.a"
        PATTERN "cmake" EXCLUDE
        PATTERN "pkgconfig" EXCLUDE)
    install(DIRECTORY ${OPENCV_HEADER_TARGET}/
        DESTINATION ${LOCATION_PREFIX}/include
        COMPONENT ${component}-dev)
    install(DIRECTORY ${OPENCV_SHARE_TARGET}/
        DESTINATION ${LOCATION_PREFIX}/share
        COMPONENT ${component}-dev)

    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian_script/02_sophon-opencv.conf
        DESTINATION ${LOCATION_PREFIX}/data
        COMPONENT ${component})
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian_script/sophon-opencv-autoconf.sh
        DESTINATION ${LOCATION_PREFIX}/data
        COMPONENT ${component})
    install(DIRECTORY ${OPENCV_LIB_TARGET}/cmake ${OPENCV_LIB_TARGET}/pkgconfig
        DESTINATION ${LOCATION_PREFIX}/lib
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        COMPONENT ${component}-dev)

endfunction(ADD_TARGET_OPENCV_LIB)


#function(ADD_TARGET_OPENCV_BMCPU target_name chip_name subtype enable_abi0 enable_ocv_contrib
#        vpu_abs_path jpu_abs_path yuv_abs_path
#        bmcv_abs_path vpp_abs_path ion_abs_path ffmpeg_abs_path component out_abs_path)
#
#    set(OPENCV_BMCPU_TARGET ${out_abs_path}/lib/libopencv_world.so.4.1)
#    set(OPENCV_WRAPPER_TARGET ${out_abs_path}/lib/libcvwrapper.so)
#
#    SET_OPENCV_ENV(${chip_name} ${subtype} soc ${enable_abi0} ${enable_ocv_contrib} ${vpu_abs_path}
#        ${jpu_abs_path} ${yuv_abs_path} ${bmcv_abs_path} ${vpp_abs_path} ${ion_abs_path} ${ffmpeg_abs_path} ${out_abs_path})
#
#    add_custom_command(OUTPUT ${OPENCV_BMCPU_TARGET}
#        COMMAND ${CMAKE_COMMAND} -E make_directory ./build_bmcpu
#        COMMAND rm -rf ./build_bmcpu/*
#        COMMAND ${CMAKE_COMMAND} -E chdir ./build_bmcpu ${CMAKE_COMMAND}
#            -DCMAKE_INSTALL_PREFIX=${out_abs_path}
#            -DENABLE_BMCPU=1 -DBUILD_opencv_apps=OFF
#            -DBUILD_EXAMPLES=OFF -DBUILD_TESTS=OFF -DBUILD_PERF_TESTS=OFF
#            ${OPENCV_CMAKE_OPTION} ..
#        COMMAND ${CMAKE_COMMAND} -E chdir ./build_bmcpu make -j`nproc` install/strip
#        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bm_opencv)
#
#    add_custom_command(OUTPUT ${OPENCV_WRAPPER_TARGET}
#        COMMAND make OPENCV_OUTPUT_DIR=${out_abs_path}
#            BMCV_OUTPUT_DIR=${bmcv_abs_path}
#            FFMPEG_OUTPUT_DIR=${ffmpeg_abs_path}/usr/local
#            BMLIB_DIR=${CMAKE_CURRENT_SOURCE_DIR}/bm_opencv/3rdparty/libbmcv
#        COMMAND cp -apf libcvwrapper.so* ${out_abs_path}/lib/
#        COMMAND make clean
#        DEPENDS ${OPENCV_BMCPU_TARGET}
#        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/cvwrapper)
#
#    add_custom_target (${target_name} ALL DEPENDS ${OPENCV_BMCPU_TARGET} ${OPENCV_WRAPPER_TARGET})
#    set_property(TARGET ${target_name} APPEND PROPERTY ADDITIONAL_CLEAN_FILES ${CMAKE_CURRENT_SOURCE_DIR}/bm_opencv/build_bmcpu/)
#
#    set(FF_LIBS libavcodec.so.58 libavformat.so.58 libavfilter.so.7 libavutil.so.56
#                libswresample.so.3 libswscale.so.5 libavdevice.so.58)
#    set(JPU_LIBS libbmjpuapi.so libbmjpulite.so)
#    set(VPU_LIBS libbmvideo.so libbmvpuapi.so libbmvpulite.so)
#    set(ION_LIBS libbmion.so)
#    set(VPP_LIBS libbmvppapi.so)
#    set(YUV_LIBS libyuv.so)
#    set(BMCV_LIBS libbmcv.so)
#    set(THIRD_PARTY_LIBS libbmlib.so)
#
#    set(OPENCV_VERSION ${PROJECT_VERSION})
#    STRING(REGEX REPLACE ".+/(.+)\\..*" "\\1" OPENCV_WORLD_LIB_FILENAME ${OPENCV_BMCPU_TARGET})
#    STRING(REGEX REPLACE ".+/(.+)\\..*" "\\1" OPENCV_WRAPPER_LIB_FILENAME ${OPENCV_WRAPPER_TARGET})
#    install(DIRECTORY ${out_abs_path}/lib/
#        DESTINATION opencv-bmcpu_${OPENCV_VERSION}
#        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
#        COMPONENT ${component}
#        FILES_MATCHING
#        PATTERN ${OPENCV_WORLD_LIB_FILENAME}*
#        PATTERN "cmake" EXCLUDE
#        PATTERN "pkgconfig" EXCLUDE)
#    install(DIRECTORY ${out_abs_path}/lib/
#        DESTINATION opencv-bmcpu_${OPENCV_VERSION}
#        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
#        COMPONENT ${component}
#        FILES_MATCHING
#        PATTERN ${OPENCV_WRAPPER_LIB_FILENAME}*
#        PATTERN "cmake" EXCLUDE
#        PATTERN "pkgconfig" EXCLUDE)
#
#    list(APPEND install_libs FF_LIBS JPU_LIBS VPU_LIBS ION_LIBS VPP_LIBS YUV_LIBS BMCV_LIBS THIRD_PARTY_LIBS)
#    list(APPEND install_src_paths ${ffmpeg_abs_path}/usr/local/lib/ ${jpu_abs_path}/lib/ ${vpu_abs_path}/lib/
#        ${ion_abs_path}/lib/ ${vpp_abs_path}/lib/ ${yuv_abs_path}/lib/ ${bmcv_abs_path}/lib/
#        ${CMAKE_CURRENT_SOURCE_DIR}/bm_opencv/3rdparty/libbmcv/lib/soc/)
#    list(LENGTH install_libs len1)
#    math(EXPR len2 "${len1} - 1")
#
#    foreach(index RANGE ${len2})
#        list(GET install_libs ${index} install_lib)
#        list(GET install_src_paths ${index} install_path)
#
#        UNSET(MATCH_LIBS)
#        foreach(f ${${install_lib}})
#            SET(MATCH_LIBS ${MATCH_LIBS} PATTERN ${f}*)
#        endforeach()
#        install(DIRECTORY ${install_path}/
#            DESTINATION opencv-bmcpu_${OPENCV_VERSION}
#            FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
#            COMPONENT ${component}
#            FILES_MATCHING ${MATCH_LIBS}
#            PATTERN "cmake" EXCLUDE
#            PATTERN "pkgconfig" EXCLUDE)
#    endforeach()
#
#endfunction(ADD_TARGET_OPENCV_BMCPU)
#
#function(ADD_TARGET_SAMPLE_BIN target_name chip_name subtype platform enable_abi0
#        vpu_abs_path jpu_abs_path yuv_abs_path bmcv_abs_path vpp_abs_path
#        ion_abs_path ffmpeg_abs_path opencv_abs_path component out_abs_path)
#
#    set(SAMPLE_VERSION ${PROJECT_VERSION})
#    set(LOCATION_PREFIX sophon-sample_${SAMPLE_VERSION})
#    set(BMMW_SAMPLES_TARGET
#        water.bin
#        test_ff_video_encode
#        test_ocv_jpumulti
#        test_ocv_vidmulti
#        test_bm_restart
#        test_ocv_vidbasic
#        test_ff_bmcv_transcode
#        test_ocv_jpubasic
#        test_ff_video_xcode
#        test_ocv_video_xcode
#        test_ff_scale_transcode)
#    set(BMMW_SAMPLES_SOURCE
#        water.bin
#        ocv_vidmulti
#        ocv_jpumulti
#        ocv_vidbasic
#        ocv_jpubasic
#        ocv_video_xcode
#        ff_video_decode
#        ff_video_encode
#        ff_bmcv_transcode
#        ff_video_xcode
#        ff_scale_transcode)
#
#    list(TRANSFORM BMMW_SAMPLES_TARGET PREPEND ${out_abs_path}/bin/)
#    set(BMMW_SAMPLES_SOURCE_DIR ${BMMW_SAMPLES_SOURCE})
#    list(TRANSFORM BMMW_SAMPLES_SOURCE_DIR PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/samples/)
#
#    if(NOT "${chip_name}" STREQUAL "bm1684")
#        message(WARNING "For ${chip_name}, ignore to build samples")
#        return()
#    endif()
#
#    if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
#        set(debug 1)
#    else()
#        set(debug 0)
#    endif()
#
#    if (${enable_abi0})
#        set(ABI_FLAG 0)
#    else()
#        set(ABI_FLAG 1)
#    endif()
#
#    set(MAKE_OPT CHIP=${chip_name}
#                SUBTYPE=${subtype}
#                DEBUG=${debug}
#                PRODUCTFORM=${platform}
#                FF_SDK_DIR=${ffmpeg_abs_path}/usr/local
#                OCV_SDK_DIR=${opencv_abs_path}
#                BMCV_SDK_DIR=${bmcv_abs_path}
#                BMLIB_DIR=${CMAKE_CURRENT_SOURCE_DIR}/bm_opencv/3rdparty/libbmcv
#                VPU_SDK_DIR=${vpu_abs_path}
#                JPU_SDK_DIR=${jpu_abs_path}
#                YUV_SDK_DIR=${yuv_abs_path}
#                VPP_SDK_DIR=${vpp_abs_path}
#                ION_SDK_DIR=${ion_abs_path}
#                INSTALL_DIR=${out_abs_path}
#                ABI_FLAG=${ABI_FLAG})
#    add_custom_command(OUTPUT ${BMMW_SAMPLES_TARGET}
#        COMMAND make ${MAKE_OPT} clean
#        COMMAND make ${MAKE_OPT}
#        COMMAND make ${MAKE_OPT} install
#        COMMAND make ${MAKE_OPT} clean
#        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/samples)
#
#    add_custom_target (${target_name} ALL
#        DEPENDS ${BMMW_SAMPLES_TARGET})
#
#    install(FILES ${BMMW_SAMPLES_TARGET}
#        DESTINATION ${LOCATION_PREFIX}/bin
#        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
#        COMPONENT ${component})
#    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian_script/sophon-sample-autoconf.sh
#        DESTINATION ${LOCATION_PREFIX}/data
#        COMPONENT ${component})
#
#    list(LENGTH BMMW_SAMPLES_SOURCE_DIR size)
#    math(EXPR size_minus_1 "${size} - 1")
#
#    foreach(index RANGE ${size_minus_1})
#        list(GET BMMW_SAMPLES_SOURCE ${index} src_dir_name)
#        list(GET BMMW_SAMPLES_SOURCE_DIR ${index} abs_dir_name)
#
#        FILE(GLOB srcs ${abs_dir_name}/*.cpp ${abs_dir_name}/*.c ${abs_dir_name}/*.h ${abs_dir_name}/*.hpp)
#        UNSET(INSTALL_SRCS)
#        foreach(f ${srcs})
#            get_filename_component(real_f ${f} REALPATH)
#            list(APPEND INSTALL_SRCS ${real_f})
#        endforeach()
#
#        install(FILES ${INSTALL_SRCS}
#            DESTINATION ${LOCATION_PREFIX}/samples/${src_dir_name}
#            PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ GROUP_WRITE WORLD_READ WORLD_WRITE
#            COMPONENT ${component})
#
#        install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/samples/CMakeLists.template
#            DESTINATION ${LOCATION_PREFIX}/samples/${src_dir_name}
#            PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ GROUP_WRITE WORLD_READ WORLD_WRITE
#            COMPONENT ${component}
#            RENAME CMakeLists.txt)
#    endforeach()
#
#endfunction(ADD_TARGET_SAMPLE_BIN)
#
#function(ADD_TARGET_MW_DOC target_name)
#
#    set(BMMW_DOC_TARGET_CN ${CMAKE_BINARY_DIR}/doc/MULTIMEDIA使用手册.pdf)
#    set(BMMW_FAQ_TARGET_CN ${CMAKE_BINARY_DIR}/doc/MULTIMEDIA常见问题手册.pdf)
#    set(BMMW_MAN_TARGET_CN ${CMAKE_BINARY_DIR}/doc/MULTIMEDIA开发参考手册.pdf)
#
#    set(BMMW_DOC_TARGET_EN ${CMAKE_BINARY_DIR}/doc/Multimedia\ User\ Guide.pdf)
#    set(BMMW_FAQ_TARGET_EN ${CMAKE_BINARY_DIR}/doc/Multimedia\ FAQ.pdf)
#    set(BMMW_MAN_TARGET_EN ${CMAKE_BINARY_DIR}/doc/Multimedia\ Technical\ Reference\ Manual.pdf)
#
#    add_custom_command(OUTPUT ${BMMW_DOC_TARGET_CN} ${BMMW_FAQ_TARGET_CN} ${BMMW_MAN_TARGET_CN} ${BMMW_DOC_TARGET_EN} ${BMMW_FAQ_TARGET_EN} ${BMMW_MAN_TARGET_EN}
#        COMMAND rm -rf ./build_faq ./build_guide ./build_manual
#        COMMAND make LANG=en html
#        COMMAND make LANG=en pdf
#        COMMAND make LANG=zh html
#        COMMAND make LANG=zh pdf
#        COMMAND ${CMAKE_COMMAND} -E copy_if_different ./Multimedia_Manual_zh.pdf ${BMMW_DOC_TARGET_CN}
#        COMMAND ${CMAKE_COMMAND} -E copy_if_different ./Multimedia_FAQ_zh.pdf ${BMMW_FAQ_TARGET_CN}
#        COMMAND ${CMAKE_COMMAND} -E copy_if_different ./Multimedia_Guide_zh.pdf ${BMMW_MAN_TARGET_CN}
#        COMMAND ${CMAKE_COMMAND} -E copy_if_different ./Multimedia_Manual_en.pdf ${BMMW_DOC_TARGET_EN}
#        COMMAND ${CMAKE_COMMAND} -E copy_if_different ./Multimedia_FAQ_en.pdf ${BMMW_FAQ_TARGET_EN}
#        COMMAND ${CMAKE_COMMAND} -E copy_if_different ./Multimedia_Guide_en.pdf ${BMMW_MAN_TARGET_EN}
#        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/document)
#
#    add_custom_target (${target_name}
#        DEPENDS ${BMMW_DOC_TARGET_CN} ${BMMW_FAQ_TARGET_CN} ${BMMW_MAN_TARGET_CN} ${BMMW_DOC_TARGET_EN} ${BMMW_FAQ_TARGET_EN} ${BMMW_MAN_TARGET_EN})
#
#endfunction(ADD_TARGET_MW_DOC)



