#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include "gstbmdwa.h"
#include "bmlib_runtime.h"
#include "gstbmallocator.h"
#include <gst/base/gstbasetransform.h>

GST_DEBUG_CATEGORY (gst_bm_dwa_debug);
#define GST_CAT_DEFAULT gst_bm_dwa_debug

#define DEFAULT_PROP_DWA_MODE                     GTS_BM_DWA_ROT
#define DEFAULT_PROP_DWA_ROT_MODE                 BMCV_ROTATION_0
#define DEFAULT_PROP_DWA_AFFINE_REG_NUM           1
#define DEFAULT_PROP_DWA_AFFINE_SIZE_W            640
#define DEFAULT_PROP_DWA_AFFINE_SIZE_H            480
#define DEFAULT_PROP_DWA_AFFINE_REG_ATTR          ""
#define DEFAULT_PROP_DWA_FISHEYE_EN               0
#define DEFAULT_PROP_DWA_FISHEYE_BGCOLOR_EN       0
#define DEFAULT_PROP_DWA_FISHEYE_BGCOLOR_Y        0
#define DEFAULT_PROP_DWA_FISHEYE_BGCOLOR_U        0
#define DEFAULT_PROP_DWA_FISHEYE_BGCOLOR_V        0
#define DEFAULT_PROP_DWA_FISHEYE_HOR_OFFSET       0
#define DEFAULT_PROP_DWA_FISHEYE_VER_OFFSET       0
#define DEFAULT_PROP_DWA_FISHEYE_TRA_COF          0
#define DEFAULT_PROP_DWA_FISHEYE_FAN_STRENGTH     0
#define DEFAULT_PROP_DWA_FISHEYE_MOUNT_MODE       BMCV_FISHEYE_DESKTOP_MOUNT
#define DEFAULT_PROP_DWA_FISHEYE_USE_MODE         BMCV_MODE_PANORAMA_360
#define DEFAULT_PROP_DWA_FISHEYE_REG_NUM          1
#define DEFAULT_PROP_DWA_FISHEYE_VIEW_MODE        BMCV_FISHEYE_VIEW_NORMAL
#define DEFAULT_PROP_DWA_FISHEYE_GRID_INFO_NAME   ""
#define DEFAULT_PROP_DWA_FISHEYE_GRID_INFO_SIZE   0
#define DEFAULT_PROP_DWA_GDC_ASPECT               0
#define DEFAULT_PROP_DWA_GDC_X_RATIO              0
#define DEFAULT_PROP_DWA_GDC_Y_RATIO              0
#define DEFAULT_PROP_DWA_GDC_XY_RATIO             0
#define DEFAULT_PROP_DWA_GDC_CENTER_X_OFFSET      0
#define DEFAULT_PROP_DWA_GDC_CENTER_Y_OFFSET      0
#define DEFAULT_PROP_DWA_GDC_DISTORT_RATIO        0
#define DEFAULT_PROP_DWA_GDC_GRID_INFO_NAME       ""
#define DEFAULT_PROP_DWA_GDC_GRID_INFO_SIZE       0
#define DEFAULT_PROP_DWA_DEWARP_GRID_INFO_NAME    ""
#define DEFAULT_PROP_DWA_DEWARP_GRID_INFO_SIZE    0
#define YUV_8BIT(y, u, v) ((((y)&0xff) << 16) | (((u)&0xff) << 8) | ((v)&0xff))
static gboolean gst_bm_dwa_start (GstBaseTransform * trans);
static gboolean gst_bm_dwa_stop (GstBaseTransform * trans);
static GstCaps *gst_bmdwa_transform_caps(GstBaseTransform *trans,
                                          GstPadDirection direction,
                                          GstCaps *caps, GstCaps *filter);
static GstFlowReturn gst_bm_dwa_transform_frame(GstVideoFilter *filter,
    GstVideoFrame *inframe, GstVideoFrame *outframe);

static int map_gstformat_to_bmformat(GstVideoFormat gst_format);
static GstVideoFormat map_bmformat_to_gstformat(int bm_format);
static bmcv_point2f_s faces[9][4] = {
		{ {.x = 722.755, .y = 65.7575}, {.x = 828.402, .y = 80.6858}, {.x = 707.827, .y = 171.405}, {.x = 813.474, .y = 186.333} },
		{ {.x = 494.919, .y = 117.918}, {.x = 605.38,  .y = 109.453}, {.x = 503.384, .y = 228.378}, {.x = 613.845, .y = 219.913} },
		{ {.x = 1509.06, .y = 147.139}, {.x = 1592.4,  .y = 193.044}, {.x = 1463.15, .y = 230.48 }, {.x = 1546.5,  .y = 276.383} },
		{ {.x = 1580.21, .y = 66.7939}, {.x = 1694.1,  .y = 70.356 }, {.x = 1576.65, .y = 180.682}, {.x = 1690.54, .y = 184.243} },
		{ {.x = 178.76,  .y = 90.4814}, {.x = 286.234, .y = 80.799 }, {.x = 188.442, .y = 197.955}, {.x = 295.916, .y = 188.273} },
		{ {.x = 1195.57, .y = 139.226}, {.x = 1292.69, .y = 104.122}, {.x = 1230.68, .y = 236.34}, {.x = 1327.79, .y = 201.236}, },
		{ {.x = 398.669, .y = 109.872}, {.x = 501.93, .y = 133.357}, {.x = 375.184, .y = 213.133}, {.x = 478.445, .y = 236.618}, },
		{ {.x = 845.989, .y = 94.591}, {.x = 949.411, .y = 63.6143}, {.x = 876.966, .y = 198.013}, {.x = 980.388, .y = 167.036}, },
		{ {.x = 1060.19, .y = 58.7882}, {.x = 1170.61, .y = 61.9105}, {.x = 1057.07, .y = 169.203}, {.x = 1167.48, .y = 172.325}, },
};

/* class initialization */
G_DEFINE_TYPE(GstBmDWA, gst_bm_dwa, GST_TYPE_VIDEO_FILTER);

