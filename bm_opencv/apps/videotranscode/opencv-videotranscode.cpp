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
#include <iostream>
#include <stdio.h>
#include <queue>
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#endif
#define INTERVAL 1
#define IMAGE_MATQUEUE_NUM 25
#define MAX_THREAD_NUM 32
#define UNUSED(x) (void)(x)
int g_thread_num   = 0;

using namespace cv;
using namespace std;

void *videoLoadThread(void *arg);
void *videoWriteThread(void *arg);
void *displayFpsThread(void *arg);
void signal_handler(int sig);
void usage(char *argv_0);


#ifdef WIN32
HANDLE load_thread[MAX_THREAD_NUM];
HANDLE write_thread[MAX_THREAD_NUM];
HANDLE displayfps_thread;
#else
pthread_t load_thread[MAX_THREAD_NUM];
pthread_t write_thread[MAX_THREAD_NUM];
pthread_t displayfps_thread;
#endif

std::mutex imageQueue_lock[MAX_THREAD_NUM];
std::mutex cap_read_lock[MAX_THREAD_NUM];
std::mutex g_thread_num_lock;

unsigned int count_load[MAX_THREAD_NUM];
unsigned int count_write[MAX_THREAD_NUM];
float fps_load[MAX_THREAD_NUM];
float fps_write[MAX_THREAD_NUM];
int queue_pop[MAX_THREAD_NUM] = {0};

VideoWriter g_writer[MAX_THREAD_NUM];

#define BM_ALIGN16(_x)             (((_x)+0x0f)&~0x0f)
#define BM_ALIGN32(_x)             (((_x)+0x1f)&~0x1f)
#define BM_ALIGN64(_x)             (((_x)+0x3f)&~0x3f)
#define MAX_READ_TIMEOUT 1000*30*50 // 30s


typedef struct  threadArg{
    const char  *inputUrl;
    const char  *codecType;
    int         frameNum;
    const char  *outputName;
    int         yuvEnable;
    int         roiEnable;
    int         deviceId;
    const char  *encodeParams;
    int         startWrite;
    int         fps;
    int         imageCols;
    int         imageRows;
    queue<Mat*> *imageQueue;
    int         thread_num;
    int         thread_index;
    int         thread_iscreated;
}THREAD_ARG;


int eof[MAX_THREAD_NUM]             = {0};  // End of File flag.
int exit_flag[MAX_THREAD_NUM + 1]   = {0};  // EXIT program flag
int takeoutcount[MAX_THREAD_NUM]    = {0};  // ensure data number.


void signal_handler(int sig)
{
    UNUSED(sig);
    int i = 0;
    for (i = 0; i < MAX_THREAD_NUM+1; i++) {
        exit_flag[i] = 1;
    }

    /* wait thread quit */
    int try_count = 50;
    while (try_count--){
        bool exit_all = true;
        for (i = 0; i < MAX_THREAD_NUM; i++) {
            if(g_thread_num){
                exit_all = false;
            }
        }
        if(exit_all){
            break;
        }
        usleep(200);
    }
}


static inline void uwait(int usecond){

#ifdef __linux__
    usleep(usecond * 1000);
#elif _WIN32
    Sleep(usecond);
#endif
}

