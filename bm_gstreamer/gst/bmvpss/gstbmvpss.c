#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "gstbmvpss.h"
#include "gstbmallocator.h"
#include <gst/base/gstbasetransform.h>
#include <gst/allocators/gstdmabuf.h>

GST_DEBUG_CATEGORY (gst_bm_vpss_debug);
#define GST_CAT_DEFAULT gst_bm_vpss_debug

enum
{
  PROP_0,
  PROP_LEFT,
  PROP_RIGHT,
  PROP_TOP,
  PROP_BOTTOM,
  PROP_PADDING_STX,
  PROP_PADDING_STY,
  PROP_PADDING_W,
  PROP_PADDING_H,
  PROP_PADDING_R,
  PROP_PADDING_G,
  PROP_PADDING_B,
  PROP_DRAWRECT_LEFT,
  PROP_DRAWRECT_TOP,
  PROP_DRAWRECT_RIGHT,
  PROP_DRAWRECT_BOTTOM,
  PROP_DRAWRECT_LINEWIDTH,
  PROP_DRAWRECT_R,
  PROP_DRAWRECT_G,
  PROP_DRAWRECT_B,
  PROP_DRAWCIRCLE_X,
  PROP_DRAWCIRCLE_Y,
  PROP_DRAWCIRCLE_RADIUS,
  PROP_DRAWCIRCLE_LINEWIDTH,
  PROP_DRAWCIRCLE_R,
  PROP_DRAWCIRCLE_G,
  PROP_DRAWCIRCLE_B,
  PROP_MOSAIC_LEFT,
  PROP_MOSAIC_TOP,
  PROP_MOSAIC_RIGHT,
  PROP_MOSAIC_BOTTOM,
  PROP_MOSAIC_EXPAND,
  PROP_OVERLAY_PATH,
  PROP_OVERLAY_LEFT,
  PROP_OVERLAY_TOP,
  PROP_OVERLAY_WIDTH,
  PROP_OVERLAY_HEIGHT,
  PROP_OVERLAY_FORMAT,
  PROP_OVERLAY_STRIDE,
  PROP_PIPELINE_LEFT,
  PROP_PIPELINE_RIGHT,
  PROP_PIPELINE_TOP,
  PROP_PIPELINE_BOTTOM,
  PROP_PIPELINE_PADDING_STX,
  PROP_PIPELINE_PADDING_STY,
  PROP_PIPELINE_PADDING_W,
  PROP_PIPELINE_PADDING_H,
  PROP_PIPELINE_PADDING_R,
  PROP_PIPELINE_PADDING_G,
  PROP_PIPELINE_PADDING_B,
  PROP_PIPELINE_INTERPOLATION,
  PROP_PIPELINE_CSC,
  PROP_PIPELINE_CTO_ALPHA0,
  PROP_PIPELINE_CTO_ALPHA1,
  PROP_PIPELINE_CTO_ALPHA2,
  PROP_PIPELINE_CTO_BETA0,
  PROP_PIPELINE_CTO_BETA1,
  PROP_PIPELINE_CTO_BETA2,
  PROP_PIPELINE_FLIP,
  PROP_PIPELINE_OVERLAY_PATH,
  PROP_PIPELINE_OVERLAY_LEFT,
  PROP_PIPELINE_OVERLAY_TOP,
  PROP_PIPELINE_OVERLAY_WIDTH,
  PROP_PIPELINE_OVERLAY_HEIGHT,
  PROP_PIPELINE_OVERLAY_FORMAT,
  PROP_PIPELINE_OVERLAY_STRIDE,
  PROP_PIPELINE_DRAWRECT_LEFT,
  PROP_PIPELINE_DRAWRECT_TOP,
  PROP_PIPELINE_DRAWRECT_RIGHT,
  PROP_PIPELINE_DRAWRECT_BOTTOM,
  PROP_PIPELINE_DRAWRECT_LINEWIDTH,
  PROP_PIPELINE_DRAWRECT_R,
  PROP_PIPELINE_DRAWRECT_G,
  PROP_PIPELINE_DRAWRECT_B,
  PROP_PIPELINE_FILLRECT_LEFT,
  PROP_PIPELINE_FILLRECT_TOP,
  PROP_PIPELINE_FILLRECT_RIGHT,
  PROP_PIPELINE_FILLRECT_BOTTOM,
  PROP_PIPELINE_FILLRECT_R,
  PROP_PIPELINE_FILLRECT_G,
  PROP_PIPELINE_FILLRECT_B,
  PROP_PIPELINE_CIRCLE_X,
  PROP_PIPELINE_CIRCLE_Y,
  PROP_PIPELINE_CIRCLE_RADIUS,
  PROP_PIPELINE_CIRCLE_LINEWIDTH,
  PROP_PIPELINE_CIRCLE_R,
  PROP_PIPELINE_CIRCLE_G,
  PROP_PIPELINE_CIRCLE_B
};

#define PROP_PIPELINE_FIRST PROP_PIPELINE_LEFT
#define PROP_PIPELINE_LAST  PROP_PIPELINE_CIRCLE_B

static void gst_bm_vpss_register_op (GstBmVPSS * self, GstBmVpssOpType op);
static GstBmVpssOpType gst_bm_vpss_op_from_prop (guint prop_id);
static gboolean gst_bm_vpss_op_is_enabled (GstBmVPSS * self, GstBmVpssOpType op);
static gboolean gst_bm_vpss_has_later_enabled_op (GstBmVPSS * self, guint idx);
static void gst_bm_vpss_ensure_op_order (GstBmVPSS * self);
static void gst_bm_vpss_update_drawrect_enable (GstBmVPSS * self);
static void gst_bm_vpss_update_drawcircle_enable (GstBmVPSS * self);
static void gst_bm_vpss_update_mosaic_enable (GstBmVPSS * self);
static void gst_bm_vpss_update_overlay_enable (GstBmVPSS * self);
static void gst_bm_vpss_invalidate_overlay (GstBmVPSS * self);
static gboolean gst_bm_vpss_overlay_format_from_string (const gchar * str,
    bm_image_format_ext * out_fmt);
static bm_status_t gst_bm_vpss_load_overlay_image (GstBmVPSS * self,
    bm_handle_t handle, gint width, gint height, bm_image_format_ext fmt,
    gint user_stride, const gchar *path, bm_image * out_img,
    gboolean * out_valid);
static bm_status_t gst_bm_vpss_load_overlay (GstBmVPSS * self, bm_handle_t handle);
static void gst_bm_vpss_destroy_temp (bm_image * img, gboolean * valid);
static bm_status_t gst_bm_vpss_alloc_temp (bm_handle_t handle, bm_image * img,
    gboolean * valid, int height, int width, bm_image_format_ext fmt);
static void gst_bm_vpss_release_device (GstBmVPSS * self);

static gboolean gst_bm_vpss_legacy_is_active (GstBmVPSS * self);
static int gst_bm_vpss_pipeline_op_canonical_index (GstBmVpssPipelineOpType op);
static gint gst_bm_vpss_pipeline_op_from_prop (guint prop_id);
static gboolean gst_bm_vpss_register_pipeline_op (GstBmVPSS * self,
    GstBmVpssPipelineOpType op);
static gboolean gst_bm_vpss_validate_pipeline_op_order (GstBmVPSS * self);
static void gst_bm_vpss_update_pipeline_crop_enable (GstBmVPSS * self);
static void gst_bm_vpss_update_pipeline_padding_enable (GstBmVPSS * self);
static void gst_bm_vpss_update_pipeline_convertto_enable (GstBmVPSS * self);
static void gst_bm_vpss_update_pipeline_flip_enable (GstBmVPSS * self);
static void gst_bm_vpss_update_pipeline_overlay_enable (GstBmVPSS * self);
static void gst_bm_vpss_update_pipeline_drawrect_enable (GstBmVPSS * self);
static void gst_bm_vpss_update_pipeline_fillrect_enable (GstBmVPSS * self);
static void gst_bm_vpss_update_pipeline_circle_enable (GstBmVPSS * self);
static void gst_bm_vpss_invalidate_pipeline_overlay (GstBmVPSS * self);
static bm_status_t gst_bm_vpss_load_pipeline_overlay (GstBmVPSS * self,
    bm_handle_t handle);
static gboolean gst_bm_vpss_pipeline_csc_from_string (const gchar * str,
    csc_type_t * out);
static gboolean gst_bm_vpss_pipeline_flip_from_string (const gchar * str,
    bmcv_flip_mode * out);
static gboolean gst_bm_vpss_pipeline_interpolation_from_string (const gchar * str,
    bmcv_resize_algorithm * out);
static csc_type_t gst_bm_vpss_resolve_pipeline_csc (GstBmVPSS * self,
    GstVideoFormat in_fmt, GstVideoFormat out_fmt);
static guint32 gst_bm_vpss_pack_rgb_color (guint8 r, guint8 g, guint8 b);
static gboolean gst_bm_vpss_pipeline_op_was_registered (GstBmVPSS * self,
    GstBmVpssPipelineOpType op);
static gboolean gst_bm_vpss_validate_pipeline_enabled_ops (GstBmVPSS * self);
static GstFlowReturn gst_bm_vpss_transform_frame_pipeline (GstBmVPSS * self,
    bm_handle_t bm_handle, bm_image * src, bm_image * dst,
    int in_width, int in_height, int out_width, int out_height,
    bm_image_format_ext out_bm_fmt, GstVideoFormat in_gst_fmt,
    GstVideoFormat out_gst_fmt, GstVideoFrame * outframe,
    gboolean dst_host_mem);

static gboolean gst_bm_vpss_start (GstBaseTransform * trans);
static gboolean gst_bm_vpss_stop (GstBaseTransform * trans);
static GstCaps *gst_bm_vpss_transform_caps(GstBaseTransform *trans,
                                          GstPadDirection direction,
                                          GstCaps *caps, GstCaps *filter);
static GstCaps *gst_bm_vpss_fixate_caps(GstBaseTransform *trans,
                                       GstPadDirection direction,
                                       GstCaps *caps,
                                       GstCaps *othercaps);
static GstFlowReturn gst_bm_vpss_transform_frame(GstVideoFilter *filter,
    GstVideoFrame *inframe, GstVideoFrame *outframe);

static int map_gstformat_to_bmformat(GstVideoFormat gst_format);

static void
gst_bm_vpss_register_op (GstBmVPSS * self, GstBmVpssOpType op)
{
  guint i;
  for (i = 0; i < self->op_order_len; i++) {
    if (self->op_order[i] == op)
      return;
  }
  if (self->op_order_len < GST_BM_VPSS_MAX_OPS)
    self->op_order[self->op_order_len++] = op;
}

static GstBmVpssOpType
gst_bm_vpss_op_from_prop (guint prop_id)
{
  if (prop_id >= PROP_LEFT && prop_id <= PROP_BOTTOM)
    return GST_BM_VPSS_OP_CROP;
  if (prop_id >= PROP_PADDING_STX && prop_id <= PROP_PADDING_B)
    return GST_BM_VPSS_OP_PADDING;
  if (prop_id >= PROP_DRAWRECT_LEFT && prop_id <= PROP_DRAWRECT_B)
    return GST_BM_VPSS_OP_DRAWRECT;
  if (prop_id >= PROP_DRAWCIRCLE_X && prop_id <= PROP_DRAWCIRCLE_B)
    return GST_BM_VPSS_OP_DRAWCIRCLE;
  if (prop_id >= PROP_MOSAIC_LEFT && prop_id <= PROP_MOSAIC_EXPAND)
    return GST_BM_VPSS_OP_MOSAIC;
  if (prop_id >= PROP_OVERLAY_PATH && prop_id <= PROP_OVERLAY_STRIDE)
    return GST_BM_VPSS_OP_OVERLAY;
  return -1;
}

static gboolean
gst_bm_vpss_op_is_enabled (GstBmVPSS * self, GstBmVpssOpType op)
{
  switch (op) {
    case GST_BM_VPSS_OP_PADDING: return self->padding_enable;
    case GST_BM_VPSS_OP_CROP: return self->crop_enable;
    case GST_BM_VPSS_OP_DRAWRECT: return self->drawrect_enable;
    case GST_BM_VPSS_OP_DRAWCIRCLE: return self->drawcircle_enable;
    case GST_BM_VPSS_OP_MOSAIC: return self->mosaic_enable;
    case GST_BM_VPSS_OP_OVERLAY: return self->overlay_enable;
    default: return FALSE;
  }
}

static gboolean
gst_bm_vpss_has_later_enabled_op (GstBmVPSS * self, guint from_index)
{
  guint j;
  for (j = from_index + 1; j < self->op_order_len; j++) {
    if (gst_bm_vpss_op_is_enabled (self, self->op_order[j]))
      return TRUE;
  }
  return FALSE;
}

static void
gst_bm_vpss_ensure_op_order (GstBmVPSS * self)
{
  if (self->op_order_len > 0)
    return;
  if (self->padding_enable)
    gst_bm_vpss_register_op (self, GST_BM_VPSS_OP_PADDING);
  if (self->crop_enable)
    gst_bm_vpss_register_op (self, GST_BM_VPSS_OP_CROP);
  if (self->drawrect_enable)
    gst_bm_vpss_register_op (self, GST_BM_VPSS_OP_DRAWRECT);
  if (self->drawcircle_enable)
    gst_bm_vpss_register_op (self, GST_BM_VPSS_OP_DRAWCIRCLE);
  if (self->mosaic_enable)
    gst_bm_vpss_register_op (self, GST_BM_VPSS_OP_MOSAIC);
  if (self->overlay_enable)
    gst_bm_vpss_register_op (self, GST_BM_VPSS_OP_OVERLAY);
}

static void
gst_bm_vpss_update_drawrect_enable (GstBmVPSS * self)
{
  self->drawrect_enable = (self->drawrect_bottom > self->drawrect_top &&
      self->drawrect_right > self->drawrect_left);
}

static void
gst_bm_vpss_update_drawcircle_enable (GstBmVPSS * self)
{
  self->drawcircle_enable = (self->drawcircle_radius > 0);
}

static void
gst_bm_vpss_update_mosaic_enable (GstBmVPSS * self)
{
  self->mosaic_enable = (self->mosaic_bottom > self->mosaic_top &&
      self->mosaic_right > self->mosaic_left);
}

static void
gst_bm_vpss_update_overlay_enable (GstBmVPSS * self)
{
  self->overlay_enable = (self->overlay_path != NULL &&
      self->overlay_path[0] != '\0' && self->overlay_width > 0 &&
      self->overlay_height > 0 && self->overlay_format_valid);
}

static void
gst_bm_vpss_invalidate_overlay (GstBmVPSS * self)
{
  if (self->overlay_bm_image_valid) {
    bm_image_destroy (&self->overlay_bm_image);
    self->overlay_bm_image_valid = FALSE;
  }
}

static gboolean
gst_bm_vpss_overlay_format_from_string (const gchar * str,
    bm_image_format_ext * out_fmt)
{
  if (!str || !str[0]) {
    *out_fmt = FORMAT_ARGB1555_PACKED;
    return TRUE;
  }
  if (!g_ascii_strcasecmp (str, "argb8888")) {
    *out_fmt = FORMAT_ARGB_PACKED;
    return TRUE;
  }
  if (!g_ascii_strcasecmp (str, "argb1555")) {
    *out_fmt = FORMAT_ARGB1555_PACKED;
    return TRUE;
  }
  if (!g_ascii_strcasecmp (str, "argb4444")) {
    *out_fmt = FORMAT_ARGB4444_PACKED;
    return TRUE;
  }
  return FALSE;
}

static int
gst_bm_vpss_overlay_bpp (bm_image_format_ext fmt)
{
  switch (fmt) {
    case FORMAT_ARGB_PACKED:
      return 4;
    case FORMAT_ARGB1555_PACKED:
    case FORMAT_ARGB4444_PACKED:
      return 2;
    default:
      return 0;
  }
}

static int
gst_bm_vpss_overlay_default_stride (gint width, bm_image_format_ext fmt)
{
  int bpp = gst_bm_vpss_overlay_bpp (fmt);
  if (bpp <= 0 || width <= 0)
    return 0;
  return ((width * bpp) + 15) & ~15;
}

