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

#include "gstbmh265enc.h"


#define GST_BM_H265_ENC(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
    GST_TYPE_BM_H265_ENC, GstBmH265Enc))

#define GST_CAT_DEFAULT bm_h265_enc_debug
GST_DEBUG_CATEGORY (GST_CAT_DEFAULT);

struct _GstBmH265Enc
{
  GstBmEnc parent;

  guint qp_init;
  guint qp_min;
  guint qp_max;
  guint qp_min_i;
  guint qp_max_i;
};

#define parent_class gst_bm_h265_enc_parent_class
G_DEFINE_TYPE (GstBmH265Enc, gst_bm_h265_enc, GST_TYPE_BM_ENC);

#define DEFAULT_PROP_QP_INIT 26
#define DEFAULT_PROP_QP_MIN 1   /* Auto */
#define DEFAULT_PROP_QP_MAX 51  /* Auto */
#define DEFAULT_PROP_QP_MIN_I 1 /* Auto */
#define DEFAULT_PROP_QP_MAX_I 51 /* Auto */
#define DEFAULT_PROP_QP_IP -2   /* Auto */

enum
{
  PROP_0,
  PROP_QP_INIT,
  PROP_QP_MIN,
  PROP_QP_MAX,
  PROP_QP_MIN_I,
  PROP_QP_MAX_I,
  PROP_LAST,
};

#define GST_BM_H265_ENC_SIZE_CAPS \
    "width  = (int) [ 64, MAX ], height = (int) [ 64, MAX ]"

static GstStaticPadTemplate gst_bm_h265_enc_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-h265, "
        GST_BM_H265_ENC_SIZE_CAPS ","
        "stream-format = (string) { byte-stream }, "
        "alignment = (string) { au } ")
        );

static GstStaticPadTemplate gst_bm_h265_enc_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw,"
        "format = (string) { " BM_ENC_FORMATS " }, "
        GST_BM_H265_ENC_SIZE_CAPS));

static void
gst_bm_h265_enc_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GstVideoEncoder *encoder = GST_VIDEO_ENCODER (object);
  GstBmH265Enc *self = GST_BM_H265_ENC (encoder);
  GstBmEnc *bmenc = GST_BM_ENC (encoder);

  switch (prop_id) {
    case PROP_QP_INIT:{
      guint qp_init = g_value_get_uint (value);
      if (self->qp_init == qp_init)
        return;

      self->qp_init = qp_init;
      break;
    }
    case PROP_QP_MIN:{
      guint qp_min = g_value_get_uint (value);
      if (self->qp_min == qp_min)
        return;

      self->qp_min = qp_min;
      break;
    }
    case PROP_QP_MAX:{
      guint qp_max = g_value_get_uint (value);
      if (self->qp_max == qp_max)
        return;

      self->qp_max = qp_max;
      break;
    }
    case PROP_QP_MIN_I:{
      guint qp_min_i = g_value_get_uint (value);
      if (self->qp_min_i == qp_min_i)
        return;

      self->qp_min_i = qp_min_i;
      break;
    }
    case PROP_QP_MAX_I:{
      guint qp_max_i = g_value_get_uint (value);
      if (self->qp_max_i == qp_max_i)
        return;

      self->qp_max_i = qp_max_i;
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      return;
  }

  bmenc->prop_dirty = TRUE;
}

