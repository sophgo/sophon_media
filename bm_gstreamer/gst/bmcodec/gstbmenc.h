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

#ifndef __GST_BM_ENC_H__
#define __GST_BM_ENC_H__

#include <gst/gst.h>
#include <gst/video/video.h>
#include "bm_vpuenc_interface.h"
#include "bmvpuapi_common.h"

G_BEGIN_DECLS

#define GST_TYPE_BM_ENC (gst_bm_enc_get_type())
G_DECLARE_FINAL_TYPE (GstBmEnc, gst_bm_enc, GST, BM_ENC, GstVideoEncoder);

#define GST_BM_ENC(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
    GST_TYPE_BM_ENC, GstBmEnc))

#define BM_PENDING_MAX 32      /* Max number of BMVPU pending frame */

struct _GstBmEnc
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

  /* frame system numbers that are ready for sending to bmvpu */
  GList *frames;

  gboolean eos_send;

  guint32 required_keyframe_number;

  guint pending_frames;
  GMutex event_mutex;
  GCond event_cond;

  /* flow return from pad task */
  GstFlowReturn task_ret;

  BmVpuEncOpenParams params;
  BmVpuFbInfo   fb_info;
  BmVpuEncInitialInfo initial_info;
  gint soc_idx;

  gint rotation;
  gint width;
  gint height;

  gint frame_idx[BM_PENDING_MAX];

  gint gop;
  guint gop_preset;
  guint max_reenc;

  guint bps;
  guint change_pos;
  guint bps_min;
  guint bps_max;

  gboolean prop_dirty;

  BmVpuCodecFormat codec_type;
  BmVpuRawFrame  input_frame;
  BmVpuEncoder  *bmVenc;
};

#define BM_ENC_IN_FORMATS \
    "NV12, I420, N21, NV16, YUV422, YUV444" \

#ifdef HAVE_VPSS
#define BM_ENC_FORMATS BM_ENC_IN_FORMATS
#else
#define BM_ENC_FORMATS BM_ENC_IN_FORMATS
#endif

gboolean gst_bm_enc_apply_properties (GstVideoEncoder * encoder);
gboolean gst_bm_enc_set_src_caps (GstVideoEncoder * encoder, GstCaps * caps);

gboolean gst_bm_enc_supported (BmVpuCodecFormat codec_type);

G_END_DECLS

#endif /* __GST_BM_ENC_H__ */