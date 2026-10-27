# --- V4L ---
if(NOT HAVE_V4L)
  set(CMAKE_REQUIRED_QUIET TRUE) # for check_include_file
  check_include_file(linux/videodev2.h HAVE_CAMV4L2)
  check_include_file(sys/videoio.h HAVE_VIDEOIO)
  if(HAVE_CAMV4L2 OR HAVE_VIDEOIO)
    set(HAVE_V4L TRUE)
    set(defs)
    if(HAVE_CAMV4L2)
      list(APPEND defs "HAVE_CAMV4L2")
    endif()
    if(HAVE_VIDEOIO)
      list(APPEND defs "HAVE_VIDEOIO")
    endif()
    # HAVE_SOPH_V4L: Sophon-specific V4L2 ISP backend (cap_soph_v4l.cpp)
    # Only available on the bm1688 SoC, which has the VI/ISP sensor pipeline.
    # cv84x6 (even on soc) and all pcie modes lack VI/ISP, so the ISP libraries
    # (3A/ISP tuning/sensor control) must not be linked there. ENABLE_ISP is
    # passed from the top-level build (soc && bm1688).
    if(ENABLE_ISP)
      set(HAVE_SOPH_V4L TRUE)  # CMake variable for CMakeLists.txt conditionals
      list(APPEND defs "HAVE_SOPH_V4L")  # Compile definition for #ifdef in source files
    endif()
    ocv_add_external_target(v4l "" "" "${defs}")
  endif()
endif()
