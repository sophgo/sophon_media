/**
  @file videocapture_basic.cpp
  @brief A very basic sample for using VideoCapture and VideoWriter
  @author PkLab.net
  @date Aug 24, 2016
*/

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/videoio/videoio_c.h>
#ifndef WIN32
#include <linux/videodev2.h>
#include <iostream>
#include <stdio.h>
#include <sys/time.h>
#include <fstream>
#endif
#include <unistd.h>
#include <csignal>
#include <thread>
#include <queue>
#include <mutex>

using namespace cv;
using namespace std;

#define MAX_THREAD_NUM 6
#define MAX_QUEUE_FRAME 3
#define PARAM_SET_INDEP 3
int quit_flag = 0;
unsigned int count_dec[MAX_THREAD_NUM];
unsigned int count_enc[MAX_THREAD_NUM];
float fps_dec[MAX_THREAD_NUM];
float fps_enc[MAX_THREAD_NUM];
int g_stop_flag[MAX_THREAD_NUM] = {0};
std::queue<Mat> g_image_enc_queue[MAX_THREAD_NUM];
std::thread dec_thread[MAX_THREAD_NUM];
std::thread enc_thread[MAX_THREAD_NUM];
std::mutex g_enc_queue_lock[MAX_THREAD_NUM];
std::mutex g_thread_num_lock[MAX_THREAD_NUM];
VideoWriter g_writer[MAX_THREAD_NUM];
VideoCapture g_reader[MAX_THREAD_NUM];
void handler(int sig) {
    quit_flag = 1;
    printf("program will exited \n");
}
int video_decoder_pthread(int framenum, int index, int use_isp, int wdr_on, int dev_num = 0);
int video_encoder_pthread(std::string output_file, int index);

int main(int argc, char* argv[])
{
    signal(SIGINT,handler);
    signal(SIGTERM,handler);
    unsigned int num_cores = std::thread::hardware_concurrency();
#ifndef WIN32

    if (argc < 4){
        cout << "usage:" << endl;
        cout << "\t" << argv[0] << " dev_num camera_id [out_name] [wdr_on_off] camera_id [out_name] [wdr_on_off] ... frame_num use_isp" << endl;
        cout << "\tframe_num: if frame_num = 1 means endless loop\n" << endl;
        cout << "\tuse_isp: use isp or not\n" << endl;
        cout << "Examples:\n\t" << "./vidsensor 2 1 /mnt/test1.264 0 2 /mnt/test2.264 0 601 1\n" << endl;
        cout << "\t./vidsensor 6 0 /mnt/test0.264 0 1 /mnt/test1.264 0 2 /mnt/test2.264 0 3 /mnt/test3.264 0 4 /mnt/test4.264 0 5 /mnt/test5.264 0 901 1\n" << endl;
        return -1;
    }
    int dev_num = atoi(argv[1]);
    if (dev_num <= 0 || dev_num >= 7) {
        printf("dev_num must be set to [1,6]\n");
        return 1;
    }
    std::string outputFiles[dev_num];
    int cameraID[dev_num];
    int wdr_off_on[dev_num];
    for (int i = 0; i < dev_num; i++) {
        cameraID[i] = atoi(argv[i * PARAM_SET_INDEP + 2]);
        outputFiles[i] = argv[i * PARAM_SET_INDEP + 3];
        wdr_off_on[i] = atoi(argv[i * PARAM_SET_INDEP + 4]);
    }

    int framenum = atoi(argv[dev_num * PARAM_SET_INDEP + 2]);
    if (framenum <= 0) {
        printf("frame num must >= 1\n");
        return -1;
    } else if (framenum == 1) {
        printf("open endless loop\n");
        for (int i = 0; i < dev_num; i++) {
            outputFiles[i] = "test_" + std::to_string(i) + "_endless.264";
        }
    }

    int use_isp = atoi(argv[dev_num*PARAM_SET_INDEP + 3]);
    if (use_isp != 0 && use_isp != 1) {
        printf("use_isp must be set to 0 or 1 \n");
        return 1;
    }

    for (int i = 0; i < dev_num; i++) {
        int core_id = (i + 1) % num_cores;
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(core_id, &cpuset);
        dec_thread[i] = std::thread(video_decoder_pthread, framenum, cameraID[i], use_isp, wdr_off_on[i], dev_num);
        pthread_setaffinity_np(dec_thread[i].native_handle(), sizeof(cpu_set_t), &cpuset);
        enc_thread[i] = std::thread(video_encoder_pthread, outputFiles[i], cameraID[i]);
        pthread_setaffinity_np(enc_thread[i].native_handle(), sizeof(cpu_set_t), &cpuset);
    }
    for (int i = 0; i < dev_num; i++) {
        if (dec_thread[i].joinable()) {
            dec_thread[i].join();
        }
        if (enc_thread[i].joinable()) {
            enc_thread[i].join();
        }
    }

    for (int i = 0; i < dev_num; i++) {
        g_writer[i].release();
        g_reader[i].release();
    }
#endif

    printf("Exit Done\n");
    return 0;
}

