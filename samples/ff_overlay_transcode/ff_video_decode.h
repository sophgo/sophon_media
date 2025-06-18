#ifndef __FF_VIDEO_DECODE_H
#define __FF_VIDEO_DECODE_H

#include <iostream>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavcodec/avcodec.h>
}

class VideoDec_FFMPEG
{
public:
    VideoDec_FFMPEG();
    ~VideoDec_FFMPEG();

    int openDec(const char* filename,int codec_name_flag,
                const char *coder_name,int output_format_mode = 100,
                int extra_frame_buffer_num = 5 ,
                int sophon_idx = 0, int pcie_no_copyback = 0);
    void closeDec();

    AVPacket pkt;
    AVFormatContext   *ifmt_ctx;
    int video_stream_idx;
    AVCodecContext    *video_dec_ctx ;
    AVCodecParameters* getCodecPar();
    AVFrame * grabFrame2();
private:
    const AVCodec     *decoder;
    AVCodecParameters *video_dec_par;

    int width;
    int height;
    int pix_fmt;

    AVFrame *frame;
    int refcount;
    int fflush_flag;

    const AVCodec* findBmDecoder(AVCodecID dec_id, const char *name = "h264_bm",
                                 int codec_name_flag = 0,
                                 enum AVMediaType type = AVMEDIA_TYPE_VIDEO);


    int openCodecContext(int *stream_idx, AVCodecContext **dec_ctx,
                         AVFormatContext *fmt_ctx, enum AVMediaType type,
                         int codec_name_flag, const char *coder_name,
                         int output_format_mode = 100,
                         int extra_frame_buffer_num = 5,
                         int sophon_idx = 0,int pcie_no_copyback = 0);

    AVFrame *flushDecoder(AVCodecContext *dec_ctx);
};


#endif /*__FF_VIDEO_DECODE_H*/