static void
gst_bm_h265_enc_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  GstVideoEncoder *encoder = GST_VIDEO_ENCODER (object);
  GstBmH265Enc *self = GST_BM_H265_ENC (encoder);

  switch (prop_id) {
    case PROP_QP_INIT:
      g_value_set_uint (value, self->qp_init);
      break;
    case PROP_QP_MIN:
      g_value_set_uint (value, self->qp_min);
      break;
    case PROP_QP_MAX:
      g_value_set_uint (value, self->qp_max);
      break;
    case PROP_QP_MIN_I:
      g_value_set_uint (value, self->qp_min_i);
      break;
    case PROP_QP_MAX_I:
      g_value_set_uint (value, self->qp_max_i);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
gst_bm_h265_enc_set_src_caps (GstVideoEncoder * encoder)
{
  GstStructure *structure;
  GstCaps *caps;

  caps = gst_caps_new_empty_simple ("video/x-h265");
  structure = gst_caps_get_structure (caps, 0);
  gst_structure_set (structure, "stream-format",
      G_TYPE_STRING, "byte-stream", NULL);
  gst_structure_set (structure, "alignment", G_TYPE_STRING, "au", NULL);

  return gst_bm_enc_set_src_caps (encoder, caps);
}

static gboolean
gst_bm_h265_enc_apply_properties (GstVideoEncoder * encoder)
{
  GstBmH265Enc *self = GST_BM_H265_ENC (encoder);
  GstBmEnc *bmenc = GST_BM_ENC (encoder);

  if (G_LIKELY (!bmenc->prop_dirty))
    return TRUE;

  bmenc->params.min_qp =  self->qp_min;
  bmenc->params.max_qp =  self->qp_max;

  if (!gst_bm_enc_apply_properties (encoder))
    return FALSE;

  return gst_bm_h265_enc_set_src_caps (encoder);
}

static gboolean
gst_bm_h265_enc_set_format (GstVideoEncoder * encoder,
    GstVideoCodecState * state)
{
  GstVideoEncoderClass *pclass = GST_VIDEO_ENCODER_CLASS (parent_class);

  if (!pclass->set_format (encoder, state))
    return FALSE;

  return gst_bm_h265_enc_apply_properties (encoder);
}

static GstFlowReturn
gst_bm_h265_enc_handle_frame (GstVideoEncoder * encoder,
    GstVideoCodecFrame * frame)
{
  GstVideoEncoderClass *pclass = GST_VIDEO_ENCODER_CLASS (parent_class);

  if (G_UNLIKELY (!gst_bm_h265_enc_apply_properties (encoder))) {
    gst_video_codec_frame_unref (frame);
    return GST_FLOW_NOT_NEGOTIATED;
  }

  return pclass->handle_frame (encoder, frame);
}

static void
gst_bm_h265_enc_init (GstBmH265Enc * self)
{
  self->parent.codec_type = BM_VPU_CODEC_FORMAT_H265;
  self->qp_init = DEFAULT_PROP_QP_INIT;
  self->qp_min = DEFAULT_PROP_QP_MIN;
  self->qp_max = DEFAULT_PROP_QP_MAX;
  self->qp_min_i = DEFAULT_PROP_QP_MIN_I;
  self->qp_max_i = DEFAULT_PROP_QP_MAX_I;
}

static void
gst_bm_h265_enc_class_init (GstBmH265EncClass * klass)
{

  GstVideoEncoderClass *encoder_class = GST_VIDEO_ENCODER_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "bmh265enc", 0,
      "BM H265 ex");

  encoder_class->set_format = GST_DEBUG_FUNCPTR (gst_bm_h265_enc_set_format);
  encoder_class->handle_frame =
      GST_DEBUG_FUNCPTR (gst_bm_h265_enc_handle_frame);

  gobject_class->set_property =
      GST_DEBUG_FUNCPTR (gst_bm_h265_enc_set_property);
  gobject_class->get_property =
      GST_DEBUG_FUNCPTR (gst_bm_h265_enc_get_property);

  g_object_class_install_property (gobject_class, PROP_QP_INIT,
      g_param_spec_uint ("qp-init", "Initial QP",
          "Initial QP (lower value means higher quality)",
          0, 51, DEFAULT_PROP_QP_INIT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_QP_MIN,
      g_param_spec_uint ("qp-min", "Min QP",
          "Min QP (1 = default)", 0, 51, DEFAULT_PROP_QP_MIN,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_QP_MAX,
      g_param_spec_uint ("qp-max", "Max QP",
          "Max QP (51 = default)", 0, 51, DEFAULT_PROP_QP_MAX,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
#if 0
  g_object_class_install_property (gobject_class, PROP_QP_MIN_I,
      g_param_spec_uint ("qp-min-i", "Min Intra QP",
          "Min Intra QP (0 = default)", 0, 51, DEFAULT_PROP_QP_MIN_I,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_QP_MAX_I,
      g_param_spec_uint ("qp-max-i", "Max Intra QP",
          "Max Intra QP (0 = default)", 0, 51, DEFAULT_PROP_QP_MAX_I,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
#endif
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_bm_h265_enc_src_template));

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_bm_h265_enc_sink_template));

  gst_element_class_set_static_metadata (element_class,
      "SOPHGO H265 Encoder", "Codec/Encoder/Video",
      "Encode video streams via SOPHGO",
      "xx <xx@sophgo.com>"
      );
}

gboolean
gst_bm_h265_enc_register (GstPlugin * plugin, guint rank)
{
  if (!gst_bm_enc_supported (BM_VPU_CODEC_FORMAT_H265))
    return FALSE;

  return gst_element_register (plugin, "bmh265enc", rank,
      gst_bm_h265_enc_get_type ());
}
