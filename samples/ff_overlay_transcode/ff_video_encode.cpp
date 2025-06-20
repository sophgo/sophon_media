#include "ff_video_encode.h"
#include <iostream>


VideoEnc_FFMPEG::VideoEnc_FFMPEG()
{
    pFormatCtx    = NULL;
    pOutfmtormat  = NULL;
    out_stream    = NULL;
    aligned_input = NULL;
    enc_frame_width    = 0;
    enc_frame_height   = 0;
    frame_idx      = 0;
    enc_pix_format = 0;
    dec_pix_format = 0;
    frameWrite = av_frame_alloc();
}

VideoEnc_FFMPEG::~VideoEnc_FFMPEG()
{
    printf("#######VideoEnc_FFMPEG exit \n");
}

int VideoEnc_FFMPEG::openEnc(const char* output_filename, const char* codec_name ,int is_by_filename,
                              int framerate,int width, int height, int encode_pix_format, int bitrate,int sophon_idx)
{

    int ret = 0;
    const AVCodec *encoder = NULL;
    AVDictionary *dict = NULL;
    frame_idx = 0;
    enc_pix_format = encode_pix_format;

    enc_frame_width = width;
    enc_frame_height = height;


    if( !output_filename )
    {
        av_log(NULL, AV_LOG_ERROR, "inputfile and outputfile cannot not be NULL\n");
        return -1;
    }

    avformat_alloc_output_context2(&pFormatCtx, NULL, NULL, output_filename);
    if (!pFormatCtx) {
        av_log(NULL, AV_LOG_ERROR, "Could not create output context\n");
        return AVERROR_UNKNOWN;
    }


    if(is_by_filename && output_filename){

        pOutfmtormat = av_guess_format(NULL, output_filename, NULL);
        if(pOutfmtormat->video_codec == AV_CODEC_ID_NONE){
            printf("Unable to assign encoder automatically by file name, please specify by parameter...\n");
            return -1;
        }
        pFormatCtx->oformat = pOutfmtormat;
        encoder = avcodec_find_encoder(pOutfmtormat->video_codec);
    }
    if(codec_name != NULL)
        encoder = avcodec_find_encoder_by_name(codec_name);
    if(!encoder){
        printf("Failed to find encoder please try again\n");
        return -1;
    }


    enc_ctx = avcodec_alloc_context3(encoder);
    if (!enc_ctx) {
        av_log(NULL, AV_LOG_FATAL, "Failed to allocate the encoder context\n");
        return AVERROR(ENOMEM);
    }
    enc_ctx->codec_id           = encoder->id;
    enc_ctx->width              = width;
    enc_ctx->height             = height;
    enc_ctx->pix_fmt            = (AVPixelFormat)enc_pix_format;
    enc_ctx->bit_rate_tolerance = bitrate;
    enc_ctx->bit_rate           = (int64_t)bitrate;
    enc_ctx->gop_size           = 32;
    /* video time_base can be set to whatever is handy and supported by encoder */
    enc_ctx->time_base          = (AVRational){1, framerate};
    enc_ctx->framerate          = (AVRational){framerate,1};
    av_log(NULL, AV_LOG_DEBUG, "enc_ctx->bit_rate = %ld\n", enc_ctx->bit_rate);

    out_stream = avformat_new_stream(pFormatCtx, encoder);
    out_stream->time_base       = enc_ctx->time_base;
    out_stream->avg_frame_rate  = enc_ctx->framerate;
    out_stream->r_frame_rate    = out_stream->avg_frame_rate;

#ifdef BM_PCIE_MODE
    av_dict_set_int(&dict, "sophon_idx", sophon_idx, 0);
#endif
    av_dict_set_int(&dict, "gop_preset", 3, 0);
    /* Use system memory */

    av_dict_set_int(&dict, "is_dma_buffer", 1 , 0);
    /* Third parameter can be used to pass settings to encoder */
    ret = avcodec_open2(enc_ctx, encoder, &dict);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open video encoder ");
        return ret;
    }

    ret = avcodec_parameters_from_context(out_stream->codecpar, enc_ctx);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to copy encoder parameters to output stream ");
        return ret;
    }

    if (!(pFormatCtx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open2(&pFormatCtx->pb, output_filename, AVIO_FLAG_WRITE,NULL,NULL);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Could not open output file '%s'", output_filename);
            return ret;
        }
    }

    /* init muxer, write output file header */
    ret = avformat_write_header(pFormatCtx, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error occurred when opening output file\n");
        return ret;
    }
    av_dict_free(&dict);
    return 0;
}