static gboolean
gst_bm_vpss_overlay_validate_file (GstBmVPSS * self, const gchar * path,
    gint width, gint height, bm_image_format_ext fmt, gsize file_size,
    int * stride_out)
{
  int default_stride, bpp;
  const gsize nv12_256 = (gsize) 256 * 256 * 3 / 2;

  if (!path || !path[0] || width <= 0 || height <= 0)
    return FALSE;

  bpp = gst_bm_vpss_overlay_bpp (fmt);
  default_stride = gst_bm_vpss_overlay_default_stride (width, fmt);
  if (bpp <= 0 || default_stride <= 0)
    return FALSE;

  if (g_str_has_suffix (path, ".nv12") || g_str_has_suffix (path, ".NV12")) {
    GST_ERROR_OBJECT (self,
        "overlay file '%s' is NV12; overlay requires ARGB8888/1555/4444 raw",
        path);
    return FALSE;
  }

  if (file_size == nv12_256) {
    GST_ERROR_OBJECT (self,
        "overlay file '%s' size %zu matches 256x256 NV12, not ARGB overlay",
        path, file_size);
    return FALSE;
  }

  if (file_size > 0) {
    gsize yuv420 = (gsize) width * height * 3 / 2;
    if (file_size == yuv420) {
      GST_ERROR_OBJECT (self,
          "overlay file '%s' size matches NV12 %dx%d, not ARGB overlay",
          path, width, height);
      return FALSE;
    }
  }

  if (*stride_out <= 0) {
    if (file_size > 0 && file_size % (gsize) height == 0) {
      int inferred = (int) (file_size / height);
      if (inferred >= default_stride)
        *stride_out = inferred;
      else
        *stride_out = default_stride;
    } else {
      *stride_out = default_stride;
    }
  }

  if (file_size > 0 && file_size < (gsize) height * (*stride_out)) {
    GST_ERROR_OBJECT (self,
        "overlay file '%s' too small: need >= %d bytes (%dx%d stride=%d), got %zu",
        path, height * (*stride_out), width, height, *stride_out, file_size);
    return FALSE;
  }

  /* Detect full-frame bin used with a smaller overlay size (stride mismatch) */
  if (file_size > 0 && bpp > 0) {
    gsize pixels = file_size / (gsize) bpp;
    int file_stride = 0;

    if (file_size % (gsize) height == 0)
      file_stride = (int) (file_size / height);
    if (file_stride > 0 && file_stride != *stride_out &&
        file_stride > default_stride) {
      GST_ERROR_OBJECT (self,
          "overlay file '%s' size %zu implies stride %d for height %d, "
          "but properties are %dx%d stride=%d; match file layout or use a "
          "cropped overlay bin",
          path, file_size, file_stride, height, width, height, *stride_out);
      return FALSE;
    }

    if (pixels == (gsize) 512 * 256 &&
        (width != 512 || height != 256)) {
      GST_ERROR_OBJECT (self,
          "overlay file '%s' is 512x256 ARGB (%zu bytes), not %dx%d; "
          "use pipelineoverlaywidth=512 pipelineoverlayheight=256 "
          "pipelineoverlaystride=1024, or a cropped %dx%d bin (stride %d)",
          path, file_size, width, height, width, height, default_stride);
      return FALSE;
    }
    if (pixels == (gsize) 256 * 256 &&
        (width != 256 || height != 256)) {
      GST_ERROR_OBJECT (self,
          "overlay file '%s' is 256x256 ARGB (%zu bytes), not %dx%d; "
          "use pipelineoverlaywidth=256 pipelineoverlayheight=256 "
          "pipelineoverlaystride=512, or a cropped %dx%d bin (stride %d)",
          path, file_size, width, height, width, height, default_stride);
      return FALSE;
    }
  }

  GST_DEBUG_OBJECT (self,
      "overlay '%s' %dx%d stride=%d file_size=%zu", path, width, height,
      *stride_out, file_size);
  return TRUE;
}

static bm_status_t
gst_bm_vpss_load_overlay_image (GstBmVPSS * self, bm_handle_t handle,
    gint width, gint height, bm_image_format_ext fmt, gint user_stride,
    const gchar * path, bm_image * out_img, gboolean * out_valid)
{
  bm_status_t ret;
  struct stat st;
  gsize file_size = 0;
  int stride = user_stride;
  int stride_arr[1];

  if (stat (path, &st) == 0)
    file_size = (gsize) st.st_size;

  if (!gst_bm_vpss_overlay_validate_file (self, path, width, height, fmt,
          file_size, &stride))
    return BM_ERR_PARAM;

  if (*out_valid) {
    bm_image_destroy (out_img);
    *out_valid = FALSE;
  }

  stride_arr[0] = stride;
  ret = bm_image_create (handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE,
      out_img, stride_arr);
  if (ret != BM_SUCCESS) {
    GST_ERROR_OBJECT (self, "overlay bm_image_create failed: %d", ret);
    return ret;
  }

  ret = bm_image_alloc_dev_mem (*out_img, BMCV_HEAP1_ID);
  if (ret != BM_SUCCESS) {
    bm_image_destroy (out_img);
    GST_ERROR_OBJECT (self, "overlay bm_image_alloc_dev_mem failed: %d", ret);
    return ret;
  }

  bm_read_bin (*out_img, path);
  *out_valid = TRUE;
  return BM_SUCCESS;
}

static bm_status_t
gst_bm_vpss_load_overlay (GstBmVPSS * self, bm_handle_t handle)
{
  return gst_bm_vpss_load_overlay_image (self, handle, self->overlay_width,
      self->overlay_height, self->overlay_bm_format, self->overlay_stride,
      self->overlay_path, &self->overlay_bm_image, &self->overlay_bm_image_valid);
}

static void
gst_bm_vpss_destroy_temp (bm_image * img, gboolean * valid)
{
  if (*valid) {
    bm_image_destroy (img);
    *valid = FALSE;
  }
}

static bm_status_t
gst_bm_vpss_alloc_temp (bm_handle_t handle, bm_image * img, gboolean * valid,
    int height, int width, bm_image_format_ext fmt)
{
  bm_status_t ret;
  gst_bm_vpss_destroy_temp (img, valid);
  bm_image_create (handle, height, width, fmt, DATA_TYPE_EXT_1N_BYTE, img, NULL);
  ret = bm_image_alloc_dev_mem (*img, BMCV_HEAP_ANY);
  if (ret != BM_SUCCESS) {
    bm_image_destroy (img);
    *valid = FALSE;
    return ret;
  }
  *valid = TRUE;
  return BM_SUCCESS;
}

static void
gst_bm_vpss_release_device (GstBmVPSS * self)
{
  gst_bm_vpss_invalidate_overlay (self);
  gst_bm_vpss_invalidate_pipeline_overlay (self);
  if (self->bm_handle_valid) {
    bm_dev_free (self->bm_handle);
    self->bm_handle = NULL;
    self->bm_handle_valid = FALSE;
  }
}

static gboolean
gst_bm_vpss_legacy_is_active (GstBmVPSS * self)
{
  return self->crop_enable || self->padding_enable || self->drawrect_enable ||
      self->drawcircle_enable || self->mosaic_enable || self->overlay_enable ||
      self->op_order_len > 0;
}

static int
gst_bm_vpss_pipeline_op_canonical_index (GstBmVpssPipelineOpType op)
{
  switch (op) {
    case GST_BM_VPSS_PIPELINE_OP_CROP: return 0;
    case GST_BM_VPSS_PIPELINE_OP_RESIZE: return 1;
    case GST_BM_VPSS_PIPELINE_OP_CSC: return 2;
    case GST_BM_VPSS_PIPELINE_OP_PADDING: return 3;
    case GST_BM_VPSS_PIPELINE_OP_CONVERTTO: return 4;
    case GST_BM_VPSS_PIPELINE_OP_FLIP: return 5;
    case GST_BM_VPSS_PIPELINE_OP_OVERLAY: return 6;
    case GST_BM_VPSS_PIPELINE_OP_DRAWRECT: return 7;
    case GST_BM_VPSS_PIPELINE_OP_FILLRECT: return 8;
    case GST_BM_VPSS_PIPELINE_OP_CIRCLE: return 9;
    default: return -1;
  }
}

static gboolean
gst_bm_vpss_register_pipeline_op (GstBmVPSS * self, GstBmVpssPipelineOpType op)
{
  guint i;
  int new_idx, max_idx = -1;

  for (i = 0; i < self->pipeline_op_order_len; i++) {
    if (self->pipeline_op_order[i] == op)
      return TRUE;
    max_idx = MAX (max_idx,
        gst_bm_vpss_pipeline_op_canonical_index (self->pipeline_op_order[i]));
  }

  new_idx = gst_bm_vpss_pipeline_op_canonical_index (op);
  if (new_idx < 0)
    return FALSE;
  if (max_idx >= 0 && new_idx < max_idx) {
    GST_ERROR_OBJECT (self,
        "pipeline property order invalid (required: "
        "Crop->Resize->CSC->Padding->ConvertTo->Flip->Overlay->"
        "DrawRect->FillRect->Circle)");
    return FALSE;
  }

  if (self->pipeline_op_order_len < GST_BM_VPSS_MAX_PIPELINE_OPS) {
    self->pipeline_op_order[self->pipeline_op_order_len++] = op;
    return TRUE;
  }
  return FALSE;
}

static gboolean
gst_bm_vpss_pipeline_op_was_registered (GstBmVPSS * self,
    GstBmVpssPipelineOpType op)
{
  guint i;
  for (i = 0; i < self->pipeline_op_order_len; i++) {
    if (self->pipeline_op_order[i] == op)
      return TRUE;
  }
  return FALSE;
}

static gboolean
gst_bm_vpss_validate_pipeline_enabled_ops (GstBmVPSS * self)
{
  if (self->pipeline_crop_enable &&
      !gst_bm_vpss_pipeline_op_was_registered (self,
          GST_BM_VPSS_PIPELINE_OP_CROP))
    return FALSE;
  if (self->pipeline_padding_enable &&
      !gst_bm_vpss_pipeline_op_was_registered (self,
          GST_BM_VPSS_PIPELINE_OP_PADDING))
    return FALSE;
  if (self->pipeline_csc_explicit &&
      !gst_bm_vpss_pipeline_op_was_registered (self,
          GST_BM_VPSS_PIPELINE_OP_CSC))
    return FALSE;
  if (self->pipeline_convertto_enable &&
      !gst_bm_vpss_pipeline_op_was_registered (self,
          GST_BM_VPSS_PIPELINE_OP_CONVERTTO))
    return FALSE;
  if (self->pipeline_flip_enable &&
      !gst_bm_vpss_pipeline_op_was_registered (self,
          GST_BM_VPSS_PIPELINE_OP_FLIP))
    return FALSE;
  if (self->pipeline_overlay_enable &&
      !gst_bm_vpss_pipeline_op_was_registered (self,
          GST_BM_VPSS_PIPELINE_OP_OVERLAY))
    return FALSE;
  if (self->pipeline_drawrect_enable &&
      !gst_bm_vpss_pipeline_op_was_registered (self,
          GST_BM_VPSS_PIPELINE_OP_DRAWRECT))
    return FALSE;
  if (self->pipeline_fillrect_enable &&
      !gst_bm_vpss_pipeline_op_was_registered (self,
          GST_BM_VPSS_PIPELINE_OP_FILLRECT))
    return FALSE;
  if (self->pipeline_circle_enable &&
      !gst_bm_vpss_pipeline_op_was_registered (self,
          GST_BM_VPSS_PIPELINE_OP_CIRCLE))
    return FALSE;
  return TRUE;
}

static gint
gst_bm_vpss_pipeline_op_from_prop (guint prop_id)
{
  if (prop_id >= PROP_PIPELINE_LEFT && prop_id <= PROP_PIPELINE_BOTTOM)
    return GST_BM_VPSS_PIPELINE_OP_CROP;
  if (prop_id >= PROP_PIPELINE_PADDING_STX && prop_id <= PROP_PIPELINE_PADDING_B)
    return GST_BM_VPSS_PIPELINE_OP_PADDING;
  if (prop_id == PROP_PIPELINE_CSC)
    return GST_BM_VPSS_PIPELINE_OP_CSC;
  if (prop_id >= PROP_PIPELINE_CTO_ALPHA0 && prop_id <= PROP_PIPELINE_CTO_BETA2)
    return GST_BM_VPSS_PIPELINE_OP_CONVERTTO;
  if (prop_id == PROP_PIPELINE_FLIP)
    return GST_BM_VPSS_PIPELINE_OP_FLIP;
  if (prop_id >= PROP_PIPELINE_OVERLAY_PATH &&
      prop_id <= PROP_PIPELINE_OVERLAY_FORMAT)
    return GST_BM_VPSS_PIPELINE_OP_OVERLAY;
  if (prop_id >= PROP_PIPELINE_DRAWRECT_LEFT &&
      prop_id <= PROP_PIPELINE_DRAWRECT_B)
    return GST_BM_VPSS_PIPELINE_OP_DRAWRECT;
  if (prop_id >= PROP_PIPELINE_FILLRECT_LEFT &&
      prop_id <= PROP_PIPELINE_FILLRECT_B)
    return GST_BM_VPSS_PIPELINE_OP_FILLRECT;
  if (prop_id >= PROP_PIPELINE_CIRCLE_X && prop_id <= PROP_PIPELINE_CIRCLE_B)
    return GST_BM_VPSS_PIPELINE_OP_CIRCLE;
  if (prop_id == PROP_PIPELINE_INTERPOLATION)
    return -1;
  return -1;
}

static gboolean
gst_bm_vpss_validate_pipeline_op_order (GstBmVPSS * self)
{
  guint i;
  int max_idx = -1;

  for (i = 0; i < self->pipeline_op_order_len; i++) {
    int idx = gst_bm_vpss_pipeline_op_canonical_index (
        self->pipeline_op_order[i]);
    if (idx < 0)
      continue;
    if (idx < max_idx)
      return FALSE;
    max_idx = idx;
  }
  return TRUE;
}

static void
gst_bm_vpss_update_pipeline_crop_enable (GstBmVPSS * self)
{
  self->pipeline_crop_enable =
      (self->pipeline_crop_right > self->pipeline_crop_left &&
       self->pipeline_crop_bottom > self->pipeline_crop_top);
}

static void
gst_bm_vpss_update_pipeline_padding_enable (GstBmVPSS * self)
{
  self->pipeline_padding_enable =
      (self->pipeline_padding_w > 0 && self->pipeline_padding_h > 0);
}

static void
gst_bm_vpss_update_pipeline_convertto_enable (GstBmVPSS * self)
{
  guint i;
  self->pipeline_convertto_enable = FALSE;
  for (i = 0; i < 3; i++) {
    if (self->pipeline_cto_alpha[i] != 1.0f ||
        self->pipeline_cto_beta[i] != 0.0f) {
      self->pipeline_convertto_enable = TRUE;
      break;
    }
  }
}

static void
gst_bm_vpss_update_pipeline_flip_enable (GstBmVPSS * self)
{
  self->pipeline_flip_enable =
      (self->pipeline_flip_mode != NO_FLIP);
}

static void
gst_bm_vpss_update_pipeline_overlay_enable (GstBmVPSS * self)
{
  self->pipeline_overlay_enable =
      (self->pipeline_overlay_path != NULL &&
       self->pipeline_overlay_path[0] != '\0' &&
       self->pipeline_overlay_width > 0 &&
       self->pipeline_overlay_height > 0 &&
       self->pipeline_overlay_format_valid);
}

static void
gst_bm_vpss_update_pipeline_drawrect_enable (GstBmVPSS * self)
{
  self->pipeline_drawrect_enable =
      (self->pipeline_drawrect_bottom > self->pipeline_drawrect_top &&
       self->pipeline_drawrect_right > self->pipeline_drawrect_left);
}

static void
gst_bm_vpss_update_pipeline_fillrect_enable (GstBmVPSS * self)
{
  self->pipeline_fillrect_enable =
      (self->pipeline_fillrect_bottom > self->pipeline_fillrect_top &&
       self->pipeline_fillrect_right > self->pipeline_fillrect_left);
}

static void
gst_bm_vpss_update_pipeline_circle_enable (GstBmVPSS * self)
{
  self->pipeline_circle_enable = (self->pipeline_circle_radius > 0);
}

static void
gst_bm_vpss_invalidate_pipeline_overlay (GstBmVPSS * self)
{
  if (self->pipeline_overlay_bm_image_valid) {
    bm_image_destroy (&self->pipeline_overlay_bm_image);
    self->pipeline_overlay_bm_image_valid = FALSE;
  }
}

