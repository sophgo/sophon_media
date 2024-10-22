#ifndef __GST_BM_DPU_H__
#define __GST_BM_DPU_H__

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>
#include <gst/video/video-format.h>

#include "bmcv_api_ext_c.h"

G_BEGIN_DECLS

#define GST_BM_DPU_TYPE_NAME "bmdpu"

#define GST_VIDEO_CAPS_MAKE_FORMATS \
    "video/x-raw, " \
    "format = (string) {I420, Y42B, Y444, GRAY8, NV12, NV21, NV16, RGB, BGR}," \
    "width = (int) [ 64, 1920 ], " \
    "height = (int) [ 64, 1080 ], " \
    "framerate = " GST_VIDEO_FPS_RANGE

#define GST_TYPE_BM_DPU (gst_bm_dpu_get_type())
#define GST_BM_DPU(obj)                                                       \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_BM_DPU, GstBmDPU))
#define GST_BM_DPU_CLASS(klass)                                               \
  (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_BM_DPU, GstBmDPUClass))
#define GST_IS_BM_DPU(obj)                                                    \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_BM_DPU))
#define GST_IS_BM_DPU_CLASS(klass)                                            \
  (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_BM_DPU))
#define GST_BM_DPU_GET_CLASS(obj)                                             \
        (G_TYPE_INSTANCE_GET_CLASS((obj),GST_TYPE_BM_DPU,GstBmDPUClass))

typedef struct _GstBmDPU GstBmDPU;
typedef struct _GstBmDPUClass GstBmDPUClass;

typedef enum _GstBmDPUMode{
    GTS_BM_DPU_SGBM   = 1,             //sgbm
    GTS_BM_DPU_ONLINE = 2,             //sgbm+fgs/sgbm+depth
    GTS_BM_DPU_FGS    = 3,             //fgs
} GstBmDPUMode;

typedef enum _GstBmDPUSgbmMode{
    GTS_BM_DPU_SGBM_MUX0 = 1,             //only sgbm,u8 disp out(no post process),16 align
    GTS_BM_DPU_SGBM_MUX1 = 2,             //only sgbm,u16 disp out(post process),16 align
    GTS_BM_DPU_SGBM_MUX2 = 3,             //only sgbm,u8 disp out(post process),16 align
} GstBmDPUSgbmMode;

typedef enum _GstBmDPUOnlineMode{
    GTS_BM_DPU_ONLINE_MUX0 = 4,  //sgbm 2 fgs online, fgs u8 disp out,16 align
    GTS_BM_DPU_ONLINE_MUX1 = 5,  //sgbm 2 fgs online, fgs u16 depth out,32 align
    GTS_BM_DPU_ONLINE_MUX2 = 6,  //sgbm 2 fgs online, sgbm u16 depth out,32 align
} GstBmDPUOnlineMode;

typedef enum _GstBmDPUFgsMode{
    GTS_BM_DPU_FGS_MUX0 = 7,              //only fgs, u8 disp out,16 align
    GTS_BM_DPU_FGS_MUX1 = 8,              //only fgs, u16 depth out,32 align
} GstBmDPUFgsMode;

typedef enum _GstBmDPUDispRange{
    GTS_BM_DPU_DISP_RANGE_DEFAULT,
    GTS_BM_DPU_DISP_RANGE_16,
    GTS_BM_DPU_DISP_RANGE_32,
    GTS_BM_DPU_DISP_RANGE_48,
    GTS_BM_DPU_DISP_RANGE_64,
    GTS_BM_DPU_DISP_RANGE_80,
    GTS_BM_DPU_DISP_RANGE_96,
    GTS_BM_DPU_DISP_RANGE_112,
    GTS_BM_DPU_DISP_RANGE_128,
    GTS_BM_DPU_DISP_RANGE_BUTT
} GstBmDPUDispRange;

typedef enum _GstBmDPUBfwMode{
    GTS_BM_DPU_BFW_MODE_DEFAULT,
    GTS_BM_DPU_BFW_MODE_1x1,
    GTS_BM_DPU_BFW_MODE_3x3,
    GTS_BM_DPU_BFW_MODE_5x5,
    GTS_BM_DPU_BFW_MODE_7x7,
    GTS_BM_DPU_BFW_MODE_BUTT
} GstBmDPUBfwMode;

typedef enum _GstBmDPUDepthUnit{
    GTS_BM_DPU_DEPTH_UNIT_DEFAULT,
    GTS_BM_DPU_DEPTH_UNIT_MM,
    GTS_BM_DPU_DEPTH_UNIT_CM,
    GTS_BM_DPU_DEPTH_UNIT_DM,
    GTS_BM_DPU_DEPTH_UNIT_M,
    GTS_BM_DPU_DEPTH_UNIT_BUTT
} GstBmDPUDepthUnit;

typedef enum _GstBmDPUDccDir{
    GTS_BM_DPU_DCC_DIR_DEFAULT,
    GTS_BM_DPU_DCC_DIR_A12,
    GTS_BM_DPU_DCC_DIR_A13,
    GTS_BM_DPU_DCC_DIR_A14,
    GTS_BM_DPU_DCC_DIR_BUTT
} GstBmDPUDccDir;

struct _GstBmDPU {
  GstVideoAggregator      base_bmdpu;
  GstAllocator *          allocator;
  GstCaps *               sink_caps1;
  GstCaps *               sink_caps2;
  GstCaps *               src_caps;
  GstVideoAggregatorPad * sink1_pad;
  GstVideoAggregatorPad * sink2_pad;
  GstBmDPUMode            dpu_mode;
  GMutex                  mutex;
  GstBmDPUSgbmMode        dpu_sgbm_mode;
  GstBmDPUOnlineMode      dpu_online_mode;
  GstBmDPUFgsMode         dpu_fgs_mode;
  bmcv_dpu_sgbm_attrs     dpu_sgbm_attr;
  bmcv_dpu_fgs_attrs      dpu_fgs_attr;
  GstVideoInfo            in_left_info;
  GstVideoInfo            in_right_info;
  GstVideoInfo            out_info;
  GQueue *                queue1;
  GQueue *                queue2;
};

struct _GstBmDPUClass {
  GstVideoAggregatorClass     base_bmdpu_class;
  guint                       soc_index;
};

typedef struct _GstBmDPUClassData
{
  GstCaps *   sink_caps1;
  GstCaps *   sink_caps2;
  GstCaps *   src_caps;
  guint       soc_index;
} GstBmDPUClassData;

GType gst_bm_dpu_get_type(void);

void gst_bm_dpu_register(GstPlugin *plugin, guint soc_idx, GstCaps *sink_caps1,
                          GstCaps *sink_caps,GstCaps *src_caps);

G_END_DECLS

#endif /* __GST_BM_DPU_H__ */
