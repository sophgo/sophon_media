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
#include "gstbmenc.h"
#include "gstbmallocator.h"
#include <stdio.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))
#endif

#ifndef SWAP
#define SWAP(x, y) ({ x ^= y; y ^= x; x ^= y; })
#endif

static void* acquire_output_buffer(void *context, size_t size, void **acquired_handle)
{
    ((void)(context));
    void *mem;

    mem = malloc(size);
    *acquired_handle = mem;
    return mem;
}
static void finish_output_buffer(void *context, void *acquired_handle)
{
    ((void)(context));
}

GST_DEBUG_CATEGORY (gst_bm_enc_debug);
#define GST_CAT_DEFAULT gst_bm_enc_debug

#define parent_class gst_bm_enc_parent_class
G_DEFINE_ABSTRACT_TYPE (GstBmEnc, gst_bm_enc, GST_TYPE_VIDEO_ENCODER);

#define GST_BM_ENC_TASK_STARTED(encoder) \
    (gst_pad_get_task_state ((encoder)->srcpad) == GST_TASK_STARTED)

#define GST_BM_ENC_MUTEX(encoder) (&GST_BM_ENC (encoder)->mutex)

#define GST_BM_ENC_LOCK(encoder) \
  GST_VIDEO_ENCODER_STREAM_UNLOCK (encoder); \
  g_mutex_lock (GST_BM_ENC_MUTEX (encoder)); \
  GST_VIDEO_ENCODER_STREAM_LOCK (encoder);

#define GST_BM_ENC_UNLOCK(encoder) \
  g_mutex_unlock (GST_BM_ENC_MUTEX (encoder));

#define GST_BM_ENC_EVENT_MUTEX(encoder) (&GST_BM_ENC (encoder)->event_mutex)
#define GST_BM_ENC_EVENT_COND(encoder) (&GST_BM_ENC (encoder)->event_cond)

#define GST_BM_ENC_BROADCAST(encoder) \
  g_mutex_lock (GST_BM_ENC_EVENT_MUTEX (encoder)); \
  g_cond_broadcast (GST_BM_ENC_EVENT_COND (encoder)); \
  g_mutex_unlock (GST_BM_ENC_EVENT_MUTEX (encoder));

#define GST_BM_ENC_WAIT(encoder, condition) \
  g_mutex_lock (GST_BM_ENC_EVENT_MUTEX (encoder)); \
  while (!(condition)) \
    g_cond_wait (GST_BM_ENC_EVENT_COND (encoder), \
        GST_BM_ENC_EVENT_MUTEX (encoder)); \
  g_mutex_unlock (GST_BM_ENC_EVENT_MUTEX (encoder));

#define GST_BM_VIDEO_INFO_HSTRIDE(i) GST_VIDEO_INFO_PLANE_STRIDE(i, 0)
#define GST_BM_VIDEO_INFO_VSTRIDE(i) \
    (GST_VIDEO_INFO_N_PLANES(i) == 1 ? GST_VIDEO_INFO_HEIGHT(i) : \
    (gint) (GST_VIDEO_INFO_PLANE_OFFSET(i, 1) / GST_BM_VIDEO_INFO_HSTRIDE(i)))

#define GST_BM_ALIGNMENT 16
#define GST_BM_ALIGN(v) GST_ROUND_UP_N (v, GST_BM_ALIGNMENT)

#define DEFAULT_PROP_GOP -1     /* Same as FPS */
#define DEFAULT_PROP_MAX_REENC 1
#define DEFAULT_PROP_BPS 0      /* Auto */
#define DEFAULT_PROP_BPS_CHANGE_POS 90 /* Auto */
#define DEFAULT_PROP_WIDTH 0    /* Original */
#define DEFAULT_PROP_HEIGHT 0   /* Original */
#define DEFAULT_FPS 30
#define DEFAULT_PROP_GOP_PRESET 2
#define DEFAULT_PROP_PRESET 2
#define DEFAULT_PROP_DEALT_QP 2
#define DEFAULT_PROP_CQP -1
#define DEFAULT_PROP_MB_RC 1
enum
{
  PROP_0,
  PROP_GOP,
  PROP_GOP_PRESET,
  PROP_PRESET,
  PROP_BPS,
  PROP_BPS_CHANGE_POS,
  PROP_DEALT_QP,
  PROP_CQP,
  PROP_MB_RC,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_LAST,
};

struct gst_bm_format
{
  GstVideoFormat gst_format;
  BmVpuEncPixFormat bm_format;
  gint pixel_stride0;
  gboolean is_yuv;
};

#define GST_BM_FORMAT(gst, bm, pixel_stride0, yuv) \
  { GST_VIDEO_FORMAT_ ## gst, BM_VPU_ENC_PIX_FORMAT_## bm, pixel_stride0, yuv}

struct gst_bm_format gst_bm_formats[] = {
  GST_BM_FORMAT (I420, YUV420P, 1, 1),
  GST_BM_FORMAT (NV12, NV12, 1, 1),
  GST_BM_FORMAT (NV21, NV21, 1, 1),
  GST_BM_FORMAT (NV16, NV16, 1, 1),
  GST_BM_FORMAT (Y42B, YUV422P, 1, 1),
  GST_BM_FORMAT (Y444, YUV444P, 1, 1),
};

#define GST_BM_GET_FORMAT(type, format) ({ \
  struct gst_bm_format *_tmp; \
  for (guint i = 0; i < ARRAY_SIZE (gst_bm_formats) || (_tmp = NULL); i++) { \
    _tmp = &gst_bm_formats[i]; \
    if (_tmp->type ## _format == (format)) break;\
  }; _tmp; \
})

