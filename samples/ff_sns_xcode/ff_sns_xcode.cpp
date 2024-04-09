#include "ff_sns_encode.hpp"
#include "ff_sns_decode.hpp"
#include "ff_avframe_convert.h"
#include <sys/time.h>
#include <csignal>
#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
#include "libavformat/avio.h"
#include "libavdevice/avdevice.h"
#include "libavutil/pixdesc.h"
}

#define MAX_THREAD_NUM 6
#define MAX_QUEUE_FRAME 3
int quit_flag = 0;
unsigned int count_dec[MAX_THREAD_NUM];
unsigned int count_enc[MAX_THREAD_NUM];
float fps_dec[MAX_THREAD_NUM];
float fps_enc[MAX_THREAD_NUM];
int g_stop_flag[MAX_THREAD_NUM] = {0};
std::thread dec_thread[MAX_THREAD_NUM];
std::thread enc_thread[MAX_THREAD_NUM];
bm_handle_t g_bmHandle        = {0};
std::queue<std::shared_ptr<AVFrame>> g_image_enc_queue[MAX_THREAD_NUM];
std::mutex g_enc_queue_lock[MAX_THREAD_NUM];
std::mutex g_thread_num_lock;
VideoEnc_FFMPEG g_writer[MAX_THREAD_NUM];
VideoDec_FFMPEG g_reader[MAX_THREAD_NUM];

static void usage(char *program_name) {
    av_log(NULL, AV_LOG_ERROR, "Usage: \n\t%s devnum <input dev> <output file> encoder framerate bitrate(kbps) use_isp_chn_num v4l2_buf_num framenum loopflag wdr_on is_yuv_sensor\n", program_name);
    av_log(NULL, AV_LOG_ERROR, "\tencoder: H264 or H265, H264 is default.\n");
    av_log(NULL, AV_LOG_ERROR, "\tdevnum: must be set to [1,6].\n");
    av_log(NULL, AV_LOG_ERROR, "\tuse_isp_chn_num: if tuse_isp_chn_num is not 0, it must be the same as devnum for all to have color.\n");
    av_log(NULL, AV_LOG_ERROR, "\tloopflag: If it is 0, it will encode and decode based on the number of frames; if it is 1, it will continuously encode and decode without generating valid videoes.\n");
    av_log(NULL, AV_LOG_ERROR, "\twdr_on: open the wdr mode or not\n");
    av_log(NULL, AV_LOG_ERROR, "\tis_yuv_sensor: is yuv sensor or not\n");
    av_log(NULL, AV_LOG_ERROR, "Examples:\n");
    av_log(NULL, AV_LOG_ERROR, "\t%s 2 /dev/video0 /dev/video1 /mnt/video0.264 /mnt/video1.264 H264 30 3000 2 8 100 0 0 0\n", program_name);
    av_log(NULL, AV_LOG_ERROR, "\t%s 1 /dev/video0 /mnt/video0.264 H264 30 3000 1 10 100 0 0 0\n", program_name);
}

void handler(int sig) {
    quit_flag = 1;
    av_log(NULL, AV_LOG_INFO, "program will exited \n");
}

int video_decoder_pthread(const char* input_file, int sophon_idx, int framenum, int v4l2_buf_num,
                        int use_isp_chn_num, int wdr_on, int is_yuv_sensor, int index);
int video_encoder_pthread(const char* output_file, int enccodec_id, int framerate, int bitrate, int index);

