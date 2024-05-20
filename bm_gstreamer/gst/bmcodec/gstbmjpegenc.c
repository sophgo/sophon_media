/*
 * Copyright (C) 2024 Sophgo
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include "gstbmjpegenc.h"
#include "gstbmallocator.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))
#endif

#ifndef SWAP
#define SWAP(x, y) ({ x ^= y; y ^= x; x ^= y; })
#endif

struct _GstBmJpegEnc
{
  GstVideoEncoder parent;

  GMutex mutex;
  GstAllocator *allocator;
  GstVideoCodecState *input_state;

  /* final input video info */
  GstVideoInfo info;

  /* stop handling new frame when flushing */
  gboolean flushing;

  /* drop frames when flushing but not draining */
  gboolean draining;

  gboolean eos_send;

  guint32 required_keyframe_number;

  GMutex event_mutex;
  GCond event_cond;

  /* flow return from pad task */
  GstFlowReturn task_ret;

  gint soc_idx;

  gint width;
  gint height;
  gint rotation;

  gint y_stride;
  gint cbcr_stride;
  gint y_offset;
  gint cb_offset;
  gint cr_offset;

  guint bps;
  guint bps_min;
  guint bps_max;
  guint q_factor;
  guint qf_min;
  guint qf_max;

  gboolean prop_dirty;
  BmJpuJPEGEncParams params;
  BmJpuColorFormat color_format;
  BmJpuJPEGEncoder  *bm_jpeg_enc;
};

GST_DEBUG_CATEGORY (gst_bm_jpeg_enc_debug);
#define GST_CAT_DEFAULT gst_bm_jpeg_enc_debug

#define parent_class gst_bm_jpeg_enc_parent_class
G_DEFINE_TYPE (GstBmJpegEnc, gst_bm_jpeg_enc, GST_TYPE_VIDEO_ENCODER);


#define GST_BM_JPEG_ENC_MUTEX(encoder) (&GST_BM_JPEG_ENC (encoder)->mutex)

#define GST_BM_JPEG_ENC_LOCK(encoder) \
  GST_VIDEO_ENCODER_STREAM_UNLOCK (encoder); \
  g_mutex_lock (GST_BM_JPEG_ENC_MUTEX (encoder)); \
  GST_VIDEO_ENCODER_STREAM_LOCK (encoder);

#define GST_BM_JPEG_ENC_UNLOCK(encoder) \
  g_mutex_unlock (GST_BM_JPEG_ENC_MUTEX (encoder));


#define GST_BM_JPEG_VIDEO_INFO_HSTRIDE(i) GST_VIDEO_INFO_PLANE_STRIDE(i, 0)
#define GST_BM_JPEG_VIDEO_INFO_VSTRIDE(i) \
    (GST_VIDEO_INFO_N_PLANES(i) == 1 ? GST_VIDEO_INFO_HEIGHT(i) : \
    (gint) (GST_VIDEO_INFO_PLANE_OFFSET(i, 1) / GST_BM_JPEG_VIDEO_INFO_HSTRIDE(i)))



#define GST_BM_ALIGNMENT 16
#define GST_BM_ALIGN(v) GST_ROUND_UP_N (v, GST_BM_ALIGNMENT)

#define DEFAULT_PROP_GOP -1     /* Same as FPS */
#define DEFAULT_PROP_MAX_REENC 1
#define DEFAULT_PROP_BPS 0      /* Auto */
#define DEFAULT_PROP_WIDTH 0    /* Original */
#define DEFAULT_PROP_HEIGHT 0   /* Original */
#define DEFAULT_FPS 30
#define DEFAULT_PROP_Q_FACTOR 80
#define DEFAULT_PROP_QF_MIN 1
#define DEFAULT_PROP_QF_MAX 99
enum
{
  PROP_0,
  PROP_BPS,
  PROP_BPS_MIN,
  PROP_BPS_MAX,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_Q_FACTOR,
  PROP_LAST,
};

#define BM_JPEG_ENC_FORMATS \
    "NV12, I420, N21, NV16, " \
    "YUV400, YUV422, YUV444"

#define GST_BM_JPEG_ENC_SIZE_CAPS \
    "width  = (int) [ 16, MAX ], height = (int) [ 16, MAX ]"

static GstStaticPadTemplate gst_bm_jpeg_enc_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("image/jpeg, "
        GST_BM_JPEG_ENC_SIZE_CAPS "," "sof-marker = { 0 }"));

static GstStaticPadTemplate gst_bm_jpeg_enc_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw,"
        "format = (string) { " BM_JPEG_ENC_FORMATS " }, "
        GST_BM_JPEG_ENC_SIZE_CAPS));

