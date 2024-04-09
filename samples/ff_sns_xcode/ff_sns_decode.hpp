#include <iostream>
#include <unistd.h>
extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
}

class VideoDec_FFMPEG
{
public:
    VideoDec_FFMPEG();
    ~VideoDec_FFMPEG();

    int openDec(const char* filename, int extra_frame_buffer_num = 5, int sophon_idx = 0, int v4l2_buf_num = 8,
                int use_isp_chn_num = 1, int wdr_on = 0, int is_yuv_sensor = 0);
    void closeDec();
    int grabFrame(AVFrame *frame);
    int flushFrame(AVFrame *frame);
    int isClosed();
private:
    AVFormatContext   *ifmt_ctx;
    const AVCodec     *decoder;
    AVCodecContext    *video_dec_ctx;

    int width;
    int height;
    int pix_fmt;
    int is_closed;
    int video_stream_idx;
    AVPacket pkt;
    int refcount;
    int fflush_flag;

    int openCodecContext(int *stream_idx, AVCodecContext **dec_ctx,
                         AVFormatContext *fmt_ctx, enum AVMediaType type,
                         int extra_frame_buffer_num = 5, int sophon_idx = 0);
};

VideoDec_FFMPEG::VideoDec_FFMPEG() {
    ifmt_ctx = NULL;
    video_dec_ctx = NULL;
    decoder = NULL;
    is_closed = 1;
    width   = 0;
    height  = 0;
    pix_fmt = 0;

    video_stream_idx = -1;
    refcount = 1;

    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;
}

VideoDec_FFMPEG::~VideoDec_FFMPEG() {
    printf("#######VideoDec_FFMPEG exit \n");
}

int VideoDec_FFMPEG::openDec(const char* filename, int extra_frame_buffer_num, int sophon_idx,
                            int v4l2_buf_num, int use_isp_chn_num, int wdr_on, int is_yuv_sensor) {
    int ret = 0;
    AVDictionary *opts = NULL;
    av_dict_set_int(&opts, "v4l2_buffer_num", v4l2_buf_num, 0);
    av_dict_set_int(&opts, "use_isp_chn_num", use_isp_chn_num, 0);
    //only support frist dev
    av_dict_set_int(&opts, "wdr_on", wdr_on, 0);
    if (is_yuv_sensor)
        av_dict_set(&opts, "pixel_format", "yuyv422", 0);

    ret = avformat_open_input(&ifmt_ctx, filename, NULL, &opts);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
        return ret;
    }

    ret = avformat_find_stream_info(ifmt_ctx, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
        return ret;
    }

    ret = openCodecContext(&video_stream_idx, &video_dec_ctx, ifmt_ctx, AVMEDIA_TYPE_VIDEO ,
                           extra_frame_buffer_num);
    if (ret >= 0) {
        width   = video_dec_ctx->width;
        height  = video_dec_ctx->height;
        pix_fmt = video_dec_ctx->pix_fmt;
        is_closed = 0;
    }
    av_log(video_dec_ctx, AV_LOG_INFO,
           "openDec video_stream_idx = %d, pix_fmt = %d\n",
           video_stream_idx, pix_fmt);
    av_dict_free(&opts);

    return ret;
}

void VideoDec_FFMPEG::closeDec() {
    if (video_dec_ctx) {
        avcodec_free_context(&video_dec_ctx);
        video_dec_ctx = NULL;
    }
    if (ifmt_ctx) {
        avformat_close_input(&ifmt_ctx);
        ifmt_ctx = NULL;
    }
    is_closed = 1;
}

int VideoDec_FFMPEG::isClosed()
{
    if (is_closed)
        return 1;
    else
        return 0;
}

