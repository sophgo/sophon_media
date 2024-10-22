#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include "gstbmvpss.h"
#include "bmlib_runtime.h"
#include "gstbmallocator.h"
#include <gst/base/gstbasetransform.h>
#include <gst/allocators/gstdmabuf.h>

#include "bmcv_api_ext_c.h"

GST_DEBUG_CATEGORY (gst_bm_vpss_debug);
#define GST_CAT_DEFAULT gst_bm_vpss_debug

enum
{
  PROP_0,
  PROP_LEFT,
  PROP_RIGHT,
  PROP_TOP,
  PROP_BOTTOM,
  PROP_PADDING_STX,
  PROP_PADDING_STY,
  PROP_PADDING_W,
  PROP_PADDING_H,
  PROP_PADDING_R,
  PROP_PADDING_G,
  PROP_PADDING_B
};

static gboolean gst_bm_vpss_start (GstBaseTransform * trans);
static gboolean gst_bm_vpss_stop (GstBaseTransform * trans);
static GstCaps *gst_bmvpss_transform_caps(GstBaseTransform *trans,
                                          GstPadDirection direction,
                                          GstCaps *caps, GstCaps *filter);
static GstFlowReturn gst_bm_vpss_transform_frame(GstVideoFilter *filter,
    GstVideoFrame *inframe, GstVideoFrame *outframe);

static int map_gstformat_to_bmformat(GstVideoFormat gst_format);
// static GstVideoFormat map_bmformat_to_gstformat(int bm_format);

/* class initialization */
G_DEFINE_TYPE(GstBmVPSS, gst_bm_vpss, GST_TYPE_VIDEO_FILTER);

