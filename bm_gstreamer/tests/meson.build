gst_check_dep = dependency('gstreamer-check-1.0', version : '>= 1.0')

test_sources = [
  'src/test_gstbmdecode.cpp',
  'src/test_gstbmencode.cpp',
  'src/test_main.cpp',
  ]

gstbmstream_test = executable('gstbmstream_test',
  test_sources,
  c_args : [gst_sophgo_args, '-Wextra', '-fno-lto'],
  include_directories: [configinc],
  dependencies : [gst_dep, gstbase_dep, gstvideo_dep, gstallocators_dep, gst_check_dep],
  install : true
)