int main(int argc, char **argv) {
    avdevice_register_all();
    int ret = 0;
    int framerate = 30;
    int bitrate = 3000;
    int sophon_idx = 0;
    int framenum = 400;
    int v4l2_buf_num = 8;
    int use_isp_chn_num = 1;
    int enccodec_id = AV_CODEC_ID_H264;
    int wdr_on = 0;
    int is_yuv_sensor = 0;
    std::thread threads[MAX_THREAD_NUM];
    signal(SIGINT,handler);
    signal(SIGTERM,handler);

    if (argc < 3) {
        usage(argv[0]);
        return -1;
    }
    int outputCount = atoi(argv[1]);
    if (outputCount <= 0 || outputCount >= 7) {
        av_log(NULL, AV_LOG_WARNING, "devnum must be set to [1,6]\n");
        return 1;
    }
    std::string outputFiles[outputCount];
    std::string inputFiles[outputCount];
    for (int i = 0; i < outputCount; i++) {
        inputFiles[i] = argv[i + 2];
        outputFiles[i] = argv[i + outputCount + 2];
    }

    if (argc > 2*outputCount+2) {
        if (strstr(argv[2*outputCount+2],"H265") != NULL) {
           enccodec_id = AV_CODEC_ID_H265;
        }
    }
    if (argc > 2*outputCount+3) {
        int temp = atoi(argv[2*outputCount+3]);
        if (temp >10 && temp <= 60) {
             framerate = temp;
        } else {
            av_log(NULL, AV_LOG_WARNING, "frameteate must be set to (10,60]\n");
            return 1;
        }
        av_log(NULL, AV_LOG_INFO, "framerate = %d \n",framerate);
    }
    if (argc > 2*outputCount+4) {
        int temp = atoi(argv[2*outputCount+4]);
        if (temp >500 && temp < 10000) {
             bitrate = temp*1000;
        } else {
            av_log(NULL, AV_LOG_WARNING, "bitrate must be set to (500,10000)\n");
            return 1;
        }
        av_log(NULL, AV_LOG_INFO, "bitrate = %d \n", bitrate);
    }
    if (argc > 2*outputCount+5) {
        int temp = atoi(argv[2*outputCount+5]);
        if (temp >= 0 && temp <= 6) {
            use_isp_chn_num = temp;
        } else {
            av_log(NULL, AV_LOG_WARNING, "use_isp_chn_num must be set to [0,6]\n");
            return 1;
        }
        av_log(NULL, AV_LOG_INFO, "use_isp_chn_num = %d \n", use_isp_chn_num);
    }
    if (argc > 2*outputCount+6) {
        int temp = atoi(argv[2*outputCount+6]);
        if (temp > 0 && temp < 32) {
            v4l2_buf_num = temp;
        } else {
            av_log(NULL, AV_LOG_WARNING, "buf num must be set to (0,32)\n");
            return 1;
        }
        av_log(NULL, AV_LOG_INFO, "v4l2_buf_num = %d \n", v4l2_buf_num);
    }
    if (argc > 2*outputCount+7) {
        int temp = atoi(argv[2*outputCount+7]);
        if (temp >= 1) {
            framenum = temp;
        } else {
            av_log(NULL, AV_LOG_WARNING, "frame num must >= 1\n");
            return 1;
        }
        av_log(NULL, AV_LOG_INFO, "framenum = %d \n", framenum);
    }
    if (argc > 2*outputCount+8) {
        int temp = atoi(argv[2*outputCount+8]);
        if (temp == 1) {
            loopflag = temp;
            av_log(NULL, AV_LOG_WARNING, "endless loop is opening , video isn't saved ");
        }
        av_log(NULL, AV_LOG_INFO, "loopflag = %d \n", loopflag);
    }

    if (argc > 2*outputCount + 9) {
        int temp = atoi(argv[2*outputCount + 9]);
        if (temp == 1) {
            wdr_on = temp;
        }
        av_log(NULL, AV_LOG_INFO, "wdr_on = %d \n", wdr_on);
    }

    if (argc > 2*outputCount + 10) {
        int temp = atoi(argv[2*outputCount + 10]);
        if (temp == 1) {
            is_yuv_sensor = temp;
        }
        av_log(NULL, AV_LOG_INFO, "is_yuv_sensor = %d \n", is_yuv_sensor);
    }

    ret = bm_dev_request(&g_bmHandle, 0);
    if (ret != BM_SUCCESS){
        av_log(NULL, AV_LOG_DEBUG, "bm_dev_request failed !\n");
        return -1;
    }

    for (int i = 0; i < outputCount; i++) {
        dec_thread[i] = std::thread(video_decoder_pthread, inputFiles[i].c_str(), sophon_idx, framenum, v4l2_buf_num,
                                    use_isp_chn_num, wdr_on, is_yuv_sensor, i);
        enc_thread[i] = std::thread(video_encoder_pthread, outputFiles[i].c_str(), enccodec_id, framerate, bitrate, i);
        usleep(10000);
    }

    for (int i = 0; i < outputCount; i++) {
        if (dec_thread[i].joinable()) {
            dec_thread[i].join();
        }
        if (enc_thread[i].joinable()) {
            enc_thread[i].join();
            if (!g_writer[i].isClosed())
                g_writer[i].closeEnc();
            if (!g_reader[i].isClosed())
                g_reader[i].closeDec();
        }
    }

    if (g_bmHandle) {
        bm_dev_free(g_bmHandle);
    }

    return 0;
}

