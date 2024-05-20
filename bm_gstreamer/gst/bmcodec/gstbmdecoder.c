#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include "gstbmdecoder.h"
#include <stdio.h>

GST_DEBUG_CATEGORY (gst_bm_decoder_debug);
#define GST_CAT_DEFAULT gst_bm_decoder_debug

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

#define parent_class gst_bm_decoder_parent_class
G_DEFINE_TYPE (GstBmDecoder, gst_bm_decoder, GST_TYPE_VIDEO_DECODER);

 #define GST_BM_DEC_TASK_STARTED(decoder) \
    (gst_pad_get_task_state ((decoder)->srcpad) == GST_TASK_STARTED)

struct BmFrameCtx
{
  void *handle;
  BMVidFrame frame;
};

static gboolean gst_bm_decoder_set_format(GstVideoDecoder *decoder,    GstVideoCodecState *state)
{
  GstBmDecoder *self = GST_BM_DECODER (decoder);
  GstStructure *structure;
  BMVidDecParam param;
  int bm_ret = 0;
  g_print("set_format \n");
	GST_DEBUG_OBJECT (self, "setting format: %" GST_PTR_FORMAT, state->caps);
  memset(&param, 0, sizeof(BMVidDecParam));
  structure = gst_caps_get_structure (state->caps, 0);

  if (gst_structure_has_name (structure, "video/x-h264"))
    param.streamFormat = 0; //h264

  if (gst_structure_has_name (structure, "video/x-h265"))
    param.streamFormat = 12; //hevc

  g_print("param.streamFormat = %d \n",param.streamFormat);
  param.wtlFormat = 0;  // non-compress mode
  param.extraFrameBufferNum = 10;
  param.streamBufferSize = 0x500000;
  param.enable_cache = 0;
  param.bsMode = 1;  // interrupt mode
  param.core_idx = -1;
  param.cbcrInterleave = 0;
  param.nv21 = 0;
  param.frameDelay = 0;
  //param.cmd_queue_depth = 4;


  if ((bm_ret = bmvpu_dec_create (&self->handle, param)) != 0) {
    GST_ERROR_OBJECT (self, "bmvpu_dec_create failed");
    return FALSE;
  }
  if (self->input_state) {
    if (gst_caps_is_strictly_equal (self->input_state->caps, state->caps))
      return TRUE;
    gst_bm_decoder_reset (self, FALSE, FALSE);
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
  g_print("flushing \n");
  gst_bm_decoder_reset (self, FALSE, FALSE);
  return TRUE;
}

static GstFlowReturn
gst_bm_decoder_finish (GstVideoDecoder * decoder)
{
  GstBmDecoder *self = GST_BM_DECODER (decoder);
  GST_DEBUG_OBJECT (decoder, "finishing");
  g_print("finish \n");
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

  return TRUE;
}

static gboolean gst_bm_decoder_close (GstVideoDecoder * decoder)
{
  GstBmDecoder *self = GST_BM_DECODER (decoder);

  GST_DEBUG_OBJECT (self, "closing decoder");
  if (self->handle) {
    bmvpu_dec_flush (self->handle);
    while (bmvpu_dec_get_status (self->handle) != BMDEC_STOP) {
      GST_DEBUG_OBJECT (self, "bmvpu get status loop");
      g_usleep (2000);
    }
    bmvpu_dec_delete (self->handle);
    self->handle = NULL;
  }

  if (self->input_state) {
    gst_video_codec_state_unref (self->input_state);
    self->input_state = NULL;
  }

  return TRUE;
}

static GstFlowReturn gst_bm_decoder_handle_frame (GstVideoDecoder * decoder, GstVideoCodecFrame * frame)
{
  GstFlowReturn ret = GST_FLOW_OK;
  GstBmDecoder *self = GST_BM_DECODER (decoder);
  GstBuffer *in_buffer = NULL;
  GstMapInfo map_info = GST_MAP_INFO_INIT;
  BMVidStream vid_stream;
  int bm_ret = 0;

  GST_DEBUG_OBJECT (self, "Handling frame %d", frame->system_frame_number);

  // TODO: deal with the frame in order of system_frame_number
  //gst_video_codec_frame_set_user_data (frame, (gpointer) decoder, NULL);

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
    return GST_FLOW_ERROR;
  }

  memset (&vid_stream, 0, sizeof (BMVidStream));
  vid_stream.buf = map_info.data;
 /*for (int i = 0; i < 40; i++) {
     g_print("val 0x%02x ", vid_stream.buf[i]);
  }
  g_print(" \n");*/
  vid_stream.length = map_info.size;
  vid_stream.pts = frame->pts;
  vid_stream.dts = frame->system_frame_number;
  vid_stream.end_of_stream = 0;

  GST_VIDEO_DECODER_STREAM_UNLOCK (decoder);
  if (self->handle != NULL) {
    GST_DEBUG_OBJECT (self, "send stream in %d \n", frame->system_frame_number);
    while ((bm_ret = bmvpu_dec_decode(self->handle, vid_stream)) != 0) {
      //g_print ("bmvpu decoder loop \n");
      g_usleep (1000);
    }
  } else {
    GST_ERROR_OBJECT (self, "bmvpu handle is invalid");
    ret = GST_FLOW_ERROR;
    goto error;
  }

  self->pending_frames++;
  self->frames =
      g_list_append (self->frames,
      GUINT_TO_POINTER (frame->system_frame_number));

error:
  GST_VIDEO_DECODER_STREAM_LOCK (decoder);

  gst_buffer_unmap (in_buffer, &map_info);
  gst_buffer_unref (in_buffer);
  gst_video_codec_frame_unref (frame);

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
  g_print("gst_bm_decoder_stop_task \n");
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
   g_print("gst_bm_decoder_stop_ok \n");
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
  self->flushing = TRUE;
  self->draining = drain;
  gst_bm_decoder_stop_task(&self->parent, drain);
  self->flushing = final;
  self->draining = FALSE;
  if (self->frames) {
    g_list_free (self->frames);
    self->frames = NULL;
  }
  self->output_type = GST_BM_DECODER_OUTPUT_TYPE_SYSTEM;
  self->configured = FALSE;
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
//  g_print("vidframe_clear in frameIdx %d \n", ctx->frame.frameIdx);
  bmvpu_dec_clear_output (ctx->handle, &ctx->frame);
  free(ctx);
}

