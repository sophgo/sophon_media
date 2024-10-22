bmstitch4way_sources = [
    'gstbmstitch4way.c',
    'plugin.c',
]

if get_option('bmstitch4way').disabled()
  subdir_done()
endif

extra_args = ['-DGST_USE_UNSTABLE_API']
extra_incdirs = [
  include_directories('/opt/sophon/libsophon-0.4.9/include'),
  include_directories('../bm_allocator'),
]

plugin_incdirs = [configinc, extra_incdirs]

gstbmstitch4way = library('gstbmstitch4way',
  bmstitch4way_sources,
  c_args : gst_sophgo_args + extra_args,
  cpp_args : gst_sophgo_args + extra_args,
  include_directories : plugin_incdirs,
  dependencies : [gstbmallocator_dep, gstpbutils_dep, gstvideo_dep, gmodule_dep, vd_dep, jpg_dep, cv_dep],
  link_with: [gstbmallocator],
  install : true,
  install_dir : plugins_install_dir
)