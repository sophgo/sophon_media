transcode_sources = [
  'demo/gst_transcode.cpp'
  ]

test_gst_transcode = executable('test_gst_transcode',
  transcode_sources,
  c_args : [gst_sophgo_args, '-Wextra', '-fno-lto'],
  include_directories: [configinc, inc_dirs],
  dependencies : [gst_dep, gstbase_dep, gstvideo_dep, gstallocators_dep],
  install : true
)

multi_test_sources = [
  'demo/gst_vc_multi.cpp'
  ]

thread_dep = dependency('threads')

test_gst_vcmulti = executable('test_gst_vcmulti',
  multi_test_sources,
  c_args : [gst_sophgo_args, '-Wextra', '-fno-lto'],
  include_directories: [configinc, inc_dirs],
  dependencies : [gst_dep, gstbase_dep, gstvideo_dep, gstallocators_dep, thread_dep],
  install : true
)