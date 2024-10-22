#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include "gstbmdpu.h"
#include "bmlib_runtime.h"
#include "gstbmallocator.h"
#include <gst/base/gstbasetransform.h>

#define DEFAULT_PROP_DPU_MODE         GTS_BM_DPU_SGBM
#define DEFAULT_PROP_BFW_MODE         (GstBmDPUBfwMode)GTS_BM_DPU_BFW_MODE_7x7
#define DEFAULT_PROP_DISP_RANGE       (GstBmDPUDispRange)GTS_BM_DPU_DISP_RANGE_16
#define DEFAULT_PROP_DISP_START_POS   0
#define DEFAULT_PROP_CENSUS_SHIFT     1
#define DEFAULT_PROP_RSHIFT1          3
#define DEFAULT_PROP_RSHIFT2          2
#define DEFAULT_PROP_DCC_DIR          (GstBmDPUDccDir)GTS_BM_DPU_DCC_DIR_A12
#define DEFAULT_PROP_CA_P1            1800
#define DEFAULT_PROP_CA_P2            14400
#define DEFAULT_PROP_UNIQ_RATIO       25
#define DEFAULT_PROP_DISP_SHIFT       4
#define DEFAULT_PROP_FGS_MAX_COUNT    19
#define DEFAULT_PROP_FGS_MAX_T        3
#define DEFAULT_PROP_FXBASELINE       864000
#define DEFAULT_PROP_DEPTH_UNIT       (GstBmDPUDepthUnit)GTS_BM_DPU_DEPTH_UNIT_MM
#define DEFAULT_PROP_DPU_SGBM_MODE    GTS_BM_DPU_SGBM_MUX2
#define DEFAULT_PROP_DPU_ONLINE_MODE  GTS_BM_DPU_ONLINE_MUX0
#define DEFAULT_PROP_DPU_FGS_MODE     GTS_BM_DPU_FGS_MUX0
GST_DEBUG_CATEGORY (gst_bm_dpu_debug);
#define GST_CAT_DEFAULT gst_bm_dpu_debug

#define GST_BM_DPU_MUTEX(dpu) (&GST_BM_DPU (dpu)->mutex)

#define GST_BM_DPU_LOCK(dpu) \
  g_mutex_lock (GST_BM_DPU_MUTEX (dpu));

#define GST_BM_DPU_UNLOCK(dpu) \
  g_mutex_unlock (GST_BM_DPU_MUTEX (dpu));

static gint map_gstformat_to_bmformat(GstVideoFormat gst_format);
static GstVideoFormat map_bmformat_to_gstformat(gint bm_format);
static GstFlowReturn
gst_bm_dpu_process_buffers (GstBmDPU *self, GstBuffer *outBuffer);
static GstFlowReturn
gst_bm_dpu_default_update_src_caps (GstAggregator * agg,
    GstCaps * caps, GstCaps ** ret);
/* class initialization */
G_DEFINE_TYPE(GstBmDPU, gst_bm_dpu, GST_TYPE_VIDEO_AGGREGATOR);
static gpointer parent_class = NULL;
static GstStaticPadTemplate gst_bm_dpu_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw, " \
    "format = (string) {I420, Y42B, Y444, GRAY8, NV12, NV21, NV16, RGB, BGR}, " \
    "width = (int) [ 64, 1920 ], " \
    "height = (int) [ 64, 1080 ], " \
    "framerate = " GST_VIDEO_FPS_RANGE));

