#include "ff_avframe_convert.h"
#include "bmlib_runtime.h"
#include <unistd.h>
#include <sys/time.h>


enum AVPixelFormat get_bmcodec_format(AVCodecContext *ctx,
                                             const enum AVPixelFormat *pix_fmts)
{
    const enum AVPixelFormat *p;

    av_log(ctx, AV_LOG_TRACE, "[%s,%d] Try to get HW surface format.\n", __func__, __LINE__);

    for (p = pix_fmts; *p != AV_PIX_FMT_NONE; p++) {
        if (*p == AV_PIX_FMT_BMCODEC) {
            AVHWFramesContext  *frames_ctx;
            int ret;

            /* create a pool of surfaces to be used by the decoder */
            ctx->hw_frames_ctx = av_hwframe_ctx_alloc(ctx->hw_device_ctx);
            if (!ctx->hw_frames_ctx)
                return AV_PIX_FMT_NONE;
            frames_ctx   = (AVHWFramesContext*)ctx->hw_frames_ctx->data;

            frames_ctx->format            = AV_PIX_FMT_BMCODEC;
            printf("frames_ctx->sw_format2=%d, ctx->sw_pix_fmt=%d\n",frames_ctx->sw_format,ctx->sw_pix_fmt);
            frames_ctx->sw_format         = ctx->sw_pix_fmt;
            printf("frames_ctx->sw_format1=%d, ctx->sw_pix_fmt=%d\n",frames_ctx->sw_format,ctx->sw_pix_fmt);

            if (ctx->coded_width > 0)
                frames_ctx->width         = FFALIGN(ctx->coded_width, 32);
            else if (ctx->width > 0)
                frames_ctx->width         = FFALIGN(ctx->width, 32);
            else
                frames_ctx->width         = FFALIGN(1920, 32);

            if (ctx->coded_height > 0)
                frames_ctx->height        = FFALIGN(ctx->coded_height, 32);
            else if (ctx->height > 0)
                frames_ctx->height        = FFALIGN(ctx->height, 32);
            else
                frames_ctx->height        = FFALIGN(1088, 32);

            frames_ctx->initial_pool_size = 0; // Don't prealloc pool.

            ret = av_hwframe_ctx_init(ctx->hw_frames_ctx);
            if (ret < 0)
                goto failed;

            av_log(ctx, AV_LOG_TRACE, "[%s,%d] Got HW surface format:%s.\n",
                   __func__, __LINE__, av_get_pix_fmt_name(AV_PIX_FMT_BMCODEC));

            return AV_PIX_FMT_BMCODEC;
        }
    }

failed:
    av_log(ctx, AV_LOG_ERROR, "Unable to decode this file using BMCODEC.\n");
    return AV_PIX_FMT_NONE;
}

int map_bmformat_to_avformat(int bmformat)
{
    int format;
    switch(bmformat){
        case FORMAT_YUV420P: format = AV_PIX_FMT_YUV420P; break;
        case FORMAT_YUV422P: format = AV_PIX_FMT_YUV422P; break;
        case FORMAT_YUV444P: format = AV_PIX_FMT_YUV444P; break;
        case FORMAT_NV12:    format = AV_PIX_FMT_NV12; break;
        case FORMAT_NV16:    format = AV_PIX_FMT_NV16; break;
        case FORMAT_GRAY:    format = AV_PIX_FMT_GRAY8; break;
        case FORMAT_RGBP_SEPARATE: format = AV_PIX_FMT_GBRP; break;
        default: printf("unsupported image format %d\n", bmformat); return -1;
    }
    return format;
}

int map_avformat_to_bmformat(int avformat)
{
    int format;
    switch(avformat){
        case AV_PIX_FMT_YUV420P: format = FORMAT_YUV420P; break;
        case AV_PIX_FMT_YUV422P: format = FORMAT_YUV422P; break;
        case AV_PIX_FMT_YUV444P: format = FORMAT_YUV444P; break;
        case AV_PIX_FMT_NV12:    format = FORMAT_NV12; break;
        case AV_PIX_FMT_NV16:    format = FORMAT_NV16; break;
        case AV_PIX_FMT_GRAY8:   format = FORMAT_GRAY; break;
        case AV_PIX_FMT_GBRP:    format = FORMAT_RGBP_SEPARATE; break;
        default: printf("unsupported av_pix_format %d\n", avformat); return -1;
    }

    return format;
}
int bm_image_sizeof_data_type(bm_image *image){

    switch(image->data_type){
    case DATA_TYPE_EXT_FLOAT32:
        return sizeof(float);
    case DATA_TYPE_EXT_1N_BYTE:
    case DATA_TYPE_EXT_1N_BYTE_SIGNED:
        return sizeof(char);
    default:
        return 1;
    }
}

