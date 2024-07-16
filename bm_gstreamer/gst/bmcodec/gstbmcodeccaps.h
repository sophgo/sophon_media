#ifndef __GST_BM_CODEC_CAPS_H__
#define __GST_BM_CODEC_CAPS_H__

#include <gst/gst.h>

typedef enum BmVideoCodecID_enum {
  BmVideoCodecID_H264 = 0,
  BmVideoCodecID_HEVC,
  BmVideoCodecID_JPEG,
  BmVideoCodecID_NumCodecs  /* Max num of codecs */,
} BmVideoCodecID;

typedef struct
{
  BmVideoCodecID codec_id;
  const gchar *codec_name;
  const gchar *sink_caps_string;
} GstBmVideoCodecCapsMap;

const gchar * gst_bm_video_codec_to_string (BmVideoCodecID codec_id);

GstCaps * gst_bm_video_codec_get_all_sink_caps (void);

#endif /* __GST_BM_CODEC_CAPS_H */