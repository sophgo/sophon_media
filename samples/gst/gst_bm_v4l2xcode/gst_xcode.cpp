#include <gst/gst.h>
#include <iostream>
#include <string>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <execinfo.h>

using std::string;
using std::cout;
using std::cin;
using std::endl;

#define MAX_CHN 64
#define MAX_PIPELINE 6
#define INTERVAL (1)

static uint64_t frame_count[MAX_CHN] = {0};
static int g_exit_flag = 0;
GMainLoop *g_main_loop = NULL;
static int g_loop_exit = 0;
static GstElement *g_pipeline = NULL;
typedef struct _codec_param {
    gchar **name;
    int trans_type;
    int num_chn;
    gchar **outname;
    gchar **width;
    gchar **height;
}codec_param;

static GstPadProbeReturn realsink_on_buffer(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)  {
    // 处理GstBuffer
    gint *chn_id = (gint*)(user_data);
    frame_count[*chn_id]++;
    return GST_PAD_PROBE_OK;
}

void signal_handler(int signum) {
   // Release handle before crash in case we cannot reopen it again.
   g_exit_flag = 1;     // exit all threads
   int try_count = 8000;
   cout << "signal " << signum << endl;

   signal(signum, SIG_IGN);
   if (g_pipeline)
        gst_element_send_event(g_pipeline, gst_event_new_eos());
   while (try_count--){
    usleep(1000);
     if (g_loop_exit){
         break;
     }
   }
   // Reset the signal handler as default
   signal(signum, SIG_DFL);

   // flush IO before exiting.
   cout.flush();

   _exit(signum);
}

void register_signal_handler(void) {
  struct sigaction action;
  memset(&action, 0, sizeof(action));
  sigfillset(&action.sa_mask);
  action.sa_handler = signal_handler;
  action.sa_flags = SA_RESTART;

  sigaction(SIGABRT, &action, nullptr);   // abort instruction
  sigaction(SIGBUS, &action, nullptr);    // Bus error
  sigaction(SIGFPE, &action, nullptr);    // float exception
  sigaction(SIGILL, &action, nullptr);    // illegal instruction
  sigaction(SIGSEGV, &action, nullptr);   // segement falut
  sigaction(SIGINT, &action, nullptr);    // ctrl_c signal
}


gboolean
bus_call (GstBus G_GNUC_UNUSED *bus, GstMessage * msg, gpointer data)
{
  GMainLoop *loop = reinterpret_cast < GMainLoop * >(data);
  switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_EOS:
      GST_INFO ("End of stream\n");
      g_main_loop_quit (loop);
      break;
    case GST_MESSAGE_ERROR:
      gchar * debug;
      GError *error;
      gst_message_parse_error (msg, &error, &debug);
      GST_ERROR ("ERROR from element %s: %s\n",
          GST_OBJECT_NAME (msg->src), error->message);
      g_error_free (error);
      g_main_loop_quit (loop);
      break;
    default:
      break;
  }
  return true;
}

// Function to add a codec branch to the tee
static gboolean add_dec_enc_branch(GstElement *tee, const gchar *enc_name, GstElement *pipeline, gint *chn_id, gchar *out_name, gint width, gint height) {
    GstElement *queue, *sink, *enc, *vpss;
    GstPad *tee_src_pad, *queue_sink_pad;

    // Create elements for the new branch
    vpss = gst_element_factory_make("bmvpss", NULL);
    enc = gst_element_factory_make(enc_name, NULL);
    sink = gst_element_factory_make("filesink", NULL);
    g_object_set(sink, "location", out_name, NULL);
    GstPad *sinkpad = gst_element_get_static_pad(sink, "sink");
    gst_pad_add_probe(sinkpad, GST_PAD_PROBE_TYPE_BUFFER, realsink_on_buffer, chn_id, NULL);
    gst_object_unref(sinkpad);

    if (strcmp(enc_name, "bmjpegenc"))
        g_object_set(G_OBJECT(enc), "gop", 50, "bps", 2000000, NULL);

    if (!vpss || !enc || !sink) {
        g_printerr("Failed to create elements for the branch with codec.\n");
        return FALSE;
    }
    GstCaps *caps = gst_caps_new_simple("video/x-raw",
                                        "format", G_TYPE_STRING, "I420",
                                        "width",  G_TYPE_INT, width,
                                        "height", G_TYPE_INT, height,
                                        NULL);


    // Add elements to the pipeline
    gst_bin_add_many(GST_BIN(pipeline), vpss, enc, sink, NULL);

    if (!gst_element_link(tee, vpss)) {
        g_printerr("Failed to link tee and vpss.\n");
        return FALSE;
    }

    if (!gst_element_link_filtered(vpss, enc, caps)) {
        g_printerr("Failed to link vpss and encoder with specified caps.\n");
        gst_caps_unref(caps);
        return FALSE;
    }
    gst_caps_unref(caps);

    // Link elements in the branch
    if (!gst_element_link(enc, sink)) {
        g_printerr("Failed to link elements in the branch with codec.\n");
        return FALSE;
    }

    // Release the pads
    gst_element_sync_state_with_parent(vpss);
    gst_element_sync_state_with_parent(enc);
    gst_element_sync_state_with_parent(sink);
    return TRUE;
}


