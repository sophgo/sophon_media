#include "codec_process.h"

#define MAX_THREAD_NUM 18
extern int global_ret;
extern int g_exit_flag;
extern void *bm_Image_queue[MAX_THREAD_NUM];
extern pthread_mutex_t bmImgQue;
extern int g_stop_flag[32];
double sta_time[MAX_NUM_INSTANCE * MAX_NUM_VPU_CORE];
long long count_enc[MAX_NUM_INSTANCE * MAX_NUM_VPU_CORE];
#define GET_STREAM_TIMEOUT  30*1000

#define ENC_PKT_DATA_SIZE 1024*1024

static void cleanup_task(void* arg)
{
    VpuEncContext* ctx = (void*)arg;
    printf("Clean up thread 0x%lx\n", pthread_self());
    if (ctx==NULL)
        return;

    /* Close the previously opened encoder instance */
    if (ctx->video_encoder != NULL)
    {
        bmvpu_enc_close(ctx->video_encoder);
        ctx->video_encoder = NULL;
    }

    if (ctx->frame_unused_queue != NULL)
    {
        bm_queue_destroy(ctx->frame_unused_queue);
        ctx->frame_unused_queue = NULL;
    }

    if (ctx->output_frame.data != NULL) {
        free(ctx->output_frame.data);
        ctx->output_frame.data = NULL;
        ctx->output_frame.data_size = 0;
    }

    if (ctx->src_fb_list) {
        free(ctx->src_fb_list);
    }

    if (ctx->src_fb_dmabuffers) {
        free(ctx->src_fb_dmabuffers);
    }

    /* Unload the VPU firmware */
    bmvpu_enc_unload(ctx->soc_idx);

    if (ctx->fout != NULL)
        fclose(ctx->fout);

    if (ctx != NULL)
        free(ctx);

    return;
}

static void* acquire_output_buffer(void *context, size_t size, void **acquired_handle)
{
    ((void)(context));
    void *mem;

    mem = malloc(size);
    *acquired_handle = mem;
    return mem;
}
static void finish_output_buffer(void *context, void *acquired_handle)
{
    ((void)(context));
}

static BmVpuFramebuffer* get_src_framebuffer(VpuEncContext *ctx)
{
    if (bm_queue_is_empty(ctx->frame_unused_queue))
    {
        printf("frame_unused_queue is empty.\n");
        return NULL;
    }

    BmVpuFramebuffer *fb = *((BmVpuFramebuffer**)bm_queue_pop(ctx->frame_unused_queue));
    int log_level;
    int i;

    if (fb==NULL)
    {
        fprintf(stderr, "frame buffer is NULL, pop\n");
        return NULL;
    }

    log_level = bmvpu_enc_get_logging_threshold();
    if (log_level > BMVPU_ENC_LOG_LEVEL_INFO)
    {
#ifdef __linux__
        printf("[%zx] myIndex = 0x%x, %p, pop\n", pthread_self(), fb->myIndex, fb);
#endif
    }
    for (i=0; i<ctx->num_src_fb; i++)
    {
        if (&(ctx->src_fb_list[i]) == fb) {
            return fb;
        }
    }

    return NULL;
}


