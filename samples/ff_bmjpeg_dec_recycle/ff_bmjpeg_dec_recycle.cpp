#include <iostream>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libavutil/file.h"
#include "libavutil/pixfmt.h"
}
#include "bmlib_runtime.h"


using namespace std;

typedef struct {
    uint8_t* start;
    int      size;
    int      pos;
} bs_buffer_t;


static int read_buffer(void *opaque, uint8_t *buf, int buf_size);
static int saveYUV(AVFrame* pFrame, int iIndex, string filename, uint8_t* data);

static int read_buffer(void *opaque, uint8_t *buf, int buf_size)
{
    bs_buffer_t* bs = (bs_buffer_t*)opaque;

    int r = bs->size - bs->pos;
    if (r <= 0) {
        cout << "EOF of AVIO." << endl;
        return AVERROR_EOF;
    }

    uint8_t* p = bs->start + bs->pos;
    int len = (r >= buf_size) ? buf_size : r;
    memcpy(buf, p, len);

    //cout << "read " << len << endl;

    bs->pos += len;

    return len;
}

static int saveYUV(AVFrame* pFrame, int iIndex, string filename, uint8_t* data)
{
    uint8_t * pY;
    uint8_t * pU;
    uint8_t * pV;
    string yuv_filename = filename + "-" + to_string(iIndex) + ".yuv";
    FILE *fp = fopen(yuv_filename.c_str(), "wb+");
    if (fp == nullptr) {
        cerr << "create yuv file failed:" <<  yuv_filename << endl;
        return -1;
    }

    if (data == nullptr) {
        pY = pFrame->data[0];
        pU = pFrame->data[1];
        pV = pFrame->data[2];
    }
    else {
        pY = data;
        pU = pY + (pFrame->data[1] - pFrame->data[0]);
        pV = pU + (pFrame->data[2] - pFrame->data[1]);
    }

    int frameWidth  = pFrame->width;
    int frameHeight = pFrame->height;
    int y_stride = pFrame->linesize[0];  // may be larger than width because of alignment
    int u_stride = pFrame->linesize[1];
    int v_stride = pFrame->linesize[2];

    cout << "Y address: " << static_cast<const void *>(pY) << " U address: " << static_cast<const void *>(pU) << " V address: " << static_cast<const void *>(pV) << endl;
    cout << "frame width: " << frameWidth << " frame height: " << frameHeight << endl;
    cout << "Y num: " << y_stride << " U num: " << u_stride << " V num: " << v_stride<< endl;

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

void print_usage(int argc, char* argv[])
{
#ifdef BM_PCIE_MODE
    cerr << "Usage: " << argv[0] << " <sophon_idx> <zero_copy> <bitstream_size> <framebuffer_size> <pic_num> input ..." << endl;
    cerr << "\t<sophon_idx>: sophon device index, range: [0, 63]" << endl;
    cerr << "\t<zero_copy>: if copy the decoded image back to CPU, [0: copy, 1: no copy]" << endl;
#else
    cerr << "Usage: " << argv[0] << " <bitstream_size> <framebuffer_size> <pic_num> input ..." << endl;
#endif
    cerr << "\t<bitstream_size>: input bitstream buffer size, should be no less than the max size of input files" << endl;
    cerr << "\t<framebuffer_size>: output frame buffer size, should be no less than the max size of decoded image data" << endl;
}

int main(int argc, char* argv[])
{
    const AVInputFormat *iformat = nullptr;
    AVFormatContext *pFormatCtx = nullptr;
    AVCodecContext *dec_ctx = nullptr;
    const AVCodec *pCodec = nullptr;
    AVDictionary *dict = nullptr;
    AVIOContext *avio_ctx = nullptr;
    AVFrame *pFrame = nullptr;
    AVPacket pkt;
    int got_picture;
    FILE *infile;
    int numBytes;
    uint8_t *aviobuffer = nullptr;
    int aviobuf_size = 32*1024; // 32K
    uint8_t *bs_buffer = nullptr;
    bs_buffer_t bs_obj = {0, 0, 0};
    int ret = 0;
    int offset = 0;
    string input_name;
    string output_file_name = "recycle-output";
    bm_handle_t handle;
    unsigned long long p_vaddr = 0;
    uint8_t *virt_addr = nullptr;

#ifdef BM_PCIE_MODE
    int zero_copy = 0;
    int sophon_idx = 0;
#endif
    int bs_size = 0;
    int fb_size = 0;
    int pic_num = 0;

#ifdef BM_PCIE_MODE
    if (argc < 7) {
        print_usage(argc, argv);
        return -1;
    }

    sophon_idx = atoi(argv[1]);
    if (sophon_idx < 0 || sophon_idx >= 64)
    {
        cerr << "Error: sophon_idx = " << sophon_idx << " , must be 0-63" << endl;
        print_usage(argc, argv);
        return -1;
    }

    zero_copy = atoi(argv[2]);
    if (zero_copy != 0 && zero_copy != 1)
    {
        cerr << "Error: zero_copy = " << zero_copy << " , must be 0 or 1" << endl;
        print_usage(argc, argv);
        return -1;
    }

    bs_size = atoi(argv[3]);
    if (bs_size <= 0)
    {
        cerr << "Error: bs_size = " << bs_size << " , must be greater than 0" << endl;
        print_usage(argc, argv);
        return -1;
    }

    fb_size = atoi(argv[4]);
    if (fb_size <= 0)
    {
        cerr << "Error: fb_size = " << fb_size << " , must be greater than 0" << endl;
        print_usage(argc, argv);
        return -1;
    }

    pic_num = atoi(argv[5]);
    if (pic_num <= 0)
    {
        cerr << "Error: pic_num = " << pic_num << " , must be greater than 0" << endl;
        print_usage(argc, argv);
        return -1;
    }

    output_file_name += "-sophon";
    output_file_name += to_string(sophon_idx);
    offset = 6;
#else
    if (argc < 5) {
        print_usage(argc, argv);
        return -1;
    }

    bs_size = atoi(argv[1]);
    if (bs_size <= 0)
    {
        cerr << "Error: bs_size = " << bs_size << " , must be greater than 0" << endl;
        print_usage(argc, argv);
        return -1;
    }

    fb_size = atoi(argv[2]);
    if (fb_size <= 0)
    {
        cerr << "Error: fb_size = " << fb_size << " , must be greater than 0" << endl;
        print_usage(argc, argv);
        return -1;
    }

    pic_num = atoi(argv[3]);
    if (pic_num <= 0)
    {
        cerr << "Error: pic_num = " << pic_num << " , must be greater than 0" << endl;
        print_usage(argc, argv);
        return -1;
    }

    offset = 4;
#endif
    pFormatCtx = avformat_alloc_context();
    /* HW JPEG decoder: jpeg_bm */
    pCodec = avcodec_find_decoder_by_name("jpeg_bm");
    if (pCodec == nullptr) {
        cerr << "Codec not found." << endl;
        ret = AVERROR_DECODER_NOT_FOUND;
        goto Func_Exit;
    }

    dec_ctx = avcodec_alloc_context3(pCodec);
    if (dec_ctx == nullptr) {
        cerr << "Could not allocate video codec context!" << endl;
        ret = AVERROR(ENOMEM);
        goto Func_Exit;
    }

    /* mjpeg demuxer */
    iformat = av_find_input_format("mjpeg");
    if (iformat == nullptr) {
        cerr << "av_find_input_format failed." << endl;
        ret = AVERROR_DEMUXER_NOT_FOUND;
        goto Func_Exit;
    }

    pFrame = av_frame_alloc();
    if (pFrame == nullptr) {
        cerr << "av frame malloc failed" << endl;
        goto Func_Exit;
    }

    // enable to unref frame by user
    // dec_ctx->refcounted_frames = 1;

#ifdef BM_PCIE_MODE
    ret = bm_dev_request(&handle, sophon_idx);
#else
    ret = bm_dev_request(&handle, 0);
#endif
    if (ret != 0)
    {
        cerr << "bmlib create handle failed." << endl;
        ret = AVERROR_UNKNOWN;
        goto Func_Exit;
    }

    /* Set parameters for jpeg_bm decoder */
    /* The output of bm jpeg decoder is chroma-separated,for example, YUVJ420P */
    av_dict_set_int(&dict, "chroma_interleave", 0, 0);
    /* The bitstream buffer size (KB) */
#define BS_MASK (1024*16-1)
    bs_size = (bs_size+BS_MASK)&(~BS_MASK); /* in unit of 16k */
#undef  BS_MASK
#define JPU_PAGE_UNIT_SIZE 256 /* each page unit of jpu is 256 byte */
    /* Avoid the false alarm that bs buffer is empty (SA3SW-252) */
    if (bs_size - numBytes < JPU_PAGE_UNIT_SIZE)
        bs_size += 16*1024;  /* in unit of 16k */
#undef JPU_PAGE_UNIT_SIZE
    bs_size /= 1024;
    cout << "bs buffer size: " << bs_size << "K" << endl;
    av_dict_set_int(&dict, "bs_buffer_size", bs_size, 0);
    /* Extra frame buffers: "0" for still jpeg, at least "2" for mjpeg */
    av_dict_set_int(&dict, "num_extra_framebuffers", 0, 0);
#ifdef BM_PCIE_MODE
    av_dict_set_int(&dict, "zero_copy", zero_copy, 0);
    av_dict_set_int(&dict, "sophon_idx", sophon_idx, 0);
#endif
    av_dict_set_int(&dict, "framebuffer_recycle", 1, 0);
    cout << "framebuffer size: " << fb_size << endl;
    av_dict_set_int(&dict, "framebuffer_size", fb_size, 0);

    ret = avcodec_open2(dec_ctx, pCodec, &dict);
    if (ret < 0) {
        cerr << "Could not open codec." << endl;
        ret = AVERROR_UNKNOWN;
        goto Func_Exit;
    }

    for (int i = 0; i < pic_num; i++) {
        input_name = argv[i+offset];
        auto pos1 = input_name.find(".jpg");
        auto pos2 = input_name.find(".jpeg");
        if (pos1 == string::npos && pos2 == string::npos) {
            cerr << "The input file is invalid jpeg file: " << input_name << endl;
            return -1;
        }

        infile = fopen(input_name.c_str(), "rb+");
        if (infile == nullptr) {
            cerr << "open file1 failed" << endl;
            goto Func_Exit;
        }

        fseek(infile, 0, SEEK_END);
        numBytes = ftell(infile);
        cout << "infile size: " << numBytes << endl;
        fseek(infile, 0, SEEK_SET);

        bs_buffer = (uint8_t *)av_malloc(numBytes);
        if (bs_buffer == nullptr) {
            cerr << "av_malloc for bs buffer failed" << endl;
            goto Func_Exit;
        }

        fread(bs_buffer, sizeof(uint8_t), numBytes, infile);
        fclose(infile);
        infile = nullptr;

        aviobuffer = (uint8_t *)av_malloc(aviobuf_size); //32k
        if (aviobuffer == nullptr) {
            cerr << "av_malloc for avio failed" << endl;
            goto Func_Exit;
        }

        bs_obj.start = bs_buffer;
        bs_obj.size  = numBytes;
        bs_obj.pos   = 0;
        avio_ctx = avio_alloc_context(aviobuffer, aviobuf_size, 0,
                                    (void*)(&bs_obj), read_buffer, nullptr, nullptr);
        if (avio_ctx == nullptr)
        {
            cerr << "avio_alloc_context failed" << endl;
            ret = AVERROR(ENOMEM);
            goto Func_Exit;
        }

        pFormatCtx->pb = avio_ctx;

        /* Open an input stream */
        ret = avformat_open_input(&pFormatCtx, nullptr, iformat, nullptr);
        if (ret != 0) {
            cerr << "Couldn't open input stream." << endl;
            goto Func_Exit;
        }

        while (av_read_frame(pFormatCtx, &pkt) >= 0) {
            ret = avcodec_send_packet(dec_ctx, &pkt);
            if (ret != 0) {
                cerr << "avcodec_send_packet error." << endl;
                goto Func_Exit;
            }

            ret = avcodec_receive_frame(dec_ctx, pFrame);
            if (ret != 0) {
                cerr << "avcodec_receive_frame error." << endl;
                goto Func_Exit;
            }

            cout << "pFrame->data[0] = " << static_cast<const void *>(pFrame->data[0]) << endl;
            cout << "pFrame->linesize[0] = " << pFrame->linesize[0] << endl;
            cout << "pFrame->data[1] = " << static_cast<const void *>(pFrame->data[1]) << endl;
            cout << "pFrame->linesize[1] = " << pFrame->linesize[1] << endl;
            cout << "pFrame->data[2] = " << static_cast<const void *>(pFrame->data[2]) << endl;
            cout << "pFrame->linesize[2] = " << pFrame->linesize[2] << endl;
            cout << "pFrame->data[3] = " << static_cast<const void *>(pFrame->data[3]) << endl;
            cout << "pFrame->linesize[3] = " << pFrame->linesize[3] << endl;
            cout << "pFrame->data[4] = " << static_cast<const void *>(pFrame->data[4]) << endl;
            cout << "pFrame->linesize[4] = " << pFrame->linesize[4] << endl;
            cout << "pFrame->data[5] = " << static_cast<const void *>(pFrame->data[5]) << endl;
            cout << "pFrame->linesize[5] = " << pFrame->linesize[5] << endl;
            cout << "pFrame->data[6] = " << static_cast<const void *>(pFrame->data[6]) << endl;
            cout << "pFrame->linesize[6] = " << pFrame->linesize[6] << endl;
            cout << "pFrame->data[7] = " << static_cast<const void *>(pFrame->data[7]) << endl;
            cout << "pFrame->linesize[7] = " << pFrame->linesize[7] << endl;

            // save YUV data
        #ifdef BM_PCIE_MODE
            if (zero_copy) {
                virt_addr = (uint8_t *)malloc(fb_size);
                bm_device_mem_t dma_buffer = bm_mem_from_device((long long unsigned int)pFrame->data[4], fb_size);
                ret = bm_memcpy_d2s_partial(handle, virt_addr, dma_buffer, fb_size);
                if (ret != 0) {
                    cerr << "Failed to call bm_memcpy_d2s_partial." << endl;
                    goto Func_Exit;
                }
            }
        #endif
            ret = saveYUV(pFrame, i, output_file_name.c_str(), virt_addr);
            if (ret < 0) {
                break;
            }

            cout << "pixel format: " << pFrame->format << endl;
            cout << "frame width : " << pFrame->width  << endl;
            cout << "frame height: " << pFrame->height << endl;

            av_frame_unref(pFrame);
            av_packet_unref(&pkt);  // free mem because av_read_frame had applied for mem
            av_init_packet(&pkt);
        }

        av_packet_unref(&pkt);

        if (avio_ctx) {
            av_freep(&avio_ctx->buffer);
            av_freep(&avio_ctx);
            avio_ctx = nullptr;
        }
    }

Func_Exit:
    av_packet_unref(&pkt);

    if (pFrame) {
        av_frame_free(&pFrame);
    }

    avformat_close_input(&pFormatCtx);

    if (avio_ctx) {
        av_freep(&avio_ctx->buffer);
        av_freep(&avio_ctx);
    }

    if (infile) {
        fclose(infile);
    }

    if (dict) {
        av_dict_free(&dict);
    }

    if (virt_addr) {
        av_free(virt_addr);
    }

    if (handle) {
        bm_dev_free(handle);
    }

    if (dec_ctx) {
        avcodec_close(dec_ctx);
    }

    if (bs_buffer) {
        av_free(bs_buffer);
    }

    return 0;
}