void *multi_dec_enc (void *arg) {
    GstElement *pipeline = NULL;
    GMainLoop *main_loop;
    codec_param *param = (codec_param *)arg;
    int num_chn = param->num_chn;
    char codec_name[10];
    gint chn[MAX_CHN] = { 0 };
    // Create the elements
    for (gint i = 0; i < num_chn; i++)
    {
        g_print("In multi_dec_enc, in_name = %s, trans_type = %d, num_chn = %d, out_name = %s\n", param->name[i], param->trans_type, i, param->outname[i]);
    }

    GstElement *sources[MAX_PIPELINE];
    for (gint i = 0; i < num_chn; ++i) {
        sources[i] = gst_element_factory_make("bmv4l2src", NULL);
        if (!sources[i]) {
            g_printerr("Failed to create elements for the branch with codec 'bmv4l2src'.\n");
            return FALSE;
        }
        g_object_set(G_OBJECT(sources[i]), "device", param->name[i], NULL);
    }

    switch (param->trans_type) {
    case 1:
        strcpy(codec_name, "bmh264enc");
        break;
    case 2:
         strcpy(codec_name,"bmh265enc");
        break;
    case 3:
         strcpy(codec_name, "bmjpegenc");
        break;
    default:
        g_print("no support codec %d default h265 \n", param->trans_type);
        strcpy(codec_name,"bmh265enc");
        break;
    }

    // Create the empty pipeline
    GstElement *pipelines[MAX_PIPELINE];
    for (gint i = 0; i < num_chn; ++i) {
        gchar pipeline_name[256];
        sprintf(pipeline_name, "pipeline_%d", i);
        pipelines[i] = gst_pipeline_new(pipeline_name);
        if (!pipelines[i]) {
            g_printerr("pipelines[%d] could be created.\n", i);
            return NULL;
        }
    }

    // Build the pipeline before the branches
    for (gint i = 0; i < num_chn; i++)
    {
        gst_bin_add_many(GST_BIN(pipelines[i]), sources[i], NULL);
    }

    for (gint i = 0; i < num_chn; i++)
    {
        chn[i] = i;
        g_print("param->width[%d] = %d, param->height[%d] = %d \n", i, std::stoi(param->width[i]), i, std::stoi(param->height[i]));

        if (!add_dec_enc_branch(sources[i], codec_name, pipelines[i], &chn[i], param->outname[i], std::stoi(param->width[i]), std::stoi(param->height[i]))) {
            g_printerr("Could not add decoder branch.\n");
            gst_object_unref(pipeline);
            return NULL;
        }
    }
    g_print("start playing \n");
    // Start playing the pipeline
    for (gint i = 0; i < num_chn; i++)
    {
        gst_element_set_state(pipelines[i], GST_STATE_PLAYING);
    }
    // Create a GLib Main Loop and set it to run
    main_loop = g_main_loop_new(NULL, FALSE);
    GstBus* bus[MAX_PIPELINE];
    guint bus_watch_id[MAX_PIPELINE];
    for (gint i = 0; i < num_chn; i++)
    {
        bus[i] = gst_pipeline_get_bus (GST_PIPELINE (pipelines[i]));
        bus_watch_id[i] = gst_bus_add_watch (bus[i], bus_call, main_loop);
        gst_object_unref (bus[i]);
    }
    g_main_loop_run(main_loop);

    for (gint i = 0; i < num_chn; i++)
    {
        gst_element_set_state(pipelines[i], GST_STATE_NULL);
        gst_object_unref(pipelines[i]);
        g_source_remove (bus_watch_id[i]);
    }

    g_main_loop_unref(main_loop);

    cout << "dec_enc loop finish!\n" << endl;
    g_loop_exit = 1;
    return NULL;
}

typedef struct _stat_cmd {
    gint num_chl;
    gint disp;
}stat_cmd;

void* stat_pthread(void *arg)
{
    stat_cmd *cmd = (stat_cmd *)arg;
    gint thread_num = cmd->num_chl;
    int dis_mode = cmd->disp;
    uint64_t last_count[MAX_CHN] = {0};
    uint64_t last_count_sum = 0;

    while(!g_exit_flag) {
        sleep(INTERVAL);
        if (dis_mode == 1) {
            for (int i = 0; i < thread_num; i++) {
                    printf("ID[%d] , FPS[%2.2f] \n",
                        i, ((double)(frame_count[i] - last_count[i]))/INTERVAL);
                last_count[i] = frame_count[i];
            }
        }
        else {
            uint64_t count_sum = 0;
            for (int i = 0; i < thread_num; i++)
              count_sum += frame_count[i];
            printf("thread %d, frame %ld, fps %2.2f", thread_num, count_sum, ((double)(count_sum-last_count_sum))/INTERVAL);
            last_count_sum = count_sum;
        }
        printf("\r");
        fflush(stdout);
    }

    for (int i = 0; i < thread_num; i++) {
        last_count[i] = frame_count[i];
    }
    fflush(stdout);

    cout << "Stat thread exit!" << endl;
    return NULL;
}

