/*****************************************************************************
 *
 *    Copyright (C) 2022 Sophgo Technologies Inc.  All rights reserved.
 *
 *    bmvid is licensed under the 2-Clause BSD License except for the
 *    third-party components.
 *
 *****************************************************************************/

#include <stdio.h>
#ifdef __linux__
#include <getopt.h>
#endif
#include "osal.h"
#include "codec_process.h"
#define MAX_THREAD_NUM 18

void *bm_Image_queue[MAX_THREAD_NUM];
int g_enable_putText = 0;

pthread_mutex_t bmImgQue;

bm_handle_t g_bmHandle        = {0};
#define VPU_ALIGN16(_x)             (((_x)+0x0f)&~0x0f)
#define VPU_ALIGN32(_x)             (((_x)+0x1f)&~0x1f)
#define VPU_ALIGN64(_x)             (((_x)+0x3f)&~0x3f)
#define VPU_ALIGN256(_x)            (((_x)+0xff)&~0xff)
#define VPU_ALIGN4096(_x)           (((_x)+0xfff)&~0xfff)

#define INTERVAL 1
#define defaultReadBlockLen 0x80000
int readBlockLen = defaultReadBlockLen;
int injectionPercent = 0;
int injectLost = 1; // default as lost data, or scramble the data
int injectWholeBlock = 0;
uint64_t count_dec[MAX_NUM_INSTANCE * MAX_NUM_VPU_CORE];
double fps_dec[MAX_NUM_INSTANCE * MAX_NUM_VPU_CORE];
int g_exit_flag = 0;
int g_stop_flag[MAX_NUM_INSTANCE] = {0};

// static bmcv_point_t org = {500, 500};
// static int fontFace = 0;
// static int fontScale = 5;
// static const char text[30] = "Hello, world!";
// static unsigned char color[3] = {255, 0, 0};
// static int thickness = 2;

int global_ret = 0;

static int parse_args(int argc, char **argv, BMTestConfig* par);

static void *stat_pthread(void *arg)
{
    int thread_num = *(int*)arg;
    int i = 0;
    uint64_t last_count_dec[MAX_NUM_INSTANCE * MAX_NUM_VPU_CORE] = {0};
    uint64_t last_count_sum = 0;
    int dis_mode = 0;
    char *display = getenv("BMVPUDEC_DISPLAY_FRAMERATE");
    if (display) dis_mode = atoi(display);
    VLOG(INFO, "BMVPUDEC_DISPLAY_FRAMERATE=%d thread_num=%d g_exit_flag=%d \n", dis_mode, thread_num, g_exit_flag);
    while(!g_exit_flag) {
#ifdef __linux__
        sleep(INTERVAL);
#elif _WIN32
        Sleep(INTERVAL*1000);
#endif
        if (dis_mode == 1) {
            for (i = 0; i < thread_num; i++) {
                if (i == 0) {
                    VLOG(INFO, "ID[%d],   FRM[%10lld], FPS[%2.2lf]\n",
                        i, (long long)count_dec[i], ((double)(count_dec[i]-last_count_dec[i]))/INTERVAL);
                } else {
                    VLOG(INFO, "ID[%d] ,  FRM[%10lld], FPS[%2.2lf]  \n",
                        i, (long long)count_dec[i], ((double)(count_dec[i]-last_count_dec[i]))/INTERVAL);
                }
                last_count_dec[i] = count_dec[i];
            }
        } else {
            uint64_t count_sum = 0;
            for (i = 0; i < thread_num; i++)
              count_sum += count_dec[i];
            VLOG(INFO, "thread %d, frame %ld, fps %2.2f\n", thread_num, count_sum, ((double)(count_sum-last_count_sum))/INTERVAL);
            last_count_sum = count_sum;
        }
    }
    VLOG(INFO, "stat_pthread over.\n");
    return NULL;
}

int map_bmFformat_to_bmformat(int bmFformat) {
    int format;
    switch (bmFformat)
    {
    case BM_VPU_DEC_PIX_FORMAT_YUV420P:
        format = FORMAT_YUV420P;
        break;
    case BM_VPU_DEC_PIX_FORMAT_NV12:
        format = FORMAT_NV12;
        break;
    case BM_VPU_DEC_PIX_FORMAT_NV21:
        format = FORMAT_NV21;
        break;
    default:
        printf("unsupported bm_pix_format: %d\n", bmFformat);
        return -1;
    }
    return format;
}

