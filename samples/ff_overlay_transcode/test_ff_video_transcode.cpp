#include "ff_video_decode.h"
#include "ff_video_encode.h"
#include "ff_video_filter.h"
extern "C"{
#include <sys/time.h>
#include <csignal>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
}

#define MAX_INST_NUM 256
#define PCIE_MODE_ARG_NUM 5
#define SOC_MODE_ARG_NUM 3
#define PCIE_CARD_NUM 1
#define SET_ALIGNMENT 8
#define ENC_ALIGNMENT 32
#define OVERLAY_NUM 32
pthread_t thread_id[MAX_INST_NUM];
int quit_flag    = 0;
int thread_count = 0;

typedef struct Overlay_args {
    char* overlay_filename;
    int x;
    int y;
} Overlay_arg;

typedef struct MultiInstTest {
    const char      *src_filename;
    const char      *decoder_name;
    const char      *output_filename;
    const char      *codecer_name;
    Overlay_arg*    overlay_args;
    int             overlay_num;
    int             eof_action;
    int             encode_pixel_format;
    int             sophon_idx;
    int             output_format_mode;
    int             pre_allocation_frame;
    int             is_by_filename;
    int             height;
    int             width;
    int             frame_rate;
    int             bitrate;
    int             is_dma_buffer;
    int             thread_index;
    int             thread_num;
    int             zero_copy;
    unsigned int    frame_nums[MAX_INST_NUM];
} THREAD_ARG;

static void usage(char *program_name);
void handler(int sig);
void *startOneInst(void *arg);


int main(int argc, char **argv)
{
    int arg_index = 0;
    int i = 0;

    if(argc < 10){
        usage(argv[0]);
        return -1;
    }

    signal(SIGINT,handler);
    signal(SIGTERM,handler);

    THREAD_ARG *thread_arg = (THREAD_ARG *)malloc(sizeof(THREAD_ARG));
    memset(thread_arg,0,sizeof(THREAD_ARG));

    thread_arg->bitrate         = 3000*1024;

    thread_arg->src_filename            = argv[++arg_index];
    thread_arg->output_filename         = argv[++arg_index];
    thread_arg->codecer_name            = argv[++arg_index];
    thread_arg->zero_copy               = atoi(argv[++arg_index]);
    thread_arg->sophon_idx              = atoi(argv[++arg_index]);
    thread_arg->overlay_num             = atoi(argv[++arg_index]);

    Overlay_arg overlay_args[thread_arg->overlay_num];
    memset(overlay_args, 0, sizeof(Overlay_arg));

    for(i=0; i<thread_arg->overlay_num; i++)
    {
        overlay_args[i].overlay_filename = argv[++arg_index];
        overlay_args[i].x = atoi(argv[++arg_index]);
        overlay_args[i].y = atoi(argv[++arg_index]);
    }

    thread_arg->overlay_args    = overlay_args;
    thread_arg->thread_num              = 1;

    int td_index = 0;

    while( thread_arg->thread_num ){
        thread_arg->thread_index = td_index;
        pthread_create(&(thread_id[td_index]), NULL, startOneInst, thread_arg);
        usleep(100000);
        td_index++;
        thread_arg->thread_index = td_index;
        thread_arg->thread_num--;
    }

    td_index = 0;
    while(1){
        if(quit_flag == 1 || !thread_count)
        {
            pthread_join(thread_id[td_index],NULL);
            td_index++;
        }
        if(td_index == thread_arg->thread_index){
            free(thread_arg);
            thread_arg = NULL;
            break;
        }
        usleep(1000 * 500);
    }
    return 0;
}