static GstVideoFormat bm_format_to_gst_format(BMVidFrame* bmframe)
{
    GstVideoFormat gst_format;
  //  g_print("bmframe->frameFormat = %d bmframe->cbcrInterleave %d \n",
  //          bmframe->frameFormat, bmframe->cbcrInterleave);
    switch (bmframe->frameFormat) {
    case 0:
        if(bmframe->cbcrInterleave == 1) {
            if(bmframe->nv21 == 0)
                gst_format = GST_VIDEO_FORMAT_NV12;
            else if(bmframe->nv21 == 1)
                gst_format = GST_VIDEO_FORMAT_NV21;
            else
                gst_format = GST_VIDEO_FORMAT_UNKNOWN ;
        } else if(bmframe->cbcrInterleave == 0)
            gst_format = GST_VIDEO_FORMAT_I420;
        else
            gst_format = GST_VIDEO_FORMAT_UNKNOWN ;
        break;
    case 1:
        gst_format = GST_VIDEO_FORMAT_NV12;
        break;
    case 2:
        gst_format = GST_VIDEO_FORMAT_YUY2;
        break;
    case 3:
        gst_format = GST_VIDEO_FORMAT_NV16 ;
        break;
    case 5:
        if(bmframe->cbcrInterleave)
            gst_format = GST_VIDEO_FORMAT_UNKNOWN;
        else
            gst_format = GST_VIDEO_FORMAT_UNKNOWN;
        break;
    case 6:
        if(bmframe->cbcrInterleave)
            gst_format = GST_VIDEO_FORMAT_UNKNOWN;
        else
            gst_format = GST_VIDEO_FORMAT_UNKNOWN;
        break;
    default:
        gst_format = GST_VIDEO_FORMAT_UNKNOWN ;
        break;
    }
//    g_print("gst_format = %d \n", gst_format);
    return gst_format;
}
static gboolean gst_bm_decoder_check_video_info_change(GstVideoDecoder *decoder, BMVidFrame *pframe)
{
  GstBmDecoder *self = GST_BM_DECODER (decoder);
  GstVideoInfo *info = &self->info;
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
        GST_DEBUG_OBJECT (self, "send eos ok \n");
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
    GST_ERROR_OBJECT (self, "bmvpu_dec_get_output failed \n");
    free (pframeCtx);
    pframeCtx = NULL;
    goto error;
  }
  if (!gst_bm_decoder_check_video_info_change(decoder,pframe)) {
    goto error;
  }

  if (self->frames){
    system_frame_number = GPOINTER_TO_UINT (g_list_nth_data (self->frames, 0));
  } else {
    goto error;
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
  out_frame->output_buffer = buffer;
  quark = gst_bm_vidframe_quark();
  pframeCtx->handle = self->handle;
  gst_mini_object_set_qdata (GST_MINI_OBJECT(mem_import), quark, pframeCtx,
		gst_bm_vidframe_clear);

  self->task_ret = gst_video_decoder_finish_frame (decoder, out_frame);

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