enum
{
  PROP_0,
  PROP_DWA_MODE,
  PROP_DWA_ROT_MODE,
  PROP_DWA_AFFINE_REG_NUM,
  PROP_DWA_AFFINE_SIZE_W,
  PROP_DWA_AFFINE_SIZE_H,
  PROP_DWA_AFFINE_REG_ATTR,
  PROP_DWA_FISHEYE_EN,
  PROP_DWA_FISHEYE_BGCOLOR_EN,
  PROP_DWA_FISHEYE_BGCOLOR_Y,
  PROP_DWA_FISHEYE_BGCOLOR_U,
  PROP_DWA_FISHEYE_BGCOLOR_V,
  PROP_DWA_FISHEYE_HOR_OFFSET,
  PROP_DWA_FISHEYE_VER_OFFSET,
  PROP_DWA_FISHEYE_TRA_COF,
  PROP_DWA_FISHEYE_FAN_STRENGTH,
  PROP_DWA_FISHEYE_MOUNT_MODE,
  PROP_DWA_FISHEYE_USE_MODE,
  PROP_DWA_FISHEYE_REG_NUM,
  PROP_DWA_FISHEYE_VIEW_MODE,
  PROP_DWA_FISHEYE_GRID_INFO_NAME,
  PROP_DWA_FISHEYE_GRID_INFO_SIZE,
  PROP_DWA_GDC_ASPECT,
  PROP_DWA_GDC_X_RATIO,
  PROP_DWA_GDC_Y_RATIO,
  PROP_DWA_GDC_XY_RATIO,
  PROP_DWA_GDC_CENTER_X_OFFSET,
  PROP_DWA_GDC_CENTER_Y_OFFSET,
  PROP_DWA_GDC_DISTORT_RATIO,
  PROP_DWA_GDC_GRID_INFO_NAME,
  PROP_DWA_GDC_GRID_INFO_SIZE,
  PROP_DWA_DEWARP_GRID_INFO_NAME,
  PROP_DWA_DEWARP_GRID_INFO_SIZE,
};

#define GST_TYPE_BM_DWA_MODE (gst_bm_dwa_mode_get_type ())
static GType
gst_bm_dwa_mode_get_type (void)
{
  static GType profile = 0;

  if (!profile) {
    static const GEnumValue profiles[] = {
      {GTS_BM_DWA_ROT, "dwa_rotation", "dwa_rotation"},
      {GTS_BM_DWA_AFFINE, "dwa_affine", "dwa_affine"},
      {GTS_BM_DWA_FISHEYE, "dwa_fisheye", "dwa_fisheye"},
      {GTS_BM_DWA_GDC, "dwa_gdc", "dwa_gdc"},
      {GTS_BM_DWA_DEWARP, "dwa_dewarp", "dwa_dewarp"},
      {0, NULL, NULL},
    };
    profile = g_enum_register_static ("GstBmDWAMode", profiles);
  }
  return profile;
}

#define GST_TYPE_BM_DWA_ROT_MODE (gst_bm_dwa_rot_mode_get_type ())
static GType
gst_bm_dwa_rot_mode_get_type (void)
{
  static GType profile = 0;

  if (!profile) {
    static const GEnumValue profiles[] = {
      {BMCV_ROTATION_0,  "dwa_rotation_0", "dwa_rotation_0"},
      {BMCV_ROTATION_90,  "dwa_rotation_90", "dwa_rotation_90"},
      {BMCV_ROTATION_180, "dwa_rotation_180", "dwa_rotation_180"},
      {BMCV_ROTATION_270, "dwa_rotation_270", "dwa_rotation_270"},
      {BMCV_ROTATION_XY_FLIP, "dwa_rotation_xy_flip", "dwa_rotation_xy_flip"},
      {0, NULL, NULL},
    };
    profile = g_enum_register_static ("GstBmDWARotMode", profiles);
  }
  return profile;
}

#define GST_TYPE_BM_DWA_FISHEYE_MOUNT_MODE (gst_bm_dwa_fisheye_mount_mode_get_type ())
static GType
gst_bm_dwa_fisheye_mount_mode_get_type (void)
{
  static GType profile = 0;

  if (!profile) {
    static const GEnumValue profiles[] = {
      {BMCV_FISHEYE_DESKTOP_MOUNT,  "desktop_mount", "desktop_mount"},
      {BMCV_FISHEYE_CEILING_MOUNT,  "ceiling_mount", "ceiling_mount"},
      {BMCV_FISHEYE_WALL_MOUNT,     "wall_mount", "wall_mount"},
      {0, NULL, NULL},
    };
    profile = g_enum_register_static ("GstBmDWAFisheyeMountMode", profiles);
  }
  return profile;
}

#define GST_TYPE_BM_DWA_FISHEYE_USAGE_MODE (gst_bm_dwa_fisheye_usage_mode_get_type ())
static GType
gst_bm_dwa_fisheye_usage_mode_get_type (void)
{
  static GType profile = 0;

  if (!profile) {
    static const GEnumValue profiles[] = {
      {BMCV_MODE_PANORAMA_360,  "panorama_360", "panorama_360"},
      {BMCV_MODE_PANORAMA_180,  "panorama_180", "panorama_180"},
      {BMCV_MODE_01_1O,     "01_1O", "01_1O"},
      {BMCV_MODE_02_1O4R,  "02_1O4R", "02_1O4R"},
      {BMCV_MODE_03_4R,  "03_4R", "03_4R"},
      {BMCV_MODE_04_1P2R,     "04_1P2R", "04_1P2R"},
      {BMCV_MODE_05_1P2R,  "05_1P2R", "05_1P2R"},
      {BMCV_MODE_06_1P,  "06_1P", "06_1P"},
      {BMCV_MODE_07_2P,     "07_2P", "07_2P"},
      {BMCV_MODE_STEREO_FIT,     "STEREO_FIT", "STEREO_FIT"},
      {0, NULL, NULL},
    };
    profile = g_enum_register_static ("GstBmDWAFisheyeUsageMode", profiles);
  }
  return profile;
}