int video_decoder_pthread(const char* input_file, int sophon_idx, int framenum, int v4l2_buf_num,
                        int use_isp_chn_num, int wdr_on, int is_yuv_sensor, int index) {
    VideoDec_FFMPEG* reader = &g_reader[index];
    pthread_t tid = pthread_self();
    std::shared_ptr<AVFrame> frame;
    struct timeval tv1, tv2;
    int ret = 0, cur = 0;
    float time = 0;
    ret = reader->openDec(input_file, 9, sophon_idx, v4l2_buf_num, use_isp_chn_num, wdr_on, is_yuv_sensor);
    if (ret < 0) {
        av_log(NULL, AV_LOG_INFO, "#################open input media failed  ##########\n");
        return -1;
    }

    gettimeofday(&tv1, NULL);
    while (!quit_flag && cur++ <= framenum) {
        frame = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame* p){av_frame_unref(p); av_frame_free(&p);});
        ret = reader->grabFrame(frame.get());
        if (ret < 0) {
            break;
        }

        while ((g_image_enc_queue[index].size() == MAX_QUEUE_FRAME) && (!quit_flag)) {
            usleep(10);
        }

        g_enc_queue_lock[index].lock();
        g_image_enc_queue[index].push(std::move(frame));
        count_dec[index]++;
        g_enc_queue_lock[index].unlock();

        if ((count_dec[index]+1) % 300 == 0) {
            gettimeofday(&tv2, NULL);
            time = (tv2.tv_sec - tv1.tv_sec)*1000 + (tv2.tv_usec - tv1.tv_usec)/1000;
            fps_dec[index] = (float)count_dec[index]*1000/time;
            av_log(NULL, AV_LOG_INFO, "current decoder process %lu is %f fps!\n", (unsigned long)tid, fps_dec[index]);
        }
        if (loopflag) cur = 0;
    }

    frame = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame* p){av_frame_unref(p); av_frame_free(&p);});
    ret = reader->flushFrame(frame.get());

    g_thread_num_lock.lock();
    g_stop_flag[index] = 1;
    g_thread_num_lock.unlock();

    av_log(NULL, AV_LOG_INFO, "video decode finish!\n");
    return 0;
}
std::shared_ptr<AVFrame> avframe_422_to_420(AVFrame  *in_frame) {
    std::shared_ptr<AVFrame> out_frame;
    out_frame = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame* p){av_frame_unref(p); av_frame_free(&p);});
    AVFrameConvert(g_bmHandle, in_frame, out_frame.get(), in_frame->height, in_frame->width, AV_PIX_FMT_YUV420P, 0, 0, 0);
    if(!out_frame.get()) {
        av_log(NULL, AV_LOG_ERROR, "no frame ! \n");
        out_frame.reset();
        return NULL;
    }
    return out_frame;
}


int video_encoder_pthread(const char* output_file, int enccodec_id, int framerate, int bitrate, int index) {
    VideoEnc_FFMPEG *writer = &g_writer[index];
    pthread_t tid = pthread_self();
    std::shared_ptr<AVFrame> frame;
    std::shared_ptr<AVFrame> out_frame;
    struct timeval tv1, tv2;
    int ret = 0;
    float time = 0;
    gettimeofday(&tv1, NULL);

    while (!quit_flag) {

        while (g_image_enc_queue[index].empty()) {
            if (g_stop_flag[index] == 1){
                goto cleanup;
            }
            if (quit_flag) break;
            usleep(100);
        }

        g_enc_queue_lock[index].lock();
        frame = g_image_enc_queue[index].front();
        g_image_enc_queue[index].pop();
        g_enc_queue_lock[index].unlock();

        if (writer->isClosed()) {
        if (frame.get()->format == AV_PIX_FMT_YUYV422) {
            ret = writer->openEnc(output_file, enccodec_id, framerate, frame->width, frame->height,
                         AV_PIX_FMT_YUV420P, bitrate);
        } else {
            ret = writer->openEnc(output_file, enccodec_id, framerate, frame->width, frame->height,
                         frame.get()->format, bitrate);
        }
            if (ret != 0) {
                av_log(NULL, AV_LOG_INFO, "writer.openEnc failed \n ");
                goto cleanup;
            }
        }

        if (frame.get()->format == AV_PIX_FMT_YUYV422) {
            out_frame = avframe_422_to_420(frame.get());
            if(out_frame == NULL)
                break;
        }

        if (frame.get()->format == AV_PIX_FMT_YUYV422) {
            ret = writer->writeAvFrame(out_frame.get());
            frame.reset();
            out_frame.reset();
        } else {
            ret = writer->writeAvFrame(frame.get());
            frame.reset();
        }

        count_enc[index]++;

        if ((count_enc[index]+1) % 300 == 0) {
            gettimeofday(&tv2, NULL);
            time = (tv2.tv_sec - tv1.tv_sec)*1000 + (tv2.tv_usec - tv1.tv_usec)/1000;
            fps_enc[index] = (float)count_enc[index]*1000/time;
            av_log(NULL, AV_LOG_INFO, "current encoder process %lu is %f fps!\n", (unsigned long)tid, fps_enc[index]);
        }

    }

    while (!g_image_enc_queue[index].empty()) {
        g_enc_queue_lock[index].lock();
        frame = g_image_enc_queue[index].front();

        if (frame.get()->format == AV_PIX_FMT_YUYV422) {
            out_frame = avframe_422_to_420(frame.get());
            if(out_frame == NULL)
                break;
        }

        g_image_enc_queue[index].pop();
        g_enc_queue_lock[index].unlock();

        if (frame.get()->format == AV_PIX_FMT_YUYV422) {
            ret = writer->writeAvFrame(out_frame.get());
            frame.reset();
            out_frame.reset();
        } else {
            ret = writer->writeAvFrame(frame.get());
            frame.reset();
        }

        count_enc[index]++;
    }

cleanup:
    if (!writer->isClosed())
        writer->closeEnc();
    av_log(NULL, AV_LOG_INFO, "encode finish!\n");
    return 0;
}