#ifdef __linux__
void *videoLoadThread(void *arg){
#elif _WIN32
DWORD WINAPI videoLoadThread(void* arg){
#endif
    //--- INITIALIZE VIDEOCAPTURE
    VideoCapture cap;
    int ret;

#ifdef WIN32
    LARGE_INTEGER liPerfFreq={0};
    QueryPerformanceFrequency(&liPerfFreq);
    LARGE_INTEGER tv1 = {0};
    LARGE_INTEGER tv2 = {0};
#else
    struct timeval tv1, tv2;
    float time;
#endif
    THREAD_ARG *threadPara          = (THREAD_ARG *)(arg);
    int index                       = threadPara->thread_index;
    threadPara->thread_iscreated    = 1;
    count_load[index]               = 0;
    int is_stream                   = 0;
    Mat *toEncImage                 = NULL;
    threadPara->imageQueue          = new queue<Mat*>;
    int read_times                  = MAX_READ_TIMEOUT;


    // open the default camera using default API
    cap.open(threadPara->inputUrl, CAP_FFMPEG, threadPara->deviceId);
    if (!cap.isOpened()) {
        cerr << "ERROR! Unable to open camera\n";
        return nullptr;
    }
    // Set Resamper
    cap.set(CAP_PROP_OUTPUT_SRC, 1.0);
    double out_sar = cap.get(CAP_PROP_OUTPUT_SRC);
    cout << "CAP_PROP_OUTPUT_SAR: " << out_sar << endl;

    if (exit_flag[index])
        goto cleanup_load;

    if(threadPara->yuvEnable == 1){
        cap.set(cv::CAP_PROP_OUTPUT_YUV, PROP_TRUE);
    }

    if(threadPara->startWrite)
    {
        threadPara->fps = cap.get(CAP_PROP_FPS);
        threadPara->imageCols = cap.get(CAP_PROP_FRAME_WIDTH);
        threadPara->imageRows = cap.get(CAP_PROP_FRAME_HEIGHT);
    }

#ifdef WIN32
    write_thread[index] = CreateThread(NULL, 0, videoWriteThread, threadPara, 0, NULL);
#else
    ret = pthread_create(&(write_thread[index]), NULL, videoWriteThread, threadPara);
#endif
    if (ret != 0) {
        return nullptr;
    }

    if(strstr(threadPara->inputUrl,"rtmp://") || strstr(threadPara->inputUrl,"rtsp://"))
        is_stream = 1;

    //--- GRAB AND WRITE LOOP
#ifdef WIN32
    QueryPerformanceCounter(&tv1);
#else
    gettimeofday(&tv1, NULL);
#endif

    for(; (int)count_load[index] < threadPara->frameNum;)
    {
        if (read_times <= 0) {
            break;
        }

        if(exit_flag[index]){
            break;
        }

        cap_read_lock[index].lock();
        Mat *image = new Mat;
        cap.read(*image);
        cap_read_lock[index].unlock();

        if (image->empty()) {
            if ((int)cap.get(CAP_PROP_STATUS) == 2) {     // eof
                cap.release();
                cap.open(threadPara->inputUrl, CAP_FFMPEG, threadPara->deviceId);
                if(threadPara->yuvEnable == 1){
                    cap.set(cv::CAP_PROP_OUTPUT_YUV, PROP_TRUE);
                }
            }
            read_times--;
            uwait(1);
        } else {
            read_times = MAX_READ_TIMEOUT;
            imageQueue_lock[index].lock();
            threadPara->imageQueue->push(image);
            imageQueue_lock[index].unlock();
            count_load[index]++;

            if (threadPara->imageQueue->size() >= IMAGE_MATQUEUE_NUM && is_stream)
            {
                imageQueue_lock[index].lock();
                toEncImage = threadPara->imageQueue->front();
                threadPara->imageQueue->pop();
                delete toEncImage;
                queue_pop[index]++;
                imageQueue_lock[index].unlock();
            }

            if(threadPara->startWrite){
                while(threadPara->imageQueue->size() >= IMAGE_MATQUEUE_NUM){
                    uwait(2);
                    if(exit_flag[index])
                        break;
                }
            }
        }
        if ((count_load[index]+1) % 30 == 0)
        {
#ifdef WIN32
            QueryPerformanceCounter(&tv2);
            time = ( ((tv2.QuadPart - tv1.QuadPart) * 1000)/liPerfFreq.QuadPart);
#else
            gettimeofday(&tv2, NULL);
            time = (tv2.tv_sec - tv1.tv_sec)*1000 + (tv2.tv_usec - tv1.tv_usec)/1000;
#endif
            fps_load[index] = (float)count_load[index]*1000/time;
        }
    }

cleanup_load:
#ifdef WIN32
    QueryPerformanceCounter(&tv2);
    time = ( ((tv2.QuadPart - tv1.QuadPart) * 1000)/liPerfFreq.QuadPart);
#else
    gettimeofday(&tv2, NULL);
    time = (tv2.tv_sec - tv1.tv_sec)*1000 + (tv2.tv_usec - tv1.tv_usec)/1000;
#endif
    fps_load[index] = (float)count_load[index]*1000/time;

    printf("End of file. \n Decode thread[%d] exit. Decode %d frames. dropped %d frames. \n",
                            index, count_load[index] - queue_pop[index], queue_pop[index]);
    eof[index] = 1;
    cap.release();
    g_thread_num_lock.lock();
    g_thread_num--;
    g_thread_num_lock.unlock();

#ifdef __linux__
    return (void *)0;
#elif _WIN32
    return -1;
#endif
}


#ifdef __linux__
void *videoWriteThread(void *arg){
#elif _WIN32
DWORD WINAPI videoWriteThread(void* arg){
#endif
    THREAD_ARG *threadPara = (THREAD_ARG *)(arg);
    int index              = threadPara->thread_index;
    FILE *fp_out           = NULL;
    char *out_buf          = NULL;
    int is_stream          = 0;
    int quit_times         = 0;
    VideoWriter *writer    = &g_writer[index];
    Mat                    image;
    string outfile         = "";
    string encodeparms     = "";
    int64_t curframe_start = 0;
    int64_t curframe_end   = 0;
    Mat *toEncImage;
    int flush_status       = 0;  // 0 is stop , 1 flushed
    struct timeval tv1, tv2;
    float time;
    count_write[index]     = 0;
    string default_outfile = "pkt.dump";
	int idx = 0;

#ifdef __linux__
    struct timeval         tv;
#endif
    if(threadPara->encodeParams)
        encodeparms = threadPara->encodeParams;

    size_t pos = default_outfile.find(".");
    if (pos != std::string::npos) {default_outfile.insert(pos, "_" + to_string(threadPara->thread_index));}
    const char *cstr = default_outfile.c_str();
    if((strcmp(threadPara->outputName,"NULL") != 0) && (strcmp(threadPara->outputName,"null") != 0)) {
        outfile = threadPara->outputName;
        size_t last_index = outfile.rfind('/');
    	string last_string = outfile.substr(last_index+1);
    	outfile = outfile.substr(0, last_index+1); //处理完last_part后，直接outfile重新加上
    	size_t pos1 = last_string.find('.');
    	if(pos1 == std::string::npos)
        	pos1 = last_string.size();
    	last_string.insert(pos1, "_"+to_string(threadPara->thread_index));
    	outfile += last_string;
        cout << "thread[" << threadPara->thread_index << "]  url: " << outfile << endl;
    }

    if(strstr(threadPara->outputName,"rtmp://") || strstr(threadPara->outputName,"rtsp://"))
        is_stream = 1;
    if(strcmp(threadPara->codecType,"H264enc") ==0)
    {
        writer->open(outfile, VideoWriter::fourcc('H', '2', '6', '4'),
        threadPara->fps,
        Size(threadPara->imageCols, threadPara->imageRows),
        encodeparms,
        true,
        threadPara->deviceId);
    }
    else if(strcmp(threadPara->codecType,"H265enc") ==0)
    {
       writer->open(outfile, VideoWriter::fourcc('h', 'v', 'c', '1'),
        threadPara->fps,
        Size(threadPara->imageCols, threadPara->imageRows),
        encodeparms,
        true,
        threadPara->deviceId);
    }
    else if(strcmp(threadPara->codecType,"MPEG2enc") ==0)
    {
       writer->open(outfile, VideoWriter::fourcc('M', 'P', 'G', '2'),
        threadPara->fps,
        Size(threadPara->imageCols, threadPara->imageRows),
        true,
        threadPara->deviceId);
    }

    if(!writer->isOpened())
    {
#ifdef __linux__
        return (void *)-1;
#elif _WIN32
        return -1;
#endif
    }

#ifdef WIN32
    QueryPerformanceCounter(&tv1);
#else
    gettimeofday(&tv1, NULL);
#endif

    if (exit_flag[index])
        goto cleanup_write;
    while(1){
        if(is_stream){
#ifdef __linux__
            gettimeofday(&tv, NULL);
            curframe_start= (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
#elif _WIN32
            FILETIME ft;
            GetSystemTimeAsFileTime(&ft);
            curframe_start = (int64_t)ft.dwHighDateTime << 32 | ft.dwLowDateTime;// unit is 100 us
            curframe_start = curframe_start / 10;
#endif
        }

        if(threadPara->startWrite)
        {
            if((strcmp(threadPara->outputName,"NULL") != 0) && (strcmp(threadPara->outputName,"null") != 0))
            {
                if (!threadPara->imageQueue->empty()){   // Debug encode single image
                    imageQueue_lock[index].lock();
                    toEncImage = threadPara->imageQueue->front();
                    threadPara->imageQueue->pop();
                    takeoutcount[index]++;
                    imageQueue_lock[index].unlock();
                    writer->write(*toEncImage);
                    delete toEncImage;
                    count_write[index]++;
                }else{
                    if( eof[index] ){   // Currently queue is empty while end of file, start to flush.
                        Mat flushMat;
                        writer->write(flushMat);
                        flush_status = 1;
                        break;
                    }
                    uwait(2);
                    quit_times++;
                }
            }else  // need data return
            {
                if(fp_out == NULL)
                    fp_out = fopen(cstr,"wb+");

                if(out_buf == NULL)
                    out_buf = (char*)malloc(threadPara->imageCols * threadPara->imageRows * 4);
                int out_buf_len = 0;

                if (!threadPara->imageQueue->empty()){
                    imageQueue_lock[index].lock();
                    toEncImage = threadPara->imageQueue->front();
                    threadPara->imageQueue->pop();
                    takeoutcount[index]++;
                    imageQueue_lock[index].unlock();
                    count_write[index]++;

                    if (threadPara->roiEnable == 1) {
                        static unsigned int roi_frame_nums = 0;
                        roi_frame_nums++;
                        CV_RoiInfo roiinfo;
                        roiinfo.field = NULL;

                        if (strcmp(threadPara->codecType,"H264enc") ==0) {
                            int nums = (BM_ALIGN16(threadPara->imageRows) >> 4) * (BM_ALIGN16(threadPara->imageCols) >> 4);
                            roiinfo.numbers = nums;
                            roiinfo.customRoiMapEnable = 1;
                            roiinfo.field = (cv::RoiField*)malloc(sizeof(cv::RoiField)*nums);
                            for (int i = 0;i <(BM_ALIGN16(threadPara->imageRows) >> 4);i++) {
                                for (int j=0;j < (BM_ALIGN16(threadPara->imageCols) >> 4);j++) {
                                    idx = i*(BM_ALIGN16(threadPara->imageCols) >> 4) + j;
                                    // roiinfo.field[idx].H264.mb_qp = roi_frame_nums%51;
                                    if ((i >= (BM_ALIGN16(threadPara->imageRows) >> 4)/2) && (j >= (BM_ALIGN16(threadPara->imageCols) >> 4)/2)) {
                                        roiinfo.field[idx].H264.mb_qp = 10;
                                    }else{
                                        roiinfo.field[idx].H264.mb_qp = 40;
                                    }
                                }
                            }
                        } else if (strcmp(threadPara->codecType,"H265enc") ==0) {
                            int nums = (BM_ALIGN64(threadPara->imageRows) >> 6) * (BM_ALIGN64(threadPara->imageCols) >> 6);
                            roiinfo.numbers = nums;
                            roiinfo.field = (cv::RoiField*)malloc(sizeof(cv::RoiField)*nums);
                            roiinfo.customRoiMapEnable    = 1;
                            roiinfo.customModeMapEnable   = 0;
                            roiinfo.customLambdaMapEnable = 0;
                            roiinfo.customCoefDropEnable  = 0;

                            for (int i = 0;i <(BM_ALIGN64(threadPara->imageRows) >> 6);i++) {
                                for (int j=0;j < (BM_ALIGN64(threadPara->imageCols) >> 6);j++) {
                                    idx = i*(BM_ALIGN64(threadPara->imageCols) >> 6) + j;
                                    if ((i >= (BM_ALIGN64(threadPara->imageRows) >> 6)/2) && (j >= (BM_ALIGN64(threadPara->imageCols) >> 6)/2)) {
                                        roiinfo.field[idx].HEVC.sub_ctu_qp_0 = 10;
                                        roiinfo.field[idx].HEVC.sub_ctu_qp_1 = 10;
                                        roiinfo.field[idx].HEVC.sub_ctu_qp_2 = 10;
                                        roiinfo.field[idx].HEVC.sub_ctu_qp_3 = 10;
                                    } else {
                                        roiinfo.field[idx].HEVC.sub_ctu_qp_0 = 40;
                                        roiinfo.field[idx].HEVC.sub_ctu_qp_1 = 40;
                                        roiinfo.field[idx].HEVC.sub_ctu_qp_2 = 40;
                                        roiinfo.field[idx].HEVC.sub_ctu_qp_3 = 40;

                                    }
                                    roiinfo.field[idx].HEVC.ctu_force_mode = 0;
                                    roiinfo.field[idx].HEVC.ctu_coeff_drop = 0;
                                    roiinfo.field[idx].HEVC.lambda_sad_0 = 0;
                                    roiinfo.field[idx].HEVC.lambda_sad_1 = 0;
                                    roiinfo.field[idx].HEVC.lambda_sad_2 = 0;
                                    roiinfo.field[idx].HEVC.lambda_sad_3 = 0;
                                }
                            }
                        }

                        writer->write(*toEncImage, out_buf, &out_buf_len, &roiinfo);
                        delete toEncImage;
                        if (roiinfo.field != NULL) {
                            free(roiinfo.field);
                            roiinfo.field = NULL;
                        }
                    } else {   // roienable = 0
                        writer->write(*toEncImage, out_buf, &out_buf_len);
                        delete toEncImage;
                    }
                }else{ // queue is empty
                    if( eof[index] ){  //end of file while queue is empty,start to flush
                        Mat flushMat;
                        while(1){
                            writer->write(flushMat, out_buf, &out_buf_len);
                            if(out_buf_len > 0){
                                fwrite(out_buf, 1, out_buf_len, fp_out);
                                out_buf_len = 0; //reset out_buf_len
                            }else
                                break;
                        }
                        flush_status = 1;
                        break;
                    }
                    // if not end of file,keep waiting
                    uwait(2);
                    quit_times++;
                }
                // write to pkt.dump
                if(out_buf_len > 0)
                    fwrite(out_buf, 1, out_buf_len, fp_out);

            }

            if( !threadPara->imageQueue->empty() && takeoutcount[index] > 0){
                //delete toEncImage;
                takeoutcount[index]--;
                quit_times = 0;
            }
        }

        if ((count_write[index]+1) % 30 == 0){
#ifdef WIN32
                QueryPerformanceCounter(&tv2);
                time = ( ((tv2.QuadPart - tv1.QuadPart) * 1000)/liPerfFreq.QuadPart);
#else
            gettimeofday(&tv2, NULL);
            time = (tv2.tv_sec - tv1.tv_sec)*1000 + (tv2.tv_usec - tv1.tv_usec)/1000;
#endif
            fps_write[index] = (float)count_write[index]*1000/time;
        }

        //only Push video stream
        if(is_stream){
#ifdef __linux__
            gettimeofday(&tv, NULL);
            curframe_end= (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
#elif _WIN32
            FILETIME ft;
            GetSystemTimeAsFileTime(&ft);
            curframe_end = (int64_t)ft.dwHighDateTime << 32 | ft.dwLowDateTime;
            curframe_end = curframe_end / 10;
#endif
            if(curframe_end - curframe_start > 1000000 / threadPara->fps) {
                continue;
            }
#ifdef __linux__
            usleep((1000000 / threadPara->fps) - (curframe_end - curframe_start));
#elif _WIN32
            Sleep((1000 / threadPara->fps) - (curframe_end - curframe_start)/1000 - 1);
#endif
        }

        if((eof[index] && threadPara->imageQueue->size() == 0 && flush_status) || quit_times >= 30000 || exit_flag[index])
        {//No bitstream exits after a delay of three seconds
            flush_status = 0;
            exit_flag[index] = 1;
            break;
        }
        else if (!eof[index] || !threadPara->imageQueue->size() == 0 || !flush_status){
            continue;
        }
    }

cleanup_write:
#ifdef WIN32
    QueryPerformanceCounter(&tv2);
    time = ( ((tv2.QuadPart - tv1.QuadPart) * 1000)/liPerfFreq.QuadPart);
#else
    gettimeofday(&tv2, NULL);
    time = (tv2.tv_sec - tv1.tv_sec)*1000 + (tv2.tv_usec - tv1.tv_usec)/1000;
#endif
    fps_write[index] = (float)count_write[index]*1000/time;

    if(fp_out != NULL){
        fclose(fp_out);
        fp_out = NULL;
    }
    if(out_buf != NULL){
        free(out_buf);
        out_buf = NULL;
    }

    printf("encode thread[%d] exit. \n", index);
    exit_flag[index] = 1;
    if (writer->isOpened())
        writer->release();
    g_thread_num_lock.lock();
    g_thread_num--;
    g_thread_num_lock.unlock();

#ifdef __linux__
    return (void *)0;
#elif _WIN32
    return -1;
#endif
}


void* displayFpsThread(void *arg)
{
    THREAD_ARG *threadPara        = (THREAD_ARG *)arg;
    int thread_num                = threadPara->thread_num;
    int interval_num              = 1;
    int dis_mode                  = 0;

    uint64_t last_count_load[MAX_THREAD_NUM]    = {0};
    uint64_t last_count_write[MAX_THREAD_NUM]   = {0};
    uint64_t count_load_sum                     = 0;
    uint64_t count_write_sum                    = 0;
    float fps_load_sum                          = 0;
    float fps_write_sum                         = 0;

    char *display = getenv("DISPLAY_FRAMERATE");
    if (display) dis_mode = atoi(display);

    while(1)
    {
        for (int i = 0; i < interval_num; i++)
        {
            if (exit_flag[MAX_THREAD_NUM]) {
                sleep(2);
                goto cleanup_displayfps;
            }
            if (g_thread_num == 1)
                goto cleanup_displayfps;
            sleep(INTERVAL);
        }

        if (dis_mode == 1) {
            for (int i = 0; i < thread_num; i++) {
                printf("ID[%d] ,DEC_FRM[%10lld], DEC_FPS[%5.2f],[%5.2f] | ENC_FRM[%10lld], ENC_FPS[%5.2f],[%5.2f], ENC_QUEUE_NUM[%ld]\n",
                    i, (long long)count_load[i],((double)(count_load[i]-last_count_load[i]))/interval_num, fps_load[i],
                       (long long)count_write[i], ((double)(count_write[i]-last_count_write[i]))/interval_num, fps_write[i], threadPara[i].imageQueue->size());

                last_count_load[i] = count_load[i];
                last_count_write[i] = count_write[i];
            }
        }
        else {
            count_load_sum = count_write_sum = 0;
            fps_load_sum = fps_write_sum = 0;
            for (int i = 0; i < thread_num; i++) {
                count_load_sum += count_load[i];
                count_write_sum += count_write[i];
                fps_load_sum += fps_load[i];
                fps_write_sum += fps_write[i];
            }
            printf("thread %d, dec frame: %ld, dec fps %2.2f, enc frame: %ld, enc fps %2.2f", thread_num,
                    count_load_sum, fps_load_sum, count_write_sum, fps_write_sum);
        }
        printf("\r");
        fflush(stdout);
    }

cleanup_displayfps:
    fflush(stdout);
    // display average fps
    for (int i = 0; i < thread_num; i++) {
        printf("ID[%d] ,END_DEC_FRM[%10lld], AVERAGE_DEC_FPS[%5.2f] | END_ENC_FRM[%10lld], AVERAGE_ENC_FPS[%5.2f]\n",
            i, (long long)count_load[i], fps_load[i], (long long)count_write[i], fps_write[i]);
    }
    printf("displayFps thread exit. \n");
    g_thread_num_lock.lock();
    g_thread_num--;
    g_thread_num_lock.unlock();
    return NULL;
}


void usage(char *argv_0){
    cout << "usage:  test encoder by HW(h.264/h.265) with different container !!" << endl;
#ifdef USING_SOC
    cout << "\t" << argv_0 << " input code_type frame_num outputname yuv_enable roi_enable [thread_num] [encodeparams]" <<endl;
    cout << "\t" << "eg: " << argv_0 << " rtsp://admin:bitmain.com@192.168.1.14:554  H265enc  10000 encoder_test265.ts 1 0 10 bitrate=1000" <<endl;
#else
    cout << "\t" << argv_0 << " input code_type frame_num outputname yuv_enable roi_enable device_id [thread_num] [encodeparams]" <<endl;
    cout << "\t" << "eg: " << argv_0 << " rtsp://admin:bitmain.com@192.168.1.14:554  H265enc  10000 encoder_test265.ts 1 0 0 10 bitrate=1000" <<endl;
#endif
    cout << "params:" << endl;
    cout << "\t" << "<code_type>: H264enc is h264; H265enc is h265." << endl;
    cout << "\t" << "<framenum>: frame number you want to output,default is video frames"<<endl;
    cout << "\t" << "<outputname>: null or NULL output pkt.dump." << endl;
    cout << "\t" << "<yuv_enable>: 0 decode output bgr; 1 decode output yuv420." << endl;
    cout << "\t" << "<roi_enable>: 0 disable roi encoder; 1 enable roi encoder." << endl;
    cout << "\t" << "              if roi_enable is 1, you should set null/Null in outputname and set roi_enable=1 in encodeparams." << endl;
    cout << "\t" << "<thread_num>: thread number you want to create, MAX thread_num is 32, default value is 0. " << endl;
    cout << "\t" << "<encodeparams>: gop=30:bitrate=800:gop_preset=2:mb_rc=1:delta_qp=3:min_qp=20:max_qp=40:roi_enable=1:enc_cmd_queue=1:push_stream=rtmp/rtsp." << endl;
    cout << "\t" << "Tip: if there is only one optional parameter, thread_num takes precedence." << endl;
    cout << "\t" << "Tip: When the output is xxx.xx file, the new file name is xxx_threadnum.xx;  eg：when thread_num is 1:\n"
                    "                                     rtmp://172.28.141.136/live/room1——》 rtmp://172.28.141.136/live/room1_0 \n"
                    "                                     rtsp://172.28.141.136/test.ts——》 rtsp://172.28.141.136/test_0.ts"<< endl;
}


int main(int argc, char* argv[])
{
    //--- INITIALIZE VIDEOCAPTURE
    VideoCapture cap;
    int ret = 0;
    int deviceId = 0;
    int thread_num = 0;
    char  *encodeParams;

    if (argc < 7){
        usage(argv[0]);
        return -1;
    }

#ifdef USING_SOC
    if (argc == 7) {
        encodeParams = NULL;
        thread_num = 1;
    }
    else if (argc == 8) {
        encodeParams = NULL;
        thread_num = atoi(argv[7]);
    }
    else if (argc == 9) {
        encodeParams = argv[8];
        thread_num = atoi(argv[7]);
    }
    else if (argc > 9) {
        usage(argv[0]);
        return -1;
    }
#else
    if (argc == 7) {
        encodeParams = NULL;
        thread_num = 1;
    }
    else if (argc == 10) {
        encodeParams = argv[9];
        thread_num = atoi(argv[8]);
        deviceId = atoi(argv[7]);
    }
#endif


#ifdef __linux__
    signal(SIGINT, signal_handler);
#elif _WIN32
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE);
#endif

    THREAD_ARG *threadParas = (THREAD_ARG *)malloc(sizeof(THREAD_ARG) * thread_num);
    memset(threadParas,0,sizeof(THREAD_ARG) * thread_num);

    if(!threadParas){
        return -1;
    }

    threadParas->outputName = argv[4];
    threadParas->codecType  = argv[2];
    threadParas->inputUrl   = argv[1];
    threadParas->frameNum   = atoi(argv[3]);
    if ( strcmp(threadParas->codecType,"H264enc") ==0
      || strcmp(threadParas->codecType,"H265enc") == 0
      || strcmp(threadParas->codecType,"MPEG2enc") == 0)
    {
        for (int i = 0; i < thread_num; i++) {
            delete threadParas[i].imageQueue;
            threadParas[i].imageQueue = NULL;
        }
        threadParas->startWrite = 1;
    } else {
        if(threadParas){
            free(threadParas);
            threadParas = NULL;
        }
        return 0;
    }

    threadParas->deviceId = deviceId;
    threadParas->thread_num = thread_num;
    threadParas->encodeParams = encodeParams;

    if ((threadParas->thread_num <= 0) || (threadParas->thread_num > MAX_THREAD_NUM)) {
        cout << "thread_num param err." << endl;
        return -1;
    }
    threadParas->roiEnable = atoi(argv[6]);
    if ((threadParas->roiEnable != 0) && (threadParas->roiEnable != 1)) {
        cout << "roi_enable param err." << endl;
        return -1;
    }
    threadParas->yuvEnable = atoi(argv[5]);
    if ((threadParas->yuvEnable != 0) && (threadParas->yuvEnable != 1)) {
        cout << "yuv_enable param err." << endl;
        return -1;
    }

    /* dec+vpp+enc threads + stat thread */
    g_thread_num = 2 * threadParas->thread_num + 1;

    for (int i = 1; i < threadParas->thread_num; i++) {
        memcpy(&threadParas[i], &threadParas[0], sizeof(THREAD_ARG));
    }

    int td_index = 0;
    /* Initialize multiple threads */
    for (; threadParas->thread_num > 0; threadParas->thread_num--){
        threadParas[td_index].thread_index = td_index;

#ifdef WIN32
        load_thread[td_index] = CreateThread(NULL, 0, videoLoadThread, threadPara, 0, NULL);
#else
        ret = pthread_create(&(load_thread[td_index]), NULL, videoLoadThread, &threadParas[td_index]);
#endif
        if (ret != 0) {
            return -1;
        }

        while (!threadParas[td_index].thread_iscreated) {
            usleep(20);
        }
        td_index++;
    }

    threadParas->thread_num = td_index;
#ifdef WIN32
        displayfps_thread = CreateThread(NULL, 0, displayFpsThread, threadPara, 0, NULL);
#else
        ret = pthread_create(&displayfps_thread, NULL, displayFpsThread, threadParas);
#endif
    if (ret != 0) {
        return -1;
    }

    for (int i = 0; i < threadParas->thread_num; i++) {
#ifdef WIN32
        WaitForSingleObject(load_thread[i], INFINITE);
		WaitForSingleObject(write_thread[i], INFINITE);
#else
        pthread_join(load_thread[i], NULL);
		pthread_join(write_thread[i], NULL);
#endif
    }

#ifdef WIN32
        WaitForSingleObject(displayfps_thread, INFINITE);
#else
    pthread_join(displayfps_thread, NULL);
#endif

    for (int i = 0; i < threadParas->thread_num; i++) {
        exit_flag[i] = 1;
        if (g_writer[i].isOpened())
            g_writer[i].release();
        if(threadParas[i].imageQueue){
            delete threadParas[i].imageQueue;
            threadParas[i].imageQueue = NULL;
        }
    }
    if(threadParas){
        free(threadParas);
        threadParas = NULL;
    }

    cout<<"VideoCapture has been released! VideoWriter has been released! Main function exit"<<endl;
    return 0;
}
