#include "ff_video_decode.h"
#include "ff_video_encode.h"
#include "ff_avframe_convert.h"
#include <queue>
#include <mutex>

extern "C"{
#include <sys/time.h>
#include <csignal>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
}

#define PCIE_MODE_ARG_NUM 5
#define SOC_MODE_ARG_NUM 3
#define PCIE_CARD_NUM 1
#define SET_ALIGNMENT 8
int g_exit_flag    = 0;
int g_thread_num   = 0;
#define MAX_THREAD_NUM 16
#define ENC_MAX_THREAD_NUM 1
#define MAX_QUEUE_FRAME 10
#define MAX_QUEUE_FRAME_5 10
#define INTERVAL 1

void *stat_pthread(void *arg);
void *video_decoder_pthread(void * arg);
void *video_encoder_pthread(void * arg);
void *video_process_pthread(void * arg);
 int  frame_index[MAX_THREAD_NUM]={0};
pthread_t dec_thread[MAX_THREAD_NUM];
pthread_t vpp_thread[MAX_THREAD_NUM];
pthread_t enc_thread[ENC_MAX_THREAD_NUM];
int wait_thread[MAX_THREAD_NUM] ={0};
bmcv_padding_attr_t  padding_arry[MAX_THREAD_NUM]=
{
    {
        .dst_crop_stx = 0,
        .dst_crop_sty = 0,
        .dst_crop_w = 480,
        .dst_crop_h = 270,

    },
    {
        .dst_crop_stx = 480,
        .dst_crop_sty = 0,
        .dst_crop_w = 480,
        .dst_crop_h = 270,

    },
    {
        .dst_crop_stx = 960,
        .dst_crop_sty = 0,
        .dst_crop_w = 480,
        .dst_crop_h = 270,

    },
    {
        .dst_crop_stx = 1440,
        .dst_crop_sty = 0,
        .dst_crop_w = 480,
        .dst_crop_h = 270,

    },
    {
        .dst_crop_stx = 0,
        .dst_crop_sty = 270,
        .dst_crop_w = 480,
        .dst_crop_h = 270,

    },
    {
        .dst_crop_stx = 480,
        .dst_crop_sty = 270,
        .dst_crop_w = 480,
        .dst_crop_h = 270,

    },
    {
        .dst_crop_stx = 960,
        .dst_crop_sty = 270,
        .dst_crop_w = 480,
        .dst_crop_h = 270,

    },
    {
        .dst_crop_stx = 1440,
        .dst_crop_sty = 270,
        .dst_crop_w = 480,
        .dst_crop_h = 270,

    },
    {
        .dst_crop_stx = 0,
        .dst_crop_sty = 540,
        .dst_crop_w = 480,
        .dst_crop_h = 270,

    },
    {
        .dst_crop_stx = 480,
        .dst_crop_sty = 540,
        .dst_crop_w = 480,
        .dst_crop_h = 270,

    },
    {
        .dst_crop_stx = 960,
        .dst_crop_sty = 540,
        .dst_crop_w = 480,
        .dst_crop_h = 270,

    },
    {
        .dst_crop_stx = 1440,
        .dst_crop_sty = 540,
        .dst_crop_w = 480,
        .dst_crop_h = 270,

    },
    {
        .dst_crop_stx = 0,
        .dst_crop_sty = 810,
        .dst_crop_w = 480,
        .dst_crop_h = 270,

    },
    {
        .dst_crop_stx = 480,
        .dst_crop_sty = 810,
        .dst_crop_w = 480,
        .dst_crop_h = 270,

    },
    {
        .dst_crop_stx = 960,
        .dst_crop_sty = 810,
        .dst_crop_w = 480,
        .dst_crop_h = 270,

    },
    {
        .dst_crop_stx = 1440,
        .dst_crop_sty = 810,
        .dst_crop_w = 480,
        .dst_crop_h = 270,

    },
};



pthread_t stat_thread;

bm_handle_t g_bmHandle        = {0};
std::queue<AVFrame*> g_image_vpp_queue[MAX_THREAD_NUM];
std::mutex g_vpp_queue_lock[MAX_THREAD_NUM];
std::queue<AVFrame*> g_image_enc_queue[1];
std::mutex g_enc_queue_lock[1];
std::mutex g_thread_num_lock;

bm_device_mem_t* g_watermark=NULL;