static GstStaticPadTemplate gst_bm_dpu_sink1_template =
GST_STATIC_PAD_TEMPLATE ("sink1",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw, " \
    "format = (string) {I420, Y42B, Y444, GRAY8, NV12, NV21, NV16, RGB, BGR}, " \
    "width = (int) [ 64, 1920 ], " \
    "height = (int) [ 64, 1080 ], " \
    "framerate = " GST_VIDEO_FPS_RANGE));

static GstStaticPadTemplate gst_bm_dpu_sink2_template =
GST_STATIC_PAD_TEMPLATE ("sink2",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw, " \
    "format = (string) {I420, Y42B, Y444, GRAY8, NV12, NV21, NV16, RGB, BGR}, " \
    "width = (int) [ 64, 1920 ], " \
    "height = (int) [ 64, 1080 ], " \
    "framerate = " GST_VIDEO_FPS_RANGE));

enum
{
  PROP_0,
  PROP_DPU_MODE,
  PROP_BFW_MODE,
  PROP_DISP_RANGE,
  PROP_DISP_START_POS,
  PROP_CENSUS_SHIFT,
  PROP_RSHIFT1,
  PROP_RSHIFT2,
  PROP_DCC_DIR,
  PROP_CA_P1,
  PROP_CA_P2,
  PROP_UNIQ_RATIO,
  PROP_DISP_SHIFT,
  PROP_FGS_MAX_COUNT,
  PROP_FGS_MAX_T,
  PROP_FXBASELINE,
  PROP_DEPTH_UNIT,
  PROP_DPU_SGBM_MODE,
  PROP_DPU_ONLINE_MODE,
  PROP_DPU_FGS_MODE,
  PROP_LAST,
};

#define GST_TYPE_BM_DPU_MODE (gst_bm_dpu_mode_get_type ())
static GType
gst_bm_dpu_mode_get_type (void)
{
  static GType profile = 0;

  if (!profile) {
    static const GEnumValue profiles[] = {
      {GTS_BM_DPU_SGBM, "dpu_sgbm", "dpu_sgbm"},
      {GTS_BM_DPU_ONLINE, "dpu_online", "dpu_online"},
      {GTS_BM_DPU_FGS, "dpu_fgs", "dpu_fgs"},
      {0, NULL, NULL},
    };
    profile = g_enum_register_static ("GstBmDpuMode", profiles);
  }
  return profile;
}

#define GST_TYPE_BM_DPU_SGBM_MODE (gst_bm_dpu_sgbm_mode_get_type ())
static GType
gst_bm_dpu_sgbm_mode_get_type (void)
{
  static GType profile = 0;

  if (!profile) {
    static const GEnumValue profiles[] = {
      {GTS_BM_DPU_SGBM_MUX0, "dpu_sgbm_mux0", "dpu_sgbm_mux0"},
      {GTS_BM_DPU_SGBM_MUX1, "dpu_sgbm_mux1", "dpu_sgbm_mux1"},
      {GTS_BM_DPU_SGBM_MUX2, "dpu_sgbm_mux2", "dpu_sgbm_mux2"},
      {0, NULL, NULL},
    };
    profile = g_enum_register_static ("GstBmDpuSgbmMode", profiles);
  }
  return profile;
}

#define GST_TYPE_BM_DPU_ONLINE_MODE (gst_bm_dpu_online_mode_get_type ())
static GType
gst_bm_dpu_online_mode_get_type (void)
{
  static GType profile = 0;

  if (!profile) {
    static const GEnumValue profiles[] = {
      {GTS_BM_DPU_ONLINE_MUX0, "dpu_online_mux0", "dpu_online_mux0"},
      {GTS_BM_DPU_ONLINE_MUX1, "dpu_online_mux1", "dpu_online_mux1"},
      {GTS_BM_DPU_ONLINE_MUX2, "dpu_online_mux2", "dpu_online_mux2"},
      {0, NULL, NULL},
    };
    profile = g_enum_register_static ("GstBmDpuOnlineMode", profiles);
  }
  return profile;
}

#define GST_TYPE_BM_DPU_FGS_MODE (gst_bm_dpu_fgs_mode_get_type ())
static GType
gst_bm_dpu_fgs_mode_get_type (void)
{
  static GType profile = 0;

  if (!profile) {
    static const GEnumValue profiles[] = {
      {GTS_BM_DPU_FGS_MUX0, "dpu_fgs_mux0", "dpu_fgs_mux0"},
      {GTS_BM_DPU_FGS_MUX1, "dpu_fgs_mux1", "dpu_fgs_mux1"},
      {0, NULL, NULL},
    };
    profile = g_enum_register_static ("GstBmDpuFgsMode", profiles);
  }
  return profile;
}

#define GST_TYPE_BM_DPU_DISP_RANGE_MODE (gst_bm_dpu_disp_range_get_type ())
static GType
gst_bm_dpu_disp_range_get_type (void)
{
  static GType profile = 0;

  if (!profile) {
    static const GEnumValue profiles[] = {
      {GTS_BM_DPU_DISP_RANGE_16, "disp_range_16", "disp_range_16"},
      {GTS_BM_DPU_DISP_RANGE_32, "disp_range_32", "disp_range_32"},
      {GTS_BM_DPU_DISP_RANGE_48, "disp_range_48", "disp_range_48"},
      {GTS_BM_DPU_DISP_RANGE_64, "disp_range_64", "disp_range_64"},
      {GTS_BM_DPU_DISP_RANGE_80, "disp_range_80", "disp_range_80"},
      {GTS_BM_DPU_DISP_RANGE_96, "disp_range_96", "disp_range_96"},
      {GTS_BM_DPU_DISP_RANGE_112, "disp_range_112", "disp_range_112"},
      {GTS_BM_DPU_DISP_RANGE_128, "disp_range_128", "disp_range_128"},
      {0, NULL, NULL},
    };
    profile = g_enum_register_static ("GstBmDpuDispRange", profiles);
  }
  return profile;
}

#define GST_TYPE_BM_DPU_BFW_MODE (gst_bm_dpu_bfw_mode_get_type ())
static GType
gst_bm_dpu_bfw_mode_get_type (void)
{
  static GType profile = 0;

  if (!profile) {
    static const GEnumValue profiles[] = {
      {GTS_BM_DPU_BFW_MODE_1x1, "bfw_mode_1x1", "bfw_mode_1x1"},
      {GTS_BM_DPU_BFW_MODE_3x3, "bfw_mode_3x3", "bfw_mode_3x3"},
      {GTS_BM_DPU_BFW_MODE_5x5, "bfw_mode_5x5", "bfw_mode_5x5"},
      {GTS_BM_DPU_BFW_MODE_7x7, "bfw_mode_7x7", "bfw_mode_7x7"},
      {0, NULL, NULL},
    };
    profile = g_enum_register_static ("GstBmDpuBfwMode", profiles);
  }
  return profile;
}

#define GST_TYPE_BM_DPU_DEPTH_UNIT (gst_bm_dpu_depth_unit_get_type ())
static GType
gst_bm_dpu_depth_unit_get_type (void)
{
  static GType profile = 0;

  if (!profile) {
    static const GEnumValue profiles[] = {
      {GTS_BM_DPU_DEPTH_UNIT_MM, "depth_unit_mm", "depth_unit_mm"},
      {GTS_BM_DPU_DEPTH_UNIT_CM, "depth_unit_cm", "depth_unit_cm"},
      {GTS_BM_DPU_DEPTH_UNIT_DM, "depth_unit_dm", "depth_unit_dm"},
      {GTS_BM_DPU_DEPTH_UNIT_M, "depth_unit_m", "depth_unit_m"},
      {0, NULL, NULL},
    };
    profile = g_enum_register_static ("GstBmDpuDpethUnit", profiles);
  }
  return profile;
}

static GstCaps *
gst_bm_dpu_update_caps (GstVideoAggregator * vagg, GstCaps * caps)
{
  GstCaps *template_caps, *ret;
  GList *l;
  gint num = 0;
  GST_OBJECT_LOCK (vagg);
  for (l = GST_ELEMENT (vagg)->sinkpads; l; l = l->next) {
    GstVideoAggregatorPad *vaggpad = l->data;
    num++;
    if (!vaggpad->info.finfo){
      GST_INFO_OBJECT (vaggpad, "vaggpad info.finfo[%d] is null",num);
      continue;
    }

    if (GST_VIDEO_INFO_FORMAT (&vaggpad->info) == GST_VIDEO_FORMAT_UNKNOWN){
      GST_INFO_OBJECT (vaggpad, "vaggpad info.finfo[%d] GST_VIDEO_FORMAT_UNKNOWN",num);
      continue;
    }

    // if (GST_VIDEO_INFO_MULTIVIEW_MODE (&vaggpad->info) !=
    //     GST_VIDEO_MULTIVIEW_MODE_NONE
    //     && GST_VIDEO_INFO_MULTIVIEW_MODE (&vaggpad->info) !=
    //     GST_VIDEO_MULTIVIEW_MODE_MONO) {
    //   GST_FIXME_OBJECT (vaggpad, "Multiview support is not implemented yet");
    //   GST_OBJECT_UNLOCK (vagg);
    //   return NULL;
    // }
  }
  GST_OBJECT_UNLOCK (vagg);

  template_caps = gst_pad_get_pad_template_caps (GST_AGGREGATOR_SRC_PAD (vagg));
  ret = gst_caps_intersect (caps, template_caps);
  if(ret ==NULL)
    GST_INFO_OBJECT (vagg, "gst_bm_dpu_update_caps caps null!\n");
  GST_INFO_OBJECT (vagg, "gst_bm_dpu_update_caps DONE\n");
  return ret;
}

static GstFlowReturn
gst_bm_dpu_default_update_src_caps (GstAggregator * agg,
    GstCaps * caps, GstCaps ** ret)
{
  GstVideoAggregator *vagg = GST_VIDEO_AGGREGATOR (agg);

  *ret = gst_bm_dpu_update_caps (vagg, caps);

  GST_INFO_OBJECT (vagg, "gst_bm_dpu_update_src_caps DONE\n");
  return GST_FLOW_OK;
}

#define GST_TYPE_BM_DPU_DCC_DIR (gst_bm_dpu_dcc_dir_get_type ())
static GType
gst_bm_dpu_dcc_dir_get_type (void)
{
  static GType profile = 0;

  if (!profile) {
    static const GEnumValue profiles[] = {
      {GTS_BM_DPU_DCC_DIR_A12, "dcc_dir_a12", "dcc_dir_a12"},
      {GTS_BM_DPU_DCC_DIR_A13, "dcc_dir_a13", "dcc_dir_a13"},
      {GTS_BM_DPU_DCC_DIR_A14, "dcc_dir_a14", "dcc_dir_a14"},
      {0, NULL, NULL},
    };
    profile = g_enum_register_static ("GstBmDpuDccDir", profiles);
  }
  return profile;
}

static void
gst_bm_dpu_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GstBmDPU *self = GST_BM_DPU (object);

  switch (prop_id) {
    case PROP_DPU_MODE:{
      GstBmDPUMode dpu_mode = g_value_get_enum (value);
      self->dpu_mode = dpu_mode;
      break;
    }
    case PROP_BFW_MODE:{
      GstBmDPUBfwMode bfw_mode = g_value_get_enum (value);
      self->dpu_sgbm_attr.bfw_mode_en = (bmcv_dpu_bfw_mode) bfw_mode;
      break;
    }
    case PROP_DISP_RANGE:{
      GstBmDPUDispRange disp_range = g_value_get_enum (value);
      self->dpu_sgbm_attr.disp_range_en = (bmcv_dpu_disp_range) disp_range;
      break;
    }
    case PROP_DISP_START_POS:{
      gushort disp_start_pos = (gushort) g_value_get_uint (value);
      self->dpu_sgbm_attr.disp_start_pos = disp_start_pos;
      break;
    }
    case PROP_CENSUS_SHIFT:{
      guint census_shift = g_value_get_uint (value);
      self->dpu_sgbm_attr.dpu_census_shift = census_shift;
      break;
    }
    case PROP_RSHIFT1:{
      guint rshift1 = g_value_get_uint (value);
      self->dpu_sgbm_attr.dpu_rshift1 = rshift1;
      break;
    }
    case PROP_RSHIFT2:{
      guint rshift2 = g_value_get_uint (value);
      self->dpu_sgbm_attr.dpu_rshift2 = rshift2;
      break;
    }
    case PROP_DCC_DIR:{
      GstBmDPUDccDir dcc_dir = g_value_get_enum (value);
      self->dpu_sgbm_attr.dcc_dir_en = (bmcv_dpu_dcc_dir) dcc_dir;
      break;
    }
    case PROP_CA_P1:{
      guint ca_p1 = g_value_get_uint (value);
      if (self->dpu_sgbm_attr.dpu_ca_p1 == ca_p1)
        return;

      self->dpu_sgbm_attr.dpu_ca_p1 = ca_p1;
      break;
    }
    case PROP_CA_P2:{
      guint ca_p2 = g_value_get_uint (value);
      self->dpu_sgbm_attr.dpu_ca_p2 = ca_p2;
      break;
    }
    case PROP_UNIQ_RATIO:{
      guint uniq_ratio= g_value_get_uint (value);
      self->dpu_sgbm_attr.dpu_uniq_ratio = uniq_ratio;
      break;
    }
    case PROP_DISP_SHIFT:{
      guint disp_shift = g_value_get_uint (value);
      self->dpu_sgbm_attr.dpu_disp_shift = disp_shift;
      break;
    }
    case PROP_FGS_MAX_COUNT:{
      guint fgs_max_count = g_value_get_uint (value);
      self->dpu_fgs_attr.fgs_max_count = fgs_max_count;
      break;
    }
    case PROP_FGS_MAX_T:{
      guint fgs_max_t = g_value_get_uint (value);
      self->dpu_fgs_attr.fgs_max_t = fgs_max_t;
      break;
    }
    case PROP_FXBASELINE:{
      guint fxbaseline = g_value_get_uint (value);
      self->dpu_fgs_attr.fxbase_line = fxbaseline;
      break;
    }
    case PROP_DEPTH_UNIT:{
      guint depth_unit = g_value_get_enum (value);
      self->dpu_fgs_attr.depth_unit_en = depth_unit;
      break;
    }
    case PROP_DPU_SGBM_MODE:{
      GstBmDPUMode dpu_sgbm_mode = g_value_get_enum (value);
      self->dpu_sgbm_mode = dpu_sgbm_mode;
      break;
    }
    case PROP_DPU_ONLINE_MODE:{
      GstBmDPUMode dpu_online_mode = g_value_get_enum (value);
      self->dpu_online_mode = dpu_online_mode;
      break;
    }
    case PROP_DPU_FGS_MODE:{
      GstBmDPUMode dpu_fgs_mode = g_value_get_enum (value);
      self->dpu_fgs_mode = dpu_fgs_mode;
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      return;
  }
}

static void
save_output(GstBuffer* buffer,char *output)
{
  GstMapInfo info;
  size_t w;
  FILE * output_file;
  output_file = fopen(output, "wb+");
  if(output_file == NULL){
    printf("outfile NULL\n");
    return ;
  }
  gst_buffer_map(buffer, &info, GST_MAP_READ);
  w = fwrite(info.data, 1, info.size, output_file);
  if(w != info.size){
    printf("written size(%d) != info.size(%d)\n", (unsigned int)w, info.size);
    return ;
  }
  gst_buffer_unmap(buffer, &info);
  fflush(output_file);
  fclose(output_file);
}

static void
save_output_size(GstBuffer* buffer,char *output,gint size)
{
  GstMapInfo info;
  size_t w;
  FILE * output_file;
  output_file = fopen(output, "wb+");
  if(output_file == NULL){
    printf("outfile NULL\n");
    return ;
  }
  gst_buffer_map(buffer, &info, GST_MAP_READ);
  w = fwrite(info.data, 1, size, output_file);
  if(w != size){
    printf("written size(%d) != info.size(%d)\n", (unsigned int)w, size);
    return ;
  }
  gst_buffer_unmap(buffer, &info);
  fflush(output_file);
  fclose(output_file);
}

static void
gst_bm_dpu_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  GstBmDPU *self = GST_BM_DPU (object);

  switch (prop_id) {
    case PROP_DPU_MODE:{
      g_value_set_enum(value, self->dpu_mode);
      break;
    }
    case PROP_BFW_MODE:{
      g_value_set_enum(value, (GstBmDPUBfwMode)self->dpu_sgbm_attr.bfw_mode_en);
      break;
    }
    case PROP_DISP_RANGE:{
      g_value_set_enum(value, (GstBmDPUDispRange)self->dpu_sgbm_attr.disp_range_en);
      break;
    }
    case PROP_DISP_START_POS:{
      g_value_set_uint(value, self->dpu_sgbm_attr.disp_start_pos);
      break;
    }
    case PROP_CENSUS_SHIFT:{
      g_value_set_uint(value, self->dpu_sgbm_attr.dpu_census_shift);
      break;
    }
    case PROP_RSHIFT1:{
      g_value_set_uint(value, self->dpu_sgbm_attr.dpu_rshift1);
      break;
    }
    case PROP_RSHIFT2:{
      g_value_set_uint(value, self->dpu_sgbm_attr.dpu_rshift2);
      break;
    }
    case PROP_DCC_DIR:{
      g_value_set_enum(value, (GstBmDPUDccDir)self->dpu_sgbm_attr.dcc_dir_en);
      break;
    }
    case PROP_CA_P1:{
      g_value_set_uint(value, self->dpu_sgbm_attr.dpu_ca_p1);
      break;
    }
    case PROP_CA_P2:{
      g_value_set_uint(value, self->dpu_sgbm_attr.dpu_ca_p2);
      break;
    }
    case PROP_UNIQ_RATIO:{
      g_value_set_uint(value, self->dpu_sgbm_attr.dpu_uniq_ratio);
      break;
    }
    case PROP_DISP_SHIFT:{
      g_value_set_uint(value, self->dpu_sgbm_attr.dpu_disp_shift);
      break;
    }
    case PROP_FGS_MAX_COUNT:{
      g_value_set_uint(value, self->dpu_fgs_attr.fgs_max_count);
      break;
    }
    case PROP_FGS_MAX_T:{
      g_value_set_uint(value, self->dpu_fgs_attr.fgs_max_t);
      break;
    }
    case PROP_FXBASELINE:{
      g_value_set_uint(value, self->dpu_fgs_attr.fxbase_line);
      break;
    }
    case PROP_DEPTH_UNIT:{
      g_value_set_enum(value, (GstBmDPUDepthUnit) self->dpu_fgs_attr.depth_unit_en);
      break;
    }
    case PROP_DPU_SGBM_MODE:{
      g_value_set_enum(value, self->dpu_sgbm_mode);
      break;
    }
    case PROP_DPU_ONLINE_MODE:{
      g_value_set_enum(value, self->dpu_online_mode);
      break;
    }
    case PROP_DPU_FGS_MODE:{
      g_value_set_enum(value, self->dpu_fgs_mode);
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      return;
  }
}

static GstFlowReturn
gst_bm_dpu_aggregate (GstVideoAggregator  * vagg, GstBuffer*  outbuf)
{
  GstFlowReturn ret;
  GstBmDPU *bmdpu = GST_BM_DPU(vagg);
  GST_BM_DPU_LOCK(bmdpu);
  GstElementClass *klass = GST_ELEMENT_GET_CLASS (vagg);
  GstVideoAggregatorClass *vagg_klass = (GstVideoAggregatorClass *) klass;
  GST_INFO_OBJECT(bmdpu, "gst_bm_dpu_aggregate start! \n");
  ret = gst_bm_dpu_process_buffers(bmdpu,outbuf);
  if(ret != GST_FLOW_OK) {
    GST_ERROR_OBJECT(bmdpu, "gst_bm_dpu_process_buffers failed. ret = %d \n", ret);
    goto done;
  }
  done:
    GST_INFO_OBJECT(bmdpu, "gst_bm_dpu_aggregate end! \n");
    GST_BM_DPU_UNLOCK(bmdpu);
    return ret;
}

static GstFlowReturn
gst_bm_dpu_create_output_buffer (GstVideoAggregator * videoaggregator,
    GstBuffer ** outbuf)
{
  GstAggregator *aggregator = GST_AGGREGATOR (videoaggregator);
  GstBufferPool *pool;
  GstFlowReturn ret = GST_FLOW_OK;
  GstBmDPU * self = GST_BM_DPU (videoaggregator);

  pool = gst_aggregator_get_buffer_pool (aggregator);

  // if (pool) {
  //   if (!gst_buffer_pool_is_active (pool)) {
  //     if (!gst_buffer_pool_set_active (pool, TRUE)) {
  //       GST_ELEMENT_ERROR (videoaggregator, RESOURCE, SETTINGS,
  //           ("failed to activate bufferpool"),
  //           ("failed to activate bufferpool"));
  //       return GST_FLOW_ERROR;
  //     }
  //   }

  //   ret = gst_buffer_pool_acquire_buffer (pool, outbuf, NULL);
  //   gst_object_unref (pool);
  // } else {
  guint outsize;
  GstAllocator *allocator;
  GstAllocationParams params;

  gst_aggregator_get_allocator (aggregator, &allocator, &params);

  outsize = GST_VIDEO_INFO_SIZE (&videoaggregator->info);
  if(self->dpu_mode == GTS_BM_DPU_SGBM) {
    if(self->dpu_sgbm_mode == GTS_BM_DPU_SGBM_MUX1){
      outsize = GST_VIDEO_INFO_SIZE (&videoaggregator->info)*2;
    }
  }else if(self->dpu_mode == GTS_BM_DPU_ONLINE) {
    if(self->dpu_online_mode != DPU_ONLINE_MUX0){
      outsize = GST_VIDEO_INFO_SIZE (&videoaggregator->info)*2;
    }
  }else if(self->dpu_mode == GTS_BM_DPU_FGS) {
    if(self->dpu_fgs_mode == DPU_FGS_MUX1){
      outsize = GST_VIDEO_INFO_SIZE (&videoaggregator->info)*2;
    }
  }else {
    GST_ERROR_OBJECT(self, "not support the dpu mode . ret = %d \n", ret);
      return GST_FLOW_ERROR;
  }
  *outbuf = gst_buffer_new_allocate (allocator, outsize, &params);
  GST_INFO_OBJECT(self, "dpu_create_output_buffer size(%d) \n", outsize);
  if (allocator)
    gst_object_unref (allocator);

  if (*outbuf == NULL) {
    GST_ELEMENT_ERROR (videoaggregator, RESOURCE, NO_SPACE_LEFT,
        (NULL), ("Could not acquire buffer of size: %d", outsize));
    ret = GST_FLOW_ERROR;
  }
  // }
  return ret;
}

static void
gst_bm_dpu_class_init (GstBmDPUClass * klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS(klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstVideoAggregatorClass *vagg_class = GST_VIDEO_AGGREGATOR_CLASS (klass);
  GstAggregatorClass *agg_class = GST_AGGREGATOR_CLASS (klass);
  gobject_class->set_property =
      GST_DEBUG_FUNCPTR (gst_bm_dpu_set_property);
  gobject_class->get_property =
      GST_DEBUG_FUNCPTR (gst_bm_dpu_get_property);
  vagg_class->aggregate_frames = GST_DEBUG_FUNCPTR (gst_bm_dpu_aggregate);
  vagg_class->create_output_buffer = GST_DEBUG_FUNCPTR (gst_bm_dpu_create_output_buffer);

  parent_class = g_type_class_peek_parent (klass);
  if(parent_class == NULL){
    printf("parent_class null \n");
  }
  g_object_class_install_property (gobject_class, PROP_DPU_MODE,
      g_param_spec_enum ("dpuMode", "Dpu mode",
          "Dpu mode",
          GST_TYPE_BM_DPU_MODE, DEFAULT_PROP_DPU_MODE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_BFW_MODE,
      g_param_spec_enum ("bfwMode", "Bfw mode",
          "Bfw mode",
          GST_TYPE_BM_DPU_BFW_MODE, DEFAULT_PROP_BFW_MODE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DISP_RANGE,
      g_param_spec_enum ("dispRange", "Disp range",
          "Disp range",
          GST_TYPE_BM_DPU_DISP_RANGE_MODE, DEFAULT_PROP_DISP_RANGE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DCC_DIR,
      g_param_spec_enum ("dccDir", "dcc dir",
          "dcc dir",
          GST_TYPE_BM_DPU_DCC_DIR, DEFAULT_PROP_DCC_DIR,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DISP_START_POS,
      g_param_spec_uint ("dispStartPos", "Disp start pos",
          "Disp start pos",0,255, DEFAULT_PROP_DISP_START_POS,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_CENSUS_SHIFT,
      g_param_spec_uint ("censusShift", "Census shift",
        "Census shift",0,16, DEFAULT_PROP_CENSUS_SHIFT,
        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_RSHIFT1,
      g_param_spec_uint ("rshift1", "rshift1",
        "rshift1",0,16, DEFAULT_PROP_RSHIFT1,
        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_RSHIFT2,
      g_param_spec_uint ("rshift2", "rshift2",
        "rshift2",0,16, DEFAULT_PROP_RSHIFT2,
        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_CA_P1,
      g_param_spec_uint ("caP1", "ca_p1",
        "ca_p1",0,65535, DEFAULT_PROP_CA_P1,
        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_CA_P2,
      g_param_spec_uint ("caP2", "ca_p2",
        "ca_p2",0,65535, DEFAULT_PROP_CA_P2,
        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_UNIQ_RATIO,
      g_param_spec_uint ("uniqRatio", "uniq_ratio",
        "uniq_ratio",0,100, DEFAULT_PROP_UNIQ_RATIO,
        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DISP_SHIFT,
      g_param_spec_uint ("dispShift", "disp_shift",
        "disp_shift",0,16, DEFAULT_PROP_DISP_SHIFT,
        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_FGS_MAX_COUNT,
      g_param_spec_uint ("fgsMaxCount", "fgs_max_count",
        "fgs_max_count",0,100, DEFAULT_PROP_FGS_MAX_COUNT,
        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_FGS_MAX_T,
      g_param_spec_uint ("fgsMaxT", "fgs_max_t",
        "fgs_max_t",0,100, DEFAULT_PROP_FGS_MAX_T,
        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_FXBASELINE,
      g_param_spec_uint ("fxbaseline", "f x baseline",
        "f x baseline",0,4294967295, DEFAULT_PROP_FXBASELINE,
        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DEPTH_UNIT,
      g_param_spec_enum ("depthUnit", "depth_unit",
          "depth_unit",
          GST_TYPE_BM_DPU_DEPTH_UNIT, DEFAULT_PROP_DEPTH_UNIT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DPU_SGBM_MODE,
      g_param_spec_enum ("dpuSgbmMode", "dpu_sgbm_mode",
          "dpu_sgbm_mode",
          GST_TYPE_BM_DPU_SGBM_MODE, DEFAULT_PROP_DPU_SGBM_MODE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DPU_ONLINE_MODE,
      g_param_spec_enum ("dpuOnlineMode", "dpu_online_mode",
          "dpu_online_mode",
          GST_TYPE_BM_DPU_ONLINE_MODE, DEFAULT_PROP_DPU_ONLINE_MODE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DPU_FGS_MODE,
      g_param_spec_enum ("dpuFgsMode", "dpu_fgs_mode","dpu_fgs_mode",
          GST_TYPE_BM_DPU_FGS_MODE, DEFAULT_PROP_DPU_FGS_MODE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

}

static void
gst_bm_dpu_subclass_init(gpointer glass, gpointer class_data)
{
  /* subclass initialization logic */
  GstBmDPUClass *bmdpu_class = GST_BM_DPU_CLASS(glass);
  GstElementClass *element_class = GST_ELEMENT_CLASS(glass);
  GstBmDPUClassData *cdata = class_data;

  gst_element_class_add_pad_template(
      element_class, gst_pad_template_new("sink1", GST_PAD_SINK, GST_PAD_ALWAYS,
                                          cdata->sink_caps1));
  gst_element_class_add_pad_template(
      element_class, gst_pad_template_new("sink2", GST_PAD_SINK, GST_PAD_ALWAYS,
                                          cdata->sink_caps2));

  gst_element_class_add_pad_template(
      element_class, gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                                          cdata->src_caps));

  gst_element_class_set_static_metadata(element_class,
                                        "SOPHGO DPU",
                                        "Filter/Effect/Video",
                                        "SOPHGO Video Process Sub-System",
                                        "Zhongbin Wang <zhongbin.wang@sophgo.com>");

  bmdpu_class->soc_index = cdata->soc_index;

  gst_caps_unref(cdata->sink_caps1);
  gst_caps_unref(cdata->sink_caps2);
  gst_caps_unref(cdata->src_caps);
  g_free(cdata);
}

static void
gst_bm_dpu_init(GstBmDPU *self)
{
  /* initialization logic */
  self->allocator = gst_bm_allocator_new();
  self->dpu_mode = DEFAULT_PROP_DPU_MODE;
  self->dpu_sgbm_attr.bfw_mode_en = DEFAULT_PROP_BFW_MODE;
  self->dpu_sgbm_attr.disp_range_en = DEFAULT_PROP_DISP_RANGE;
  self->dpu_sgbm_attr.disp_start_pos = DEFAULT_PROP_DISP_START_POS;
  self->dpu_sgbm_attr.dpu_census_shift = DEFAULT_PROP_CENSUS_SHIFT;
  self->dpu_sgbm_attr.dpu_rshift1 = DEFAULT_PROP_RSHIFT1;
  self->dpu_sgbm_attr.dpu_rshift2 = DEFAULT_PROP_RSHIFT2;
  self->dpu_sgbm_attr.dcc_dir_en = DEFAULT_PROP_DCC_DIR;
  self->dpu_sgbm_attr.dpu_ca_p1 = DEFAULT_PROP_CA_P1;
  self->dpu_sgbm_attr.dpu_ca_p2 = DEFAULT_PROP_CA_P2;
  self->dpu_sgbm_attr.dpu_uniq_ratio = DEFAULT_PROP_UNIQ_RATIO;
  self->dpu_sgbm_attr.dpu_disp_shift = DEFAULT_PROP_DISP_SHIFT;
  self->dpu_fgs_attr.fgs_max_count = DEFAULT_PROP_FGS_MAX_COUNT;
  self->dpu_fgs_attr.fgs_max_t = DEFAULT_PROP_FGS_MAX_T;
  self->dpu_fgs_attr.fxbase_line = DEFAULT_PROP_FXBASELINE;
  self->dpu_fgs_attr.depth_unit_en = DEFAULT_PROP_DEPTH_UNIT;
  self->dpu_sgbm_mode = DEFAULT_PROP_DPU_SGBM_MODE;
  self->dpu_online_mode = DEFAULT_PROP_DPU_ONLINE_MODE;

  GST_INFO_OBJECT (self, "bmdpu initial");
}

static void
gst_bm_dpu_subinstance_init(GTypeInstance G_GNUC_UNUSED *instance,
                             gpointer G_GNUC_UNUSED g_class)
{
  GstElement *element = GST_ELEMENT(instance);
  GstElementClass *element_class = GST_ELEMENT_GET_CLASS(instance);
  GstBmDPU *self = GST_BM_DPU(instance);
  GstPadTemplate *sink1_pad_template,*sink2_pad_template;
  sink1_pad_template = gst_static_pad_template_get (&gst_bm_dpu_sink1_template);
  if (!sink1_pad_template) {
    g_print ("Failed to get sink1 pad template\n");
    return;
  }

  sink2_pad_template = gst_static_pad_template_get (&gst_bm_dpu_sink2_template);
  if (!sink2_pad_template) {
    g_print ("Failed to get sink2 pad template\n");
    return;
  }

  self->sink1_pad = g_object_new(GST_TYPE_VIDEO_AGGREGATOR_PAD, "name", "sink1", "direction", GST_PAD_SINK,
      "template", sink1_pad_template, NULL);

  self->sink2_pad = g_object_new(GST_TYPE_VIDEO_AGGREGATOR_PAD, "name", "sink2", "direction", GST_PAD_SINK,
      "template", sink2_pad_template, NULL);

  if (!GST_IS_VIDEO_AGGREGATOR_PAD(self->sink1_pad)) {
    g_print("Error: pad1 is not a GstVideoAggregatorPad\n");
    gst_object_unref(GST_PAD(self->sink1_pad));
    gst_object_unref(GST_PAD(self->sink2_pad));
    return -1;
  }

  if (!GST_IS_VIDEO_AGGREGATOR_PAD(self->sink2_pad)) {
    g_print("Error: pad2 is not a GstVideoAggregatorPad\n");
    gst_object_unref(GST_PAD(self->sink1_pad));
    gst_object_unref(GST_PAD(self->sink2_pad));
    return -1;
  }
  gst_element_add_pad(element,GST_PAD(self->sink1_pad));
  gst_element_add_pad(element,GST_PAD(self->sink2_pad));
  g_print ("element_class sink pad num[%d] \n",element->numsinkpads);
}

static GstFlowReturn
gst_bm_dpu_process_frame (bm_handle_t bm_handle, GstBmDPU *self, GstVideoFrame *inFrameL,
    GstVideoFrame *inFrameR, GstVideoFrame *outFrame, gint *dst_stride, bm_image_data_format_ext output_dtype)
{
  GstFlowReturn ret = GST_FLOW_OK;
  gint src_stride[4];
  bm_image *src_left    = NULL;
  bm_image *src_right    = NULL;
  bm_image *dst   = NULL;
  guint output_size;
  GST_INFO_OBJECT(self, "gst_bm_dpu_process_frame start! \n");
  src_left = (bm_image *) malloc(sizeof(bm_image));
  src_right = (bm_image *) malloc(sizeof(bm_image));
  dst = (bm_image *) malloc(sizeof(bm_image));
  if (src_left == NULL || src_right == NULL ||dst == NULL) {
    GST_ERROR_OBJECT(self, "Failed to allocate memory for bm_image");
    return GST_FLOW_ERROR;
  }

  bm_image_format_ext bmInFormatL =
      (bm_image_format_ext)map_gstformat_to_bmformat(inFrameL->info.finfo->format);
  gint in_height_L = inFrameL->info.height;
  gint in_width_L = inFrameL->info.width;

  bm_image_format_ext bmInFormatR =
      (bm_image_format_ext)map_gstformat_to_bmformat(inFrameR->info.finfo->format);
  gint in_height_R = inFrameR->info.height;
  gint in_width_R = inFrameR->info.width;

  bm_image_format_ext bmOutFormat =
      (bm_image_format_ext)map_gstformat_to_bmformat(outFrame->info.finfo->format);
  gint out_height = outFrame->info.height;
  gint out_width = outFrame->info.width;

  if (in_height_L != in_height_R || in_height_L != out_height ||in_height_R != out_height) {
    GST_ERROR_OBJECT(self, "the height of src image is not the same as dst image!  ");
    GST_ERROR_OBJECT(self, "in_height_L(%d) in_height_R(%d) out_height(%d) ",in_height_L,in_height_R,out_height);
    return GST_FLOW_ERROR;
  }
  if (in_width_L != in_width_R || in_width_L != out_width ||in_width_R != out_width) {
    GST_ERROR_OBJECT(self, "the width of src image is not the same as dst image! ");
    GST_ERROR_OBJECT(self, "in_width_L(%d) in_width_R(%d) out_width(%d) ",in_height_L,in_height_R,out_height);
    return GST_FLOW_ERROR;
  }

  // if(bmInFormatL != FORMAT_GRAY || bmInFormatR != FORMAT_GRAY || bmOutFormat != FORMAT_GRAY) {
  //   GST_ERROR_OBJECT(self, "the frame format is not FORMAT_GRAY! ");
  //   return GST_FLOW_ERROR;
  // }

  GST_INFO_OBJECT(self, "gst_bm_dpu_process_frame [1]! \n");
  bm_dpu_image_calc_stride(bm_handle, in_height_L, in_width_L, FORMAT_GRAY, DATA_TYPE_EXT_1N_BYTE, src_stride, false);

  ret = bm_image_create(bm_handle, in_height_L, in_width_L, FORMAT_GRAY, DATA_TYPE_EXT_1N_BYTE, src_left, src_stride);
  if(ret != BM_SUCCESS){
      GST_ERROR_OBJECT(self, "left bm_image create failed. ret = %d \n", ret);
      return GST_FLOW_ERROR;
  }

  ret = bm_image_create(bm_handle, in_height_L, in_width_L, FORMAT_GRAY, DATA_TYPE_EXT_1N_BYTE, src_right, src_stride);
  if(ret != BM_SUCCESS){
      GST_ERROR_OBJECT(self, "right bm_image create failed. ret = %d \n", ret);
      return GST_FLOW_ERROR;
  }

  ret = bm_image_create(bm_handle, out_height, out_width, FORMAT_GRAY, output_dtype, dst, dst_stride);
  if(ret != BM_SUCCESS){
      GST_ERROR_OBJECT(self, "output bm_image create failed. ret = %d \n", ret);
      return GST_FLOW_ERROR;
  }

  // output_size = dst_stride[0]*out_height;
  // if(GST_VIDEO_FRAME_SIZE(outFrame)!= output_size){
  //     GST_ERROR_OBJECT(self, "gst OutFrame size(%d) not equal dpu outbuffer size(%d) \n", GST_VIDEO_FRAME_SIZE(outFrame),output_size);
  //     return GST_FLOW_ERROR;
  // }

  if(bm_image_alloc_dev_mem(*dst, BMCV_HEAP_ANY) != BM_SUCCESS){
      GST_ERROR_OBJECT(self, "bm_image_alloc_dev_mem_dispImg failed \n");
      return GST_FLOW_ERROR;
  }

  guint8 *src_left_ptr[4];
  for (gint i = 0; i < GST_VIDEO_FRAME_N_PLANES(inFrameL); i++) {
    src_left_ptr[i] = GST_VIDEO_FRAME_PLANE_DATA(inFrameL, i);
  }

  guint8 *src_right_ptr[4];
  for (gint i = 0; i < GST_VIDEO_FRAME_N_PLANES(inFrameR); i++) {
    src_right_ptr[i] = GST_VIDEO_FRAME_PLANE_DATA(inFrameR, i);
  }
  guint8 *dst_ptr[4];
  for (gint i = 0; i < GST_VIDEO_FRAME_N_PLANES(outFrame); i++) {
    dst_ptr[i] = GST_VIDEO_FRAME_PLANE_DATA(outFrame, i);
  }
  GST_INFO_OBJECT(self, "gst_bm_dpu_process_frame [2]! \n");
  bm_image_copy_host_to_device(*src_left, (void **)src_left_ptr);
  bm_image_copy_host_to_device(*src_right, (void **)src_right_ptr);
  if(self->dpu_mode == GTS_BM_DPU_SGBM) {
      ret = bmcv_dpu_sgbm_disp(bm_handle, src_left, src_right, dst, &self->dpu_sgbm_attr, self->dpu_sgbm_mode);
      if(ret != BM_SUCCESS) {
            GST_ERROR_OBJECT(self, "bmcv_dpu_sgbm_disp failed. ret = %d \n", ret);
            return GST_FLOW_ERROR;
      }
  }else if(self->dpu_mode == GTS_BM_DPU_ONLINE) {
      ret = bmcv_dpu_online_disp(bm_handle, src_left, src_right, dst, &self->dpu_sgbm_attr, &self->dpu_fgs_attr, self->dpu_online_mode);
      if(ret != BM_SUCCESS) {
            GST_ERROR_OBJECT(self, "bmcv_dpu_online_disp failed. ret = %d \n", ret);
            return GST_FLOW_ERROR;
      }
  }else if(self->dpu_mode == GTS_BM_DPU_FGS) {
      ret = bmcv_dpu_fgs_disp(bm_handle, src_left, src_right, dst, &self->dpu_fgs_attr, self->dpu_fgs_mode);
      if(ret != BM_SUCCESS) {
            GST_ERROR_OBJECT(self, "bmcv_dpu_fgs_disp failed. ret = %d \n", ret);
            return GST_FLOW_ERROR;
      }
  }else {
      GST_ERROR_OBJECT(self, "not support the dpu mode . ret = %d \n", ret);
      return GST_FLOW_ERROR;
  }
  GST_INFO_OBJECT(self, "gst_bm_dpu_process_frame [3]! \n");
  bm_image_copy_device_to_host(*dst, (void **)dst_ptr);

  bm_image_destroy(src_left);
  bm_image_destroy(src_right);
  bm_image_destroy(dst);
  free(src_left);
  free(src_right);
  free(dst);
  GST_INFO_OBJECT(self, "bmcv_dpu process frame done!\n");
  return ret;
}

static GstFlowReturn
gst_bm_dpu_process_buffers (GstBmDPU *self, GstBuffer *outBuffer)
{
  GstVideoFrame *inFrameL;
  GstVideoFrame *inFrameR;
  GstVideoFrame outFrame;
  GstVideoInfo infoL;
  GstVideoInfo infoR;
  GstVideoInfo infoOut;
  gint dst_stride[4];
  GstBuffer *inBufferL;
  GstBuffer *inBufferR;
  GstAggregatorPad* pad_L;
  GstAggregatorPad* pad_R;
  GstVideoAggregatorPad *mpad_L;
  GstVideoAggregatorPad *mpad_R;
  gst_video_info_init (&infoL);
  gst_video_info_init (&infoR);
  gst_video_info_init (&infoOut);
  GstAggregator *agg =GST_AGGREGATOR (self);
  GstVideoAggregator *vagg =GST_VIDEO_AGGREGATOR (self);
  GList *l;
  gint num =0;
  GstFlowReturn ret = GST_FLOW_OK;
  bm_handle_t bm_handle = NULL;
  GstVideoAggregatorPadClass *vaggpad_class;
  bm_image_data_format_ext output_dtype = DATA_TYPE_EXT_1N_BYTE;
  bm_dev_request(&bm_handle, 0);
  GST_INFO_OBJECT(self, "gst_bm_dpu_process_buffers start! \n");
  if(self->dpu_mode == GTS_BM_DPU_SGBM) {
    (self->dpu_sgbm_mode == GTS_BM_DPU_SGBM_MUX1) ? (output_dtype = DATA_TYPE_EXT_U16) :
                            (output_dtype = DATA_TYPE_EXT_1N_BYTE);
  }else if(self->dpu_mode == GTS_BM_DPU_ONLINE) {
    (self->dpu_online_mode == DPU_ONLINE_MUX0) ? (output_dtype = DATA_TYPE_EXT_1N_BYTE) :
                                  (output_dtype = DATA_TYPE_EXT_U16);
  }else if(self->dpu_mode == GTS_BM_DPU_FGS) {
    (self->dpu_fgs_mode == DPU_FGS_MUX0) ? (output_dtype = DATA_TYPE_EXT_1N_BYTE) :
                                    (output_dtype = DATA_TYPE_EXT_U16);
  }else {
    GST_ERROR_OBJECT(self, "not support the dpu mode . ret = %d \n", ret);
      return GST_FLOW_ERROR;
  }
  // GST_INFO_OBJECT(self, "gst_bm_dpu_process_buffers [1.1]! \n");
  // GstVideoAggregatorPad *vaggpad = (GST_ELEMENT (agg)->srcpads);
  // if(!vaggpad ){
  //   GST_ERROR_OBJECT(self, "GST_ELEMENT (agg)->srcpads NULL! \n");
  //   return GST_FLOW_ERROR;
  // }
  // GST_INFO_OBJECT(self, "gst_bm_dpu_process_buffers [1.2]! \n");
  // vaggpad_class = GST_VIDEO_AGGREGATOR_PAD_GET_CLASS(vaggpad);
  // GST_INFO_OBJECT(self, "gst_bm_dpu_process_buffers [1.3]! \n");
  if(outBuffer == NULL){
    GST_ERROR_OBJECT(self, "[pre]out buffer null\n");
    return GST_FLOW_ERROR;
  }

  // ret = vaggpad_class->prepare_frame (vaggpad, vagg, outBuffer,outFrame);
  // if(ret != GST_FLOW_OK){
  //   GST_ERROR_OBJECT(self, "prepare outframe fail! \n");
  //   return GST_FLOW_ERROR;
  // }
  GST_INFO_OBJECT(self, "gst_bm_dpu_process_buffers [1.4]! \n");
  GstCaps *caps = gst_pad_get_current_caps(GST_PAD((GST_ELEMENT (agg)->srcpads)->data));
  if (caps) {
    if (gst_video_info_from_caps(&infoOut, caps)) {
        gint width = GST_VIDEO_INFO_WIDTH(&infoOut);
        gint height = GST_VIDEO_INFO_HEIGHT(&infoOut);
        gint fps_numerator = GST_VIDEO_INFO_FPS_N(&infoOut);
        gint fps_denominator = GST_VIDEO_INFO_FPS_D(&infoOut);

        g_print("out frame width:%d\n", width);
        g_print("out frame height:%d\n", height);
        g_print("ut frame rate:%d/%d\n", fps_numerator, fps_denominator);
    }else{
      GST_ERROR_OBJECT(self, "gst_video_info_from_caps fail\n");
      gst_caps_unref(caps);
      return GST_FLOW_ERROR;
    }
    gst_caps_unref(caps);
  }
  if (!gst_video_frame_map (&outFrame, &infoOut, outBuffer, GST_MAP_WRITE)) {
    GST_WARNING_OBJECT (vagg, "Could not map input buffer");
    return FALSE;
  }
  GST_INFO_OBJECT(self, "gst_bm_dpu_process_buffers [1]! \n");
  for (l = GST_ELEMENT (agg)->sinkpads; l; l = l->next) {
    num ++;
    if(num == 1){
      mpad_L = l->data;
      pad_L = (GstAggregatorPad*)mpad_L;
      // inBufferL = gst_video_aggregator_pad_get_current_buffer(pad_L);
      // if(inBufferL){
      //   char name[128]="imgL_tmp.bin";
      //   save_output(inBufferL,name);
      // }else{
      //   GST_ERROR_OBJECT(self, "inBufferL NULL\n");
      // }
    }else if(num == 2){
      mpad_R = l->data;
      pad_R = (GstAggregatorPad*)mpad_R;
      // inBufferR = gst_video_aggregator_pad_get_current_buffer(pad_R);
      // if(inBufferR){
      //   char name[128]="imgR_tmp.bin";
      //   save_output(inBufferR,name);
      // }else{
      //   GST_ERROR_OBJECT(self, "inBufferR NULL\n");
      // }
    }else{
      GST_ERROR_OBJECT(self, "sink pad num more than 2! \n");
      return GST_FLOW_ERROR;
    }
  }
  GST_INFO_OBJECT(self, "gst_bm_dpu_process_buffers [2]! \n");
  inFrameL = gst_video_aggregator_pad_get_prepared_frame(mpad_L);
  if(inFrameL == NULL){
    GST_INFO_OBJECT(self, "inFrameL NULL ,need to gst_video_frame_map ");
    if (!gst_video_frame_map (inFrameL, &infoL, inBufferL, GST_MAP_READ))
      GST_ERROR_OBJECT(self, "gst_video_frame_map inFrameL failed ");

  }
  inFrameR = gst_video_aggregator_pad_get_prepared_frame(mpad_R);
  if(inFrameR == NULL){
    GST_INFO_OBJECT(self, "inFrameR NULL ,need to gst_video_frame_map ");
    if (!gst_video_frame_map (inFrameR, &infoR, inBufferR, GST_MAP_READ))
        GST_ERROR_OBJECT(self, "gst_video_frame_map inFrameR failed ");
  }
  GST_INFO_OBJECT(self, "gst_bm_dpu_process_buffers [3]! \n");
  gint height = inFrameL->info.height;
  gint width = inFrameL->info.width;
  bm_dpu_image_calc_stride(bm_handle, height, width, FORMAT_GRAY, output_dtype, dst_stride, false);
  ret = gst_bm_dpu_process_frame(bm_handle, self, inFrameL, inFrameR, &outFrame, dst_stride, output_dtype);
  if (ret != GST_FLOW_OK) {
    GST_ERROR_OBJECT(self, "gst_bm_dpu_process_frame failed");
    return GST_FLOW_ERROR;
  }

  // if(outBuffer == NULL){
  //   GST_ERROR_OBJECT(self, "[back]out buffer null\n");
  //   return GST_FLOW_ERROR;
  // }else{
  //     gint size = outFrame.info.width *outFrame.info.height;
  //     if(self->dpu_mode == GTS_BM_DPU_SGBM) {
  //       if(self->dpu_sgbm_mode == GTS_BM_DPU_SGBM_MUX1){
  //         size = outFrame.info.width *outFrame.info.height*2;
  //       }
  //     }else if(self->dpu_mode == GTS_BM_DPU_ONLINE) {
  //       if(self->dpu_online_mode != DPU_ONLINE_MUX0){
  //         size = outFrame.info.width *outFrame.info.height*2;
  //       }
  //     }else if(self->dpu_mode == GTS_BM_DPU_FGS) {
  //       if(self->dpu_fgs_mode == DPU_FGS_MUX1){
  //         size = outFrame.info.width *outFrame.info.height*2;
  //       }
  //     }
  //   char name[128]="outbuffer.bin";
  //   save_output_size(outBuffer,name,size);
  // }
  gst_video_frame_unmap(&outFrame);
  GST_INFO_OBJECT(self, "gst_bm_dpu_process_buffers end! \n");
  return ret;
}

static gint
map_gstformat_to_bmformat(GstVideoFormat gst_format)
{
  gint format;
  switch (gst_format)
  {
  case GST_VIDEO_FORMAT_I420:
    format = FORMAT_YUV420P;
    break;
  case GST_VIDEO_FORMAT_Y42B:
    format = FORMAT_YUV422P;
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
  case GST_VIDEO_FORMAT_NV16:
    format = FORMAT_NV16;
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

static
GstVideoFormat map_bmformat_to_gstformat(gint bm_format)
{
  GstVideoFormat format;
  switch (bm_format)
  {
  case FORMAT_YUV420P:
    format = GST_VIDEO_FORMAT_I420;
    break;
  case FORMAT_YUV422P:
    format = GST_VIDEO_FORMAT_Y42B;
    break;
  case FORMAT_YUV444P:
    format = GST_VIDEO_FORMAT_Y444;
    break;
  case FORMAT_GRAY:
    format = GST_VIDEO_FORMAT_GRAY8;
    break;
  case FORMAT_NV12:
    format = GST_VIDEO_FORMAT_NV12;
    break;
  case FORMAT_NV16:
    format = GST_VIDEO_FORMAT_NV16;
    break;
  case FORMAT_RGB_PACKED:
    format = GST_VIDEO_FORMAT_RGB;
    break;
  case FORMAT_BGR_PACKED:
    format = GST_VIDEO_FORMAT_BGR;
    break;
  default:
    GST_ERROR("Unsupported BM format %d", bm_format);
    format = GST_VIDEO_FORMAT_UNKNOWN;
    break;
  }
  return format;
}

void gst_bm_dpu_register(GstPlugin *plugin, guint soc_idx, GstCaps *sink_caps1,
                          GstCaps *sink_caps,GstCaps *src_caps)
{
  GType type;
  gchar *type_name;
  GstBmDPUClassData *class_data;
  GTypeInfo type_info = {
    sizeof (GstBmDPUClass),
    NULL,
    NULL,
    (GClassInitFunc) gst_bm_dpu_subclass_init,
    NULL,
    NULL,
    sizeof (GstBmDPU),
    0,
    (GInstanceInitFunc) gst_bm_dpu_subinstance_init,
    NULL,
  };

  GST_DEBUG_CATEGORY_INIT (gst_bm_dpu_debug, "bmdpu", 0, "BM DPU Plugin");

  class_data = g_new0 (GstBmDPUClassData, 1);
  class_data->sink_caps1 = gst_caps_ref (sink_caps1);
  class_data->sink_caps2 = gst_caps_ref (sink_caps);
  class_data->src_caps = gst_caps_ref (src_caps);
  class_data->soc_index = soc_idx;

  type_name = g_strdup (GST_BM_DPU_TYPE_NAME);
  type_info.class_data = class_data;
  type = g_type_register_static(GST_TYPE_BM_DPU, type_name, &type_info, 0);

  if (!gst_element_register(plugin, type_name, GST_RANK_PRIMARY, type)) {
    GST_WARNING("Failed to register DPU plugin '%s'", type_name);
  }

  g_free (type_name);
}
