#ifndef __GST_BM_VPSS_H__
#define __GST_BM_VPSS_H__

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>
#include <gst/video/video-format.h>


G_BEGIN_DECLS

#define GST_BM_VPSS_TYPE_NAME "bmvpss"

#define GST_VIDEO_CAPS_MAKE_FORMATS \
    "video/x-raw, " \
    "format = (string) { I420, Y42B, Y444, GRAY8, NV12, NV21, NV16, RGB, BGR }, " \
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

typedef struct _GstBmVPSS GstBmVPSS;
typedef struct _GstBmVPSSClass GstBmVPSSClass;

struct _GstBmVPSS {
  GstVideoFilter base_bmvpss;
  GstAllocator *allocator;

  GstVideoInfo in_info;
  GstVideoInfo out_info;
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