unsigned int count_dec[MAX_THREAD_NUM];
unsigned int count_enc;
unsigned int count_vpp;
float fps_dec[MAX_THREAD_NUM];
float fps_enc;
float fps_vpp;
VideoEnc_FFMPEG g_writer[1];
VideoDec_FFMPEG g_reader[MAX_THREAD_NUM];
int g_stop_flag[MAX_THREAD_NUM] = {0};
int g_enc_stop_flag = 0;

typedef struct MultiInstTest {
    int          pcie_mode;
    const char  *src_filename;
    const char  *output_filename;
    const char  *codecer_name;
    int          encode_pixel_format;
    int          sophon_idx;
    int          output_format_mode;
    int          pre_allocation_frame;
    int          is_by_filename;
    int          height;
    int          width;
    int          frame_rate;
    int          bitrate;
    int          is_dma_buffer;
    int          thread_index;
    int          thread_num;
    int          zero_copy;
    // unsigned int frame_nums[MAX_INST_NUM];
} THREAD_ARG;

static void usage(char *program_name);
void handler(int sig);


int main(int argc, char **argv)
{
    int arg_index = 0;
    unsigned int chipid = 0;
    int ret = 0;

    if(argc < 13){
        usage(argv[0]);
        return -1;
    }

    signal(SIGINT,handler);
    signal(SIGTERM,handler);
    THREAD_ARG *thread_arg = (THREAD_ARG *)malloc(sizeof(THREAD_ARG));

    memset(thread_arg,0,sizeof(THREAD_ARG));

    const char * platform               = argv[++arg_index];
    thread_arg->src_filename            = argv[++arg_index];
    thread_arg->output_filename         = argv[++arg_index];
    const char * ch_encode_pixel_format = argv[++arg_index];
    thread_arg->codecer_name            = argv[++arg_index];
    thread_arg->width                   = atoi(argv[++arg_index]);
    thread_arg->height                  = atoi(argv[++arg_index]);
    thread_arg->frame_rate              = atoi(argv[++arg_index]);
    thread_arg->bitrate                 = atoi(argv[++arg_index]);
    thread_arg->thread_num              = atoi(argv[++arg_index]);
    thread_arg->zero_copy               = atoi(argv[++arg_index]);

    thread_arg->width                   = 1920;
    thread_arg->height                  = 1080;
    g_thread_num = 16+16+1+1; /* dec+vpp+enc threads + stat thread */
    if (strcmp(platform, "pcie") == 0)
        thread_arg->pcie_mode = 1;
    else if (strcmp(platform, "soc") == 0)
        thread_arg->pcie_mode = 0;
    else {
        usage(argv[0]);
        return -1;
    }

    if(!thread_arg->src_filename || !ch_encode_pixel_format \
        || !thread_arg->output_filename || !thread_arg->codecer_name){
        usage(argv[0]);
        return -1;
    }

    if(thread_arg->height < 0 || thread_arg->width > 4096){
        usage(argv[0]);
        return -1;
    }

    if(thread_arg->width < 0 || thread_arg->width > 4096){
        usage(argv[0]);
        return -1;
    }

    if(thread_arg->frame_rate < 0 ){
        usage(argv[0]);
        return -1;
    }

    if(thread_arg->bitrate > 500 && thread_arg->bitrate <10000 ){
        thread_arg->bitrate = thread_arg->bitrate * 1000;
    }else{
        usage(argv[0]);
        return -1;
    }
    if(thread_arg->thread_num <= 0 ||  thread_arg->thread_num > MAX_THREAD_NUM){
        usage(argv[0]);
        return -1;
    }
    if (strcmp(ch_encode_pixel_format, "I420") == 0)
        thread_arg->encode_pixel_format = AV_PIX_FMT_YUV420P;
    else if (strcmp(ch_encode_pixel_format, "NV12") == 0)
        thread_arg->encode_pixel_format = AV_PIX_FMT_NV12;
    else {
        usage(argv[0]);
        return -1;
    }

    thread_arg->sophon_idx         = atoi(argv[++arg_index]);
    if (thread_arg->pcie_mode){
        if(thread_arg->sophon_idx < 0 && thread_arg->sophon_idx > 64){
            av_log(NULL, AV_LOG_ERROR, "ERROR Pcie mode: Invalid sophon_idx=%d \n", thread_arg->sophon_idx);
            usage(argv[0]);
            return -1;
        }
    }
    else{
        if(thread_arg->sophon_idx != 0)
            av_log(NULL, AV_LOG_WARNING, "WARNING Soc mode: Invalid sophon_idx=%d , it will be set to 0\n", thread_arg->sophon_idx);
        thread_arg->sophon_idx = 0;
        if(thread_arg->zero_copy != 0)
            av_log(NULL, AV_LOG_WARNING, "WARNING Soc mode: Invalid zero_copy=%d , it will be set to 0\n", thread_arg->zero_copy);

        thread_arg->sophon_idx = 0;
        thread_arg->zero_copy = 0;
    }

    ret = bm_dev_request(&g_bmHandle, thread_arg->sophon_idx);
    if (ret != BM_SUCCESS){
        av_log(NULL, AV_LOG_DEBUG, "bm_dev_request failed !\n");
        return -1;
    }
    bm_get_chipid(g_bmHandle, &chipid);

    int td_index = 0;
    ret = pthread_create(&(enc_thread[0]), NULL, video_encoder_pthread, thread_arg);
    if (ret != 0) {
        av_log(NULL, AV_LOG_ERROR, "video_encoder pthread[%d] create failed \n", td_index);
        return -1;
    }
    usleep(100);
    /* Initialize multiple threads */
    while( thread_arg->thread_num ){
        thread_arg->thread_index = td_index;
        ret = pthread_create(&(dec_thread[td_index]), NULL, video_decoder_pthread, thread_arg);
        if (ret != 0) {
            av_log(NULL, AV_LOG_ERROR, "video_decoder pthread[%d] create failed \n", td_index);
            return -1;
        }
        usleep(100);
        ret = pthread_create(&(vpp_thread[td_index]), NULL, video_process_pthread, thread_arg);
        if (ret != 0) {
            av_log(NULL, AV_LOG_ERROR, "video_process pthread[%d] create failed \n", td_index);
            return -1;
        }


        usleep(10000);
        td_index++;
        thread_arg->thread_index = td_index;
        thread_arg->thread_num--;
    }



    thread_arg->thread_num = td_index;
    ret = pthread_create(&(stat_thread), NULL, stat_pthread, thread_arg);
    if (ret != 0) {
        av_log(NULL, AV_LOG_ERROR, "stat pthread create failed \n");
        return -1;
    }

    while(1){
        bool exit_all = true;
        if(g_thread_num != 0){
            exit_all = false;
        }

        if(exit_all){
            g_exit_flag = 1;
            free(thread_arg);
            thread_arg = NULL;
            handler(0);
            break;
        }
        usleep(1000 * 5);
    }

    return 0;
}