void bmBufferDeviceMemFree(void *opaque, uint8_t *data)
{
#if 1
    if(opaque == NULL){
        printf("parameter error\n");
    }
    transcode_t *testTranscoed = (transcode_t *)opaque;
    av_freep(&testTranscoed->buf0);
        testTranscoed->buf0 = NULL;
    if(testTranscoed->hwpic)
        av_freep(&testTranscoed->hwpic);

    int ret =  0;
    ret = bm_image_destroy(testTranscoed->bmImg);



    if(testTranscoed->bmImg){
        free(testTranscoed->bmImg);
        testTranscoed->bmImg =NULL;
    }
    if(ret != 0)
        printf("bm_image destroy failed\n");
    free(testTranscoed);
    testTranscoed = NULL;
#endif
    return ;
}
static void bmBufferDeviceMemFree2(void *opaque, uint8_t *data)
{
    return ;
}

int avframe_to_bm_image(bm_handle_t &bm_handle,AVFrame &in, bm_image &out){

    static int mem_flags = USEING_MEM_HEAP1;


        //if (in.channel_layout == 101) {/* COMPRESSED NV12 FORMAT */
       if ((0 == in.height) || (0 == in.width) || \
        (0 == in.linesize[0]) || (0 == in.linesize[1]) || (0 == in.linesize[2]) || (0 == in.linesize[3]) || \
        (0 == in.data[0]) || (0 == in.data[1]) || (0 == in.data[2]) || (0 == in.data[3])) {
          printf("bm_image_from_frame: get yuv failed!!");
          return BM_ERR_PARAM;
        }
        bm_image cmp_bmimg;
        bm_image_create (bm_handle,
                  in.height,
                  in.width,
                  FORMAT_COMPRESSED,
                  DATA_TYPE_EXT_1N_BYTE,
                  &cmp_bmimg,
                  NULL
                  );

        bm_device_mem_t input_addr[4];
        int size = in.height * in.linesize[0];
        input_addr[0] = bm_mem_from_device((unsigned long long)in.data[2], size);
        size = (in.height / 2) * in.linesize[1];
        input_addr[1] = bm_mem_from_device((unsigned long long)in.data[0], size);
        size = in.linesize[2];
        input_addr[2] = bm_mem_from_device((unsigned long long)in.data[3], size);
        size = in.linesize[3];
        input_addr[3] = bm_mem_from_device((unsigned long long)in.data[1], size);
        bm_image_attach(cmp_bmimg, input_addr);

        bm_image_create (bm_handle,
                in.height,
                in.width,
                FORMAT_YUV420P,
                DATA_TYPE_EXT_1N_BYTE,
                &out,
                NULL);
        if(mem_flags == USEING_MEM_HEAP1 && bm_image_alloc_dev_mem_heap_mask(out,USEING_MEM_HEAP1) != BM_SUCCESS){
            printf("bmcv allocate mem failed!!!");
        }

        bmcv_rect_t crop_rect = {0, 0, (unsigned int)in.width, (unsigned int)in.height};
        bmcv_image_vpp_convert(bm_handle, 1, cmp_bmimg, &out, &crop_rect,BMCV_INTER_LINEAR);
        bm_image_destroy(&cmp_bmimg);
       return BM_SUCCESS;
}