gboolean
gst_bm_jpeg_enc_supported (void)
{
  BmJpuJPEGEncoder *bm_jpeg_enc = NULL;
  gint buf_size = 1024*1024;
  if (bm_jpu_jpeg_enc_open(&bm_jpeg_enc, buf_size, 0)) {
    return FALSE;
  }
  bm_jpu_jpeg_enc_close(bm_jpeg_enc);
  return TRUE;
}

gboolean
gst_bm_jpeg_enc_video_info_align (GstVideoInfo * info)
{
  GstVideoAlignment align;
  guint stride;
  gint hstride = 0, vstride = 0;
  guint i;

  if (!hstride)
    hstride = GST_BM_ALIGN (GST_BM_JPEG_VIDEO_INFO_HSTRIDE (info));

  if (!vstride)
    vstride = GST_BM_ALIGN (GST_BM_JPEG_VIDEO_INFO_VSTRIDE (info));

  if (hstride == GST_BM_JPEG_VIDEO_INFO_HSTRIDE (info) &&
      vstride == GST_BM_JPEG_VIDEO_INFO_VSTRIDE (info))
    return TRUE;

  GST_DEBUG ("aligning %dx%d to %dx%d", GST_VIDEO_INFO_WIDTH (info),
      GST_VIDEO_INFO_HEIGHT (info), hstride, vstride);

  if (hstride < GST_VIDEO_INFO_WIDTH (info) ||
      vstride < GST_VIDEO_INFO_HEIGHT (info)) {
    GST_ERROR ("unable to align %dx%d to %dx%d", GST_VIDEO_INFO_WIDTH (info),
        GST_VIDEO_INFO_HEIGHT (info), hstride, vstride);
    return FALSE;
  }

  gst_video_alignment_reset (&align);

  /* Apply vstride */
  align.padding_bottom = vstride - GST_VIDEO_INFO_HEIGHT (info);
  if (!gst_video_info_align (info, &align))
    return FALSE;

  /* Apply vstride for single-plane */
  if (GST_VIDEO_INFO_N_PLANES (info) == 1)
    GST_VIDEO_INFO_SIZE (info) =
        GST_VIDEO_INFO_PLANE_STRIDE (info, 0) * vstride;

  if (GST_VIDEO_INFO_PLANE_STRIDE (info, 0) == hstride)
    return TRUE;

  /* Apply hstride */
  stride = GST_VIDEO_INFO_PLANE_STRIDE (info, 0);
  for (i = 0; i < GST_VIDEO_INFO_N_PLANES (info); i++) {
    GST_VIDEO_INFO_PLANE_STRIDE (info, i) =
        GST_VIDEO_INFO_PLANE_STRIDE (info, i) * hstride / stride;
    GST_VIDEO_INFO_PLANE_OFFSET (info, i) =
        GST_VIDEO_INFO_PLANE_OFFSET (info, i) / stride * hstride;

    GST_DEBUG ("plane %d, stride %d, offset %" G_GSIZE_FORMAT, i,
        GST_VIDEO_INFO_PLANE_STRIDE (info, i),
        GST_VIDEO_INFO_PLANE_OFFSET (info, i));
  }
  GST_VIDEO_INFO_SIZE (info) = GST_VIDEO_INFO_SIZE (info) / stride * hstride;

 // GST_DEBUG ("aligned size" G_GSIZE_FORMAT, GST_VIDEO_INFO_SIZE (info));

  return TRUE;
}
static void
gst_bm_jpeg_enc_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GstVideoEncoder *encoder = GST_VIDEO_ENCODER (object);
  GstBmJpegEnc *self = GST_BM_JPEG_ENC (encoder);

  switch (prop_id) {
    case PROP_BPS:{
      guint bps = g_value_get_uint (value);
      if (self->bps == bps)
        return;

      self->bps = bps;
      break;
    }
    case PROP_WIDTH:{
      if (self->input_state)
        GST_WARNING_OBJECT (encoder, "unable to change width");
      else
        self->width = g_value_get_uint (value);
      return;
    }
    case PROP_HEIGHT:{
      if (self->input_state)
        GST_WARNING_OBJECT (encoder, "unable to change height");
      else
        self->height = g_value_get_uint (value);
      return;
    }
    case PROP_Q_FACTOR:{
      guint q_factor = g_value_get_uint (value);
      if (self->q_factor == q_factor)
        return;
      self->q_factor = q_factor;
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      return;
  }

  self->prop_dirty = TRUE;
}

