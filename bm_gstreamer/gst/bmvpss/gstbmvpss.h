#ifndef __GST_BM_VPSS_H__
#define __GST_BM_VPSS_H__

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>
#include <gst/video/video-format.h>
#include "bmcv_api_ext_c.h"

G_BEGIN_DECLS

#define GST_BM_VPSS_TYPE_NAME "bmvpss"

#define GST_VIDEO_CAPS_MAKE_FORMATS \
    "video/x-raw, " \
    "format = (string) { I420, Y42B, Y444, GRAY8, NV12, NV21, NV16, NV61, RGB, BGR, YUY2, YVYU, UYVY, VYUY}, " \
    "width = (int) [ 16, 8192 ], " \
    "height = (int) [ 16, 8192 ], " \
    "framerate = " GST_VIDEO_FPS_RANGE

#define GST_TYPE_BM_VPSS (gst_bm_vpss_get_type())
#define GST_BM_VPSS(obj)                                                       \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_BM_VPSS, GstBmVPSS))
#define GST_BM_VPSS_CLASS(klass)                                               \
  (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_BM_VPSS, GstBmVPSSClass))
#define GST_IS_BM_VPSS(obj)                                                    \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_BM_VPSS))
#define GST_IS_BM_VPSS_CLASS(klass)                                            \
  (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_BM_VPSS))

#define GST_BM_VPSS_MAX_OPS 8
#define GST_BM_VPSS_MAX_PIPELINE_OPS 10

typedef enum
{
  GST_BM_VPSS_OP_PADDING = 0,
  GST_BM_VPSS_OP_CROP,
  GST_BM_VPSS_OP_DRAWRECT,
  GST_BM_VPSS_OP_DRAWCIRCLE,
  GST_BM_VPSS_OP_MOSAIC,
  GST_BM_VPSS_OP_OVERLAY
} GstBmVpssOpType;

typedef enum
{
  GST_BM_VPSS_PIPELINE_OP_CROP = 0,
  GST_BM_VPSS_PIPELINE_OP_RESIZE,
  GST_BM_VPSS_PIPELINE_OP_CSC,
  GST_BM_VPSS_PIPELINE_OP_PADDING,
  GST_BM_VPSS_PIPELINE_OP_CONVERTTO,
  GST_BM_VPSS_PIPELINE_OP_FLIP,
  GST_BM_VPSS_PIPELINE_OP_OVERLAY,
  GST_BM_VPSS_PIPELINE_OP_DRAWRECT,
  GST_BM_VPSS_PIPELINE_OP_FILLRECT,
  GST_BM_VPSS_PIPELINE_OP_CIRCLE
} GstBmVpssPipelineOpType;

typedef struct _GstBmVPSS GstBmVPSS;
typedef struct _GstBmVPSSClass GstBmVPSSClass;

struct _GstBmVPSS {
  GstVideoFilter base_bmvpss;

  GstVideoInfo in_info;
  GstVideoInfo out_info;

  gint crop_left;
  gint crop_right;
  gint crop_top;
  gint crop_bottom;
  gboolean crop_enable;

  gint padding_dst_stx;
  gint padding_dst_sty;
  gint padding_dst_w;
  gint padding_dst_h;
  guint8 padding_r;
  guint8 padding_g;
  guint8 padding_b;
  gint padding_if_memset;
  gboolean padding_enable;

  gint drawrect_left;
  gint drawrect_top;
  gint drawrect_right;
  gint drawrect_bottom;
  gint drawrect_line_width;
  guint8 drawrect_r;
  guint8 drawrect_g;
  guint8 drawrect_b;
  gboolean drawrect_enable;

  gint drawcircle_x;
  gint drawcircle_y;
  gint drawcircle_radius;
  gint drawcircle_line_width;
  guint8 drawcircle_r;
  guint8 drawcircle_g;
  guint8 drawcircle_b;
  gboolean drawcircle_enable;

  gint mosaic_left;
  gint mosaic_top;
  gint mosaic_right;
  gint mosaic_bottom;
  gboolean mosaic_expand;
  gboolean mosaic_enable;

