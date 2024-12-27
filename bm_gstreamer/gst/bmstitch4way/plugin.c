#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/video/video-format.h>
#include "gstbmstitch4way.h"

GST_DEBUG_CATEGORY (gst_bmstitch4way_debug);
#define GST_CAT_DEFAULT gst_bmstitch4way_debug


static gboolean
plugin_init(GstPlugin *plugin)
{
  GST_DEBUG_CATEGORY_INIT(gst_bmstitch4way_debug, "bmstitch4way", 0, "bmstitch4way");

  GstCaps *sink_template1 = gst_caps_from_string(GST_VIDEO_CAPS_MAKE_FORMATS_SINK);
  GstCaps *sink_template2 = gst_caps_from_string(GST_VIDEO_CAPS_MAKE_FORMATS_SINK);
  GstCaps *src_template = gst_caps_from_string(GST_VIDEO_CAPS_MAKE_FORMATS_SRC);

  if (!sink_template1) {
      g_print ("Failed to create sink_template1\n");
      return -1;
  }
  if (!sink_template2) {
      g_print ("Failed to create sink_template2\n");
      return -1;
  }
  if (!src_template) {
      g_print ("Failed to create src_template\n");
      return -1;
  }
  GST_INFO("BM stitch, sink template: %" GST_PTR_FORMAT
      ", src template1: %" GST_PTR_FORMAT", src template1: %" GST_PTR_FORMAT, sink_template1, sink_template2,src_template);
  /* 0 --> soc_idx */
  gst_bm_stitch4way_register(plugin, 0, sink_template1, sink_template2,src_template);

  /* unref Caps */
  gst_caps_unref(sink_template1);
  gst_caps_unref(sink_template2);
  gst_caps_unref(src_template);

  return TRUE;
}

GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    bmstitch4way,
    "GStreamer bmstitch4way plugin",
    plugin_init,
    VERSION,
    "LGPL",
    GST_PACKAGE_NAME,
    GST_PACKAGE_ORIGIN
)
