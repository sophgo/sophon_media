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

int g_enable_mosaic    = 0;    /* enable mosaic process (only bm1686 support) */
int g_enable_watermark = 0;    /* enable watermark process (only bm1686 support) */

#define MAX_THREAD_NUM 18
#define PCIE_MODE_ARG_NUM 5
#define SOC_MODE_ARG_NUM 3
#define PCIE_CARD_NUM 1
#define SET_ALIGNMENT 8

int g_quit_flag    = 0;
int g_thread_num   = 0;
#define MAX_QUEUE_FRAME_3 3
#define MAX_QUEUE_FRAME_5 5
#define INTERVAL 1

void *stat_pthread(void *arg);
void *video_decoder_pthread(void * arg);
void *video_encoder_pthread(void * arg);
void *video_process_pthread(void * arg);
pthread_t stat_thread;
pthread_t dec_thread[MAX_THREAD_NUM];
pthread_t vpp_thread[MAX_THREAD_NUM];
pthread_t enc_thread[MAX_THREAD_NUM];

bm_handle_t g_bmHandle        = {0};
std::queue<AVFrame*> g_image_vpp_queue[MAX_THREAD_NUM];
std::queue<AVFrame*> g_image_enc_queue[MAX_THREAD_NUM];

std::mutex g_vpp_queue_lock[MAX_THREAD_NUM];
std::mutex g_enc_queue_lock[MAX_THREAD_NUM];
std::mutex g_thread_num_lock;

bm_device_mem_t* g_watermark=NULL;

unsigned int count_dec[MAX_THREAD_NUM];
unsigned int count_enc[MAX_THREAD_NUM];
unsigned int count_vpp[MAX_THREAD_NUM];
float fps_dec[MAX_THREAD_NUM];
float fps_enc[MAX_THREAD_NUM];
float fps_vpp[MAX_THREAD_NUM];

VideoEnc_FFMPEG g_writer[MAX_THREAD_NUM];
VideoDec_FFMPEG g_reader[MAX_THREAD_NUM];
int g_stop_flag[MAX_THREAD_NUM] = {0};
typedef struct MultiInstTest {
    int         pcie_mode;
    const char  *src_filename;
    const char  *decoder_name;
    const char  *output_filename;
    const char  *codecer_name;
    int          sophon_idx;
    int          height;
    int          width;
    int          frame_rate;
    int          bitrate;
    int          is_dma_buffer;
    int          thread_index;
    int          thread_num;
    int          en_bmx264;
    int          zero_copy;
} THREAD_ARG;
THREAD_ARG *thread_arg = NULL;

static void usage(char *program_name);
void handler(int sig);
void *startOneInst(void *arg);
int find_watermark_path(char watermark_path[512]);

