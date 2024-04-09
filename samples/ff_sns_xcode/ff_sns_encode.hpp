/*
 * Copyright (c) 2019 BitMain
 *
 *
 * @file
 *  example for video encode from BGR data,
 *   ps: i implement a C++ encoder it will be more easy to understand ffmpeg
 */
#ifdef __cplusplus
extern "C" {
#endif
#include "libavcodec/avcodec.h"
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavformat/avformat.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#ifdef __cplusplus
}
#endif

#include <iostream>

int loopflag = 0;
class VideoEnc_FFMPEG
{
public:
    VideoEnc_FFMPEG();
    ~VideoEnc_FFMPEG();
    int openEnc(const char* filename, int codecId, int framerate,
                int width, int height, int inputformat, int bitrate,
                int sophon_idx = 0);

    int flush_encoder();
    void closeEnc();
    int writeAvFrame(AVFrame *inputPicture);
    int isClosed();
private:
    AVCodecContext  *enc_ctx;
    AVFormatContext *ofmt_ctx;
    AVStream        *out_stream;
    int              frame_idx;
    int              is_closed;
};

VideoEnc_FFMPEG::VideoEnc_FFMPEG() {
    ofmt_ctx = NULL;
    out_stream = NULL;
    frame_idx = 0;
    is_closed = 1;
}
VideoEnc_FFMPEG::~VideoEnc_FFMPEG() {
    printf("#######VideoEnc_FFMPEG exit \n");
}

int VideoEnc_FFMPEG::openEnc(const char* filename, int codecId, int framerate,
                             int width, int height, int inputformat, int bitrate,
                             int sophon_idx) {
    const AVCodec *encoder;
    AVDictionary *opts = NULL;
    frame_idx = 0;
    int ret = 0;

    avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, filename);
    if (!ofmt_ctx) {
        av_log(NULL, AV_LOG_ERROR, "Could not create output context\n");
        return AVERROR_UNKNOWN;
    }

    /* Find the hardware video encoder */
    encoder = avcodec_find_encoder((AVCodecID)codecId);
    if (!encoder) {
        av_log(NULL, AV_LOG_FATAL, "Necessary Hardware encoder not found\n");
        return AVERROR_INVALIDDATA;
    }

    enc_ctx = avcodec_alloc_context3(encoder);
    if (!enc_ctx) {
        av_log(NULL, AV_LOG_FATAL, "Failed to allocate the encoder context\n");
        return AVERROR(ENOMEM);
    }
    enc_ctx->codec_id = (AVCodecID)codecId;
    enc_ctx->height   = height;
    enc_ctx->width    = width;
    enc_ctx->pix_fmt  = AV_PIX_FMT_YUV420P;
    if (encoder->pix_fmts) {
        for (int i=0; encoder->pix_fmts[i] != AV_PIX_FMT_NONE; i++) {
            av_log(enc_ctx, AV_LOG_DEBUG, "format %d: %s\n", i, av_get_pix_fmt_name(encoder->pix_fmts[i]));
            if (encoder->pix_fmts[i] == inputformat) {
                enc_ctx->pix_fmt = (AVPixelFormat)inputformat;
                break;
            }
        }
    }
    av_log(enc_ctx, AV_LOG_INFO, "input format %s\n", av_get_pix_fmt_name((AVPixelFormat)inputformat));
    av_log(enc_ctx, AV_LOG_INFO, "selected format %s\n", av_get_pix_fmt_name(enc_ctx->pix_fmt));

    enc_ctx->bit_rate_tolerance = bitrate;
    enc_ctx->bit_rate = (int)bitrate;
    enc_ctx->gop_size = 32;

    /* video time_base can be set to whatever is handy and supported by encoder */
    enc_ctx->time_base.num = 1;
    enc_ctx->time_base.den = framerate;
    enc_ctx->framerate.num = framerate;
    enc_ctx->framerate.den = 1;

    out_stream = avformat_new_stream(ofmt_ctx, encoder);
    out_stream->time_base = enc_ctx->time_base;
    out_stream->avg_frame_rate = enc_ctx->framerate;
    out_stream->r_frame_rate = out_stream->avg_frame_rate;

    av_dict_set_int(&opts, "gop_preset", 9, 0);
    av_dict_set_int(&opts, "is_dma_buffer", 1, 0);
    av_dict_set_int(&opts, "enc_cmd_queue", 1, 0);
    /* Third parameter can be used to pass settings to encoder */
    ret = avcodec_open2(enc_ctx, encoder, &opts);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open video encoder ");
        return ret;
    }
    av_dict_free(&opts);

    ret = avcodec_parameters_from_context(out_stream->codecpar, enc_ctx);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to copy encoder parameters to output stream ");
        return ret;
    }
    if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmt_ctx->pb, filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Could not open output file '%s'", filename);
            return ret;
        }
    }

    /* init muxer, write output file header */
    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error occurred when opening output file\n");
        return ret;
    }
    av_dict_free(&opts);
    is_closed = 0;

    return 0;
}

