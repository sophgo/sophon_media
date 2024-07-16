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

#ifndef __GST_BM_JPEG_ENC_H__
#define __GST_BM_JPEG_ENC_H__

#include <gst/gst.h>
#include <gst/video/video.h>
#include "bm_jpeg_interface.h"

G_BEGIN_DECLS

#define GST_TYPE_BM_JPEG_ENC (gst_bm_jpeg_enc_get_type())
G_DECLARE_FINAL_TYPE (GstBmJpegEnc, gst_bm_jpeg_enc, GST, BM_JPEG_ENC, GstVideoEncoder);

#define GST_BM_JPEG_ENC(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
    GST_TYPE_BM_JPEG_ENC, GstBmJpegEnc))

gboolean gst_bm_jpeg_enc_register (GstPlugin * plugin, guint rank);

G_END_DECLS

#endif /* __GST_BM_JPEG_ENC_H__ */