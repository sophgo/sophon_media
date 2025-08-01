#ifndef __GST_BM_DECODER_H__
#define __GST_BM_DECODER_H__

#include <gst/gst.h>
#include <gst/video/video.h>
#include "gstbmcodeccaps.h"
#include "gstbmallocator.h"

#include "bm_vpudec_interface.h"
#include "bm_jpeg_interface.h"

G_BEGIN_DECLS

#define GST_BM_DECODER_TYPE_NAME "bmdec"

#define GST_TYPE_BM_DECODER \
  (gst_bm_decoder_get_type())
#define GST_BM_DECODER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_BM_DECODER,GstBmDecoder))
#define GST_BM_DECODER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_BM_DECODER,GstBmDecoderClass))
#define GST_IS_BM_DECODER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_BM_DECODER))
#define GST_IS_BM_DECODER_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_TYPE((obj),GST_TYPE_BM_DECODER))

typedef struct _GstBmDecoder GstBmDecoder;
typedef struct _GstBmDecoderClass GstBmDecoderClass;

typedef enum
{
  GST_BM_DECODER_OUTPUT_TYPE_SYSTEM = 0,
  GST_BM_DECODER_OUTPUT_TYPE_DEVICE,
} GstBmDecoderOutputType;

typedef void* BmDecoderHandle;

struct _GstBmDecoder
{
  GstVideoDecoder parent;
  BMVidCodHandle handle;
  BmJpuJPEGDecoder *jpeg_decoder;
  BmVideoCodecID codec_id;

  gboolean configured;

  GMutex mutex;

  guint8 *bitstream_buffer;
  gsize bitstream_buffer_alloc_size;
  gsize bitstream_buffer_offset;

  guint *slice_offsets;
  guint slice_offsets_alloc_len;
  guint num_slices;

  guint width, height;
  guint coded_width, coded_height;
  guint bitdepth;
  guint chroma_format_idc;
  gint max_dpb_size;

  gboolean interlaced;
  gboolean flushing;
  gboolean flushed;
  gboolean closing;
  /* flow return from pad task */
  GstFlowReturn task_ret;

  guint frame_mode;
  guint extraFrameBufferNum;
  GList *frames;

  GArray *ref_list;

  GstAllocator *allocator;
  GstVideoCodecState *input_state;

  GstVideoInfo info;
  GstVideoInfo coded_info;

  GstBmDecoderOutputType output_type;

  guint system_frame_number;
};

typedef struct _GstBmDecoderClass
{
  GstVideoDecoderClass parent_class;

  guint soc_index;
}GstBmDecoderClass;

typedef struct _GstBmDecoderClassData
{
  GstCaps *sink_caps;
  GstCaps *src_caps;
  guint soc_index;
} GstBmDecoderClassData;

typedef struct _GstBmDecoderFrame
{
  gint index;
  guintptr devptr;
  guint pitch;

  gboolean mapped;

  /* Extra frame allocated for AV1 film grain */
  gint decode_frame_index;

  /*< private >*/
  GstBmDecoder *decoder;

  gint ref_count;
} GstBmDecoderFrame;

void gst_bm_decoder_register (GstPlugin * plugin, guint soc_idx, GstCaps * sink_caps, GstCaps * src_caps);


G_END_DECLS

#endif /* __GST_BM_DECODER_H__ */