static void
gst_bm_jpeg_enc_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  GstVideoEncoder *encoder = GST_VIDEO_ENCODER (object);
  GstBmJpegEnc *self = GST_BM_JPEG_ENC (encoder);

  switch (prop_id) {
    case PROP_BPS:
      g_value_set_uint (value, self->bps);
      break;
    case PROP_WIDTH:
      g_value_set_uint (value, self->width);
      break;
    case PROP_HEIGHT:
      g_value_set_uint (value, self->height);
      break;
    case PROP_Q_FACTOR:
      g_value_set_uint (value, self->q_factor);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      return;
  }
}

gboolean
gst_bm_jpeg_enc_apply_properties (GstVideoEncoder * encoder)
{
  GstBmJpegEnc *self = GST_BM_JPEG_ENC(encoder);
  GstVideoInfo *info = &self->info;
  gint fps_num = GST_VIDEO_INFO_FPS_N (info);
  gint fps_denorm = GST_VIDEO_INFO_FPS_D (info);
  gint fps = fps_num / fps_denorm;

  if (!self->prop_dirty)
    return TRUE;

  self->params.quality_factor = self->q_factor;
  self->prop_dirty = FALSE;

  if (!self->bps)
    self->bps =
        GST_VIDEO_INFO_WIDTH (info) * GST_VIDEO_INFO_HEIGHT (info) / 8 * fps;

  return TRUE;
}

gboolean
gst_bm_jpeg_enc_set_src_caps (GstVideoEncoder * encoder, GstCaps * caps)
{
  GstBmJpegEnc *self = GST_BM_JPEG_ENC (encoder);
  GstVideoInfo *info = &self->info;
  GstVideoCodecState *output_state;

  gst_caps_set_simple (caps,
      "width", G_TYPE_INT, GST_VIDEO_INFO_WIDTH (info),
      "height", G_TYPE_INT, GST_VIDEO_INFO_HEIGHT (info), NULL);

  GST_DEBUG_OBJECT (self, "output caps: %" GST_PTR_FORMAT, caps);

  output_state = gst_video_encoder_set_output_state (encoder,
      caps, self->input_state);

  GST_VIDEO_INFO_WIDTH (&output_state->info) = GST_VIDEO_INFO_WIDTH (info);
  GST_VIDEO_INFO_HEIGHT (&output_state->info) = GST_VIDEO_INFO_HEIGHT (info);
  gst_video_codec_state_unref (output_state);

  return gst_video_encoder_negotiate (encoder);
}

static void
gst_bm_jpeg_enc_reset (GstVideoEncoder * encoder, gboolean drain, gboolean final)
{
  GstBmJpegEnc *self = GST_BM_JPEG_ENC (encoder);

  GST_BM_JPEG_ENC_LOCK (encoder);

  GST_DEBUG_OBJECT (self, "resetting");

  self->flushing = TRUE;
  self->draining = drain;

  self->flushing = final;
  self->draining = FALSE;

  self->task_ret = GST_FLOW_OK;
  /* Force re-apply prop */
  self->prop_dirty = TRUE;

  GST_BM_JPEG_ENC_UNLOCK (encoder);
}

static gboolean
gst_bm_jpeg_enc_start (GstVideoEncoder * encoder)
{
  GstBmJpegEnc *self = GST_BM_JPEG_ENC (encoder);
  int ret;

  GST_DEBUG_OBJECT (self, "starting");

  gst_video_info_init (&self->info);

  self->allocator = gst_bm_allocator_new ();
  if (!self->allocator) {
    GST_ERROR_OBJECT (self, "gst_bm_allocator_new fail \n");
    return FALSE;
  }

  self->soc_idx = 0;
  ret = bm_jpu_enc_load(self->soc_idx);
  if (ret != BM_JPU_ENC_RETURN_CODE_OK) {
      GST_ERROR_OBJECT(self, "bm_jpu_enc_load failed!\n");
      return FALSE;
  }

  self->task_ret = GST_FLOW_OK;
  self->input_state = NULL;
  self->flushing = FALSE;
  self->required_keyframe_number = 0;

  g_mutex_init (&self->mutex);
  g_mutex_init (&self->event_mutex);
  g_cond_init (&self->event_cond);

  GST_DEBUG_OBJECT (self, "started");

  return TRUE;
}