int bmFrame_to_bmImage(BMVidFrame *in, bm_image *out) {
    int plane = 0;
    int data_five_denominator = -1;
    int data_six_denominator = -1;

    switch (in->pixel_format)
    {
    case BM_VPU_DEC_PIX_FORMAT_YUV420P:
        plane = 3;
        data_five_denominator = 4;
        data_six_denominator = 4;
        break;
    case BM_VPU_DEC_PIX_FORMAT_NV12:
    case BM_VPU_DEC_PIX_FORMAT_NV21:
        plane = 2;
        data_five_denominator = 2;
        data_six_denominator = -1;
        break;
    case BM_VPU_DEC_PIX_FORMAT_COMPRESSED:
        break;
    default:
        VLOG(INFO, "unsupported format, only yuv420p, nv12, nv21 supported\n");
        break;
    }

    if (in->pixel_format == BM_VPU_DEC_PIX_FORMAT_COMPRESSED) {
        if ((0 == in->width) || (0 == in->height) || \
            (0 == in->stride[0]) || (0 == in->stride[1]) || (0 == in->stride[2]) || (0 == in->stride[3]) || \
            (0 == in->buf[0]) || (0 == in->buf[1]) || (0 == in->buf[2]) || (0 == in->buf[3])) {
                VLOG(ERR, "bm_image_from_frame: get yuv failed\n");
                return BM_ERR_PARAM;
            }
        bm_image cmp_bmImg;
        bm_image_create(g_bmHandle, in->height, in->width, FORMAT_COMPRESSED, DATA_TYPE_EXT_1N_BYTE, &cmp_bmImg, NULL);
        bm_device_mem_t input_addr[4];
        int size = in->height * in->stride[4];
        input_addr[0] = bm_mem_from_device((unsigned long long)in->buf[6], size);
        size = (in->height / 2) * in->stride[5];
        input_addr[1] = bm_mem_from_device((unsigned long long)in->buf[4], size);
        size = in->stride[6];
        input_addr[2] = bm_mem_from_device((unsigned long long)in->buf[7], size);
        size = in->stride[7];
        input_addr[3] = bm_mem_from_device((unsigned long long)in->buf[5], size);
        bm_image_attach(cmp_bmImg, input_addr);
        bm_image_create(g_bmHandle, in->height, in->width, FORMAT_YUV420P, DATA_TYPE_EXT_1N_BYTE, out, NULL);
        int ret = bm_image_alloc_dev_mem(*out, BMCV_HEAP_ANY);
        if (ret != BM_SUCCESS) {
            VLOG(ERR, "bm_image_alloc_dev_mem_out failed\n");
            return BM_ERR_PARAM;
        }
        bmcv_rect_t rect = {0, 0, (unsigned int)in->width, (unsigned int)in->height};
        bmcv_image_vpp_convert(g_bmHandle, 1, cmp_bmImg, out, &rect, BMCV_INTER_LINEAR);
        bm_image_destroy(&cmp_bmImg);
    } else {
        int stride[3];
        bm_image_format_ext bm_format;
        bm_device_mem_t input_addr[3] = {0};
        if (plane == 2) {
            if ((0 == in->width) || (0 == in->height) || \
                (0 == in->stride[4]) || (0 == in->buf[4]) || \
                (0 == in->stride[5]) || (0 == in->buf[5])) {
                return BM_ERR_PARAM;
            }
            stride[0] = in->stride[4];
            stride[1] = in->stride[5];
        } else if (plane == 3) {
            if ((0 == in->width) || (0 == in->height) || \
                (0 == in->stride[4]) || (0 == in->buf[4]) || \
                (0 == in->stride[5]) || (0 == in->buf[5]) || \
                (0 == in->stride[6]) || (0 == in->buf[6])) {
                return BM_ERR_PARAM;
            }
            stride[0] = in->stride[4];
            stride[1] = in->stride[5];
            stride[2] = in->stride[6];
        }
        bm_format = (bm_image_format_ext)map_bmFformat_to_bmformat((int)(in->pixel_format));
        bm_image_create(g_bmHandle, in->height, in->width, bm_format, DATA_TYPE_EXT_1N_BYTE, out, stride);
        int size = in->height * stride[0];
        input_addr[0] = bm_mem_from_device((unsigned long long)in->buf[4], size);
        if(data_five_denominator != -1 ){
            input_addr[1] = bm_mem_from_device((unsigned long long)in->buf[5], size/data_five_denominator);
        }
        if(data_six_denominator != -1){
            input_addr[2] = bm_mem_from_device((unsigned long long)in->buf[6], size/data_six_denominator);
        }
        bm_image_attach(*out, input_addr);
    }

    return BM_SUCCESS;
}
void *dec_get_process(void* arg)
{
    int Ret = 0;
    BMTestConfig *testConfigPara = (BMTestConfig *)arg;
    BMVidCodHandle vidCodHandle = (BMVidCodHandle)testConfigPara->vidCodHandle;
    int instanceId = testConfigPara->instanceNum;
    struct timeval TimeoutVal;
    fd_set read_fds;
    int fd;

    BMVidFrame *pFrame=NULL;
    uint8_t* pRefMem = NULL;
    int cur_frame_idx = 0;
    uint64_t dec_time;
    VLOG(INFO, "Enter dec_get_process!\n");

    int frame_write_num = testConfigPara->frame_number;
    int enc_enable = 1;

    int core_idx = bmvpu_dec_get_core_idx(vidCodHandle);
    int inst_idx = bmvpu_dec_get_inst_idx(vidCodHandle);

    uint64_t start_time = osal_gettime();
    pFrame = (BMVidFrame *)malloc(sizeof(BMVidFrame));
    fd = bmvpu_dec_get_device_fd(vidCodHandle);

    bm_image *bmImagein = NULL;
    bmImagein = (bm_image*)malloc(sizeof(bm_image));
    //TODO - add enc_format check
    while(testConfigPara->bStop != 1)
    {
        TimeoutVal.tv_sec = 1;
        TimeoutVal.tv_usec = 0;
        FD_ZERO(&read_fds);
        FD_SET(fd, &read_fds);
        Ret = select(fd + 1, &read_fds, NULL, NULL, &TimeoutVal);
        if(Ret < 0){
            continue;
        }

        if((Ret = bmvpu_dec_get_output(vidCodHandle, pFrame)) == BM_SUCCESS)
        {
            bm_image *bmImageout = NULL;
            bmImageout = (bm_image*)malloc(sizeof(bm_image));
            bm_image_create(g_bmHandle, pFrame->height, pFrame->width, FORMAT_YUV420P, DATA_TYPE_EXT_1N_BYTE, bmImageout, NULL);
            bm_image_alloc_dev_mem(*bmImageout, BMCV_HEAP1_ID);
           if (!bmImageout) {
                global_ret = -1;
                bm_image_destroy(bmImageout);
                free(bmImageout);
                break;
            }

            if(cur_frame_idx != 0 && (cur_frame_idx % 1000) == 0) {
                dec_time = osal_gettime();
                if(dec_time - start_time != 0) {
                    fps_dec[testConfigPara->instanceNum] = (float)(cur_frame_idx+1)*1000/(dec_time-start_time);
                }
            }

            // collect bmcv bm_image
            if (enc_enable && frame_write_num != 0) {
                if (bmFrame_to_bmImage(pFrame, bmImagein) != BM_SUCCESS) {
                    global_ret = -1;
                    bm_image_destroy(bmImagein);
                    bm_image_destroy(bmImageout);
                    free(bmImageout);
                    break;
                }
                int ret = bmcv_image_rotate(g_bmHandle, *bmImagein, *bmImageout, 180);
                if (ret != BM_SUCCESS){
                    VLOG(ERR, "bm_image rotate failed !\n");
                    global_ret = -1;
                    bm_image_destroy(bmImagein);
                    bm_image_destroy(bmImageout);
                    free(bmImageout);
                    break;
                }
                while (bm_queue_is_full(bm_Image_queue[instanceId])){
                    usleep(10);
                }
                pthread_mutex_lock(&bmImgQue);
                bm_queue_push(bm_Image_queue[instanceId], &bmImageout);
                pthread_mutex_unlock(&bmImgQue);
                if (frame_write_num > 0){
                    frame_write_num--;
                }
            }

            cur_frame_idx += 1;
            bm_image_destroy(bmImagein);
            bmvpu_dec_clear_output(vidCodHandle, pFrame);
            count_dec[testConfigPara->instanceNum]++;
        }
        else
        {
            if(Ret == BM_ERR_VDEC_ILLEGAL_PARAM){
                global_ret = -1;
                break;
            }
#ifdef __linux__
            usleep(1000);
#elif _WIN32
            Sleep(100);
#endif
        }
        usleep(100);
    }

    VLOG(INFO, "Exit chn %d core %d dec_get_process!\n", inst_idx, core_idx);
    if(pRefMem)
        free(pRefMem);
    if(pFrame != NULL)
    {
        free(pFrame);
        pFrame = NULL;
    }
    g_stop_flag[instanceId] = 1;
    free(bmImagein);

    return NULL;
}