/* data is alligned with 32 */
int VideoEnc_FFMPEG::writeFrame(AVFrame * inPic)
{
    int ret = 0 ;
    inPic->pts = frame_idx;
    frame_idx++;
    av_log(NULL, AV_LOG_DEBUG, "Encoding frame\n");

    /* encode filtered frame */
    AVPacket enc_pkt;
    enc_pkt.data = NULL;
    enc_pkt.size = 0;
    av_init_packet(&enc_pkt);

    enc_pkt.pts= inPic->pts;

    if ((ret = avcodec_send_frame(enc_ctx, inPic)) < 0) {
        av_log(NULL, AV_LOG_ERROR, " avcodec_send_frame error ret=%d\n",ret);
    }
    while (1) {
        ret = avcodec_receive_packet(enc_ctx, &enc_pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
            return ret;
        }
        else if (ret == 0){
            enc_pkt.stream_index = 0;
            break;
        }
        else if (ret<0){
            av_log(NULL, AV_LOG_ERROR, " avcodec_receive_packet error ret=%d\n",ret);
        }
    }

    if (ret < 0)
        return ret;

    /* prepare packet for muxing */
    av_log(NULL, AV_LOG_DEBUG, "enc_pkt.pts=%ld, enc_pkt.dts=%ld\n",
           enc_pkt.pts, enc_pkt.dts);
    av_packet_rescale_ts(&enc_pkt, enc_ctx->time_base,out_stream->time_base);
    av_log(NULL, AV_LOG_DEBUG, "rescaled enc_pkt.pts=%ld, enc_pkt.dts=%ld\n",
           enc_pkt.pts,enc_pkt.dts);

    av_log(NULL, AV_LOG_DEBUG, "Muxing frame\n");

    /* mux encoded frame */
    ret = av_interleaved_write_frame(pFormatCtx, &enc_pkt);
    return ret;

}

int  VideoEnc_FFMPEG::flush_encoder()
{
    int ret;
    if (!(enc_ctx->codec->capabilities & AV_CODEC_CAP_DELAY))
        return 0;
    while (1) {
        av_log(NULL, AV_LOG_INFO, "Flushing video encoder\n");
        AVPacket enc_pkt;
        enc_pkt.data = NULL;
        enc_pkt.size = 0;
        av_init_packet(&enc_pkt);
        //printf("xxxx\n");
         ret = avcodec_send_frame(enc_ctx, NULL);
        while(1) {
            ret = avcodec_receive_packet(enc_ctx, &enc_pkt);
            if (ret == AVERROR(EAGAIN)) {
                av_packet_unref(&enc_pkt);
                break;
            }
            else if (ret == 0) {
                break;
            }
            else if (ret == AVERROR_EOF) {
                av_packet_unref(&enc_pkt);
                av_log(NULL, AV_LOG_DEBUG, "%s : flush finish !!\n",__func__);
                return ret;
            }
            else{
                av_log(NULL, AV_LOG_ERROR, "%s :avcodec_receive_packet error,ret=%d \n",__func__,ret);
                return ret;
            }
        }

        if (ret != 0){
            continue;
        }

        /* prepare packet for muxing */
        av_log(NULL, AV_LOG_DEBUG, "enc_pkt.pts=%ld, enc_pkt.dts=%ld\n",
               enc_pkt.pts,enc_pkt.dts);
        av_packet_rescale_ts(&enc_pkt, enc_ctx->time_base,out_stream->time_base);
        av_log(NULL, AV_LOG_DEBUG, "rescaled enc_pkt.pts=%ld, enc_pkt.dts=%ld\n",
               enc_pkt.pts,enc_pkt.dts);
        /* mux encoded frame */
        av_log(NULL, AV_LOG_DEBUG, "Muxing frame\n");
        ret = av_interleaved_write_frame(pFormatCtx, &enc_pkt);
        if (ret < 0)
            break;
    }
    return ret;
}

void VideoEnc_FFMPEG::closeEnc()
{

    flush_encoder();
    av_write_trailer(pFormatCtx);

    if(frameWrite)
        av_frame_free(&frameWrite);

    avcodec_free_context(&enc_ctx);
    if (pFormatCtx && !(pFormatCtx->oformat->flags & AVFMT_NOFILE))
        avio_closep(&pFormatCtx->pb);
    avformat_free_context(pFormatCtx);
}