static gboolean
gst_bm_jpeg_enc_stop (GstVideoEncoder * encoder)
{
  GstBmJpegEnc *self = GST_BM_JPEG_ENC (encoder);

  GST_DEBUG_OBJECT (self, "stopping");

  GST_VIDEO_ENCODER_STREAM_LOCK (encoder);
  gst_bm_jpeg_enc_reset (encoder, FALSE, TRUE);
  GST_VIDEO_ENCODER_STREAM_UNLOCK (encoder);

  g_cond_clear (&self->event_cond);
  g_mutex_clear (&self->event_mutex);

  g_mutex_clear (&self->mutex);

  bm_jpu_jpeg_enc_close(self->bm_jpeg_enc);

  gst_object_unref (self->allocator);

  if (self->input_state)
    gst_video_codec_state_unref (self->input_state);

  if (self->soc_idx >= 0)
  {
      int ret;
      ret = bm_jpu_enc_unload(self->soc_idx);
      if (ret != BM_JPU_ENC_RETURN_CODE_OK) {
          GST_ERROR_OBJECT(self, "Failed to call bm_jpu_enc_unload\n");
          return FALSE;
      } else {
          self->soc_idx = -1;
      }
  }

  GST_DEBUG_OBJECT (self, "stopped");

  return TRUE;
}

static gboolean
gst_bm_jpeg_enc_flush (GstVideoEncoder * encoder)
{
  GST_DEBUG_OBJECT (encoder, "flushing");
  gst_bm_jpeg_enc_reset (encoder, FALSE, FALSE);
  return TRUE;
}

static gboolean
gst_bm_jpeg_enc_finish (GstVideoEncoder * encoder)
{
  GST_DEBUG_OBJECT (encoder, "finishing");
  gst_bm_jpeg_enc_reset (encoder, TRUE, FALSE);
  return GST_FLOW_OK;
}

static gboolean
gst_bm_jpeg_gst_format_bm_format(GstBmJpegEnc *self)
{
  GstVideoInfo *info = &self->info;
  BmJpuColorFormat format;
  gint hstride, vstride;
  gint interleave;
  hstride = GST_BM_JPEG_VIDEO_INFO_HSTRIDE (info);
  vstride = GST_BM_JPEG_VIDEO_INFO_VSTRIDE (info);
  switch (GST_VIDEO_INFO_FORMAT (info))
  {
      case GST_VIDEO_FORMAT_I420:
          format = BM_JPU_COLOR_FORMAT_YUV420;
          self->cbcr_stride = hstride / 2;
          self->y_offset = 0;
          self->cb_offset = hstride * vstride;
          self->cr_offset = hstride * vstride;
          interleave = 0;
          break;
      case GST_VIDEO_FORMAT_NV12:
          format = BM_JPU_COLOR_FORMAT_YUV420;
          self->cbcr_stride = hstride;
          self->y_offset = 0;
          self->cb_offset = hstride * vstride;
          self->cr_offset = 0;
          interleave = 1;
          break;
      case GST_VIDEO_FORMAT_NV21:
          format = BM_JPU_COLOR_FORMAT_YUV420;
          self->cbcr_stride = hstride;
          self->y_offset = 0;
          self->cb_offset = hstride * vstride;
          self->cr_offset = 0;
          interleave = 2;
          break;
      case GST_VIDEO_FORMAT_Y42B:
          format = BM_JPU_COLOR_FORMAT_YUV422_HORIZONTAL;
          self->cbcr_stride = hstride;
          self->y_offset = 0;
          self->cb_offset = hstride * vstride;
          self->cr_offset = self->cb_offset + hstride * vstride / 2;
          interleave = 0;
          break;
      case GST_VIDEO_FORMAT_NV16 :
          format = BM_JPU_COLOR_FORMAT_YUV422_HORIZONTAL;
          self->cbcr_stride = hstride;
          self->y_offset = 0;
          self->cb_offset = hstride * vstride;
          self->cr_offset = 0;
          interleave = 1;
          break;
      case GST_VIDEO_FORMAT_NV61 :
          format = BM_JPU_COLOR_FORMAT_YUV422_HORIZONTAL;
          self->cbcr_stride = hstride;
          self->y_offset = 0;
          self->cb_offset = hstride * vstride;
          self->cr_offset = 0;
          interleave = 2;
          break;
      case GST_VIDEO_FORMAT_Y444:
          format = BM_JPU_COLOR_FORMAT_YUV444;
          self->cbcr_stride = hstride;
          self->y_offset = 0;
          self->cb_offset = hstride * vstride;
          self->cr_offset = self->cb_offset + hstride * vstride ;
          interleave = 0;
          break;
      default:
          format = BM_JPU_COLOR_FORMAT_BUTT;
          break;
  }
  if (format == BM_JPU_COLOR_FORMAT_BUTT) {
     return FALSE;
  }
  self->params.color_format = format;
  self->color_format = format;
  self->params.chroma_interleave = interleave;
  return TRUE;
}