#define GST_TYPE_BM_DWA_FISHEYE_VIEW_MODE (gst_bm_dwa_fisheye_view_mode_get_type ())
static GType
gst_bm_dwa_fisheye_view_mode_get_type (void)
{
  static GType profile = 0;

  if (!profile) {
    static const GEnumValue profiles[] = {
      {BMCV_FISHEYE_VIEW_360_PANORAMA,  "panorama_360", "panorama_360"},
      {BMCV_FISHEYE_VIEW_180_PANORAMA,  "panorama_180", "panorama_180"},
      {BMCV_FISHEYE_VIEW_NORMAL,     "normal", "panorama_normal"},
      {BMCV_FISHEYE_NO_TRANSFORMATION,  "dwa_fisheye_view_transformation", "dwa_fisheye_view_transformation"},
      {0, NULL, NULL},
    };
    profile = g_enum_register_static ("GstBmDWAFisheyeViewMode", profiles);
  }
  return profile;
}

static void
gst_bm_dwa_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GstBmDWA *self = GST_BM_DWA (object);

  switch (prop_id) {
    case PROP_DWA_MODE:{
      GstBmDWAMode dwa_mode = g_value_get_enum (value);
      self->dwa_mode = dwa_mode;
      break;
    }
    case PROP_DWA_ROT_MODE:{
      bmcv_rot_mode rot_mode = g_value_get_enum (value);
      self->rot_mode = rot_mode;
      break;
    }
    case PROP_DWA_AFFINE_REG_NUM:{
      guint region_num = g_value_get_uint (value);
      self->affine_attr.u32RegionNum = region_num;
      break;
    }
    case PROP_DWA_AFFINE_SIZE_W:{
      guint width = g_value_get_uint (value);
      self->affine_attr.stDestSize.u32Width = width;
      break;
    }
    case PROP_DWA_AFFINE_SIZE_H:{
      guint height = g_value_get_uint (value);
      self->affine_attr.stDestSize.u32Height = height;
      break;
    }
    case PROP_DWA_AFFINE_REG_ATTR:{
      gchar *affine_region_attr_name = g_value_get_string (value);
      memcpy(self->affine_region_attr_name,affine_region_attr_name,128);
      break;
    }
    case PROP_DWA_FISHEYE_EN:{
      gboolean bEnable = g_value_get_boolean (value);
      self->fisheye_attr.bEnable = bEnable;
      break;
    }
    case PROP_DWA_FISHEYE_BGCOLOR_EN:{
      gboolean bBgColor = g_value_get_boolean (value);
      self->fisheye_attr.bBgColor = bBgColor;
      break;
    }
    case PROP_DWA_FISHEYE_BGCOLOR_Y:{
      guint yuv_8bit_y = g_value_get_uint (value);
      self->yuv_8bit_y = yuv_8bit_y;
      break;
    }
    case PROP_DWA_FISHEYE_BGCOLOR_U:{
      guint yuv_8bit_u = g_value_get_uint (value);
      self->yuv_8bit_u = yuv_8bit_u;
      break;
    }
    case PROP_DWA_FISHEYE_BGCOLOR_V:{
      guint yuv_8bit_v = g_value_get_uint (value);
      self->yuv_8bit_v = yuv_8bit_v;
      break;
    }
    case PROP_DWA_FISHEYE_HOR_OFFSET:{
      gint s32HorOffset = g_value_get_int (value);
      self->fisheye_attr.s32HorOffset = s32HorOffset;
      break;
    }
    case PROP_DWA_FISHEYE_VER_OFFSET:{
      gint s32VerOffset = g_value_get_int (value);
      self->fisheye_attr.s32VerOffset = s32VerOffset;
      break;
    }
    case PROP_DWA_FISHEYE_TRA_COF:{
      guint u32TrapezoidCoef = g_value_get_uint (value);
      self->fisheye_attr.u32TrapezoidCoef = u32TrapezoidCoef;
      break;
    }
    case PROP_DWA_FISHEYE_FAN_STRENGTH:{
      gint s32FanStrength = g_value_get_int (value);
      self->fisheye_attr.s32FanStrength = s32FanStrength;
      break;
    }
    case PROP_DWA_FISHEYE_MOUNT_MODE:{
      bmcv_fisyeye_mount_mode_e enMountMode = g_value_get_enum (value);
      self->fisheye_attr.enMountMode = enMountMode;
      break;
    }
    case PROP_DWA_FISHEYE_USE_MODE:{
      bmcv_usage_mode enUseMode = g_value_get_enum (value);
      self->fisheye_attr.enUseMode = enUseMode;
      break;
    }
    case PROP_DWA_FISHEYE_REG_NUM:{
      guint u32RegionNum = g_value_get_uint (value);
      self->fisheye_attr.u32RegionNum = u32RegionNum;
      break;
    }
    case PROP_DWA_FISHEYE_VIEW_MODE:{
      bmcv_fisheye_view_mode_e enViewMode = g_value_get_enum (value);
      self->fisheye_attr.enViewMode = enViewMode;
      break;
    }
    case PROP_DWA_FISHEYE_GRID_INFO_NAME:{
      gchar *fisheye_gridinfo_name = g_value_get_string (value);
      memcpy(self->fisheye_gridinfo_name,fisheye_gridinfo_name,128);
      break;
    }
    case PROP_DWA_FISHEYE_GRID_INFO_SIZE:{
      guint  fisheye_gridinfo_size = g_value_get_uint (value);
      self->fisheye_gridinfo_size = fisheye_gridinfo_size;
      break;
    }
    case PROP_DWA_GDC_ASPECT:{
      gboolean bAspect = g_value_get_boolean (value);
      self->ldc_attr.bAspect = bAspect;
      break;
    }
    case PROP_DWA_GDC_X_RATIO:{
      int s32XRatio = g_value_get_int (value);
      self->ldc_attr.s32XRatio = s32XRatio;
      break;
    }
    case PROP_DWA_GDC_Y_RATIO:{
      int s32YRatio = g_value_get_int (value);
      self->ldc_attr.s32YRatio = s32YRatio;
      break;
    }
    case PROP_DWA_GDC_XY_RATIO:{
      int s32XYRatio = g_value_get_int (value);
      self->ldc_attr.s32XYRatio = s32XYRatio;
      break;
    }
    case PROP_DWA_GDC_CENTER_X_OFFSET:{
      int s32CenterXOffset = g_value_get_int (value);
      self->ldc_attr.s32CenterXOffset = s32CenterXOffset;
      break;
    }
    case PROP_DWA_GDC_CENTER_Y_OFFSET:{
      int s32CenterYOffset = g_value_get_int (value);
      self->ldc_attr.s32CenterYOffset = s32CenterYOffset;
      break;
    }
    case PROP_DWA_GDC_DISTORT_RATIO:{
      int s32DistortionRatio = g_value_get_int (value);
      self->ldc_attr.s32DistortionRatio = s32DistortionRatio;
      break;
    }
    case PROP_DWA_GDC_GRID_INFO_NAME:{
      gchar* gdc_gridinfo_name = g_value_get_string (value);
      memcpy(self->gdc_gridinfo_name,gdc_gridinfo_name,128);
      break;
    }
    case PROP_DWA_GDC_GRID_INFO_SIZE:{
      guint  gdc_gridinfo_size = g_value_get_uint (value);
      self->gdc_gridinfo_size = gdc_gridinfo_size;
      break;
    }
    case PROP_DWA_DEWARP_GRID_INFO_NAME:{
      gchar* dewarp_gridinfo_name = g_value_get_string (value);
      memcpy(self->dewarp_gridinfo_name,dewarp_gridinfo_name,128);
      GST_INFO_OBJECT(self, "dewarp_gridinfo_name: %s", self->dewarp_gridinfo_name);
      break;
    }
    case PROP_DWA_DEWARP_GRID_INFO_SIZE:{
      guint  dewarp_gridinfo_size = g_value_get_uint (value);
      self->dewarp_gridinfo_size = dewarp_gridinfo_size;
      GST_INFO_OBJECT(self, "dewarp_gridinfo_size: %d ", self->dewarp_gridinfo_size);
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      return;
  }
}