int VideoEnc_FFMPEG::writeAvFrame(AVFrame *inputPicture) {
    int ret = 0 ;
    inputPicture->pts = frame_idx;
    frame_idx++;
    AVPacket enc_pkt;

    av_log(enc_ctx, AV_LOG_DEBUG, "Encoding frame\n");

    /* encode filtered frame */
    enc_pkt.data = NULL;
    enc_pkt.size = 0;
    av_init_packet(&enc_pkt);
    enc_pkt.pts= inputPicture->pts;

    if ((ret = avcodec_send_frame(enc_ctx, inputPicture)) < 0) {
        av_log(NULL, AV_LOG_ERROR, " avcodec_send_frame error ret=%d\n", ret);
        return ret;
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

    if (!loopflag) {
        /* prepare packet for muxing */
        av_log(enc_ctx, AV_LOG_DEBUG, "[%s,%d] enc_pkt.pts=%ld,enc_pkt.dts=%ld\n",
            __func__, __LINE__, enc_pkt.pts,enc_pkt.dts);

        av_packet_rescale_ts(&enc_pkt, enc_ctx->time_base,out_stream->time_base);

        av_log(enc_ctx, AV_LOG_DEBUG, "[%s,%d] real enc_pkt.pts=%ld,enc_pkt.dts=%ld\n",
            __func__, __LINE__, enc_pkt.pts,enc_pkt.dts);

        /* mux encoded frame */
        av_log(enc_ctx, AV_LOG_DEBUG, "Muxing frame\n");

        ret = av_interleaved_write_frame(ofmt_ctx, &enc_pkt);
    }
    av_packet_unref(&enc_pkt);

    return ret;
}

int  VideoEnc_FFMPEG::flush_encoder() {
    int ret = 0;

    if (!(enc_ctx->codec->capabilities & AV_CODEC_CAP_DELAY))
        return 0;

    while (1) {
        AVPacket enc_pkt;
        enc_pkt.data = NULL;
        enc_pkt.size = 0;
        av_init_packet(&enc_pkt);

        av_log(enc_ctx, AV_LOG_INFO, "Flushing video encoder\n");
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

        if (ret < 0)
            return ret;
        if (!loopflag) {
            /* prepare packet for muxing */
            av_log(enc_ctx, AV_LOG_DEBUG, "[%s,%d] enc_pkt.pts=%ld,enc_pkt.dts=%ld\n",
                __func__, __LINE__, enc_pkt.pts, enc_pkt.dts);

            av_packet_rescale_ts(&enc_pkt, enc_ctx->time_base,out_stream->time_base);

            av_log(enc_ctx, AV_LOG_DEBUG, "[%s,%d] real enc_pkt.pts=%ld,enc_pkt.dts=%ld\n",
                __func__, __LINE__, enc_pkt.pts, enc_pkt.dts);

            /* mux encoded frame */
            av_log(enc_ctx, AV_LOG_DEBUG, "Muxing frame\n");

            ret = av_interleaved_write_frame(ofmt_ctx, &enc_pkt);
        }
        av_packet_unref(&enc_pkt);
        if (ret < 0)
            break;
    }

    return ret;
}

void VideoEnc_FFMPEG::closeEnc() {
    flush_encoder();
    av_write_trailer(ofmt_ctx);

    avcodec_free_context(&enc_ctx);

    if (ofmt_ctx && !(ofmt_ctx->oformat->flags & AVFMT_NOFILE))
        avio_closep(&ofmt_ctx->pb);
    avformat_free_context(ofmt_ctx);
    is_closed = 1;
}
int VideoEnc_FFMPEG::isClosed()
{
    if(is_closed)
        return 1;
    else
        return 0;
}