static gboolean
gst_bm_jpeg_enc_set_format (GstVideoEncoder * encoder, GstVideoCodecState * state)
{
  GstBmJpegEnc *self = GST_BM_JPEG_ENC (encoder);
  GstVideoInfo *info = &self->info;
  gint width, height, hstride;
  GstCaps *caps;

  GST_DEBUG_OBJECT (self, "setting format: %" GST_PTR_FORMAT, state->caps);

  if (self->input_state) {
    if (gst_caps_is_strictly_equal (self->input_state->caps, state->caps))
      return TRUE;

    gst_bm_jpeg_enc_reset (encoder, TRUE, FALSE);

    gst_video_codec_state_unref (self->input_state);
    self->input_state = NULL;
  }

  self->input_state = gst_video_codec_state_ref (state);

  *info = state->info;

  if (!gst_bm_jpeg_enc_video_info_align (info))
    return FALSE;

  width = GST_VIDEO_INFO_WIDTH (info);
  height = GST_VIDEO_INFO_HEIGHT (info);

  if (self->rotation % 180)
    SWAP (width, height);

  width = self->width ? : width;
  height = self->height ? : height;

  /*
   * Check for conversion
   * NOTE: Not checking the strides here, since they might not be the actual
   * ones (could be overrided by video-meta)
   */
  if (self->rotation || width != GST_VIDEO_INFO_WIDTH (info) ||
      height != GST_VIDEO_INFO_HEIGHT (info)) {
      return FALSE;
    /*todo*/
    /* Prefer NV12 when using vpss conversion */
  }

  hstride = GST_BM_JPEG_VIDEO_INFO_HSTRIDE (info);

  self->y_stride = hstride;

  if (!gst_bm_jpeg_gst_format_bm_format(self)) {
      return FALSE;
  }
  self->params.frame_width = width;
  self->params.frame_height = height;

  if (!GST_VIDEO_INFO_FPS_N (info) || GST_VIDEO_INFO_FPS_N (info) > 240) {
    GST_WARNING_OBJECT (self, "framerate (%d/%d) is insane!",
        GST_VIDEO_INFO_FPS_N (info), GST_VIDEO_INFO_FPS_D (info));
    GST_VIDEO_INFO_FPS_N (info) = DEFAULT_FPS;
  }
  //self->params.fps_num = GST_VIDEO_INFO_FPS_N (info);
  //self->params.fps_den = GST_VIDEO_INFO_FPS_D (info);
  gint buf_size = width * height;
  if (bm_jpu_jpeg_enc_open(&self->bm_jpeg_enc, buf_size, self->soc_idx)) {
     GST_ERROR_OBJECT(self, "bm_jpu_jpeg_enc_open open failed!\n");
    return FALSE;
  }
  caps = gst_caps_new_empty_simple ("image/jpeg");
  return gst_bm_jpeg_enc_set_src_caps (encoder, caps);
}

static gboolean
gst_bm_jpeg_enc_propose_allocation (GstVideoEncoder * encoder, GstQuery * query)
{
  GstBmJpegEnc *self = GST_BM_JPEG_ENC (encoder);
  GstStructure *config, *params;
  GstVideoAlignment align;
  GstBufferPool *pool;
  GstVideoInfo info;
  GstCaps *caps;
  guint size;

  GST_DEBUG_OBJECT (self, "propose allocation");

  gst_query_parse_allocation (query, &caps, NULL);
  if (caps == NULL)
    return FALSE;

  if (!gst_video_info_from_caps (&info, caps))
    return FALSE;

  gst_bm_jpeg_enc_video_info_align (&info);
  size = GST_VIDEO_INFO_SIZE (&info);

  gst_video_alignment_reset (&align);
  align.padding_right = GST_BM_JPEG_VIDEO_INFO_HSTRIDE (&info) -
      GST_VIDEO_INFO_WIDTH (&info);
  align.padding_bottom = GST_BM_JPEG_VIDEO_INFO_VSTRIDE (&info) -
      GST_VIDEO_INFO_HEIGHT (&info);

  /* Expose alignment to video-meta */
  params = gst_structure_new ("video-meta",
      "padding-top", G_TYPE_UINT, align.padding_top,
      "padding-bottom", G_TYPE_UINT, align.padding_bottom,
      "padding-left", G_TYPE_UINT, align.padding_left,
      "padding-right", G_TYPE_UINT, align.padding_right, NULL);
  gst_query_add_allocation_meta (query, GST_VIDEO_META_API_TYPE, params);
  gst_structure_free (params);

  pool = gst_video_buffer_pool_new ();

  config = gst_buffer_pool_get_config (pool);
  gst_buffer_pool_config_set_params (config, caps, size, 0, 0);
  gst_buffer_pool_config_set_allocator (config, self->allocator, NULL);

  /* Expose alignment to pool */
  gst_buffer_pool_config_add_option (config,
      GST_BUFFER_POOL_OPTION_VIDEO_ALIGNMENT);
  gst_buffer_pool_config_set_video_alignment (config, &align);

  gst_buffer_pool_set_config (pool, config);

  gst_query_add_allocation_pool (query, pool, size, BM_PENDING_MAX, 0);
  gst_query_add_allocation_param (query, self->allocator, NULL);

  gst_object_unref (pool);

  return GST_VIDEO_ENCODER_CLASS (parent_class)->propose_allocation (encoder,
      query);
}