static int kbhit (void)
{
    struct timeval tv;
    fd_set rdfs;

    tv.tv_sec = 0;
    tv.tv_usec = 0;

    FD_ZERO(&rdfs);
    FD_SET (STDIN_FILENO, &rdfs);

    select(STDIN_FILENO+1, &rdfs, NULL, NULL, &tv);
    return FD_ISSET(STDIN_FILENO, &rdfs);
}


int main(int argc, char *argv[]) {
    // Initialize GStreamer
    gchar **video_paths = NULL;
    gchar **out_paths = NULL;
    gchar **out_width = NULL;
    gchar **out_height = NULL;
    GOptionContext *ctx;
    GError *err = NULL;
    gint trans_type = 0, num_chnl = 0, is_enc = 0, disp = 0;
    pthread_t stat_h;
    pthread_t codec_th;
    stat_cmd st_cmd;
    codec_param param;

    pthread_attr_t attr;
    struct sched_param attr_param;

    attr_param.sched_priority = 80;
    pthread_attr_init(&attr);
    pthread_attr_setschedpolicy(&attr, SCHED_RR);
    pthread_attr_setschedparam(&attr, &attr_param);

    GOptionEntry entries[] = {
        { "video_path", 'v', 0, G_OPTION_ARG_STRING_ARRAY, &video_paths,
            "The path of the video file.", "FILE" },
        { "out_width", 'w', 0, G_OPTION_ARG_STRING_ARRAY, &out_width,
            "The width of the output video.", "WIDTH" },
        { "out_height", 'h', 0, G_OPTION_ARG_STRING_ARRAY, &out_height,
            "The height of the output video.", "HEIGHT" },
        { "is_enc", 'e', 0, G_OPTION_ARG_INT, &is_enc,
            "if enc multi test set 1 or is dec multi test", "enc" },
        { "num_chl", 'n', 0, G_OPTION_ARG_INT, &num_chnl,
            "the num of codec chnl", "chl" },
        { "disp", 'd', 0, G_OPTION_ARG_INT, &disp,
            "disp every chnl fps", "chl" },
        { "trans_type", 't', 0, G_OPTION_ARG_INT, &trans_type,
            "trans codec enc type when is_enc set 2 valid 1:h264 2:h265 3:JPEG", "trans_type" },
        { "outpath", 'o', 0, G_OPTION_ARG_STRING_ARRAY, &out_paths,
            "The path of the output video file.", "FILE" },
        { NULL, ' ', 0, G_OPTION_ARG_STRING, NULL, NULL, NULL}
    };

    if (argc < 2) {
        return 0;
    }
    ctx = g_option_context_new ("demo");
    g_option_context_add_main_entries (ctx, entries, NULL);
    g_option_context_add_group (ctx, gst_init_get_option_group ());
    if (!g_option_context_parse (ctx, &argc, &argv, &err)) {
        g_print ("Failed to initialize: %s\n", err->message);
        g_error_free (err);
        return 1;
    }

    int num_video_paths = g_strv_length(video_paths);
    int num_out_paths = g_strv_length(out_paths);
    g_print("num_video_paths = %d, num_out_paths = %d, num_chnl = %d \n", num_video_paths, num_out_paths, num_chnl);
    if (num_chnl != num_video_paths || num_chnl != num_out_paths || num_video_paths != num_out_paths) {
        g_print("Error: The number of channels does not match the number of input or output paths.\n");
        exit(1);
    }
    if ((!video_paths && !is_enc) || (is_enc==2 && !trans_type) || !num_chnl) {
        g_print("param set error \n");
        g_option_context_free(ctx);
        return 0;
    }

    st_cmd.num_chl = num_chnl;
    st_cmd.disp    = disp;
    g_main_loop = NULL;
    g_loop_exit = 0;
    register_signal_handler();
    pthread_create(&stat_h, NULL, stat_pthread, &st_cmd);

    gst_init (&argc, &argv);
    param.name = video_paths;
    param.num_chn = num_chnl;
    param.trans_type = trans_type;
    param.outname = out_paths;
    param.width = out_width;
    param.height = out_height;

    for (gint i = 0; i < param.num_chn; i++)
    {
        g_print("i = %d, in_name = %s, out_name = %s, param.width = %d, param.height = %d\n", i, param.name[i], param.outname[i], std::stoi(param.width[i]), std::stoi(param.height[i]));
    }

    pthread_create(&codec_th, &attr, multi_dec_enc, &param);

    char cmd = 0;
    while(cmd != 'q')
    {
        if (kbhit())
            cin >> cmd;
        else
            sleep(1);
    }
    g_exit_flag = 1;
    if (g_pipeline)
        gst_element_send_event(g_pipeline, gst_event_new_eos());
    pthread_join(codec_th, NULL);
    pthread_join(stat_h, NULL);
    g_option_context_free(ctx);
}

