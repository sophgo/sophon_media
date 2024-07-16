bmvpss_sources = [
    'gstbmvpss.c',
    'plugin.c',
]

if get_option('bmvpss').disabled()
  subdir_done()
endif

extra_args = ['-DGST_USE_UNSTABLE_API']
extra_incdirs = [
  include_directories('/opt/sophon/libsophon-0.4.9/include'),
  include_directories('../bm_allocator'),
]

plugin_incdirs = [configinc, extra_incdirs]

gstbmvpss = library('gstbmvpss',
  bmvpss_sources,
  c_args : gst_sophgo_args + extra_args,
  cpp_args : gst_sophgo_args + extra_args,
  include_directories : plugin_incdirs,
  dependencies : [gstbmallocator_dep, gstpbutils_dep, gstvideo_dep, gmodule_dep, cv_dep],
  link_with: [gstbmallocator],
  install : true,
  install_dir : plugins_install_dir
)