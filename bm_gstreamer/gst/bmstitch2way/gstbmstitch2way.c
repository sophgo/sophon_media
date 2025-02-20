#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "gstbmstitch2way.h"
#include "bmlib_runtime.h"
#include "gstbmallocator.h"
#include <gst/base/gstbasetransform.h>

#define  DEFAULT_PROP_STITCH_DST_W        6912
#define  DEFAULT_PROP_STITCH_DST_H        288
#define  DEFAULT_PROP_STITCH_OVLP_LX      2304
#define  DEFAULT_PROP_STITCH_OVLP_RX      4607
#define  DEFAULT_PROP_STITCH_WGT_NAME_L   0
#define  DEFAULT_PROP_STITCH_WGT_NAME_R   0
#define  DEFAULT_PROP_STITCH_WGT_MODE     BM_STITCH_WGT_YUV_SHARE
#define ALIGN(x, a)      (((x) + ((a)-1)) & ~((a)-1))
GST_DEBUG_CATEGORY (gst_bm_stitch2way_debug);
#define GST_CAT_DEFAULT gst_bm_stitch2way_debug

#define GST_BM_STITCH_MUTEX(stitch) (&GST_BM_STITCH2WAY (stitch)->mutex)

#define GST_BM_STITCH_LOCK(stitch) \
  g_mutex_lock (GST_BM_STITCH_MUTEX (stitch));

#define GST_BM_STITCH_UNLOCK(stitch) \
  g_mutex_unlock (GST_BM_STITCH_MUTEX (stitch));

static gint map_gstformat_to_bmformat(GstVideoFormat gst_format);
static GstVideoFormat map_bmformat_to_gstformat(gint bm_format);
static GstFlowReturn
gst_bm_stitch_process_buffers (GstBmSTITCH2WAY *self, GstBuffer *outBuffer);
/* class initialization */
G_DEFINE_TYPE(GstBmSTITCH2WAY, gst_bm_stitch2way, GST_TYPE_VIDEO_AGGREGATOR);
static gpointer parent_class = NULL;
/*
static GstStaticPadTemplate gst_bm_stitch_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw, " \
    "format = (string) {I420, Y42B, Y444, GRAY8, NV12, NV21, NV16, RGB, BGR}, " \
    "width = (int) [ 64, 8604 ], " \
    "height = (int) [ 64, 4080 ], " \
    "framerate = " GST_VIDEO_FPS_RANGE));
*/