void *startOneInst(void *arg){
    thread_count++;
    THREAD_ARG *thread_arg      = (THREAD_ARG *)arg;
    int index                   = thread_arg->thread_index;
    const char *src_filename    = thread_arg->src_filename;
    //const char *decoder_name    = thread_arg->decoder_name;
    const char *output_filename = thread_arg->output_filename;
    const char *codecer_name    = thread_arg->codecer_name;
    Overlay_arg* overlay_args   = thread_arg->overlay_args;
    int overlay_num             = thread_arg->overlay_num;
    int eof_action              = thread_arg->eof_action;
    int output_format_mode      = thread_arg->output_format_mode;
    //int pre_allocation_frame    = thread_arg->pre_allocation_frame;
    //int is_by_filename          = thread_arg->is_by_filename;
    //int height                  = thread_arg->height;
    //int width                   = thread_arg->width;
    //int encode_pixel_format     = thread_arg->encode_pixel_format;
    int frame_rate              = thread_arg->frame_rate;
    int bitrate                 = thread_arg->bitrate;
    //int is_dma_buffer           = thread_arg->is_dma_buffer;
    int zero_copy               = thread_arg->zero_copy;
    int sophon_idx              = thread_arg->sophon_idx;

    int i;
    int ret                     =  0;
    char file_name[256]         = {0};
    char name_start[256]        = {0};
    char name_end[256]          = {0};
    struct timeval tv1, tv2;
    unsigned int time;

    AVRational main_time_base;

    AVFrame *main_frame             = NULL;
    AVFrame *overlay_frame          = NULL;
    AVFrame *filter_frame           = NULL;
    AVFrame *overlay_frames[OVERLAY_NUM] = {0};
    char args_main[256]         = {0};
    char* args_overlay_pics[OVERLAY_NUM] = {0};
    char* args_overlay[OVERLAY_NUM] = {0};

    VideoDec_FFMPEG main_reader;
    VideoDec_FFMPEG overlay_reader[OVERLAY_NUM];
    VideoEnc_FFMPEG writer;

    AVFilterContext *src_ctx_overlay[OVERLAY_NUM] = {0};
    AVFilterContext *overlay_ctx[OVERLAY_NUM] = {0};
    ffmpegFilterContext filter;
    filter.overlay_num = overlay_num;
    filter.src_ctx_overlay = src_ctx_overlay;
    filter.overlay_ctx = overlay_ctx;

    for(i=0; i<overlay_num; i++)
    {
        args_overlay_pics[i] = (char*)malloc(256);
        args_overlay[i] = (char*)malloc(256);
    }

    strcpy(file_name,output_filename);
    const char *name_start_temp  = strtok(file_name, ".");
    const char *name_end_temp    = strrchr(output_filename, 46);//ascii '.' = 46
    strncpy(name_start, name_start_temp, strlen(name_start_temp));
    strncpy(name_end,   name_end_temp, strlen(name_end_temp));
    // sprintf(file_name,"%s%d%s",name_start, index, name_end);
    sprintf(file_name,"%s%s",name_start, name_end);

#ifdef BM_PCIE_MODE
    ret = main_reader.openDec(src_filename,1,NULL,output_format_mode, 5 ,sophon_idx,zero_copy);
#else
    ret = main_reader.openDec(src_filename,1,NULL,output_format_mode,5);
#endif
    if(ret < 0 )
    {
        printf("open input media failed\n");
        thread_count--;
        return (void *)-1;
    }

    main_time_base = main_reader.ifmt_ctx->streams[main_reader.video_stream_idx]->time_base;

    for(i=0; i<overlay_num; i++)
    {
        ret = overlay_reader[i].openDec(overlay_args[i].overlay_filename,1,NULL,output_format_mode,5,sophon_idx,zero_copy);
        if(ret < 0 )
        {
            printf("open input media failed\n");
            thread_count--;
            return (void *)-1;
        }
    }

    sprintf(args_main, "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
                main_reader.video_dec_ctx->width, main_reader.video_dec_ctx->height, main_reader.video_dec_ctx->pix_fmt,
                main_time_base.num, main_time_base.den,
                main_reader.video_dec_ctx->sample_aspect_ratio.num, main_reader.video_dec_ctx->sample_aspect_ratio.den);


    for(i=0; i<overlay_num; i++)
    {
        sprintf(args_overlay_pics[i], "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
                overlay_reader[i].video_dec_ctx->width, overlay_reader[i].video_dec_ctx->height, overlay_reader[i].video_dec_ctx->pix_fmt,
                main_time_base.num, main_time_base.den,
                overlay_reader[i].video_dec_ctx->sample_aspect_ratio.num, overlay_reader[i].video_dec_ctx->sample_aspect_ratio.den);

        sprintf(args_overlay[i], "x=%d:y=%d:zero_copy=%d:sophon_idx=%d:eof_action=%d",
                overlay_args[i].x, overlay_args[i].y,
                zero_copy, sophon_idx, eof_action);
    }

    initFilter(&filter, args_main, args_overlay_pics, args_overlay);

    ret = writer.openEnc(file_name, codecer_name , 0, 25,
                        main_reader.video_dec_ctx->width, main_reader.video_dec_ctx->height,
                        main_reader.video_dec_ctx->pix_fmt, bitrate, sophon_idx);
    if (ret !=0 ) {
        av_log(NULL, AV_LOG_ERROR,"writer.openEnc failed\n");
        thread_count--;
        return (void *)-1;
    }

    gettimeofday(&tv1, NULL);
    while(!quit_flag){
        main_frame = main_reader.grabFrame2();
        if(!main_frame){
            break;
        }

        for(i=0; i<overlay_num; i++)
        {
            overlay_frame = overlay_reader[i].grabFrame2();
            if(!overlay_frame)
                break;
            overlay_frames[i] = overlay_frame;
        }

        filter_frame = getFilterFrame(filter, main_frame, overlay_frames);
        if(!filter_frame)
            break;

        if ((thread_arg->frame_nums[index]+1) % 300 == 0){
               gettimeofday(&tv2, NULL);
               time = (tv2.tv_sec - tv1.tv_sec)*1000 + (tv2.tv_usec - tv1.tv_usec)/1000;
               printf("%dth thread process is %5.4f fps!\n", index ,(thread_arg->frame_nums[index] * 1000.0) / (float)time);
        }

        writer.writeFrame(filter_frame);
        av_frame_unref(filter_frame);
        thread_arg->frame_nums[index]++;
    }

    gettimeofday(&tv2, NULL);
    time = (tv2.tv_sec - tv1.tv_sec)*1000 + (tv2.tv_usec - tv1.tv_usec)/1000;
    printf("%dth thread overlay %d frame in total, avg: %5.4f fps!, time: %dms!\n", index ,thread_arg->frame_nums[index],(float)thread_arg->frame_nums[index] * 1000 / (float)time, time);

    main_reader.closeDec();
    deInitFilter(filter);
    writer.closeEnc();
    av_log(NULL, AV_LOG_INFO, "encode finish!\n");
    thread_count--;
    return (void *)0;
}