int main(int argc, char **argv)
{
    int ret = 0;
    int arg_index = 0;
    int output_format_mode = 101;
    unsigned int chipid = 0;

    if(argc < 13){
        av_log(NULL, AV_LOG_ERROR, "Missing command line argument \n");
        usage(argv[0]);
        return -1;
    }

    signal(SIGINT,handler);
    signal(SIGTERM,handler);
    thread_arg = (THREAD_ARG *)malloc(sizeof(THREAD_ARG));
    memset(thread_arg,0,sizeof(THREAD_ARG));

    const char * platform               = argv[++arg_index];
    thread_arg->src_filename            = argv[++arg_index];
    thread_arg->output_filename         = argv[++arg_index];
    thread_arg->codecer_name            = argv[++arg_index];
    thread_arg->width                   = atoi(argv[++arg_index]);
    thread_arg->height                  = atoi(argv[++arg_index]);
    thread_arg->frame_rate              = atoi(argv[++arg_index]);
    thread_arg->bitrate                 = atoi(argv[++arg_index]);
    thread_arg->thread_num              = atoi(argv[++arg_index]);
    thread_arg->en_bmx264               = atoi(argv[++arg_index]);
    thread_arg->zero_copy               = atoi(argv[++arg_index]);
    thread_arg->sophon_idx              = atoi(argv[++arg_index]);

    if (strcmp(platform, "pcie") == 0)
        thread_arg->pcie_mode = 1;
    else if (strcmp(platform, "soc") == 0)
        thread_arg->pcie_mode = 0;
    else {
        av_log(NULL, AV_LOG_ERROR, "Invalid platform %s\n", platform);
        usage(argv[0]);
        return -1;
    }

    if(!thread_arg->src_filename \
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

    if(thread_arg->en_bmx264 < 0 ||  thread_arg->en_bmx264 > 1){
        usage(argv[0]);
        return -1;
    }

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
        thread_arg->zero_copy = 0;
    }

    int en_bmx264_thread = 0;
    if(thread_arg->en_bmx264 == 1)
        en_bmx264_thread = 1;

    ret = bm_dev_request(&g_bmHandle, thread_arg->sophon_idx);
    if (ret != BM_SUCCESS){
        av_log(NULL, AV_LOG_ERROR, "bm_dev_request failed !\n");
        return -1;
    }

    bm_get_chipid(g_bmHandle, &chipid);
    if (argc > 13){
        g_enable_mosaic = atoi(argv[++arg_index]);
        if (g_enable_mosaic)
            g_enable_mosaic = 1;
        else
            g_enable_mosaic = 0;
    }
    if (argc > 14){
        g_enable_watermark = atoi(argv[++arg_index]);
        if (g_enable_watermark)
            g_enable_watermark = 1;
        else
            g_enable_watermark = 0;
    }
    av_log(NULL, AV_LOG_INFO, "Mosaic function:    %s \n", (g_enable_mosaic==1) ? "enabled" : "disabled");
    av_log(NULL, AV_LOG_INFO, "Watermark function: %s \n", (g_enable_watermark==1) ? "enabled" : "disabled");

    if (g_enable_watermark){
        char water_bin_path[512];
        ret = find_watermark_path(water_bin_path);
        if(ret != 0)
        {
            free(thread_arg);
            if(g_bmHandle){
                bm_dev_free(g_bmHandle);
            }
            av_log(NULL, AV_LOG_ERROR, "water.bin not find, please copy water.bin to the current folder!!!\n");
            return -1;
        }

        FILE * fp_src = fopen(water_bin_path, "rb");
        if(!fp_src)
        {
            free(thread_arg);
            if(g_bmHandle){
                bm_dev_free(g_bmHandle);
            }
            av_log(NULL, AV_LOG_ERROR, "water.bin open failed !!!\n");
            return -1;
        }

        int water_w = 117;
        int water_h = 79;
        g_watermark = (bm_device_mem_t*)malloc(sizeof(bm_device_mem_t));
        ret = bm_malloc_device_byte_heap_mask(g_bmHandle, g_watermark, 0x2, 9243);
        char *watermark_file = (char *)malloc(9243);
        fread((void *)watermark_file, 1, 9243, fp_src);
        bm_memcpy_s2d(g_bmHandle, *g_watermark, (void *)watermark_file);
        fclose(fp_src);
        free(watermark_file);
    }

    int td_index = 0;

    //Initialize multiple threads
    while( thread_arg->thread_num ){
        thread_arg->thread_index = td_index;

        if(en_bmx264_thread == 1 && thread_arg->thread_num == 1 && thread_arg->pcie_mode == 1)
            thread_arg->en_bmx264 = 1;
        else
            thread_arg->en_bmx264 = 0;

        /* open decoder */
        VideoDec_FFMPEG* reader = &g_reader[td_index];
        ret = reader->openDec(thread_arg->src_filename,1,NULL,output_format_mode, thread_arg->sophon_idx, thread_arg->zero_copy);
        if(ret < 0 ){
            av_log(NULL, AV_LOG_ERROR, "open Decoder failed\n");
            return -1;
        }

        /* open encoder */
        VideoEnc_FFMPEG *writer = &g_writer[td_index];
        writer->hw_device_ctx = reader->hw_device_ctx;
        int width = (thread_arg->width + SET_ALIGNMENT - 1) & ~(SET_ALIGNMENT - 1);
        int height = (thread_arg->height + SET_ALIGNMENT - 1) & ~(SET_ALIGNMENT - 1);
        char file_name[256]         = {0};
        char file_name_tmp[256]     = {0};
        strcpy(file_name_tmp, thread_arg->output_filename);
        const char *name_start  = strtok(file_name_tmp, ".");
        const char *name_end    = strrchr(thread_arg->output_filename, 46);//ascii '.' = 46
        sprintf(file_name, "%s%d%s", name_start, td_index, name_end);
        if (thread_arg->en_bmx264)
            ret = writer->openEnc(file_name, "bmx264", 0 , thread_arg->frame_rate , width, height, AV_PIX_FMT_BMCODEC, thread_arg->bitrate, thread_arg->sophon_idx);
        else
            ret = writer->openEnc(file_name, thread_arg->codecer_name , 0 , thread_arg->frame_rate , width, height, AV_PIX_FMT_BMCODEC, thread_arg->bitrate, thread_arg->sophon_idx);
        if (ret !=0 ) {
            av_log(NULL, AV_LOG_ERROR,"writer.openEnc failed\n");
            return -1;
        }

        ret = pthread_create(&(dec_thread[td_index]), NULL, video_decoder_pthread, thread_arg);
        if (ret != 0) {
            av_log(NULL, AV_LOG_ERROR, "video_decoder pthread[%d] create failed \n", td_index);
            return -1;
        }

        ret = pthread_create(&(vpp_thread[td_index]), NULL, video_process_pthread, thread_arg);
        if (ret != 0) {
            av_log(NULL, AV_LOG_ERROR, "video_process pthread[%d] create failed \n", td_index);
            return -1;
        }

        ret = pthread_create(&(enc_thread[td_index]), NULL, video_encoder_pthread, thread_arg);
        if (ret != 0) {
            av_log(NULL, AV_LOG_ERROR, "video_encoder pthread[%d] create failed \n", td_index);
            return -1;
        }
        usleep(100000);
        td_index++;
        thread_arg->thread_index = td_index;
        thread_arg->thread_num--;
    }

    thread_arg->thread_num = td_index;
    g_thread_num = 3*thread_arg->thread_num;
    ret = pthread_create(&(stat_thread), NULL, stat_pthread, thread_arg);
    if (ret != 0) {
        av_log(NULL, AV_LOG_ERROR, "stat pthread create failed \n");
        return -1;
    }

    td_index = 0;
    while(1){
        bool exit_all = true;
        if(g_thread_num != 0){
            exit_all = false;
        }
        if(exit_all){
            g_quit_flag = 1;
            free(thread_arg);
            thread_arg = NULL;
            handler(0);
            break;
        }
        usleep(1000 * 500);
    }
    return 0;
}