static bm_status_t
gst_bm_vpss_load_pipeline_overlay (GstBmVPSS * self, bm_handle_t handle)
{
  gst_bm_vpss_invalidate_pipeline_overlay (self);
  return gst_bm_vpss_load_overlay_image (self, handle,
      self->pipeline_overlay_width, self->pipeline_overlay_height,
      self->pipeline_overlay_bm_format, self->pipeline_overlay_stride,
      self->pipeline_overlay_path, &self->pipeline_overlay_bm_image,
      &self->pipeline_overlay_bm_image_valid);
}

static gboolean
gst_bm_vpss_pipeline_csc_from_string (const gchar * str, csc_type_t * out)
{
  if (!str || !str[0] || !g_ascii_strcasecmp (str, "auto"))
    return FALSE;
  if (!g_ascii_strcasecmp (str, "max") ||
      !g_ascii_strcasecmp (str, "none")) {
    *out = CSC_MAX_ENUM;
    return TRUE;
  }
  if (!g_ascii_strcasecmp (str, "yuv2rgb_bt601")) {
    *out = CSC_YCbCr2RGB_BT601;
    return TRUE;
  }
  if (!g_ascii_strcasecmp (str, "rgb2yuv_bt601")) {
    *out = CSC_RGB2YCbCr_BT601;
    return TRUE;
  }
  if (!g_ascii_strcasecmp (str, "yuv2rgb_bt709")) {
    *out = CSC_YCbCr2RGB_BT709;
    return TRUE;
  }
  if (!g_ascii_strcasecmp (str, "rgb2yuv_bt709")) {
    *out = CSC_RGB2YCbCr_BT709;
    return TRUE;
  }
  return FALSE;
}

static gboolean
gst_bm_vpss_pipeline_flip_from_string (const gchar * str, bmcv_flip_mode * out)
{
  if (!str || !str[0] || !g_ascii_strcasecmp (str, "none")) {
    *out = NO_FLIP;
    return TRUE;
  }
  if (!g_ascii_strcasecmp (str, "horizontal")) {
    *out = HORIZONTAL_FLIP;
    return TRUE;
  }
  if (!g_ascii_strcasecmp (str, "vertical")) {
    *out = VERTICAL_FLIP;
    return TRUE;
  }
  if (!g_ascii_strcasecmp (str, "rotate180") ||
      !g_ascii_strcasecmp (str, "rotate_180")) {
    *out = ROTATE_180;
    return TRUE;
  }
  return FALSE;
}

static gboolean
gst_bm_vpss_pipeline_interpolation_from_string (const gchar * str,
    bmcv_resize_algorithm * out)
{
  if (!str || !str[0]) {
    *out = BMCV_INTER_LINEAR;
    return TRUE;
  }
  if (!g_ascii_strcasecmp (str, "nearest")) {
    *out = BMCV_INTER_NEAREST;
    return TRUE;
  }
  if (!g_ascii_strcasecmp (str, "linear")) {
    *out = BMCV_INTER_LINEAR;
    return TRUE;
  }
  if (!g_ascii_strcasecmp (str, "bicubic")) {
    *out = BMCV_INTER_BICUBIC;
    return TRUE;
  }
  if (!g_ascii_strcasecmp (str, "area")) {
    *out = BMCV_INTER_AREA;
    return TRUE;
  }
  return FALSE;
}

static gboolean
gst_bm_vpss_gst_format_is_yuv (GstVideoFormat fmt)
{
  switch (fmt) {
    case GST_VIDEO_FORMAT_I420:
    case GST_VIDEO_FORMAT_Y42B:
    case GST_VIDEO_FORMAT_Y444:
    case GST_VIDEO_FORMAT_NV12:
    case GST_VIDEO_FORMAT_NV21:
    case GST_VIDEO_FORMAT_NV16:
    case GST_VIDEO_FORMAT_NV61:
    case GST_VIDEO_FORMAT_YUY2:
    case GST_VIDEO_FORMAT_YVYU:
    case GST_VIDEO_FORMAT_UYVY:
    case GST_VIDEO_FORMAT_VYUY:
      return TRUE;
    default:
      return FALSE;
  }
}

static gboolean
gst_bm_vpss_gst_format_is_rgb (GstVideoFormat fmt)
{
  return (fmt == GST_VIDEO_FORMAT_RGB || fmt == GST_VIDEO_FORMAT_BGR);
}

static csc_type_t
gst_bm_vpss_resolve_pipeline_csc (GstBmVPSS * self, GstVideoFormat in_fmt,
    GstVideoFormat out_fmt)
{
  if (self->pipeline_csc_explicit)
    return self->pipeline_csc_type;
  if (in_fmt == out_fmt)
    return CSC_MAX_ENUM;
  if (gst_bm_vpss_gst_format_is_yuv (in_fmt) &&
      gst_bm_vpss_gst_format_is_rgb (out_fmt))
    return CSC_YCbCr2RGB_BT601;
  if (gst_bm_vpss_gst_format_is_rgb (in_fmt) &&
      gst_bm_vpss_gst_format_is_yuv (out_fmt))
    return CSC_RGB2YCbCr_BT601;
  if (gst_bm_vpss_gst_format_is_yuv (in_fmt) &&
      gst_bm_vpss_gst_format_is_yuv (out_fmt))
    return CSC_MAX_ENUM;
  if (gst_bm_vpss_gst_format_is_rgb (in_fmt) &&
      gst_bm_vpss_gst_format_is_rgb (out_fmt))
    return CSC_MAX_ENUM;
  return CSC_MAX_ENUM;
}

static guint32
gst_bm_vpss_pack_rgb_color (guint8 r, guint8 g, guint8 b)
{
  return ((guint32) r << 16) | ((guint32) g << 8) | (guint32) b;
}

static GstFlowReturn
gst_bm_vpss_transform_frame_pipeline (GstBmVPSS * self, bm_handle_t bm_handle,
    bm_image * src, bm_image * dst, int in_width, int in_height,
    int out_width, int out_height, bm_image_format_ext out_bm_fmt,
    GstVideoFormat in_gst_fmt, GstVideoFormat out_gst_fmt,
    GstVideoFrame * outframe, gboolean dst_host_mem)
{
  GstMemory *outmem;
  bm_status_t bm_ret;
  bm_image pipeline_out;
  bm_image *out_img = dst;
  gboolean pipeline_out_valid = FALSE;
  bmcv_rect_t crop_rect, *p_crop = NULL;
  bmcv_padding_attr_t pad_attr, *p_pad = NULL;
  bmcv_convert_to_attr cto_attr, *p_cto = NULL;
  bmcv_overlay_attr ov_attr, *p_ov = NULL;
  bmcv_rect_t ov_info;
  bmcv_draw_rect_attr dr_attr, *p_dr = NULL;
  bmcv_fill_rect_attr fr_attr, *p_fr = NULL;
  bmcv_circle_attr cir_attr, *p_cir = NULL;
  csc_type_t csc_type;
  bmcv_flip_mode flip_mode;

  if (!gst_bm_vpss_validate_pipeline_op_order (self) ||
      !gst_bm_vpss_validate_pipeline_enabled_ops (self)) {
    GST_ERROR_OBJECT (self,
        "invalid pipeline op order (configure properties in order: "
        "Crop->...->Padding->...->Circle)");
    return GST_FLOW_ERROR;
  }
  if (gst_bm_vpss_legacy_is_active (self)) {
    GST_ERROR_OBJECT (self,
        "pipeline* and legacy properties cannot be used together");
    return GST_FLOW_ERROR;
  }

  if (!outframe || !outframe->buffer) {
    GST_ERROR_OBJECT (self, "invalid output frame");
    return GST_FLOW_ERROR;
  }
  outmem = gst_buffer_peek_memory (outframe->buffer, 0);
  if (!outmem) {
    GST_ERROR_OBJECT (self, "output buffer has no memory");
    return GST_FLOW_ERROR;
  }

  if (self->pipeline_crop_enable) {
    crop_rect.start_x = (unsigned int) self->pipeline_crop_left;
    crop_rect.start_y = (unsigned int) self->pipeline_crop_top;
    crop_rect.crop_w =
        (unsigned int) (self->pipeline_crop_right - self->pipeline_crop_left);
    crop_rect.crop_h =
        (unsigned int) (self->pipeline_crop_bottom - self->pipeline_crop_top);
    if (crop_rect.start_x + crop_rect.crop_w > (unsigned int) in_width ||
        crop_rect.start_y + crop_rect.crop_h > (unsigned int) in_height) {
      GST_ERROR_OBJECT (self, "pipeline crop out of input bounds");
      return GST_FLOW_ERROR;
    }
    p_crop = &crop_rect;
  }

  if (self->pipeline_padding_enable) {
    pad_attr.dst_crop_stx = (unsigned int) self->pipeline_padding_stx;
    pad_attr.dst_crop_sty = (unsigned int) self->pipeline_padding_sty;
    pad_attr.dst_crop_w = (unsigned int) self->pipeline_padding_w;
    pad_attr.dst_crop_h = (unsigned int) self->pipeline_padding_h;
    pad_attr.padding_r = self->pipeline_padding_r;
    pad_attr.padding_g = self->pipeline_padding_g;
    pad_attr.padding_b = self->pipeline_padding_b;
    pad_attr.if_memset = self->pipeline_padding_if_memset;
    if (pad_attr.dst_crop_stx + pad_attr.dst_crop_w > (unsigned int) out_width ||
        pad_attr.dst_crop_sty + pad_attr.dst_crop_h > (unsigned int) out_height) {
      GST_ERROR_OBJECT (self, "pipeline padding out of output bounds");
      return GST_FLOW_ERROR;
    }
    p_pad = &pad_attr;
  }

  csc_type = gst_bm_vpss_resolve_pipeline_csc (self, in_gst_fmt, out_gst_fmt);
  flip_mode = self->pipeline_flip_enable ?
      self->pipeline_flip_mode : NO_FLIP;

  if (self->pipeline_convertto_enable) {
    cto_attr.alpha_0 = self->pipeline_cto_alpha[0];
    cto_attr.beta_0 = self->pipeline_cto_beta[0];
    cto_attr.alpha_1 = self->pipeline_cto_alpha[1];
    cto_attr.beta_1 = self->pipeline_cto_beta[1];
    cto_attr.alpha_2 = self->pipeline_cto_alpha[2];
    cto_attr.beta_2 = self->pipeline_cto_beta[2];
    p_cto = &cto_attr;
  }

  if (self->pipeline_overlay_enable) {
    int rw, rh;
    if (!self->pipeline_overlay_bm_image_valid) {
      bm_ret = gst_bm_vpss_load_pipeline_overlay (self, bm_handle);
      if (bm_ret != BM_SUCCESS) {
        GST_ERROR_OBJECT (self, "pipeline overlay load failed: %d", bm_ret);
        return GST_FLOW_ERROR;
      }
    }
    ov_info.start_x = (unsigned int) self->pipeline_overlay_left;
    ov_info.start_y = (unsigned int) self->pipeline_overlay_top;
    ov_info.crop_w = (unsigned int) self->pipeline_overlay_width;
    ov_info.crop_h = (unsigned int) self->pipeline_overlay_height;
    rw = (int) ov_info.crop_w;
    rh = (int) ov_info.crop_h;
    if (ov_info.start_x + ov_info.crop_w > (unsigned int) out_width ||
        ov_info.start_y + ov_info.crop_h > (unsigned int) out_height) {
      GST_ERROR_OBJECT (self, "pipeline overlay out of output bounds");
      return GST_FLOW_ERROR;
    }
    ov_attr.overlay_num = 1;
    ov_attr.overlay_info = &ov_info;
    ov_attr.overlay_image = &self->pipeline_overlay_bm_image;
    p_ov = &ov_attr;
    (void) rw;
    (void) rh;
  }

  if (self->pipeline_drawrect_enable) {
    int rw, rh;
    dr_attr.rect_num = 1;
    dr_attr.draw_rect[0].start_x =
        (unsigned int) self->pipeline_drawrect_left;
    dr_attr.draw_rect[0].start_y =
        (unsigned int) self->pipeline_drawrect_top;
    rw = self->pipeline_drawrect_right - self->pipeline_drawrect_left;
    rh = self->pipeline_drawrect_bottom - self->pipeline_drawrect_top;
    dr_attr.draw_rect[0].crop_w = (unsigned int) rw;
    dr_attr.draw_rect[0].crop_h = (unsigned int) rh;
    dr_attr.color[0] = gst_bm_vpss_pack_rgb_color (self->pipeline_drawrect_r,
        self->pipeline_drawrect_g, self->pipeline_drawrect_b);
    dr_attr.line_width[0] = (short) self->pipeline_drawrect_line_width;
    if (dr_attr.draw_rect[0].start_x + dr_attr.draw_rect[0].crop_w >
            (unsigned int) out_width ||
        dr_attr.draw_rect[0].start_y + dr_attr.draw_rect[0].crop_h >
            (unsigned int) out_height ||
        self->pipeline_drawrect_line_width <= 0 ||
        self->pipeline_drawrect_line_width > rw / 2 ||
        self->pipeline_drawrect_line_width > rh / 2) {
      GST_ERROR_OBJECT (self, "pipeline drawrect invalid");
      return GST_FLOW_ERROR;
    }
    p_dr = &dr_attr;
  }

  if (self->pipeline_fillrect_enable) {
    fr_attr.rect_num = 1;
    fr_attr.fill_rect[0].start_x =
        (unsigned int) self->pipeline_fillrect_left;
    fr_attr.fill_rect[0].start_y =
        (unsigned int) self->pipeline_fillrect_top;
    fr_attr.fill_rect[0].crop_w = (unsigned int)
        (self->pipeline_fillrect_right - self->pipeline_fillrect_left);
    fr_attr.fill_rect[0].crop_h = (unsigned int)
        (self->pipeline_fillrect_bottom - self->pipeline_fillrect_top);
    fr_attr.color[0] = gst_bm_vpss_pack_rgb_color (self->pipeline_fillrect_r,
        self->pipeline_fillrect_g, self->pipeline_fillrect_b);
    if (fr_attr.fill_rect[0].start_x + fr_attr.fill_rect[0].crop_w >
            (unsigned int) out_width ||
        fr_attr.fill_rect[0].start_y + fr_attr.fill_rect[0].crop_h >
            (unsigned int) out_height) {
      GST_ERROR_OBJECT (self, "pipeline fillrect invalid");
      return GST_FLOW_ERROR;
    }
    p_fr = &fr_attr;
  }

  if (self->pipeline_circle_enable) {
    int rad = self->pipeline_circle_radius;
    cir_attr.center.x = self->pipeline_circle_x;
    cir_attr.center.y = self->pipeline_circle_y;
    cir_attr.radius = (short) rad;
    cir_attr.color.r = self->pipeline_circle_r;
    cir_attr.color.g = self->pipeline_circle_g;
    cir_attr.color.b = self->pipeline_circle_b;
    cir_attr.line_width = (signed char) self->pipeline_circle_line_width;
    if (cir_attr.line_width >= 1 && cir_attr.line_width <= 15 &&
        cir_attr.line_width > rad) {
      GST_ERROR_OBJECT (self, "pipeline circle line_width > radius");
      return GST_FLOW_ERROR;
    }
    if (cir_attr.line_width >= 1 && cir_attr.line_width <= 15 &&
        (cir_attr.center.x - rad < 0 || cir_attr.center.y - rad < 0 ||
         cir_attr.center.x + rad >= out_width ||
         cir_attr.center.y + rad >= out_height)) {
      GST_ERROR_OBJECT (self, "pipeline circle out of output bounds");
      return GST_FLOW_ERROR;
    }
    p_cir = &cir_attr;
  }

  if (dst_host_mem) {
    memset (&pipeline_out, 0, sizeof (pipeline_out));
    bm_ret = bm_image_create (bm_handle, out_height, out_width, out_bm_fmt,
        DATA_TYPE_EXT_1N_BYTE, &pipeline_out, NULL);
    if (bm_ret != BM_SUCCESS) {
      GST_ERROR_OBJECT (self, "pipeline output bm_image_create failed: %d",
          bm_ret);
      return GST_FLOW_ERROR;
    }
    pipeline_out_valid = TRUE;
    out_img = &pipeline_out;
  } else if (dst == NULL) {
    GST_ERROR_OBJECT (self, "pipeline output bm_image missing");
    return GST_FLOW_ERROR;
  }

  bm_ret = bmcv_image_csc_overlay (bm_handle, 1, *src, out_img, p_crop, p_pad,
      self->pipeline_algorithm, csc_type, flip_mode, p_cto, p_ov, p_dr, p_fr,
      p_cir);
  if (bm_ret != BM_SUCCESS) {
    GST_ERROR_OBJECT (self, "bmcv_image_csc_overlay failed: %d", bm_ret);
    if (pipeline_out_valid)
      bm_image_destroy (&pipeline_out);
    return GST_FLOW_ERROR;
  }

  if (dst_host_mem) {
    guint8 *dst_in_ptr[4];
    guint i;
    for (i = 0; i < GST_VIDEO_FRAME_N_PLANES (outframe); i++)
      dst_in_ptr[i] = GST_VIDEO_FRAME_PLANE_DATA (outframe, i);
    bm_ret = bm_image_copy_device_to_host (pipeline_out, (void **) dst_in_ptr);
    if (bm_ret != BM_SUCCESS) {
      GST_ERROR_OBJECT (self, "pipeline copy device to host failed: %d", bm_ret);
      bm_image_destroy (&pipeline_out);
      return GST_FLOW_ERROR;
    }
    bm_image_destroy (&pipeline_out);
  }

  return GST_FLOW_OK;
}

