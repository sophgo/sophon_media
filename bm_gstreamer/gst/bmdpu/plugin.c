#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/video/video-format.h>
#include "gstbmdpu.h"

GST_DEBUG_CATEGORY (gst_bmdpu_debug);
#define GST_CAT_DEFAULT gst_bmdpu_debug


static gboolean
plugin_init(GstPlugin *plugin)
{
  GST_DEBUG_CATEGORY_INIT(gst_bmdpu_debug, "bmdpu", 0, "bmdpu");

  GstCaps *sink_template = gst_caps_from_string(GST_VIDEO_CAPS_MAKE_FORMATS);
  GstCaps *src_template1 = gst_caps_from_string(GST_VIDEO_CAPS_MAKE_FORMATS);
  GstCaps *src_template2 = gst_caps_from_string(GST_VIDEO_CAPS_MAKE_FORMATS);

  if (!sink_template) {
      g_print ("Failed to create sink_template\n");
      return -1;
  }
  if (!src_template1) {
      g_print ("Failed to create src_template1\n");
      return -1;
  }
  if (!src_template2) {
      g_print ("Failed to create src_template2\n");
      return -1;
  }
  GST_INFO("BM dpu, sink template: %" GST_PTR_FORMAT
      ", src template1: %" GST_PTR_FORMAT", src template1: %" GST_PTR_FORMAT, sink_template, src_template1,src_template2);
  /* 0 --> soc_idx */
  gst_bm_dpu_register(plugin, 0, sink_template, src_template1,src_template2);

  /* unref Caps */
  gst_caps_unref(sink_template);
  gst_caps_unref(src_template1);
  gst_caps_unref(src_template2);

  return TRUE;
}

GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    bmdpu,
    "GStreamer bmdpu plugin",
    plugin_init,
    VERSION,
    "LGPL",
    GST_PACKAGE_NAME,
    GST_PACKAGE_ORIGIN
)