int VideoDec_FFMPEG::openCodecContext(int *stream_idx, AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx,
                                      enum AVMediaType type, int extra_frame_buffer_num, int sophon_idx) {
    int ret, stream_index;
    AVStream *st;
    const AVCodec *dec = NULL;
    AVDictionary *opts = NULL;

    ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not find %s stream \n",av_get_media_type_string(type));
        return ret;
    }

    stream_index = ret;
    st = fmt_ctx->streams[stream_index];

    /* find decoder for the stream */
    decoder = avcodec_find_decoder(st->codecpar->codec_id);
    if (!decoder) {
        av_log(NULL, AV_LOG_FATAL,"Failed to find %s codec\n",
               av_get_media_type_string(type));
        return AVERROR(EINVAL);
    }

    /* Allocate a codec context for the decoder */
    *dec_ctx = avcodec_alloc_context3(decoder);
    if (!*dec_ctx) {
        av_log(NULL, AV_LOG_FATAL, "Failed to allocate the %s codec context\n",
               av_get_media_type_string(type));
        return AVERROR(ENOMEM);
    }

    /* Copy codec parameters from input stream to output codec context */
    ret = avcodec_parameters_to_context(*dec_ctx, st->codecpar);
    if (ret < 0) {
        av_log(NULL, AV_LOG_FATAL, "Failed to copy %s codec parameters to decoder context\n",
               av_get_media_type_string(type));
        return ret;
    }

    /* Init the decoders, with or without reference counting */
    av_dict_set(&opts, "refcounted_frames", refcount ? "1" : "0", 0);
    av_dict_set(&opts, "sg_vi", 1 ? "1" : "0", 0);
    if(extra_frame_buffer_num > 5)
       av_dict_set_int(&opts, "extra_frame_buffer_num", extra_frame_buffer_num, 0);  // if we use dma_buffer mode

    ret = avcodec_open2(*dec_ctx, dec, &opts);
    if (ret < 0) {
        av_log(NULL, AV_LOG_FATAL, "Failed to open %s codec\n",
               av_get_media_type_string(type));
        return ret;
    }
    *stream_idx = stream_index;

    av_dict_free(&opts);

    return 0;
}

int VideoDec_FFMPEG::grabFrame(AVFrame *frame) {
    int ret = 0;

      while (1) {
        av_packet_unref(&pkt);
        ret = av_read_frame(ifmt_ctx, &pkt);
        if (ret < 0) {
            continue;
        }

        if (pkt.stream_index != video_stream_idx) {
            continue;
        }

        if (!frame) {
            av_log(video_dec_ctx, AV_LOG_ERROR, "Could not allocate frame\n");
            return ret;
        }

        ret = avcodec_send_packet(video_dec_ctx, &pkt);
        if (ret < 0)
        {
            fprintf(stderr, "Error sending a packet for decoding\n");
            return ret;
        }
        while(1) {
            ret = avcodec_receive_frame(video_dec_ctx, frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF || ret == 0) {
                break;
            }
            else if (ret<0) {
                av_log(NULL, AV_LOG_ERROR, " avcodec_receive_frame error ret=%d\n",ret);
                return ret;
            }
        }
        if (ret < 0) {
            if(ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
                av_log(video_dec_ctx, AV_LOG_ERROR, "Error decoding video frame (%d)\n", ret);
                return ret;
            }

            continue; // TODO
        }

        width   = video_dec_ctx->width;
        height  = video_dec_ctx->height;
        pix_fmt = video_dec_ctx->pix_fmt;

        if (frame->width != width || frame->height != height || frame->format != pix_fmt) {
            av_log(video_dec_ctx, AV_LOG_ERROR,
                   "Error: Width, height and pixel format have to be "
                   "constant in a rawvideo file, but the width, height or "
                   "pixel format of the input video changed:\n"
                   "old: width = %d, height = %d, format = %s\n"
                   "new: width = %d, height = %d, format = %s\n",
                   width, height, av_get_pix_fmt_name((AVPixelFormat)pix_fmt),
                   frame->width, frame->height,
                   av_get_pix_fmt_name((AVPixelFormat)frame->format));
            continue;
        }

        break;
    }

    return ret;
}

int VideoDec_FFMPEG::flushFrame(AVFrame *frame) {
    int ret = 0;

    ret = avcodec_send_packet(video_dec_ctx, NULL);
    while (true) {
        ret = avcodec_receive_frame(video_dec_ctx, frame);
        if (ret == AVERROR(EAGAIN)) {
            break;
        } else if (ret < 0) {
            if (ret != AVERROR_EOF) {
                av_log(NULL, AV_LOG_ERROR, "Error receiving decoded frame: %s\n", __func__, ret);
            }
            else {
                av_log(NULL, AV_LOG_DEBUG, "%s : flush finish !!\n",__func__);
            }
            return ret;
        }
    }
    return 0;
}