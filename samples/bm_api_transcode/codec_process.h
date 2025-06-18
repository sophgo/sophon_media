#ifndef __bm_VIDEO_CODEC_
#define __bm_VIDEO_CODEC_
#include "bm_vpudec_interface.h"
#include "bm_vpuenc_interface.h"
#ifdef __linux__
#include <sys/time.h>
#include <pthread.h>
#elif _WIN32
#include <windows.h>
#include <time.h>
#include "windows/libusb-1.0.18/examples/getopt/getopt.h"
#endif
#include "bmcv_api_ext_c.h"
#include "bmvpuenc.h"
#include "bmqueue.h"
#include "osal.h"

typedef struct {
    int soc_idx; /* only for pcie mode */

    int enc_fmt; /* 0: H.264, 1: H.265 */

    int pix_fmt; /* 0: yuv420, 1: nv12 */

    int crop_w;  /* actual width  */
    int crop_h;  /* actual height */

    int y_stride;
    int c_stride;

    int aligned_height;

    int bit_rate; /* kbps */
    int fps;  /*default is 30*/
    int cqp;

    int gop_preset;
    int intra_period;
} EncParameter;

typedef struct {
    BmVpuEncOpenParams open_params;
    BmVpuEncoder* video_encoder;

    BmVpuEncInitialInfo initial_info;

    BmVpuFramebuffer* src_fb_list;
    BmVpuEncDMABuffer* src_fb_dmabuffers;
    void*             frame_unused_queue;
    int num_src_fb;
    BmVpuFramebuffer* src_fb;

    BmVpuEncDMABuffer bs_dma_buffer;
    size_t bs_buffer_size;
    uint32_t bs_buffer_alignment;

    BmVpuEncParams enc_params;
    BmVpuRawFrame input_frame;
    BmVpuEncodedFrame output_frame;

    // FILE*  fin;
    FILE*  fout;
    int    soc_idx;  /* only for pcie mode */
    int    core_idx; /* the index of video encoder core */

    int    preset;   /* 0, 1, 2 */

    struct timeval tv_beg;
    struct timeval tv_end;

    int    first_pkt_recevie_flag;  // 1: have recv first pkt
} VpuEncContext;

typedef struct BMTestConfig_struct {
    // shared param
    int instanceNum;
    // pressure test num
    int press_num;
    // BmVpuDecLogLevel log_level;
    int log_level;
    char outputPath[MAX_FILE_PATH];
    char inputPath[MAX_FILE_PATH];
    char InterYuvPath[MAX_FILE_PATH];
    BOOL result;

    // dec param
    int streamFormat;
    int bsMode;

    int cbcr_interleave;
    int nv21;
    int wirteYuv;

    int picWidth;
    int picHeight;

    int extraFrame;

    int wtlFormat;
    int cmd_queue;
    int min_frame_cnt;
    int frame_delay;
    BMVidCodHandle vidCodHandle;

    unsigned char bStop;    /* replace bmvpu_dec_get_status */

    // enc param
    int loop;
    int frame_number;
    EncParameter enc;
} BMTestConfig;
// void *dec_send_process(void* arg);
// void *dec_get_process(void* arg);
void *enc_process(void* arg);
#endif