void *dec_send_process(void* arg)
{
    FILE* fpIn;
    uint8_t* pInMem;
    int32_t readLen = -1;
    BMVidStream vidStream;
    BMVidDecParam param = {0};
    BMTestConfig *testConfigPara = (BMTestConfig *)arg;
    BMTestConfig dec_get_process_Para;
    BMVidCodHandle vidHandle;
    char *inputPath = (char *)testConfigPara->inputPath;
    pthread_t vpu_thread;
    struct timeval tv;
    int wrptr = 0;
    uint8_t bFindStart, bFindEnd;
    int UsedBytes, FreeLen;
    int frame_size_coeff = 1;
    int eof_flag = 0;
    int ret = 0;
    bm_handle_t bm_handle = NULL;

    // testConfigPara->bsMode = 1;
    memcpy(&dec_get_process_Para, testConfigPara, sizeof(BMTestConfig));

    fpIn = fopen(inputPath, "rb");
    if(fpIn==NULL)
    {
        VLOG(ERR, "Can't open input file: %s\n", inputPath);
        ret = -1;
        return NULL;
    }

    if(testConfigPara->streamFormat != BMDEC_AVC && testConfigPara->streamFormat != BMDEC_HEVC) {
        VLOG(ERR, "Error: the stream type is invalid!\n");
        global_ret = -1;
        return NULL;
    }
    param.streamFormat = testConfigPara->streamFormat;
    param.wtlFormat = testConfigPara->wtlFormat;
    VLOG(INFO, "param.wtlFormat = %d\n", param.wtlFormat);
    param.extraFrameBufferNum = 1;
    param.streamBufferSize = 0x500000;
    param.enable_cache = 1;
    param.bsMode = testConfigPara->bsMode;   /* VIDEO_MODE_STREAM */
    param.core_idx=-1;
    param.picWidth = testConfigPara->picWidth;
    param.picHeight = testConfigPara->picHeight;
    param.cmd_queue_depth = testConfigPara->cmd_queue;
    if(testConfigPara->cbcr_interleave == 1) {
        if(testConfigPara->nv21 == 1)
            param.pixel_format = BM_VPU_DEC_PIX_FORMAT_NV21;
        else
            param.pixel_format = BM_VPU_DEC_PIX_FORMAT_NV12;
    }
    else {
        param.pixel_format = BM_VPU_DEC_PIX_FORMAT_YUV420P;
    }

#ifdef    BM_PCIE_MODE
    param.pcie_board_id = testConfigPara->pcie_board_id;
#endif
    if (bmvpu_dec_create(&vidHandle, param)!=(BMVidDecRetStatus)BM_SUCCESS)
    {
        VLOG(ERR, "Can't create decoder.\n");
        global_ret = -1;
        return NULL;
    }

    VLOG(INFO, "Decoder Create success, chn %d, inputpath %s!\n", (int)*(int *)vidHandle, inputPath);

    pInMem = (unsigned char *)malloc(defaultReadBlockLen);
    if(pInMem == NULL)
    {
        VLOG(ERR, "Can't get input memory\n");
        global_ret = -1;
        return NULL;
    }

    vidStream.buf = pInMem;
    vidStream.header_size = 0;
    vidStream.pts = 0;

    //int core_idx = bmvpu_dec_get_core_idx(vidHandle);
    //int inst_idx = bmvpu_dec_get_inst_idx(vidHandle);
    dec_get_process_Para.vidCodHandle = vidHandle;
    dec_get_process_Para.bStop = 0;

    pthread_create(&vpu_thread, NULL, dec_get_process, (void*)&dec_get_process_Para);

#ifdef __linux__
    gettimeofday(&tv, NULL);
    srand((unsigned int)tv.tv_usec);
#elif _WIN32
    SYSTEMTIME wtm;
    GetLocalTime(&wtm);
    srand((unsigned int)wtm.wSecond);
#endif

    for (int i = 0; i < testConfigPara->press_num; i++) {
        VLOG(INFO, "total_loop = %d, current_loop: %d\n", testConfigPara->press_num, i+1);

        UsedBytes = 0;
        while (1)
        {
            /* code */
            if(testConfigPara->bsMode == 0){ /* BS_MODE_INTERRUPT */
#ifdef BM_PCIE_MODE
                memset(pInMem,0,defaultReadBlockLen);
#endif
                wrptr = 0;
                do{
                    FreeLen = (defaultReadBlockLen-wrptr) >readBlockLen? readBlockLen:(defaultReadBlockLen-wrptr);
                    if( FreeLen == 0 )
                        break;
                    if((readLen = fread(pInMem+wrptr, 1, FreeLen, fpIn))>0)
                    {
                        int toLost = 0;
                        if(readLen != FreeLen)
                            vidStream.end_of_stream = 1;
                        else
                            vidStream.end_of_stream = 0;

                        if(injectionPercent == 0)
                        {
                            wrptr += readLen;
                            if(testConfigPara->result == FALSE)
                            {
                                goto OUT1;
                            }
                        }
                        else{
                            if((rand()%100) <= injectionPercent)
                            {
                                if(injectWholeBlock)
                                {
                                    toLost = readLen;
                                    if (injectLost)
                                        continue;
                                    else
                                    {
                                        for(int j=0;j<toLost;j++)
                                            *((char*)(pInMem+wrptr+j)) = rand()%256;
                                        wrptr += toLost;
                                    }
                                }
                                else{
                                    toLost = rand()%readLen;
                                    while(toLost == readLen)
                                    {
                                        toLost = rand()%readLen;
                                    }
                                    if(injectLost)
                                    {
                                        readLen -= toLost;
                                        memset(pInMem+wrptr+readLen, 0, toLost);
                                    }
                                    else
                                    {
                                        for(int j=0;j<toLost;j++)
                                            *((char *)(pInMem+wrptr+readLen-toLost+j)) = rand()%256;
                                    }
                                    wrptr += readLen;
                                }
                            }
                            else
                            {
                                wrptr += readLen;
                            }
                        }
                    }
                }while(readLen > 0);
//1682 pcie card cdma need 4B aligned
#ifdef BM_PCIE_MODE
                readLen = (wrptr + 3)/4*4;
                vidStream.length = readLen;
#else
                vidStream.length = wrptr;
#endif
            }
            else{
                bFindStart = 0;
                bFindEnd = 0;
                int i;

GET_BITSTREAM_DATA:
                global_ret = fseek(fpIn, UsedBytes, SEEK_SET);
                readLen = fread(pInMem, 1, defaultReadBlockLen * frame_size_coeff, fpIn);
                if(readLen == 0){
                    break;
                }
                if (readLen < (defaultReadBlockLen * frame_size_coeff))
                    eof_flag = 1;

                if(testConfigPara->streamFormat == 0) { /* H264 */
                    for (i = 0; i < readLen - 8; i++) {
                        int tmp = pInMem[i + 3] & 0x1F;

                        if (pInMem[i] == 0 && pInMem[i + 1] == 0 && pInMem[i + 2] == 1 &&
                            (((tmp == 0x5 || tmp == 0x1) && ((pInMem[i + 4] & 0x80) == 0x80)) ||
                            (tmp == 20 && (pInMem[i + 7] & 0x80) == 0x80))) {
                            bFindStart = 1;
                            i += 8;
                            break;
                        }
                    }

                    for (; i < readLen - 8; i++) {
                        int tmp = pInMem[i + 3] & 0x1F;

                        if (pInMem[i] == 0 && pInMem[i + 1] == 0 && pInMem[i + 2] == 1 &&
                            (tmp == 15 || tmp == 7 || tmp == 8 || tmp == 6 ||
                                ((tmp == 5 || tmp == 1) && ((pInMem[i + 4] & 0x80) == 0x80)) ||
                                (tmp == 20 && (pInMem[i + 7] & 0x80) == 0x80))) {
                            bFindEnd = 1;
                            break;
                        }
                    }

                    if (i > 0)
                        readLen = i;
                    if (bFindStart == 0) {
                        printf("chn %d can not find H264 start code!readLen %d, s32UsedBytes %d.!\n",
                                (int)*((int *)vidHandle), readLen, UsedBytes);
                    }
                    if (bFindEnd == 0) {
                        readLen = i + 8;
                    }
                }
                else if(testConfigPara->streamFormat == 12) { /* H265 */
                    int bNewPic = 0;

                    for(i=0; i<readLen-6; i++){
                        int tmp = (pInMem[i + 3] & 0x7E) >> 1;

                        bNewPic = (pInMem[i + 0] == 0 && pInMem[i + 1] == 0 && pInMem[i + 2] == 1 &&
                       (tmp <= 21) && ((pInMem[i + 5] & 0x80) == 0x80));

                        if (bNewPic) {
                            bFindStart = 1;
                            i += 6;
                            break;
                        }
                    }

                    for (; i < readLen - 6; i++) {
                        int tmp = (pInMem[i + 3] & 0x7E) >> 1;

                        bNewPic = (pInMem[i + 0] == 0 && pInMem[i + 1] == 0 && pInMem[i + 2] == 1 &&
                            (tmp == 32 || tmp == 33 || tmp == 34 || tmp == 39 || tmp == 40 ||
                                ((tmp <= 21) && (pInMem[i + 5] & 0x80) == 0x80)));

                        if (bNewPic) {
                            bFindEnd = 1;
                            break;
                        }
                    }
                    if (i > 0)
                        readLen = i;

                    if (bFindEnd == 0) {
                        readLen = i + 6;
                    }
                }

                if (bFindEnd==0 && eof_flag==0) {
                    frame_size_coeff++;
                    VLOG(INFO, "chn %d need more data to analyze! coeff=%d\n", (int)*((int *)vidHandle), frame_size_coeff);
                    if (pInMem)
                        free(pInMem);

                    pInMem = (unsigned char *)malloc(defaultReadBlockLen * frame_size_coeff);
                    vidStream.buf = pInMem;
                    memset(pInMem, 0, defaultReadBlockLen * frame_size_coeff);
                    goto GET_BITSTREAM_DATA;
                }

                vidStream.length = readLen;
                vidStream.end_of_stream = 0;
            }

            int result = 0;
            while((result = bmvpu_dec_decode(vidHandle, vidStream))!=BM_SUCCESS){
                if(result == BM_ERR_VDEC_ILLEGAL_PARAM){
                    VLOG(ERR, "Error: stream param error.\n");
                    global_ret = -1;
                    goto OUT2;
                }
#ifdef __linux__
                usleep(1000);
#elif _WIN32
                Sleep(1);
#endif
            }

            if(testConfigPara->bsMode == 0){
                if (wrptr < defaultReadBlockLen){
                    break;
                }
            }
            else{
                UsedBytes += readLen;
                vidStream.pts += 30;
            }
            usleep(1000);
        }
        fseek(fpIn, 0, SEEK_SET);
        eof_flag = 0;
    }
OUT1:
    bmvpu_dec_flush(vidHandle);
    while ((ret = bmvpu_dec_get_status(vidHandle)) != BMDEC_STOP)
    {
        if(/*ret == BMDEC_FRAMEBUFFER_NOTENOUGH || */ret == BMDEC_WRONG_RESOLUTION) {
            global_ret = -1;
            break;
        }
#ifdef __linux__
        usleep(2);
#elif _WIN32
        Sleep(1);
#endif
    }
OUT2:
    dec_get_process_Para.bStop = 1;

    pthread_join(vpu_thread, NULL);
    VLOG(INFO, "EXIT\n");
    bmvpu_dec_delete(vidHandle);
    free(pInMem);
    pInMem = NULL;

    if(bm_handle != NULL)
        bm_dev_free(bm_handle);
    fclose(fpIn);
    return NULL;
}

