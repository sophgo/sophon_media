bmv4l2src_sources = [
    'gstbmv4l2src.c',
    'gstv4l2allocator.c',
    'gstv4l2object.c',
    'gstv4l2bufferpool.c',
    'v4l2_calls.c',
    'v4l2-utils.c',
    'plugin.c',
]

if get_option('bmv4l2src').disabled()
  subdir_done()
endif

extra_args = ['-DGST_USE_UNSTABLE_API','-DGST_USE_UNSTABLE_API']
extra_incdirs = [
  include_directories('/opt/sophon/libsophon-0.4.9/include'),
  include_directories('/opt/sophon/sophon-soc-libisp-dev_1.0.0/include/'),
  include_directories('../bm_allocator'),
]

plugin_incdirs = [configinc, extra_incdirs]

helper_dep = cc.find_library('ispv4l2_helper', required: true)
adapter_dep = cc.find_library('ispv4l2_adapter', required: true)
sensor_dep = cc.find_library('sns_full', required: true)
ae_dep = cc.find_library('ae', required: true)
af_dep = cc.find_library('af', required: true)
awb_dep = cc.find_library('awb', required: true)
cvi_bin_dep = cc.find_library('cvi_bin', required: true)
cvi_bin_isp_dep = cc.find_library('cvi_bin_isp', required: true)
isp_dep = cc.find_library('isp', required: true)
isp_algo_dep = cc.find_library('isp_algo', required: true)
ispv4l2_dep = [helper_dep,adapter_dep,sensor_dep,ae_dep,af_dep,awb_dep,cvi_bin_dep,cvi_bin_isp_dep,isp_dep,isp_algo_dep]

gstbmv4l2src = library('gstbmv4l2src',
  bmv4l2src_sources,
  c_args : gst_sophgo_args + extra_args,
  cpp_args : gst_sophgo_args + extra_args,
  include_directories : plugin_incdirs,
  dependencies : [gstbase_dep, gstbmallocator_dep, gstvideo_dep, gstallocators_dep, gstpbutils_dep, gmodule_dep, ispv4l2_dep],
  link_with: [gstbmallocator],
  install : true,
  install_dir : plugins_install_dir
)