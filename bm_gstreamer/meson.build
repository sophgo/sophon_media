project('gstreamer-bm', ['c', 'cpp'],
  version : '1.14.4',
  license : 'LGPL',
  meson_version : '>= 0.47',
  default_options : [ 'warning_level=2',
                      'buildtype=debug','c_std=c99', 'cpp_std=c++14'])

gst_version = meson.project_version()
version_arr = gst_version.split('.')
gst_version_major = version_arr[0].to_int()
gst_version_minor = version_arr[1].to_int()
gst_version_micro = version_arr[2].to_int()

glib_req = '>= 2.32'
gst_req = '>= @0@.@1@.0'.format(gst_version_major, gst_version_minor)

api_version = '1.0'
soversion = 0
# maintaining compatibility with the previous libtool versioning
# current = minor * 100 + micro
libversion = '@0@.@1@.0'.format(soversion, gst_version_minor * 100 + gst_version_micro)

plugins_install_dir = '@0@/gstreamer-1.0'.format(get_option('libdir'))

cc = meson.get_compiler('c')
pkg = import('pkgconfig')

# Symbol visibility
#if cc.has_argument('-fvisibility=hidden')
#  add_project_arguments('-fvisibility=hidden', language: 'c')
#endif

# Disable strict aliasing
if cc.has_argument('-fno-strict-aliasing')
  add_project_arguments('-fno-strict-aliasing', language: 'c')
endif

message('Disabling deprecated GLib API')
add_project_arguments('-DG_DISABLE_DEPRECATED', language: 'c')

add_project_arguments('-DUNUSED=__attribute__((unused))', language: 'c')

cdata = configuration_data()

cdata.set_quoted('VERSION', gst_version)
cdata.set_quoted('PACKAGE', 'gst-bm')
cdata.set_quoted('PACKAGE_VERSION', gst_version)
cdata.set_quoted('PACKAGE_NAME', 'GStreamer Sophgo Plug-ins')
cdata.set_quoted('GST_API_VERSION', api_version)
cdata.set_quoted('GST_LICENSE', 'LGPL')
cdata.set_quoted('LIBDIR', join_paths(get_option('prefix'), get_option('libdir')))

# GStreamer package name and origin url
gst_package_name = get_option('package-name')
if gst_package_name == ''
  gst_package_name = 'GStreamer Sophgo Plug-ins'
endif
cdata.set_quoted('GST_PACKAGE_NAME', gst_package_name)
cdata.set_quoted('GST_PACKAGE_ORIGIN', get_option('package-origin'))

gst_dep = dependency('gstreamer-1.0', version : gst_req,
  fallback : ['gstreamer', 'gst_dep'])
gstbase_dep = dependency('gstreamer-base-1.0', version : gst_req,
  fallback : ['gstreamer', 'gst_base_dep'])
gstallocators_dep = dependency('gstreamer-allocators-1.0', version : gst_req,
  fallback : ['gst-plugins-base', 'allocators_dep'])
gstvideo_dep = dependency('gstreamer-video-1.0', version : gst_req,
  fallback : ['gst-plugins-base', 'video_dep'])
gstpbutils_dep = dependency('gstreamer-pbutils-1.0', version : gst_req,
  fallback : ['gst-plugins-base', 'pbutils_dep'])

glib_dep = dependency('glib-2.0', version : glib_req,
  fallback : ['glib', 'libglib_dep'])
gmodule_dep = dependency('gmodule-no-export-2.0')

cc = meson.get_compiler('c')
#sophon_dep = dependency('sophon', required : true)
bmlib_dep = cc.find_library('bmlib', required: true)
vd_dep = cc.find_library('bmvd', required: true)
venc_dep = cc.find_library('bmvenc', required: true)
jpg_dep = cc.find_library('bmjpeg', required: true)
cv_dep = cc.find_library('bmcv', required: true)

gst_sophgo_args = ['-DHAVE_CONFIG_H']
configinc = include_directories('.')

subdir('gst')
subdir('tests')
subdir('samples')

configure_file(output : 'config.h', configuration : cdata)