static const char * const bm_preset_names[] = { "slow", "medium", "fast", 0 };
#define GST_BM_ENC_PRESET_TYPE (gst_bm_enc_preset_get_type())
static GType
gst_bm_enc_preset_get_type (void)
{
  static GType speed_preset = 0;
  static GEnumValue *speed_presets;
  int n, i;

  if (speed_preset != 0)
    return speed_preset;

  n = 0;
  while (bm_preset_names[n] != NULL)
    n++;

  speed_presets = g_new0 (GEnumValue, n + 2);

  for (i = 0; i < n; i++) {
    speed_presets[i].value = i;
    speed_presets[i].value_name = bm_preset_names[i];
    speed_presets[i].value_nick = bm_preset_names[i];
  }

  speed_preset = g_enum_register_static ("GstBmSpeedPreset", speed_presets);

  return speed_preset;
}

static const char * const bm_gop_preset_names[] = { "all-I", "I-P-P", "I-B-B-B",
"I-B-P-B-P","I-B-B-B-P","I-P-P-P-P", "I-B-B-B-B-B-B-B-B", 0 };
#define GST_BM_ENC_GOP_PRESET_TYPE (gst_bm_enc_gop_preset_get_type())
static GType
gst_bm_enc_gop_preset_get_type (void)
{
  static GType gop_preset = 0;
  static GEnumValue *gop_preset_values;
  int n, i;

  if (gop_preset != 0)
    return gop_preset;

  n = 0;
  while (bm_gop_preset_names[n] != NULL)
    n++;

  gop_preset_values = g_new0 (GEnumValue, n + 2);

  gop_preset_values[0].value = 0;
  gop_preset_values[0].value_name = "No set";
  gop_preset_values[0].value_nick = "No set";

  for (i = 0; i < n; i++) {
    gop_preset_values[i + 1].value = i + 1;
    gop_preset_values[i + 1].value_name = bm_gop_preset_names[i];
    gop_preset_values[i + 1].value_nick = bm_gop_preset_names[i];
  }

  gop_preset = g_enum_register_static ("GstBmGopPreset", gop_preset_values);

  return gop_preset;
}


guint
gst_bm_get_pixel_stride (GstVideoInfo * info)
{
  GstVideoFormat gst_format = GST_VIDEO_INFO_FORMAT (info);
  struct gst_bm_format *format = GST_BM_GET_FORMAT (gst, gst_format);
  guint hstride = GST_BM_VIDEO_INFO_HSTRIDE (info);

  if (!format)
    return hstride;

  return hstride / format->pixel_stride0;
}

gboolean
gst_bm_enc_supported (BmVpuCodecFormat codec_type)
{
  BmVpuEncoder  *bmVenc;
  BmVpuEncOpenParams open_params;
  BmVpuEncInitialInfo initial_info;
  bmvpu_enc_set_default_open_params(&open_params, codec_type);
  open_params.frame_width = 352;
  open_params.frame_height = 288;
  if(bmvpu_enc_open(&bmVenc, &open_params, NULL, &initial_info)) {
    return FALSE;
  }
  bmvpu_enc_close(bmVenc);
  return TRUE;
}

static void
gst_bm_enc_convert_dma_buffer(bm_device_mem_t *bm_dma_buffer, BmVpuEncDMABuffer *bm_venc_dma_buffer)
{
  bm_venc_dma_buffer->size = bm_dma_buffer->size;
  bm_venc_dma_buffer->phys_addr = bm_dma_buffer->u.device.device_addr;
  bm_venc_dma_buffer->dmabuf_fd = bm_dma_buffer->u.device.dmabuf_fd;
}