void *video_decoder_pthread(void *arg){
    THREAD_ARG *thread_arg      = (THREAD_ARG *)arg;
    int index                   = thread_arg->thread_index;
    const char *src_filename    = thread_arg->src_filename;
    int zero_copy               = thread_arg->zero_copy;
    int sophon_idx              = thread_arg->sophon_idx;
    int encode_pixel_format     = thread_arg->encode_pixel_format;
    int output_format_mode      = 101;

    int ret                     =  0;
    char file_name[256]         = {0};
    struct timeval tv1, tv2;
    float time;
    count_dec[index] = 0;

    if (encode_pixel_format != AV_PIX_FMT_YUV420P)
        output_format_mode = 0;

    VideoDec_FFMPEG* reader = &g_reader[index];
    ret = reader->openDec(src_filename,1,NULL,output_format_mode,sophon_idx,zero_copy);
    if(ret < 0 )
    {
        av_log(NULL, AV_LOG_ERROR, "open input media failed\n");
        return (void *)-1;
    }

    gettimeofday(&tv1, NULL);
    while(!g_exit_flag){
        int got_frame = 0;
        AVFrame *frame = av_frame_alloc();
        got_frame = reader->grabFrame2(frame);
        if(!got_frame){
            av_frame_unref(frame);
            av_frame_free(&frame);
            break;
        }

        while ((g_image_vpp_queue[index].size() == MAX_QUEUE_FRAME) && (!g_exit_flag))
        {
            usleep(100);
        }
        g_vpp_queue_lock[index].lock();


        g_image_vpp_queue[index].push(frame);
        g_vpp_queue_lock[index].unlock();
        count_dec[index]++;

        if ((count_dec[index]+1) % 100 == 0){
            gettimeofday(&tv2, NULL);
            time = (tv2.tv_sec - tv1.tv_sec)*1000 + (tv2.tv_usec - tv1.tv_usec)/1000;
            fps_dec[index] = (float)count_dec[index]*1000/time;
        }
    }
    /* fflush */
    while(!g_exit_flag){
        ret = 0;
        AVFrame *frame = av_frame_alloc();;
        ret = reader->flushFrame(frame);
        if(ret < 0){
            av_frame_unref(frame);
            av_frame_free(&frame);
            break;
        }

        while ((g_image_vpp_queue[index].size() == MAX_QUEUE_FRAME) && (!g_exit_flag))
        {
            usleep(10);
        }
        g_vpp_queue_lock[index].lock();
        g_image_vpp_queue[index].push(frame);
        g_vpp_queue_lock[index].unlock();
        count_dec[index]++;
    }

    g_thread_num_lock.lock();
    g_thread_num--;
    g_stop_flag[index] = 1;
    g_thread_num_lock.unlock();

    if (!(reader->isClosed()))
        reader->closeDec();

    av_log(NULL, AV_LOG_INFO, "video decode finish!\n");
    return (void *)0;
}