static GstStaticPadTemplate gst_bm_stitch_sink1_template =
GST_STATIC_PAD_TEMPLATE ("sink1",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw, " \
    "format = (string) {I420, Y42B, Y444, GRAY8, NV12, NV21, NV16, RGB, BGR}, " \
    "width = (int) [ 64, 4608 ], " \
    "height = (int) [ 64, 4080 ], " \
    "framerate = " GST_VIDEO_FPS_RANGE));

static GstStaticPadTemplate gst_bm_stitch_sink2_template =
GST_STATIC_PAD_TEMPLATE ("sink2",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw, " \
    "format = (string) {I420, Y42B, Y444, GRAY8, NV12, NV21, NV16, RGB, BGR}, " \
    "width = (int) [ 64, 4608 ], " \
    "height = (int) [ 64, 4080 ], " \
    "framerate = " GST_VIDEO_FPS_RANGE));

enum
{
  PROP_0,
  PROP_STITCH_DST_W,
  PROP_STITCH_DST_H,
  PROP_STITCH_OVLP_LX,
  PROP_STITCH_OVLP_RX,
  PROP_STITCH_WGT_NAME_L,
  PROP_STITCH_WGT_NAME_R,
  PROP_STITCH_WGT_MODE,
};

#define GST_TYPE_BM_STITCH2WAY_WGT_MODE (gst_bm_stitch2way_wgt_mode_get_type ())
static GType
gst_bm_stitch2way_wgt_mode_get_type (void)
{
  static GType profile = 0;

  if (!profile) {
    static const GEnumValue profiles[] = {
      {BM_STITCH_WGT_YUV_SHARE, "wgt yuv share", "wgt yuv share"},
      {BM_STITCH_WGT_UV_SHARE, "wgt uv share", "wgt uv share"},
      {BM_STITCH_WGT_SEP, "wgt sep", "wgt sep"},
      {0, NULL, NULL},
    };
    profile = g_enum_register_static ("GstBmSTITCH2WAYWgtMode", profiles);
  }
  return profile;
}

static void bm_dem_read_bin(bm_handle_t handle, bm_device_mem_t* dmem, const char *input_name, unsigned int size)
{
  if (access(input_name, F_OK) != 0 || strlen(input_name) == 0 || 0 >= size)
  {
    return;
  }

  char* input_ptr = (char *)malloc(size);
  FILE *fp_src = fopen(input_name, "rb+");

  if (fread((void *)input_ptr, 1, size, fp_src) < (unsigned int)size){
      printf("file size is less than %d required bytes\n", size);
  };
  fclose(fp_src);

  if (BM_SUCCESS != bm_malloc_device_byte(handle, dmem, size)){
    printf("bm_malloc_device_byte failed\n");
  }


  if (BM_SUCCESS != bm_memcpy_s2d(handle, *dmem, input_ptr)){
    printf("bm_memcpy_s2d failed\n");
  }

  free(input_ptr);
  return;
}

static void
gst_bm_stitch_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GstBmSTITCH2WAY *self = GST_BM_STITCH2WAY (object);

  switch (prop_id) {
    case PROP_STITCH_DST_W:{
      gint width = g_value_get_int (value);
      self->dst_w = width;
      break;
    }
    case PROP_STITCH_DST_H:{
      gint height = g_value_get_int (value);
      self->dst_h = height;
      break;
    }
    case PROP_STITCH_OVLP_LX:{
      short ovlp_lx = (short)g_value_get_int (value);
      self->stitch_config.ovlap_attr.ovlp_lx[0] = ovlp_lx;
      break;
    }
    case PROP_STITCH_OVLP_RX:{
      short ovlp_rx = (short)g_value_get_int (value);
      self->stitch_config.ovlap_attr.ovlp_rx[0] = ovlp_rx;
      break;
    }
    case PROP_STITCH_WGT_NAME_L:{
      const gchar *wgt_name_l = g_value_get_string (value);
      memcpy(self->wgt_name_l,wgt_name_l,128);
      break;
    }
    case PROP_STITCH_WGT_NAME_R:{
      const gchar *wgt_name_r = g_value_get_string (value);
      memcpy(self->wgt_name_r,wgt_name_r,128);
      break;
    }
    case PROP_STITCH_WGT_MODE:{
      enum bm_stitch_wgt_mode wgt_mode = g_value_get_enum (value);
      self->stitch_config.wgt_mode = wgt_mode;
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      return;
  }
}

static void __attribute__((unused))
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
    printf("written size(%d) != info.size(%ld)\n", (unsigned int)w, info.size);
    return ;
  }
  gst_buffer_unmap(buffer, &info);
  fflush(output_file);
  fclose(output_file);
}

static void
gst_bm_stitch_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  GstBmSTITCH2WAY *self = GST_BM_STITCH2WAY (object);

  switch (prop_id) {
    case PROP_STITCH_DST_W:{
      g_value_set_int(value,self->dst_w);
      break;
    }
    case PROP_STITCH_DST_H:{
      g_value_set_int (value,self->dst_h);
      break;
    }
    case PROP_STITCH_OVLP_LX:{
      g_value_set_int (value,(int)self->stitch_config.ovlap_attr.ovlp_lx[0]);
      break;
    }
    case PROP_STITCH_OVLP_RX:{
      g_value_set_int (value,(int)self->stitch_config.ovlap_attr.ovlp_rx[0]);
      break;
    }
    case PROP_STITCH_WGT_NAME_L:{
      g_value_set_string (value,*self->wgt_name_l);
      break;
    }
    case PROP_STITCH_WGT_NAME_R:{
      g_value_set_string (value,*self->wgt_name_r);
      break;
    }
    case PROP_STITCH_WGT_MODE:{
      g_value_set_enum (value,self->stitch_config.wgt_mode);
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      return;
  }
}

static GstFlowReturn
gst_bm_stitch_aggregate (GstVideoAggregator  * vagg, GstBuffer*  outbuf)
{
  GstFlowReturn ret;
  GstBmSTITCH2WAY *bmstitch = GST_BM_STITCH2WAY(vagg);
  GST_BM_STITCH_LOCK(bmstitch);
//  GstElementClass *klass = GST_ELEMENT_GET_CLASS (vagg);
//  GstVideoAggregatorClass *vagg_klass = (GstVideoAggregatorClass *) klass;
  GST_INFO_OBJECT(bmstitch, "gst_bm_stitch_aggregate start! \n");
  ret = gst_bm_stitch_process_buffers(bmstitch,outbuf);
  if(ret != GST_FLOW_OK) {
    GST_ERROR_OBJECT(bmstitch, "gst_bm_stitch_process_buffers failed. ret = %d \n", ret);
    goto done;
  }
  done:
    GST_INFO_OBJECT(bmstitch, "gst_bm_stitch_aggregate end! \n");
    GST_BM_STITCH_UNLOCK(bmstitch);
    return ret;
}