void *enc_process(void* arg) {
    VpuEncContext* ctx = NULL;
    BmVpuEncOpenParams* eop = NULL;
    BmVpuCodecFormat codec_fmt = BM_VPU_CODEC_FORMAT_H265;
    // int get_stream_time = 0;
    BMTestConfig *enc_testConfigPara = (BMTestConfig *)arg;
    EncParameter* enc_par = &(enc_testConfigPara->enc);
    int send_frame_status = 0;
    int instanceId = enc_testConfigPara->instanceNum;
    int l = 0;
    int ret = 0;
    int get_stream_times = 0;

    char output_filename[512] = {0};
    FILE *fout;
    ret = strncmp(enc_testConfigPara->outputPath, "/dev/null", 9);
    if (ret != 0)
    {
        if (enc_par->enc_fmt == 0)
            sprintf(output_filename, "%s-%d.264", enc_testConfigPara->outputPath, instanceId);
        else
            sprintf(output_filename, "%s-%d.265", enc_testConfigPara->outputPath, instanceId);
        fout = fopen(output_filename, "wb");
    }
    else
    {
        fout = fopen("/dev/null", "wb");
    }

    if (fout == NULL)
    {
        fprintf(stderr, "Failed to open %s for writing: %s\n",
                output_filename, strerror(errno));
        enc_testConfigPara->result = -1;
    }

    ctx = calloc(1, sizeof(VpuEncContext));
    if (ctx == NULL) {
        fprintf(stderr, "calloc failed!\n");
        enc_testConfigPara->result = -1;
    }
    #ifdef __linux__
        pthread_cleanup_push(cleanup_task, (void*)ctx);
        int old_canclestate;
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &old_canclestate);
    #endif
    ctx->fout = fout;
    ctx->first_pkt_recevie_flag = 0;
    ctx->soc_idx = enc_par->soc_idx;
    ctx->core_idx = bmvpu_enc_get_core_idx(enc_par->soc_idx);

    eop = &(ctx->open_params);
    if (enc_par->enc_fmt == 0)
        codec_fmt = BM_VPU_CODEC_FORMAT_H264;
    else
        codec_fmt = BM_VPU_CODEC_FORMAT_H265;

    bmvpu_enc_set_default_open_params(eop, codec_fmt);

    eop->soc_idx = ctx->soc_idx;

    if (enc_par->pix_fmt == 0)
        eop->pix_format = BM_VPU_ENC_PIX_FORMAT_YUV420P;
    else
        eop->pix_format = BM_VPU_ENC_PIX_FORMAT_NV12;

    eop->frame_width = enc_par->crop_w;
    eop->frame_height = enc_par->crop_h;
    eop->timebase_num = 1;
    eop->timebase_den = 1;
    eop->fps_num = enc_par->fps;
    eop->fps_den = 1;
    eop->bitrate = enc_par->bit_rate * 1000;
    if (enc_par->bit_rate <= 0) {
        eop->cqp = enc_par->cqp;
    }
    eop->intra_period = enc_par->intra_period;
    eop->gop_preset = enc_par->gop_preset;

    ret = bmvpu_enc_load(ctx->soc_idx);
    if (ret != BM_VPU_ENC_RETURN_CODE_OK) {
        fprintf(stderr, "bmvpu_enc_load faile!\n");
        global_ret = -1;
        goto cleanup;
    }

    bmvpu_enc_get_bitstream_buffer_info(&(ctx->bs_buffer_size), &(ctx->bs_buffer_alignment));
    unsigned int bs_buffer_array[12] = {0, 7, 7, 7, 10, 13, 7, 7, 18, 7};
    ctx->bs_buffer_size = ((ctx->bs_buffer_size + (4*1024-1)) & (~(4*1024-1))) * bs_buffer_array[eop->gop_preset];
    ret = bmvpu_enc_dma_buffer_allocate(ctx->core_idx, &(ctx->bs_dma_buffer), ctx->bs_buffer_size);
    if (ret != 0) {
        fprintf(stderr, "bm_malloc_device_byte for bs_dmabuffer failed!\n");
        global_ret = -1;
        goto cleanup;
    }

    ret = bmvpu_enc_open(&(ctx->video_encoder), eop, &(ctx->bs_dma_buffer), &(ctx->initial_info));
    if (ret != BM_VPU_ENC_RETURN_CODE_OK) {
        fprintf(stderr, "bmvpu_enc_open failed\n");
        global_ret = -1;
        goto cleanup;
    }
    ctx->initial_info.src_fb.y_size = ctx->initial_info.src_fb.y_stride * ctx->initial_info.src_fb.height;
    ctx->initial_info.src_fb.c_size = ctx->initial_info.src_fb.y_stride / 2 * ctx->initial_info.src_fb.height / 2;

    ctx->num_src_fb = ctx->initial_info.min_num_src_fb;
    ctx->src_fb_list = (BmVpuFramebuffer *)malloc(sizeof(BmVpuFramebuffer) * ctx->num_src_fb);
    if (ctx->src_fb_list == NULL) {
        fprintf(stderr, "malloc failed\n");
        ret = -1;
        goto cleanup;
    }
    ctx->src_fb_dmabuffers = (BmVpuEncDMABuffer*)malloc(sizeof(BmVpuEncDMABuffer) * ctx->num_src_fb);
    if (ctx->src_fb_dmabuffers == NULL) {
        fprintf(stderr, "malloc failed\n");
        global_ret = -1;
        goto cleanup;
    }
    for (int i = 0; i < ctx->num_src_fb; i++) {
        int src_id = i;
        ret = bmvpu_fill_framebuffer_params(&(ctx->src_fb_list[i]),
                                            &(ctx->initial_info.src_fb),
                                            &(ctx->src_fb_dmabuffers[i]),
                                            src_id, NULL);
        if (ret != 0) {
            fprintf(stderr, "bmvpu_fill_framebuffer_params failed\n");
            global_ret = -1;
            goto cleanup;
        }
    }
    /* Create queue for source frame unused */
    ctx->frame_unused_queue = bm_queue_create(ctx->num_src_fb, sizeof(BmVpuFramebuffer*));
    if (ctx->frame_unused_queue == NULL)
    {
        fprintf(stderr, "bm_queue_create failed\n");
        global_ret = -1;
        goto cleanup;
    }
    for (int i=0; i<ctx->num_src_fb; i++)
    {
        BmVpuFramebuffer *fb = &(ctx->src_fb_list[i]);
        bm_queue_push(ctx->frame_unused_queue, (void*)(&fb));
    }
    // bm_queue_show(ctx->frame_unused_queue);

    /* Set the encoding parameters for this frame. */
    memset(&(ctx->enc_params), 0, sizeof(BmVpuEncParams));
    ctx->enc_params.acquire_output_buffer = acquire_output_buffer;
    ctx->enc_params.finish_output_buffer  = finish_output_buffer;
    ctx->enc_params.output_buffer_context = NULL;
    ctx->enc_params.skip_frame = 0; // TODO

    /* Set up the input frame.
     * The only field that needs to be set is the input framebuffer.
     * The encoder will read from it.
     * The rest can remain zero/NULL. */
    memset(&(ctx->input_frame), 0, sizeof(ctx->input_frame));

    /* Set up the output frame.
     * Simply setting all fields to zero/NULL is enough.
     * The encoder will fill in data. */
    memset(&(ctx->output_frame), 0, sizeof(ctx->output_frame));
    ctx->output_frame.data_size = ENC_PKT_DATA_SIZE;

    // int frame_size = ctx->initial_info.src_fb.size;
    int count = 0;

    pthread_setcancelstate(old_canclestate, NULL);
    pthread_testcancel();
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &old_canclestate);

    l = 0;
    while (1)
    {
        count++;
        if (g_exit_flag) break;
        gettimeofday(&(ctx->tv_beg), NULL);
        for (int i=0; i<enc_testConfigPara->frame_number; i++)
        {
            if (g_exit_flag) break;

            /* Stop encoding if EOF was reached */
            ctx->src_fb = get_src_framebuffer(ctx);
            if (ctx->src_fb == NULL)
            {
                fprintf(stderr, "get_src_framebuffer failed\n");
                global_ret = -1;
                goto cleanup;
            }

            if (ctx->first_pkt_recevie_flag != 1) {
                if (bm_queue_is_empty(ctx->frame_unused_queue)) {
                    ctx->first_pkt_recevie_flag = 1;
                }
            }

            while (bm_queue_is_empty(bm_Image_queue[instanceId])) {
                if (g_stop_flag[instanceId] == 1) {
                    goto flush;
                }
                usleep(10);
            }
            pthread_mutex_lock(&bmImgQue);
            bm_image *src_bm_img = *((bm_image **)bm_queue_pop(bm_Image_queue[instanceId]));
            pthread_mutex_unlock(&bmImgQue);

            bm_device_mem_t mem[4];
            bm_image_get_device_mem(*src_bm_img, mem);
            unsigned long long phys_addr = bm_mem_get_device_addr(mem[0]);
            int get_byte_size[4] = {0};
            bm_image_get_byte_size(*src_bm_img, get_byte_size);
            int total_size = get_byte_size[0] + get_byte_size[1] + get_byte_size[2] + get_byte_size[3];
            ctx->src_fb->dma_buffer->phys_addr = phys_addr;
            ctx->src_fb->dma_buffer->size = total_size;
            ctx->src_fb_list[ctx->src_fb->myIndex].context = (void *) src_bm_img;
            ctx->src_fb->dma_buffer->dmabuf_fd = mem->u.device.dmabuf_fd;
            ctx->input_frame.framebuffer = ctx->src_fb;

            get_stream_times = 0;
re_send:
            if (g_exit_flag) break;
            send_frame_status = bmvpu_enc_send_frame(ctx->video_encoder, &(ctx->input_frame), &(ctx->enc_params));

get_stream:
            if (g_exit_flag) break;
            ret = bmvpu_enc_get_stream(ctx->video_encoder, &(ctx->output_frame), &(ctx->enc_params));
            if (ret == BM_VPU_ENC_RETURN_CODE_ENC_END)
            {
                printf("[%zx] Encoding end\n", pthread_self());
                goto cleanup;
            }

            if (ctx->output_frame.data_size > 0)
            {
                int j;
                // 1. Collect the input frames released
                for (j=0; j<ctx->num_src_fb; j++)
                {
                    BmVpuFramebuffer* fb = &(ctx->src_fb_list[j]);
                    if (ctx->output_frame.src_idx == fb->myIndex)
                    {
                        bm_image * img = (bm_image *)ctx->src_fb_list[j].context;
                        bm_image_destroy(img);
                        free(img);
                        bm_queue_push(ctx->frame_unused_queue, &fb);
                        break;
                    }
                }

                // 2. Write out the encoded frame to the output file.
                fwrite(ctx->output_frame.data, 1, ctx->output_frame.data_size, fout);
                ctx->output_frame.data_size = 0;
                free(ctx->output_frame.data);
                ctx->output_frame.data = NULL;
                gettimeofday(&(ctx->tv_end), NULL);
                sta_time[instanceId] += ((ctx->tv_end.tv_sec*1000.0 + ctx->tv_end.tv_usec/1000.0) -
                                    (ctx->tv_beg.tv_sec*1000.0 + ctx->tv_beg.tv_usec/1000.0));
                count_enc[instanceId]++;
                gettimeofday(&(ctx->tv_beg), NULL);
            }
            else{
                if (ctx->first_pkt_recevie_flag == 1) {
                    if (get_stream_times < GET_STREAM_TIMEOUT) {
                        usleep(1000*5);
                        get_stream_times++;
                        goto get_stream;
                    }
                }
            }
            if (send_frame_status == BM_VPU_ENC_RETURN_CODE_RESEND_FRAME) {
                usleep(10);
                goto re_send;
            }
            pthread_setcancelstate(old_canclestate, NULL);
            pthread_testcancel();
            pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &old_canclestate);
        }
        if (enc_testConfigPara->loop > 0 && ++l >= enc_testConfigPara->loop){
            break;
        }
    }