void *video_process_pthread(void *arg){
    THREAD_ARG *thread_arg      = (THREAD_ARG *)arg;
    int index                   = thread_arg->thread_index;
    int height                  = thread_arg->height;
    int width                   = thread_arg->width;
    int encode_pixel_format     = thread_arg->encode_pixel_format;
    int sophon_idx              = thread_arg->sophon_idx;
    int loop_count              =  0;
    int ret                     =  0;
    struct timeval tv1, tv2;
    float time;

    count_vpp = 0;

    AVFrame* in_frame;
    AVFrame* out_frame;

    if(encode_pixel_format == AV_PIX_FMT_YUV420P){
        width  = (width + SET_ALIGNMENT - 1) & ~(SET_ALIGNMENT - 1);
        height = (height + SET_ALIGNMENT - 1) & ~(SET_ALIGNMENT - 1);
    }
    gettimeofday(&tv1, NULL);
    while(!g_exit_flag){
        while(g_image_vpp_queue[index].empty())
        {
            if (g_stop_flag[index] == 1)
            {
               goto cleanup_vpp;
            }
            if (g_exit_flag) break;
            usleep(10);
            loop_count++;
        }

        if (g_exit_flag)
            break;

        g_vpp_queue_lock[index].lock();
        in_frame = g_image_vpp_queue[index].front();

        g_image_vpp_queue[index].pop();
        g_vpp_queue_lock[index].unlock();


        if(encode_pixel_format == AV_PIX_FMT_YUV420P){
            out_frame = av_frame_alloc();
            ret=AVFrameConvert(g_bmHandle, in_frame, out_frame, height, width, encode_pixel_format,padding_arry,index);

            if(ret != 0) {
                av_frame_unref(out_frame);
                av_frame_free(&out_frame);
                continue;
            }
        }


        /* push image to enc queue */
        while ((g_image_enc_queue[0].size() >= MAX_QUEUE_FRAME_5 ) && (!g_exit_flag)){ //
            usleep(10);
        }

        g_enc_queue_lock[0].lock();
        if(encode_pixel_format == AV_PIX_FMT_YUV420P){
            g_image_enc_queue[0].push(out_frame);
        }
        else
            g_image_enc_queue[0].push(in_frame);
        g_enc_queue_lock[0].unlock();

        count_vpp++;
        if ((count_vpp+1) % 10 == 0){
            -(&tv2, NULL);
            time = (tv2.tv_sec - tv1.tv_sec)*1000 + (tv2.tv_usec - tv1.tv_usec)/1000;
            fps_vpp = count_vpp*1000/time;
        }

    }

cleanup_vpp:


    g_thread_num_lock.lock();
    g_thread_num--;
    g_enc_stop_flag ++;
    printf("g_enc_stop_flag=%d\n", g_enc_stop_flag);
    g_thread_num_lock.unlock();
    av_log(NULL, AV_LOG_INFO, "video_process finish!\n");
    return (void *)0;
}

