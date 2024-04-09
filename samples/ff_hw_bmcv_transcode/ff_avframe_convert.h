#ifndef __AVFRAME_CONVERT_H_
#define __AVFRAME_CONVERT_H_

extern "C" {
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "libavformat/avformat.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/opt.h"
#include "libavutil/pixdesc.h"
#include <stdio.h>
#include <unistd.h>
#include "bmcv_internal.h"
#include "bmcv_api_ext.h"
#include "libavutil/hwcontext_bmcodec.h"
}

//In SoC mode, heap distribution is the same as that of PCIe
/*
 * heap0   tpu
 * heap1   vpp
 * heap2   vpu
*/
#define USEING_MEM_HEAP1 2

typedef struct{
        bm_image *bmImg;
        uint8_t* buf0;
        uint8_t* buf1;
        uint8_t* buf2;
        AVBmCodecFrame* hwpic;
}transcode_t;

enum AVPixelFormat get_bmcodec_format(AVCodecContext *ctx,const enum AVPixelFormat *pix_fmts);
int AVFrameConvert(bm_handle_t &bmHandle,AVFrame *inPic,AVFrame *outPic,int enc_frame_height,int enc_frame_width,int enc_pix_format,AVBufferRef *hw_frames_ctx,int enable_mosaic,int enable_watermark, bm_device_mem_t* watermark);

#endif