static void
gst_bm_dwa_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  GstBmDWA *self = GST_BM_DWA (object);
switch (prop_id) {
    case PROP_DWA_MODE:{
      g_value_set_enum(value,self->dwa_mode);
      break;
    }
    case PROP_DWA_ROT_MODE:{
      g_value_set_enum(value,self->rot_mode);
      break;
    }
    case PROP_DWA_AFFINE_REG_NUM:{
      g_value_set_uint(value,self->affine_attr.u32RegionNum);
      break;
    }
    case PROP_DWA_AFFINE_SIZE_W:{
      g_value_set_uint(value,self->affine_attr.stDestSize.u32Width);
      break;
    }
    case PROP_DWA_AFFINE_SIZE_H:{
      g_value_set_uint(value,self->affine_attr.stDestSize.u32Height);
      break;
    }
    case PROP_DWA_AFFINE_REG_ATTR:{
      g_value_set_string(value,self->affine_region_attr_name);
      break;
    }
    case PROP_DWA_FISHEYE_EN:{
      g_value_set_boolean(value,self->fisheye_attr.bEnable);
      break;
    }
    case PROP_DWA_FISHEYE_BGCOLOR_EN:{
      g_value_set_boolean(value,self->fisheye_attr.bBgColor);
      break;
    }
    case PROP_DWA_FISHEYE_BGCOLOR_Y:{
      g_value_set_uint(value,self->yuv_8bit_y);
      break;
    }
    case PROP_DWA_FISHEYE_BGCOLOR_U:{
      g_value_set_uint(value,self->yuv_8bit_u);
      break;
    }
    case PROP_DWA_FISHEYE_BGCOLOR_V:{
      g_value_set_uint(value,self->yuv_8bit_v);
      break;
    }
    case PROP_DWA_FISHEYE_HOR_OFFSET:{
      g_value_set_int (value,self->fisheye_attr.s32HorOffset);
      break;
    }
    case PROP_DWA_FISHEYE_VER_OFFSET:{
      g_value_set_int (value,self->fisheye_attr.s32VerOffset);
      break;
    }
    case PROP_DWA_FISHEYE_TRA_COF:{
      g_value_set_uint(value,self->fisheye_attr.u32TrapezoidCoef);
      break;
    }
    case PROP_DWA_FISHEYE_FAN_STRENGTH:{
      g_value_set_int (value,self->fisheye_attr.s32FanStrength);
      break;
    }
    case PROP_DWA_FISHEYE_MOUNT_MODE:{
      g_value_set_enum (value,self->fisheye_attr.enMountMode);
      break;
    }
    case PROP_DWA_FISHEYE_USE_MODE:{
      g_value_set_enum (value,self->fisheye_attr.enUseMode);
      break;
    }
    case PROP_DWA_FISHEYE_REG_NUM:{
      g_value_set_uint (value,self->fisheye_attr.u32RegionNum);
      break;
    }
    case PROP_DWA_FISHEYE_VIEW_MODE:{
      g_value_set_enum (value,self->fisheye_attr.enViewMode);
      break;
    }
    case PROP_DWA_FISHEYE_GRID_INFO_NAME:{
      g_value_set_string (value,self->fisheye_gridinfo_name);
      break;
    }
    case PROP_DWA_FISHEYE_GRID_INFO_SIZE:{
      g_value_set_uint (value,self->fisheye_gridinfo_size);
      break;
    }
    case PROP_DWA_GDC_ASPECT:{
      g_value_set_boolean (value,self->ldc_attr.bAspect);
      break;
    }
    case PROP_DWA_GDC_X_RATIO:{
      g_value_set_int (value,self->ldc_attr.s32XRatio);
      break;
    }
    case PROP_DWA_GDC_Y_RATIO:{
      g_value_set_int (value,self->ldc_attr.s32YRatio);
      break;
    }
    case PROP_DWA_GDC_XY_RATIO:{
      g_value_set_int (value,self->ldc_attr.s32XYRatio);
      break;
    }
    case PROP_DWA_GDC_CENTER_X_OFFSET:{
      g_value_set_int (value,self->ldc_attr.s32CenterXOffset);
      break;
    }
    case PROP_DWA_GDC_CENTER_Y_OFFSET:{
      g_value_set_int (value,self->ldc_attr.s32CenterYOffset);
      break;
    }
    case PROP_DWA_GDC_DISTORT_RATIO:{
      g_value_set_int (value,self->ldc_attr.s32DistortionRatio);
      break;
    }
    case PROP_DWA_GDC_GRID_INFO_NAME:{
      g_value_set_string (value,self->gdc_gridinfo_name);
      break;
    }
    case PROP_DWA_GDC_GRID_INFO_SIZE:{
      g_value_set_uint (value,self->gdc_gridinfo_size);
      break;
    }
    case PROP_DWA_DEWARP_GRID_INFO_NAME:{
      g_value_set_string (value,self->dewarp_gridinfo_name);
      break;
    }
    case PROP_DWA_DEWARP_GRID_INFO_SIZE:{
      g_value_set_uint (value,self->dewarp_gridinfo_size);
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      return;
  }
}

