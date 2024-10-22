# determine the version number from the #define in libyuv/version.h

#if (${PLATFORM} STREQUAL "pcie")
#    set(CMAKE_SYSTEM_PROCESSOR x86_64)
#    set(ARCH "amd64")
#    set(MW_POSTFIX "")
#elseif (${PLATFORM} STREQUAL "soc")
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(ARCH "arm64")
set(MW_POSTFIX "-soc")
#elseif (${PLATFORM} STREQUAL "pcie_arm64")
#    set(CMAKE_SYSTEM_PROCESSOR aarch64)
#    set(ARCH "arm64")
#    set(MW_POSTFIX "")
#elseif (${PLATFORM} STREQUAL "pcie_mips64")
#    set(CMAKE_SYSTEM_PROCESSOR mips64)
#    set(ARCH "mips64")
#    set(MW_POSTFIX "")
#elseif (${PLATFORM} STREQUAL "pcie_loongarch64")
#    set(CMAKE_SYSTEM_PROCESSOR loongarch64)
#    set(ARCH "loongarch64")
#    set(MW_POSTFIX "")
#elseif (${PLATFORM} STREQUAL "pcie_sw64")
#    set(CMAKE_SYSTEM_PROCESSOR sw_64)
#    set(ARCH "sw64")
#    set(MW_POSTFIX "")
#elseif (${PLATFORM} STREQUAL "pcie_riscv64")
#    set(CMAKE_SYSTEM_PROCESSOR riscv_64)
#    set(ARCH "riscv64")
#    set(MW_POSTFIX "")
#else()
#    message(FATAL_ERROR "Unknown processor:" ${CMAKE_SYSTEM_PROCESSOR})
#endif()

SET(FFMPEG_VER_MAJOR    ${MAJOR_VERSION})
SET(FFMPEG_VER_MINOR    ${MINOR_VERSION})
SET(FFMPEG_VER_PATCH    ${PATCH_VERSION})
SET(OPENCV_VER_MAJOR    ${MAJOR_VERSION})
SET(OPENCV_VER_MINOR    ${MINOR_VERSION})
SET(OPENCV_VER_PATCH    ${PATCH_VERSION})
SET(GSTREAMER_VER_MAJOR ${MAJOR_VERSION})
SET(GSTREAMER_VER_MINOR ${MINOR_VERSION})
SET(GSTREAMER_VER_PATCH ${PATCH_VERSION})
SET(SAMPLE_VER_MAJOR    ${MAJOR_VERSION})
SET(SAMPLE_VER_MINOR    ${MINOR_VERSION})
SET(SAMPLE_VER_PATCH    ${PATCH_VERSION})
SET(FFMPEG_VERSION      ${FFMPEG_VER_MAJOR}.${FFMPEG_VER_MINOR}.${FFMPEG_VER_PATCH})
SET(OPENCV_VERSION      ${OPENCV_VER_MAJOR}.${OPENCV_VER_MINOR}.${OPENCV_VER_PATCH})
SET(GSTREAMER_VERSION   ${GSTREAMER_VER_MAJOR}.${GSTREAMER_VER_MINOR}.${GSTREAMER_VER_PATCH})
SET(SAMPLE_VERSION      ${SAMPLE_VER_MAJOR}.${SAMPLE_VER_MINOR}.${SAMPLE_VER_PATCH})
SET(COMPATIBLE_VERSION  0.4.4)

include (InstallRequiredSystemLibraries)