static void
gst_bm_vpss_set_property(GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstBmVPSS *self = GST_BM_VPSS (object);

  switch (prop_id) {
    case PROP_LEFT:
      self->crop_left = g_value_get_int(value);
      break;
    case PROP_RIGHT:
      self->crop_right = g_value_get_int(value);
      break;
    case PROP_TOP:
      self->crop_top = g_value_get_int(value);
      break;
    case PROP_BOTTOM:
      self->crop_bottom = g_value_get_int(value);
      break;
    case PROP_PADDING_STX:
      self->padding_dst_stx = g_value_get_int(value);
      break;
    case PROP_PADDING_STY:
      self->padding_dst_sty = g_value_get_int(value);
      break;
    case PROP_PADDING_W:
      self->padding_dst_w = g_value_get_int(value);
      break;
    case PROP_PADDING_H:
      self->padding_dst_h = g_value_get_int(value);
      break;
    case PROP_PADDING_R:
      self->padding_r = g_value_get_uint(value);
      self->padding_if_memset = 1;
      break;
    case PROP_PADDING_G:
      self->padding_g = g_value_get_uint(value);
      break;
    case PROP_PADDING_B:
      self->padding_b = g_value_get_uint(value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
  if (self->crop_bottom > 0 && self->crop_right > 0) {
    self->crop_enable = TRUE;
    GST_DEBUG_OBJECT(
        self, "Current crop settings - Left: %d, Right: %d, Top: %d, Bottom: %d",
        self->crop_left, self->crop_right, self->crop_top, self->crop_bottom);
  }
  if (self->padding_dst_w > 0 && self->padding_dst_h > 0) {
    self->padding_enable = TRUE;
    GST_DEBUG_OBJECT(self,
                     "Current padding settings - STX:%d, STY: %d, W: %d, H: "
                     "%d, R: %d, G: %d, B: %d, if_memset: %d",
                     self->padding_dst_stx, self->padding_dst_sty,
                     self->padding_dst_w, self->padding_dst_h, self->padding_r,
                     self->padding_g, self->padding_b, self->padding_if_memset);
  }
}

static void
gst_bm_vpss_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstBmVPSS *self = GST_BM_VPSS (object);
  switch (prop_id) {
    case PROP_LEFT:
      g_value_set_int (value, self->crop_left);
      break;
    case PROP_RIGHT:
      g_value_set_int (value, self->crop_right);
      break;
    case PROP_TOP:
      g_value_set_int (value, self->crop_top);
      break;
    case PROP_BOTTOM:
      g_value_set_int (value, self->crop_bottom);
      break;
    case PROP_PADDING_STX:
      g_value_set_int (value, self->padding_dst_stx);
      break;
    case PROP_PADDING_STY:
      g_value_set_int (value, self->padding_dst_sty);
      break;
    case PROP_PADDING_W:
      g_value_set_int (value, self->padding_dst_w);
      break;
    case PROP_PADDING_H:
      g_value_set_int (value, self->padding_dst_h);
      break;
    case PROP_PADDING_R:
      g_value_set_int (value, self->padding_r);
      break;
    case PROP_PADDING_G:
      g_value_set_int (value, self->padding_g);
      break;
    case PROP_PADDING_B:
      g_value_set_int (value, self->padding_b);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_bm_vpss_class_init (GstBmVPSSClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GstBaseTransformClass *base_transform_class = GST_BASE_TRANSFORM_CLASS(klass);
  GstVideoFilterClass *video_filter_class = GST_VIDEO_FILTER_CLASS(klass);

  gobject_class->set_property = GST_DEBUG_FUNCPTR(gst_bm_vpss_set_property);
  gobject_class->get_property = GST_DEBUG_FUNCPTR(gst_bm_vpss_get_property);

  base_transform_class->start = GST_DEBUG_FUNCPTR(gst_bm_vpss_start);
  base_transform_class->stop = GST_DEBUG_FUNCPTR(gst_bm_vpss_stop);
  base_transform_class->transform_caps = GST_DEBUG_FUNCPTR(gst_bmvpss_transform_caps);

  video_filter_class->transform_frame = GST_DEBUG_FUNCPTR(gst_bm_vpss_transform_frame);

  g_object_class_install_property (gobject_class, PROP_LEFT,
      g_param_spec_int ("left", "Left","Pixels to crop at left ", 0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_RIGHT,
      g_param_spec_int ("right", "Right","Pixels to crop at right ", 0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_TOP,
      g_param_spec_int ("top", "Top", "Pixels to crop at top ",0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_BOTTOM,
      g_param_spec_int ("bottom", "Bottom","Pixels to crop at bottom ", 0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PADDING_STX,
      g_param_spec_int ("paddingstx", "Padding STX", "Padding start_x", 0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PADDING_STY,
      g_param_spec_int ("paddingsty", "Padding STY", "Padding start_y", 0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PADDING_W,
      g_param_spec_int ("paddingw", "Padding Width", "Padding Width", 0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PADDING_H,
      g_param_spec_int ("paddingh", "Padding Height", "Padding Height", 0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PADDING_R,
      g_param_spec_uint ("paddingR", "Padding R", "R value", 0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PADDING_G,
      g_param_spec_uint ("paddingG", "Padding G", "G value", 0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PADDING_B,
      g_param_spec_uint ("paddingB", "Padding B", "B value", 0, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gst_bm_vpss_subclass_init(gpointer glass, gpointer class_data)
{
  /* subclass initialization logic */
  GstBmVPSSClass *bmvpss_class = GST_BM_VPSS_CLASS(glass);
  GstElementClass *element_class = GST_ELEMENT_CLASS(glass);
  GstBmVPSSClassData *cdata = class_data;

  gst_element_class_add_pad_template(
      element_class, gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
                                          cdata->sink_caps));
  gst_element_class_add_pad_template(
      element_class, gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                                          cdata->src_caps));

  gst_element_class_set_static_metadata(element_class,
                                        "SOPHGO VPSS",
                                        "Filter/Effect/Video",
                                        "SOPHGO Video Process Sub-System",
                                        "Jian Fang <jian.fang@sophgo.com>");

  bmvpss_class->soc_index = cdata->soc_index;

  gst_caps_unref(cdata->sink_caps);
  gst_caps_unref(cdata->src_caps);
  g_free(cdata);
}

static void
gst_bm_vpss_init(GstBmVPSS *self)
{
  /* initialization logic */
  self->allocator = gst_bm_allocator_new();
  self->crop_enable = FALSE;
  self->padding_enable = FALSE;
  self->padding_if_memset = 0;
  GST_DEBUG_OBJECT (self, "bmvpss initial");
}

static void
gst_bm_vpss_subinstance_init(GTypeInstance G_GNUC_UNUSED *instance,
                             gpointer G_GNUC_UNUSED g_class)
{
  // TODO: do init
}

static gboolean
gst_bm_vpss_start (GstBaseTransform * trans)
{
  GstBmVPSS *self = GST_BM_VPSS (trans);

  GST_DEBUG_OBJECT (self, "bmvpss start");

  return TRUE;
}

static gboolean
gst_bm_vpss_stop (GstBaseTransform * trans)
{
  GstBmVPSS *self = GST_BM_VPSS (trans);

  GST_DEBUG_OBJECT (self, "bmvpss stop");

  return TRUE;
}


static GstCaps *
gst_bmvpss_transform_caps(GstBaseTransform *trans,
                          GstPadDirection direction,
                          GstCaps *caps, GstCaps *filter)
{
  (void)caps;
  GstCaps *ret = NULL;
  GstBmVPSS *self = GST_BM_VPSS(trans);

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
gst_bm_vpss_transform_frame (GstVideoFilter * filter, GstVideoFrame * inframe,
    GstVideoFrame * outframe)
{
  GstFlowReturn ret = GST_FLOW_OK;
  bm_status_t bm_ret = 0;
  GstBmVPSS *self = GST_BM_VPSS (filter);

  bm_handle_t bm_handle = NULL;
  bm_dev_request(&bm_handle, 0);

  bm_image *src    = NULL;
  bm_image *dst   = NULL;

  src = (bm_image *) malloc(sizeof(bm_image));
  dst = (bm_image *) malloc(sizeof(bm_image));
  if (src == NULL || dst == NULL)
  {
    GST_ERROR_OBJECT(self, "Failed to allocate memory for bm_image");
    return GST_FLOW_ERROR;
  }

  int in_height = inframe->info.height;
  int in_width = inframe->info.width;
  bm_image_format_ext bmInFormat =
      (bm_image_format_ext)map_gstformat_to_bmformat(inframe->info.finfo->format);

  int out_height = outframe->info.height;
  int out_width = outframe->info.width;
  bm_image_format_ext bmOutFormat =
      (bm_image_format_ext)map_gstformat_to_bmformat(outframe->info.finfo->format);

  GstBuffer *inbuf, *outbuf;
  GstMemory *inmem, *outmem;
  bm_device_mem_t *fb_dma_buffer;

  inbuf = inframe->buffer;
  inmem = gst_buffer_peek_memory(inbuf, 0);
  outbuf = outframe->buffer;
  outmem = gst_buffer_peek_memory(outbuf, 0);

  if (gst_is_dmabuf_memory(outmem)) {
    GST_DEBUG_OBJECT(self, "outmem is dmabuf");
    if (bmOutFormat == FORMAT_RGB_PACKED) {
      bmOutFormat = FORMAT_BGR_PACKED;
    } else if (bmOutFormat == FORMAT_BGR_PACKED) {
      bmOutFormat = FORMAT_RGB_PACKED;
    }
  }

  int in_stride[4] = {0};
  for (guint i = 0; i < GST_VIDEO_FRAME_N_PLANES(inframe); i++) {
    in_stride[i] = GST_VIDEO_FRAME_PLANE_STRIDE(inframe, i);
  }

  bm_image_create(bm_handle, in_height, in_width, bmInFormat, DATA_TYPE_EXT_1N_BYTE, src, in_stride);
  bm_image_create(bm_handle, out_height, out_width, bmOutFormat, DATA_TYPE_EXT_1N_BYTE, dst, NULL);

  if (g_type_is_a(G_OBJECT_TYPE(inmem->allocator), GST_TYPE_BM_ALLOCATOR)) {
    fb_dma_buffer = gst_bm_allocator_get_bm_buffer(inmem);
    unsigned long long base_addr = bm_mem_get_device_addr(*fb_dma_buffer);
    int plane_num = GST_VIDEO_FRAME_N_PLANES(inframe);
    int plane_size[3] = {0};
    unsigned long long input_addr_phy[3] = {0};
    bm_device_mem_t input_addr[3] = {0};

    for (int i = 0; i < plane_num; i++) {
      plane_size[i] = GST_VIDEO_FRAME_COMP_STRIDE(inframe, i) *
                      GST_VIDEO_FRAME_COMP_HEIGHT(inframe, i);
      input_addr_phy[i] = base_addr + GST_VIDEO_FRAME_PLANE_OFFSET(inframe, i);
      input_addr[i] = bm_mem_from_device(input_addr_phy[i], plane_size[i]);
    }
    bm_image_attach(*src, input_addr);
  } else {
    GST_DEBUG_OBJECT(self, "inframe->buffer isn't bm_allocator");
    bm_image_alloc_dev_mem(*src, BMCV_HEAP_ANY);
    guint8 *src_in_ptr[4];
    for (guint i = 0; i < GST_VIDEO_FRAME_N_PLANES(inframe); i++) {
      src_in_ptr[i] = GST_VIDEO_FRAME_PLANE_DATA(inframe, i);
    }
    bm_image_copy_host_to_device(*src, (void **)src_in_ptr);
  }

  if (g_type_is_a(G_OBJECT_TYPE(outmem->allocator), GST_TYPE_BM_ALLOCATOR)) {
    fb_dma_buffer = gst_bm_allocator_get_bm_buffer(outmem);
    unsigned long long base_addr = bm_mem_get_device_addr(*fb_dma_buffer);
    int plane_num = GST_VIDEO_FRAME_N_PLANES(outframe);
    int plane_size[3] = {0};
    unsigned long long output_addr_phy[3] = {0};
    bm_device_mem_t output_addr[3] = {0};

    for (int i = 0; i < plane_num; i++) {
      plane_size[i] = GST_VIDEO_FRAME_COMP_STRIDE(outframe, i) *
                      GST_VIDEO_FRAME_COMP_HEIGHT(outframe, i);
      output_addr_phy[i] = base_addr + GST_VIDEO_FRAME_PLANE_OFFSET(outframe, i);
      output_addr[i] = bm_mem_from_device(output_addr_phy[i], plane_size[i]);
    }
    bm_image_attach(*dst, output_addr);
  } else if (gst_is_dmabuf_memory(outmem)) {
    gint fd = gst_dmabuf_memory_get_fd(outmem);
    GST_DEBUG_OBJECT(self, "outmem fd: %d", fd);
    int plane_num = GST_VIDEO_FRAME_N_PLANES(outframe);
    int plane_size[3] = {0};
    bm_device_mem_t output_addr[3] = {0};

    for (int i = 0; i < plane_num; i++) {
      plane_size[i] = GST_VIDEO_FRAME_COMP_STRIDE(outframe, i) *
                      GST_VIDEO_FRAME_COMP_HEIGHT(outframe, i);
      output_addr[i] = bm_mem_from_device(fd, plane_size[i]);
    }
    bm_image_attach(*dst, output_addr);
  } else {
    GST_DEBUG_OBJECT(self, "outframe->buffer isn't dmabuffer");
    bm_image_alloc_dev_mem(*dst, BMCV_HEAP_ANY);
  }

  bmcv_rect_t crop_rect = {0, 0, inframe->info.width, inframe->info.height};

  if (self->crop_enable) {
    crop_rect.start_x = self->crop_left;
    crop_rect.start_y = self->crop_top;
    crop_rect.crop_w = self->crop_right - self->crop_left;
    crop_rect.crop_h = self->crop_bottom - self->crop_top;
  }
  GST_DEBUG_OBJECT(self, "crop_rect: %d %d %d %d", crop_rect.start_x,
                   crop_rect.start_y, crop_rect.crop_w, crop_rect.crop_h);

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

#if 0
  static gint count = 0;
  guint8 *src_in_ptr[4];
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

  if(self->padding_enable) {
    bmcv_padding_attr_t padding_attr = {
        .dst_crop_stx  = self->padding_dst_stx,
        .dst_crop_sty  = self->padding_dst_sty,
        .dst_crop_w    = self->padding_dst_w,
        .dst_crop_h    = self->padding_dst_h,
        .padding_r     = self->padding_r,
        .padding_g     = self->padding_g,
        .padding_b     = self->padding_b,
        .if_memset     = self->padding_if_memset
    };
    bm_ret = bmcv_image_vpp_convert_padding(bm_handle, 1, *src, dst, &padding_attr, &crop_rect, BMCV_INTER_LINEAR);
  } else {
    bm_ret = bmcv_image_vpp_convert(bm_handle, 1, *src, dst, &crop_rect, BMCV_INTER_LINEAR);
  }
  if(bm_ret != BM_SUCCESS) {
    GST_ERROR_OBJECT(self, "Failed to convert image, error code: %d", bm_ret);
    ret = GST_FLOW_ERROR;
    goto error;
  }

  if (!g_type_is_a(G_OBJECT_TYPE(outmem->allocator), GST_TYPE_BM_ALLOCATOR) &&
      !gst_is_dmabuf_memory(outmem)) {
    GST_DEBUG_OBJECT(self, "outframe is copy");
    guint8 *dst_in_ptr[4];
    for (guint i = 0; i < GST_VIDEO_FRAME_N_PLANES(outframe); i++) {
      dst_in_ptr[i] = GST_VIDEO_FRAME_PLANE_DATA(outframe, i);
    }
    bm_image_copy_device_to_host(*dst, (void **)dst_in_ptr);
  } else {
    GST_DEBUG_OBJECT(self, "outframe is zerocopy");
  }

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

error:
  bm_image_destroy(src);
  bm_image_destroy(dst);
  free(src);
  free(dst);

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
  case GST_VIDEO_FORMAT_NV21:
    format = FORMAT_NV21;
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

// static
// GstVideoFormat map_bmformat_to_gstformat(int bm_format)
// {
//   GstVideoFormat format;
//   switch (bm_format)
//   {
//   case FORMAT_YUV420P:
//     format = GST_VIDEO_FORMAT_I420;
//     break;
//   case FORMAT_YUV422P:
//     format = GST_VIDEO_FORMAT_Y42B;
//     break;
//   case FORMAT_YUV444P:
//     format = GST_VIDEO_FORMAT_Y444;
//     break;
//   case FORMAT_GRAY:
//     format = GST_VIDEO_FORMAT_GRAY8;
//     break;
//   case FORMAT_NV12:
//     format = GST_VIDEO_FORMAT_NV12;
//     break;
//   case FORMAT_NV21:
//     format = GST_VIDEO_FORMAT_NV21;
//     break;
//   case FORMAT_NV16:
//     format = GST_VIDEO_FORMAT_NV16;
//     break;
//   case FORMAT_RGB_PACKED:
//     format = GST_VIDEO_FORMAT_RGB;
//     break;
//   case FORMAT_BGR_PACKED:
//     format = GST_VIDEO_FORMAT_BGR;
//     break;
//   default:
//     GST_ERROR("Unsupported BM format %d", bm_format);
//     format = GST_VIDEO_FORMAT_UNKNOWN;
//     break;
//   }
//   return format;
// }


void gst_bm_vpss_register(GstPlugin *plugin, guint soc_idx, GstCaps *sink_caps,
                          GstCaps *src_caps)
{
  GType type;
  gchar *type_name;
  GstBmVPSSClassData *class_data;
  GTypeInfo type_info = {
    sizeof (GstBmVPSSClass),
    NULL,
    NULL,
    (GClassInitFunc) gst_bm_vpss_subclass_init,
    NULL,
    NULL,
    sizeof (GstBmVPSS),
    0,
    (GInstanceInitFunc) gst_bm_vpss_subinstance_init,
    NULL,
  };

  GST_DEBUG_CATEGORY_INIT (gst_bm_vpss_debug, "bmvpss", 0, "BM VPSS Plugin");

  class_data = g_new0 (GstBmVPSSClassData, 1);
  class_data->sink_caps = gst_caps_ref (sink_caps);
  class_data->src_caps = gst_caps_ref (src_caps);
  class_data->soc_index = soc_idx;

  type_name = g_strdup (GST_BM_VPSS_TYPE_NAME);
  type_info.class_data = class_data;
  type = g_type_register_static(GST_TYPE_BM_VPSS, type_name, &type_info, 0);

  if (!gst_element_register(plugin, type_name, GST_RANK_PRIMARY, type)) {
    GST_WARNING("Failed to register VPSS plugin '%s'", type_name);
  }

  g_free (type_name);
}