int bm_image_to_avframe(bm_handle_t &bm_handle,bm_image *in,AVFrame *out,AVBufferRef *hw_frames_ctx){
#if 1
    transcode_t *ImgOut  = NULL;
    ImgOut = (transcode_t *)malloc(sizeof(transcode_t));
    ImgOut->bmImg = in;
    bm_image_format_info_t image_info;
    int idx       = 0;
    if(in == NULL || out == NULL){
        free(ImgOut);
        return -1;
    }

    out->format = AV_PIX_FMT_BMCODEC;
    out->height = ImgOut->bmImg->height;
    out->width = ImgOut->bmImg->width;


    AVBmCodecFrame* hwpic = NULL;
    hwpic = (AVBmCodecFrame *)av_mallocz(sizeof(AVBmCodecFrame));
    if (hwpic == NULL) {
        av_log(NULL, AV_LOG_ERROR, "av_mallocz failed\n");
        return AVERROR(ENOMEM);
    }
    hwpic->type = 0;
    hwpic->buffer = NULL;
    hwpic->coded_width = ImgOut->bmImg->height;
    hwpic->coded_height = ImgOut->bmImg->width;
    ImgOut->hwpic = hwpic;
    hwpic->maptype = 0;

    bm_device_mem_t mem_tmp[4];
    if(bm_image_get_device_mem(*(ImgOut->bmImg),mem_tmp) != BM_SUCCESS ){
        free(ImgOut);
        free(in);
        return -1;
    }

    if(bm_image_get_format_info(ImgOut->bmImg, &image_info) != BM_SUCCESS ){
        free(ImgOut);
        free(in);
        return -1;
    }

    for (idx=0; idx< 4; idx++) {
        out->data[idx] = hwpic->data[idx] = (uint8_t *)mem_tmp[idx].u.device.device_addr;
        out->linesize[idx] = hwpic->linesize[idx] =  image_info.stride[idx];
    }

    out->data[4] = (uint8_t*)hwpic;

    if(ImgOut->bmImg->width > 0 && ImgOut->bmImg->height > 0
        && ImgOut->bmImg->height * ImgOut->bmImg->width <= 8192*4096) {
        ImgOut->buf0 = (uint8_t*)av_malloc(ImgOut->bmImg->height * ImgOut->bmImg->width * 3 / 2);
        ImgOut->buf1 = ImgOut->buf0 + (unsigned int)(ImgOut->bmImg->height * ImgOut->bmImg->width);
        ImgOut->buf2 = ImgOut->buf0 + (unsigned int)(ImgOut->bmImg->height * ImgOut->bmImg->width * 5 / 4);
    }

    out->buf[0] = av_buffer_create(ImgOut->buf0,ImgOut->bmImg->width * ImgOut->bmImg->height, \
                                  bmBufferDeviceMemFree,ImgOut,AV_BUFFER_FLAG_READONLY);
    out->buf[1] = av_buffer_create(ImgOut->buf1,ImgOut->bmImg->width * ImgOut->bmImg->height / 2 /2 , \
                                  bmBufferDeviceMemFree2,NULL,AV_BUFFER_FLAG_READONLY);
    out->buf[2] = av_buffer_create(ImgOut->buf2,ImgOut->bmImg->width * ImgOut->bmImg->height / 2 /2 , \
                                  bmBufferDeviceMemFree2,NULL,AV_BUFFER_FLAG_READONLY);

    //if(!out->buf[2] || !out->buf[1] || !out->buf[0]){
    //    av_buffer_unref(&out->buf[0]);
    //    av_buffer_unref(&out->buf[1]);
    //    av_buffer_unref(&out->buf[2]);
    //    free(ImgOut);
    //    free(in);
    //    free(hwpic);
    //    return -1;
    //}
    if (hw_frames_ctx) {
        out->hw_frames_ctx = av_buffer_ref(hw_frames_ctx);
        if (!out->hw_frames_ctx)
            return AVERROR(ENOMEM);
    }

    return 0;
#endif

}

