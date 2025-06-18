#include <iostream>

extern "C" {
#include <sys/time.h>
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include <pthread.h>
#include "libavutil/file.h"
#include "libavutil/pixfmt.h"
}


using namespace std;
#define MAX_THREADS 64

typedef struct {
    int total_loops;
    char input_file[256];
    int sophon_idx;
    int zero_copy;
    int thread_id;
} ThreadConfig;

typedef struct {
    pthread_t tid;
    ThreadConfig config;
} ThreadInfo;


void* decode_thread(void* arg);
void* encode_thread(void* arg);
static int saveYUV(AVFrame* pFrame, int iIndex, string filename);

static int saveYUV(AVFrame* pFrame, int iIndex, string filename)
{
    string yuv_filename = filename + ".yuv";
    FILE *fp = fopen(yuv_filename.c_str(), "wb+");
    if (fp == nullptr) {
        cerr << "create yuv file failed:" <<  yuv_filename << endl;
        return -1;
    }

    uint8_t * pY = pFrame->data[0];
    uint8_t * pU = pFrame->data[1];
    uint8_t * pV = pFrame->data[2];
    int frameWidth  = pFrame->width;
    int frameHeight = pFrame->height;
    int y_stride = pFrame->linesize[0];//may be larger than width because of alignment
    int u_stride = pFrame->linesize[1];
    int v_stride = pFrame->linesize[2];

    cout << "frame width:" << frameWidth << " frame height:" << frameHeight << endl;
    cout << "Y num:" << y_stride << " U num:" << u_stride << " V num:" << v_stride<< endl;

    if (AV_PIX_FMT_YUVJ420P == pFrame->format || AV_PIX_FMT_YUV420P == pFrame->format) {
        cout << "Pixel format is YUV420P" << endl;
        for (int j=0; j < frameHeight; j++) {
            fwrite(pY + j*y_stride, 1, frameWidth, fp);
        }

        for (int j=0; j < frameHeight / 2; j++) {
            fwrite(pU + j*u_stride, 1, frameWidth/2, fp);
        }

        for (int j=0; j < frameHeight / 2; j++) {
            fwrite(pV + j*v_stride, 1, frameWidth/2, fp);
        }
    } else if (AV_PIX_FMT_YUVJ422P == pFrame->format || AV_PIX_FMT_YUV422P == pFrame->format) {
        cout << "Pixel format is YUV422P" << endl;
        for (int j=0; j < frameHeight; j++) {
            fwrite(pY + j*y_stride, 1, frameWidth, fp);
        }

        for (int j=0; j < frameHeight; j++) {
            fwrite(pU + j*u_stride, 1, frameWidth/2, fp);
        }

        for (int j=0; j < frameHeight; j++) {
            fwrite(pV + j*v_stride, 1, frameWidth/2, fp);
        }
    } else if (AV_PIX_FMT_YUVJ444P == pFrame->format || AV_PIX_FMT_YUV444P == pFrame->format) {
        cout << "Pixel format is YUV444P" << endl;
        for (int j=0; j < frameHeight; j++) {
            fwrite(pY + j*y_stride, 1, frameWidth, fp);
        }

        for (int j=0; j < frameHeight; j++) {
            fwrite(pU + j*u_stride, 1, frameWidth, fp);
        }

        for (int j=0; j < frameHeight; j++) {
            fwrite(pV + j*v_stride, 1, frameWidth, fp);
        }
    } else if (AV_PIX_FMT_GRAY8 == pFrame->format) {
        cout << "Pixel format is YUV400" << endl;
        for (int j=0; j < frameHeight; j++) {
            fwrite(pY + j*y_stride, 1, frameWidth, fp);
        }
    } else {
        cerr << "Not not support format:" << pFrame->format << endl;
        return -1;
    }

    fclose(fp);
    return 0;
}