static GstBuffer *
gst_bm_jpeg_enc_convert (GstVideoEncoder * encoder, GstVideoCodecFrame * frame)
{
  GstBmJpegEnc *self = GST_BM_JPEG_ENC (encoder);
  GstVideoInfo src_info = self->input_state->info;
  GstVideoInfo dst_info = self->info;
  GstVideoFrame src_frame, dst_frame;
  GstBuffer *outbuf, *inbuf;
  GstMemory *in_mem, *out_mem = NULL;
  GstVideoMeta *meta;
  gsize size, maxsize, offset;
  //gint src_hstride, src_vstride;
  guint i;

  inbuf = frame->input_buffer;

  meta = gst_buffer_get_video_meta (inbuf);
  if (meta) {
    for (i = 0; i < meta->n_planes; i++) {
      GST_VIDEO_INFO_PLANE_STRIDE (&src_info, i) = meta->stride[i];
      GST_VIDEO_INFO_PLANE_OFFSET (&src_info, i) = meta->offset[i];
    }
  }

  size = gst_buffer_get_sizes (inbuf, &offset, &maxsize);
  if (size < GST_VIDEO_INFO_SIZE (&src_info)) {
    GST_ERROR_OBJECT (self, "input buffer too small (%" G_GSIZE_FORMAT
        " < %" G_GSIZE_FORMAT ")", size, GST_VIDEO_INFO_SIZE (&src_info));
    return NULL;
  }

  outbuf = gst_buffer_new ();
  if (!outbuf)
    goto err;

  if (gst_buffer_n_memory (inbuf) != 1)
    goto convert;

  in_mem = gst_buffer_peek_memory (inbuf, 0);
  out_mem = gst_bm_allocator_import_gst_memory (self->allocator, in_mem);
  if (!out_mem)
    goto convert;

  //src_hstride = GST_BM_VIDEO_INFO_HSTRIDE (&src_info);
  //src_vstride = GST_BM_VIDEO_INFO_VSTRIDE (&src_info);

  //!gst_video_info_align (&dst_info, src_hstride, src_vstride) ||
  if (!gst_bm_jpeg_enc_video_info_align (&dst_info)) {
    GST_DEBUG_OBJECT (self,"align fail \n");
    goto convert;
  }

 // gst_bm_enc_apply_strides (encoder, src_hstride, src_vstride);
  if (!gst_bm_jpeg_enc_apply_properties (encoder)) {
    GST_DEBUG_OBJECT (self,"apply fail \n");
    goto err;
  }

  gst_buffer_append_memory (outbuf, out_mem);

  GST_DEBUG_OBJECT (self, "using imported buffer");
  goto out;

convert:
  if (out_mem)
    gst_memory_unref (out_mem);

  out_mem = gst_allocator_alloc (self->allocator,
      GST_VIDEO_INFO_SIZE (&dst_info), NULL);
  if (!out_mem)
    goto err;

  gst_buffer_append_memory (outbuf, out_mem);

  if (gst_video_frame_map (&src_frame, &src_info, inbuf, GST_MAP_READ)) {
    if (gst_video_frame_map (&dst_frame, &dst_info, outbuf, GST_MAP_WRITE)) {
      GST_VIDEO_ENCODER_STREAM_UNLOCK (encoder);
      if (!gst_video_frame_copy (&dst_frame, &src_frame)) {
        GST_VIDEO_ENCODER_STREAM_LOCK (encoder);
        gst_video_frame_unmap (&dst_frame);
        gst_video_frame_unmap (&src_frame);
        goto err;
      }
      GST_VIDEO_ENCODER_STREAM_LOCK (encoder);
      gst_video_frame_unmap (&dst_frame);
    }
    gst_video_frame_unmap (&src_frame);
  }

  GST_DEBUG_OBJECT (self, "using software converted buffer");

out:
  gst_buffer_copy_into (outbuf, inbuf,
      GST_BUFFER_COPY_FLAGS | GST_BUFFER_COPY_TIMESTAMPS, 0, 0);

  dst_info = self->info;
  gst_buffer_add_video_meta_full (outbuf, GST_VIDEO_FRAME_FLAG_NONE,
      GST_VIDEO_INFO_FORMAT (&dst_info),
      GST_VIDEO_INFO_WIDTH (&dst_info), GST_VIDEO_INFO_HEIGHT (&dst_info),
      GST_VIDEO_INFO_N_PLANES (&dst_info), dst_info.offset, dst_info.stride);

  return outbuf;
err:
  if (out_mem)
    gst_memory_unref (out_mem);

  if (outbuf)
    gst_buffer_unref (outbuf);

  GST_ERROR_OBJECT (self, "failed to convert frame");
  return NULL;
}

