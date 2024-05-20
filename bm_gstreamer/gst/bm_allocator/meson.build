
inc_dirs = include_directories('/opt/sophon/libsophon-0.4.9/include')
bmallocator_sources = [
  'gstbmallocator.c'
  ]

gstbmallocator = library('gstbmallocator',
  bmallocator_sources,
  c_args : [gst_sophgo_args, '-Wextra', '-fno-lto'],
  include_directories: [configinc, inc_dirs],
  dependencies : [gstbase_dep, gstvideo_dep, gstallocators_dep, bmlib_dep, vd_dep, venc_dep],
  install : true,
)

gstbmallocator_dep = declare_dependency(
  include_directories: [configinc, inc_dirs],
  dependencies : [gstbase_dep, gstvideo_dep, gstallocators_dep, bmlib_dep, vd_dep, venc_dep],
)