#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstbmv4l2src.h"

GST_DEBUG_CATEGORY (bm_v4l2_debug);

static gboolean
plugin_init(GstPlugin *plugin)
{
  GST_DEBUG_CATEGORY_INIT(bm_v4l2_debug, "bmv4l2src", 0, "bmv4l2src");

  GstCaps *src_template = gst_caps_from_string(GST_BM_V4L2SRC_SRC_FORMATS);
  GstCaps *sink_template = gst_caps_from_string(GST_BM_V4L2SRC_SINK_FORMATS);

  GST_INFO("BM v4l2src, sink template: %" GST_PTR_FORMAT
      ", src template: %" GST_PTR_FORMAT, sink_template, src_template);

  gst_bmv4l2src_register(plugin, sink_template, src_template);

  /* unref Caps */
  gst_caps_unref(sink_template);
  gst_caps_unref(src_template);

  return TRUE;
}

GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    bmv4l2src,
    "GStreamer bmv4l2src plugin",
    plugin_init,
    VERSION,
    "LGPL",
    GST_PACKAGE_NAME,
    GST_PACKAGE_ORIGIN
)
