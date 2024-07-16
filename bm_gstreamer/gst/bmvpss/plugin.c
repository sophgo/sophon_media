#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/video/video-format.h>
#include "gstbmvpss.h"

GST_DEBUG_CATEGORY (gst_bmvpss_debug);
#define GST_CAT_DEFAULT gst_bmvpss_debug


static gboolean
plugin_init(GstPlugin *plugin)
{
  GST_DEBUG_CATEGORY_INIT(gst_bmvpss_debug, "bmvpss", 0, "bmvpss");

  GstCaps *sink_template = gst_caps_from_string(GST_VIDEO_CAPS_MAKE_FORMATS);
  GstCaps *src_template = gst_caps_from_string(GST_VIDEO_CAPS_MAKE_FORMATS);

  GST_INFO("BM vpss, sink template: %" GST_PTR_FORMAT
      ", src template: %" GST_PTR_FORMAT, sink_template, src_template);
  /* 0 --> soc_idx */
  gst_bm_vpss_register(plugin, 0, sink_template, src_template);

  /* unref Caps */
  gst_caps_unref(sink_template);
  gst_caps_unref(src_template);

  return TRUE;
}

GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    bmvpss,
    "GStreamer BMVPSS plugin",
    plugin_init,
    VERSION,
    "LGPL",
    GST_PACKAGE_NAME,
    GST_PACKAGE_ORIGIN
)
