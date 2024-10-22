#ifndef __GST_BM_STITCH_H__
#define __GST_BM_STITCH_H__

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>
#include <gst/video/video-format.h>

#include "bmcv_api_ext_c.h"

G_BEGIN_DECLS

#define GST_BM_STITCH2WAY_TYPE_NAME "bmstitch2way"

#define GST_VIDEO_CAPS_MAKE_FORMATS_SINK \
    "video/x-raw, " \
    "format = (string) {I420, Y42B, Y444, GRAY8, NV12, NV21, NV16, RGB, BGR}," \
    "width = (int) [ 64, 4608 ], " \
    "height = (int) [ 64, 4080 ], " \
    "framerate = " GST_VIDEO_FPS_RANGE

#define GST_VIDEO_CAPS_MAKE_FORMATS_SRC \
    "video/x-raw, " \
    "format = (string) {I420, Y42B, Y444, GRAY8, NV12, NV21, NV16, RGB, BGR}," \
    "width = (int) [ 64, 8604 ], " \
    "height = (int) [ 64, 4080 ], " \
    "framerate = " GST_VIDEO_FPS_RANGE

#define GST_TYPE_BM_STITCH2WAY (gst_bm_stitch2way_get_type())
#define GST_BM_STITCH2WAY(obj)                                                       \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_BM_STITCH2WAY, GstBmSTITCH2WAY))
#define GST_BM_STITCH2WAY_CLASS(klass)                                               \
  (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_BM_STITCH2WAY, GstBmSTITCH2WAYClass))
#define GST_IS_BM_STITCH2WAY(obj)                                                    \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_BM_STITCH2WAY))
#define GST_IS_BM_STITCH2WAY_CLASS(klass)                                            \
  (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_BM_STITCH2WAY))
#define GST_BM_STITCH2WAY_GET_CLASS(obj)                                             \
        (G_TYPE_INSTANCE_GET_CLASS((obj),GST_TYPE_BM_STITCH2WAY,GstBmSTITCH2WAYClass))

typedef struct _GstBmSTITCH2WAY GstBmSTITCH2WAY;
typedef struct _GstBmSTITCH2WAYClass GstBmSTITCH2WAYClass;

struct _GstBmSTITCH2WAY {
  GstVideoAggregator      base_bmstitch;
  GstAllocator *          allocator;
  GstCaps *               sink_caps1;
  GstCaps *               sink_caps2;
  GstCaps *               src_caps;
  GMutex                  mutex;
  GstVideoInfo            in_left_info;
  GstVideoInfo            in_right_info;
  GstVideoInfo            out_info;
  GQueue *                queue1;
  GQueue *                queue2;
  GstVideoAggregatorPad * sink1_pad;
  GstVideoAggregatorPad * sink2_pad;

  struct stitch_param     stitch_config;
  gint                    dst_w;
  gint                    dst_h;
  gchar*                  wgt_name_l[128];
  gchar*                  wgt_name_r[128];
};

struct _GstBmSTITCH2WAYClass {
  GstVideoAggregatorClass     base_bmstitch_class;
  guint                       soc_index;
};

typedef struct _GstBmSTITCHClass2WAYData
{
  GstCaps *   sink_caps1;
  GstCaps *   sink_caps2;
  GstCaps *   src_caps;
  guint       soc_index;
} GstBmSTITCHClass2WAYData;

GType gst_bm_stitch2way_get_type(void);

void gst_bm_stitch2way_register(GstPlugin *plugin, guint soc_idx, GstCaps *sink_caps1,
                          GstCaps *sink_caps,GstCaps *src_caps);

G_END_DECLS

#endif /* __GST_BM_STITCH_H__ */
