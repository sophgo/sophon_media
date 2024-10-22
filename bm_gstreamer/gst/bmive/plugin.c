#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/video/video-format.h>
#include "gstbmive.h"

GST_DEBUG_CATEGORY (gst_bmive_debug);
#define GST_CAT_DEFAULT gst_bmive_debug


static gboolean
plugin_init(GstPlugin *plugin)
{
//   GstCaps *sink_template = NULL;
//   GstCaps *src_template = NULL;

  GST_DEBUG_CATEGORY_INIT(gst_bmive_debug, "bmive", 0, "bmive");

//   const GstVideoFormat sink_formats[] = {GST_VIDEO_FORMAT_NV12,
//                                          GST_VIDEO_FORMAT_RGB};
//   const GstVideoFormat src_formats[] = {GST_VIDEO_FORMAT_RGB,
//                                         GST_VIDEO_FORMAT_NV12};

//   sink_template =
//       gst_video_make_raw_caps(sink_formats, G_N_ELEMENTS(sink_formats));
//   src_template =
//       gst_video_make_raw_caps(src_formats, G_N_ELEMENTS(src_formats));

  GstCaps *sink_template = gst_caps_from_string(GST_VIDEO_CAPS_MAKE_FORMATS);
  GstCaps *src_template = gst_caps_from_string(GST_VIDEO_CAPS_MAKE_FORMATS);

  GST_INFO("BM ive, sink template: %" GST_PTR_FORMAT
      ", src template: %" GST_PTR_FORMAT, sink_template, src_template);
  /* 0 --> soc_idx */
  gst_bm_ive_register(plugin, 0, sink_template, src_template);

  /* unref Caps */
  gst_caps_unref(sink_template);
  gst_caps_unref(src_template);

  return TRUE;
}

GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    bmive,
    "GStreamer BMIVE plugin",
    plugin_init,
    VERSION,
    "LGPL",
    GST_PACKAGE_NAME,
    GST_PACKAGE_ORIGIN
)
