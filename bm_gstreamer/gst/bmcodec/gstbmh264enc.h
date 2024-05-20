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

#ifndef  __GST_BM_H264_ENC_H__
#define  __GST_BM_H264_ENC_H__

#include "gstbmenc.h"

G_BEGIN_DECLS;

#define GST_TYPE_BM_H264_ENC (gst_bm_h264_enc_get_type())
G_DECLARE_FINAL_TYPE (GstBmH264Enc, gst_bm_h264_enc, GST,
    BM_H264_ENC, GstBmEnc);

gboolean gst_bm_h264_enc_register (GstPlugin * plugin, guint rank);

G_END_DECLS;

#endif /* __GST_BM_H264_ENC_H__ */