static void
Help(const char *programName)
{
    fprintf(stderr, "------------------------------------------------------------------------------\n");
    // fprintf(stderr, "%s(API v%d.%d.%d)\n", programName, API_VERSION_MAJOR, API_VERSION_MINOR, API_VERSION_PATCH);
    fprintf(stderr, "%s\n", programName);
    fprintf(stderr, "\tAll rights reserved by Bitmain\n");
    fprintf(stderr, "------------------------------------------------------------------------------\n");
    fprintf(stderr, "%s [option] --input bistream\n", programName);
    fprintf(stderr, "-h                 help\n");
    fprintf(stderr, "-v                 set max log level\n");
    fprintf(stderr, "                   0: none, 1: error(default), 2: warning, 3: info, 4: trace\n");
    fprintf(stderr, "-m                 bitstream mode\n");
    fprintf(stderr, "                   0 : BS_MODE_INTERRUPT\n");
    fprintf(stderr, "                   2 : BS_MODE_PIC_END\n");
    fprintf(stderr, "-y                 luma stride (optional)\n");
    fprintf(stderr, "--dec_width        decode input width\n");
    fprintf(stderr, "--dec_height       decode input height\n");
    fprintf(stderr, "--input            bitstream path\n");
    fprintf(stderr, "--output           encode stream path\n");
    fprintf(stderr, "--InterYuv         decode output yuv path, to add in the future\n");
    fprintf(stderr, "--cbcr_interleave  chorma interleave. default 0.\n");
    fprintf(stderr, "--nv21             nv21 output. default 0.\n");
    fprintf(stderr, "--stream-type      0,12, default 0 (H.264:0, H.265:12)\n");
    fprintf(stderr, "--instance         instance number\n");
    fprintf(stderr, "--extraFrame       extra frame nums. default 2.\n");
    fprintf(stderr, "--write_yuv        0 no writing , num write frame numbers\n");
    fprintf(stderr, "--wtl-format       yuv format. default 0.\n");
    fprintf(stderr, "--read-block-len   block length of read from file, default is 0x80000\n");
    fprintf(stderr, "--pix_fmt          pixel format. 0: YUV420(default); 1: NV12\n");
    fprintf(stderr, "--codec            video encoder. 0: H.264; 1: H.265(default)\n");
    fprintf(stderr, "--intra            intra period. Default value is 28.\n");
    fprintf(stderr, "--gop_preset       gop preset index\n");
    fprintf(stderr, "       1: all I,          all Intra, gopsize = 1\n");
    fprintf(stderr, "       2: I-P-P,          consecutive P, cyclic gopsize = 1\n");
    fprintf(stderr, "       3: I-B-B-B,        consecutive B, cyclic gopsize = 1\n");
    fprintf(stderr, "       4: I-B-P-B-P,      gopsize = 2\n");
    fprintf(stderr, "       5: I-B-B-B-P,      gopsize = 4 (default)\n");
    fprintf(stderr, "       6: I-P-P-P-P,      consecutive P, cyclic gopsize = 4\n");
    fprintf(stderr, "       7: I-B-B-B-B,      consecutive B, cyclic gopsize = 4\n");
    fprintf(stderr, "       8: random access, I-B-B-B-B-B-B-B-B, cyclic gopsize = 8\n");
    fprintf(stderr, "       Low delay cases are 1, 2, 3, 6, 7 \n");
    fprintf(stderr, "--enc_width        encode actual width\n");
    fprintf(stderr, "--enc_height       encode actual height\n");
    fprintf(stderr, "--fps              framerate, default 30\n");
    fprintf(stderr, "--frame_number     output frame_number\n");
    fprintf(stderr, "For example,\n");
    fprintf(stderr, "bm_test --input 240p.265 --output 240p.264 --enc_width 1920 --enc_height 1080\n");
}