/* class initialization */
G_DEFINE_TYPE(GstBmVPSS, gst_bm_vpss, GST_TYPE_VIDEO_FILTER);

static void
gst_bm_vpss_set_property(GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstBmVPSS *self = GST_BM_VPSS (object);
  gint pop;

  if (prop_id >= PROP_PIPELINE_FIRST && prop_id <= PROP_PIPELINE_LAST) {
    if (gst_bm_vpss_legacy_is_active (self)) {
      self->mode_conflict = TRUE;
      GST_ELEMENT_ERROR (GST_ELEMENT (self), RESOURCE, SETTINGS,
          ("Pipeline and legacy properties cannot be used together"),
          ("Refused to set '%s' because legacy properties are already active",
              g_param_spec_get_name (pspec)));
      return;
    }
    self->pipeline_mode_active = TRUE;

    switch (prop_id) {
      case PROP_PIPELINE_LEFT:
        self->pipeline_crop_left = g_value_get_int (value);
        break;
      case PROP_PIPELINE_RIGHT:
        self->pipeline_crop_right = g_value_get_int (value);
        break;
      case PROP_PIPELINE_TOP:
        self->pipeline_crop_top = g_value_get_int (value);
        break;
      case PROP_PIPELINE_BOTTOM:
        self->pipeline_crop_bottom = g_value_get_int (value);
        break;
      case PROP_PIPELINE_PADDING_STX:
        self->pipeline_padding_stx = g_value_get_int (value);
        break;
      case PROP_PIPELINE_PADDING_STY:
        self->pipeline_padding_sty = g_value_get_int (value);
        break;
      case PROP_PIPELINE_PADDING_W:
        self->pipeline_padding_w = g_value_get_int (value);
        break;
      case PROP_PIPELINE_PADDING_H:
        self->pipeline_padding_h = g_value_get_int (value);
        break;
      case PROP_PIPELINE_PADDING_R:
        self->pipeline_padding_r = g_value_get_uint (value);
        self->pipeline_padding_if_memset = 1;
        break;
      case PROP_PIPELINE_PADDING_G:
        self->pipeline_padding_g = g_value_get_uint (value);
        self->pipeline_padding_if_memset = 1;
        break;
      case PROP_PIPELINE_PADDING_B:
        self->pipeline_padding_b = g_value_get_uint (value);
        self->pipeline_padding_if_memset = 1;
        break;
      case PROP_PIPELINE_INTERPOLATION: {
        const gchar *s = g_value_get_string (value);
        bmcv_resize_algorithm alg;
        if (s && gst_bm_vpss_pipeline_interpolation_from_string (s, &alg))
          self->pipeline_algorithm = alg;
        break;
      }
      case PROP_PIPELINE_CSC: {
        const gchar *s = g_value_get_string (value);
        csc_type_t ct;
        g_free (self->pipeline_csc_str);
        self->pipeline_csc_str = (s && s[0]) ? g_strdup (s) : g_strdup ("auto");
        if (gst_bm_vpss_pipeline_csc_from_string (self->pipeline_csc_str, &ct)) {
          self->pipeline_csc_type = ct;
          self->pipeline_csc_explicit = TRUE;
        } else {
          self->pipeline_csc_type = CSC_MAX_ENUM;
          self->pipeline_csc_explicit = FALSE;
        }
        break;
      }
      case PROP_PIPELINE_CTO_ALPHA0:
        self->pipeline_cto_alpha[0] = g_value_get_float (value);
        break;
      case PROP_PIPELINE_CTO_ALPHA1:
        self->pipeline_cto_alpha[1] = g_value_get_float (value);
        break;
      case PROP_PIPELINE_CTO_ALPHA2:
        self->pipeline_cto_alpha[2] = g_value_get_float (value);
        break;
      case PROP_PIPELINE_CTO_BETA0:
        self->pipeline_cto_beta[0] = g_value_get_float (value);
        break;
      case PROP_PIPELINE_CTO_BETA1:
        self->pipeline_cto_beta[1] = g_value_get_float (value);
        break;
      case PROP_PIPELINE_CTO_BETA2:
        self->pipeline_cto_beta[2] = g_value_get_float (value);
        break;
      case PROP_PIPELINE_FLIP: {
        const gchar *s = g_value_get_string (value);
        bmcv_flip_mode fm;
        if (!s || !gst_bm_vpss_pipeline_flip_from_string (s, &fm)) {
          GST_WARNING_OBJECT (self, "invalid pipelineflip value");
          return;
        }
        self->pipeline_flip_mode = fm;
        break;
      }
      case PROP_PIPELINE_OVERLAY_PATH: {
        const gchar *path = g_value_get_string (value);
        g_free (self->pipeline_overlay_path);
        self->pipeline_overlay_path =
            (path && path[0]) ? g_strdup (path) : NULL;
        gst_bm_vpss_invalidate_pipeline_overlay (self);
        break;
      }
      case PROP_PIPELINE_OVERLAY_LEFT:
        self->pipeline_overlay_left = g_value_get_int (value);
        break;
      case PROP_PIPELINE_OVERLAY_TOP:
        self->pipeline_overlay_top = g_value_get_int (value);
        break;
      case PROP_PIPELINE_OVERLAY_WIDTH:
        self->pipeline_overlay_width = g_value_get_int (value);
        gst_bm_vpss_invalidate_pipeline_overlay (self);
        break;
      case PROP_PIPELINE_OVERLAY_STRIDE:
        self->pipeline_overlay_stride = g_value_get_int (value);
        gst_bm_vpss_invalidate_pipeline_overlay (self);
        break;
      case PROP_PIPELINE_OVERLAY_HEIGHT:
        self->pipeline_overlay_height = g_value_get_int (value);
        gst_bm_vpss_invalidate_pipeline_overlay (self);
        break;
      case PROP_PIPELINE_OVERLAY_FORMAT: {
        const gchar *fmt = g_value_get_string (value);
        bm_image_format_ext nf;
        g_free (self->pipeline_overlay_format_str);
        self->pipeline_overlay_format_str =
            (fmt && fmt[0]) ? g_strdup (fmt) : g_strdup ("argb1555");
        self->pipeline_overlay_format_valid =
            gst_bm_vpss_overlay_format_from_string (
                self->pipeline_overlay_format_str, &nf);
        if (self->pipeline_overlay_format_valid)
          self->pipeline_overlay_bm_format = nf;
        gst_bm_vpss_invalidate_pipeline_overlay (self);
        break;
      }
      case PROP_PIPELINE_DRAWRECT_LEFT:
        self->pipeline_drawrect_left = g_value_get_int (value);
        break;
      case PROP_PIPELINE_DRAWRECT_TOP:
        self->pipeline_drawrect_top = g_value_get_int (value);
        break;
      case PROP_PIPELINE_DRAWRECT_RIGHT:
        self->pipeline_drawrect_right = g_value_get_int (value);
        break;
      case PROP_PIPELINE_DRAWRECT_BOTTOM:
        self->pipeline_drawrect_bottom = g_value_get_int (value);
        break;
      case PROP_PIPELINE_DRAWRECT_LINEWIDTH:
        self->pipeline_drawrect_line_width = g_value_get_int (value);
        break;
      case PROP_PIPELINE_DRAWRECT_R:
        self->pipeline_drawrect_r = g_value_get_uint (value);
        break;
      case PROP_PIPELINE_DRAWRECT_G:
        self->pipeline_drawrect_g = g_value_get_uint (value);
        break;
      case PROP_PIPELINE_DRAWRECT_B:
        self->pipeline_drawrect_b = g_value_get_uint (value);
        break;
      case PROP_PIPELINE_FILLRECT_LEFT:
        self->pipeline_fillrect_left = g_value_get_int (value);
        break;
      case PROP_PIPELINE_FILLRECT_TOP:
        self->pipeline_fillrect_top = g_value_get_int (value);
        break;
      case PROP_PIPELINE_FILLRECT_RIGHT:
        self->pipeline_fillrect_right = g_value_get_int (value);
        break;
      case PROP_PIPELINE_FILLRECT_BOTTOM:
        self->pipeline_fillrect_bottom = g_value_get_int (value);
        break;
      case PROP_PIPELINE_FILLRECT_R:
        self->pipeline_fillrect_r = g_value_get_uint (value);
        break;
      case PROP_PIPELINE_FILLRECT_G:
        self->pipeline_fillrect_g = g_value_get_uint (value);
        break;
      case PROP_PIPELINE_FILLRECT_B:
        self->pipeline_fillrect_b = g_value_get_uint (value);
        break;
      case PROP_PIPELINE_CIRCLE_X:
        self->pipeline_circle_x = g_value_get_int (value);
        break;
      case PROP_PIPELINE_CIRCLE_Y:
        self->pipeline_circle_y = g_value_get_int (value);
        break;
      case PROP_PIPELINE_CIRCLE_RADIUS:
        self->pipeline_circle_radius = g_value_get_int (value);
        break;
      case PROP_PIPELINE_CIRCLE_LINEWIDTH:
        self->pipeline_circle_line_width = g_value_get_int (value);
        break;
      case PROP_PIPELINE_CIRCLE_R:
        self->pipeline_circle_r = g_value_get_uint (value);
        break;
      case PROP_PIPELINE_CIRCLE_G:
        self->pipeline_circle_g = g_value_get_uint (value);
        break;
      case PROP_PIPELINE_CIRCLE_B:
        self->pipeline_circle_b = g_value_get_uint (value);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        return;
    }

    gst_bm_vpss_update_pipeline_crop_enable (self);
    gst_bm_vpss_update_pipeline_padding_enable (self);
    gst_bm_vpss_update_pipeline_convertto_enable (self);
    gst_bm_vpss_update_pipeline_flip_enable (self);
    gst_bm_vpss_update_pipeline_overlay_enable (self);
    gst_bm_vpss_update_pipeline_drawrect_enable (self);
    gst_bm_vpss_update_pipeline_fillrect_enable (self);
    gst_bm_vpss_update_pipeline_circle_enable (self);

    pop = gst_bm_vpss_pipeline_op_from_prop (prop_id);
    if (pop == GST_BM_VPSS_PIPELINE_OP_CSC && !self->pipeline_csc_explicit)
      pop = -1;
    if (pop == GST_BM_VPSS_PIPELINE_OP_FLIP && !self->pipeline_flip_enable)
      pop = -1;
    if (pop >= 0)
      gst_bm_vpss_register_pipeline_op (self, (GstBmVpssPipelineOpType) pop);
    return;
  }

  if (self->pipeline_mode_active) {
    self->mode_conflict = TRUE;
    GST_ELEMENT_ERROR (GST_ELEMENT (self), RESOURCE, SETTINGS,
        ("Pipeline and legacy properties cannot be used together"),
        ("Refused to set '%s' because pipeline properties are already active",
            g_param_spec_get_name (pspec)));
    return;
  }

  switch (prop_id) {
    case PROP_LEFT:
      self->crop_left = g_value_get_int(value);
      break;
    case PROP_RIGHT:
      self->crop_right = g_value_get_int(value);
      break;
    case PROP_TOP:
      self->crop_top = g_value_get_int(value);
      break;
    case PROP_BOTTOM:
      self->crop_bottom = g_value_get_int(value);
      break;
    case PROP_PADDING_STX:
      self->padding_dst_stx = g_value_get_int(value);
      break;
    case PROP_PADDING_STY:
      self->padding_dst_sty = g_value_get_int(value);
      break;
    case PROP_PADDING_W:
      self->padding_dst_w = g_value_get_int(value);
      break;
    case PROP_PADDING_H:
      self->padding_dst_h = g_value_get_int(value);
      break;
    case PROP_PADDING_R:
      self->padding_r = g_value_get_uint(value);
      self->padding_if_memset = 1;
      break;
    case PROP_PADDING_G:
      self->padding_g = g_value_get_uint(value);
      self->padding_if_memset = 1;
      break;
    case PROP_PADDING_B:
      self->padding_b = g_value_get_uint(value);
      self->padding_if_memset = 1;
      break;
    case PROP_DRAWRECT_LEFT: self->drawrect_left = g_value_get_int (value); break;
    case PROP_DRAWRECT_TOP: self->drawrect_top = g_value_get_int (value); break;
    case PROP_DRAWRECT_RIGHT: self->drawrect_right = g_value_get_int (value); break;
    case PROP_DRAWRECT_BOTTOM: self->drawrect_bottom = g_value_get_int (value); break;
    case PROP_DRAWRECT_LINEWIDTH: self->drawrect_line_width = g_value_get_int (value); break;
    case PROP_DRAWRECT_R: self->drawrect_r = g_value_get_uint (value); break;
    case PROP_DRAWRECT_G: self->drawrect_g = g_value_get_uint (value); break;
    case PROP_DRAWRECT_B: self->drawrect_b = g_value_get_uint (value); break;
    case PROP_DRAWCIRCLE_X: self->drawcircle_x = g_value_get_int (value); break;
    case PROP_DRAWCIRCLE_Y: self->drawcircle_y = g_value_get_int (value); break;
    case PROP_DRAWCIRCLE_RADIUS: self->drawcircle_radius = g_value_get_int (value); break;
    case PROP_DRAWCIRCLE_LINEWIDTH: self->drawcircle_line_width = g_value_get_int (value); break;
    case PROP_DRAWCIRCLE_R: self->drawcircle_r = g_value_get_uint (value); break;
    case PROP_DRAWCIRCLE_G: self->drawcircle_g = g_value_get_uint (value); break;
    case PROP_DRAWCIRCLE_B: self->drawcircle_b = g_value_get_uint (value); break;
    case PROP_MOSAIC_LEFT: self->mosaic_left = g_value_get_int (value); break;
    case PROP_MOSAIC_TOP: self->mosaic_top = g_value_get_int (value); break;
    case PROP_MOSAIC_RIGHT: self->mosaic_right = g_value_get_int (value); break;
    case PROP_MOSAIC_BOTTOM: self->mosaic_bottom = g_value_get_int (value); break;
    case PROP_MOSAIC_EXPAND: self->mosaic_expand = g_value_get_boolean (value); break;
    case PROP_OVERLAY_PATH: {
      const gchar *path = g_value_get_string (value);
      g_free (self->overlay_path);
      self->overlay_path = (path && path[0]) ? g_strdup (path) : NULL;
      gst_bm_vpss_invalidate_overlay (self);
      break;
    }
    case PROP_OVERLAY_LEFT: self->overlay_left = g_value_get_int (value); break;
    case PROP_OVERLAY_TOP: self->overlay_top = g_value_get_int (value); break;
    case PROP_OVERLAY_WIDTH:
      self->overlay_width = g_value_get_int (value);
      gst_bm_vpss_invalidate_overlay (self);
      break;
    case PROP_OVERLAY_STRIDE:
      self->overlay_stride = g_value_get_int (value);
      gst_bm_vpss_invalidate_overlay (self);
      break;
    case PROP_OVERLAY_HEIGHT:
      self->overlay_height = g_value_get_int (value);
      gst_bm_vpss_invalidate_overlay (self);
      break;
    case PROP_OVERLAY_FORMAT: {
      const gchar *fmt = g_value_get_string (value);
      bm_image_format_ext nf;
      g_free (self->overlay_format_str);
      self->overlay_format_str = (fmt && fmt[0]) ? g_strdup (fmt) : g_strdup ("argb1555");
      self->overlay_format_valid =
          gst_bm_vpss_overlay_format_from_string (self->overlay_format_str, &nf);
      if (self->overlay_format_valid)
        self->overlay_bm_format = nf;
      gst_bm_vpss_invalidate_overlay (self);
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }

  {
    gint op = gst_bm_vpss_op_from_prop (prop_id);
    if (op >= 0)
      gst_bm_vpss_register_op (self, (GstBmVpssOpType) op);
  }

  if (self->crop_bottom > 0 && self->crop_right > 0)
    self->crop_enable = TRUE;
  if (self->padding_dst_w > 0 && self->padding_dst_h > 0)
    self->padding_enable = TRUE;
  if (prop_id >= PROP_DRAWRECT_LEFT && prop_id <= PROP_DRAWRECT_B)
    gst_bm_vpss_update_drawrect_enable (self);
  if (prop_id >= PROP_DRAWCIRCLE_X && prop_id <= PROP_DRAWCIRCLE_B)
    gst_bm_vpss_update_drawcircle_enable (self);
  if (prop_id >= PROP_MOSAIC_LEFT && prop_id <= PROP_MOSAIC_EXPAND)
    gst_bm_vpss_update_mosaic_enable (self);
  if (prop_id >= PROP_OVERLAY_PATH && prop_id <= PROP_OVERLAY_STRIDE)
    gst_bm_vpss_update_overlay_enable (self);
}

static void
gst_bm_vpss_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstBmVPSS *self = GST_BM_VPSS (object);
  switch (prop_id) {
    case PROP_LEFT:
      g_value_set_int (value, self->crop_left);
      break;
    case PROP_RIGHT:
      g_value_set_int (value, self->crop_right);
      break;
    case PROP_TOP:
      g_value_set_int (value, self->crop_top);
      break;
    case PROP_BOTTOM:
      g_value_set_int (value, self->crop_bottom);
      break;
    case PROP_PADDING_STX:
      g_value_set_int (value, self->padding_dst_stx);
      break;
    case PROP_PADDING_STY:
      g_value_set_int (value, self->padding_dst_sty);
      break;
    case PROP_PADDING_W:
      g_value_set_int (value, self->padding_dst_w);
      break;
    case PROP_PADDING_H:
      g_value_set_int (value, self->padding_dst_h);
      break;
    case PROP_PADDING_R:
      g_value_set_int (value, self->padding_r);
      break;
    case PROP_PADDING_G:
      g_value_set_int (value, self->padding_g);
      break;
    case PROP_PADDING_B:
      g_value_set_uint (value, self->padding_b);
      break;
    case PROP_DRAWRECT_LEFT: g_value_set_int (value, self->drawrect_left); break;
    case PROP_DRAWRECT_TOP: g_value_set_int (value, self->drawrect_top); break;
    case PROP_DRAWRECT_RIGHT: g_value_set_int (value, self->drawrect_right); break;
    case PROP_DRAWRECT_BOTTOM: g_value_set_int (value, self->drawrect_bottom); break;
    case PROP_DRAWRECT_LINEWIDTH: g_value_set_int (value, self->drawrect_line_width); break;
    case PROP_DRAWRECT_R: g_value_set_uint (value, self->drawrect_r); break;
    case PROP_DRAWRECT_G: g_value_set_uint (value, self->drawrect_g); break;
    case PROP_DRAWRECT_B: g_value_set_uint (value, self->drawrect_b); break;
    case PROP_DRAWCIRCLE_X: g_value_set_int (value, self->drawcircle_x); break;
    case PROP_DRAWCIRCLE_Y: g_value_set_int (value, self->drawcircle_y); break;
    case PROP_DRAWCIRCLE_RADIUS: g_value_set_int (value, self->drawcircle_radius); break;
    case PROP_DRAWCIRCLE_LINEWIDTH: g_value_set_int (value, self->drawcircle_line_width); break;
    case PROP_DRAWCIRCLE_R: g_value_set_uint (value, self->drawcircle_r); break;
    case PROP_DRAWCIRCLE_G: g_value_set_uint (value, self->drawcircle_g); break;
    case PROP_DRAWCIRCLE_B: g_value_set_uint (value, self->drawcircle_b); break;
    case PROP_MOSAIC_LEFT: g_value_set_int (value, self->mosaic_left); break;
    case PROP_MOSAIC_TOP: g_value_set_int (value, self->mosaic_top); break;
    case PROP_MOSAIC_RIGHT: g_value_set_int (value, self->mosaic_right); break;
    case PROP_MOSAIC_BOTTOM: g_value_set_int (value, self->mosaic_bottom); break;
    case PROP_MOSAIC_EXPAND: g_value_set_boolean (value, self->mosaic_expand); break;
    case PROP_OVERLAY_PATH:
      g_value_set_string (value, self->overlay_path ? self->overlay_path : "");
      break;
    case PROP_OVERLAY_LEFT: g_value_set_int (value, self->overlay_left); break;
    case PROP_OVERLAY_TOP: g_value_set_int (value, self->overlay_top); break;
    case PROP_OVERLAY_WIDTH: g_value_set_int (value, self->overlay_width); break;
    case PROP_OVERLAY_STRIDE: g_value_set_int (value, self->overlay_stride); break;
    case PROP_OVERLAY_HEIGHT: g_value_set_int (value, self->overlay_height); break;
    case PROP_OVERLAY_FORMAT:
      g_value_set_string (value,
          self->overlay_format_str ? self->overlay_format_str : "argb1555");
      break;
    case PROP_PIPELINE_LEFT:
      g_value_set_int (value, self->pipeline_crop_left);
      break;
    case PROP_PIPELINE_RIGHT:
      g_value_set_int (value, self->pipeline_crop_right);
      break;
    case PROP_PIPELINE_TOP:
      g_value_set_int (value, self->pipeline_crop_top);
      break;
    case PROP_PIPELINE_BOTTOM:
      g_value_set_int (value, self->pipeline_crop_bottom);
      break;
    case PROP_PIPELINE_PADDING_STX:
      g_value_set_int (value, self->pipeline_padding_stx);
      break;
    case PROP_PIPELINE_PADDING_STY:
      g_value_set_int (value, self->pipeline_padding_sty);
      break;
    case PROP_PIPELINE_PADDING_W:
      g_value_set_int (value, self->pipeline_padding_w);
      break;
    case PROP_PIPELINE_PADDING_H:
      g_value_set_int (value, self->pipeline_padding_h);
      break;
    case PROP_PIPELINE_PADDING_R:
      g_value_set_uint (value, self->pipeline_padding_r);
      break;
    case PROP_PIPELINE_PADDING_G:
      g_value_set_uint (value, self->pipeline_padding_g);
      break;
    case PROP_PIPELINE_PADDING_B:
      g_value_set_uint (value, self->pipeline_padding_b);
      break;
    case PROP_PIPELINE_INTERPOLATION:
      g_value_set_string (value, "linear");
      break;
    case PROP_PIPELINE_CSC:
      g_value_set_string (value,
          self->pipeline_csc_str ? self->pipeline_csc_str : "auto");
      break;
    case PROP_PIPELINE_CTO_ALPHA0:
      g_value_set_float (value, self->pipeline_cto_alpha[0]);
      break;
    case PROP_PIPELINE_CTO_ALPHA1:
      g_value_set_float (value, self->pipeline_cto_alpha[1]);
      break;
    case PROP_PIPELINE_CTO_ALPHA2:
      g_value_set_float (value, self->pipeline_cto_alpha[2]);
      break;
    case PROP_PIPELINE_CTO_BETA0:
      g_value_set_float (value, self->pipeline_cto_beta[0]);
      break;
    case PROP_PIPELINE_CTO_BETA1:
      g_value_set_float (value, self->pipeline_cto_beta[1]);
      break;
    case PROP_PIPELINE_CTO_BETA2:
      g_value_set_float (value, self->pipeline_cto_beta[2]);
      break;
    case PROP_PIPELINE_FLIP:
      if (self->pipeline_flip_mode == HORIZONTAL_FLIP)
        g_value_set_string (value, "horizontal");
      else if (self->pipeline_flip_mode == VERTICAL_FLIP)
        g_value_set_string (value, "vertical");
      else if (self->pipeline_flip_mode == ROTATE_180)
        g_value_set_string (value, "rotate180");
      else
        g_value_set_string (value, "none");
      break;
    case PROP_PIPELINE_OVERLAY_PATH:
      g_value_set_string (value,
          self->pipeline_overlay_path ? self->pipeline_overlay_path : "");
      break;
    case PROP_PIPELINE_OVERLAY_LEFT:
      g_value_set_int (value, self->pipeline_overlay_left);
      break;
    case PROP_PIPELINE_OVERLAY_TOP:
      g_value_set_int (value, self->pipeline_overlay_top);
      break;
    case PROP_PIPELINE_OVERLAY_WIDTH:
      g_value_set_int (value, self->pipeline_overlay_width);
      break;
    case PROP_PIPELINE_OVERLAY_STRIDE:
      g_value_set_int (value, self->pipeline_overlay_stride);
      break;
    case PROP_PIPELINE_OVERLAY_HEIGHT:
      g_value_set_int (value, self->pipeline_overlay_height);
      break;
    case PROP_PIPELINE_OVERLAY_FORMAT:
      g_value_set_string (value,
          self->pipeline_overlay_format_str ?
          self->pipeline_overlay_format_str : "argb1555");
      break;
    case PROP_PIPELINE_DRAWRECT_LEFT:
      g_value_set_int (value, self->pipeline_drawrect_left);
      break;
    case PROP_PIPELINE_DRAWRECT_TOP:
      g_value_set_int (value, self->pipeline_drawrect_top);
      break;
    case PROP_PIPELINE_DRAWRECT_RIGHT:
      g_value_set_int (value, self->pipeline_drawrect_right);
      break;
    case PROP_PIPELINE_DRAWRECT_BOTTOM:
      g_value_set_int (value, self->pipeline_drawrect_bottom);
      break;
    case PROP_PIPELINE_DRAWRECT_LINEWIDTH:
      g_value_set_int (value, self->pipeline_drawrect_line_width);
      break;
    case PROP_PIPELINE_DRAWRECT_R:
      g_value_set_uint (value, self->pipeline_drawrect_r);
      break;
    case PROP_PIPELINE_DRAWRECT_G:
      g_value_set_uint (value, self->pipeline_drawrect_g);
      break;
    case PROP_PIPELINE_DRAWRECT_B:
      g_value_set_uint (value, self->pipeline_drawrect_b);
      break;
    case PROP_PIPELINE_FILLRECT_LEFT:
      g_value_set_int (value, self->pipeline_fillrect_left);
      break;
    case PROP_PIPELINE_FILLRECT_TOP:
      g_value_set_int (value, self->pipeline_fillrect_top);
      break;
    case PROP_PIPELINE_FILLRECT_RIGHT:
      g_value_set_int (value, self->pipeline_fillrect_right);
      break;
    case PROP_PIPELINE_FILLRECT_BOTTOM:
      g_value_set_int (value, self->pipeline_fillrect_bottom);
      break;
    case PROP_PIPELINE_FILLRECT_R:
      g_value_set_uint (value, self->pipeline_fillrect_r);
      break;
    case PROP_PIPELINE_FILLRECT_G:
      g_value_set_uint (value, self->pipeline_fillrect_g);
      break;
    case PROP_PIPELINE_FILLRECT_B:
      g_value_set_uint (value, self->pipeline_fillrect_b);
      break;
    case PROP_PIPELINE_CIRCLE_X:
      g_value_set_int (value, self->pipeline_circle_x);
      break;
    case PROP_PIPELINE_CIRCLE_Y:
      g_value_set_int (value, self->pipeline_circle_y);
      break;
    case PROP_PIPELINE_CIRCLE_RADIUS:
      g_value_set_int (value, self->pipeline_circle_radius);
      break;
    case PROP_PIPELINE_CIRCLE_LINEWIDTH:
      g_value_set_int (value, self->pipeline_circle_line_width);
      break;
    case PROP_PIPELINE_CIRCLE_R:
      g_value_set_uint (value, self->pipeline_circle_r);
      break;
    case PROP_PIPELINE_CIRCLE_G:
      g_value_set_uint (value, self->pipeline_circle_g);
      break;
    case PROP_PIPELINE_CIRCLE_B:
      g_value_set_uint (value, self->pipeline_circle_b);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_bm_vpss_finalize (GObject * object)
{
  GstBmVPSS *self = GST_BM_VPSS (object);
  gst_bm_vpss_release_device (self);
  g_free (self->overlay_path);
  g_free (self->overlay_format_str);
  g_free (self->pipeline_csc_str);
  g_free (self->pipeline_overlay_path);
  g_free (self->pipeline_overlay_format_str);
  G_OBJECT_CLASS (gst_bm_vpss_parent_class)->finalize (object);
}

static void
gst_bm_vpss_class_init (GstBmVPSSClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GstBaseTransformClass *base_transform_class = GST_BASE_TRANSFORM_CLASS(klass);
  GstVideoFilterClass *video_filter_class = GST_VIDEO_FILTER_CLASS(klass);

  gobject_class->finalize = gst_bm_vpss_finalize;
  gobject_class->set_property = GST_DEBUG_FUNCPTR(gst_bm_vpss_set_property);
  gobject_class->get_property = GST_DEBUG_FUNCPTR(gst_bm_vpss_get_property);

  base_transform_class->start = GST_DEBUG_FUNCPTR(gst_bm_vpss_start);
  base_transform_class->stop = GST_DEBUG_FUNCPTR(gst_bm_vpss_stop);
  base_transform_class->transform_caps = GST_DEBUG_FUNCPTR(gst_bm_vpss_transform_caps);
  base_transform_class->fixate_caps = GST_DEBUG_FUNCPTR(gst_bm_vpss_fixate_caps);

  video_filter_class->transform_frame = GST_DEBUG_FUNCPTR(gst_bm_vpss_transform_frame);

  g_object_class_install_property (gobject_class, PROP_LEFT,
      g_param_spec_int ("left", "Left","Pixels to crop at left ", 0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_RIGHT,
      g_param_spec_int ("right", "Right","Pixels to crop at right ", 0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_TOP,
      g_param_spec_int ("top", "Top", "Pixels to crop at top ",0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_BOTTOM,
      g_param_spec_int ("bottom", "Bottom","Pixels to crop at bottom ", 0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PADDING_STX,
      g_param_spec_int ("paddingstx", "Padding STX",
          "Source region X on output canvas (BMCV dst_crop_stx)", 0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PADDING_STY,
      g_param_spec_int ("paddingsty", "Padding STY",
          "Source region Y on output canvas (BMCV dst_crop_sty)", 0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PADDING_W,
      g_param_spec_int ("paddingw", "Padding Width",
          "Width of scaled source region on padding canvas (not full canvas width; "
          "canvas size is paddingstx+paddingw by paddingsty+paddingh)", 0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PADDING_H,
      g_param_spec_int ("paddingh", "Padding Height",
          "Height of scaled source region on padding canvas", 0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PADDING_R,
      g_param_spec_uint ("paddingR", "Padding R", "R value", 0, 255, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PADDING_G,
      g_param_spec_uint ("paddingG", "Padding G", "G value", 0, 255, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PADDING_B,
      g_param_spec_uint ("paddingB", "Padding B", "B value", 0, 255, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_DRAWRECT_LEFT,
      g_param_spec_int ("drawrectleft", "Draw Rect Left", "Rectangle left", 0,
          G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_DRAWRECT_TOP,
      g_param_spec_int ("drawrecttop", "Draw Rect Top", "Rectangle top", 0,
          G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_DRAWRECT_RIGHT,
      g_param_spec_int ("drawrectright", "Draw Rect Right", "Rectangle right", 0,
          G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_DRAWRECT_BOTTOM,
      g_param_spec_int ("drawrectbottom", "Draw Rect Bottom", "Rectangle bottom", 0,
          G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_DRAWRECT_LINEWIDTH,
      g_param_spec_int ("drawrectlinewidth", "Draw Rect Line Width",
          "Rectangle border width", 1, G_MAXINT, 2,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_DRAWRECT_R,
      g_param_spec_uint ("drawrectR", "Draw Rect R", "Rectangle R", 0, 255, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_DRAWRECT_G,
      g_param_spec_uint ("drawrectG", "Draw Rect G", "Rectangle G", 0, 255, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_DRAWRECT_B,
      g_param_spec_uint ("drawrectB", "Draw Rect B", "Rectangle B", 0, 255, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_DRAWCIRCLE_X,
      g_param_spec_int ("drawcirclex", "Draw Circle X", "Circle center X", 0,
          G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_DRAWCIRCLE_Y,
      g_param_spec_int ("drawcircley", "Draw Circle Y", "Circle center Y", 0,
          G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_DRAWCIRCLE_RADIUS,
      g_param_spec_int ("drawcircleradius", "Draw Circle Radius", "Circle radius", 0,
          G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_DRAWCIRCLE_LINEWIDTH,
      g_param_spec_int ("drawcirclelinewidth", "Draw Circle Line Width",
          "Circle line width (1-15)", 1, 15, 2,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_DRAWCIRCLE_R,
      g_param_spec_uint ("drawcircleR", "Draw Circle R", "Circle R", 0, 255, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_DRAWCIRCLE_G,
      g_param_spec_uint ("drawcircleG", "Draw Circle G", "Circle G", 0, 255, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_DRAWCIRCLE_B,
      g_param_spec_uint ("drawcircleB", "Draw Circle B", "Circle B", 0, 255, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_MOSAIC_LEFT,
      g_param_spec_int ("mosaicleft", "Mosaic Left", "Mosaic left", 0,
          G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_MOSAIC_TOP,
      g_param_spec_int ("mosaictop", "Mosaic Top", "Mosaic top", 0,
          G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_MOSAIC_RIGHT,
      g_param_spec_int ("mosaicright", "Mosaic Right", "Mosaic right", 0,
          G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_MOSAIC_BOTTOM,
      g_param_spec_int ("mosaicbottom", "Mosaic Bottom", "Mosaic bottom", 0,
          G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_MOSAIC_EXPAND,
      g_param_spec_boolean ("mosaicexpand", "Mosaic Expand",
          "Column expansion mode (is_expand)", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_OVERLAY_PATH,
      g_param_spec_string ("overlaypath", "Overlay Path",
          "Overlay raw file (ARGB8888/1555/4444)", NULL,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_OVERLAY_LEFT,
      g_param_spec_int ("overlayleft", "Overlay Left", "Overlay X", 0,
          G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_OVERLAY_TOP,
      g_param_spec_int ("overlaytop", "Overlay Top", "Overlay Y", 0,
          G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_OVERLAY_WIDTH,
      g_param_spec_int ("overlaywidth", "Overlay Width", "Overlay width", 0,
          G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_OVERLAY_STRIDE,
      g_param_spec_int ("overlaystride", "Overlay Stride",
          "Overlay row pitch in bytes (0=auto from file or width)", 0,
          G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_OVERLAY_HEIGHT,
      g_param_spec_int ("overlayheight", "Overlay Height", "Overlay height", 0,
          G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_OVERLAY_FORMAT,
      g_param_spec_string ("overlayformat", "Overlay Format",
          "argb8888, argb1555, or argb4444", "argb1555",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_PIPELINE_LEFT,
      g_param_spec_int ("pipelineleft", "Pipeline Left",
          "Crop left (pipeline mode)", 0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_RIGHT,
      g_param_spec_int ("pipelineright", "Pipeline Right",
          "Crop right (pipeline mode)", 0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_TOP,
      g_param_spec_int ("pipelinetop", "Pipeline Top",
          "Crop top (pipeline mode)", 0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_BOTTOM,
      g_param_spec_int ("pipelinebottom", "Pipeline Bottom",
          "Crop bottom (pipeline mode)", 0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_PADDING_STX,
      g_param_spec_int ("pipelinepaddingstx", "Pipeline Padding STX",
          "Padding content X on output", 0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_PADDING_STY,
      g_param_spec_int ("pipelinepaddingsty", "Pipeline Padding STY",
          "Padding content Y on output", 0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_PADDING_W,
      g_param_spec_int ("pipelinepaddingw", "Pipeline Padding Width",
          "Padding content width", 0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_PADDING_H,
      g_param_spec_int ("pipelinepaddingh", "Pipeline Padding Height",
          "Padding content height", 0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_PADDING_R,
      g_param_spec_uint ("pipelinepaddingR", "Pipeline Padding R",
          "Padding R", 0, 255, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_PADDING_G,
      g_param_spec_uint ("pipelinepaddingG", "Pipeline Padding G",
          "Padding G", 0, 255, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_PADDING_B,
      g_param_spec_uint ("pipelinepaddingB", "Pipeline Padding B",
          "Padding B", 0, 255, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_INTERPOLATION,
      g_param_spec_string ("pipelineinterpolation", "Pipeline Interpolation",
          "nearest, linear, bicubic, area", "linear",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_CSC,
      g_param_spec_string ("pipelinecsc", "Pipeline CSC",
          "auto, max, yuv2rgb_bt601, rgb2yuv_bt601, ...", "auto",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_CTO_ALPHA0,
      g_param_spec_float ("pipelinectoalpha0", "Pipeline CTO Alpha0",
          "ConvertTo alpha ch0", -G_MAXFLOAT, G_MAXFLOAT, 1.0f,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_CTO_ALPHA1,
      g_param_spec_float ("pipelinectoalpha1", "Pipeline CTO Alpha1",
          "ConvertTo alpha ch1", -G_MAXFLOAT, G_MAXFLOAT, 1.0f,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_CTO_ALPHA2,
      g_param_spec_float ("pipelinectoalpha2", "Pipeline CTO Alpha2",
          "ConvertTo alpha ch2", -G_MAXFLOAT, G_MAXFLOAT, 1.0f,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_CTO_BETA0,
      g_param_spec_float ("pipelinectobeta0", "Pipeline CTO Beta0",
          "ConvertTo beta ch0", -G_MAXFLOAT, G_MAXFLOAT, 0.0f,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_CTO_BETA1,
      g_param_spec_float ("pipelinectobeta1", "Pipeline CTO Beta1",
          "ConvertTo beta ch1", -G_MAXFLOAT, G_MAXFLOAT, 0.0f,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_CTO_BETA2,
      g_param_spec_float ("pipelinectobeta2", "Pipeline CTO Beta2",
          "ConvertTo beta ch2", -G_MAXFLOAT, G_MAXFLOAT, 0.0f,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_FLIP,
      g_param_spec_string ("pipelineflip", "Pipeline Flip",
          "none, horizontal, vertical, rotate180", "none",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_OVERLAY_PATH,
      g_param_spec_string ("pipelineoverlaypath", "Pipeline Overlay Path",
          "Overlay raw file", NULL,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_OVERLAY_LEFT,
      g_param_spec_int ("pipelineoverlayleft", "Pipeline Overlay Left",
          "Overlay X", 0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_OVERLAY_TOP,
      g_param_spec_int ("pipelineoverlaytop", "Pipeline Overlay Top",
          "Overlay Y", 0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_OVERLAY_WIDTH,
      g_param_spec_int ("pipelineoverlaywidth", "Pipeline Overlay Width",
          "Overlay width", 0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_OVERLAY_STRIDE,
      g_param_spec_int ("pipelineoverlaystride", "Pipeline Overlay Stride",
          "Overlay row pitch in bytes (0=auto from file or width)", 0,
          G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_OVERLAY_HEIGHT,
      g_param_spec_int ("pipelineoverlayheight", "Pipeline Overlay Height",
          "Overlay height", 0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_OVERLAY_FORMAT,
      g_param_spec_string ("pipelineoverlayformat", "Pipeline Overlay Format",
          "argb8888, argb1555, argb4444", "argb1555",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_DRAWRECT_LEFT,
      g_param_spec_int ("pipelinedrawrectleft", "Pipeline DrawRect Left",
          "Draw rect left", 0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_DRAWRECT_TOP,
      g_param_spec_int ("pipelinedrawrecttop", "Pipeline DrawRect Top",
          "Draw rect top", 0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_DRAWRECT_RIGHT,
      g_param_spec_int ("pipelinedrawrectright", "Pipeline DrawRect Right",
          "Draw rect right", 0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_DRAWRECT_BOTTOM,
      g_param_spec_int ("pipelinedrawrectbottom", "Pipeline DrawRect Bottom",
          "Draw rect bottom", 0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_DRAWRECT_LINEWIDTH,
      g_param_spec_int ("pipelinedrawrectlinewidth", "Pipeline DrawRect LineWidth",
          "Draw rect border width", 1, 15, 2,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_DRAWRECT_R,
      g_param_spec_uint ("pipelinedrawrectR", "Pipeline DrawRect R",
          "Draw rect R", 0, 255, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_DRAWRECT_G,
      g_param_spec_uint ("pipelinedrawrectG", "Pipeline DrawRect G",
          "Draw rect G", 0, 255, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_DRAWRECT_B,
      g_param_spec_uint ("pipelinedrawrectB", "Pipeline DrawRect B",
          "Draw rect B", 0, 255, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_FILLRECT_LEFT,
      g_param_spec_int ("pipelinefillrectleft", "Pipeline FillRect Left",
          "Fill rect left", 0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_FILLRECT_TOP,
      g_param_spec_int ("pipelinefillrecttop", "Pipeline FillRect Top",
          "Fill rect top", 0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_FILLRECT_RIGHT,
      g_param_spec_int ("pipelinefillrectright", "Pipeline FillRect Right",
          "Fill rect right", 0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_FILLRECT_BOTTOM,
      g_param_spec_int ("pipelinefillrectbottom", "Pipeline FillRect Bottom",
          "Fill rect bottom", 0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_FILLRECT_R,
      g_param_spec_uint ("pipelinefillrectR", "Pipeline FillRect R",
          "Fill rect R", 0, 255, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_FILLRECT_G,
      g_param_spec_uint ("pipelinefillrectG", "Pipeline FillRect G",
          "Fill rect G", 0, 255, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_FILLRECT_B,
      g_param_spec_uint ("pipelinefillrectB", "Pipeline FillRect B",
          "Fill rect B", 0, 255, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_CIRCLE_X,
      g_param_spec_int ("pipelinecirclex", "Pipeline Circle X",
          "Circle center X", 0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_CIRCLE_Y,
      g_param_spec_int ("pipelinecircley", "Pipeline Circle Y",
          "Circle center Y", 0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_CIRCLE_RADIUS,
      g_param_spec_int ("pipelinecircleradius", "Pipeline Circle Radius",
          "Circle radius", 0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_CIRCLE_LINEWIDTH,
      g_param_spec_int ("pipelinecirclelinewidth", "Pipeline Circle LineWidth",
          "Line width 1-15, or -1/-2 for shape modes", -2, 15, 2,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_CIRCLE_R,
      g_param_spec_uint ("pipelinecircleR", "Pipeline Circle R",
          "Circle R", 0, 255, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_CIRCLE_G,
      g_param_spec_uint ("pipelinecircleG", "Pipeline Circle G",
          "Circle G", 0, 255, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PIPELINE_CIRCLE_B,
      g_param_spec_uint ("pipelinecircleB", "Pipeline Circle B",
          "Circle B", 0, 255, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gst_bm_vpss_subclass_init(gpointer glass, gpointer class_data)
{
  /* subclass initialization logic */
  GstBmVPSSClass *bmvpss_class = GST_BM_VPSS_CLASS(glass);
  GstElementClass *element_class = GST_ELEMENT_CLASS(glass);
  GstBmVPSSClassData *cdata = class_data;

  gst_element_class_add_pad_template(
      element_class, gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
                                          cdata->sink_caps));
  gst_element_class_add_pad_template(
      element_class, gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                                          cdata->src_caps));

  gst_element_class_set_static_metadata(element_class,
                                        "SOPHGO VPSS",
                                        "Filter/Effect/Video",
                                        "SOPHGO Video Process Sub-System",
                                        "Jian Fang <jian.fang@sophgo.com>");

  bmvpss_class->soc_index = cdata->soc_index;

  gst_caps_unref(cdata->sink_caps);
  gst_caps_unref(cdata->src_caps);
  g_free(cdata);
}

static void
gst_bm_vpss_init(GstBmVPSS *self)
{
  /* initialization logic */
  self->crop_enable = FALSE;
  self->padding_enable = FALSE;
  self->padding_if_memset = 0;
  self->drawrect_enable = FALSE;
  self->drawrect_line_width = 2;
  self->drawcircle_enable = FALSE;
  self->drawcircle_line_width = 2;
  self->mosaic_enable = FALSE;
  self->mosaic_expand = FALSE;
  self->overlay_path = NULL;
  self->overlay_format_str = g_strdup ("argb1555");
  self->overlay_bm_format = FORMAT_ARGB1555_PACKED;
  self->overlay_format_valid = TRUE;
  self->overlay_stride = 0;
  self->overlay_enable = FALSE;
  self->overlay_bm_image_valid = FALSE;
  self->op_order_len = 0;
  self->pipeline_mode_active = FALSE;
  self->mode_conflict = FALSE;
  self->pipeline_crop_enable = FALSE;
  self->pipeline_padding_enable = FALSE;
  self->pipeline_padding_if_memset = 0;
  self->pipeline_algorithm = BMCV_INTER_LINEAR;
  self->pipeline_csc_str = g_strdup ("auto");
  self->pipeline_csc_type = CSC_MAX_ENUM;
  self->pipeline_csc_explicit = FALSE;
  self->pipeline_cto_alpha[0] = 1.0f;
  self->pipeline_cto_alpha[1] = 1.0f;
  self->pipeline_cto_alpha[2] = 1.0f;
  self->pipeline_cto_beta[0] = 0.0f;
  self->pipeline_cto_beta[1] = 0.0f;
  self->pipeline_cto_beta[2] = 0.0f;
  self->pipeline_convertto_enable = FALSE;
  self->pipeline_flip_mode = NO_FLIP;
  self->pipeline_flip_enable = FALSE;
  self->pipeline_overlay_path = NULL;
  self->pipeline_overlay_format_str = g_strdup ("argb1555");
  self->pipeline_overlay_bm_format = FORMAT_ARGB1555_PACKED;
  self->pipeline_overlay_format_valid = TRUE;
  self->pipeline_overlay_stride = 0;
  self->pipeline_overlay_enable = FALSE;
  self->pipeline_overlay_bm_image_valid = FALSE;
  self->pipeline_drawrect_enable = FALSE;
  self->pipeline_drawrect_line_width = 2;
  self->pipeline_fillrect_enable = FALSE;
  self->pipeline_circle_enable = FALSE;
  self->pipeline_circle_line_width = 2;
  self->pipeline_op_order_len = 0;
  self->bm_handle = NULL;
  self->bm_handle_valid = FALSE;
  GST_DEBUG_OBJECT (self, "bmvpss initial");
}

static void
gst_bm_vpss_subinstance_init(GTypeInstance G_GNUC_UNUSED *instance,
                             gpointer G_GNUC_UNUSED g_class)
{
  // TODO: do init
}

static gboolean
gst_bm_vpss_start (GstBaseTransform * trans)
{
  GstBmVPSS *self = GST_BM_VPSS (trans);
  GstBmVPSSClass *klass = GST_BM_VPSS_CLASS (GST_ELEMENT_GET_CLASS (self));
  bm_status_t ret;

  if (!self->bm_handle_valid) {
    ret = bm_dev_request (&self->bm_handle, klass->soc_index);
    if (ret != BM_SUCCESS) {
      GST_ERROR_OBJECT (self, "bm_dev_request failed: %d", ret);
      return FALSE;
    }
    self->bm_handle_valid = TRUE;
  }
  return TRUE;
}

static gboolean
gst_bm_vpss_stop (GstBaseTransform * trans)
{
  GstBmVPSS *self = GST_BM_VPSS (trans);
  gst_bm_vpss_release_device (self);
  return TRUE;
}


static GstCaps *
gst_bm_vpss_transform_caps(GstBaseTransform *trans,
                          GstPadDirection direction,
                          GstCaps *caps, GstCaps *filter)
{
  (void)caps;
  GstCaps *ret = NULL;
  GstBmVPSS *self = GST_BM_VPSS(trans);

  if (direction == GST_PAD_SRC) {
    // GstPad *sinkpad = gst_element_get_static_pad(GST_ELEMENT(trans), "sink");
    GstPad *sinkpad = GST_BASE_TRANSFORM_SINK_PAD(trans);
    if (sinkpad) {
      GstCaps *upstream_caps = gst_pad_peer_query_caps(sinkpad, filter);
      // gst_object_unref(sinkpad);
      if (upstream_caps) {
        ret = gst_caps_copy(upstream_caps);
        gst_caps_unref(upstream_caps);
        gchar *caps_str = gst_caps_to_string(ret); 
        GST_DEBUG_OBJECT(self, "sinkpad caps: %s", caps_str);
        g_free(caps_str);
      }
    }
  } else if (direction == GST_PAD_SINK) {
    // GstPad *srcpad = gst_element_get_static_pad(GST_ELEMENT(trans), "src");
    GstPad *srcpad = GST_BASE_TRANSFORM_SRC_PAD(trans);
    if (srcpad) {
      GstCaps *downstream_caps = gst_pad_peer_query_caps(srcpad, filter);
      // gst_object_unref(srcpad);
      if (downstream_caps) {
        ret = gst_caps_copy(downstream_caps);
        gst_caps_unref(downstream_caps);
        gchar *caps_str = gst_caps_to_string(ret); 
        GST_DEBUG_OBJECT(self, "srcpad caps: %s", caps_str);
        g_free(caps_str);
      }
    }
  }

  return ret;
}

/*
 * Fixate caps with minimal assumptions:
 * - Only fixate width/height/framerate when missing or not fixed
 * - If format is missing or not fixed, prefer reference format; otherwise default NV12
 * - Never override a fixed format chosen by downstream
 */
static GstCaps *
gst_bm_vpss_fixate_caps(GstBaseTransform *trans,
                       GstPadDirection direction,
                       GstCaps *caps,
                       GstCaps *othercaps)
{
  GstBmVPSS *self = GST_BM_VPSS(trans);
  GST_DEBUG_OBJECT(self, "gst_bm_vpss_fixate_caps begin, direction = %s",
                   (direction == GST_PAD_SRC) ? "SRC" : "SINK");

  // For src pad, just return the proposed caps
  if (direction == GST_PAD_SRC) {
    GST_DEBUG_OBJECT(self, "SRC direction: returning othercaps without change");
    return gst_caps_ref(othercaps);
  }

  // If caps are already fixed, return as-is
  if (gst_caps_is_fixed(othercaps)) {
    GST_DEBUG_OBJECT(self, "SINK direction: othercaps already fixed, returning as-is");
    return gst_caps_ref(othercaps);
  }

  GstCaps *result = gst_caps_copy(othercaps);
  GstStructure *other_s = gst_caps_get_structure(result, 0);
  const GstStructure *ref_s = gst_caps_get_structure(caps, 0);

  gint fixed_width = 0, fixed_height = 0;
  gboolean has_fixed_width = gst_structure_get_int(other_s, "width", &fixed_width);
  gboolean has_fixed_height = gst_structure_get_int(other_s, "height", &fixed_height);

  if (has_fixed_width && has_fixed_height) {
    GST_DEBUG_OBJECT(self, "Width and height already fixed: %dx%d", fixed_width, fixed_height);
  } else {
    gint ref_width = 0, ref_height = 0;
    gst_structure_get_int(ref_s, "width", &ref_width);
    gst_structure_get_int(ref_s, "height", &ref_height);

    if (!has_fixed_width && ref_width > 0) {
      gst_structure_fixate_field_nearest_int(other_s, "width", ref_width);
      GST_DEBUG_OBJECT(self, "Fixated width to nearest: %d", ref_width);
    }

    if (!has_fixed_height && ref_height > 0) {
      gst_structure_fixate_field_nearest_int(other_s, "height", ref_height);
      GST_DEBUG_OBJECT(self, "Fixated height to nearest: %d", ref_height);
    }
  }

  // Fixate format: prefer input format, fall back to I420
  const GValue *fmt_val = gst_structure_get_value(other_s, "format");
  if (!fmt_val || !G_VALUE_HOLDS_STRING (fmt_val)) {
    const GValue *ref_fmt = gst_structure_get_value(ref_s, "format");
    if (ref_fmt && G_VALUE_HOLDS_STRING (ref_fmt)) {
      gst_structure_fixate_field_string(other_s, "format", g_value_get_string (ref_fmt));
      GST_DEBUG_OBJECT(self, "Format fixated to input format: %s", g_value_get_string(ref_fmt));
    } else {
      gst_structure_fixate_field_string(other_s, "format", "I420");
      GST_DEBUG_OBJECT(self, "Format not present, set default format to I420");
    }
  } else {
    GST_DEBUG_OBJECT(self, "Format already fixed: %s", g_value_get_string(fmt_val));
  }

  // Fixate framerate
  const GValue *fps_val = gst_structure_get_value(other_s, "framerate");
  if (!fps_val || !GST_VALUE_HOLDS_FRACTION(fps_val) || !gst_value_is_fixed(fps_val)) {
    gint ref_fps_n = 0, ref_fps_d = 1;
    if (gst_structure_get_fraction(ref_s, "framerate", &ref_fps_n, &ref_fps_d) && ref_fps_n > 0) {
      GST_DEBUG_OBJECT(self, "Fixating framerate to reference: %d/%d", ref_fps_n, ref_fps_d);
      gst_structure_fixate_field_nearest_fraction(other_s, "framerate", ref_fps_n, ref_fps_d);
    } else {
      // fallback to 25/1
      GST_DEBUG_OBJECT(self, "No valid framerate found, defaulting to 25/1");
      gst_structure_fixate_field_nearest_fraction(other_s, "framerate", 25, 1);
    }
  } else {
    GST_DEBUG_OBJECT(self, "Framerate already fixed: %s", g_value_get_string(fps_val));
  }

  gchar *caps_str = gst_caps_to_string(result);
  GST_DEBUG_OBJECT(self, "Final fixated src caps: %s", caps_str);
  g_free(caps_str);

  return result;
}

static GstFlowReturn
gst_bm_vpss_transform_frame (GstVideoFilter * filter, GstVideoFrame * inframe,
    GstVideoFrame * outframe)
{
  GstFlowReturn ret = GST_FLOW_OK;
  bm_status_t bm_ret = 0;
  GstBmVPSS *self = GST_BM_VPSS (filter);
  bm_handle_t bm_handle;

  if (!self->bm_handle_valid) {
    GST_ERROR_OBJECT (self, "device handle not ready");
    return GST_FLOW_ERROR;
  }
  if (self->mode_conflict ||
      (self->pipeline_mode_active && gst_bm_vpss_legacy_is_active (self))) {
    GST_ELEMENT_ERROR (GST_ELEMENT (self), RESOURCE, SETTINGS,
        ("Pipeline and legacy properties cannot be used together"),
        ("Fix element properties and restart the pipeline"));
    return GST_FLOW_ERROR;
  }
  bm_handle = self->bm_handle;

  bm_image *src    = NULL;
  bm_image *dst   = NULL;

  gboolean dst_bm_created = FALSE;
  bm_image temp_a, temp_b;
  gboolean temp_a_valid = FALSE, temp_b_valid = FALSE;

  src = (bm_image *) malloc(sizeof(bm_image));
  dst = (bm_image *) malloc(sizeof(bm_image));
  if (src == NULL || dst == NULL)
  {
    GST_ERROR_OBJECT(self, "Failed to allocate memory for bm_image");
    return GST_FLOW_ERROR;
  }
  memset (src, 0, sizeof (bm_image));
  memset (dst, 0, sizeof (bm_image));

  int in_height = inframe->info.height;
  int in_width = inframe->info.width;
  bm_image_format_ext bmInFormat =
      (bm_image_format_ext)map_gstformat_to_bmformat(inframe->info.finfo->format);

  int out_height = outframe->info.height;
  int out_width = outframe->info.width;
  bm_image_format_ext bmOutFormat =
      (bm_image_format_ext)map_gstformat_to_bmformat(outframe->info.finfo->format);

  GstBuffer *inbuf, *outbuf;
  GstMemory *inmem, *outmem;
  bm_device_mem_t *fb_dma_buffer;

  inbuf = inframe->buffer;
  inmem = gst_buffer_peek_memory(inbuf, 0);
  outbuf = outframe->buffer;
  outmem = gst_buffer_peek_memory(outbuf, 0);

  if (gst_is_dmabuf_memory(outmem)) {
    GST_DEBUG_OBJECT(self, "outmem is dmabuf");
    if (bmOutFormat == FORMAT_RGB_PACKED) {
      bmOutFormat = FORMAT_BGR_PACKED;
    } else if (bmOutFormat == FORMAT_BGR_PACKED) {
      bmOutFormat = FORMAT_RGB_PACKED;
    }
  }

  int in_stride[4] = {0};
  gboolean dst_host_mem = FALSE;

  if (!inmem) {
    GST_ERROR_OBJECT (self, "input buffer has no memory");
    ret = GST_FLOW_ERROR;
    goto error;
  }
  if (!outmem) {
    GST_ERROR_OBJECT (self, "output buffer has no memory");
    ret = GST_FLOW_ERROR;
    goto error;
  }

  dst_host_mem =
      !g_type_is_a (G_OBJECT_TYPE (outmem->allocator), GST_TYPE_BM_ALLOCATOR) &&
      !gst_is_dmabuf_memory (outmem);

  for (guint i = 0; i < GST_VIDEO_FRAME_N_PLANES(inframe); i++) {
    in_stride[i] = GST_VIDEO_FRAME_PLANE_STRIDE(inframe, i);
  }

  bm_ret = bm_image_create (bm_handle, in_height, in_width, bmInFormat,
      DATA_TYPE_EXT_1N_BYTE, src, in_stride);
  if (bm_ret != BM_SUCCESS) {
    GST_ERROR_OBJECT (self, "src bm_image_create failed: %d", bm_ret);
    ret = GST_FLOW_ERROR;
    goto error;
  }

  /* Pipeline + host output: csc_overlay uses its own device buffer (see test). */
  if (!(self->pipeline_mode_active && dst_host_mem)) {
    bm_ret = bm_image_create (bm_handle, out_height, out_width, bmOutFormat,
        DATA_TYPE_EXT_1N_BYTE, dst, NULL);
    if (bm_ret != BM_SUCCESS) {
      GST_ERROR_OBJECT (self, "dst bm_image_create failed: %d", bm_ret);
      ret = GST_FLOW_ERROR;
      goto error;
    }
    dst_bm_created = TRUE;
  }

  if (g_type_is_a(G_OBJECT_TYPE(inmem->allocator), GST_TYPE_BM_ALLOCATOR)) {
    fb_dma_buffer = gst_bm_allocator_get_bm_buffer(inmem);
    unsigned long long base_addr = bm_mem_get_device_addr(*fb_dma_buffer);
    int plane_num = GST_VIDEO_FRAME_N_PLANES(inframe);
    int plane_size[3] = {0};
    unsigned long long input_addr_phy[3] = {0};
    bm_device_mem_t input_addr[3] = {0};

    for (int i = 0; i < plane_num; i++) {
      plane_size[i] = GST_VIDEO_FRAME_COMP_STRIDE(inframe, i) *
                      GST_VIDEO_FRAME_COMP_HEIGHT(inframe, i);
      input_addr_phy[i] = base_addr + GST_VIDEO_FRAME_PLANE_OFFSET(inframe, i);
      input_addr[i] = bm_mem_from_device(input_addr_phy[i], plane_size[i]);
    }
    bm_image_attach(*src, input_addr);
  } else {
    GST_DEBUG_OBJECT(self, "inframe->buffer isn't bm_allocator");
    bm_image_alloc_dev_mem(*src, BMCV_HEAP1_ID);
    guint8 *src_in_ptr[4];
    for (guint i = 0; i < GST_VIDEO_FRAME_N_PLANES(inframe); i++) {
      src_in_ptr[i] = GST_VIDEO_FRAME_PLANE_DATA(inframe, i);
    }
    bm_image_copy_host_to_device(*src, (void **)src_in_ptr);
  }

  if (g_type_is_a(G_OBJECT_TYPE(outmem->allocator), GST_TYPE_BM_ALLOCATOR)) {
    fb_dma_buffer = gst_bm_allocator_get_bm_buffer(outmem);
    unsigned long long base_addr = bm_mem_get_device_addr(*fb_dma_buffer);
    int plane_num = GST_VIDEO_FRAME_N_PLANES(outframe);
    int plane_size[3] = {0};
    unsigned long long output_addr_phy[3] = {0};
    bm_device_mem_t output_addr[3] = {0};

    for (int i = 0; i < plane_num; i++) {
      plane_size[i] = GST_VIDEO_FRAME_COMP_STRIDE(outframe, i) *
                      GST_VIDEO_FRAME_COMP_HEIGHT(outframe, i);
      output_addr_phy[i] = base_addr + GST_VIDEO_FRAME_PLANE_OFFSET(outframe, i);
      output_addr[i] = bm_mem_from_device(output_addr_phy[i], plane_size[i]);
    }
    bm_image_attach(*dst, output_addr);
  } else if (gst_is_dmabuf_memory(outmem)) {
    gint fd = gst_dmabuf_memory_get_fd(outmem);
    GST_DEBUG_OBJECT(self, "outmem fd: %d", fd);
    int plane_num = GST_VIDEO_FRAME_N_PLANES(outframe);
    int plane_size[3] = {0};
    bm_device_mem_t output_addr[3] = {0};

    for (int i = 0; i < plane_num; i++) {
      plane_size[i] = GST_VIDEO_FRAME_COMP_STRIDE(outframe, i) *
                      GST_VIDEO_FRAME_COMP_HEIGHT(outframe, i);
      output_addr[i] = bm_mem_from_device(fd, plane_size[i]);
    }
    bm_image_attach(*dst, output_addr);
  } else {
    GST_DEBUG_OBJECT(self, "outframe->buffer isn't dmabuffer");
    if (!self->pipeline_mode_active) {
      if (bm_image_alloc_dev_mem (*dst, BMCV_HEAP1_ID) != BM_SUCCESS) {
        GST_ERROR_OBJECT (self, "dst dev alloc failed");
        ret = GST_FLOW_ERROR;
        goto error;
      }
    }
  }

  if (self->pipeline_mode_active) {
    ret = gst_bm_vpss_transform_frame_pipeline (self, bm_handle, src,
        dst_bm_created ? dst : NULL, in_width, in_height, out_width,
        out_height, bmOutFormat, inframe->info.finfo->format,
        outframe->info.finfo->format, outframe, dst_host_mem);
    goto error;
  }

  guint temp_toggle = 0;
  bm_image *cur = NULL;
  bm_image *prev_temp = NULL;
  int cur_w, cur_h;
  guint op_idx;
  gboolean any_op = FALSE;
  bmcv_rect_t pass_rect = {0, 0, 0, 0};

  cur = src;
  cur_w = in_width;
  cur_h = in_height;
  gst_bm_vpss_ensure_op_order (self);

#if 0
  GST_DEBUG_OBJECT(self, "inframe: %d x %d, %s, %d planes", inframe->info.width,
                   inframe->info.height,
                   gst_video_format_to_string(inframe->info.finfo->format),
                   GST_VIDEO_FRAME_N_PLANES(inframe));

  GST_DEBUG_OBJECT(self, "inframe->info.stride: %d, %d, %d, %d",
                   inframe->info.stride[0], inframe->info.stride[1],
                   inframe->info.stride[2], inframe->info.stride[3]);

  GST_DEBUG_OBJECT(self, "inframe->info.offset: %d, %d, %d, %d",
                   inframe->info.offset[0], inframe->info.offset[1],
                   inframe->info.offset[2], inframe->info.offset[3]);

  GST_DEBUG_OBJECT(self, "outframe: %d x %d, %s, %d planes",
                   outframe->info.width, outframe->info.height,
                   gst_video_format_to_string(outframe->info.finfo->format),
                   GST_VIDEO_FRAME_N_PLANES(outframe));

  GST_DEBUG_OBJECT(self, "outframe->info.stride: %d, %d, %d, %d",
                   outframe->info.stride[0], outframe->info.stride[1],
                   outframe->info.stride[2], outframe->info.stride[3]);

  GST_DEBUG_OBJECT(self, "outframe->info.offset: %d, %d, %d, %d",
                   outframe->info.offset[0], outframe->info.offset[1],
                   outframe->info.offset[2], outframe->info.offset[3]);
#endif

#if 0
  static gint count = 0;
  guint8 *src_in_ptr[4];
  if (count == 0) {
    const char *file_name = "src.bin";
    FILE *fp_image = fopen(file_name, "wb+");
    if (fp_image == NULL) {
      fprintf(stderr, "Failed to open file %s for writing.\n", file_name);
    }

    gint src_stride[4] = {0};
    gint src_offset[4] = {0};
    gint src_height[4] = {0};
    for (gint i = 0; i < GST_VIDEO_FRAME_N_PLANES(inframe); i++) {
      src_in_ptr[i] = GST_VIDEO_FRAME_PLANE_DATA(inframe, i);
      src_stride[i] = GST_VIDEO_FRAME_PLANE_STRIDE(inframe, i);
      src_height[i] = GST_VIDEO_FRAME_COMP_HEIGHT(inframe, i);
      src_offset[i] = GST_VIDEO_FRAME_PLANE_OFFSET(inframe, i);
      fwrite(src_in_ptr[i], 1, src_stride[i] * src_height[i], fp_image);
    }

    // Clean up.
    fclose(fp_image);
    GST_DEBUG_OBJECT(self, "src_stride: %d, %d, %d, %d", src_stride[0], src_stride[1], src_stride[2], src_stride[3]);
    GST_DEBUG_OBJECT(self, "src_height: %d, %d, %d, %d", src_height[0], src_height[1], src_height[2], src_height[3]);
    GST_DEBUG_OBJECT(self, "src_offset: %d, %d, %d, %d", src_offset[0], src_offset[1], src_offset[2], src_offset[3]);
  }
#endif

  for (op_idx = 0; op_idx < self->op_order_len; op_idx++) {
    GstBmVpssOpType op = self->op_order[op_idx];
    bm_image *out_img = NULL;
    gboolean use_dst = FALSE;
    int next_w = 0, next_h = 0;

    if (!gst_bm_vpss_op_is_enabled (self, op))
      continue;
    any_op = TRUE;

    switch (op) {
      case GST_BM_VPSS_OP_PADDING:{
        bmcv_rect_t in_rect = {0, 0, cur_w, cur_h};
        bmcv_padding_attr_t padding_attr;
        gboolean has_later = gst_bm_vpss_has_later_enabled_op (self, op_idx);

        /* BMCV canvas must fit dst_crop rect; size is independent of negotiated out caps */
        next_w = self->padding_dst_stx + self->padding_dst_w;
        next_h = self->padding_dst_sty + self->padding_dst_h;

        if (next_w <= 0 || next_h <= 0) {
          GST_ERROR_OBJECT (self, "invalid padding region (%d,%d)+(%d,%d)",
              self->padding_dst_stx, self->padding_dst_sty,
              self->padding_dst_w, self->padding_dst_h);
          ret = GST_FLOW_ERROR;
          goto error;
        }

        /* Last op only: may write directly to dst when dimensions match caps */
        use_dst = !has_later && next_w == out_width && next_h == out_height;
        out_img = use_dst ? dst : ((temp_toggle & 1) ? &temp_a : &temp_b);
        if (!use_dst) {
          bm_ret = gst_bm_vpss_alloc_temp (bm_handle, out_img,
              (temp_toggle & 1) ? &temp_a_valid : &temp_b_valid,
              next_h, next_w, bmOutFormat);
          if (bm_ret != BM_SUCCESS) {
            ret = GST_FLOW_ERROR;
            goto error;
          }
          temp_toggle++;
        }
        padding_attr.dst_crop_stx = (unsigned int) self->padding_dst_stx;
        padding_attr.dst_crop_sty = (unsigned int) self->padding_dst_sty;
        padding_attr.dst_crop_w = (unsigned int) self->padding_dst_w;
        padding_attr.dst_crop_h = (unsigned int) self->padding_dst_h;
        padding_attr.padding_r = self->padding_r;
        padding_attr.padding_g = self->padding_g;
        padding_attr.padding_b = self->padding_b;
        padding_attr.if_memset = self->padding_if_memset;
        bm_ret = bmcv_image_vpp_convert_padding (bm_handle, 1, *cur, out_img,
            &padding_attr, &in_rect, BMCV_INTER_LINEAR);
        break;
      }
      case GST_BM_VPSS_OP_CROP:{
        bmcv_rect_t crop_rect;
        crop_rect.start_x = self->crop_left;
        crop_rect.start_y = self->crop_top;
        crop_rect.crop_w = self->crop_right - self->crop_left;
        crop_rect.crop_h = self->crop_bottom - self->crop_top;
        next_w = crop_rect.crop_w;
        next_h = crop_rect.crop_h;
        use_dst = !gst_bm_vpss_has_later_enabled_op (self, op_idx) &&
            next_w == out_width && next_h == out_height;
        out_img = use_dst ? dst : ((temp_toggle & 1) ? &temp_a : &temp_b);
        if (!use_dst) {
          bm_ret = gst_bm_vpss_alloc_temp (bm_handle, out_img,
              (temp_toggle & 1) ? &temp_a_valid : &temp_b_valid,
              next_h, next_w, bmOutFormat);
          if (bm_ret != BM_SUCCESS) {
            ret = GST_FLOW_ERROR;
            goto error;
          }
          temp_toggle++;
        }
        bm_ret = bmcv_image_vpp_convert (bm_handle, 1, *cur, out_img,
            &crop_rect, BMCV_INTER_LINEAR);
        break;
      }
      case GST_BM_VPSS_OP_DRAWRECT:{
        bmcv_rect_t r;
        int rw, rh;
        r.start_x = self->drawrect_left;
        r.start_y = self->drawrect_top;
        rw = self->drawrect_right - self->drawrect_left;
        rh = self->drawrect_bottom - self->drawrect_top;
        r.crop_w = rw;
        r.crop_h = rh;
        if (r.start_x < 0 || r.start_y < 0 || r.start_x + rw > cur_w ||
            r.start_y + rh > cur_h || self->drawrect_line_width <= 0 ||
            self->drawrect_line_width > rw / 2 ||
            self->drawrect_line_width > rh / 2) {
          ret = GST_FLOW_ERROR;
          goto error;
        }
        bm_ret = bmcv_image_draw_rectangle (bm_handle, *cur, 1, &r,
            self->drawrect_line_width, self->drawrect_r, self->drawrect_g,
            self->drawrect_b);
        out_img = cur;
        break;
      }
      case GST_BM_VPSS_OP_DRAWCIRCLE:{
        bmcv_point_t c;
        bmcv_color_t col;
        int rad = self->drawcircle_radius;
        c.x = self->drawcircle_x;
        c.y = self->drawcircle_y;
        col.r = self->drawcircle_r;
        col.g = self->drawcircle_g;
        col.b = self->drawcircle_b;
        if (c.x < 0 || c.y < 0 || c.x >= cur_w || c.y >= cur_h ||
            c.x - rad < 0 || c.y - rad < 0 || c.x + rad >= cur_w ||
            c.y + rad >= cur_h || self->drawcircle_line_width < 1 ||
            self->drawcircle_line_width > 15 ||
            self->drawcircle_line_width > rad) {
          ret = GST_FLOW_ERROR;
          goto error;
        }
        bm_ret = bmcv_image_circle (bm_handle, *cur, c, rad, col,
            self->drawcircle_line_width);
        out_img = cur;
        break;
      }
      case GST_BM_VPSS_OP_MOSAIC:{
        bmcv_rect_t mr;
        int rw, rh;
        mr.start_x = self->mosaic_left;
        mr.start_y = self->mosaic_top;
        rw = self->mosaic_right - self->mosaic_left;
        rh = self->mosaic_bottom - self->mosaic_top;
        mr.crop_w = rw;
        mr.crop_h = rh;
        if (mr.start_x < 0 || mr.start_y < 0 || mr.start_x + rw > cur_w ||
            mr.start_y + rh > cur_h || rw < 8 || rh < 8) {
          ret = GST_FLOW_ERROR;
          goto error;
        }
        bm_ret = bmcv_image_mosaic (bm_handle, 1, *cur, &mr,
            self->mosaic_expand ? 1 : 0);
        out_img = cur;
        break;
      }
      case GST_BM_VPSS_OP_OVERLAY:{
        bmcv_rect_t oi;
        if (!self->overlay_bm_image_valid) {
          bm_ret = gst_bm_vpss_load_overlay (self, bm_handle);
          if (bm_ret != BM_SUCCESS) {
            ret = GST_FLOW_ERROR;
            goto error;
          }
        }
        oi.start_x = self->overlay_left;
        oi.start_y = self->overlay_top;
        oi.crop_w = self->overlay_width;
        oi.crop_h = self->overlay_height;
        if (oi.start_x < 0 || oi.start_y < 0 ||
            oi.start_x + oi.crop_w > cur_w || oi.start_y + oi.crop_h > cur_h) {
          ret = GST_FLOW_ERROR;
          goto error;
        }
        bm_ret = bmcv_image_overlay (bm_handle, *cur, 1, &oi,
            &self->overlay_bm_image);
        out_img = cur;
        break;
      }
      default:
        continue;
    }

    if (bm_ret != BM_SUCCESS) {
      GST_ERROR_OBJECT (self, "VPSS op %d failed: %d", op, bm_ret);
      ret = GST_FLOW_ERROR;
      goto error;
    }

    if (out_img != cur && out_img != NULL) {
      if (prev_temp != NULL && prev_temp != src && prev_temp != dst) {
        if (prev_temp == &temp_a)
          gst_bm_vpss_destroy_temp (&temp_a, &temp_a_valid);
        else
          gst_bm_vpss_destroy_temp (&temp_b, &temp_b_valid);
      }
      if (out_img == &temp_a || out_img == &temp_b)
        prev_temp = out_img;
      cur = out_img;
      if (op == GST_BM_VPSS_OP_PADDING) {
        cur_w = next_w;
        cur_h = next_h;
      } else if (op == GST_BM_VPSS_OP_CROP) {
        cur_w = next_w;
        cur_h = next_h;
      }
    }
  }

  if (!any_op) {
    pass_rect.start_x = 0;
    pass_rect.start_y = 0;
    pass_rect.crop_w = in_width;
    pass_rect.crop_h = in_height;
    bm_ret = bmcv_image_vpp_convert (bm_handle, 1, *src, dst, &pass_rect,
        BMCV_INTER_LINEAR);
    if (bm_ret != BM_SUCCESS) {
      ret = GST_FLOW_ERROR;
      goto error;
    }
    cur = dst;
  } else if (cur != dst) {
    /* Copy or resize/scaler into negotiated dst (e.g. 256x256 -> caps 128x128) */
    pass_rect.start_x = 0;
    pass_rect.start_y = 0;
    pass_rect.crop_w = cur_w;
    pass_rect.crop_h = cur_h;
    bm_ret = bmcv_image_vpp_convert (bm_handle, 1, *cur, dst, &pass_rect,
        BMCV_INTER_LINEAR);
    if (bm_ret != BM_SUCCESS) {
      GST_ERROR_OBJECT (self,
          "final convert %dx%d -> %dx%d failed: %d", cur_w, cur_h,
          out_width, out_height, bm_ret);
      ret = GST_FLOW_ERROR;
      goto error;
    }
    if (cur_w != out_width || cur_h != out_height) {
      GST_DEBUG_OBJECT (self,
          "scaled pipeline result %dx%d to negotiated output %dx%d",
          cur_w, cur_h, out_width, out_height);
    }
  }

  gst_bm_vpss_destroy_temp (&temp_a, &temp_a_valid);
  gst_bm_vpss_destroy_temp (&temp_b, &temp_b_valid);

  if (!g_type_is_a(G_OBJECT_TYPE(outmem->allocator), GST_TYPE_BM_ALLOCATOR) &&
      !gst_is_dmabuf_memory(outmem)) {
    GST_DEBUG_OBJECT(self, "outframe is copy");
    guint8 *dst_in_ptr[4];
    for (guint i = 0; i < GST_VIDEO_FRAME_N_PLANES(outframe); i++) {
      dst_in_ptr[i] = GST_VIDEO_FRAME_PLANE_DATA(outframe, i);
    }
    bm_image_copy_device_to_host(*dst, (void **)dst_in_ptr);
  } else {
    GST_DEBUG_OBJECT(self, "outframe is zerocopy");
  }

#if 0
  if (count <= 0) {
    char src_filename[50];
    char dst_filename[50];
    sprintf(src_filename, "src_%d.bin", count);
    sprintf(dst_filename, "dst_%d.bin", count);
    dump_bmimage(src, src_filename);
    dump_bmimage(dst, dst_filename);
    GST_DEBUG_OBJECT(self, "count = %d", count);
    count++;
  }
#endif

error:
  gst_bm_vpss_destroy_temp (&temp_a, &temp_a_valid);
  gst_bm_vpss_destroy_temp (&temp_b, &temp_b_valid);
  if (src != NULL) {
    if (src->image_private != NULL)
      bm_image_destroy (src);
    free (src);
  }
  if (dst != NULL) {
    if (dst_bm_created && dst->image_private != NULL)
      bm_image_destroy (dst);
    free (dst);
  }

  return ret;
}

static int
map_gstformat_to_bmformat(GstVideoFormat gst_format)
{
  int format;
  switch (gst_format)
  {
  case GST_VIDEO_FORMAT_I420:
    format = FORMAT_YUV420P;
    break;
  case GST_VIDEO_FORMAT_Y42B:
    format = FORMAT_YUV422P;
    break;
  case GST_VIDEO_FORMAT_YUY2:
    format = FORMAT_YUV422_YUYV;
    break;
  case GST_VIDEO_FORMAT_YVYU:
    format = FORMAT_YUV422_YVYU;
    break;
  case GST_VIDEO_FORMAT_UYVY:
    format = FORMAT_YUV422_UYVY;
    break;
  case GST_VIDEO_FORMAT_VYUY:
    format = FORMAT_YUV422_VYUY;
    break;
  case GST_VIDEO_FORMAT_Y444:
    format = FORMAT_YUV444P;
    break;
  case GST_VIDEO_FORMAT_GRAY8:
    format = FORMAT_GRAY;
    break;
  case GST_VIDEO_FORMAT_NV12:
    format = FORMAT_NV12;
    break;
  case GST_VIDEO_FORMAT_NV21:
    format = FORMAT_NV21;
    break;
  case GST_VIDEO_FORMAT_NV16:
    format = FORMAT_NV16;
    break;
  case GST_VIDEO_FORMAT_NV61:
    format = FORMAT_NV61;
    break;
  case GST_VIDEO_FORMAT_RGB:
    format = FORMAT_RGB_PACKED;
    break;
  case GST_VIDEO_FORMAT_BGR:
    format = FORMAT_BGR_PACKED;
    break;
  default:
    GST_ERROR("Error: Unsupported GstVideoFormat %d\n", gst_format);
    return -1;
  }
  return format;
}

// static
// GstVideoFormat map_bmformat_to_gstformat(int bm_format)
// {
//   GstVideoFormat format;
//   switch (bm_format)
//   {
//   case FORMAT_YUV420P:
//     format = GST_VIDEO_FORMAT_I420;
//     break;
//   case FORMAT_YUV422P:
//     format = GST_VIDEO_FORMAT_Y42B;
//     break;
//   case FORMAT_YUV444P:
//     format = GST_VIDEO_FORMAT_Y444;
//     break;
//   case FORMAT_GRAY:
//     format = GST_VIDEO_FORMAT_GRAY8;
//     break;
//   case FORMAT_NV12:
//     format = GST_VIDEO_FORMAT_NV12;
//     break;
//   case FORMAT_NV21:
//     format = GST_VIDEO_FORMAT_NV21;
//     break;
//   case FORMAT_NV16:
//     format = GST_VIDEO_FORMAT_NV16;
//     break;
//   case FORMAT_RGB_PACKED:
//     format = GST_VIDEO_FORMAT_RGB;
//     break;
//   case FORMAT_BGR_PACKED:
//     format = GST_VIDEO_FORMAT_BGR;
//     break;
//   default:
//     GST_ERROR("Unsupported BM format %d", bm_format);
//     format = GST_VIDEO_FORMAT_UNKNOWN;
//     break;
//   }
//   return format;
// }


void gst_bm_vpss_register(GstPlugin *plugin, guint soc_idx, GstCaps *sink_caps,
                          GstCaps *src_caps)
{
  GType type;
  gchar *type_name;
  GstBmVPSSClassData *class_data;
  GTypeInfo type_info = {
    sizeof (GstBmVPSSClass),
    NULL,
    NULL,
    (GClassInitFunc) gst_bm_vpss_subclass_init,
    NULL,
    NULL,
    sizeof (GstBmVPSS),
    0,
    (GInstanceInitFunc) gst_bm_vpss_subinstance_init,
    NULL,
  };

  GST_DEBUG_CATEGORY_INIT (gst_bm_vpss_debug, "bmvpss", 0, "BM VPSS Plugin");

  class_data = g_new0 (GstBmVPSSClassData, 1);
  class_data->sink_caps = gst_caps_ref (sink_caps);
  class_data->src_caps = gst_caps_ref (src_caps);
  class_data->soc_index = soc_idx;

  type_name = g_strdup (GST_BM_VPSS_TYPE_NAME);
  type_info.class_data = class_data;
  type = g_type_register_static(GST_TYPE_BM_VPSS, type_name, &type_info, 0);

  if (!gst_element_register(plugin, type_name, GST_RANK_PRIMARY, type)) {
    GST_WARNING("Failed to register VPSS plugin '%s'", type_name);
  }

  g_free (type_name);
}