static GstFlowReturn
gst_bm_stitch_create_output_buffer (GstVideoAggregator * videoaggregator,
    GstBuffer ** outbuf)
{
  GstAggregator *aggregator = GST_AGGREGATOR (videoaggregator);
  GstBufferPool *pool;
  GstFlowReturn ret = GST_FLOW_OK;
  GstBmSTITCH2WAY * self = GST_BM_STITCH2WAY (videoaggregator);

  pool = gst_aggregator_get_buffer_pool (aggregator);

  if (pool) {
    if (!gst_buffer_pool_is_active (pool)) {
      if (!gst_buffer_pool_set_active (pool, TRUE)) {
        GST_ELEMENT_ERROR (videoaggregator, RESOURCE, SETTINGS,
            ("failed to activate bufferpool"),
            ("failed to activate bufferpool"));
        return GST_FLOW_ERROR;
      }
    }

    ret = gst_buffer_pool_acquire_buffer (pool, outbuf, NULL);
    gst_object_unref (pool);
  } else {
    guint outsize;
    GstAllocator *allocator;
    GstAllocationParams params;

    gst_aggregator_get_allocator (aggregator, &allocator, &params);

    outsize = GST_VIDEO_INFO_SIZE (&videoaggregator->info);
    *outbuf = gst_buffer_new_allocate (allocator, outsize, &params);
    GST_INFO_OBJECT(self, "stitch_create_output_buffer size(%d) \n", outsize);
    if (allocator)
      gst_object_unref (allocator);

    if (*outbuf == NULL) {
      GST_ELEMENT_ERROR (videoaggregator, RESOURCE, NO_SPACE_LEFT,
          (NULL), ("Could not acquire buffer of size: %d", outsize));
      ret = GST_FLOW_ERROR;
    }
  }
  return ret;
}