int main(int argc, char **argv)
{
    int i = 0;
    pthread_t vpu_thread[MAX_NUM_INSTANCE * MAX_NUM_VPU_CORE];
    pthread_t enc_thread[MAX_NUM_INSTANCE];
    pthread_t monitor_thread;
    BMTestConfig *testConfigPara = malloc(MAX_NUM_INSTANCE * MAX_NUM_VPU_CORE * sizeof(BMTestConfig));//[MAX_NUM_INSTANCE * MAX_NUM_VPU_CORE];
    BMTestConfig testConfigOption;
    memset(&testConfigOption, 0, sizeof(BMTestConfig));
    parse_args(argc, argv, &testConfigOption);
    bmvpu_dec_set_logging_threshold(testConfigOption.log_level);
    bmvpu_enc_set_logging_function(logging_fn);

    unsigned int chipid = 0;
    int ret = 0;
    ret = bm_dev_request(&g_bmHandle, 0);
    if (ret != BM_SUCCESS){
        VLOG(ERR, "bm_dev_request failed !\n");
        return -1;
    }
    bm_get_chipid(g_bmHandle, &chipid);

    for(i = 0; i < testConfigOption.instanceNum; i++){
        memset(&testConfigPara[i], 0, sizeof(BMTestConfig));
        bm_Image_queue[i] = bm_queue_create(10, sizeof(bm_image*));
    }

    for(i = 0; i < testConfigOption.instanceNum; i++)
    {
        memcpy(&(testConfigPara[i]), &(testConfigOption), sizeof(BMTestConfig));
        testConfigPara[i].instanceNum = i;
        testConfigPara[i].result = TRUE;
    }
    for(i = 0; i < testConfigOption.instanceNum; i++)
    {
        pthread_create(&vpu_thread[i], NULL, dec_send_process, (void*)&testConfigPara[i]);
    }

    for(i = 0; i < testConfigOption.instanceNum; i++)
    {
        pthread_create(&enc_thread[i], NULL, enc_process, (void*)&testConfigPara[i]);
    }

    ret = pthread_create(&monitor_thread, NULL, stat_pthread, (void*)&testConfigOption.instanceNum);
    if (ret != 0) {
        VLOG(INFO, "Error creating monitor thread: %d\n", ret);
        return -1;
    }

    for(i = 0; i < testConfigOption.instanceNum; i++)
    {
        pthread_join(vpu_thread[i], NULL);
    }

    for(i = 0; i < testConfigOption.instanceNum; i++)
    {
        pthread_join(enc_thread[i], NULL);
    }
    VLOG(INFO, "set g_exit_flag=1\n");
    g_exit_flag = 1;
    pthread_join(monitor_thread, NULL);

    for(i = 0; i < testConfigOption.instanceNum; i++){
        bm_queue_destroy(bm_Image_queue[i]);
        bm_Image_queue[i] = NULL;
    }

    bm_dev_free(g_bmHandle);
    free(testConfigPara);
    return global_ret;
}

