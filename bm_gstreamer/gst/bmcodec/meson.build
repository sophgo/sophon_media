bmcodec_sources = [
  'gstbmdecoder.c',
  'gstbmcodeccaps.c',
  'gstbmenc.c',
  'gstbmh264enc.c',
  'gstbmh265enc.c',
  'gstbmjpegenc.c',
  'plugin.c',
]

if get_option('bmcodec').disabled()
  subdir_done()
endif

extra_args = ['-DGST_USE_UNSTABLE_API']
extra_incdirs = [
  include_directories('/opt/sophon/libsophon-0.4.9/include'),
  include_directories('../bm_allocator'),
]

plugin_incdirs = [configinc, extra_incdirs]

gstbmcodec = library('gstbmcodec',
  bmcodec_sources,
  c_args : gst_sophgo_args + extra_args,
  cpp_args : gst_sophgo_args + extra_args,
  include_directories : plugin_incdirs,
  dependencies : [gstbmallocator_dep, gstpbutils_dep, gmodule_dep, vd_dep,jpg_dep],
  link_with: [gstbmallocator],
  install : true,
  install_dir : plugins_install_dir
)