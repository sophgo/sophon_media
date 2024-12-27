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

typedef enum {
  CODEC_NONE,
  CODEC_H264,
  CODEC_H265,
  CODEC_JPEG,
  CODEC_BUTT,
}CODEC_TYPE;

#define MAX_CHN 64
#define INTERVAL (1)

static uint64_t frame_count[MAX_CHN] = {0};
static int g_exit_flag = 0;
GMainLoop *g_main_loop = NULL;
static int g_loop_exit = 0;
static GstElement *g_pipeline = NULL;
typedef struct _codec_param {
    gchar *name;
    int codec_type;
    int trans_type;
    int num_chn;
}codec_param;

static void fakesink_on_buffer(GstElement G_GNUC_UNUSED *fakesink, GstBuffer G_GNUC_UNUSED *buffer, GstPad G_GNUC_UNUSED *pad, gpointer chn) {
    // 处理GstBuffer
    gint *chn_id = (gint*)(chn);
    frame_count[*chn_id]++;
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
  sigaction(SIGINT, &action, nullptr);   // ctrl_c signal
}


// Function to add a codec branch to the tee
static gboolean add_codec_branch(GstElement *tee, const gchar *codec_name, GstElement *pipeline, gint *chn_id) {
    GstElement *queue, *codec, *sink;
    GstPad *tee_src_pad, *queue_sink_pad;

    // Create elements for the new branch
    queue = gst_element_factory_make("queue", NULL);
    codec = gst_element_factory_make(codec_name, NULL);
    sink = gst_element_factory_make("fakesink", NULL);
    g_object_set(sink, "signal-handoffs", TRUE, NULL);
    g_signal_connect(sink, "handoff", G_CALLBACK(fakesink_on_buffer), chn_id);

    if (!strcmp(codec_name, "bmdec"))
        g_object_set(G_OBJECT(codec), "ex-buf-num", 5, NULL);

    if (!strcmp(codec_name, "bmh265enc") || !strcmp(codec_name, "bmh264enc"))
        g_object_set(G_OBJECT(codec), "gop", 50, "bps", 2000000, NULL);

    if (!queue || !codec || !sink) {
        g_printerr("Failed to create elements for the branch with codec '%s'.\n", codec_name);
        return FALSE;
    }

    // Add elements to the pipeline
    gst_bin_add_many(GST_BIN(pipeline), queue, codec,  sink, NULL);

    // Link elements in the branch
    if (!gst_element_link_many(queue, codec, sink, NULL)) {
        g_printerr("Failed to link elements in the branch with codec '%s'.\n", codec_name);
        return FALSE;
    }

    // Get a request pad from the tee and link it to the queue
    tee_src_pad = gst_element_get_request_pad(tee, "src_%u");
    queue_sink_pad = gst_element_get_static_pad(queue, "sink");

    if (gst_pad_link(tee_src_pad, queue_sink_pad) != GST_PAD_LINK_OK) {
        g_printerr("Tee could not be linked to the new queue for decoder '%s'.\n", codec_name);
        return FALSE;
    }

    // Release the pads
    gst_object_unref(queue_sink_pad);
    gst_object_unref(tee_src_pad);
    // Sync the state with the parent
    gst_element_sync_state_with_parent(queue);
    gst_element_sync_state_with_parent(codec);
    gst_element_sync_state_with_parent(sink);

    return TRUE;
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
static gboolean add_dec_enc_branch(GstElement *tee, const gchar *dec_name, const gchar *enc_name, GstElement *pipeline, gint *chn_id) {
    GstElement *queue, *dec, *sink, *enc;
    GstPad *tee_src_pad, *queue_sink_pad;

    // Create elements for the new branch
    queue = gst_element_factory_make("queue", NULL);
    dec = gst_element_factory_make(dec_name, NULL);
    enc = gst_element_factory_make(enc_name, NULL);
    sink = gst_element_factory_make("fakesink", NULL);
    g_object_set(sink, "signal-handoffs", TRUE, NULL);
    g_signal_connect(sink, "handoff", G_CALLBACK(fakesink_on_buffer), chn_id);

    if (!strcmp(dec_name, "bmdec"))
        g_object_set(G_OBJECT(dec), "ex-buf-num", 5, NULL);

    if (strcmp(enc_name, "bmjpegenc"))
        g_object_set(G_OBJECT(enc), "gop", 50, "bps", 2000000, NULL);

    if (!queue || !dec || !enc || !sink) {
        g_printerr("Failed to create elements for the branch with codec '%s'.\n", dec_name);
        return FALSE;
    }

    // Add elements to the pipeline
    gst_bin_add_many(GST_BIN(pipeline), queue, dec, enc, sink, NULL);

    // Link elements in the branch
    if (!gst_element_link_many(queue, dec, enc, sink, NULL)) {
        g_printerr("Failed to link elements in the branch with codec '%s'.\n", dec_name);
        return FALSE;
    }

    // Get a request pad from the tee and link it to the queue
    tee_src_pad = gst_element_get_request_pad(tee, "src_%u");
    queue_sink_pad = gst_element_get_static_pad(queue, "sink");

    if (gst_pad_link(tee_src_pad, queue_sink_pad) != GST_PAD_LINK_OK) {
        g_printerr("Tee could not be linked to the new queue for decoder '%s'.\n", dec_name);
        return FALSE;
    }

    // Release the pads
    gst_object_unref(queue_sink_pad);
    gst_object_unref(tee_src_pad);
    // Sync the state with the parent
    gst_element_sync_state_with_parent(queue);
    gst_element_sync_state_with_parent(dec);
    gst_element_sync_state_with_parent(enc);
    gst_element_sync_state_with_parent(sink);
    return TRUE;
}

void *multi_decode (void *arg) {
    GstElement *pipeline, *source, *parse = NULL, *tee;
    GMainLoop *main_loop;
    codec_param *param = (codec_param *)arg;
    gchar *name = param->name;
    int codec_type = param->codec_type;
    int num_chn = param->num_chn;
    gint chn[MAX_CHN] = { 0 };
    // Create the elements
    g_print("name %s, codec_type %d num_chn %d \n", name, codec_type, num_chn);
    source = gst_element_factory_make("multifilesrc", "source");
    g_object_set(G_OBJECT(source), "location", name, "loop", TRUE, NULL);

    switch (codec_type) {
    case CODEC_H264:
        parse = gst_element_factory_make("h264parse", "parse");;
        break;
    case CODEC_H265:
         parse = gst_element_factory_make("h265parse", "parse");
        break;
    case CODEC_JPEG:
        parse = gst_element_factory_make("jpegparse", "parse");
        break;
    default:
        g_print("no support codec %d \n", codec_type);
        break;
    }

    tee = gst_element_factory_make("tee", "tee");

    // Create the empty pipeline
    pipeline = gst_pipeline_new("multidec-pipeline");
    g_pipeline = pipeline;
    if (!pipeline || !source || !parse || !tee) {
        g_printerr("Not all elements could be created.\n");
        return NULL;
    }

    // Build the pipeline before the branches
    gst_bin_add_many(GST_BIN(pipeline), source, parse, tee, NULL);
    if (!gst_element_link_many(source, parse, tee, NULL)) {
        g_printerr("Elements could not be linked.\n");
        gst_object_unref(pipeline);
        return NULL;
    }

    for (gint i = 0; i < num_chn; i++)
    {
        chn[i] = i;
        if (!add_codec_branch(tee, "bmdec", pipeline, &chn[i])) {
            g_printerr("Could not add decoder branch.\n");
            gst_object_unref(pipeline);
            return NULL;
        }
    }
    g_print("start playing \n");
    // Start playing the pipeline
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    // Create a GLib Main Loop and set it to run
    main_loop = g_main_loop_new(NULL, FALSE);
    auto bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
    guint bus_watch_id = gst_bus_add_watch (bus, bus_call, main_loop);
    gst_object_unref (bus);
    g_main_loop_run(main_loop);

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    g_source_remove (bus_watch_id);
    g_main_loop_unref(main_loop);
    cout << "dec loop finish!" << endl;
    g_loop_exit = 1;
    return NULL;
}

void *multi_encode (void *arg) {
    GstElement *pipeline, *source, *caps_filter, *tee;
    GstCaps *caps;
    char codec_name[10];
    GMainLoop *main_loop;
    gint chn[MAX_CHN] = { 0 };
    codec_param *param = (codec_param *)arg;
    int codec_type = param->codec_type;
    int num_chn = param->num_chn;
    // Create the elements

    g_print("codec_type %d num_chn %d \n", codec_type, num_chn);
    source = gst_element_factory_make("videotestsrc", "source");
    caps_filter = gst_element_factory_make("capsfilter", "filter");
    tee = gst_element_factory_make("tee", "tee");

    // Create the empty pipeline
    pipeline = gst_pipeline_new("multienc-pipeline");
    g_pipeline = pipeline;

    if (!pipeline || !source || !caps_filter || !tee) {
        g_printerr("Not all elements could be created.\n");
        return NULL;
    }
    caps = gst_caps_new_simple("video/x-raw",
                               "width", G_TYPE_INT, 1920,
                               "height", G_TYPE_INT, 1080,
                               NULL);
    g_object_set(caps_filter, "caps", caps, NULL);
    gst_caps_unref(caps);

    // Build the pipeline before the branches
    gst_bin_add_many(GST_BIN(pipeline), source, caps_filter, tee, NULL);
    if (!gst_element_link_many(source, caps_filter, tee, NULL)) {
        g_printerr("Elements could not be linked.\n");
        gst_object_unref(pipeline);
        return NULL;
    }

    switch (codec_type) {
    case CODEC_H264:
        strcpy(codec_name, "bmh264enc");
        break;
    case CODEC_H265:
         strcpy(codec_name,"bmh265enc");
        break;
    case CODEC_JPEG:
         strcpy(codec_name, "bmjpegenc");
        break;
    default:
        g_print("no support codec %d \n", codec_type);
        break;
    }

    for (gint i = 0; i < num_chn; i++)
    {
        chn[i] = i;
        if (!add_codec_branch(tee, codec_name, pipeline, &chn[i])) {
            g_printerr("Could not add decoder branch.\n");
            gst_object_unref(pipeline);
            return NULL;
        }
    }
    g_print("start playing \n");
    // Start playing the pipeline
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    // Create a GLib Main Loop and set it to run
    main_loop = g_main_loop_new(NULL, FALSE);
    auto bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
    guint bus_watch_id = gst_bus_add_watch (bus, bus_call, main_loop);
    gst_object_unref (bus);
    g_main_loop_run(main_loop);

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    g_source_remove (bus_watch_id);
    g_main_loop_unref(main_loop);

    cout << "enc loop finish!" << endl;
    usleep(1000);
    g_loop_exit = 1;
    return NULL;
}

void *multi_dec_enc (void *arg) {
    GstElement *pipeline, *source, *parse = NULL, *tee;
    GMainLoop *main_loop;
    codec_param *param = (codec_param *)arg;
    gchar *name = param->name;
    int codec_type = param->codec_type;
    int num_chn = param->num_chn;
    char codec_name[10];
    gint chn[MAX_CHN] = { 0 };
    // Create the elements
    g_print("name %s, codec_type %d num_chn %d \n", name, codec_type, num_chn);
    source = gst_element_factory_make("multifilesrc", "source");
    g_object_set(G_OBJECT(source), "location", name, "loop", TRUE, NULL);

    switch (codec_type) {
    case CODEC_H264:
        parse = gst_element_factory_make("h264parse", "parse");;
        break;
    case CODEC_H265:
         parse = gst_element_factory_make("h265parse", "parse");
        break;
    case CODEC_JPEG:
        parse = gst_element_factory_make("jpegparse", "parse");
        break;
    default:
        g_print("no support codec %d \n", codec_type);
        break;
    }

    switch (param->trans_type) {
    case CODEC_H264:
        strcpy(codec_name, "bmh264enc");
        break;
    case CODEC_H265:
         strcpy(codec_name,"bmh265enc");
        break;
    case CODEC_JPEG:
         strcpy(codec_name, "bmjpegenc");
        break;
    default:
        g_print("no support codec %d default h265 \n", param->trans_type);
        strcpy(codec_name,"bmh265enc");
        break;
    }

    tee = gst_element_factory_make("tee", "tee");

    // Create the empty pipeline
    pipeline = gst_pipeline_new("multidec-pipeline");
    g_pipeline = pipeline;
    if (!pipeline || !source || !parse || !tee) {
        g_printerr("Not all elements could be created.\n");
        return NULL;
    }

    // Build the pipeline before the branches
    gst_bin_add_many(GST_BIN(pipeline), source, parse, tee, NULL);
    if (!gst_element_link_many(source, parse, tee, NULL)) {
        g_printerr("Elements could not be linked.\n");
        gst_object_unref(pipeline);
        return NULL;
    }

    for (gint i = 0; i < num_chn; i++)
    {
        chn[i] = i;
        if (!add_dec_enc_branch(tee, "bmdec", codec_name, pipeline, &chn[i])) {
            g_printerr("Could not add decoder branch.\n");
            gst_object_unref(pipeline);
            return NULL;
        }
    }
    g_print("start playing \n");
    // Start playing the pipeline
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    // Create a GLib Main Loop and set it to run
    main_loop = g_main_loop_new(NULL, FALSE);
    auto bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
    guint bus_watch_id = gst_bus_add_watch (bus, bus_call, main_loop);
    gst_object_unref (bus);
    g_main_loop_run(main_loop);

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    g_source_remove (bus_watch_id);
    g_main_loop_unref(main_loop);

    cout << "dec_enc loop finish!" << endl;
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
    char *display = getenv("VIDMULTI_DISPLAY_FRAMERATE");
    if (display) dis_mode = atoi(display);

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
    gchar *video_path = NULL;
    GOptionContext *ctx;
    GError *err = NULL;
    gint code_type = 0, trans_type = 0, num_chnl = 0, is_enc = 0, disp = 0;
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
        { "video_path", 'v', 0, G_OPTION_ARG_STRING, &video_path,
            "The path of the video file.", "FILE" },
        { "codec_type", 'c', 0, G_OPTION_ARG_INT, &code_type,
            "h26x codec format type 1:h264 2:h265 3:JPEG", "type" },
        { "is_enc", 'e', 0, G_OPTION_ARG_INT, &is_enc,
            "if enc multi test set 1 or is dec multi test", "enc" },
        { "num_chl", 'n', 0, G_OPTION_ARG_INT, &num_chnl,
            "the num of codec chnl", "chl" },
        { "disp", 'd', 0, G_OPTION_ARG_INT, &disp,
            "disp every chnl fps", "chl" },
        { "trans_type", 't', 0, G_OPTION_ARG_INT, &trans_type,
        "trans codec enc type when is_enc set 2 valid 1:h264 2:h265 3:JPEG", "trans_type" },
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
    if ((!video_path && !is_enc) || !code_type ||
        (is_enc==2 && !trans_type) || !num_chnl) {
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
    param.name = video_path;
    param.codec_type = code_type;
    param.num_chn = num_chnl;
    param.trans_type = trans_type;
    if (is_enc == 1) {
        pthread_create(&codec_th, &attr, multi_encode, &param);
        //multi_encode(&param);
    } else if (!is_enc) {
        pthread_create(&codec_th, &attr, multi_decode, &param);
        //multi_encode(&param);
    } else {
        pthread_create(&codec_th, &attr, multi_dec_enc, &param);
        //multi_decode (&param);
    }
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

