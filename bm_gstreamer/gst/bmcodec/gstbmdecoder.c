#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include "gstbmdecoder.h"
#include <stdio.h>

GST_DEBUG_CATEGORY (gst_bm_decoder_debug);
#define GST_CAT_DEFAULT gst_bm_decoder_debug
#define DEFAULT_PROP_EX_BUF 10
#define DEFAULT_PROP_BS_MODE 1    /* Original */

// static int count = 0;  // for debug

static void gst_bm_decoder_process_output (GstVideoDecoder *decoder);

static void gst_bm_decoder_reset (GstBmDecoder * self, gboolean drain, gboolean final);
static void gst_bm_decoder_dispose (GObject * object);

static gboolean gst_bm_decoder_open (GstVideoDecoder * decoder);
static gboolean gst_bm_decoder_close (GstVideoDecoder * decoder);
static GstFlowReturn gst_bm_decoder_handle_frame (GstVideoDecoder * decoder, GstVideoCodecFrame * frame);
static gboolean gst_bm_decoder_negotiate (GstVideoDecoder * decoder);
static gboolean gst_bm_decoder_decide_allocation (GstVideoDecoder * decoder, GstQuery * query);
static GstFlowReturn gst_bm_decoder_drain (GstVideoDecoder * decoder);
static gboolean gst_bm_decoder_sink_query (GstVideoDecoder * decoder, GstQuery * query);
static gboolean gst_bm_decoder_src_query (GstVideoDecoder * decoder, GstQuery * query);
static gboolean gst_bm_decoder_check_jpg_info_change(GstVideoDecoder *decoder, BmJpuJPEGDecInfo* jpeg_dec_info);
static GQuark gst_bm_jpuframe_quark (void);
static void gst_bm_jpuframe_clear(gpointer ptr);

#define parent_class gst_bm_decoder_parent_class
G_DEFINE_TYPE (GstBmDecoder, gst_bm_decoder, GST_TYPE_VIDEO_DECODER);

 #define GST_BM_DEC_TASK_STARTED(decoder) \
    (gst_pad_get_task_state ((decoder)->srcpad) == GST_TASK_STARTED)

#define GST_BM_DEC_MUTEX(decoder) (&GST_BM_DECODER (decoder)->mutex)

#define GST_BM_DEC_LOCK(decoder) \
  GST_VIDEO_DECODER_STREAM_UNLOCK (decoder); \
  g_mutex_lock (GST_BM_DEC_MUTEX (decoder)); \
  GST_VIDEO_DECODER_STREAM_LOCK (decoder);

#define GST_BM_DEC_UNLOCK(decoder) \
  g_mutex_unlock (GST_BM_DEC_MUTEX (decoder));

struct BmFrameCtx
{
  void *handle;
  BMVidFrame frame;
  gint system_num;
};

struct BmJpuFrameCtx
{
  BmJpuJPEGDecoder *jpeg_decoder;
  BmJpuFramebuffer *framebuffer;
};

enum
{
  PROP_0,
  PROP_EX_BUFNUM,
  PROP_BS_MODE,
  PROP_LAST,
};

