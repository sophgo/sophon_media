#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstbmdecoder.h"
#include "gstbmh264enc.h"
#include "gstbmh265enc.h"
#include "gstbmjpegenc.h"

GST_DEBUG_CATEGORY (gst_bmcodec_debug);
#define GST_CAT_DEFAULT gst_bmcodec_debug

static gboolean
plugin_init (GstPlugin * plugin)
{
  GstCaps *sink_template = NULL;
  GstCaps *src_template = NULL;

  GST_DEBUG_CATEGORY_INIT (gst_bmcodec_debug, "bmcodec", 0, "bmcodec");

  sink_template = gst_bm_video_codec_get_all_sink_caps ();
  // TODO: support more format for src
  src_template = gst_caps_from_string (GST_VIDEO_CAPS_MAKE ("NV12"));

  GST_INFO ("BM video decoder, sink template %" GST_PTR_FORMAT
      "src template %" GST_PTR_FORMAT, sink_template, src_template);

  // TODO: register decoder on all chips?
  gst_bm_decoder_register (plugin, 0, sink_template, src_template);

  gst_bm_h264_enc_register (plugin, GST_RANK_PRIMARY + 1);

  gst_bm_h265_enc_register (plugin, GST_RANK_PRIMARY + 1);

  gst_bm_jpeg_enc_register(plugin, GST_RANK_PRIMARY + 1);

  gst_caps_unref (sink_template);
  gst_caps_unref (src_template);

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR, GST_VERSION_MINOR, bmcodec,
    "GStreamer BMCODEC plugin", plugin_init, VERSION, "LGPL",
    GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