static struct option   options[] = {
    {"output",                1, NULL, 0},
    {"input",                 1, NULL, 0},
    {"codec",                 1, NULL, 0},
    {"stream-type",           1, NULL, 0},
    {"InterYuv",              1, NULL, 0},
    {"instance",              1, NULL, 0},
    {"write_yuv",             1, NULL, 0},
    {"wtl-format",            1, NULL, 0},
    {"comp-skip",             1, NULL, 0},
    {"read-block-len",        1, NULL, 0},
    {"inject-percent",        1, NULL, 0},
    {"inject-lost",           1, NULL, 0},
    {"inject-whole-block",    1, NULL, 0},
    {"cbcr_interleave",       1, NULL, 0},
    {"nv21",                  1, NULL, 0},
    {"dec_width",             1, NULL, 0},
    {"dec_height",            1, NULL, 0},
    {"min_frame_cnt",         1, NULL, 0},
    {"frame_delay",           1, NULL, 0},
    {"cmd_queue",             1, NULL, 0},
    {"extraFrame",            1, NULL, 0},
    {"frame_number",          1, NULL, 0},
    {"soc",                   1, NULL, 0},
    {"pix_fmt",               1, NULL, 0},
    {"codec",                 1, NULL, 0},
    {"gop_preset",            1, NULL, 0},
    {"enc_width",             1, NULL, 0},
    {"enc_height",            1, NULL, 0},
    {"fps",                   1, NULL, 0},
    {NULL,                    0, NULL, 0},
};