# define all the variables needed by CPack to create .deb and .rpm packages
#tgz cpack
SET(CPACK_PACKAGE_VENDOR               "SophGO Inc.")
SET(CPACK_PACKAGE_CONTACT              "supporter@sophgo.com")
SET(CPACK_PACKAGE_VERSION               ${OPENCV_VERSION})
SET(CPACK_PACKAGE_VERSION_MAJOR         ${OPENCV_VER_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR         ${OPENCV_VER_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH         ${OPENCV_VER_PATCH})
#SET(CPACK_RESOURCE_FILE_LICENSE         ${PROJECT_SOURCE_DIR}/LICENSE )
SET(CPACK_PACKAGE_NAME                  ${PROJECT_NAME})
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY   "Sophon multimedia library" )
SET(CPACK_PACKAGE_DESCRIPTION           "Sophon multimedia library: sophon-ffmpeg sophon-opencv sophon-gstreamer" )
SET(CPACK_PACKAGE_FILE_NAME             ${PROJECT_NAME}${MW_POSTFIX}_${OPENCV_VERSION}_${CMAKE_SYSTEM_PROCESSOR})
SET(CPACK_PACKAGING_INSTALL_PREFIX      "/opt/sophon")
#message(STATUS "CPACK_PACKAGE_FILE_NAME............... " ${CPACK_PACKAGE_FILE_NAME})

if(${ENABLE_ABI0})
    #    SET(CPACK_GENERATOR             "RPM")
    #
    #    # RPM config
    #    SET(CPACK_RPM_COMPONENT_INSTALL      YES)
    #    SET(CPACK_RPM_PACKAGE_NAME           "sophon-mw${MW_POSTFIX}")
    #    SET(CPACK_RPM_PACKAGE_ARCHITECTURE   ${ARCH})
    #    SET(CPACK_RPM_FILE_NAME              RPM-DEFAULT)
    #    SET(CPACK_RPM_PACKAGE_VERSION        ${CPACK_PACKAGE_VERSION})
    #    SET(CPACK_RPM_PACKAGE_AUTOREQPROV    OFF)
    #    SET(CPACK_RPM_COMPONENT_INSTALL      ON)
    #
    #    SET(CPACK_RPM_SOPHON-FFMPEG_FILE_NAME ${CPACK_RPM_PACKAGE_NAME}-sophon-ffmpeg_${CPACK_RPM_PACKAGE_VERSION}_${CPACK_RPM_PACKAGE_ARCHITECTURE}.rpm)
    #    SET(CPACK_RPM_SOPHON-FFMPEG-DEV_FILE_NAME ${CPACK_RPM_PACKAGE_NAME}-sophon-ffmpeg-dev_${CPACK_RPM_PACKAGE_VERSION}_${CPACK_RPM_PACKAGE_ARCHITECTURE}.rpm)
    #    SET(CPACK_RPM_SOPHON-OPENCV-ABI0_FILE_NAME ${CPACK_RPM_PACKAGE_NAME}-sophon-opencv-abi0_${CPACK_RPM_PACKAGE_VERSION}_${CPACK_RPM_PACKAGE_ARCHITECTURE}.rpm)
    #    SET(CPACK_RPM_SOPHON-OPENCV-ABI0-DEV_FILE_NAME ${CPACK_RPM_PACKAGE_NAME}-sophon-opencv-abi0-dev_${CPACK_RPM_PACKAGE_VERSION}_${CPACK_RPM_PACKAGE_ARCHITECTURE}.rpm)
    #    SET(CPACK_RPM_SOPHON-SAMPLE_FILE_NAME ${CPACK_RPM_PACKAGE_NAME}-sophon-sample_${CPACK_RPM_PACKAGE_VERSION}_${CPACK_RPM_PACKAGE_ARCHITECTURE}.rpm)
    #
    #
    #    # RPM dependency
    #    SET(CPACK_RPM_SOPHON-FFMPEG_PACKAGE_REQUIRES          "sophon-libsophon >= ${COMPATIBLE_VERSION}")
    #    SET(CPACK_RPM_SOPHON-FFMPEG-DEV_PACKAGE_REQUIRES      "${CPACK_RPM_PACKAGE_NAME}-sophon-ffmpeg = ${CPACK_RPM_PACKAGE_VERSION}")
    #    SET(CPACK_RPM_SOPHON-OPENCV-ABI0_PACKAGE_REQUIRES     "${CPACK_RPM_PACKAGE_NAME}-sophon-ffmpeg = ${CPACK_RPM_PACKAGE_VERSION}")
    #    SET(CPACK_RPM_SOPHON-OPENCV-ABI0-DEV_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_NAME}-sophon-opencv-abi0 = ${CPACK_RPM_PACKAGE_VERSION}")
    #    SET(CPACK_RPM_SOPHON-SAMPLE_PACKAGE_REQUIRES          "${CPACK_RPM_PACKAGE_NAME}-sophon-ffmpeg >= ${CPACK_RPM_PACKAGE_VERSION}, ${CPACK_RPM_PACKAGE_NAME}-sophon-opencv-abi0 >= ${CPACK_RPM_PACKAGE_VERSION}")
    #
    #    SET(CPACK_RPM_SOPHON-FFMPEG_POST_INSTALL_SCRIPT_FILE   ${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian_script/sophon-ffmpeg/postinst)
    #    SET(CPACK_RPM_SOPHON-FFMPEG_PRE_UNINSTALL_SCRIPT_FILE  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian_script/sophon-ffmpeg/prerm)
    #    SET(CPACK_RPM_SOPHON-FFMPEG_POST_UNINSTALL_SCRIPT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian_script/sophon-ffmpeg/postrm)
    #
    #    SET(CPACK_RPM_SOPHON-FFMPEG-DEV_POST_INSTALL_SCRIPT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian_script/sophon-ffmpeg-devel/postinst)
    #
    #    SET(CPACK_RPM_SOPHON-OPENCV-ABI0_POST_INSTALL_SCRIPT_FILE   ${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian_script/sophon-opencv/postinst)
    #    SET(CPACK_RPM_SOPHON-OPENCV-ABI0_PRE_UNINSTALL_SCRIPT_FILE  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian_script/sophon-opencv/prerm)
    #    SET(CPACK_RPM_SOPHON-OPENCV-ABI0_POST_UNINSTALL_SCRIPT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian_script/sophon-opencv/postrm)
    #
    #    SET(CPACK_RPM_SOPHON-OPENCV-ABI0-DEV_POST_INSTALL_SCRIPT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian_script/sophon-opencv-devel/postinst)
    #
    #    SET(CPACK_RPM_SOPHON-SAMPLE_POST_INSTALL_SCRIPT_FILE   ${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian_script/sophon-sample/postinst)
    #    SET(CPACK_RPM_SOPHON-SAMPLE_PRE_UNINSTALL_SCRIPT_FILE  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian_script/sophon-sample/prerm)
    #    SET(CPACK_RPM_SOPHON-SAMPLE_POST_UNINSTALL_SCRIPT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian_script/sophon-sample/postrm)
else()
    SET(CPACK_GENERATOR                     "DEB;TGZ")

    # DEB config
    SET(CPACK_DEB_COMPONENT_INSTALL         YES)
    SET(CPACK_COMPONENTS_GROUPING           IGNORE)
    SET(CPACK_DEBIAN_PACKAGE_NAME           "${PROJECT_NAME}${MW_POSTFIX}")
    SET(CPACK_DEBIAN_PACKAGE_MAINTAINER     "sophon team")
    SET(CPACK_DEBIAN_PACKAGE_ARCHITECTURE   ${ARCH})
    SET(CPACK_DEBIAN_FILE_NAME              DEB-DEFAULT)
    SET(CPACK_DEBIAN_PACKAGE_VERSION        ${CPACK_PACKAGE_VERSION})
    #message(STATUS "CPACK_DEB_COMPONENT_INSTALL............... " ${CPACK_DEB_COMPONENT_INSTALL})
    #message(STATUS "CPACK_DEBIAN_PACKAGE_NAME............... " ${CPACK_DEBIAN_PACKAGE_NAME})
    #message(STATUS "CPACK_DEBIAN_PACKAGE_ARCHITECTURE............... " ${CPACK_DEBIAN_PACKAGE_ARCHITECTURE})
    #message(STATUS "CPACK_DEBIAN_FILE_NAME............... " ${CPACK_DEBIAN_FILE_NAME})
    #message(STATUS "CPACK_DEBIAN_PACKAGE_VERSION............... " ${CPACK_DEBIAN_PACKAGE_VERSION})

    # DEB dependency
    SET(CPACK_DEBIAN_SOPHON-FFMPEG_PACKAGE_DEPENDS          "sophon${MW_POSTFIX}-libsophon (>= ${COMPATIBLE_VERSION}), libc6 (>= 2.19)")
    SET(CPACK_DEBIAN_SOPHON-FFMPEG-DEV_PACKAGE_DEPENDS      "${CPACK_DEBIAN_PACKAGE_NAME}-sophon-ffmpeg (= ${CPACK_DEBIAN_PACKAGE_VERSION})")
    SET(CPACK_DEBIAN_SOPHON-OPENCV_PACKAGE_DEPENDS          "${CPACK_DEBIAN_PACKAGE_NAME}-sophon-ffmpeg (>= ${CPACK_DEBIAN_PACKAGE_VERSION}), libatomic1(>= 4.8), libc6 (>= 2.23), libstdc++6 (>= 5.4.0-6), zlib1g (>= 1:1.1.4)")
    SET(CPACK_DEBIAN_SOPHON-OPENCV-DEV_PACKAGE_DEPENDS      "${CPACK_DEBIAN_PACKAGE_NAME}-sophon-opencv (= ${CPACK_DEBIAN_PACKAGE_VERSION})")
    SET(CPACK_DEBIAN_SOPHON-GSTREAMER_PACKAGE_DEPENDS       "sophon${MW_POSTFIX}-libsophon (>= ${COMPATIBLE_VERSION})")
    SET(CPACK_DEBIAN_SOPHON-GSTREAMER-DEV_PACKAGE_DEPENDS   "${CPACK_DEBIAN_PACKAGE_NAME}-sophon-gstreamer (= ${CPACK_DEBIAN_PACKAGE_VERSION})")
    SET(CPACK_DEBIAN_SOPHON-SAMPLE_PACKAGE_DEPENDS          "${CPACK_DEBIAN_PACKAGE_NAME}-sophon-ffmpeg (>= ${CPACK_DEBIAN_PACKAGE_VERSION}), ${CPACK_DEBIAN_PACKAGE_NAME}-sophon-opencv (>= ${CPACK_DEBIAN_PACKAGE_VERSION})")

    SET(CPACK_DEBIAN_SOPHON-FFMPEG_PACKAGE_CONTROL_EXTRA
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian_script/sophon-ffmpeg/postinst
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian_script/sophon-ffmpeg/prerm
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian_script/sophon-ffmpeg/postrm)

    SET(CPACK_DEBIAN_SOPHON-FFMPEG-DEV_PACKAGE_CONTROL_EXTRA
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian_script/sophon-ffmpeg-devel/postinst)

    SET(CPACK_DEBIAN_SOPHON-OPENCV_PACKAGE_CONTROL_EXTRA
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian_script/sophon-opencv/postinst
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian_script/sophon-opencv/prerm
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian_script/sophon-opencv/postrm)

    SET(CPACK_DEBIAN_SOPHON-OPENCV-DEV_PACKAGE_CONTROL_EXTRA
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian_script/sophon-opencv-devel/postinst)

    SET(CPACK_DEBIAN_SOPHON-GSTREAMER_PACKAGE_CONTROL_EXTRA
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian_script/sophon-gstreamer/postinst
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian_script/sophon-gstreamer/prerm
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian_script/sophon-gstreamer/postrm)

    #SET(CPACK_DEBIAN_SOPHON-GSTREAMER-DEV_PACKAGE_CONTROL_EXTRA
    #    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian_script/sophon-gstreamer-devel/postinst)

    SET(CPACK_DEBIAN_SOPHON-SAMPLE_PACKAGE_CONTROL_EXTRA
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian_script/sophon-sample/postinst
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian_script/sophon-sample/prerm
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian_script/sophon-sample/postrm)
endif()

# remove unspecified component introduced by libyuv

string(REPLACE "/" "\\\/" REPLACE_CPACK_INSTALL_PREFIX ${CPACK_PACKAGING_INSTALL_PREFIX})
add_custom_target(FFMPEG_DEB_SCRIPTS ALL
    COMMAND mkdir -p sophon-ffmpeg sophon-ffmpeg-devel
    # post-install
    COMMAND echo "rm -f ${CPACK_PACKAGING_INSTALL_PREFIX}/sophon-ffmpeg-latest" > sophon-ffmpeg/postinst
    COMMAND echo "ln -s ${CPACK_PACKAGING_INSTALL_PREFIX}/sophon-ffmpeg_${FFMPEG_VERSION} ${CPACK_PACKAGING_INSTALL_PREFIX}/sophon-ffmpeg-latest" >> sophon-ffmpeg/postinst
    COMMAND echo >> sophon-ffmpeg/postinst
    COMMAND echo "cp -f ${CPACK_PACKAGING_INSTALL_PREFIX}/sophon-ffmpeg-latest/data/01_sophon-ffmpeg.conf /etc/ld.so.conf.d/" >> sophon-ffmpeg/postinst
    COMMAND echo "ldconfig" >> sophon-ffmpeg/postinst
    COMMAND echo "cp -f ${CPACK_PACKAGING_INSTALL_PREFIX}/sophon-ffmpeg-latest/data/sophon-ffmpeg-autoconf.sh /etc/profile.d/" >> sophon-ffmpeg/postinst
    # pre-remove
    COMMAND echo "rm -f /etc/ld.so.conf.d/01_sophon-ffmpeg.conf" > sophon-ffmpeg/prerm
    COMMAND echo "ldconfig" >> sophon-ffmpeg/prerm
    COMMAND echo "rm -f /etc/profile.d/sophon-ffmpeg-autoconf.sh" >> sophon-ffmpeg/prerm
    # post-remove
    COMMAND echo "rm -f ${CPACK_PACKAGING_INSTALL_PREFIX}/sophon-ffmpeg-latest" > sophon-ffmpeg/postrm
    COMMAND echo "latest=\$(ls -dr ${CPACK_PACKAGING_INSTALL_PREFIX}/sophon-ffmpeg_* 2>/dev/null | head -n 1)" >> sophon-ffmpeg/postrm VERBATIM
    COMMAND echo "if [ -n \"\${latest}\" ]; then ln -s \${latest} ${CPACK_PACKAGING_INSTALL_PREFIX}/sophon-ffmpeg-latest; fi" >> sophon-ffmpeg/postrm
    # devel-post-install
    COMMAND echo "sed -i \"s/\\\/usr\\\/local/${REPLACE_CPACK_INSTALL_PREFIX}\\\/sophon-ffmpeg-latest/g\" ${CPACK_PACKAGING_INSTALL_PREFIX}/sophon-ffmpeg-latest/lib/pkgconfig/*.pc" > sophon-ffmpeg-devel/postinst
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian_script
)

#if (${PLATFORM} STREQUAL "soc")
add_custom_target(OPENCV_DEB_SCRIPTS ALL
    COMMAND mkdir -p sophon-opencv sophon-opencv-devel
    # post-install
    COMMAND echo "rm -f ${CPACK_PACKAGING_INSTALL_PREFIX}/sophon-opencv-latest" > sophon-opencv/postinst
    COMMAND echo "ln -s ${CPACK_PACKAGING_INSTALL_PREFIX}/sophon-opencv_${OPENCV_VERSION} ${CPACK_PACKAGING_INSTALL_PREFIX}/sophon-opencv-latest" >> sophon-opencv/postinst
    COMMAND echo >> sophon-opencv/postinst
    COMMAND echo "cp -f ${CPACK_PACKAGING_INSTALL_PREFIX}/sophon-opencv-latest/data/02_sophon-opencv.conf /etc/ld.so.conf.d/" >> sophon-opencv/postinst
    COMMAND echo "ldconfig" >> sophon-opencv/postinst
    COMMAND echo "cp -f ${CPACK_PACKAGING_INSTALL_PREFIX}/sophon-opencv-latest/data/sophon-opencv-autoconf.sh /etc/profile.d/" >> sophon-opencv/postinst
    # pre-remove
    COMMAND echo "rm -f /etc/ld.so.conf.d/02_sophon-opencv.conf" > sophon-opencv/prerm
    COMMAND echo "ldconfig" >> sophon-opencv/prerm
    COMMAND echo "rm -f /etc/profile.d/sophon-opencv-autoconf.sh" >> sophon-opencv/prerm
    # post-remove
    COMMAND echo "rm -f ${CPACK_PACKAGING_INSTALL_PREFIX}/sophon-opencv-latest" > sophon-opencv/postrm
    COMMAND echo "latest=\$(ls -dr ${CPACK_PACKAGING_INSTALL_PREFIX}/sophon-opencv_* 1>/dev/null | head -n 1)" >> sophon-opencv/postrm VERBATIM
    COMMAND echo "if [ -n \"\${latest}\" ]; then ln -s \${latest} ${CPACK_PACKAGING_INSTALL_PREFIX}/sophon-opencv-latest; fi" >> sophon-opencv/postrm
    COMMAND echo >> sophon-opencv/postrm
    #COMMAND echo "rm -f ${CPACK_PACKAGING_INSTALL_PREFIX}/opencv-bmcpu-latest" >> sophon-opencv/postrm
    #COMMAND echo "bmcpu_latest=\$(ls -dr ${CPACK_PACKAGING_INSTALL_PREFIX}/opencv-bmcpu-* 2>/dev/null | head -n 1)" >> sophon-opencv/postrm VERBATIM
    #COMMAND echo "if [ -n \"\${bmcpu_latest}\" ]; then ln -s \${bmcpu_latest} ${CPACK_PACKAGING_INSTALL_PREFIX}/opencv-bmcpu-latest; fi" >> sophon-opencv/postrm
    COMMAND echo "sed -i \"s/^prefix=.*$/prefix=\\\/opt\\\/sophon\\\/sophon-opencv-latest/g\" ${CPACK_PACKAGING_INSTALL_PREFIX}/sophon-opencv-latest/lib/pkgconfig/opencv4.pc" > sophon-opencv-devel/postinst
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian_script
)
#else()
#add_custom_target(OPENCV_DEB_SCRIPTS ALL
#    COMMAND mkdir -p sophon-opencv sophon-opencv-devel
#    # post-install
#    COMMAND echo "rm -f ${CPACK_PACKAGING_INSTALL_PREFIX}/sophon-opencv-latest" > sophon-opencv/postinst
#    COMMAND echo "ln -s ${CPACK_PACKAGING_INSTALL_PREFIX}/sophon-opencv_${OPENCV_VERSION} ${CPACK_PACKAGING_INSTALL_PREFIX}/sophon-opencv-latest" >> sophon-opencv/postinst
#    COMMAND echo >> sophon-opencv/postinst
#    COMMAND echo "rm -f ${CPACK_PACKAGING_INSTALL_PREFIX}/opencv-bmcpu-latest" >> sophon-opencv/postinst
#    COMMAND echo "ln -s ${CPACK_PACKAGING_INSTALL_PREFIX}/opencv-bmcpu_${OPENCV_VERSION} ${CPACK_PACKAGING_INSTALL_PREFIX}/opencv-bmcpu-latest" >> sophon-opencv/postinst
#    COMMAND echo >> sophon-opencv/postinst
#    COMMAND echo "cp -f ${CPACK_PACKAGING_INSTALL_PREFIX}/sophon-opencv-latest/data/02_sophon-opencv.conf /etc/ld.so.conf.d/" >> sophon-opencv/postinst
#    COMMAND echo "ldconfig" >> sophon-opencv/postinst
#    COMMAND echo "cp -f ${CPACK_PACKAGING_INSTALL_PREFIX}/sophon-opencv-latest/data/sophon-opencv-autoconf.sh /etc/profile.d/" >> sophon-opencv/postinst
#    # pre-remove
#    COMMAND echo "rm -f /etc/ld.so.conf.d/02_sophon-opencv.conf" > sophon-opencv/prerm
#    COMMAND echo "ldconfig" >> sophon-opencv/prerm
#    COMMAND echo "rm -f /etc/profile.d/sophon-opencv-autoconf.sh" >> sophon-opencv/prerm
#    # post-remove
#    COMMAND echo "rm -f ${CPACK_PACKAGING_INSTALL_PREFIX}/sophon-opencv-latest" > sophon-opencv/postrm
#    COMMAND echo "latest=\$(ls -dr ${CPACK_PACKAGING_INSTALL_PREFIX}/sophon-opencv-* 2>/dev/null | head -n 1)" >> sophon-opencv/postrm VERBATIM
#    COMMAND echo "if [ -n \"\${latest}\" ]; then ln -s \${latest} ${CPACK_PACKAGING_INSTALL_PREFIX}/sophon-opencv-latest; fi" >> sophon-opencv/postrm
#    COMMAND echo >> sophon-opencv/postrm
#    COMMAND echo "rm -f ${CPACK_PACKAGING_INSTALL_PREFIX}/opencv-bmcpu-latest" >> sophon-opencv/postrm
#    COMMAND echo "bmcpu_latest=\$(ls -dr ${CPACK_PACKAGING_INSTALL_PREFIX}/opencv-bmcpu-* 2>/dev/null | head -n 1)" >> sophon-opencv/postrm VERBATIM
#    COMMAND echo "if [ -n \"\${bmcpu_latest}\" ]; then ln -s \${bmcpu_latest} ${CPACK_PACKAGING_INSTALL_PREFIX}/opencv-bmcpu-latest; fi" >> sophon-opencv/postrm
#    COMMAND echo "sed -i \"s/^prefix=.*$/prefix=\\\/opt\\\/sophon\\\/sophon-opencv-latest/g\" ${CPACK_PACKAGING_INSTALL_PREFIX}/sophon-opencv-latest/lib/pkgconfig/opencv4.pc" > sophon-opencv-devel/postinst
#    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian_script
#)
#endif()

add_custom_target(GSTREAMER_DEB_SCRIPTS ALL
    COMMAND mkdir -p sophon-gstreamer sophon-gstreamer-devel
    # post-install
    COMMAND echo "rm -f ${CPACK_PACKAGING_INSTALL_PREFIX}/sophon-gstreamer-latest" > sophon-gstreamer/postinst
    COMMAND echo "ln -s ${CPACK_PACKAGING_INSTALL_PREFIX}/sophon-gstreamer_${GSTREAMER_VERSION} ${CPACK_PACKAGING_INSTALL_PREFIX}/sophon-gstreamer-latest" >> sophon-gstreamer/postinst
    COMMAND echo >> sophon-gstreamer/postinst
    COMMAND echo "cp -f ${CPACK_PACKAGING_INSTALL_PREFIX}/sophon-gstreamer-latest/data/03_sophon-gstreamer.conf /etc/ld.so.conf.d/" >> sophon-gstreamer/postinst
    COMMAND echo "ldconfig" >> sophon-gstreamer/postinst
    COMMAND echo "cp -f ${CPACK_PACKAGING_INSTALL_PREFIX}/sophon-gstreamer-latest/data/sophon-gstreamer-autoconf.sh /etc/profile.d/" >> sophon-gstreamer/postinst
    # pre-remove
    COMMAND echo "rm -f /etc/ld.so.conf.d/03_sophon-gstreamer.conf" > sophon-gstreamer/prerm
    COMMAND echo "ldconfig" >> sophon-gstreamer/prerm
    COMMAND echo "rm -f /etc/profile.d/sophon-gstreamer-autoconf.sh" >> sophon-gstreamer/prerm
    # post-remove
    COMMAND echo "rm -f ${CPACK_PACKAGING_INSTALL_PREFIX}/sophon-gstreamer-latest" > sophon-gstreamer/postrm
    COMMAND echo "latest=\$(ls -dr ${CPACK_PACKAGING_INSTALL_PREFIX}/sophon-gstreamer-* 2>/dev/null | head -n 1)" >> sophon-gstreamer/postrm VERBATIM
    COMMAND echo "if [ -n \"\${latest}\" ]; then ln -s \${latest} ${CPACK_PACKAGING_INSTALL_PREFIX}/sophon-gstreamer-latest; fi" >> sophon-gstreamer/postrm
    # devel-post-install
    #COMMAND echo "sed -i \"s/\\\/usr\\\/local/${REPLACE_CPACK_INSTALL_PREFIX}\\\/sophon-gstreamer-latest/g\" ${CPACK_PACKAGING_INSTALL_PREFIX}/sophon-gstreamer-latest/lib/pkgconfig/*.pc" > sophon-gstreamer-devel/postinst
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian_script
)

get_cmake_property(CPACK_COMPONENTS_ALL                 COMPONENTS)
list(REMOVE_ITEM CPACK_COMPONENTS_ALL                   "Unspecified" "libsophon" "libsophon-dev" "bmcpu" "bmcpu-dev" "driver")

add_custom_target(SAMPLES_DEB_SCRIPTS ALL
    COMMAND mkdir -p sophon-sample
    # post-install
    COMMAND echo "rm -f ${CPACK_PACKAGING_INSTALL_PREFIX}/sophon-sample-latest" > sophon-sample/postinst
    COMMAND echo "ln -s ${CPACK_PACKAGING_INSTALL_PREFIX}/sophon-sample_${SAMPLE_VERSION} ${CPACK_PACKAGING_INSTALL_PREFIX}/sophon-sample-latest" >> sophon-sample/postinst
    COMMAND echo >> sophon-sample/postinst
    COMMAND echo "cp -f ${CPACK_PACKAGING_INSTALL_PREFIX}/sophon-sample-latest/data/sophon-sample-autoconf.sh /etc/profile.d/" >> sophon-sample/postinst
    # pre-remove
    COMMAND echo "rm -f /etc/profile.d/sophon-sample-autoconf.sh" >> sophon-sample/prerm
    # post-remove
    COMMAND echo "rm -f ${CPACK_PACKAGING_INSTALL_PREFIX}/sophon-sample-latest" > sophon-sample/postrm
    COMMAND echo "latest=\$(ls -dr ${CPACK_PACKAGING_INSTALL_PREFIX}/sophon-sample_* 2>/dev/null | head -n 1)" >> sophon-sample/postrm VERBATIM
    COMMAND echo "if [ -n \"\${latest}\" ]; then ln -s \${latest} ${CPACK_PACKAGING_INSTALL_PREFIX}/sophon-sample-latest; fi" >> sophon-sample/postrm
    COMMAND echo >> sophon-sample/postrm
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian_script
)# create the .deb and .rpm files (you'll need build-essential and rpm tools)
INCLUDE( CPack )