static void
gst_bm_stitch2way_class_init (GstBmSTITCH2WAYClass * klass)
{
//  GstElementClass *element_class = GST_ELEMENT_CLASS(klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstVideoAggregatorClass *vagg_class = GST_VIDEO_AGGREGATOR_CLASS (klass);
//  GstAggregatorClass *agg_class = GST_AGGREGATOR_CLASS (klass);
  gobject_class->set_property =
      GST_DEBUG_FUNCPTR (gst_bm_stitch_set_property);
  gobject_class->get_property =
      GST_DEBUG_FUNCPTR (gst_bm_stitch_get_property);
  vagg_class->aggregate_frames = GST_DEBUG_FUNCPTR (gst_bm_stitch_aggregate);
  vagg_class->create_output_buffer = GST_DEBUG_FUNCPTR (gst_bm_stitch_create_output_buffer);

  parent_class = g_type_class_peek_parent (klass);
  if(parent_class == NULL){
    printf("parent_class null \n");
  }
  vagg_class->aggregate_frames = GST_DEBUG_FUNCPTR (gst_bm_stitch_aggregate);
  g_object_class_install_property (gobject_class, PROP_STITCH_DST_W,
      g_param_spec_int ("DstW", "STITCH dst width",
          "STITCH dst width",
          0,8604, DEFAULT_PROP_STITCH_DST_W,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_STITCH_DST_H,
      g_param_spec_int ("DstH", "STITCH dst height",
          "STITCH dst height",
          0,2280, DEFAULT_PROP_STITCH_DST_H,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_STITCH_OVLP_LX,
      g_param_spec_int ("OvlpLx", "STITCH overlap lx",
          "STITCH overlap lx",
          0,8604, DEFAULT_PROP_STITCH_OVLP_LX,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_STITCH_OVLP_RX,
      g_param_spec_int ("OvlpRx", "STITCH overlap rx",
          "STITCH overlap rx",
          0,8604, DEFAULT_PROP_STITCH_OVLP_RX,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_STITCH_WGT_NAME_L,
      g_param_spec_string ("WgtNameL", "STITCH wgt name L",
          "STITCH wgt name L",
          DEFAULT_PROP_STITCH_WGT_NAME_L,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_STITCH_WGT_NAME_R,
      g_param_spec_string ("WgtNameR", "STITCH wgt name R",
          "STITCH wgt name R",
          DEFAULT_PROP_STITCH_WGT_NAME_R,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_STITCH_WGT_MODE,
      g_param_spec_enum ("WgtMode", "STITCH wgt mode",
          "STITCH wgt mode",
          GST_TYPE_BM_STITCH2WAY_WGT_MODE,DEFAULT_PROP_STITCH_WGT_MODE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gst_bm_stitch2way_subclass_init(gpointer glass, gpointer class_data)
{
  /* subclass initialization logic */
  GstBmSTITCH2WAYClass *bmstitch_class = GST_BM_STITCH2WAY_CLASS(glass);
  GstElementClass *element_class = GST_ELEMENT_CLASS(glass);
  GstBmSTITCHClass2WAYData *cdata = class_data;

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
                                        "SOPHGO STITCH",
                                        "Filter/Effect/Video",
                                        "SOPHGO Video Process Sub-System",
                                        "Zhongbin Wang <zhongbin.wang@sophgo.com>");

  bmstitch_class->soc_index = cdata->soc_index;

  gst_caps_unref(cdata->sink_caps1);
  gst_caps_unref(cdata->sink_caps2);
  gst_caps_unref(cdata->src_caps);
  g_free(cdata);
}

static void
gst_bm_stitch2way_init(GstBmSTITCH2WAY *self)
{
  /* initialization logic */
  self->allocator = gst_bm_allocator_new();
  self->dst_w = DEFAULT_PROP_STITCH_DST_W;
  self->dst_h = DEFAULT_PROP_STITCH_DST_H;
  self->stitch_config.ovlap_attr.ovlp_lx[0] = DEFAULT_PROP_STITCH_OVLP_LX;
  self->stitch_config.ovlap_attr.ovlp_rx[0] = DEFAULT_PROP_STITCH_OVLP_RX;
  memset(self->wgt_name_l,DEFAULT_PROP_STITCH_WGT_NAME_L,sizeof(self->wgt_name_l[0]) * 128);
  memset(self->wgt_name_r,DEFAULT_PROP_STITCH_WGT_NAME_R,sizeof(self->wgt_name_l[0]) * 128);
  self->stitch_config.wgt_mode = DEFAULT_PROP_STITCH_WGT_MODE;
  GST_DEBUG_OBJECT (self, "bmstitch initial");

  GST_INFO_OBJECT (self, "bmstitch initial");
}

static void
gst_bm_stitch2way_subinstance_init(GTypeInstance G_GNUC_UNUSED *instance,
                             gpointer G_GNUC_UNUSED g_class)
{
  GstElement *element = GST_ELEMENT(instance);
//  GstElementClass *element_class = GST_ELEMENT_GET_CLASS(instance);
  GstBmSTITCH2WAY *self = GST_BM_STITCH2WAY(instance);
  GstPadTemplate *sink1_pad_template,*sink2_pad_template;
  sink1_pad_template = gst_static_pad_template_get (&gst_bm_stitch_sink1_template);
  if (!sink1_pad_template) {
    g_print ("Failed to get sink1 pad template\n");
    return;
  }

  sink2_pad_template = gst_static_pad_template_get (&gst_bm_stitch_sink2_template);
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
    return;
  }

  if (!GST_IS_VIDEO_AGGREGATOR_PAD(self->sink2_pad)) {
    g_print("Error: pad2 is not a GstVideoAggregatorPad\n");
    gst_object_unref(GST_PAD(self->sink1_pad));
    gst_object_unref(GST_PAD(self->sink2_pad));
    return;
  }
  gst_element_add_pad(element,GST_PAD(self->sink1_pad));
  gst_element_add_pad(element,GST_PAD(self->sink2_pad));
  g_print ("element_class sink pad num[%d] \n",element->numsinkpads);
}

static GstFlowReturn
gst_bm_stitch_process_frame (bm_handle_t bm_handle, GstBmSTITCH2WAY *self, GstVideoFrame *inFrameL,
    GstVideoFrame *inFrameR, GstVideoFrame *outFrame)
{
  GstFlowReturn ret = GST_FLOW_OK;
//  gint src_stride[4];
  bm_image    src[2];
  bm_image       dst;
  int  wgt_len ,wgtWidth, wgtHeight;
  char** wgt_name_l;
  char** wgt_name_r;

  bm_image_format_ext bmInFormatL =
      (bm_image_format_ext)map_gstformat_to_bmformat(inFrameL->info.finfo->format);
  gint in_height_L = inFrameL->info.height;
  gint in_width_L = inFrameL->info.width;

  bm_image_format_ext bmInFormatR =
      (bm_image_format_ext)map_gstformat_to_bmformat(inFrameR->info.finfo->format);
  gint in_height_R = inFrameR->info.height;
  gint in_width_R = inFrameR->info.width;

//  bm_image_format_ext bmOutFormat =
//      (bm_image_format_ext)map_gstformat_to_bmformat(outFrame->info.finfo->format);
  gint out_height = outFrame->info.height;
  gint out_width = outFrame->info.width;

  ret = bm_image_create(bm_handle, in_height_L, in_width_L, bmInFormatL, DATA_TYPE_EXT_1N_BYTE, &src[0], NULL);
  if(ret != (GstFlowReturn)BM_SUCCESS){
      GST_ERROR_OBJECT(self, "left bm_image create failed. ret = %d \n", ret);
      return GST_FLOW_ERROR;
  }

  ret = bm_image_create(bm_handle, in_height_R, in_width_R, bmInFormatR, DATA_TYPE_EXT_1N_BYTE, &src[1], NULL);
  if(ret != (GstFlowReturn)BM_SUCCESS){
      GST_ERROR_OBJECT(self, "right bm_image create failed. ret = %d \n", ret);
      return GST_FLOW_ERROR;
  }

  ret = bm_image_create(bm_handle, out_height, out_width, FORMAT_YUV420P, DATA_TYPE_EXT_1N_BYTE, &dst, NULL);
  if(ret != (GstFlowReturn)BM_SUCCESS){
      GST_ERROR_OBJECT(self, "output bm_image create failed. ret = %d \n", ret);
      return GST_FLOW_ERROR;
  }
  GST_INFO_OBJECT(self, "Dst image datatype(%d) width(%d) height(%d) format(%d)\n",
                    dst.data_type, dst.width, dst.height, dst.image_format);
  wgtWidth = ALIGN(self->stitch_config.ovlap_attr.ovlp_rx[0] - self->stitch_config.ovlap_attr.ovlp_lx[0] + 1, 16);
  wgtHeight = in_height_R;
  wgt_len = wgtWidth*wgtHeight;
  wgt_name_l = self->wgt_name_l;
  wgt_name_r = self->wgt_name_r;
  bm_dem_read_bin(bm_handle, &self->stitch_config.wgt_phy_mem[0][0], *wgt_name_l,  wgt_len);
  bm_dem_read_bin(bm_handle, &self->stitch_config.wgt_phy_mem[0][1], *wgt_name_r,  wgt_len);
  GST_INFO_OBJECT(self, "wgt_len(%d) wgtWidth(%d) wgtHeight(%d) \n",
                    wgt_len, wgtWidth, wgtHeight);
  GST_INFO_OBJECT(self, "wgt_phy_mem_L name(%s) \n",*wgt_name_l);
  GST_INFO_OBJECT(self, "wgt_phy_mem_R name(%s) \n",*wgt_name_r);
  if(bm_image_alloc_dev_mem(dst, 1) != BM_SUCCESS){
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

  bm_image_copy_host_to_device(src[0], (void **)src_left_ptr);
  bm_image_copy_host_to_device(src[1], (void **)src_right_ptr);
  GST_INFO_OBJECT(self, "src image1 datatype(%d) width(%d) height(%d) format(%d)\n",
                    src[0].data_type,src[0].width,src[0].height,src[0].image_format);
  GST_INFO_OBJECT(self, "src image2 datatype(%d) width(%d) height(%d) format(%d)\n",
                    src[1].data_type,src[1].width,src[1].height,src[1].image_format);
  ret = bmcv_blending(bm_handle, 2, src, dst, self->stitch_config);
  bm_image_copy_device_to_host(dst, (void **)dst_ptr);
  bm_image_destroy(&src[0]);
  bm_image_destroy(&src[1]);
  bm_image_destroy(&dst);
  return ret;
}

static GstFlowReturn
gst_bm_stitch_process_buffers (GstBmSTITCH2WAY *self, GstBuffer *outBuffer)
{
  GstVideoFrame *inFrameL;
  GstVideoFrame *inFrameR;
  GstVideoFrame outFrame;
  GstVideoInfo infoL;
  GstVideoInfo infoR;
  GstVideoInfo infoOut;
//  gint dst_stride[4];
  GstBuffer *inBufferL = NULL;
  GstBuffer *inBufferR = NULL;
//  GstAggregatorPad* pad_L;
//  GstAggregatorPad* pad_R;
  GstVideoAggregatorPad *mpad_L = NULL;
  GstVideoAggregatorPad *mpad_R = NULL;
  gst_video_info_init (&infoL);
  gst_video_info_init (&infoR);
  gst_video_info_init (&infoOut);
  GstAggregator *agg =GST_AGGREGATOR (self);
  GstVideoAggregator *vagg =GST_VIDEO_AGGREGATOR (self);
  GList *l;
  gint num =0;
  GstFlowReturn ret = GST_FLOW_OK;
  bm_handle_t bm_handle = NULL;
//  GstVideoAggregatorPadClass *vaggpad_class;
//  bm_image_data_format_ext output_dtype = DATA_TYPE_EXT_1N_BYTE;
  bm_dev_request(&bm_handle, 0);
  GST_INFO_OBJECT(self, "gst_bm_stitch_process_buffers start! \n");

  if(outBuffer == NULL){
    GST_ERROR_OBJECT(self, "[pre]out buffer null\n");
    return GST_FLOW_ERROR;
  }

  GST_INFO_OBJECT(self, "gst_bm_stitch_process_buffers [1]! \n");
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
  GST_INFO_OBJECT(self, "gst_bm_stitch_process_buffers [1]! \n");
  for (l = GST_ELEMENT (agg)->sinkpads; l; l = l->next) {
    num ++;
    if(num == 1){
      mpad_L = l->data;
    //  pad_L = (GstAggregatorPad*)mpad_L;
      // inBufferL = gst_video_aggregator_pad_get_current_buffer(pad_L);
      // if(inBufferL){
      //   char name[128]="imgL_tmp.bin";
      //   save_output(inBufferL,name);
      // }else{
      //   GST_ERROR_OBJECT(self, "inBufferL NULL\n");
      // }
    }else if(num == 2){
      mpad_R = l->data;
    //  pad_R = (GstAggregatorPad*)mpad_R;
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
  GST_INFO_OBJECT(self, "gst_bm_stitch_process_buffers [2]! \n");
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
  GST_INFO_OBJECT(self, "gst_bm_stitch_process_buffers [3]! \n");

  ret = gst_bm_stitch_process_frame(bm_handle, self, inFrameL, inFrameR, &outFrame);
  if (ret != GST_FLOW_OK) {
    GST_ERROR_OBJECT(self, "gst_bm_stitch_process_frame failed");
    return GST_FLOW_ERROR;
  }
  // if(outBuffer == NULL){
  //   GST_ERROR_OBJECT(self, "[back]out buffer null\n");
  //   return GST_FLOW_ERROR;
  // }else{
  //   char name[128]="outbuffer.bin";
  //   save_output(outBuffer,name);
  // }
  gst_video_frame_unmap(&outFrame);
  GST_INFO_OBJECT(self, "gst_bm_stitch_process_buffers end! \n");
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

static __attribute__((unused))
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

void gst_bm_stitch2way_register(GstPlugin *plugin, guint soc_idx, GstCaps *sink_caps1,
                          GstCaps *sink_caps,GstCaps *src_caps)
{
  GType type;
  gchar *type_name;
  GstBmSTITCHClass2WAYData *class_data;
  GTypeInfo type_info = {
    sizeof (GstBmSTITCH2WAYClass),
    NULL,
    NULL,
    (GClassInitFunc) gst_bm_stitch2way_subclass_init,
    NULL,
    NULL,
    sizeof (GstBmSTITCH2WAY),
    0,
    (GInstanceInitFunc) gst_bm_stitch2way_subinstance_init,
    NULL,
  };

  GST_DEBUG_CATEGORY_INIT (gst_bm_stitch2way_debug, "bmstitch2way", 0, "BM STITCH2WAY Plugin");

  class_data = g_new0 (GstBmSTITCHClass2WAYData, 1);
  class_data->sink_caps1 = gst_caps_ref (sink_caps1);
  class_data->sink_caps2 = gst_caps_ref (sink_caps);
  class_data->src_caps = gst_caps_ref (src_caps);
  class_data->soc_index = soc_idx;

  type_name = g_strdup (GST_BM_STITCH2WAY_TYPE_NAME);
  type_info.class_data = class_data;
  type = g_type_register_static(GST_TYPE_BM_STITCH2WAY, type_name, &type_info, 0);

  if (!gst_element_register(plugin, type_name, GST_RANK_PRIMARY, type)) {
    GST_WARNING("Failed to register STITCH plugin '%s'", type_name);
  }

  g_free (type_name);
}