  gchar *overlay_path;
  gint overlay_left;
  gint overlay_top;
  gint overlay_width;
  gint overlay_height;
  gint overlay_stride;
  gchar *overlay_format_str;
  bm_image_format_ext overlay_bm_format;
  gboolean overlay_format_valid;
  gboolean overlay_enable;
  gboolean overlay_bm_image_valid;
  bm_image overlay_bm_image;

  GstBmVpssOpType op_order[GST_BM_VPSS_MAX_OPS];
  guint op_order_len;

  /* Pipeline mode (bmcv_image_csc_overlay), activated by any pipeline* property */
  gboolean pipeline_mode_active;
  gboolean mode_conflict;

  gint pipeline_crop_left;
  gint pipeline_crop_right;
  gint pipeline_crop_top;
  gint pipeline_crop_bottom;
  gboolean pipeline_crop_enable;

  gint pipeline_padding_stx;
  gint pipeline_padding_sty;
  gint pipeline_padding_w;
  gint pipeline_padding_h;
  guint8 pipeline_padding_r;
  guint8 pipeline_padding_g;
  guint8 pipeline_padding_b;
  gint pipeline_padding_if_memset;
  gboolean pipeline_padding_enable;

  bmcv_resize_algorithm pipeline_algorithm;
  gchar *pipeline_csc_str;
  csc_type_t pipeline_csc_type;
  gboolean pipeline_csc_explicit;

  float pipeline_cto_alpha[3];
  float pipeline_cto_beta[3];
  gboolean pipeline_convertto_enable;

  bmcv_flip_mode pipeline_flip_mode;
  gboolean pipeline_flip_enable;

  gchar *pipeline_overlay_path;
  gint pipeline_overlay_left;
  gint pipeline_overlay_top;
  gint pipeline_overlay_width;
  gint pipeline_overlay_height;
  gint pipeline_overlay_stride;
  gchar *pipeline_overlay_format_str;
  bm_image_format_ext pipeline_overlay_bm_format;
  gboolean pipeline_overlay_format_valid;
  gboolean pipeline_overlay_enable;
  gboolean pipeline_overlay_bm_image_valid;
  bm_image pipeline_overlay_bm_image;

  gint pipeline_drawrect_left;
  gint pipeline_drawrect_top;
  gint pipeline_drawrect_right;
  gint pipeline_drawrect_bottom;
  gint pipeline_drawrect_line_width;
  guint8 pipeline_drawrect_r;
  guint8 pipeline_drawrect_g;
  guint8 pipeline_drawrect_b;
  gboolean pipeline_drawrect_enable;

  gint pipeline_fillrect_left;
  gint pipeline_fillrect_top;
  gint pipeline_fillrect_right;
  gint pipeline_fillrect_bottom;
  guint8 pipeline_fillrect_r;
  guint8 pipeline_fillrect_g;
  guint8 pipeline_fillrect_b;
  gboolean pipeline_fillrect_enable;

  gint pipeline_circle_x;
  gint pipeline_circle_y;
  gint pipeline_circle_radius;
  gint pipeline_circle_line_width;
  guint8 pipeline_circle_r;
  guint8 pipeline_circle_g;
  guint8 pipeline_circle_b;
  gboolean pipeline_circle_enable;

  GstBmVpssPipelineOpType pipeline_op_order[GST_BM_VPSS_MAX_PIPELINE_OPS];
  guint pipeline_op_order_len;

  bm_handle_t bm_handle;
  gboolean bm_handle_valid;
};

struct _GstBmVPSSClass {
  GstVideoFilterClass base_bmvpss_class;
  guint soc_index;
};

typedef struct _GstBmVPSSClassData
{
  GstCaps *sink_caps;
  GstCaps *src_caps;
  guint soc_index;
} GstBmVPSSClassData;

GType gst_bm_vpss_get_type(void);

void gst_bm_vpss_register(GstPlugin *plugin, guint soc_idx, GstCaps *sink_caps,
                          GstCaps *src_caps);

G_END_DECLS

#endif /* __GST_BM_VPSS_H__ */