void *video_decoder_pthread(void *arg){
    int sophon_idx              = 0;
    THREAD_ARG *thread_arg      = (THREAD_ARG *)arg;
    int index                   = thread_arg->thread_index;
    const char *src_filename    = thread_arg->src_filename;
    int height                  = thread_arg->height;
    int width                   = thread_arg->width;
    int en_bmx264               = thread_arg->en_bmx264;
    int zero_copy               = thread_arg->zero_copy;
    sophon_idx                  = thread_arg->sophon_idx;

    int ret                     =  0;
    struct timeval tv1, tv2;
    float time;

    width = (width + SET_ALIGNMENT - 1) & ~(SET_ALIGNMENT - 1);
    height = (height + SET_ALIGNMENT - 1) & ~(SET_ALIGNMENT - 1);

    VideoDec_FFMPEG* reader = &g_reader[index];

    gettimeofday(&tv1, NULL);
    while(!g_quit_flag){

        AVFrame *frame = av_frame_alloc();
        ret = reader->grabFrame2(frame);
        if(ret<0) {
            av_frame_unref(frame);
            av_frame_free(&frame);
            break;
        }
        while ((g_image_vpp_queue[index].size() == MAX_QUEUE_FRAME_3) && (!g_quit_flag))
        {
            usleep(10);
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
    int encode_pixel_format     = AV_PIX_FMT_BMCODEC;

    struct timeval tv1, tv2;
    unsigned int time;

    AVFrame* in_frame;
    AVFrame* out_frame;
    width = (width + SET_ALIGNMENT - 1) & ~(SET_ALIGNMENT - 1);
    height = (height + SET_ALIGNMENT - 1) & ~(SET_ALIGNMENT - 1);

    gettimeofday(&tv1, NULL);
    while(!g_quit_flag){
        while(g_image_vpp_queue[index].empty())
        {
            if (g_stop_flag[index] == 1)
                goto cleanup_vpp;
            if (g_quit_flag) break;
            usleep(100);
        }
        if (g_quit_flag)
            break;

        g_vpp_queue_lock[index].lock();
        in_frame = g_image_vpp_queue[index].front();
        g_image_vpp_queue[index].pop();
        g_vpp_queue_lock[index].unlock();

        out_frame = av_frame_alloc();
        AVFrameConvert(g_bmHandle, in_frame, out_frame, height, width, encode_pixel_format, g_writer[index].enc_ctx->hw_frames_ctx, g_enable_mosaic, g_enable_watermark, g_watermark);
        if(!(&out_frame)){
            av_log(NULL, AV_LOG_ERROR, "no frame ! \n");
            break;
        }

        av_frame_unref(in_frame);
        av_frame_free(&in_frame);

        /* push image to enc queue */
        while ((g_image_enc_queue[index].size() >= MAX_QUEUE_FRAME_5 ) && (!g_quit_flag)){
            usleep(10);
        }
        g_enc_queue_lock[index].lock();
        g_image_enc_queue[index].push(out_frame);
        g_enc_queue_lock[index].unlock();

        count_vpp[index]++;
        if ((count_vpp[index]+1) % 100 == 0){
            gettimeofday(&tv2, NULL);
            time = (tv2.tv_sec - tv1.tv_sec)*1000 + (tv2.tv_usec - tv1.tv_usec)/1000;
            fps_vpp[index] = count_vpp[index]*1000/time;
        }
    }

cleanup_vpp:
    g_thread_num_lock.lock();
    g_thread_num--;
    g_stop_flag[index] = 2;
    g_thread_num_lock.unlock();
    av_log(NULL, AV_LOG_INFO, "video_process finish!\n");
    return (void *)0;
}

void *video_encoder_pthread(void *arg){

    THREAD_ARG *thread_arg      = (THREAD_ARG *)arg;
    int index                   = thread_arg->thread_index;

    struct timeval tv1, tv2;
    unsigned int time;

    VideoEnc_FFMPEG *writer = &g_writer[index];;

    gettimeofday(&tv1, NULL);
    while(!g_quit_flag){
        AVFrame *frame;
        while(g_image_enc_queue[index].empty())
        {
            if (g_stop_flag[index] == 2){
                goto cleanup_enc;
            }
            if (g_quit_flag) break;
            usleep(100);
        }
        if (g_quit_flag)
            break;

        g_enc_queue_lock[index].lock();
        frame = g_image_enc_queue[index].front();
        g_image_enc_queue[index].pop();
        g_enc_queue_lock[index].unlock();

        writer->writeFrame(frame);

        if(frame){
            av_frame_unref(frame);
            av_frame_free(&frame);
        }

        count_enc[index]++;
        if ((count_enc[index]+1) % 100 == 0){
            gettimeofday(&tv2, NULL);
            time = (tv2.tv_sec - tv1.tv_sec)*1000 + (tv2.tv_usec - tv1.tv_usec)/1000;
            fps_enc[index] = (float)count_enc[index]*1000/time;
        }

    }

cleanup_enc:
    g_thread_num_lock.lock();
    g_thread_num--;
    g_thread_num_lock.unlock();
    av_log(NULL, AV_LOG_INFO, "encode finish!\n");
    return (void *)0;
}

void* stat_pthread(void *arg)
{
    THREAD_ARG *thread_arg      = (THREAD_ARG *)arg;
    int thread_num = thread_arg->thread_num;

    uint64_t last_count_dec[MAX_THREAD_NUM] = {0};
    uint64_t last_count_enc[MAX_THREAD_NUM] = {0};
    uint64_t last_count_vpp[MAX_THREAD_NUM] = {0};
    uint64_t last_count_sum = 0;

    int dis_mode = 0;
    char *display = getenv("DISPLAY_FRAMERATE");
    if (display) dis_mode = atoi(display);
    while(!g_quit_flag)
    {
        sleep(INTERVAL);
        if (dis_mode == 1) {
            for (int i = 0; i < thread_num; i++) {
                printf("ID[%2d] ,DEC_FRM[%10lld], DEC_FPS[%2.2f],[%2.2f] | VPP_FRM[%10lld], VPP_FPS[%2.2f],[%2.2f]| ENC_FRM[%10lld], ENC_FPS[%2.2f],[%2.2f], VPP_QUEUE[%d] ENC_QUEUE[%d]\n",
                    i, (long long)count_dec[i],((double)(count_dec[i]-last_count_dec[i]))/INTERVAL, fps_dec[i],
                       (long long)count_vpp[i],((double)(count_vpp[i]-last_count_vpp[i]))/INTERVAL, fps_vpp[i],
                       (long long)count_enc[i], ((double)(count_enc[i]-last_count_enc[i]))/INTERVAL, fps_enc[i], g_image_vpp_queue[i].size(), g_image_enc_queue[i].size());

                last_count_dec[i] = count_dec[i];
                last_count_vpp[i] = count_vpp[i];
                last_count_enc[i] = count_enc[i];

            }
        }
        else {
            uint64_t count_sum = 0;
            for (int i = 0; i < thread_num; i++)
              count_sum += count_enc[i];
            printf("thread %d, frame %lld, enc_fps %2.2f", thread_num, count_sum, ((double)(count_sum-last_count_sum))/INTERVAL);
            last_count_sum = count_sum;
        }
        printf("\r");
        fflush(stdout);
    }
    fflush(stdout);
    return NULL;
}

static void usage(char *program_name)
{
    av_log(NULL, AV_LOG_ERROR, "Usage: \n\t%s [platform] [src_filename] [output_filename] [codecer_name] [width] [height] [frame_rate] [bitrate] [thread_num] [en_bmx264] [zero_copy] [sophon_idx]  <optional: enable_mosaic> <optional: enable_watermark>\n", program_name);
    av_log(NULL, AV_LOG_ERROR, "\t[platform]                platform: pcie or soc \n");
    av_log(NULL, AV_LOG_ERROR, "\t[src_filename]            input file name x.mp4 x.ts...\n");
    av_log(NULL, AV_LOG_ERROR, "\t[output_filename]         encode output file name x.mp4,x.ts...\n");
    av_log(NULL, AV_LOG_ERROR, "\t[encoder_name]            encode h264_bm,h265_bm.\n");
    av_log(NULL, AV_LOG_ERROR, "\t[width]                   encode 32<width<=4096.\n");
    av_log(NULL, AV_LOG_ERROR, "\t[height]                  encode 32<height<=4096.\n");
    av_log(NULL, AV_LOG_ERROR, "\t[frame_rate]              encode frame_rate.\n");
    av_log(NULL, AV_LOG_ERROR, "\t[bitrate]                 encode bitrate 500 < bitrate < 10000\n");
    av_log(NULL, AV_LOG_ERROR, "\t[thread_num]              thread num 1~18.\n");
    av_log(NULL, AV_LOG_ERROR, "\t[en_bmx264]               pcie mode: 1 enable bmx264, 0 disable bmx264\n");
    av_log(NULL, AV_LOG_ERROR, "\t                          soc mode:  0 \n");
    av_log(NULL, AV_LOG_ERROR, "\t[zero_copy]               pcie mode: 0: copy host mem, 1: nocopy.\n");
    av_log(NULL, AV_LOG_ERROR, "\t                          soc mode:  0~N any number is acceptable, but it is invalid\n");
    av_log(NULL, AV_LOG_ERROR, "\t[sophon_idx]              pcie mode: sophon devices idx\n");
    av_log(NULL, AV_LOG_ERROR, "\t                          soc mode:  0, other number is invalid\\n");
    av_log(NULL, AV_LOG_ERROR, "\tOptional:                                                        \n");
    av_log(NULL, AV_LOG_ERROR, "\t<enable_mosaic>           Optional, add mosaic on the left_top corner, only bm1686 support now\n");
    av_log(NULL, AV_LOG_ERROR, "\t<enable_watermark>        Optional, add watermark on video, only bm1686 support now\n");
    av_log(NULL, AV_LOG_ERROR, "\t                                                                 \n");
    av_log(NULL, AV_LOG_ERROR, "\tExample:                                                         \n");
    av_log(NULL, AV_LOG_ERROR, "\t%s pcie example.mp4 test.ts h264_bm 800 400 25 3000 3 0 0 0\n", program_name);
}

void handler(int sig)
{
    g_quit_flag = 1;
    av_log(NULL, AV_LOG_INFO, "signal %d is received! \n", sig);
    av_log(NULL, AV_LOG_INFO, "program will exited \n");
    sleep(1);

    if (thread_arg)
        free(thread_arg);

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

    for (int i = 0; i < MAX_THREAD_NUM; i++) {
        if (!g_reader[i].isClosed())
            g_reader[i].closeDec();
        if (!g_writer[i].isClosed())
            g_writer[i].closeEnc();

        while(!g_image_enc_queue[i].empty())
        {
            AVFrame *frame;
            frame = g_image_enc_queue[i].front();
            g_image_enc_queue[i].pop();
            if (frame){
                av_frame_unref(frame);
                av_frame_free(&frame);
            }
        }
        while(!g_image_vpp_queue[i].empty())
        {
            AVFrame *frame;
            frame = g_image_vpp_queue[i].front();
            g_image_vpp_queue[i].pop();
            if (frame){
                av_frame_unref(frame);
                av_frame_free(&frame);
            }
        }
    }

    if (g_enable_watermark){
        bm_free_device(g_bmHandle, *g_watermark);
        free(g_watermark);
    }
    if(g_bmHandle){
        bm_dev_free(g_bmHandle);
    }

    exit(0);
}

int find_watermark_path(char watermark_path[512]){
    int ret = 0;

    char path1[512];
    getcwd(path1, 512);
    strcat(path1, "/water.bin");
    ret = access(path1, F_OK);
    if (ret == 0)
    {
        memset(watermark_path, 0, 512);
        strcpy(watermark_path, strcpy(watermark_path, path1));
        return 0;
    }

    const char* path2 = "/opt/sophon/sophon-sample-latest/bin/water.bin";
    ret = access(path2, F_OK);
    if (ret == 0)
    {
        memset(watermark_path, 0, 512);
        strcpy(watermark_path, path2);
        return 0;
    }

    return -1;
}