void *video_encoder_pthread(void *arg){
    THREAD_ARG *thread_arg      = (THREAD_ARG *)arg;
    const char *output_filename = thread_arg->output_filename;
    int encode_pixel_format     = thread_arg->encode_pixel_format;
    int frame_rate              = thread_arg->frame_rate;
    int bitrate                 = thread_arg->bitrate;
    int sophon_idx              = thread_arg->sophon_idx;
    int index                   = 0;
    VideoEnc_FFMPEG *writer = &g_writer[index];
    AVFrame *frame;

    int ret                     =  0;
    char file_name[256]         = {0};
    char file_name_tmp[256]     = {0};
    const char *codecer_name    = thread_arg->codecer_name;

    strcpy(file_name_tmp, output_filename);
    const char *name_start  = strtok(file_name_tmp, ".");
    const char *name_end    = strrchr(output_filename, 46);//ascii '.' = 46
    sprintf(file_name,"%s%d%s",name_start,index,name_end);
    struct timeval tv1, tv2;
    float time;
    count_enc = 0;

    gettimeofday(&tv1, NULL);
    while(!g_exit_flag){
        while(g_image_enc_queue[index].empty())
        {
            if (g_enc_stop_flag == 16){
                goto cleanup;
            }
            if (g_exit_flag) break;
            usleep(100);
        }
        if (g_exit_flag)
            break;

        g_enc_queue_lock[index].lock();
        frame = g_image_enc_queue[index].front();

        g_image_enc_queue[index].pop();
        g_enc_queue_lock[index].unlock();

        if (writer->isClosed()){
            ret = writer->openEnc(file_name, codecer_name, 0, frame_rate, 1920, 1080, encode_pixel_format, bitrate, sophon_idx);
            if (ret !=0 ) {
                av_log(NULL, AV_LOG_ERROR,"writer.openEnc failed\n");
                goto cleanup;
            }
        }

        writer->writeFrame(frame);
        if (frame)
        {
            av_frame_unref(frame);
            av_frame_free(&frame);
        }
        count_enc++;

        if ((count_enc+1) % 10 == 0){
            gettimeofday(&tv2, NULL);
            time = (tv2.tv_sec - tv1.tv_sec)*1000 + (tv2.tv_usec - tv1.tv_usec)/1000;
            fps_enc = (float)count_enc*1000/time;
        }
    }

cleanup:

    if (!writer->isClosed())
        writer->closeEnc();
    g_thread_num_lock.lock();
    g_thread_num--;
    g_thread_num_lock.unlock();
    av_log(NULL, AV_LOG_INFO, "encode finish!\n");
    return (void *)0;
}

static void usage(char *program_name)
{
    av_log(NULL, AV_LOG_ERROR, "Usage: \n\t%s [platform] [src_filename] [output_filename] [encode_pixel_format] [codecer_name] [width] [height] [frame_rate] [bitrate] [thread_num] [zero_copy] [sophon_idx] <optional: enable_mosaic> <optional: enable_watermark>\n", program_name);
    av_log(NULL, AV_LOG_ERROR, "\t[platform]                platform: soc or pcie \n");
    av_log(NULL, AV_LOG_ERROR, "\t[src_filename]            input file name x.mp4 x.ts...\n");
    av_log(NULL, AV_LOG_ERROR, "\t[output_filename]         encode output file name x.mp4,x.ts...\n");
    av_log(NULL, AV_LOG_ERROR, "\t[encode_pixel_format]     encode format I420.\n");
    av_log(NULL, AV_LOG_ERROR, "\t[encoder_name]            encode h264_bm,h265_bm.\n");
    av_log(NULL, AV_LOG_ERROR, "\t[width]                   encode 32<width<=4096.\n");
    av_log(NULL, AV_LOG_ERROR, "\t[height]                  encode 32<height<=4096.\n");
    av_log(NULL, AV_LOG_ERROR, "\t[frame_rate]              encode frame_rate.\n");
    av_log(NULL, AV_LOG_ERROR, "\t[bitrate]                 encode bitrate 500 < bitrate < 10000\n");
    av_log(NULL, AV_LOG_ERROR, "\t[thread_num]              thread num.\n");
    av_log(NULL, AV_LOG_ERROR, "\t[zero_copy ]              PCie platform: 0: copy host mem, 1: nocopy.\n");
    av_log(NULL, AV_LOG_ERROR, "\t                          SoC  platform: any number is acceptable, but it is invalid\n");
    av_log(NULL, AV_LOG_ERROR, "\t[sophon_idx]              PCie platform: sophon devices idx\n");
    av_log(NULL, AV_LOG_ERROR, "\t                          SoC  platform: this option set 0, other number is invalid\n");
    av_log(NULL, AV_LOG_ERROR, "\tOptional:                                                                          \n");
    av_log(NULL, AV_LOG_ERROR, "\t<enable_mosaic>           Optional, add mosaic on the left_top corner, only bm1686 support now and [encode_pixel_format] need I420 \n");
    av_log(NULL, AV_LOG_ERROR, "\t<enable_watermark>        Optional, add watermark on video, only bm1686 support now and [encode_pixel_format] need I420 \n");
    av_log(NULL, AV_LOG_ERROR, "\tPCIE Mode example: \n");
    av_log(NULL, AV_LOG_ERROR, "\t%s pcie example.mp4 test.ts I420 h264_bm 800 400 25 3000 3 0 0\n", program_name);
    av_log(NULL, AV_LOG_ERROR, "\tSOC Mode example: \n");
    av_log(NULL, AV_LOG_ERROR, "\t%s soc example.mp4 test.ts I420 h264_bm 800 400 25 3000 3 0 0\n", program_name);
}