static void
gst_bm_dwa_class_init (GstBmDWAClass * klass)
{
  GstBaseTransformClass *base_transform_class = GST_BASE_TRANSFORM_CLASS(klass);
  GstVideoFilterClass *video_filter_class = GST_VIDEO_FILTER_CLASS(klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  base_transform_class->start = GST_DEBUG_FUNCPTR(gst_bm_dwa_start);
  base_transform_class->stop = GST_DEBUG_FUNCPTR(gst_bm_dwa_stop);
  base_transform_class->transform_caps = GST_DEBUG_FUNCPTR(gst_bmdwa_transform_caps);

  video_filter_class->transform_frame = GST_DEBUG_FUNCPTR(gst_bm_dwa_transform_frame);
  gobject_class->set_property =
      GST_DEBUG_FUNCPTR (gst_bm_dwa_set_property);
  gobject_class->get_property =
      GST_DEBUG_FUNCPTR (gst_bm_dwa_get_property);
  g_object_class_install_property (gobject_class, PROP_DWA_MODE,
      g_param_spec_enum ("dwaMode", "DWA mode",
          "DWA mode",
          GST_TYPE_BM_DWA_MODE, DEFAULT_PROP_DWA_MODE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DWA_ROT_MODE,
      g_param_spec_enum ("RotMode", "DWA rotation mode",
          "DWA rotation mode",
          GST_TYPE_BM_DWA_ROT_MODE, DEFAULT_PROP_DWA_ROT_MODE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_DWA_AFFINE_REG_NUM,
      g_param_spec_uint ("AffineRegNum", "DWA affine region num",
          "DWA affine region num",
          0,65535, DEFAULT_PROP_DWA_AFFINE_REG_NUM,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_DWA_AFFINE_SIZE_W,
      g_param_spec_uint ("AffineSizeW", "DWA affine size width",
          "DWA affine size width",
          0,65535, DEFAULT_PROP_DWA_AFFINE_SIZE_W,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_DWA_AFFINE_SIZE_H,
      g_param_spec_uint ("AffineSizeH", "DWA affine size height",
          "DWA affine size height",
          0,65535, DEFAULT_PROP_DWA_AFFINE_SIZE_H,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_DWA_AFFINE_REG_ATTR,
      g_param_spec_string ("AffineRegAttr", "DWA affine region attr name",
          "DWA affine region attr name",
           DEFAULT_PROP_DWA_AFFINE_REG_ATTR,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_DWA_FISHEYE_EN,
      g_param_spec_boolean ("FisheyeEn", "DWA fisheye enable",
          "DWA fisheye enable",
          DEFAULT_PROP_DWA_FISHEYE_EN,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_DWA_FISHEYE_BGCOLOR_EN,
      g_param_spec_boolean ("FisheyeBgcolorEn", "DWA fisheye bgcolor enable",
          "DWA fisheye bgcolor enable",
          DEFAULT_PROP_DWA_FISHEYE_BGCOLOR_EN,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_DWA_FISHEYE_BGCOLOR_Y,
      g_param_spec_uint ("FisheyeBgcolorY", "DWA fisheye bgcolor Y",
          "DWA fisheye bgcolor Y",
          0,65535, DEFAULT_PROP_DWA_FISHEYE_BGCOLOR_Y,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_DWA_FISHEYE_BGCOLOR_U,
      g_param_spec_uint ("FisheyeBgcolorU", "DWA fisheye bgcolor U",
          "DWA fisheye bgcolor U",
          0,65535, DEFAULT_PROP_DWA_FISHEYE_BGCOLOR_U,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_DWA_FISHEYE_BGCOLOR_V,
      g_param_spec_uint ("FisheyeBgcolorV", "DWA fisheye bgcolor V",
          "DWA fisheye bgcolor V",
          0,65535, DEFAULT_PROP_DWA_FISHEYE_BGCOLOR_V,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_DWA_FISHEYE_HOR_OFFSET,
      g_param_spec_int ("FisheyeHorOffset", "DWA fisheye HorOffet",
          "DWA fisheye HorOffet",
          -65535,65535, DEFAULT_PROP_DWA_FISHEYE_HOR_OFFSET,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_DWA_FISHEYE_VER_OFFSET,
      g_param_spec_int ("FisheyeVerOffset", "DWA fisheye VerOffet",
          "DWA fisheye VerOffet",
          -65535,65535, DEFAULT_PROP_DWA_FISHEYE_VER_OFFSET,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_DWA_FISHEYE_TRA_COF,
      g_param_spec_uint ("FisheyeTraCoef", "DWA fisheye TrapezoidCoef",
          "DWA fisheye TrapezoidCoef",
          0,65535, DEFAULT_PROP_DWA_FISHEYE_TRA_COF,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_DWA_FISHEYE_FAN_STRENGTH,
      g_param_spec_int ("FisheyeFanStrength", "DWA fisheye FanStrength",
          "DWA fisheye FanStrength",
          -65535,65535, DEFAULT_PROP_DWA_FISHEYE_FAN_STRENGTH,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_DWA_FISHEYE_MOUNT_MODE,
      g_param_spec_enum ("FisheyeMountMode", "DWA fisheye mount mode",
          "DWA fisheye mount mode",
          GST_TYPE_BM_DWA_FISHEYE_MOUNT_MODE, DEFAULT_PROP_DWA_FISHEYE_MOUNT_MODE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_DWA_FISHEYE_USE_MODE,
      g_param_spec_enum ("FisheyeUseMode", "DWA fisheye use mode",
          "DWA fisheye use mode",
          GST_TYPE_BM_DWA_FISHEYE_USAGE_MODE, DEFAULT_PROP_DWA_FISHEYE_USE_MODE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_DWA_FISHEYE_REG_NUM,
      g_param_spec_uint ("FisheyeRegNum", "DWA fisheye region num",
          "DWA fisheye region num",
          0,65535, DEFAULT_PROP_DWA_FISHEYE_REG_NUM,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_DWA_FISHEYE_VIEW_MODE,
      g_param_spec_enum ("FisheyeViewMode", "DWA fisheye view mode",
          "DWA fisheye view mode",
          GST_TYPE_BM_DWA_FISHEYE_VIEW_MODE, DEFAULT_PROP_DWA_FISHEYE_VIEW_MODE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_DWA_FISHEYE_GRID_INFO_NAME,
      g_param_spec_string ("FisheyeGridinfoName", "DWA fisheye gridinfo name",
          "DWA fisheye gridinfo name",
           DEFAULT_PROP_DWA_FISHEYE_GRID_INFO_NAME,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_DWA_FISHEYE_GRID_INFO_SIZE,
      g_param_spec_uint ("FisheyeGridinfoSize", "DWA fisheye gridinfo size",
          "DWA fisheye gridinfo size",
          0,4294967295, DEFAULT_PROP_DWA_FISHEYE_GRID_INFO_SIZE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_DWA_GDC_ASPECT,
      g_param_spec_boolean ("GdcAspect", "DWA gdc aspect",
          "DWA gdc aspect",
           DEFAULT_PROP_DWA_GDC_ASPECT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_DWA_GDC_X_RATIO,
      g_param_spec_int ("GdcXRatio", "DWA gdc X ratio",
          "DWA gdc X ratio",
          0,100, DEFAULT_PROP_DWA_GDC_X_RATIO,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_DWA_GDC_Y_RATIO,
      g_param_spec_int ("GdcYRatio", "DWA gdc Y ratio",
          "DWA gdc Y ratio",
          0,100, DEFAULT_PROP_DWA_GDC_Y_RATIO,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_DWA_GDC_XY_RATIO,
      g_param_spec_int ("GdcXYRatio", "DWA gdc XY ratio",
          "DWA gdc XY ratio",
          0,100, DEFAULT_PROP_DWA_GDC_XY_RATIO,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_DWA_GDC_CENTER_X_OFFSET,
      g_param_spec_int ("GdcCenterXOffset", "DWA gdc CENTER_X_OFFSET",
          "DWA gdc CENTER_X_OFFSET",
          -511,511, DEFAULT_PROP_DWA_GDC_CENTER_X_OFFSET,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_DWA_GDC_CENTER_Y_OFFSET,
      g_param_spec_int ("GdcCenterYOffset", "DWA gdc CENTER_Y_OFFSET",
          "DWA gdc CENTER_Y_OFFSET",
          -511,511, DEFAULT_PROP_DWA_GDC_CENTER_Y_OFFSET,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_DWA_GDC_DISTORT_RATIO,
      g_param_spec_int ("GdcDistortRatio", "DWA gdc DISTORT_RATIO",
          "DWA gdc DISTORT_RATIO",
          -511,511, DEFAULT_PROP_DWA_GDC_DISTORT_RATIO,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_DWA_GDC_GRID_INFO_NAME,
      g_param_spec_string ("GdceGridinfoName", "DWA Gdc gridinfo name",
          "DWA Gdc gridinfo name",
           DEFAULT_PROP_DWA_GDC_GRID_INFO_NAME,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_DWA_GDC_GRID_INFO_SIZE,
      g_param_spec_uint ("GdcGridinfoSize", "DWA Gdc gridinfo size",
          "DWA Gdc gridinfo size",
          0,4294967295, DEFAULT_PROP_DWA_GDC_GRID_INFO_SIZE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_DWA_DEWARP_GRID_INFO_NAME,
      g_param_spec_string ("DewarpGridinfoName", "DWA Dewarp gridinfo name",
          "DWA Dewarp gridinfo name",
           DEFAULT_PROP_DWA_DEWARP_GRID_INFO_NAME,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_DWA_DEWARP_GRID_INFO_SIZE,
      g_param_spec_uint ("DewarpGridinfoSize", "DWA Gdc gridinfo size",
          "DWA Gdc gridinfo size",
          0,4294967295, DEFAULT_PROP_DWA_DEWARP_GRID_INFO_SIZE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gst_bm_dwa_subclass_init(gpointer glass, gpointer class_data)
{
  /* subclass initialization logic */
  GstBmDWAClass *bmdwa_class = GST_BM_DWA_CLASS(glass);
  GstElementClass *element_class = GST_ELEMENT_CLASS(glass);
  GstBmDWAClassData *cdata = class_data;

  gst_element_class_add_pad_template(
      element_class, gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
                                          cdata->sink_caps));
  gst_element_class_add_pad_template(
      element_class, gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                                          cdata->src_caps));

  gst_element_class_set_static_metadata(element_class,
                                        "SOPHGO DWA",
                                        "Filter/Effect/Video",
                                        "SOPHGO Video Process Sub-System",
                                        "zhongbin wang <zhongbin.wang@sophgo.com>");

  bmdwa_class->soc_index = cdata->soc_index;

  gst_caps_unref(cdata->sink_caps);
  gst_caps_unref(cdata->src_caps);
  g_free(cdata);
}

static void
gst_bm_dwa_init(GstBmDWA *self)
{
  /* initialization logic */
  self->allocator = gst_bm_allocator_new();
  self->dwa_mode = DEFAULT_PROP_DWA_MODE;
  self->rot_mode = DEFAULT_PROP_DWA_ROT_MODE;
  self->affine_attr.u32RegionNum = DEFAULT_PROP_DWA_AFFINE_REG_NUM;
  self->affine_attr.stDestSize.u32Width = DEFAULT_PROP_DWA_AFFINE_SIZE_W;
  self->affine_attr.stDestSize.u32Height = DEFAULT_PROP_DWA_AFFINE_SIZE_H;
  memset(self->affine_region_attr_name,DEFAULT_PROP_DWA_AFFINE_REG_ATTR,sizeof(char));
  memcpy(self->affine_attr.astRegionAttr, faces, sizeof(faces));
  self->fisheye_attr.bEnable = DEFAULT_PROP_DWA_FISHEYE_EN;
  self->fisheye_attr.bBgColor = DEFAULT_PROP_DWA_FISHEYE_BGCOLOR_EN;
  self->fisheye_attr.u32BgColor = DEFAULT_PROP_DWA_FISHEYE_BGCOLOR_Y;
  self->fisheye_attr.s32HorOffset = DEFAULT_PROP_DWA_FISHEYE_HOR_OFFSET;
  self->fisheye_attr.s32VerOffset = DEFAULT_PROP_DWA_FISHEYE_VER_OFFSET;
  self->fisheye_attr.u32TrapezoidCoef = DEFAULT_PROP_DWA_FISHEYE_TRA_COF;
  self->fisheye_attr.s32FanStrength = DEFAULT_PROP_DWA_FISHEYE_FAN_STRENGTH;
  self->fisheye_attr.enMountMode = DEFAULT_PROP_DWA_FISHEYE_MOUNT_MODE;
  self->fisheye_attr.u32RegionNum = DEFAULT_PROP_DWA_FISHEYE_REG_NUM;
  self->fisheye_attr.enViewMode = DEFAULT_PROP_DWA_FISHEYE_VIEW_MODE;
  memset(self->affine_region_attr_name,DEFAULT_PROP_DWA_FISHEYE_GRID_INFO_NAME,sizeof(char));
  self->fisheye_gridinfo_size = DEFAULT_PROP_DWA_FISHEYE_GRID_INFO_SIZE;
  self->ldc_attr.bAspect = DEFAULT_PROP_DWA_GDC_ASPECT;
  self->ldc_attr.s32XRatio = DEFAULT_PROP_DWA_GDC_X_RATIO;
  self->ldc_attr.s32YRatio = DEFAULT_PROP_DWA_GDC_Y_RATIO;
  self->ldc_attr.s32XYRatio = DEFAULT_PROP_DWA_GDC_XY_RATIO;
  self->ldc_attr.s32CenterXOffset = DEFAULT_PROP_DWA_GDC_CENTER_X_OFFSET;
  self->ldc_attr.s32CenterYOffset = DEFAULT_PROP_DWA_GDC_CENTER_Y_OFFSET;
  self->ldc_attr.s32DistortionRatio = DEFAULT_PROP_DWA_GDC_DISTORT_RATIO;
  memset(self->affine_region_attr_name,DEFAULT_PROP_DWA_GDC_GRID_INFO_NAME,sizeof(char));
  self->gdc_gridinfo_size = DEFAULT_PROP_DWA_GDC_GRID_INFO_SIZE;
  memset(self->affine_region_attr_name,DEFAULT_PROP_DWA_DEWARP_GRID_INFO_NAME,sizeof(char));
  self->dewarp_gridinfo_size = DEFAULT_PROP_DWA_DEWARP_GRID_INFO_SIZE;
  self->yuv_8bit_y = DEFAULT_PROP_DWA_FISHEYE_BGCOLOR_Y;
  self->yuv_8bit_u = DEFAULT_PROP_DWA_FISHEYE_BGCOLOR_U;
  self->yuv_8bit_v = DEFAULT_PROP_DWA_FISHEYE_BGCOLOR_V;
  GST_DEBUG_OBJECT (self, "bmdwa initial");
}

static void
gst_bm_dwa_subinstance_init(GTypeInstance G_GNUC_UNUSED *instance,
                             gpointer G_GNUC_UNUSED g_class)
{
  // TODO: do init
}

static gboolean
gst_bm_dwa_start (GstBaseTransform * trans)
{
  GstBmDWA *self = GST_BM_DWA (trans);

  GST_DEBUG_OBJECT (self, "bmdwa start");

  return TRUE;
}

static gboolean
gst_bm_dwa_stop (GstBaseTransform * trans)
{
  GstBmDWA *self = GST_BM_DWA (trans);

  GST_DEBUG_OBJECT (self, "bmdwa stop");

  return TRUE;
}


static GstCaps *
gst_bmdwa_transform_caps(GstBaseTransform *trans,
                          GstPadDirection direction,
                          GstCaps *caps, GstCaps *filter)
{
  GstCaps *ret = NULL;
  GstBmDWA *self = GST_BM_DWA(trans);

  if (direction == GST_PAD_SRC) {
    // GstPad *sinkpad = gst_element_get_static_pad(GST_ELEMENT(trans), "sink");
    GstPad *sinkpad = GST_BASE_TRANSFORM_SINK_PAD(trans);
    if (sinkpad) {
      GstCaps *upstream_caps = gst_pad_peer_query_caps(sinkpad, filter);
      // gst_object_unref(sinkpad);
      if (upstream_caps) {
        ret = gst_caps_copy(upstream_caps);
        gst_caps_unref(upstream_caps);
        GST_DEBUG_OBJECT(self, "sinkpad caps: %s", gst_caps_to_string(ret));
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
        GST_DEBUG_OBJECT(self, "srcpad caps: %s", gst_caps_to_string(ret));
      }
    }
  }

  return ret;
}

static GstFlowReturn
gst_bm_dwa_transform_frame (GstVideoFilter * filter, GstVideoFrame * inframe,
    GstVideoFrame * outframe)
{
  GstFlowReturn ret = GST_FLOW_OK;
  GstBmDWA *self = GST_BM_DWA (filter);

  bm_handle_t bm_handle = NULL;
  bm_dev_request(&bm_handle, 0);

  bm_image src ;
  bm_image dst ;


  bmcv_rect_t crop_rect = {0, 0, inframe->info.width, inframe->info.height};

  bm_image_format_ext bmInFormat =
      (bm_image_format_ext)map_gstformat_to_bmformat(inframe->info.finfo->format);
  int in_height = inframe->info.height;
  int in_width = inframe->info.width;

  bm_image_format_ext bmOutFormat =
      (bm_image_format_ext)map_gstformat_to_bmformat(outframe->info.finfo->format);
  int out_height = outframe->info.height;
  int out_width = outframe->info.width;

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

  bm_image_create(bm_handle, in_height, in_width, bmInFormat, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
  bm_image_alloc_dev_mem(src, BMCV_HEAP_ANY);
  bm_image_create(bm_handle, out_height, out_width, bmOutFormat, DATA_TYPE_EXT_1N_BYTE, &dst, NULL);
  bm_image_alloc_dev_mem(dst, BMCV_HEAP_ANY);

  guint8 *src_in_ptr[4];

#if 0
  static gint count = 0;
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

  for (gint i = 0; i < GST_VIDEO_FRAME_N_PLANES(inframe); i++) {
    src_in_ptr[i] = GST_VIDEO_FRAME_PLANE_DATA(inframe, i);
  }

  guint8 *dst_in_ptr[4];
  for (gint i = 0; i < GST_VIDEO_FRAME_N_PLANES(outframe); i++) {
    dst_in_ptr[i] = GST_VIDEO_FRAME_PLANE_DATA(outframe, i);
  }

  bm_image_copy_host_to_device(src, (void **)src_in_ptr);
  if(self->dwa_mode == GTS_BM_DWA_ROT){
    bmcv_dwa_rot(bm_handle, src, dst, self->rot_mode);
  }else if(self->dwa_mode == GTS_BM_DWA_AFFINE){
    bmcv_dwa_affine(bm_handle, src, dst, self->affine_attr);
  }else if(self->dwa_mode == GTS_BM_DWA_FISHEYE){
    self->fisheye_attr.bBgColor = YUV_8BIT(self->yuv_8bit_y, self->yuv_8bit_u, self->yuv_8bit_v);
    bmcv_dwa_fisheye(bm_handle, src, dst, self->fisheye_attr);
  }else if(self->dwa_mode == GTS_BM_DWA_GDC){
    bmcv_dwa_gdc(bm_handle, src, dst, self->ldc_attr);
  }else if(self->dwa_mode == GTS_BM_DWA_DEWARP){
    FILE *fp = fopen(self->dewarp_gridinfo_name, "rb");
    if (!fp) {
        printf("open file:%s failed.\n", self->dewarp_gridinfo_name);
        exit(-1);
    }
    char *buffer =(char *)malloc(self->dewarp_gridinfo_size);
    self->dmem.u.system.system_addr = (void *)buffer;
    self->dmem.size = self->dewarp_gridinfo_size;
    GST_INFO_OBJECT(self, "dewarp_gridinfo_name: %s, size: %d ", self->dewarp_gridinfo_name,self->dewarp_gridinfo_size);
    gint size = fread(buffer, 1, self->dewarp_gridinfo_size, fp);
    GST_INFO_OBJECT(self, "load size: %d ", size);
    bmcv_dwa_dewarp(bm_handle, src, dst, self->dmem);
    GST_INFO_OBJECT(self, "dewarp 1 ");
    free(buffer);
    GST_INFO_OBJECT(self, "dewarp 2 ");
  }
  bm_image_copy_device_to_host(dst, (void **)dst_in_ptr);
  GST_INFO_OBJECT(self, "dewarp 3 ");
  bm_image_destroy(&src);
  bm_image_destroy(&dst);
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
GstVideoFormat map_bmformat_to_gstformat(int bm_format)
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

void gst_bm_dwa_register(GstPlugin *plugin, guint soc_idx, GstCaps *sink_caps,
                          GstCaps *src_caps)
{
  GType type;
  gchar *type_name;
  GstBmDWAClassData *class_data;
  GTypeInfo type_info = {
    sizeof (GstBmDWAClass),
    NULL,
    NULL,
    (GClassInitFunc) gst_bm_dwa_subclass_init,
    NULL,
    NULL,
    sizeof (GstBmDWA),
    0,
    (GInstanceInitFunc) gst_bm_dwa_subinstance_init,
    NULL,
  };

  GST_DEBUG_CATEGORY_INIT (gst_bm_dwa_debug, "bmdwa", 0, "BM DWA Plugin");

  class_data = g_new0 (GstBmDWAClassData, 1);
  class_data->sink_caps = gst_caps_ref (sink_caps);
  class_data->src_caps = gst_caps_ref (src_caps);
  class_data->soc_index = soc_idx;

  type_name = g_strdup (GST_BM_DWA_TYPE_NAME);
  type_info.class_data = class_data;
  type = g_type_register_static(GST_TYPE_BM_DWA, type_name, &type_info, 0);

  if (!gst_element_register(plugin, type_name, GST_RANK_PRIMARY, type)) {
    GST_WARNING("Failed to register DWA plugin '%s'", type_name);
  }

  g_free (type_name);
}