gboolean
gst_bm_enc_video_info_align (GstVideoInfo * info)
{
  GstVideoAlignment align;
  guint stride;
  gint hstride = 0, vstride = 0;
  guint i;

  if (!hstride)
    hstride = GST_BM_ALIGN (GST_BM_VIDEO_INFO_HSTRIDE (info));

  if (!vstride)
    vstride = GST_BM_VIDEO_INFO_VSTRIDE (info);

  if (hstride == GST_BM_VIDEO_INFO_HSTRIDE (info) &&
      vstride == GST_BM_VIDEO_INFO_VSTRIDE (info))
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

  GST_DEBUG ("aligned size %" G_GSIZE_FORMAT, GST_VIDEO_INFO_SIZE (info));

  return TRUE;
}
static void
gst_bm_enc_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GstVideoEncoder *encoder = GST_VIDEO_ENCODER (object);
  GstBmEnc *self = GST_BM_ENC (encoder);

  switch (prop_id) {
    case PROP_GOP: {
      gint gop = g_value_get_int (value);
      if (self->gop == gop)
        return;
      self->gop = gop;
      break;
    }
    case PROP_GOP_PRESET: {
      gint gop_preset = g_value_get_enum (value);
      if (self->gop_preset == gop_preset)
        return;
      self->gop_preset = gop_preset;
      break;
    }
    case PROP_BPS:{
      guint bps = g_value_get_uint (value);
      if (self->bps == bps)
        return;

      self->bps = bps;
      break;
    }
    case PROP_BPS_CHANGE_POS:{
      guint change_pos = g_value_get_uint (value);
      if (self->change_pos == change_pos)
        return;

      self->change_pos = change_pos;
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
    case PROP_PRESET:{
      gint preset = g_value_get_enum (value);
      if (self->preset == preset)
        return;
      self->preset = preset;
      break;
    }
    case PROP_DEALT_QP:{
      gint dealt_qp = g_value_get_int (value);
      if (self->dealt_qp == dealt_qp)
        return;

      self->dealt_qp = dealt_qp;
      break;
    }
    case PROP_CQP:{
      gint cqp = g_value_get_int (value);
      if (self->cqp == cqp)
        return;

      self->cqp = cqp;
      break;
    }
    case PROP_MB_RC:{
      guint mb_rc = g_value_get_uint (value);
      if (self->mb_rc == mb_rc)
        return;

      self->mb_rc = mb_rc;
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      return;
  }

  self->prop_dirty = TRUE;
}

static void
gst_bm_enc_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  GstVideoEncoder *encoder = GST_VIDEO_ENCODER (object);
  GstBmEnc *self = GST_BM_ENC (encoder);

  switch (prop_id) {
    case PROP_GOP:
      g_value_set_int (value, self->gop);
      break;
    case PROP_GOP_PRESET:
      g_value_set_enum (value, self->gop_preset);
      break;
    case PROP_BPS:
      g_value_set_uint (value, self->bps);
      break;
    case PROP_BPS_CHANGE_POS:
      g_value_set_uint (value, self->change_pos);
      break;
    case PROP_WIDTH:
      g_value_set_uint (value, self->width);
      break;
    case PROP_HEIGHT:
      g_value_set_uint (value, self->height);
      break;
     case PROP_PRESET:
      g_value_set_enum (value, self->preset);
      break;
    case PROP_DEALT_QP:
      g_value_set_int (value, self->dealt_qp);
      break;
    case PROP_CQP:{
      g_value_set_int (value, self->cqp);
      break;
    }
    case PROP_MB_RC:{
      g_value_set_uint (value, self->mb_rc);
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      return;
  }
}

gboolean
gst_bm_enc_apply_properties (GstVideoEncoder * encoder)
{
  GstBmEnc *self = GST_BM_ENC (encoder);
  GstVideoInfo *info = &self->info;
  gint fps_num = GST_VIDEO_INFO_FPS_N (info);
  gint fps_denorm = GST_VIDEO_INFO_FPS_D (info);
  gint fps = fps_num / fps_denorm;
  if (fps == 0) {
      // set default fps to 30.
      fps = DEFAULT_FPS;
      fps_num = DEFAULT_FPS;
      fps_denorm = 1 ;
  }

  if (!self->prop_dirty)
    return TRUE;

  self->prop_dirty = FALSE;
  self->params.intra_period = self->gop < 0 ? fps : self->gop;

  if (!self->bps) {
    self->bps = GST_VIDEO_INFO_WIDTH (info) * GST_VIDEO_INFO_HEIGHT (info) / 8 * fps;
  }

  self->params.bitrate = self->bps;
  self->params.intra_period = self->gop;
  self->params.fps_num = fps_num;
  self->params.fps_den = fps_denorm;
  self->params.gop_preset = self->gop_preset;
  self->params.enc_mode = self->preset;
  self->params.delta_qp = self->dealt_qp;
  self->params.cqp = self->cqp;
  self->params.mb_rc = self->mb_rc;
  return TRUE;
}

gboolean
gst_bm_enc_set_src_caps (GstVideoEncoder * encoder, GstCaps * caps)
{
  GstBmEnc *self = GST_BM_ENC (encoder);
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
gst_bm_enc_stop_task (GstVideoEncoder * encoder, gboolean drain)
{
  GstBmEnc *self = GST_BM_ENC (encoder);
  GstTask *task = encoder->srcpad->task;

  if (!GST_BM_ENC_TASK_STARTED (encoder))
    return;

  GST_DEBUG_OBJECT (self, "stopping encoding thread");

  /* Discard pending frames */
  if (!drain)
    self->pending_frames = 0;

  GST_BM_ENC_BROADCAST (encoder);

  GST_VIDEO_ENCODER_STREAM_UNLOCK (encoder);
  /* Wait for task thread to pause */
  if (task) {
    GST_OBJECT_LOCK (task);
    while (GST_TASK_STATE (task) == GST_TASK_STARTED)
      GST_TASK_WAIT (task);
    GST_OBJECT_UNLOCK (task);
  }

  gst_pad_stop_task (encoder->srcpad);
  GST_VIDEO_ENCODER_STREAM_LOCK (encoder);
}

static void
gst_bm_enc_reset (GstVideoEncoder * encoder, gboolean drain, gboolean final)
{
  GstBmEnc *self = GST_BM_ENC (encoder);

  GST_BM_ENC_LOCK (encoder);

  GST_DEBUG_OBJECT (self, "resetting");

  self->flushing = TRUE;
  self->draining = drain;

  gst_bm_enc_stop_task (encoder, drain);

  self->flushing = final;
  self->draining = FALSE;

  self->task_ret = GST_FLOW_OK;
  self->pending_frames = 0;

  if (self->frames) {
    g_list_free (self->frames);
    self->frames = NULL;
  }

  /* Force re-apply prop */
  self->prop_dirty = TRUE;

  GST_BM_ENC_UNLOCK (encoder);
}

static gboolean
gst_bm_enc_start (GstVideoEncoder * encoder)
{
  GstBmEnc *self = GST_BM_ENC (encoder);
  BmVpuEncReturnCodes ret;

  GST_DEBUG_OBJECT (self, "starting");

  gst_video_info_init (&self->info);

  self->allocator = gst_bm_allocator_new ();
  if (!self->allocator) {
    GST_ERROR_OBJECT (self, "gst_bm_allocator_new fail \n");
    return FALSE;
  }

  self->soc_idx = 0;
  ret = bmvpu_enc_load(self->soc_idx);
  if (ret != BM_VPU_ENC_RETURN_CODE_OK) {
      GST_ERROR_OBJECT(self, "bmvpu_enc_load failed!\n");
      return FALSE;
  }

  bmvpu_enc_set_default_open_params(&self->params, self->codec_type);
  self->params.gop_preset = DEFAULT_PROP_GOP_PRESET;
  self->task_ret = GST_FLOW_OK;
  self->input_state = NULL;
  self->flushing = FALSE;
  self->pending_frames = 0;
  self->frames = NULL;
  self->required_keyframe_number = 0;

  g_mutex_init (&self->mutex);
  g_mutex_init (&self->event_mutex);
  g_cond_init (&self->event_cond);

  GST_DEBUG_OBJECT (self, "started");

  return TRUE;
}

static gboolean
gst_bm_enc_stop (GstVideoEncoder * encoder)
{
  GstBmEnc *self = GST_BM_ENC (encoder);

  GST_DEBUG_OBJECT (self, "stopping");

  GST_VIDEO_ENCODER_STREAM_LOCK (encoder);
  gst_bm_enc_reset (encoder, FALSE, TRUE);
  GST_VIDEO_ENCODER_STREAM_UNLOCK (encoder);

  g_cond_clear (&self->event_cond);
  g_mutex_clear (&self->event_mutex);

  g_mutex_clear (&self->mutex);

  if (self->bmVenc) {
    bmvpu_enc_close (self->bmVenc);
    self->bmVenc = NULL;
    self->is_enc_open = false;
  }

  if (self->allocator) {
    gst_object_unref (self->allocator);
    self->allocator= NULL;
  }

  if (self->input_state)
    gst_video_codec_state_unref (self->input_state);

  if (self->soc_idx >= 0)
  {
      BmVpuEncReturnCodes ret;
      ret = bmvpu_enc_unload(self->soc_idx);
      if (ret != BM_VPU_ENC_RETURN_CODE_OK) {
           GST_ERROR_OBJECT(self,  "Failed to call bmvpu_enc_unload \n");
          return FALSE;
      }else
          self->soc_idx = -1;
  }

  GST_DEBUG_OBJECT (self, "stopped");

  return TRUE;
}

static gboolean
gst_bm_enc_flush (GstVideoEncoder * encoder)
{
  GST_DEBUG_OBJECT (encoder, "flushing");
  gst_bm_enc_reset (encoder, FALSE, FALSE);
  return TRUE;
}

static gboolean
gst_bm_enc_finish (GstVideoEncoder * encoder)
{
  GST_DEBUG_OBJECT (encoder, "finishing");
  gst_bm_enc_reset (encoder, TRUE, FALSE);
  return GST_FLOW_OK;
}

static void
gst_bm_enc_format_info_set(GstBmEnc *self, GstVideoInfo *info) {
  gint hstride, vstride, cstride, c_size = 0, uv_size = 0;
  BmVpuEncPixFormat format;

  hstride = GST_VIDEO_INFO_PLANE_STRIDE (info, 0);
  cstride = GST_VIDEO_INFO_PLANE_STRIDE (info, 1);
  vstride = GST_BM_VIDEO_INFO_VSTRIDE (info);

  switch (GST_VIDEO_INFO_FORMAT (info))
  {
      case GST_VIDEO_FORMAT_I420:
          format = BM_VPU_ENC_PIX_FORMAT_YUV420P;
          // self->params.chroma_interleave = 0;
          c_size = cstride * vstride >> 1;
          uv_size = c_size;
          break;
      case GST_VIDEO_FORMAT_NV12:
          format = BM_VPU_ENC_PIX_FORMAT_NV12;
          // self->params.chroma_interleave = 1;
          c_size = cstride * vstride >> 1;
          uv_size = c_size;
          break;
      case GST_VIDEO_FORMAT_NV21:
          format = BM_VPU_ENC_PIX_FORMAT_NV21;
          // self->params.chroma_interleave = 2;
          c_size = cstride * vstride >> 1;
          uv_size = c_size;
          break;
      case GST_VIDEO_FORMAT_Y42B:
          format = BM_VPU_ENC_PIX_FORMAT_YUV422P;
          // self->params.chroma_interleave = 0;
          c_size = cstride * vstride;
          uv_size = c_size*2;
          break;
      case GST_VIDEO_FORMAT_NV16 :
          format = BM_VPU_ENC_PIX_FORMAT_NV16;
          // self->params.chroma_interleave = 1;
          c_size = cstride * vstride;
          uv_size = c_size;
          break;
      case GST_VIDEO_FORMAT_Y444 :
          format = BM_VPU_ENC_PIX_FORMAT_YUV444P;
          // self->params.chroma_interleave = 0;
          c_size = cstride * vstride;
          uv_size = c_size*2;
          break;
      default:
          format = BM_VPU_ENC_PIX_FORMAT_YUV400 + 1;
          break;
  }

  self->params.pix_format = format;
  self->fb_info.width = GST_VIDEO_INFO_WIDTH (info);
  self->fb_info.height = GST_VIDEO_INFO_HEIGHT (info);
  self->fb_info.y_stride = hstride;
  self->fb_info.c_stride = cstride;
  // self->fb_info.h_stride = vstride;
  self->fb_info.y_size = self->fb_info.y_stride * GST_BM_VIDEO_INFO_VSTRIDE (info);
  self->fb_info.c_size = c_size;
  self->fb_info.size = self->fb_info.y_size + uv_size;
  // self->fb_info.size = self->fb_info.y_size + self->fb_info.c_size *
  //                     (self->params.chroma_interleave ? 1 : 2);
}

static gboolean
gst_bm_enc_set_format (GstVideoEncoder * encoder, GstVideoCodecState * state)
{
  GstBmEnc *self = GST_BM_ENC (encoder);
  GstVideoInfo *info = &self->info;
  gint width, height;

  GST_DEBUG_OBJECT (self, "setting format: %" GST_PTR_FORMAT, state->caps);

  if (self->input_state) {
    if (gst_caps_is_strictly_equal (self->input_state->caps, state->caps)) {
      return TRUE;
    }

    gst_bm_enc_reset (encoder, TRUE, FALSE);

    gst_video_codec_state_unref (self->input_state);
    self->input_state = NULL;
  }

  self->input_state = gst_video_codec_state_ref (state);

  *info = state->info;

  if (!gst_bm_enc_video_info_align (info)) {
    return FALSE;
  }

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

  gst_bm_enc_format_info_set(self, info);
  self->params.frame_width = width;
  self->params.frame_height = height;
  if (self->rotation || self->params.pix_format > BM_VPU_ENC_PIX_FORMAT_YUV400  ||
      width != GST_VIDEO_INFO_WIDTH (info) ||
      height != GST_VIDEO_INFO_HEIGHT (info)) {
      return FALSE;
    /*todo*/
    /* Prefer NV12 when using vpss conversion */
  }


  if (!GST_VIDEO_INFO_FPS_N (info) || GST_VIDEO_INFO_FPS_N (info) > 240) {
    GST_WARNING_OBJECT (self, "framerate (%d/%d) is insane!",
        GST_VIDEO_INFO_FPS_N (info), GST_VIDEO_INFO_FPS_D (info));
    GST_VIDEO_INFO_FPS_N (info) = DEFAULT_FPS;
  }
  self->params.fps_num = GST_VIDEO_INFO_FPS_N (info);
  self->params.fps_den = GST_VIDEO_INFO_FPS_D (info);
  return TRUE;
}

static gboolean
gst_bm_enc_propose_allocation (GstVideoEncoder * encoder, GstQuery * query)
{
  GstBmEnc *self = GST_BM_ENC (encoder);
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

  gst_bm_enc_video_info_align (&info);
  size = GST_VIDEO_INFO_SIZE (&info);

  gst_video_alignment_reset (&align);
  align.padding_right = gst_bm_get_pixel_stride (&info) -
      GST_VIDEO_INFO_WIDTH (&info);
  align.padding_bottom = GST_BM_VIDEO_INFO_VSTRIDE (&info) -
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
gst_bm_enc_convert (GstVideoEncoder * encoder, GstVideoCodecFrame * frame)
{
  GstBmEnc *self = GST_BM_ENC (encoder);
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
  if (!gst_bm_enc_video_info_align (&dst_info)) {
    GST_DEBUG_OBJECT (self,"align fail \n");
    goto convert;
  }

 // gst_bm_enc_apply_strides (encoder, src_hstride, src_vstride);
  if (!gst_bm_enc_apply_properties (encoder)) {
    GST_DEBUG_OBJECT (self,"apply fail \n");
    goto err;
  }

  gst_buffer_append_memory (outbuf, out_mem);
  dst_info = src_info;
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
  dst_info = self->info;
  GST_DEBUG_OBJECT (self, "using software converted buffer");

out:
  gst_buffer_copy_into (outbuf, inbuf,
      GST_BUFFER_COPY_FLAGS | GST_BUFFER_COPY_TIMESTAMPS, 0, 0);


  gst_buffer_add_video_meta_full (outbuf, GST_VIDEO_FRAME_FLAG_NONE,
      GST_VIDEO_INFO_FORMAT (&dst_info),
      GST_VIDEO_INFO_WIDTH (&dst_info), GST_VIDEO_INFO_HEIGHT (&dst_info),
      GST_VIDEO_INFO_N_PLANES (&dst_info), dst_info.offset, dst_info.stride);

  gst_bm_enc_format_info_set(self, &dst_info);

  return outbuf;
err:
  if (out_mem)
    gst_memory_unref (out_mem);

  if (outbuf)
    gst_buffer_unref (outbuf);

  GST_ERROR_OBJECT (self, "failed to convert frame");
  return NULL;
}

static gboolean
gst_bm_enc_force_keyframe (GstVideoEncoder * encoder, gboolean keyframe)
{
  GstBmEnc *self = GST_BM_ENC (encoder);


  if (!keyframe) {
    self->prop_dirty = TRUE;
    return gst_bm_enc_apply_properties (encoder);
  }

  GST_INFO_OBJECT (self, "forcing keyframe");

  return TRUE;
}

static gboolean
gst_bm_enc_send_frame (GstVideoEncoder * encoder)
{
  GstBmEnc *self = GST_BM_ENC (encoder);
  GstVideoCodecFrame *frame;
  GstMemory *mem;
  BmVpuFramebuffer framebuffer;
  gboolean keyframe;
  guint32 frame_number;
  guint32 frame_idx = 0;
  gint ret = 0;
  bm_device_mem_t *fb_dma_buffer;
  BmVpuEncDMABuffer venc_dma_buffer;
  BmVpuEncParams encoding_params;

  memset(&encoding_params, 0, sizeof(BmVpuEncParams));
  encoding_params.customMapOpt = NULL;

  if (!self->frames) {
    if (self->flushing && !self->eos_send) {
        memset(&self->input_frame, 0, sizeof(BmVpuRawFrame));
        ret = bmvpu_enc_send_frame(self->bmVenc, &(self->input_frame), &encoding_params);
        if (!ret){
          g_print("enc eos_send ok \n");
          self->eos_send = TRUE;
        }

    }
    return FALSE;
  }

  frame_number = GPOINTER_TO_UINT (g_list_nth_data (self->frames, 0));
  frame = gst_video_encoder_get_frame (encoder, frame_number);

  keyframe = GST_VIDEO_CODEC_FRAME_IS_FORCE_KEYFRAME (frame);

  if (keyframe) {
    self->required_keyframe_number = frame_number;
    gst_bm_enc_force_keyframe (encoder, TRUE);
  }

  frame_idx = frame_number % BM_PENDING_MAX;
  mem = gst_buffer_peek_memory (frame->output_buffer, 0);
  fb_dma_buffer = gst_bm_allocator_get_bm_buffer (mem);
  if (!fb_dma_buffer) {
    GST_ERROR_OBJECT (self,"gst_bm_allocator get_bm_buffer fail \n");
    gst_video_codec_frame_unref (frame);
    return FALSE;
  }
  GST_DEBUG_OBJECT(self, "width %d, height %d, ", self->fb_info.width, self->fb_info.height);
  GST_DEBUG_OBJECT(self, "y_stride %d, c_stride %d, ", self->fb_info.y_stride, self->fb_info.c_stride);
  // GST_DEBUG_OBJECT(self, "h_stride %d, y_size %d, ", self->fb_info.h_stride, self->fb_info.y_size);
  GST_DEBUG_OBJECT(self, "c_size %d \n", self->fb_info.c_size);

  memset(&venc_dma_buffer, 0, sizeof(BmVpuEncDMABuffer));
  gst_bm_enc_convert_dma_buffer(fb_dma_buffer, &venc_dma_buffer);

  bmvpu_fill_framebuffer_params(&framebuffer, &self->fb_info, &venc_dma_buffer,
                               frame_idx, NULL);

  self->frame_idx[frame_idx] = frame_number;
  memset(&self->input_frame, 0, sizeof(BmVpuRawFrame));
  self->input_frame.framebuffer = &framebuffer;
  self->input_frame.pts = frame->pts;
	self->input_frame.dts = frame->dts;

  ret = bmvpu_enc_send_frame(self->bmVenc, &(self->input_frame), &encoding_params);
  gst_video_codec_frame_unref (frame);
  if (ret){  //send frame may fail,so need resend
    g_usleep (1000);
    return FALSE;
  }

  GST_DEBUG_OBJECT (self, "encoding frame %d", frame_number);
  self->frames = g_list_delete_link (self->frames, self->frames);
  return TRUE;
}

static gboolean
gst_bm_enc_get_stream(GstVideoEncoder * encoder)
{
  GstBmEnc *self = GST_BM_ENC (encoder);
  GstVideoCodecFrame *frame;
  BmVpuEncodedFrame encoded_frame;
  BmVpuEncParams encoding_params;
  guint32 frame_number;
  GstBuffer *buffer;
  gint pkt_size;

  memset(&encoded_frame, 0, sizeof(BmVpuEncodedFrame));

  memset(&encoding_params, 0, sizeof(BmVpuEncParams));
  encoding_params.acquire_output_buffer = acquire_output_buffer;
  encoding_params.finish_output_buffer = finish_output_buffer;
  encoding_params.output_buffer_context = NULL;
  encoding_params.skip_frame = 0;

  bmvpu_enc_get_stream(self->bmVenc, &encoded_frame, &encoding_params);
  if (!encoded_frame.data_size)
    return FALSE;

  /* Wake up the frame producer */
  self->pending_frames--;
  GST_BM_ENC_BROADCAST (encoder);

  g_assert(encoded_frame.src_idx < BM_PENDING_MAX);
  frame_number =  self->frame_idx[encoded_frame.src_idx];
  GST_DEBUG_OBJECT(self, "get stream frame_number %d \n", frame_number);

  frame = gst_video_encoder_get_frame (encoder, frame_number);

  if (self->flushing && !self->draining)
    goto drop;


  if (frame->system_frame_number == self->required_keyframe_number) {
    gst_bm_enc_force_keyframe (encoder, FALSE);
    self->required_keyframe_number = 0;
  }

  pkt_size = encoded_frame.data_size;
  {
    buffer = gst_video_encoder_allocate_output_buffer (encoder, pkt_size);
    if (!buffer)
      goto error;

    gst_buffer_fill (buffer, 0, encoded_frame.data, pkt_size);
  }

  gst_buffer_replace (&frame->output_buffer, buffer);
  gst_buffer_unref (buffer);

  GST_DEBUG_OBJECT (self, "finish frame ts=%" GST_TIME_FORMAT,
      GST_TIME_ARGS (frame->pts));

  gst_video_encoder_finish_frame (encoder, frame);

  if (encoded_frame.data) {
    free(encoded_frame.data);
  }

out:
  return TRUE;
error:
  GST_WARNING_OBJECT (self, "process cur frame fail");
drop:
  GST_DEBUG_OBJECT (self, "drop frame");
  gst_buffer_replace (&frame->output_buffer, NULL);
  gst_video_encoder_finish_frame (encoder, frame);
  goto out;
}

static void
gst_bm_enc_thread (GstVideoEncoder * encoder)
{
  GstBmEnc *self = GST_BM_ENC (encoder);
  gboolean ret = false;
  GST_BM_ENC_WAIT (encoder, self->pending_frames || self->flushing);

  GST_VIDEO_ENCODER_STREAM_LOCK (encoder);

  if (self->flushing && !self->pending_frames) {
    GST_INFO_OBJECT (self, "flushing");
    self->task_ret = GST_FLOW_FLUSHING;
    goto out;
  }
  do {
     ret = gst_bm_enc_send_frame(encoder);
  }while (ret);

  do {
    ret = gst_bm_enc_get_stream(encoder);
  }while(ret);

out:
  if (self->task_ret != GST_FLOW_OK) {
    GST_DEBUG_OBJECT (self, "leaving output thread: %s",
        gst_flow_get_name (self->task_ret));

    gst_pad_pause_task (encoder->srcpad);
  }

  GST_VIDEO_ENCODER_STREAM_UNLOCK (encoder);
}

static GstFlowReturn
gst_bm_enc_handle_frame (GstVideoEncoder * encoder, GstVideoCodecFrame * frame)
{
  GstBmEnc *self = GST_BM_ENC (encoder);
  GstBuffer *buffer;
  GstFlowReturn ret = GST_FLOW_OK;

  GST_DEBUG_OBJECT (self, "handling frame %d", frame->system_frame_number);

  GST_BM_ENC_LOCK (encoder);
  if (!self->is_enc_open) {
    if (!gst_bm_enc_apply_properties (encoder)) {
      GST_DEBUG_OBJECT (self,"apply fail \n");
      return GST_FLOW_ERROR;
    }


    if (bmvpu_enc_open(&self->bmVenc, &self->params, NULL, &(self->initial_info))) {
      g_print("bmvpu_enc_open open failed!\n");
      GST_BM_ENC_UNLOCK (encoder);
      return GST_FLOW_ERROR;
    }

    // if (bmvpu_enc_get_initial_info(self->bmVenc, &(self->initial_info))) {
    //     GST_ERROR_OBJECT(self, "bmvpu_enc_get_initial_info failed!\n");
    //     bmvpu_enc_close(self->bmVenc);
    //     GST_BM_ENC_UNLOCK (encoder);
    //     return GST_FLOW_ERROR;
    // }
    // self->fb_info = self->initial_info.src_fb;
    self->is_enc_open = true;
  }

  if (G_UNLIKELY (self->flushing))
    goto flushing;

  if (G_UNLIKELY (!GST_BM_ENC_TASK_STARTED (encoder))) {
    GST_DEBUG_OBJECT (self, "starting encoding thread");

    gst_pad_start_task (encoder->srcpad,
        (GstTaskFunction) gst_bm_enc_thread, encoder, NULL);
  }
  buffer = gst_bm_enc_convert (encoder, frame);
  if (G_UNLIKELY (!buffer))
    goto not_negotiated;

  frame->output_buffer = buffer;

  if (G_UNLIKELY (self->pending_frames >= BM_PENDING_MAX)) {
    GST_VIDEO_ENCODER_STREAM_UNLOCK (encoder);
    GST_BM_ENC_WAIT (encoder, self->pending_frames < BM_PENDING_MAX
        || self->flushing);
    GST_VIDEO_ENCODER_STREAM_LOCK (encoder);
  }

  if (G_UNLIKELY (self->flushing))
    goto flushing;

  self->pending_frames++;
  self->frames =
      g_list_append (self->frames,
      GUINT_TO_POINTER (frame->system_frame_number));

  GST_BM_ENC_BROADCAST (encoder);
  gst_video_codec_frame_unref (frame);
  GST_BM_ENC_UNLOCK (encoder);
  return self->task_ret;

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
  GST_BM_ENC_UNLOCK (encoder);

  return ret;
}

static GstStateChangeReturn
gst_bm_enc_change_state (GstElement * element, GstStateChange transition)
{
  GstVideoEncoder *encoder = GST_VIDEO_ENCODER (element);

  if (transition == GST_STATE_CHANGE_PAUSED_TO_READY) {
    GST_VIDEO_ENCODER_STREAM_LOCK (encoder);
    gst_bm_enc_reset (encoder, FALSE, TRUE);
    GST_VIDEO_ENCODER_STREAM_UNLOCK (encoder);
  }

  return GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
}

static void
gst_bm_enc_init (GstBmEnc * self)
{
  self->codec_type = BM_VPU_CODEC_FORMAT_H264;
  self->gop = DEFAULT_PROP_GOP;
  self->max_reenc = DEFAULT_PROP_MAX_REENC;
  self->bps = DEFAULT_PROP_BPS;
  self->change_pos = DEFAULT_PROP_BPS_CHANGE_POS;
  self->prop_dirty = TRUE;
  self->gop_preset = DEFAULT_PROP_GOP_PRESET;
  self->cqp = DEFAULT_PROP_CQP;
  self->soc_idx = -1;
  self->is_enc_open = false;
}

static void
gst_bm_enc_class_init (GstBmEncClass * klass)
{
  GstVideoEncoderClass *encoder_class = GST_VIDEO_ENCODER_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  GST_DEBUG_CATEGORY_INIT (gst_bm_enc_debug, "bmenc", 0, "sophon encoder");

  encoder_class->start = GST_DEBUG_FUNCPTR (gst_bm_enc_start);
  encoder_class->stop = GST_DEBUG_FUNCPTR (gst_bm_enc_stop);
  encoder_class->flush = GST_DEBUG_FUNCPTR (gst_bm_enc_flush);
  encoder_class->finish = GST_DEBUG_FUNCPTR (gst_bm_enc_finish);
  encoder_class->set_format = GST_DEBUG_FUNCPTR (gst_bm_enc_set_format);
  encoder_class->propose_allocation =
      GST_DEBUG_FUNCPTR (gst_bm_enc_propose_allocation);
  encoder_class->handle_frame = GST_DEBUG_FUNCPTR (gst_bm_enc_handle_frame);

  gobject_class->set_property = GST_DEBUG_FUNCPTR (gst_bm_enc_set_property);
  gobject_class->get_property = GST_DEBUG_FUNCPTR (gst_bm_enc_get_property);

  g_object_class_install_property (gobject_class, PROP_GOP,
      g_param_spec_int ("gop", "Group of pictures",
          "Group of pictures starting with I frame (-1 = FPS, 1 = all I frames)",
          -1, G_MAXINT, DEFAULT_PROP_GOP,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_GOP_PRESET,
      g_param_spec_enum ("gop-preset", "Group of pictures preset",
          "A GOP structure preset option (2 = default)",
          GST_BM_ENC_GOP_PRESET_TYPE, DEFAULT_PROP_GOP_PRESET,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_BPS,
      g_param_spec_uint ("bps", "Target BPS",
          "Target BPS (0 = auto calculate)",
          0, G_MAXINT, DEFAULT_PROP_BPS,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
#if 0
  g_object_class_install_property (gobject_class, PROP_BPS_CHANGE_POS,
      g_param_spec_uint ("bps-change-pos", "bps change pos",
          "bps change pos (20 = auto calculate)",
          20, 100, DEFAULT_PROP_BPS_CHANGE_POS,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
#endif
  g_object_class_install_property (gobject_class, PROP_PRESET,
      g_param_spec_enum ("speed-preset", "Speed preset",
          "Preset name for speed/quality tradeoff options",
          GST_BM_ENC_PRESET_TYPE, DEFAULT_PROP_PRESET,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DEALT_QP,
      g_param_spec_int ("dealt-qp", "Delta QP between I and P",
          "Delta QP between I and P (-2 = default)",
          -4, 8, DEFAULT_PROP_DEALT_QP,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_CQP,
      g_param_spec_int ("cqp", "quantization parameter for constant quality mode",
          "constant quality mode (-1 = default)",
          -1, 51, DEFAULT_PROP_CQP,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
#if 0
  g_object_class_install_property (gobject_class, PROP_MB_RC,
      g_param_spec_uint ("mb-rc", "MB-level/CU-level rate control",
          "Enable MB-level/CU-level rate control (1 = default)",
          0, 1, DEFAULT_PROP_MB_RC,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
#endif
  element_class->change_state = GST_DEBUG_FUNCPTR (gst_bm_enc_change_state);
}