static gint gst_bm_jpu_write_output_data(void *context, uint8_t const *data,
                                uint32_t size,
                                BmJpuEncodedFrame G_GNUC_UNUSED *encoded_frame)
{
  GstVideoEncoder * encoder = context;
  GstVideoCodecFrame *frame;
  GstBuffer *buffer;
  GstBmJpegEnc *self = GST_BM_JPEG_ENC(encoder);

  frame = gst_video_encoder_get_oldest_frame (encoder);

  if (!frame)
    goto error;

  buffer = gst_video_encoder_allocate_output_buffer (encoder, size);
  if (!buffer)
    goto error;

  gst_buffer_fill (buffer, 0, data, size);
  gst_buffer_replace (&frame->output_buffer, buffer);
  gst_buffer_unref (buffer);

  GST_DEBUG_OBJECT (self, "finish frame ts=%" GST_TIME_FORMAT,
      GST_TIME_ARGS (frame->pts));

  gst_video_encoder_finish_frame (encoder, frame);
out:
  return TRUE;
error:
  GST_WARNING_OBJECT (self, "process cur frame fail");
  GST_DEBUG_OBJECT (self, "drop frame");
  gst_buffer_replace (&frame->output_buffer, NULL);
  gst_video_encoder_finish_frame (encoder, frame);
  goto out;
}

static GstFlowReturn
gst_bm_jpeg_enc_handle_frame (GstVideoEncoder * encoder, GstVideoCodecFrame * frame)
{
  GstBmJpegEnc *self = GST_BM_JPEG_ENC (encoder);
  GstBuffer *buffer;
  GstMemory *mem;

  BmJpuFramebuffer framebuffer;
  bm_device_mem_t *fb_dma_buffer;
  GstFlowReturn ret = GST_FLOW_OK;
  size_t out_size = 0;
  BmJpuEncReturnCodes jpeg_ret;

  if (G_UNLIKELY (!gst_bm_jpeg_enc_apply_properties (encoder))) {
    gst_video_codec_frame_unref (frame);
    return GST_FLOW_NOT_NEGOTIATED;
  }

  GST_DEBUG_OBJECT (self, "handling frame %d", frame->system_frame_number);

  GST_BM_JPEG_ENC_LOCK (encoder);

  if (G_UNLIKELY (self->flushing))
    goto flushing;

  buffer = gst_bm_jpeg_enc_convert (encoder, frame);
  if (G_UNLIKELY (!buffer))
    goto not_negotiated;

  frame->output_buffer = buffer;
  mem = gst_buffer_peek_memory (frame->output_buffer, 0);
  fb_dma_buffer = gst_bm_allocator_get_bm_buffer (mem);
  if (!fb_dma_buffer) {
    GST_ERROR_OBJECT (self,"gst_bm_allocator get_bm_buffer fail \n");
    return GST_FLOW_ERROR;
  }

  memset(&framebuffer, 0, sizeof(BmJpuFramebuffer));
  framebuffer.dma_buffer = fb_dma_buffer;
  framebuffer.y_stride = self->y_stride;
  framebuffer.cbcr_stride = self->cbcr_stride;
  framebuffer.y_offset = self->y_offset;
  framebuffer.cb_offset = self->cb_offset;
  framebuffer.cr_offset = self->cr_offset;

  self->params.packed_format  = 0;
  self->params.output_buffer_context = (void*)encoder;
  self->params.write_output_data = gst_bm_jpu_write_output_data;

  jpeg_ret = bm_jpu_jpeg_enc_encode(self->bm_jpeg_enc,
                                     &framebuffer,
                                     &self->params,
                                     NULL,
                                     &out_size);

  if (jpeg_ret != BM_JPU_ENC_RETURN_CODE_OK) {
      GST_ERROR_OBJECT(self,"jpeg encode failed!\n");
      goto drop;
  }

  gst_video_codec_frame_unref (frame);

  GST_BM_JPEG_ENC_UNLOCK (encoder);

  return GST_FLOW_OK;

flushing:
  GST_WARNING_OBJECT (self, "flushing");
  ret = GST_FLOW_FLUSHING;
  goto drop;
not_negotiated:
  GST_ERROR_OBJECT (self, "not negotiated");
  ret = GST_FLOW_NOT_NEGOTIATED;
  goto drop;
drop:
  GST_WARNING_OBJECT (self, "can't handle this frame");
  gst_video_encoder_finish_frame (encoder, frame);

  GST_BM_JPEG_ENC_UNLOCK (encoder);

  return ret;
}