/*   AVFrameConvert
*    dec AVFrmae->bm_image(nv12/compress)->bm_image(yuv420p&resize)->enc AVFrame
*    @bmHandle    operation device handle
*    @inPic        dec AVFarme
*    @outPic        enc AVFrame
*    @enc_frame_height enc_frame_width enc_pix_format enc para
*    @enable_mosaic       0: Nothing 1:Mosaic operation on video
*    @enable_watermark    0: Nothing 1:watermark operation on video
*    @watermark           bm_device_mem_t* for watermark paddr
*    convert success return 0 else return -1.
*/
int AVFrameConvert(bm_handle_t &bmHandle,AVFrame *inPic,AVFrame *outPic,int enc_frame_height,int enc_frame_width,int enc_pix_format, AVBufferRef *hw_frames_ctx, int enable_mosaic,int enable_watermark, bm_device_mem_t* watermark){
    if(!inPic){
        return -1;
    }

    bm_image *bmImagein    = NULL;
    //bm_image *bmImageout    = NULL;
    int ret                   = 0;
    //release this func
    bmImagein = (bm_image *) malloc(sizeof(bm_image));
    //bmImageout = (bm_image *) malloc(sizeof(bm_image));
    //release by callback
    //av_frame_unref(outPic);
    if(!bmImagein){
        return -1;
    }
    if(avframe_to_bm_image(bmHandle,*inPic,*bmImagein)!= BM_SUCCESS){
        return -1;
    }
#if 0
    static int mem_flags = USEING_MEM_HEAP1;
    bm_image_format_ext bmOutFormat = (bm_image_format_ext)map_avformat_to_bmformat(FORMAT_YUV420P);
    int stride_yuv = ((enc_frame_width +31) >> 5) << 5;
    int stride_bmi[3] = {0};
    stride_bmi[0] = stride_yuv;
    stride_bmi[1] = stride_bmi[2] = stride_yuv / 2;
    bm_image_create(bmHandle,enc_frame_height,enc_frame_width,bmOutFormat,DATA_TYPE_EXT_1N_BYTE, bmImageout,stride_bmi,NULL);
    //vpu cannot use other heap memory(soc and pcie)

    if(mem_flags == USEING_MEM_HEAP1 && bm_image_alloc_dev_mem_heap_mask(*bmImageout,USEING_MEM_HEAP1) != BM_SUCCESS){
        printf("bmcv allocate mem failed!!!");
    }

    if(!bm_image_is_attached(*bmImageout)){
        return -1;
    }
    bmcv_rect_t crop_rect = {0, 0, inPic->width, inPic->height};
    ret = bmcv_image_vpp_convert(bmHandle, 1, *bmImagein, bmImageout ,&crop_rect,BMCV_INTER_LINEAR);
    if(ret != BM_SUCCESS){
        printf("convert error coed = %d\n",ret);
        return -1;
    }
#endif

    /* mosaic process */
    if (enable_mosaic){
        bmcv_rect_t mosaic_rect = {0, 0, (unsigned int)(inPic->width/2), (unsigned int)(inPic->height/2)};
        ret = bmcv_image_mosaic(bmHandle, 1, *bmImagein, &mosaic_rect, 0);
        if(ret != BM_SUCCESS){
            printf("mosaic_process failed \n");
            return -1;
        }
    }

    /* watermark process*/
    if (enable_watermark){
        int interval = 300;
        int water_w = 117;
        int water_h = 79;
        int font_idx = 0;

        bmcv_rect_t * rect;
        bmcv_color_t color;
        color.r = 128;
        color.g = 128;
        color.b = 128;

        int font_num = ((inPic->width - water_w) / (water_w + interval) + 1) * ((inPic->height - water_h) / (water_h + interval) + 1);

        rect = (bmcv_rect_t*)malloc(sizeof(bmcv_rect_t)*font_num);
        for(int w = 0; w < ((inPic->width - water_w) / (water_w + interval) + 1); w++){
            for(int h = 0; h < ((inPic->height - water_h) / (water_h + interval) + 1); h++){
                rect[font_idx].start_x = w * (water_w + interval);
                rect[font_idx].start_y = h * (water_h + interval);
                rect[font_idx].crop_w = water_w;
                rect[font_idx].crop_h = water_h;
                font_idx = font_idx + 1;
            }
        }

        bmcv_image_watermark_repeat_superpose(bmHandle, *bmImagein, *watermark, font_num, 0, water_w, rect, color);
        free(rect);
    }

    if(bm_image_to_avframe(bmHandle,bmImagein,outPic,hw_frames_ctx) != 0){
        return -1;
    }
    //bm_image_destroy(*bmImagein);
    //free(bmImagein);
    return 0;
}