void* decode_thread(void* arg)
{
    ThreadInfo* info = (ThreadInfo*)arg;
    AVFormatContext* fmt_ctx = NULL;
    AVCodecContext* codec_ctx = NULL;
    AVDictionary *dict = nullptr;
    AVPacket pkt;
    AVFrame* frame = NULL;
    int ret = 0;
    const char* filename = info->config.input_file;

    if (avformat_open_input(&fmt_ctx, filename, NULL, NULL) != 0) {
        cerr << "Thread " << info->config.thread_id << ": open input failed" << endl;
        return NULL;
    }

    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        cerr << "Thread " << info->config.thread_id << ": find stream failed" << endl;
        avformat_close_input(&fmt_ctx);
        return NULL;
    }

    const AVCodec*  codec = avcodec_find_decoder_by_name("jpeg_bm");
    if (!codec) {
        cerr << "Thread " << info->config.thread_id << ": codec not found" << endl;
        avformat_close_input(&fmt_ctx);
        return NULL;
    }

    codec_ctx = avcodec_alloc_context3(codec);

    #ifdef BM_PCIE_MODE
        av_dict_set_int(&dict, "zero_copy", info->config.zero_copy, 0);
        av_dict_set_int(&dict, "sophon_idx", info->config.sophon_idx, 0);
    #endif

    if (avcodec_open2(codec_ctx, codec, &dict) < 0) {
        cerr << "Thread " << info->config.thread_id << ": open codec failed" << endl;
        avformat_close_input(&fmt_ctx);
        avcodec_free_context(&codec_ctx);
        return NULL;
    }

    av_init_packet(&pkt);
    if (av_read_frame(fmt_ctx, &pkt) < 0) {
        cerr << "Thread " << info->config.thread_id << ": read frame failed" << endl;
    }

    for (int i = 0; i < info->config.total_loops; ++i) {

        ret = avcodec_send_packet(codec_ctx, &pkt);
        if (ret < 0) {
            cerr << "Thread " << info->config.thread_id << ": send packet failed" << endl;
            break;
        }

        while (ret >= 0) {
            frame = av_frame_alloc();
            ret = avcodec_receive_frame(codec_ctx, frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                av_frame_free(&frame);
                break;
            }

            if (i == info->config.total_loops - 1) {
                char output_file[256];
                snprintf(output_file, sizeof(output_file),
                        "dec-thread%d",
                        info->config.thread_id);
                saveYUV(frame, i, output_file);
            }

            av_frame_free(&frame);
        }
    }

    av_packet_unref(&pkt);
    avformat_close_input(&fmt_ctx);
    avcodec_free_context(&codec_ctx);
    return NULL;
}

void* encode_thread(void* arg)
{
    ThreadInfo* info = (ThreadInfo*)arg;
    AVFormatContext* dec_fmt_ctx = NULL;
    AVCodecContext* dec_ctx = NULL, *enc_ctx = NULL;
    AVPacket dec_pkt, enc_pkt;
    AVDictionary *dec_dict = nullptr;
    AVDictionary *enc_dict = nullptr;
    AVFrame* frame = NULL;
    int ret = 0;
    const char* filename = info->config.input_file;

    if (avformat_open_input(&dec_fmt_ctx, filename, NULL, NULL) != 0) {
        cerr << "Encoder thread " << info->config.thread_id << ": open input failed" << endl;
        return NULL;
    }

    if (avformat_find_stream_info(dec_fmt_ctx, NULL) < 0) {
        cerr << "Encoder thread " << info->config.thread_id << ": find stream failed" << endl;
        avformat_close_input(&dec_fmt_ctx);
        return NULL;
    }

    const AVCodec*  dec_codec = avcodec_find_decoder_by_name("jpeg_bm");
    if (!dec_codec) {
        cerr << "Encoder thread " << info->config.thread_id << ": decoder not found" << endl;
        avformat_close_input(&dec_fmt_ctx);
        return NULL;
    }

    dec_ctx = avcodec_alloc_context3(dec_codec);
    #ifdef BM_PCIE_MODE
        av_dict_set_int(&dec_dict, "zero_copy", info->config.zero_copy, 0);
        av_dict_set_int(&dec_dict, "sophon_idx", info->config.sophon_idx, 0);
    #endif

    if (avcodec_open2(dec_ctx, dec_codec, &dec_dict) < 0) {
        cerr << "Encoder thread " << info->config.thread_id << ": open decoder failed" << endl;
        avformat_close_input(&dec_fmt_ctx);
        avcodec_free_context(&dec_ctx);
        return NULL;
    }

    av_init_packet(&dec_pkt);

    if (av_read_frame(dec_fmt_ctx, &dec_pkt) < 0) {
        cerr << "Encoder thread " << info->config.thread_id << ": read frame failed" << endl;
        av_packet_unref(&dec_pkt);
    }

    ret = avcodec_send_packet(dec_ctx, &dec_pkt);

    frame = av_frame_alloc();
    ret = avcodec_receive_frame(dec_ctx, frame);

    av_packet_unref(&dec_pkt);

    const AVCodec* enc_codec = avcodec_find_encoder_by_name("jpeg_bm");
    if (!enc_codec) {
        cerr << "Encoder thread " << info->config.thread_id << ": encoder not found" << endl;
        avformat_close_input(&dec_fmt_ctx);
        avcodec_free_context(&dec_ctx);
        return NULL;
    }

    enc_ctx = avcodec_alloc_context3(enc_codec);
    if (enc_ctx == nullptr) {
        cerr << "Could not allocate video codec context!" << endl;
        return NULL;
    }

    enc_ctx->pix_fmt = AVPixelFormat(frame->format);
    enc_ctx->width   = frame->width;
    enc_ctx->height  = frame->height;
    enc_ctx->time_base = (AVRational){1, 25};
    enc_ctx->framerate = (AVRational){25, 1};

    int64_t value = info->config.zero_copy;
    av_dict_set_int(&enc_dict, "is_dma_buffer", value, 0);

    if (avcodec_open2(enc_ctx, enc_codec, &enc_dict) < 0) {
        cerr << "Encoder thread " << info->config.thread_id << ": open encoder failed" << endl;
        avcodec_free_context(&enc_ctx);
        avformat_close_input(&dec_fmt_ctx);
        avcodec_free_context(&dec_ctx);
        return NULL;
    }

    for (int loop = 0; loop < info->config.total_loops; ++loop) {
        av_init_packet(&enc_pkt);
        enc_pkt.data = NULL;
        enc_pkt.size = 0;

        ret = avcodec_send_frame(enc_ctx, frame);
        if (ret < 0) {
            cerr << "Encoder thread " << info->config.thread_id << ": send frame failed" << endl;
            av_packet_unref(&enc_pkt);
            break;
        }

        while (ret >= 0) {
            ret = avcodec_receive_packet(enc_ctx, &enc_pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                av_packet_unref(&enc_pkt);
                break;
            }

            if (loop == info->config.total_loops - 1) {
                char output_file[256];
                snprintf(output_file, sizeof(output_file),
                        "enc_t%d_final.jpg",
                        info->config.thread_id);

                FILE* fp = fopen(output_file, "wb");
                if (fp) {
                    fwrite(enc_pkt.data, 1, enc_pkt.size, fp);
                    fclose(fp);
                }
            }

            av_packet_unref(&enc_pkt);
        }
    }
    av_frame_free(&frame);

    avformat_close_input(&dec_fmt_ctx);
    avcodec_free_context(&dec_ctx);
    avcodec_free_context(&enc_ctx);
    return NULL;
}