int video_decoder_pthread(int framenum, int index, int use_isp, int wdr_on, int dev_num) {
    VideoCapture &cap = g_reader[index];
    pthread_t tid = pthread_self();
    auto startTime = std::chrono::steady_clock::now();
    auto currentTime = std::chrono::steady_clock::now();
    std::chrono::milliseconds::rep elapsedTime;
    int ret = 0, cur = 0;
    float time = 0;
    Mat frame = Mat();
    cap.open(index, CAP_SOPH_V4L);
    if (!cap.isOpened()) {
        cerr << "ERROR! Unable to open " << index << " camera\n";
        return -1;
    }

    cap.set(CAP_PROP_OUTPUT_USE_MW, use_isp);
    cap.set(CAP_PROP_OUTPUT_WDR_ON, wdr_on);
    cap.set(CV_CAP_PROP_FOURCC, V4L2_PIX_FMT_NV21);
    cap.set(CAP_PROP_OUTPUT_DEV_NUM, dev_num);  //if this parameter dev_num == 0, v4l2 will set automatically.

    while (!quit_flag && cur++ <= framenum) {
        if (framenum == 1)
            cur = 0;
        cap.read(frame);

        if (frame.empty()) {
            cout << "End of video" << endl;
            continue;
        }
        while ((g_image_enc_queue[index].size() == MAX_QUEUE_FRAME) && (!quit_flag)) {
            usleep(10);
        }

        g_enc_queue_lock[index].lock();
        g_image_enc_queue[index].push(std::move(frame));
        count_dec[index]++;
        g_enc_queue_lock[index].unlock();
        if (count_dec[index] == 100) {
            currentTime = std::chrono::steady_clock::now();
            elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count();

            fps_dec[index] = static_cast<float>(count_dec[index]) / (elapsedTime / 1000.0f);
            printf("current decoder process %lu is %f fps!\n", (unsigned long)tid, fps_dec[index]);

            count_dec[index] = 0;
            startTime = currentTime;
        }

    }

    g_thread_num_lock[index].lock();
    g_stop_flag[index] = 1;
    g_thread_num_lock[index].unlock();
    printf("decode thread-%d finish!\n", index);
    return 0;
}

int video_encoder_pthread(std::string output_file, int index)
{
    VideoWriter &writer = g_writer[index];
    pthread_t tid = pthread_self();
    auto startTime = std::chrono::steady_clock::now();
    auto currentTime = std::chrono::steady_clock::now();
    std::chrono::milliseconds::rep elapsedTime;
    Mat frame = Mat();
    int ret = 0, cur = 0;
    float time = 0;

    while (!quit_flag) {
        while (g_image_enc_queue[index].empty()) {
            if (g_stop_flag[index] == 1){
                goto cleanup;
            }
            if (quit_flag) goto cleanup;
            usleep(10);
        }
        g_enc_queue_lock[index].lock();
        frame = g_image_enc_queue[index].front();
        g_image_enc_queue[index].pop();
        g_enc_queue_lock[index].unlock();

        if (!writer.isOpened()) {
            writer.open(output_file, cv::VideoWriter::fourcc('H', '2', '6', '4'), 30, frame.size(),"gop=30:gop_preset=9:bitrate=3000:enc_cmd_queue=1",
                        true, 0);
            if (!writer.isOpened()) {
                return -1;
            }
        }

        writer.write(frame);
        frame.release();
        count_enc[index]++;

        if (count_enc[index] == 100) {
            currentTime = std::chrono::steady_clock::now();
            elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count();

            fps_enc[index] = static_cast<float>(count_enc[index]) / (elapsedTime / 1000.0f);
            printf("current encoder process %lu is %f fps!\n", (unsigned long)tid, fps_enc[index]);

            count_enc[index] = 0;
            startTime = currentTime;
        }

    }

    while (!g_image_enc_queue[index].empty()) {
        g_enc_queue_lock[index].lock();
        frame = g_image_enc_queue[index].front();
        g_image_enc_queue[index].pop();
        g_enc_queue_lock[index].unlock();

        writer.write(frame);
        frame.release();
        count_enc[index]++;
    }

cleanup:
    printf("encode thread-%d finish!\n", index);
    return 0;
}