static int parse_args(int argc, char **argv, BMTestConfig* par)
{
    int index;
    int32_t opt;

    /* default setting. */
    memset(par, 0, sizeof(BMTestConfig));

    par->instanceNum = 1;
    par->log_level = BMVPU_DEC_LOG_LEVEL_ERR;
    par->streamFormat = 0; // H264   0 264  12 265
    par->bsMode = 0;
    par->press_num = 1;
    par->extraFrame = 2;

    // enc param
    par->enc.soc_idx = 0;
    par->enc.enc_fmt = 1;
    par->enc.gop_preset = BM_VPU_ENC_GOP_PRESET_IBBBP;
    par->enc.intra_period = 28;
    par->enc.bit_rate = 0;
    par->enc.cqp = 32;
    par->enc.fps = 30;


    while ((opt=getopt_long(argc, argv, "v:h:m:y:p:", options, &index)) != -1)
    {
        switch (opt)
        {
        case 'm':
            par->bsMode = atoi(optarg);
            break;
        case 'y':
            par->enc.y_stride = atoi(optarg);
            break;
        case 'p':
            par->press_num = atoi(optarg);
            break;
        case 0:
            if (!strcmp(options[index].name, "output"))
            {
                memcpy(par->outputPath, optarg, strlen(optarg));
                // ChangePathStyle(par->outputPath);
            }
            else if (!strcmp(options[index].name, "input"))
            {
                memcpy(par->inputPath, optarg, strlen(optarg));
                // ChangePathStyle(par->inputPath);
            }
            else if (!strcmp(options[index].name, "InterYuv"))
            {
                memcpy(par->InterYuvPath, optarg, strlen(optarg));
                // ChangePathStyle(par->refYuvPath);
            }
            else if (!strcmp(options[index].name, "stream-type"))
            {
                par->streamFormat = (int)atoi(optarg);
            }
            else if (!strcmp(options[index].name, "instance"))
            {
                par->instanceNum = atoi(optarg);
            }
            else if (!strcmp(options[index].name, "write_yuv"))
            {
                par->wirteYuv = atoi(optarg);
            }
            else if (!strcmp(options[index].name, "wtl-format"))
            {
                par->wtlFormat = (int)atoi(optarg);
            }
            else if(!strcmp(options[index].name, "read-block-len"))
            {
                readBlockLen = atoi(optarg);
            }
            else if(!strcmp(options[index].name, "inject-percent"))
            {
                injectionPercent = atoi(optarg);
            }
            else if(!strcmp(options[index].name, "inject-lost"))
            {
                injectLost = atoi(optarg);
            }
            else if(!strcmp(options[index].name, "inject-whole-block"))
            {
                injectWholeBlock = atoi(optarg);
            }
            else if (!strcmp(options[index].name, "cbcr_interleave"))
            {
                par->cbcr_interleave = atoi(optarg);
            }
            else if (!strcmp(options[index].name, "nv21"))
            {
                par->nv21 = atoi(optarg);
            }
            else if(!strcmp(options[index].name, "dec_height"))
            {
                par->picHeight = atoi(optarg);
            }
            else if(!strcmp(options[index].name, "dec_width"))
            {
                par->picWidth = atoi(optarg);
            }
            else if (!strcmp(options[index].name, "extraFrame"))
            {
                par->extraFrame = atoi(optarg);
            }
            else if (!strcmp(options[index].name, "frame_number"))
            {
                par->frame_number = atoi(optarg);
            }
            else if (!strcmp(options[index].name, "soc"))
            {
                par->enc.soc_idx = atoi(optarg);
            }
            else if (!strcmp(options[index].name, "pix_fmt"))
            {
                par->enc.pix_fmt = atoi(optarg);
            }
            else if (!strcmp(options[index].name, "codec"))
            {
                par->enc.enc_fmt = atoi(optarg);
            }
            else if (!strcmp(options[index].name, "intra"))
            {
                par->enc.intra_period = atoi(optarg);
            }
            else if (!strcmp(options[index].name, "gop_preset"))
            {
                par->enc.gop_preset = atoi(optarg);
            }
            else if (!strcmp(options[index].name, "enc_width"))
            {
                par->enc.crop_w = atoi(optarg);
            }
            else if (!strcmp(options[index].name, "enc_height"))
            {
                par->enc.crop_h = atoi(optarg);
            }
            else if (!strcmp(options[index].name, "fps"))
            {
                par->enc.fps = atoi(optarg);
            }
            break;
        case 'v':
            par->log_level = atoi(optarg);
            break;
        case 'h':
        case '?':
        default:
            fprintf(stderr, "%s\n", optarg);
            Help(argv[0]);
            exit(1);
        }
    }

    if(par->nv21)
        par->cbcr_interleave = 1;

    if (par->instanceNum <= 0 || par->instanceNum > MAX_NUM_INSTANCE * MAX_NUM_VPU_CORE)
    {
        fprintf(stderr, "Invalid instanceNum(%d)\n", par->instanceNum);
        Help(argv[0]);
        exit(1);
    }

    if (par->log_level < BMVPU_DEC_LOG_LEVEL_NONE || par->log_level > BMVPU_DEC_LOG_LEVEL_TRACE)
    {
        fprintf(stderr, "Wrong log level: %d\n", par->log_level);
        Help(argv[0]);
        exit(1);
    }

    if (par->enc.soc_idx < 0)
    {
        fprintf(stderr, "Error! invalid soc_idx (%d)\n",
                par->enc.soc_idx);
        Help(argv[0]);
        return RETVAL_ERROR;
    }

    if (par->enc.soc_idx != 0)
    {
        fprintf(stderr, "[Warning] invalid soc_idx (%d), set soc_idx=0.\n",
                par->enc.soc_idx);
        par->enc.soc_idx = 0;
    }

    if (par->enc.enc_fmt < 0 || par->enc.enc_fmt > 1)
    {
        fprintf(stderr, "enc_fmt is NOT one of 0, 1\n\n");
        Help(argv[0]);
        return RETVAL_ERROR;
    }

    if (par->enc.pix_fmt < 0 || par->enc.pix_fmt > 1)
    {
        fprintf(stderr, "pix_fmt is NOT one of 0, 1\n\n");
        Help(argv[0]);
        return RETVAL_ERROR;
    }

    if (par->enc.crop_w <= 0)
    {
        fprintf(stderr, "width <= 0\n\n");
        Help(argv[0]);
        return RETVAL_ERROR;
    }
    if (par->enc.crop_h <= 0)
    {
        fprintf(stderr, "height <= 0\n\n");
        Help(argv[0]);
        return RETVAL_ERROR;
    }

    if (par->enc.y_stride == 0)
        par->enc.y_stride = par->enc.crop_w;
    else if (par->enc.y_stride < par->enc.crop_w)
    {
        fprintf(stderr, "luma stride is less than width\n\n");
        Help(argv[0]);
        return RETVAL_ERROR;
    }

    if (par->enc.y_stride%8)
    {
        fprintf(stderr, "luma stride is NOT multiple of 8\n\n");
        Help(argv[0]);
        return RETVAL_ERROR;
    }

    if (par->enc.aligned_height == 0)
        par->enc.aligned_height = par->enc.crop_h;
    else if (par->enc.aligned_height < par->enc.crop_h)
    {
        fprintf(stderr, "aligned height is less than actual height\n\n");
        Help(argv[0]);
        return RETVAL_ERROR;
    }

    if (par->enc.pix_fmt == 0)
        par->enc.c_stride = par->enc.y_stride/2;
    else
        par->enc.c_stride = par->enc.y_stride;

    if (par->enc.bit_rate < 0)
        par->enc.bit_rate = 0;

    if (par->enc.fps <= 0 )
        par->enc.fps = 30;

    if (par->enc.cqp < 0 || par->enc.cqp > 51)
    {
        fprintf(stderr, "Invalid quantization parameter %d\n", par->enc.cqp);
        Help(argv[0]);
        return RETVAL_ERROR;
    }

    if (par->frame_number <= 0)
        par->frame_number = 0x7fffffff;

    return 0;
}
