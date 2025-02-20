#ifndef __GST_BM_IVE_H__
#define __GST_BM_IVE_H__

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>
#include <gst/video/video-format.h>

#include "bmcv_api_ext_c.h"
#include "bmlib_runtime.h"

G_BEGIN_DECLS

#define USE_BM_ALLOCATOR

#define GST_BM_IVE_TYPE_NAME "bmive"

#define GST_VIDEO_CAPS_MAKE_FORMATS \
    "video/x-raw, " \
    "format = (string) { GRAY8, NV12, RGB }, " \
    "width = (int) [ 32, 1920 ], " \
    "height = (int) [ 32, 1080 ], " \
    "framerate = " GST_VIDEO_FPS_RANGE

#define GST_TYPE_BM_IVE (gst_bm_ive_get_type())
// G_DECLARE_FINAL_TYPE (GstBmIVE, gst_bm_ive, GST, BM_IVE, GstVideoFilter);
#define GST_BM_IVE(obj)                                                       \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_BM_IVE, GstBmIVE))
#define GST_BM_IVE_CLASS(klass)                                               \
  (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_BM_IVE, GstBmIVEClass))
#define GST_IS_BM_IVE(obj)                                                    \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_BM_IVE))
#define GST_IS_BM_IVE_CLASS(klass)                                            \
  (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_BM_IVE))


enum
{
	PROP_TYPE = 1,
	PROP_MODE1,
	PROP_MODE2,
	PROP_LOW_THR,
	PROP_HIGHT_THR,
	PROP_MIN_VAL,
	PROP_MID_VAL,
	PROP_MAX_VAL,
	PROP_NORM,
	PROP_INIT_AREA_THR,
	PROP_STEP,
	PROP_THR_VALUE,
	PROP_WIDTH,
	PROP_HEIGHT,
	PROP_X,
	PROP_Y
};
typedef enum
{
	IVE_ADD = 0x0,
	IVE_AND,
	IVE_SUB,
	IVE_XOR,
	IVE_OR,
	IVE_DMA,
	IVE_HIST,
	IVE_SOBEL,
	IVE_CCL,
	IVE_FILTER,
	IVE_ERODE,
	IVE_DILATE,
	IVE_THRESH,
	IVE_MAP,
	IVE_THRESH_U16,
	IVE_THRESH_S16,
	IVE_LBP,
	IVE_INTG,
	IVE_NCC,
	IVE_ORDSATAFILTER,
	IVE_MAGANDANG,
	IVE_NORMGRAD,
	IVE_CANNYHYSEDGE,
	IVE_GMM,
	IVE_GMM2,
	IVE_STCANDICORNER,
	IVE_GRADFG,
	IVE_SAD,
	IVE_BGMODEL
} GstBmIVEType;

struct _GstBmIVE {
  GstVideoFilter base_bmive;
  GstAllocator *allocator;

  GstVideoInfo in_info;
  GstVideoInfo out_info;
  gboolean prop_dirty;
  gboolean init_mem;

  GstBmIVEType type;
  //usually used mode1 as input mode, sad case need mode1 and mode2
  guint mode1;
  guint mode2;

  //alloc memeory addr;
  bm_device_mem_t mem;

  //used by thresh\thresh_u16\thresh_s16
  guint low_thr;
  guint hight_thr;
  guint min_val;
  guint mid_val;
  guint max_val;

  //used by filter
  guint norm;

  //used by ccl
  guint init_area_thr;
  guint step;

  //used by magandang
  guint thr_value;

  //used by add
  guint x;
  guint y;
};

struct _GstBmIVEClass {
  GstVideoFilterClass base_bmive_class;
  guint soc_index;
};

typedef struct _GstBmIVEClassData
{
  GstCaps *sink_caps;
  GstCaps *src_caps;
  guint soc_index;
} GstBmIVEClassData;

typedef struct _GstBmIVE GstBmIVE;
typedef struct _GstBmIVEClass GstBmIVEClass;

GType gst_bm_ive_get_type(void);

void gst_bm_ive_register(GstPlugin *plugin, guint soc_idx, GstCaps *sink_caps,
                          GstCaps *src_caps);

G_END_DECLS

#endif /* __GST_BM_IVE_H__ */
