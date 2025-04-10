#ifndef __GST_BM_V4L2SRC_H__
#define __GST_BM_V4L2SRC_H__

#include <gst/gst.h>
#include <config.h>
#include <gst/base/gstbasesrc.h>
#include <gst/base/gstpushsrc.h>

#include "bmcv_api_ext_c.h"
#include "gstv4l2object.h"

G_BEGIN_DECLS

#define GST_BM_V4L2SRC_TYPE_NAME "bmv4l2src"

#define GST_BM_V4L2SRC_SRC_FORMATS \
    "video/x-raw, " \
    "format = (string) {NV21, YUYV, YUY2}, " \
    "width = (int) [ 16, 8192 ], " \
    "height = (int) [ 16, 8192 ], " \
    "framerate = " GST_VIDEO_FPS_RANGE

#define GST_BM_V4L2SRC_SINK_FORMATS ""

#define gst_bmv4l2src_parent_class parent_class

#define GST_TYPE_BM_V4L2SRC (gst_bmv4l2src_get_type())
#define GST_BM_V4L2SRC(obj)                                                       \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_BM_V4L2SRC, GstBmV4l2Src))
#define GST_BM_V4L2SRC_CLASS(klass)                                               \
  (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_BM_V4L2SRC, GstBmV4l2SrcClass))
#define GST_IS_BM_V4L2SRC(obj)                                                    \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_BM_V4L2SRC))
#define GST_IS_BM_V4L2SRC_CLASS(klass)                                            \
  (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_BM_V4L2SRC))

typedef struct _GstBmV4l2Src GstBmV4l2Src;
typedef struct _GstBmV4l2SrcClass GstBmV4l2SrcClass;

struct _GstBmV4l2Src {
  GstPushSrc pushsrc;

  gchar *device_path;
  gint video_fd;
  gint index_id;
  guint cast_dmabuf;
  const gchar *element;

  /*< private >*/
  GstV4l2Object * v4l2object;

  guint64 offset;
  gboolean next_offset_same;

  /* offset adjust after renegotiation */
  guint64 renegotiation_adjust;

  GstClockTime ctrl_time;

  gboolean pending_set_fmt;

  guint crop_top;
  guint crop_left;
  guint crop_bottom;
  guint crop_right;

  struct v4l2_rect crop_bounds;

  gboolean apply_crop_settings;
  struct v4l2_rect crop_rect;

  /* Timestamp sanity check */
  GstClockTime last_timestamp;
  gboolean has_bad_timestamp;

  /* maintain signal status, updated during negotiation */
  gboolean no_signal;
};

struct _GstBmV4l2SrcClass {
  GstPushSrcClass parent_class;
  GList *v4l2_class_devices;
  gchar *default_device;
  guint soc_index;
};

typedef struct _GstBmV4l2SrcClassData
{
  GstCaps *sink_caps;
  GstCaps *src_caps;
  guint soc_index;
} GstBmV4l2SrcClassData;

GType gst_bmv4l2src_get_type(void);

void gst_bmv4l2src_register(GstPlugin *plugin,
                             GstCaps *sink_caps, GstCaps *src_caps);

G_END_DECLS

#endif /* __GST_BM_V4L2SRC_H__ */