static GstStateChangeReturn
gst_bm_jpeg_enc_change_state (GstElement * element, GstStateChange transition)
{
  GstVideoEncoder *encoder = GST_VIDEO_ENCODER (element);

  if (transition == GST_STATE_CHANGE_PAUSED_TO_READY) {
    GST_VIDEO_ENCODER_STREAM_LOCK (encoder);
    gst_bm_jpeg_enc_reset (encoder, FALSE, TRUE);
    GST_VIDEO_ENCODER_STREAM_UNLOCK (encoder);
  }

  return GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
}

static void
gst_bm_jpeg_enc_init (GstBmJpegEnc * self)
{
  memset(&self->params, 0, sizeof(self->params));
  self->bps = DEFAULT_PROP_BPS;
  self->q_factor = DEFAULT_PROP_Q_FACTOR;
  self->qf_min = DEFAULT_PROP_QF_MIN;
  self->qf_max = DEFAULT_PROP_QF_MAX;
  self->prop_dirty = TRUE;
  self->soc_idx = -1;
}

static void
gst_bm_jpeg_enc_class_init (GstBmJpegEncClass * klass)
{
  GstVideoEncoderClass *encoder_class = GST_VIDEO_ENCODER_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "bmjpegenc", 0, "sophon encoder");

  encoder_class->start = GST_DEBUG_FUNCPTR (gst_bm_jpeg_enc_start);
  encoder_class->stop = GST_DEBUG_FUNCPTR (gst_bm_jpeg_enc_stop);
  encoder_class->flush = GST_DEBUG_FUNCPTR (gst_bm_jpeg_enc_flush);
  encoder_class->finish = GST_DEBUG_FUNCPTR (gst_bm_jpeg_enc_finish);
  encoder_class->set_format = GST_DEBUG_FUNCPTR (gst_bm_jpeg_enc_set_format);
  encoder_class->propose_allocation =
      GST_DEBUG_FUNCPTR (gst_bm_jpeg_enc_propose_allocation);
  encoder_class->handle_frame = GST_DEBUG_FUNCPTR (gst_bm_jpeg_enc_handle_frame);

  gobject_class->set_property = GST_DEBUG_FUNCPTR (gst_bm_jpeg_enc_set_property);
  gobject_class->get_property = GST_DEBUG_FUNCPTR (gst_bm_jpeg_enc_get_property);

  g_object_class_install_property (gobject_class, PROP_BPS,
      g_param_spec_uint ("bps", "Target BPS",
          "Target BPS (0 = auto calculate)",
          0, G_MAXINT, DEFAULT_PROP_BPS,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_Q_FACTOR,
      g_param_spec_uint ("q-factor", "Quality Factor",
          "Quality Factor", 1, 99, DEFAULT_PROP_Q_FACTOR,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_bm_jpeg_enc_src_template));

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_bm_jpeg_enc_sink_template));

  gst_element_class_set_static_metadata (element_class,
      "SOPHGO JPEG Encoder", "Codec/Encoder/Video",
      "Encode video streams via SOPHGO",
      "xx <xx@sophgo.com>"
      );
  element_class->change_state = GST_DEBUG_FUNCPTR (gst_bm_jpeg_enc_change_state);
}

gboolean
gst_bm_jpeg_enc_register (GstPlugin * plugin, guint rank)
{
  if (!gst_bm_jpeg_enc_supported ())
    return FALSE;

  return gst_element_register (plugin, "bmjpegenc", rank,
      gst_bm_jpeg_enc_get_type ());
}







