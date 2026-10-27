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
#include "bmcv_api_ext.h"
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
}transcode_t;

/*
 * in_coded_width/in_coded_height: coded(aligned) size of the input frame buffer,
 * only AVCodecContext has it. Needed by the FBD(FBC) compressed path, pass 0 to
 * fall back to the display size carried by the AVFrame.
 */
int AVFrameConvert(bm_handle_t &bmHandle,AVFrame *inPic,AVFrame *outPic,int enc_frame_height,int enc_frame_width,int enc_pix_format, int enable_mosaic=0, int enable_watermark=0, bm_device_mem_t* watermark=NULL, int in_coded_width=0, int in_coded_height=0);
#endif
