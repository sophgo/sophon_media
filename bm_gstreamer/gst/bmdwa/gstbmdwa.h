#ifndef __GST_BM_DWA_H__
#define __GST_BM_DWA_H__

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>
#include <gst/video/video-format.h>

#include "bmcv_api_ext_c.h"

G_BEGIN_DECLS

#define GST_BM_DWA_TYPE_NAME "bmdwa"

#define GST_VIDEO_CAPS_MAKE_FORMATS \
    "video/x-raw, " \
    "format = (string) { I420, Y42B, Y444, GRAY8, NV12, NV21, NV16, RGB, BGR }, " \
    "width = (int) [ 16, 8192 ], " \
    "height = (int) [ 16, 8192 ], " \
    "framerate = " GST_VIDEO_FPS_RANGE

#define GST_TYPE_BM_DWA (gst_bm_dwa_get_type())
#define GST_BM_DWA(obj)                                                       \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_BM_DWA, GstBmDWA))
#define GST_BM_DWA_CLASS(klass)                                               \
  (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_BM_DWA, GstBmDWAClass))
#define GST_IS_BM_DWA(obj)                                                    \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_BM_DWA))
#define GST_IS_BM_DWA_CLASS(klass)                                            \
  (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_BM_DWA))

typedef struct _GstBmDWA GstBmDWA;
typedef struct _GstBmDWAClass GstBmDWAClass;

typedef enum _GstBmDWAMode{
    GTS_BM_DWA_ROT        = 1,             //ROT
    GTS_BM_DWA_AFFINE     = 2,             //AFFINE
    GTS_BM_DWA_FISHEYE    = 3,             //FISHEYE
    GTS_BM_DWA_GDC        = 4,             //GDC
    GTS_BM_DWA_DEWARP     = 5,             //DEWARP
} GstBmDWAMode;

struct _GstBmDWA {
  GstVideoFilter      base_bmdwa;
  GstAllocator       *allocator;

  GstVideoInfo        in_info;
  GstVideoInfo        out_info;

  GstBmDWAMode        dwa_mode;
  bmcv_rot_mode       rot_mode;
  bmcv_gdc_attr       ldc_attr;
  bmcv_fisheye_attr_s fisheye_attr;
  bm_device_mem_t     dmem;
  bmcv_affine_attr_s  affine_attr;
  gchar               affine_region_attr_name[128];
  gchar               fisheye_gridinfo_name[128];
  gchar               gdc_gridinfo_name[128];
  gchar               dewarp_gridinfo_name[128];
  guint               fisheye_gridinfo_size;
  guint               gdc_gridinfo_size;
  guint               dewarp_gridinfo_size;
  gint                yuv_8bit_y;
  gint                yuv_8bit_u;
  gint                yuv_8bit_v;
};

struct _GstBmDWAClass {
  GstVideoFilterClass base_bmdwa_class;
  guint soc_index;
};

typedef struct _GstBmDWAClassData
{
  GstCaps *sink_caps;
  GstCaps *src_caps;
  guint soc_index;
} GstBmDWAClassData;

GType gst_bm_dwa_get_type(void);

void gst_bm_dwa_register(GstPlugin *plugin, guint soc_idx, GstCaps *sink_caps,
                          GstCaps *src_caps);

G_END_DECLS

#endif /* __GST_BM_DWA_H__ */