int main(int argc, char* argv[])
{
    ThreadInfo threads[MAX_THREADS];
    int thread_count = 0;
    int mode = 1;
    int total_threads = 1;
    ThreadConfig base_cfg;

#ifdef BM_PCIE_MODE
    if (argc != 7) {
        cerr << "pcie mode:\n"
             << argv[0]
             << " input.jpg mode threads loops sophon_idx zero_copy" << endl;
        return -1;
    }

    strncpy(base_cfg.input_file, argv[1], 255);
    base_cfg.input_file[255] = '\0';
    base_cfg.total_loops = atoi(argv[4]);
    base_cfg.sophon_idx = atoi(argv[5]);
    base_cfg.zero_copy = atoi(argv[6]);
#else
    if (argc != 5) {
        cerr << "soc mode:\n"
             << argv[0]
             << " input.jpg mode threads loops" << endl;
        return -1;
    }

    strncpy(base_cfg.input_file, argv[1], 255);
    base_cfg.input_file[255] = '\0';
    base_cfg.total_loops = atoi(argv[4]);
    base_cfg.sophon_idx = 0;
    base_cfg.zero_copy = 0;
#endif

    mode = atoi(argv[2]);
    total_threads = atoi(argv[3]);

    if (mode < 1 || mode > 3) {
        cerr << "Invalid mode! (1/2/3)" << endl;
        return -1;
    }

    int decode_threads = 0, encode_threads = 0;
    switch (mode) {
    case 1:
        decode_threads = total_threads;
        break;
    case 2:
        encode_threads = total_threads;
        break;
    case 3:
        decode_threads = total_threads / 2 + 1;
        encode_threads = total_threads - decode_threads;
        break;
    }

    struct timeval tv1, tv2;
    unsigned int time;
    double fps;
    gettimeofday(&tv1, NULL);

    for (int i = 0; i < decode_threads; ++i) {
        threads[thread_count].config = base_cfg;
        threads[thread_count].config.thread_id = thread_count;
        pthread_create(&threads[thread_count].tid, NULL, decode_thread, &threads[thread_count]);
        thread_count++;
    }

    for (int i = 0; i < encode_threads; ++i) {
        threads[thread_count].config = base_cfg;
        threads[thread_count].config.thread_id = thread_count;
        pthread_create(&threads[thread_count].tid, NULL, encode_thread, &threads[thread_count]);
        thread_count++;
    }

    for (int i = 0; i < thread_count; ++i) {
        pthread_join(threads[i].tid, NULL);
    }

    gettimeofday(&tv2, NULL);
    time = (tv2.tv_sec - tv1.tv_sec)*1000 + (tv2.tv_usec - tv1.tv_usec)/1000;
    fps = double (total_threads * base_cfg.total_loops) / (double)time * 1000;

    printf("Total time: %d ms, Frame %d, FPS: %.0f\n", time, total_threads * base_cfg.total_loops, fps);
    cout << "All threads completed!" << endl;
    return 0;
}