void handler(int sig)
{
    int i = 0;

    g_exit_flag = 1;
    av_log(NULL, AV_LOG_INFO, "signal %d is received! \n", sig);
    av_log(NULL, AV_LOG_INFO, "program will exited \n");
    sleep(1);

    /* wait thread quit */
    int try_count = 50;
    while (try_count--){
        bool exit_all = true;
        for (int i = 0; i < MAX_THREAD_NUM; i++) {
            if(g_thread_num){
                exit_all = false;
            }
        }
        if(exit_all){
            break;
        }

        usleep(100);
    }

    if (try_count<=0){
        pthread_cancel(stat_thread);
        for (int i = 0; i < MAX_THREAD_NUM; i++) {
            pthread_cancel(dec_thread[i]);
            pthread_cancel(dec_thread[i]);
            pthread_cancel(dec_thread[i]);
        }
    }
    if (!g_writer[0].isClosed())
            g_writer[0].closeEnc();
    for (int i = 0; i < MAX_THREAD_NUM; i++) {

        if (!g_reader[i].isClosed())
            g_reader[i].closeDec();
    }

    if(g_bmHandle){
        bm_dev_free(g_bmHandle);
    }

    exit(0);
}

void* stat_pthread(void *arg)
{
    THREAD_ARG *thread_arg      = (THREAD_ARG *)arg;
    int thread_num = thread_arg->thread_num;

    uint64_t last_count_dec[MAX_THREAD_NUM] = {0};
    uint64_t last_count_enc = 0;
    uint64_t last_count_vpp = 0;
    uint64_t last_count_sum = 0;

    int dis_mode = 0;
    char *display = getenv("DISPLAY_FRAMERATE");
    if (display) dis_mode = atoi(display);
    while(!g_exit_flag && g_thread_num!=1)
    {
        sleep(INTERVAL);
        if (dis_mode == 1) {
            for (int i = 0; i < thread_num; i++) {
                printf("ID[%d] ,DEC_FRM[%10lld], DEC_FPS[%2.2f],[%2.2f] | VPP_FRM[%10lld], VPP_FPS[%2.2f],[%2.2f]| ENC_FRM[%10lld], ENC_FPS[%2.2f],[%2.2f], VPP_QUEUE[%d] ENC_QUEUE[%d]\n",
                    i, (long long)count_dec[i],((double)(count_dec[i]-last_count_dec[i]))/INTERVAL, fps_dec[i],
                       (long long)count_vpp,((double)(count_vpp-last_count_vpp))/INTERVAL, fps_vpp,
                       (long long)count_enc, ((double)(count_enc-last_count_enc))/INTERVAL, fps_enc, g_image_vpp_queue[i].size(), g_image_enc_queue[0].size());
                last_count_dec[i] = count_dec[i];
                last_count_vpp = count_vpp;
                last_count_enc = count_enc;
            }
        }
        else {
            uint64_t count_sum = 0;
            // for (int i = 0; i < thread_num; i++)
               count_sum = count_enc;
            printf("thread %d, frame %lld, enc_fps %2.2f", thread_num, count_enc, ((double)(count_sum-last_count_sum))/INTERVAL);
            last_count_sum = count_sum;
        }
        printf("\r");
        fflush(stdout);
    }
    fflush(stdout);
    g_thread_num_lock.lock();
    g_thread_num--;
    g_thread_num_lock.unlock();
    return NULL;
}

