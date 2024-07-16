#include "gstbmcodeccaps.h"


// sink_caps_string need be less than upstream,
// otherwise will get "Internal data stream error"

const GstBmVideoCodecCapsMap bm_video_codec_caps_map_list[] = {
  {BmVideoCodecID_H264, "h264",
      "video/x-h264, stream-format = (string) byte-stream"
      ", alignment = (string) au"
      ", profile = (string) { baseline, main, high }"},
  {BmVideoCodecID_HEVC, "h265",
      "video/x-h265, stream-format = (string) byte-stream"
      ", alignment = (string) au, profile = (string) { main }"},
  {BmVideoCodecID_JPEG, "jpeg", "image/jpeg"}
};

const gchar * gst_bm_video_codec_to_string (BmVideoCodecID codec_id)
{
  guint i;

  for (i = 0; i < G_N_ELEMENTS (bm_video_codec_caps_map_list); i++) {
    if (bm_video_codec_caps_map_list[i].codec_id == codec_id)
      return bm_video_codec_caps_map_list[i].codec_name;
  }

  return "unknown";
}

GstCaps * gst_bm_video_codec_get_all_sink_caps (void)
{
  GstCaps *sink_caps = NULL;
  GstCaps *temp_cap = NULL;
  guint i;

  sink_caps = gst_caps_new_empty ();
  for (i = 0; i < G_N_ELEMENTS (bm_video_codec_caps_map_list); i++) {
    temp_cap = gst_caps_from_string (bm_video_codec_caps_map_list[i].sink_caps_string);
    gst_caps_append (sink_caps, temp_cap);
  }

  return sink_caps;
}