flush:
    gettimeofday(&(ctx->tv_beg), NULL);
    while(1)
    {
        if (g_exit_flag) break;

        ctx->input_frame.framebuffer = NULL;
        ctx->input_frame.context = NULL;
        ctx->input_frame.pts = 0L;
        ctx->input_frame.dts = 0L;

        send_frame_status = bmvpu_enc_send_frame(ctx->video_encoder, &(ctx->input_frame), &(ctx->enc_params));
        ret = bmvpu_enc_get_stream(ctx->video_encoder, &(ctx->output_frame), &(ctx->enc_params));
        if (ret == BM_VPU_ENC_RETURN_CODE_ENC_END)
        {
            printf("[%zx] Encoding end\n", pthread_self());
            break;
        }

        if (ctx->output_frame.data_size > 0)
        {

            int j;
            /* Collect the input frames released */
            for (j=0; j<ctx->num_src_fb; j++)
            {
                BmVpuFramebuffer* fb = &(ctx->src_fb_list[j]);
                if (ctx->output_frame.src_idx == fb->myIndex)
                {
                    bm_image * img = (bm_image *)ctx->src_fb_list[j].context;
                    bm_image_destroy(img);
                    free(img);
                    bm_queue_push(ctx->frame_unused_queue, &fb);
                    // if (par->log_level > BMVPU_ENC_LOG_LEVEL_INFO)
                    //     printf("[%zx] myIndex = 0x%x, push\n", pthread_self(), fb->myIndex);
                }
            }

            /* Write out the encoded frame to the output file. */
            fwrite(ctx->output_frame.data, 1, ctx->output_frame.data_size, fout);
            free(ctx->output_frame.data);
            ctx->output_frame.data_size = 0;
            ctx->output_frame.data = NULL;


        gettimeofday(&(ctx->tv_end), NULL);
        sta_time[instanceId] += ((ctx->tv_end.tv_sec*1000.0 + ctx->tv_end.tv_usec/1000.0) -
                          (ctx->tv_beg.tv_sec*1000.0 + ctx->tv_beg.tv_usec/1000.0));
        count_enc[instanceId]++;

        gettimeofday(&(ctx->tv_beg), NULL);

        }
    }

cleanup:
    cleanup_task((void*)ctx);
    enc_testConfigPara->result = ret;
#ifdef __linux__
    pthread_cleanup_pop(0);
#endif
    return NULL;
}