static void usage(char *program_name)
{
    av_log(NULL, AV_LOG_ERROR, "Usage: \n\t%s [src_filename] [output_filename] [encode_pixel_format] [encoder_name] [height] [width] [frame_rate] [bitrate] [thread_num] [zero_copy] [sophon_idx]\n", program_name);
    av_log(NULL, AV_LOG_ERROR, "\t[src_filename]            input file name x.mp4 x.ts...\n");
    av_log(NULL, AV_LOG_ERROR, "\t[output_filename]         encode output file name x.mp4,x.ts...\n");
    av_log(NULL, AV_LOG_ERROR, "\t[encoder_name]            encode h264_bm,hevc_bm,h265_bm\n");
    av_log(NULL, AV_LOG_ERROR, "\t[zero_copy ]              0: copy host mem,1: nocopy.\n");
    av_log(NULL, AV_LOG_ERROR, "\t[sophon_idx]              sophon devices idx\n");
    av_log(NULL, AV_LOG_ERROR, "\t[overlay_num]             overlay num\n");
    av_log(NULL, AV_LOG_ERROR, "\t[overlay_filepath_1]      overlay file path 1\n");
    av_log(NULL, AV_LOG_ERROR, "\t[x]                       x position for overlay1 on src\n");
    av_log(NULL, AV_LOG_ERROR, "\t[y]                       y position for overlay1 on src\n");
    av_log(NULL, AV_LOG_ERROR, "\t[overlay_filepath_2]      overlay file path 2\n");
    av_log(NULL, AV_LOG_ERROR, "\t[x]                       x position for overlay2 on src\n");
    av_log(NULL, AV_LOG_ERROR, "\t[y]                       y position for overlay2 on src\n");
    av_log(NULL, AV_LOG_ERROR, "\t%s src.mp4 out.ts h264_bm 0 0 2 overlay_1.264 10 10 overlay_2.264 500 500\n", program_name);
}

void handler(int sig)
{
    quit_flag = 1;
    printf("program will exited \n");
}