static void
gst_bm_dec_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GstVideoDecoder *decoder = GST_VIDEO_DECODER (object);
  GstBmDecoder *self = GST_BM_DECODER (decoder);

  switch (prop_id) {
    case PROP_EX_BUFNUM:{
      if (self->input_state)
        GST_WARNING_OBJECT (decoder, "unable to set extra FrameBufferNum");
      else
        self->extraFrameBufferNum = g_value_get_uint (value);
      break;
    }
    case PROP_BS_MODE :{
      if (self->input_state)
        GST_WARNING_OBJECT (decoder, "unable to change width");
      else
        self->frame_mode = g_value_get_uint (value);
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_bm_dec_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  GstVideoDecoder *decoder = GST_VIDEO_DECODER (object);
  GstBmDecoder *self = GST_BM_DECODER (decoder);

  switch (prop_id) {
    case PROP_EX_BUFNUM:
      g_value_set_uint (value, self->extraFrameBufferNum);
      break;
    case PROP_BS_MODE:
      g_value_set_uint (value, self->frame_mode);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      return;
  }
}

static gboolean gst_bm_decoder_set_format(GstVideoDecoder *decoder, GstVideoCodecState *state)
{
  GstBmDecoder *self = GST_BM_DECODER (decoder);
  GstStructure *structure;
  GstVideoFormat format = GST_VIDEO_FORMAT_I420;
  int bm_ret = 0;

  GST_DEBUG_OBJECT (self, "setting format: %" GST_PTR_FORMAT, state->caps);

  self->codec_id = BmVideoCodecID_NumCodecs;
  structure = gst_caps_get_structure (state->caps, 0);
  if (gst_structure_has_name (structure, "video/x-h264")) {
    self->codec_id = BmVideoCodecID_H264;
  }
  if (gst_structure_has_name (structure, "video/x-h265")) {
    self->codec_id = BmVideoCodecID_HEVC;
  }

  {
    gchar const *format_str = NULL;
    GValue const *format_value = NULL;
    GstCaps *srccaps = NULL;
    srccaps = gst_pad_get_allowed_caps(GST_VIDEO_DECODER_SRC_PAD(decoder));

		if (srccaps != NULL)
		{
			GST_DEBUG_OBJECT(self, "allowed srcccaps: %" GST_PTR_FORMAT, (gpointer)srccaps);
			/* Look at the sample format values from the first structure */
			GstStructure *structure = gst_caps_get_structure(srccaps, 0);
			format_value = gst_structure_get_value(structure, "format");
      if (!format_value) {
        format = GST_VIDEO_FORMAT_I420;
      } else if (GST_VALUE_HOLDS_LIST(format_value)) {
				/* if value is a format list, pick the first entry */
				GValue const *fmt_list_value = gst_value_list_get_value(format_value, 0);
				format_str = g_value_get_string(fmt_list_value);
			}
			else if (G_VALUE_HOLDS_STRING(format_value)) {
				/* if value is a string, use it directly */
				format_str = g_value_get_string(format_value);
			} else {
        format = GST_VIDEO_FORMAT_I420;
			}
      if (format_str)
			  format = gst_video_format_from_string(format_str);

      GST_DEBUG_OBJECT(self,"format = %d format_str %s \n", format, format_str);
      gst_clear_caps (&srccaps);
    }
  }

  if (self->codec_id == BmVideoCodecID_H264 || self->codec_id == BmVideoCodecID_HEVC) {
    BMVidDecParam param;

    memset(&param, 0, sizeof(BMVidDecParam));
    param.streamFormat = -1;
    param.wtlFormat = 0;  // non-compress mode
    param.extraFrameBufferNum = self->extraFrameBufferNum;
    param.streamBufferSize = 0x500000;
    param.enable_cache = 0;
    param.bsMode = 1;  // interrupt mode
    param.core_idx = -1;
    param.frameDelay = 0;
    //param.cmd_queue_depth = 4;

    param.bsMode = self->frame_mode;
    if (self->codec_id == BmVideoCodecID_H264)
      param.streamFormat = 0; //h264

    if (self->codec_id == BmVideoCodecID_HEVC)
      param.streamFormat = 12; //hevc

    switch (format)
    {
    case GST_VIDEO_FORMAT_I420:
      /* code */
      param.pixel_format = BM_VPU_DEC_PIX_FORMAT_YUV420P;
      break;
    case GST_VIDEO_FORMAT_NV12:
      /* code */
      param.pixel_format = BM_VPU_DEC_PIX_FORMAT_NV12;
      break;
    case GST_VIDEO_FORMAT_NV21:
      param.pixel_format = BM_VPU_DEC_PIX_FORMAT_NV21;
      break;
    default:
      GST_ERROR_OBJECT(self, "gst format no support %d \n", format);
      return FALSE;
    }

    if ((bm_ret = bmvpu_dec_create (&self->handle, param)) != 0) {
      GST_ERROR_OBJECT (self, "bmvpu_dec_create failed");
      return FALSE;
    }
  }

  if (gst_structure_has_name (structure, "image/jpeg")) {
    BmJpuDecOpenParams jpeg_param;

    memset(&jpeg_param, 0, sizeof(BmJpuDecOpenParams));
    self->codec_id = BmVideoCodecID_JPEG;
    jpeg_param.bs_buffer_size = 0x500000;
    switch (format)
    {
    case GST_VIDEO_FORMAT_I420:
      /* code */
      jpeg_param.color_format = BM_JPU_COLOR_FORMAT_YUV420;
      jpeg_param.chroma_interleave = 0;
      break;
    case GST_VIDEO_FORMAT_NV12:
      /* code */
      jpeg_param.color_format = BM_JPU_COLOR_FORMAT_YUV420;
      jpeg_param.chroma_interleave = 1;
      break;
    case GST_VIDEO_FORMAT_NV21:
      jpeg_param.color_format = BM_JPU_COLOR_FORMAT_YUV420;
      jpeg_param.chroma_interleave = 2;
      break;
    case GST_VIDEO_FORMAT_NV16:
      jpeg_param.color_format = BM_JPU_COLOR_FORMAT_YUV422_HORIZONTAL;
      jpeg_param.chroma_interleave = 1;
      break;
    case GST_VIDEO_FORMAT_Y42B:
      jpeg_param.color_format = BM_JPU_COLOR_FORMAT_YUV422_HORIZONTAL;
      jpeg_param.chroma_interleave = 0;
      break;
    case GST_VIDEO_FORMAT_Y444:
      jpeg_param.color_format = BM_JPU_COLOR_FORMAT_YUV444;
      jpeg_param.chroma_interleave = 0;
      break;
    default:
      jpeg_param.color_format = BM_JPU_COLOR_FORMAT_BUTT;
      break;
    }

    bm_ret = bm_jpu_jpeg_dec_open(&self->jpeg_decoder, &jpeg_param, 0);

    if (bm_ret != BM_JPU_DEC_RETURN_CODE_OK) {
      GST_ERROR_OBJECT(self, "bm_jpu_jpeg_dec_open failed");
      return FALSE;
    }
  }

  if (self->input_state) {
    if (gst_caps_is_strictly_equal (self->input_state->caps, state->caps))
      return TRUE;
    if(self->codec_id != BmVideoCodecID_JPEG) {
      gst_bm_decoder_reset (self, FALSE, FALSE);
    }
    self->input_state = NULL;
  }
  self->input_state = gst_video_codec_state_ref (state);
	return TRUE;
}

static gboolean
gst_bm_decoder_flush (GstVideoDecoder * decoder)
{
  GstBmDecoder *self = GST_BM_DECODER (decoder);

  GST_DEBUG_OBJECT (decoder, "flushing");
  gst_bm_decoder_reset (self, FALSE, FALSE);
  return TRUE;
}

static GstFlowReturn
gst_bm_decoder_finish (GstVideoDecoder * decoder)
{
  GstBmDecoder *self = GST_BM_DECODER (decoder);

  GST_DEBUG_OBJECT (decoder, "finishing");
  gst_bm_decoder_reset (self, TRUE, FALSE);
  return GST_FLOW_OK;
}

static void gst_bm_decoder_class_init (GstBmDecoderClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstVideoDecoderClass *video_decoder_class = GST_VIDEO_DECODER_CLASS (klass);

  gobject_class->dispose = GST_DEBUG_FUNCPTR (gst_bm_decoder_dispose);

  video_decoder_class->open = GST_DEBUG_FUNCPTR (gst_bm_decoder_open);
  video_decoder_class->close = GST_DEBUG_FUNCPTR (gst_bm_decoder_close);
  video_decoder_class->handle_frame = GST_DEBUG_FUNCPTR (gst_bm_decoder_handle_frame);
  video_decoder_class->negotiate = GST_DEBUG_FUNCPTR (gst_bm_decoder_negotiate);
  video_decoder_class->decide_allocation = GST_DEBUG_FUNCPTR (gst_bm_decoder_decide_allocation);
  video_decoder_class->drain = GST_DEBUG_FUNCPTR (gst_bm_decoder_drain);
  video_decoder_class->flush = GST_DEBUG_FUNCPTR (gst_bm_decoder_flush);
  video_decoder_class->finish = GST_DEBUG_FUNCPTR (gst_bm_decoder_finish);
  video_decoder_class->sink_query = GST_DEBUG_FUNCPTR (gst_bm_decoder_sink_query);
  video_decoder_class->src_query = GST_DEBUG_FUNCPTR (gst_bm_decoder_src_query);
  video_decoder_class->set_format = GST_DEBUG_FUNCPTR (gst_bm_decoder_set_format);
  gobject_class->set_property = GST_DEBUG_FUNCPTR (gst_bm_dec_set_property);
  gobject_class->get_property = GST_DEBUG_FUNCPTR (gst_bm_dec_get_property);

  g_object_class_install_property (gobject_class, PROP_EX_BUFNUM,
      g_param_spec_uint ("ex-buf-num", "extra buffernum",
          "h26x extra buffer for decode (10 = original)",
          0, G_MAXINT, DEFAULT_PROP_EX_BUF,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_BS_MODE,
      g_param_spec_uint ("bs-mode", "bs mode",
          "h26x bs mode (1 = original)",
          0, G_MAXINT, DEFAULT_PROP_BS_MODE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void gst_bm_decoder_subclass_init (gpointer g_class, gpointer data)
{
  GstBmDecoderClass *bmdec_class = GST_BM_DECODER_CLASS (g_class);
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);
  GstBmDecoderClassData *cdata = data;

  gst_element_class_add_pad_template (element_class,
      gst_pad_template_new ("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
      cdata->sink_caps));
  gst_element_class_add_pad_template (element_class,
      gst_pad_template_new ("src", GST_PAD_SRC, GST_PAD_ALWAYS,
      cdata->src_caps));

  gst_element_class_set_static_metadata (element_class,
      "SOPHGO Video Decoder",
      "Codec/Decoder/Video/Hardware",
      "SOPHGO Video Decoder",
      "Xin Guo <xin.guo@sophgo.com>");

  bmdec_class->soc_index = cdata->soc_index;

  gst_caps_unref (cdata->sink_caps);
  gst_caps_unref (cdata->src_caps);
  g_free (cdata);
}

static void gst_bm_decoder_init (GstBmDecoder * self)
{
  self->allocator = gst_bm_allocator_new ();
  self->extraFrameBufferNum =  DEFAULT_PROP_EX_BUF;
  self->frame_mode = DEFAULT_PROP_BS_MODE;
}

static void gst_bm_decoder_subinstance_init (GTypeInstance G_GNUC_UNUSED * instance, gpointer G_GNUC_UNUSED g_class)
{
  // TODO: do init
}

static void gst_bm_decoder_dispose (GObject * object)
{
  GstBmDecoder *self = GST_BM_DECODER (object);
  gst_bm_decoder_reset (self, TRUE, FALSE);
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static gboolean gst_bm_decoder_open (GstVideoDecoder * decoder)
{
  GstBmDecoder *self = GST_BM_DECODER (decoder);

  GST_DEBUG_OBJECT (self, "opening decoder");
  gst_video_info_init (&self->info);
  g_mutex_init (&self->mutex);

  return TRUE;
}

static gboolean gst_bm_decoder_close (GstVideoDecoder * decoder)
{
  GstBmDecoder *self = GST_BM_DECODER (decoder);

  GST_DEBUG_OBJECT (self, "closing decoder");

  if (self->codec_id == BmVideoCodecID_H264 || self->codec_id == BmVideoCodecID_HEVC) {
      if (self->handle) {
        bmvpu_dec_flush (self->handle);
        while (bmvpu_dec_get_status (self->handle) != BMDEC_STOP) {
          GST_DEBUG_OBJECT (self, "bmvpu get status loop");
          g_usleep (2000);
        }
        bmvpu_dec_delete (self->handle);
        self->handle = NULL;
      }
  }

  if (self->codec_id == BmVideoCodecID_JPEG) {
    if (self->jpeg_decoder != NULL) {
      bm_jpu_jpeg_dec_close(self->jpeg_decoder);
      self->jpeg_decoder = NULL;
    }
  }

  if (self->input_state) {
    gst_video_codec_state_unref (self->input_state);
    self->input_state = NULL;
  }

  if (self->allocator) {
    gst_object_unref (self->allocator);
    self->allocator = NULL;
  }

   g_mutex_clear (&self->mutex);

  return TRUE;
}

static GstFlowReturn gst_bm_decoder_jpg_handle_frame(GstVideoDecoder * decoder, GstVideoCodecFrame * frame)
{
  GstBmDecoder *self = GST_BM_DECODER (decoder);
  GstBuffer *in_buffer = NULL;
  GstMapInfo map_info = GST_MAP_INFO_INIT;
  BmJpuJPEGDecInfo jpeg_dec_info;
  unsigned long long phys_addr = 0;
  GstVideoInfo *info = &self->info;
  GstMemory *mem_import = NULL;
  GstBuffer *buffer = NULL;
  GQuark quark;
  int buf_size = 0;

  struct BmJpuFrameCtx *pJpuFrameCtx = NULL;

  in_buffer = gst_buffer_ref (frame->input_buffer);
  if (!gst_buffer_map (in_buffer, &map_info, GST_MAP_READ)) {
    GST_ERROR_OBJECT (self, "failed to map input buffer");
    gst_buffer_unref (in_buffer);
    gst_video_codec_frame_unref (frame);
    return GST_FLOW_ERROR;
  }
  bm_jpu_jpeg_dec_decode(self->jpeg_decoder, map_info.data, map_info.size, 0, 0);
  bm_jpu_jpeg_dec_get_info(self->jpeg_decoder, &jpeg_dec_info);
  if (!gst_bm_decoder_check_jpg_info_change(decoder, &jpeg_dec_info)) {
    goto error;
  }

  GST_DEBUG_OBJECT (self, "aligned frame size: %u x %u\n"
         "pixel actual frame size: %u x %u\n"
         "pixel Y/Cb/Cr stride: %u/%u/%u\n"
         "pixel Y/Cb/Cr size: %u/%u/%u\n"
         "pixel Y/Cb/Cr offset: %u/%u/%u\n"
         "image format: %d\n",
         jpeg_dec_info.aligned_frame_width, jpeg_dec_info.aligned_frame_height,
         jpeg_dec_info.actual_frame_width,  jpeg_dec_info.actual_frame_height,
         jpeg_dec_info.y_stride,            jpeg_dec_info.cbcr_stride,             jpeg_dec_info.cbcr_stride,
         jpeg_dec_info.y_size,              jpeg_dec_info.cbcr_size,               jpeg_dec_info.cbcr_size,
         jpeg_dec_info.y_offset,            jpeg_dec_info.cb_offset,               jpeg_dec_info.cr_offset,
         jpeg_dec_info.image_format);

  phys_addr = bm_mem_get_device_addr(*(jpeg_dec_info.framebuffer->dma_buffer));
  buf_size = jpeg_dec_info.framebuffer->dma_buffer->size > GST_VIDEO_INFO_SIZE (info) ?
             GST_VIDEO_INFO_SIZE (info) : jpeg_dec_info.framebuffer->dma_buffer->size;

  mem_import = gst_bm_allocator_import_bmjpg_buf(self->allocator,
                                              phys_addr,
                                              buf_size);



  if (!mem_import) {
    GST_ERROR_OBJECT (self, "import jpeg_dec yuv buffer failed");
    goto error;
  }

  buffer = gst_buffer_new ();
  if (!buffer) {
    GST_ERROR_OBJECT (self, "create gst buffer failed");
    // goto error;
  }
  gst_buffer_append_memory (buffer, mem_import);
  gst_buffer_add_video_meta_full (buffer, GST_VIDEO_FRAME_FLAG_NONE,
                                  GST_VIDEO_INFO_FORMAT (info),
                                  GST_VIDEO_INFO_WIDTH (info),
                                  GST_VIDEO_INFO_HEIGHT (info),
                                  GST_VIDEO_INFO_N_PLANES (info),
                                   info->offset, info->stride);

  frame->output_buffer = buffer;

  pJpuFrameCtx = (struct BmJpuFrameCtx *) malloc (sizeof (struct BmJpuFrameCtx));
  if (!pJpuFrameCtx) {
    GST_ERROR_OBJECT (self, "jpu frame ctx malloc fail \n");
    goto error;
  }

  pJpuFrameCtx->jpeg_decoder = self->jpeg_decoder;
  pJpuFrameCtx->framebuffer  = jpeg_dec_info.framebuffer;
  quark = gst_bm_jpuframe_quark();

  gst_mini_object_set_qdata (GST_MINI_OBJECT(mem_import), quark, pJpuFrameCtx,
                            gst_bm_jpuframe_clear);

  gst_video_decoder_finish_frame(decoder, frame);
  if (in_buffer) {
    gst_buffer_unmap (in_buffer, &map_info);
    gst_buffer_unref (in_buffer);
  }

  return GST_FLOW_OK;

error:
  if (buffer) {
    gst_buffer_unref (buffer);
    buffer = NULL;
  }

  if (mem_import) {
    gst_memory_unref (mem_import);
    mem_import = NULL;
  }

  if (pJpuFrameCtx) {
    bm_jpu_jpeg_dec_frame_finished(self->jpeg_decoder, jpeg_dec_info.framebuffer);
    free (pJpuFrameCtx);
    pJpuFrameCtx = NULL;
  }

  gst_buffer_unmap (in_buffer, &map_info);
  gst_buffer_unref (in_buffer);
  gst_video_codec_frame_unref (frame);

  return GST_FLOW_ERROR;

}

static GstFlowReturn gst_bm_decoder_handle_frame (GstVideoDecoder * decoder, GstVideoCodecFrame * frame)
{
  GstFlowReturn ret = GST_FLOW_OK;
  GstBmDecoder *self = GST_BM_DECODER (decoder);
  GstBuffer *in_buffer = NULL;
  GstMapInfo map_info = GST_MAP_INFO_INIT;
  BMVidStream vid_stream;
  int bm_ret = 0;

  if (self->codec_id == BmVideoCodecID_JPEG) {
    gst_bm_decoder_jpg_handle_frame(decoder, frame);
    return GST_FLOW_OK;
  }

  GST_DEBUG_OBJECT (self, "Handling frame %d", frame->system_frame_number);

  // TODO: deal with the frame in order of system_frame_number
  //gst_video_codec_frame_set_user_data (frame, (gpointer) decoder, NULL);

  GST_BM_DEC_LOCK (decoder);
  if (G_UNLIKELY (self->flushing))
    goto flushing;

  if (G_UNLIKELY (!GST_BM_DEC_TASK_STARTED (decoder))) {
    GST_DEBUG_OBJECT (self, "starting dec thread");
    //GstVideoCodecState *state = self->input_state;
    gst_pad_start_task (decoder->srcpad,
                      (GstTaskFunction) gst_bm_decoder_process_output,
                      decoder, NULL);
  }

  in_buffer = gst_buffer_ref (frame->input_buffer);
  if (!gst_buffer_map (in_buffer, &map_info, GST_MAP_READ)) {
    GST_ERROR_OBJECT (self, "failed to map input buffer");
    gst_buffer_unref (in_buffer);
    gst_video_codec_frame_unref (frame);
    GST_BM_DEC_UNLOCK (decoder);
    return GST_FLOW_ERROR;
  }

  memset (&vid_stream, 0, sizeof (BMVidStream));
  vid_stream.buf = map_info.data;
  vid_stream.length = map_info.size;
  vid_stream.pts = frame->pts;
  vid_stream.dts = frame->system_frame_number;
  vid_stream.end_of_stream = 0;

 /*for (size_t i = 0; i < 10; i++)
  {
     g_print("nal[%d] = %x ",i, map_info.data[i]);
  }
  g_print("\n");*/

  GST_VIDEO_DECODER_STREAM_UNLOCK (decoder);
  if (self->handle != NULL) {
    GST_DEBUG_OBJECT (self, "send stream in %d \n", frame->system_frame_number);
    gint time_out_cnt = 1000;
    while ((bm_ret = bmvpu_dec_decode(self->handle, vid_stream)) != 0) {
      g_usleep (1000);
      if (time_out_cnt-- < 0) {
          g_print("send frame fail \n");
          GST_VIDEO_DECODER_STREAM_LOCK (decoder);
          goto error;
      }
    }
  } else {
    GST_ERROR_OBJECT (self, "bmvpu handle is invalid");
    ret = GST_FLOW_ERROR;
    GST_VIDEO_DECODER_STREAM_LOCK (decoder);
    goto error;
  }
  GST_VIDEO_DECODER_STREAM_LOCK (decoder);

  self->pending_frames++;
  if (!self->frame_mode) {
    self->frames = g_list_append (self->frames,
                  GUINT_TO_POINTER (frame->system_frame_number));
  }

  if (in_buffer) {
    gst_buffer_unmap (in_buffer, &map_info);
    gst_buffer_unref (in_buffer);
  }

  gst_video_codec_frame_unref (frame);
  GST_BM_DEC_UNLOCK (decoder);
  return ret;
flushing:
  GST_WARNING_OBJECT (self, "flushing");
  ret = GST_FLOW_FLUSHING;
error:
  GST_WARNING_OBJECT (self, "can't handle this frame");

  if (in_buffer) {
    gst_buffer_unmap (in_buffer, &map_info);
    gst_buffer_unref (in_buffer);
  }
  gst_video_decoder_release_frame (decoder, frame);
  GST_BM_DEC_UNLOCK (decoder);
  return ret;
}

static gboolean gst_bm_decoder_negotiate (GstVideoDecoder * decoder)
{
  GstBmDecoder *self = GST_BM_DECODER (decoder);
  //GstVideoCodecState *state;

  GST_DEBUG_OBJECT (self, "negotiate");

  return GST_VIDEO_DECODER_CLASS (parent_class)->negotiate (decoder);
}

static gboolean gst_bm_decoder_decide_allocation (GstVideoDecoder * decoder, GstQuery * query)
{
  GstBmDecoder *self = GST_BM_DECODER (decoder);

  GST_DEBUG_OBJECT (self, "decide allocation");

  if (self->output_type == GST_BM_DECODER_OUTPUT_TYPE_DEVICE) {
    // TODO: make sure bufferpool
  }

  return GST_VIDEO_DECODER_CLASS (parent_class)->decide_allocation (decoder, query);
}

static void gst_bm_decoder_stop_task(GstVideoDecoder *decoder, gboolean drain)
{
  GstBmDecoder *self = GST_BM_DECODER (decoder);
  GstTask *task = decoder->srcpad->task;

  if (!GST_BM_DEC_TASK_STARTED (decoder))
    return;

  GST_DEBUG_OBJECT (self, "stopping decoder thread");

  /* Discard pending frames */
  if (!drain)
    self->pending_frames = 0;

//  GST_BM_DEC_BROADCAST (decoder);

  GST_VIDEO_DECODER_STREAM_UNLOCK (decoder);
  /* Wait for task thread to pause */
  if (task) {
    GST_OBJECT_LOCK (task);
    while (GST_TASK_STATE (task) == GST_TASK_STARTED)
      GST_TASK_WAIT (task);
    GST_OBJECT_UNLOCK (task);
  }
  gst_pad_stop_task (decoder->srcpad);
  GST_VIDEO_DECODER_STREAM_LOCK (decoder);
}


static GstFlowReturn gst_bm_decoder_drain (GstVideoDecoder * decoder)
{
  GstBmDecoder *self = GST_BM_DECODER (decoder);

  GST_DEBUG_OBJECT (self, "draining decoder");

  if (self->handle != NULL) {
    bmvpu_dec_get_all_frame_in_buffer(self->handle);
  }

  return GST_FLOW_OK;
}

static gboolean gst_bm_decoder_sink_query (GstVideoDecoder * decoder, GstQuery * query)
{
  GstBmDecoder *self = GST_BM_DECODER (decoder);

  GST_DEBUG_OBJECT (self, "sink query");

  return GST_VIDEO_DECODER_CLASS (parent_class)->sink_query (decoder, query);
}

static gboolean gst_bm_decoder_src_query (GstVideoDecoder * decoder, GstQuery * query)
{
  GstBmDecoder *self = GST_BM_DECODER (decoder);

  GST_DEBUG_OBJECT (self, "src query");

  return GST_VIDEO_DECODER_CLASS (parent_class)->src_query (decoder, query);
}

GstBmDecoder * gst_bm_decoder_new (GObject G_GNUC_UNUSED * object)
{
  GstBmDecoder *self;

  self = g_object_new (GST_TYPE_BM_DECODER, NULL);
  gst_object_ref_sink (self);

  return self;
}

gboolean gst_bm_decoder_is_configured (GstBmDecoder * decoder)
{
  g_return_val_if_fail (GST_IS_BM_DECODER (decoder), FALSE);

  return decoder->configured;
}

gboolean gst_bm_decoder_configure (GstBmDecoder * decoder, GstVideoInfo * info,
    gint coded_width, gint coded_height, guint coded_bitdepth)
{
  //GstVideoFormat format;
  gboolean ret = TRUE;

  g_return_val_if_fail (GST_IS_BM_DECODER (decoder), FALSE);
  g_return_val_if_fail (info != NULL, FALSE);
  g_return_val_if_fail (coded_width >= GST_VIDEO_INFO_WIDTH (info), FALSE);
  g_return_val_if_fail (coded_height >= GST_VIDEO_INFO_HEIGHT (info), FALSE);
  g_return_val_if_fail (coded_bitdepth >= 8, FALSE);

  gst_bm_decoder_reset (decoder, TRUE, FALSE);

  decoder->info = *info;
  gst_video_info_set_format (&decoder->coded_info, GST_VIDEO_INFO_FORMAT (info),
      coded_width, coded_height);

 // format = GST_VIDEO_INFO_FORMAT (info);

  decoder->configured = TRUE;

  return ret;
}

static void gst_bm_decoder_reset (GstBmDecoder * self, gboolean drain, gboolean final)
{
  // TODO: handle reset?
  GstVideoDecoder *decoder = &self->parent;

  GST_BM_DEC_LOCK (decoder);
  self->flushing = TRUE;
  self->draining = drain;
  gst_bm_decoder_stop_task(&self->parent, drain);
  self->flushing = final;
  self->draining = FALSE;
  self->output_type = GST_BM_DECODER_OUTPUT_TYPE_SYSTEM;
  self->configured = FALSE;
  if (self->frames) {
    g_list_free (self->frames);
    self->frames = NULL;
  }

  GST_BM_DEC_UNLOCK (decoder);
}

static GQuark
gst_bm_vidframe_quark (void)
{
	static GQuark quark = 0;
	if (quark == 0)
	  quark = g_quark_from_string ("bm-vidframe");
	return quark;
}

static void gst_bm_vidframe_clear(gpointer ptr) {

  struct BmFrameCtx *ctx = (struct BmFrameCtx *)ptr;

  GST_DEBUG_OBJECT(NULL, "vidframe_clear in system_num %d \n", ctx->system_num);
  bmvpu_dec_clear_output (ctx->handle, &ctx->frame);
  free(ctx);
}

static GstVideoFormat bm_format_to_gst_format(BMVidFrame* bmframe)
{
    GstVideoFormat gst_format;

    GST_DEBUG_OBJECT(NULL, "bmframe->pixel_format = %d bmframe->pixel_format %d \n",
            bmframe->pixel_format, bmframe->pixel_format);

    if (bmframe->pixel_format == BM_VPU_DEC_PIX_FORMAT_NV12)
        gst_format = GST_VIDEO_FORMAT_NV12;
    else if (bmframe->pixel_format == BM_VPU_DEC_PIX_FORMAT_NV21)
        gst_format = GST_VIDEO_FORMAT_NV21;
    else if (bmframe->pixel_format == BM_VPU_DEC_PIX_FORMAT_COMPRESSED)
        gst_format = GST_VIDEO_FORMAT_NV12;
    else
        gst_format = GST_VIDEO_FORMAT_I420;

    GST_DEBUG_OBJECT(NULL, "gst_format = %d \n", gst_format);
    return gst_format;
}
static gboolean gst_bm_decoder_check_video_info_change(GstVideoDecoder *decoder, BMVidFrame *pframe)
{
  GstBmDecoder *self = GST_BM_DECODER (decoder);
  GstVideoInfo *info = &self->info;
  guint i = 0;
  gint stride, hstride;

  if (self->width != pframe->width || self->height != pframe->height ||
    self->coded_width != pframe->coded_width ||
    self->coded_height != pframe->coded_height) {
    GstVideoCodecState *output_state;
    GstVideoFormat format = bm_format_to_gst_format(pframe);
    g_return_val_if_fail (format != GST_VIDEO_FORMAT_UNKNOWN, FALSE);
    self->width = pframe->width;
    self->height = pframe->height;
    self->coded_width = pframe->coded_width;
    self->coded_height = pframe->coded_height;
    if (GST_VIDEO_INFO_FORMAT (info) == GST_VIDEO_FORMAT_UNKNOWN) {
      gst_video_info_set_format (info, format, pframe->width, pframe->height);
    }

    hstride = pframe->stride[0];
    stride = GST_VIDEO_INFO_PLANE_STRIDE (info, 0);
    for (i = 0; i < GST_VIDEO_INFO_N_PLANES (info); i++) {
      GST_VIDEO_INFO_PLANE_STRIDE (info, i) = pframe->stride[i];
      if (i == 0) {
         GST_VIDEO_INFO_PLANE_OFFSET (info, i) = 0;
      } else {
         GST_VIDEO_INFO_PLANE_OFFSET (info, i) = pframe->buf[i] - pframe->buf[0];
      }

      GST_DEBUG("plane %d, stride %d, offset %" G_GSIZE_FORMAT, i,
          GST_VIDEO_INFO_PLANE_STRIDE (info, i),
          GST_VIDEO_INFO_PLANE_OFFSET (info, i));
    }

    GST_VIDEO_INFO_SIZE (info) = GST_VIDEO_INFO_SIZE (info) / stride * hstride;

    GST_DEBUG_OBJECT (self, "stride %d self->coded_width %d self->coded_height %d \n",
           GST_VIDEO_INFO_PLANE_STRIDE (info, 0),  self->coded_width, self->coded_height);

    output_state = gst_video_decoder_set_output_state (decoder, format,
          GST_ROUND_UP_2 (pframe->width), GST_ROUND_UP_2 (pframe->height), self->input_state);
    output_state->caps = gst_video_info_to_caps (&output_state->info);
    gst_video_codec_state_unref (output_state);

    if (!gst_video_decoder_negotiate (decoder))
      return FALSE;
  }
 return TRUE;
}

static GstVideoFormat bm_jpeg_format_to_gst_format(BmJpuImageFormat jpg_image_format)
{
  GstVideoFormat gst_format;
  switch (jpg_image_format)
  {
    case BM_JPU_IMAGE_FORMAT_YUV420P:
        gst_format = GST_VIDEO_FORMAT_I420;
        break;
    case BM_JPU_IMAGE_FORMAT_NV12:
        gst_format = GST_VIDEO_FORMAT_NV12;
        break;
    case BM_JPU_IMAGE_FORMAT_NV21:
        gst_format = GST_VIDEO_FORMAT_NV21;
        break;
    case BM_JPU_IMAGE_FORMAT_YUV422P:
        gst_format = GST_VIDEO_FORMAT_Y42B;
        break;
    case BM_JPU_IMAGE_FORMAT_NV16:
        gst_format = GST_VIDEO_FORMAT_NV16;
        break;
    case BM_JPU_IMAGE_FORMAT_NV61:
        gst_format = GST_VIDEO_FORMAT_NV61;
        break;
    case BM_JPU_IMAGE_FORMAT_YUV444P:
        gst_format = GST_VIDEO_FORMAT_Y444;
        break;
    case BM_JPU_IMAGE_FORMAT_GRAY:
        gst_format = GST_VIDEO_FORMAT_GRAY8;
        break;
    default:
        gst_format = GST_VIDEO_FORMAT_UNKNOWN;
        break;
  }
  return gst_format;
}

static GQuark
gst_bm_jpuframe_quark (void)
{
	static GQuark quark = 0;
	if (quark == 0)
	  quark = g_quark_from_string ("bm-jpuframe");
	return quark;
}

static void gst_bm_jpuframe_clear(gpointer ptr)
{

  struct BmJpuFrameCtx *ctx = (struct BmJpuFrameCtx *)ptr;

  bm_jpu_jpeg_dec_frame_finished (ctx->jpeg_decoder, ctx->framebuffer);
  free(ctx);
}

static gboolean gst_bm_decoder_check_jpg_info_change(GstVideoDecoder *decoder, BmJpuJPEGDecInfo* jpeg_dec_info)
{
  GstBmDecoder *self = GST_BM_DECODER (decoder);
  GstVideoInfo *info = &self->info;
  GstVideoAlignment align;
  gint hstride, stride;
  guint i;
  gint jpeg_stride[3] = {jpeg_dec_info->y_stride, jpeg_dec_info->cbcr_stride, jpeg_dec_info->cbcr_stride};
  gint offset_info[3] = {jpeg_dec_info->y_offset,  jpeg_dec_info->cb_offset, jpeg_dec_info->cr_offset};

  if (self->width        != jpeg_dec_info->actual_frame_width  ||
      self->height       != jpeg_dec_info->actual_frame_height ||
      self->coded_width  != jpeg_dec_info->aligned_frame_width ||
      self->coded_height != jpeg_dec_info->aligned_frame_height) {
    GstVideoCodecState *output_state;
    GstVideoFormat format = bm_jpeg_format_to_gst_format(jpeg_dec_info->image_format);
    g_return_val_if_fail (format != GST_VIDEO_FORMAT_UNKNOWN, FALSE);
    self->width        = jpeg_dec_info->actual_frame_width;
    self->height       = jpeg_dec_info->actual_frame_height;
    self->coded_width  = jpeg_dec_info->aligned_frame_width;
    self->coded_height = jpeg_dec_info->aligned_frame_height;
    if (GST_VIDEO_INFO_FORMAT (info) == GST_VIDEO_FORMAT_UNKNOWN) {
      gst_video_info_set_format (info, format, jpeg_dec_info->actual_frame_width, jpeg_dec_info->actual_frame_height);
    }

    gst_video_alignment_reset (&align);
    hstride = jpeg_dec_info->y_stride;
    align.padding_bottom = jpeg_dec_info->aligned_frame_height - GST_VIDEO_INFO_HEIGHT (info);

    if (!gst_video_info_align (info, &align)) {
      return false;
    }
    stride = GST_VIDEO_INFO_PLANE_STRIDE (info, 0);
    for (i = 0; i < GST_VIDEO_INFO_N_PLANES (info); i++) {
      GST_VIDEO_INFO_PLANE_STRIDE (info, i) = jpeg_stride[i];
      if (i == 0) {
         GST_VIDEO_INFO_PLANE_OFFSET (info, i) = 0;
      } else {
         GST_VIDEO_INFO_PLANE_OFFSET (info, i) = offset_info[i];
      }
      GST_DEBUG("plane %d, stride %d, offset %" G_GSIZE_FORMAT, i,
          GST_VIDEO_INFO_PLANE_STRIDE (info, i),
          GST_VIDEO_INFO_PLANE_OFFSET (info, i));
    }

    GST_VIDEO_INFO_SIZE (info) = GST_VIDEO_INFO_SIZE (info) / stride * hstride;
    GST_DEBUG_OBJECT (self, "stride %d self->coded_width %d self->coded_height %d \n",
            GST_VIDEO_INFO_PLANE_STRIDE (info, 0),  self->coded_width, self->coded_height);

    output_state = gst_video_decoder_set_output_state (decoder, format,
                                    GST_ROUND_UP_2 (jpeg_dec_info->actual_frame_width),
                                    GST_ROUND_UP_2 (jpeg_dec_info->actual_frame_height),
                                    self->input_state);

    output_state->caps = gst_video_info_to_caps (&output_state->info);
    gst_video_codec_state_unref (output_state);

    if (!gst_video_decoder_negotiate (decoder))
      return FALSE;
  }
 return TRUE;
}

static void gst_bm_decoder_process_output (GstVideoDecoder *decoder)
{
  GstBmDecoder *self = GST_BM_DECODER (decoder);
  GstVideoInfo *info = &self->info;
  struct BmFrameCtx *pframeCtx = NULL;
  BMVidFrame *pframe = NULL;
  int bm_ret = 0;
  GstMemory *mem_import = NULL;
  gint system_frame_number;
  GstVideoCodecFrame *out_frame;
  GstBuffer *buffer = NULL;
  GQuark quark;
  self->task_ret = GST_FLOW_OK;

  if (self->codec_id == BmVideoCodecID_JPEG)
    return;

  if (!self->allocator) {
    GST_ERROR_OBJECT (self, "no valid allocator \n");
    goto error;
  }

  GST_VIDEO_DECODER_STREAM_LOCK (decoder);
  if (self->flushing && !self->eos_send) {
      BMVidStream vid_stream;
      memset (&vid_stream, 0, sizeof (BMVidStream));
      vid_stream.end_of_stream = 1;
      if (bmvpu_dec_decode(self->handle, vid_stream) == 0) {
        g_print ("dec send eos ok \n");
        self->eos_send = TRUE;
      }
  }

  if (self->flushing && !self->pending_frames) {
    self->task_ret = GST_FLOW_FLUSHING;
    goto error;
  }

  if (!self->pending_frames) {
    goto error;
  }
  pframeCtx = (struct BmFrameCtx *) malloc (sizeof (struct BmFrameCtx));
  if (!pframeCtx) {
    GST_ERROR_OBJECT (self, "frame ctx malloc fail \n");
    goto error;
  }
  pframe = &pframeCtx->frame;

  if ((bm_ret = bmvpu_dec_get_output (self->handle, pframe)) != 0) {
    GST_DEBUG_OBJECT (self, "bmvpu_dec_get_output failed \n");
    free (pframeCtx);
    pframeCtx = NULL;
    goto error;
  }
  if (!gst_bm_decoder_check_video_info_change(decoder,pframe)) {
    goto error;
  }
  if (!self->frame_mode) {
    if (self->frames){
      system_frame_number = GPOINTER_TO_UINT (g_list_nth_data (self->frames, 0));
    } else {
      goto error;
    }
  } else {
    system_frame_number = pframe->dts;
  }

  GST_DEBUG_OBJECT (self, "system_frame_number = %d frameIdx %d\n",
                    system_frame_number, pframe->frameIdx);

	out_frame = gst_video_decoder_get_frame(decoder, system_frame_number);
  if (0)
  {
    char yuv_filename[256] = {0};
    int y_size = pframe->stride[0] * pframe->height;
    FILE *fp = NULL;

    sprintf(yuv_filename, "dump_%d.yuv", system_frame_number);
    fp = fopen (yuv_filename, "wb+");
    if (fp != NULL) {
      fwrite (pframe->buf[0], y_size, 1, fp);
      fwrite (pframe->buf[1], y_size / 2, 1, fp);
      fclose (fp);
    } else {
      GST_DEBUG_OBJECT (self, "can't open file %s", yuv_filename);
    }
  }

  if (G_UNLIKELY(out_frame == NULL))
  {
    GST_WARNING_OBJECT(self, "no gstframe exists with number #%" G_GUINT32_FORMAT " - discarding decoded frame",
                       system_frame_number);
    goto error;
  }

  mem_import = gst_bm_allocator_import_bmvidbuf (self->allocator, pframe);
  if (!mem_import) {
    GST_ERROR_OBJECT (self, "import bmvid buffer failed");
    goto error;
  }

  buffer = gst_buffer_new ();
  if (!buffer) {
    GST_ERROR_OBJECT (self, "create gst buffer failed");
    goto error;
  }

  gst_buffer_append_memory (buffer, mem_import);
  // TODO: add metadata

  gst_buffer_add_video_meta_full (buffer, GST_VIDEO_FRAME_FLAG_NONE,
                                  GST_VIDEO_INFO_FORMAT (info),
                                  GST_VIDEO_INFO_WIDTH (info),
                                  GST_VIDEO_INFO_HEIGHT (info),
                                  GST_VIDEO_INFO_N_PLANES (info),
                                   info->offset, info->stride);

  GST_DEBUG_OBJECT (self, "finish frame pts=%" GST_TIME_FORMAT,
                   GST_TIME_ARGS (out_frame->pts));

  if (pframe->interlacedFrame) {
    GST_BUFFER_FLAG_SET (buffer, GST_VIDEO_BUFFER_FLAG_INTERLACED);
    GST_BUFFER_FLAG_UNSET (buffer, GST_VIDEO_BUFFER_FLAG_TFF); //BOT_FIRST
   // GST_BUFFER_FLAG_SET (buffer, GST_VIDEO_BUFFER_FLAG_TFF);
  }
  out_frame->output_buffer = buffer;
  quark = gst_bm_vidframe_quark();
  pframeCtx->handle = self->handle;
  pframeCtx->system_num = system_frame_number;
  gst_mini_object_set_qdata (GST_MINI_OBJECT(mem_import), quark, pframeCtx,
		gst_bm_vidframe_clear);

  self->task_ret = gst_video_decoder_finish_frame (decoder, out_frame);

  if (!self->frame_mode)
    self->frames = g_list_delete_link (self->frames, self->frames);

  self->pending_frames--;
  GST_VIDEO_DECODER_STREAM_UNLOCK (decoder);

  goto finish;

error:
  if (buffer) {
    gst_buffer_unref (buffer);
    buffer = NULL;
  }

  if (mem_import) {
    gst_memory_unref (mem_import);
    mem_import = NULL;
  }

  if (pframeCtx) {
    bmvpu_dec_clear_output (self->handle, &pframeCtx->frame);
    free (pframeCtx);
    pframeCtx = NULL;
  }

  if (self->task_ret != GST_FLOW_OK) {
    GST_DEBUG_OBJECT (self, "leaving output thread: %s",
        gst_flow_get_name (self->task_ret));

    gst_pad_pause_task (decoder->srcpad);
  }
  GST_VIDEO_DECODER_STREAM_UNLOCK (decoder);
  g_usleep (1000);
finish:
  return;
}
#if 0
static gboolean gst_bm_decoder_copy_frame_to_system (GstBmDecoder * decoder, GstBmDecoderFrame * frame, GstBuffer * buffer)
{
  GstVideoFrame video_frame;
  gint i = 0;
  gboolean ret = TRUE;

  if (!gst_video_frame_map (&video_frame, &decoder->info, buffer,
          GST_MAP_WRITE)) {
    GST_ERROR_OBJECT (decoder, "Couldn't map video frame");
    return FALSE;
  }

  for (i = 0; i < GST_VIDEO_FRAME_N_PLANES (&video_frame); i++) {
    // TODO: memcpy per channel
  }

done:
  gst_video_frame_unmap (&video_frame);

  GST_LOG_OBJECT (decoder, "Copy frame to system ret %d", ret);

  return ret;
}

static gboolean gst_bm_decoder_copy_frame_to_device (GstBmDecoder * decoder, GstBmDecoderFrame * frame, GstBuffer * buffer)
{
  // TODO: allocate device memory
  return TRUE;
}
#endif
void gst_bm_decoder_register (GstPlugin * plugin, guint soc_idx, GstCaps * sink_caps, GstCaps * src_caps)
{
  GType type;
  gchar *type_name;
  GstBmDecoderClassData *class_data;
 // const GValue *value;
 // GstStructure *s;
  GTypeInfo type_info = {
    sizeof (GstBmDecoderClass),
    NULL,
    NULL,
    (GClassInitFunc) gst_bm_decoder_subclass_init,
    NULL,
    NULL,
    sizeof (GstBmDecoder),
    0,
    (GInstanceInitFunc) gst_bm_decoder_subinstance_init,
    NULL,
  };

  GST_DEBUG_CATEGORY_INIT (gst_bm_decoder_debug, "bmdecoder", 0, "bmdecoder");

  class_data = g_new0 (GstBmDecoderClassData, 1);
  class_data->sink_caps = gst_caps_ref (sink_caps);
  class_data->src_caps = gst_caps_ref (src_caps);
  class_data->soc_index = soc_idx;

  type_name = g_strdup (GST_BM_DECODER_TYPE_NAME);
  type_info.class_data = class_data;
  type = g_type_register_static (GST_TYPE_BM_DECODER, type_name, &type_info, 0);

  if (!gst_element_register (plugin, type_name, GST_RANK_PRIMARY, type)) {
    GST_WARNING ("Failed to register plugin '%s'", type_name);
  }

  g_free (type_name);
}
