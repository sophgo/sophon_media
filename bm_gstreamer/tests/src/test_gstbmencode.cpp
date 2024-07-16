/*
 * Copyright (C) 2024 Sophgo
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include <gst/check/gstcheck.h>
#include <gst/video/video.h>
#include <unistd.h>
#include <cstdio>

static GstStaticPadTemplate srctemplate =
  GST_STATIC_PAD_TEMPLATE("src",
                          GST_PAD_SRC,
                          GST_PAD_ALWAYS,
                          GST_STATIC_CAPS(GST_VIDEO_CAPS_MAKE("{ NV21, NV12, I420}")));

static GstStaticPadTemplate sinktemplate =
  GST_STATIC_PAD_TEMPLATE("sink",
                          GST_PAD_SINK,
                          GST_PAD_ALWAYS,
                          GST_STATIC_CAPS("video/x-h264, stream-format=byte-stream, alignment=au;"
                                          "video/x-h265, stream-format=byte-stream, alignment=au;"
                                          "image/jpeg;")
                                          );

static GstPad *bmsinkpad, *bmsrcpad;
static const char *input_name = NULL, *output_name = NULL;
static FILE *test_stream = NULL, *output_file = NULL;
static guint input_count = 0, output_count = 0, frame_count = 0;
static gboolean got_eos = FALSE;

static char*
GetExePath(void)
{
  static char exe_path[1024];
  static bool start_flag = true;

  if (start_flag) {
    memset(exe_path, 0, sizeof(exe_path));
    int cnt = readlink("/proc/self/exe", exe_path, sizeof(exe_path));
    if (cnt < 0 || cnt >= (int)sizeof(exe_path)) {
      printf("%s.error, readlink size:%d\n", __FUNCTION__, cnt);
      memset(exe_path, 0, sizeof(exe_path));
    } else {
      for (int i = cnt; i >= 0; --i) {
        if ('/' == exe_path[i]) {
          exe_path[i + 1] = '\0';
          break;
        }
      }
    }
    start_flag = false;
  }
  return exe_path;
}

static gboolean
setup_stream(const gchar* input, const gchar* output)
{
  if (test_stream == NULL) {
    test_stream = fopen(input, "rb");
    fail_unless(test_stream != NULL);
  }

  input_name = input;

  if (output_file == NULL) {
    output_file = fopen(output, "wb");
    fail_unless(output_file != NULL);
  }

  output_name = output;

  return TRUE;
}

static void
cleanup_stream(void)
{
  if (test_stream) {
    fflush(test_stream);
    fclose(test_stream);
    test_stream = NULL;
  }

  if (output_file) {
    fflush(output_file);
    fclose(output_file);
    output_file = NULL;
  }
}

static gboolean
feed_stream(GstVideoFormat format, guint width, guint height, guint num, GstAllocator *allocator)
{
  int frame_size = 0;
  GstSegment seg;
  GstBuffer* buffer;
  gsize  offset[GST_VIDEO_MAX_PLANES] = {0};
  gint   stride[GST_VIDEO_MAX_PLANES] = {0};
  int n_plane = 2;

  input_count = output_count = 0;
  frame_count = 0;
  got_eos = FALSE;


  if (format == GST_VIDEO_FORMAT_NV12 || format == GST_VIDEO_FORMAT_NV21 ||
      format == GST_VIDEO_FORMAT_I420) {
    frame_size = width * height * 3 / 2;
    stride[0] = width;
    stride[1] = width;
    offset[0] = 0;
    offset[1] = width * height;
    if (format == GST_VIDEO_FORMAT_I420) {
      n_plane = 3;
      stride[0] = width;
      stride[1] = (width + 1) / 2;
      stride[2] = (width + 1) / 2;
      offset[1] = stride[0] * height;
      offset[2] = offset[1]  + stride[1] * height / 2;
    }
  } else if (format == GST_VIDEO_FORMAT_BGRA || format == GST_VIDEO_FORMAT_RGBA || format == GST_VIDEO_FORMAT_ABGR ||
             format == GST_VIDEO_FORMAT_ARGB) {
    frame_size = width * height * 4;
    n_plane = 1;
  } else {
    fail("unsupported video format!");
  }

  fail_unless(test_stream != NULL, "test stream is NULL");

  fseek(test_stream, 0, SEEK_END);
  int file_len = ftell(test_stream);
  fail_unless(file_len > 0, "input file size invalid!");
  fseek(test_stream, 0, SEEK_SET);

  gst_segment_init(&seg, GST_FORMAT_TIME);

  fail_unless(gst_pad_push_event(bmsrcpad, gst_event_new_segment(&seg)));

  frame_count = file_len / frame_size;
  if (file_len % frame_size != 0)
    frame_count++;

  int length = file_len;
  while (length > 0) {
    if (!allocator) {
      buffer = gst_buffer_new_and_alloc(frame_size);
    } else {
      GstAllocationParams params;
      gst_allocation_params_init(&params);
      buffer = gst_buffer_new ();
      GstMemory *memory = gst_allocator_alloc(allocator, frame_size, &params);
      gst_buffer_append_memory (buffer, memory);
    }
    gst_buffer_memset(buffer, 0, 0, frame_size);

    GstMapInfo info;
    size_t nr = length < frame_size ? length : frame_size;
    size_t r;
    gst_buffer_map(buffer, &info, GST_MAP_WRITE);
    r = fread(info.data, 1, nr, test_stream);
    fail_unless(r == nr, "read r(%d) != nr(%d)", r, nr);
    gst_buffer_unmap(buffer, &info);

    GST_BUFFER_TIMESTAMP(buffer) = gst_util_uint64_scale(input_count, GST_SECOND, 25);
    GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale(1, GST_SECOND, 25);

    gst_buffer_add_video_meta_full (buffer, GST_VIDEO_FRAME_FLAG_NONE,
      format,
      width, height,
      n_plane, offset, stride);

    fail_unless(gst_pad_push(bmsrcpad, buffer) == GST_FLOW_OK);
    input_count++;
    length -= nr;
    if(input_count >= num) {
       break;
    }
  }

  fail_unless(gst_pad_push_event(bmsrcpad, gst_event_new_eos()));

  return TRUE;
}

static void
save_output(GstBuffer* buffer)
{
  fail_unless(output_file != NULL, "output file is NULL");

  GstMapInfo info;
  size_t w;

  gst_buffer_map(buffer, &info, GST_MAP_READ);
  w = fwrite(info.data, 1, info.size, output_file);
  fail_unless(w == info.size, "written size(%d) != info.size(%d)", (unsigned int)w, info.size);

  gst_buffer_unmap(buffer, &info);
}

static GstFlowReturn
bmsinkpad_chain(GstPad G_GNUC_UNUSED *pad, GstObject G_GNUC_UNUSED *parent, GstBuffer* buffer)
{
  save_output(buffer);

  gst_buffer_unref(buffer);

  output_count++;

  if (output_count >= frame_count) {
    // g_print("output_count(%d) >= frame_count(%d), EOS for MLU100\n",
    //    output_count, frame_count);
    // got_eos = TRUE;
  }

  return GST_FLOW_OK;
}

/* this function handles sink events */
static gboolean
bmsinkpad_event(GstPad* pad, GstObject* parent, GstEvent* event)
{
  gboolean ret = TRUE;

  // g_print("Received %s event: %p\n", GST_EVENT_TYPE_NAME(event), event);

  switch (GST_EVENT_TYPE(event)) {
    case GST_EVENT_CAPS: {
      GstCaps* caps;
      gst_event_parse_caps(event, &caps);
      //g_print("bmsinkpad_event_set_caps: %s\n", gst_caps_to_string(caps));
      gst_event_unref(event);
      break;
    }
    case GST_EVENT_EOS: {
      got_eos = TRUE;
      // g_print ("Got EOS\n");
      gst_event_unref(event);
      break;
    }
    default:
      ret = gst_pad_event_default(pad, parent, event);
      break;
  }
  return ret;
}

static GstElement*
setup_bmvideoenc(const gchar* src_caps_str, int codec, GstAllocator **allocator)
{
  GstElement* bmvideoenc;
  GstCaps* srccaps = NULL;
  GstQuery *query;

  if (src_caps_str) {
    srccaps = gst_caps_from_string(src_caps_str);
    fail_unless(srccaps != NULL);
  }
  // check factory make element
  if (codec == 0)
    bmvideoenc = gst_check_setup_element("bmh264enc");
  else if (codec == 1)
    bmvideoenc = gst_check_setup_element("bmh265enc");
  else if (codec == 2)
    bmvideoenc = gst_check_setup_element("bmjpegenc");

  query = gst_query_new_allocation (srccaps, TRUE);

  fail_unless(bmvideoenc != NULL);
  // check element's sink pad link
  bmsrcpad = gst_check_setup_src_pad(bmvideoenc, &srctemplate);


  bmsinkpad = gst_check_setup_sink_pad(bmvideoenc, &sinktemplate);
  gst_pad_set_chain_function(bmsinkpad, GST_DEBUG_FUNCPTR(bmsinkpad_chain));
  gst_pad_set_event_function(bmsinkpad, GST_DEBUG_FUNCPTR(bmsinkpad_event));
  gst_pad_set_active(bmsrcpad, TRUE);

  gst_pad_set_active(bmsinkpad, TRUE);
  // check START/SEGMENT/CAPS event
  gst_check_setup_events(bmsrcpad, bmvideoenc, srccaps, GST_FORMAT_TIME);


  // check element state change
  fail_unless(gst_element_set_state(bmvideoenc, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE,
              "could not set to playing");

  gst_pad_peer_query (bmsrcpad, query);
  if (gst_query_get_n_allocation_pools (query) > 0) {
    GstAllocationParams params;
    gst_query_parse_nth_allocation_param(query, 0, allocator, &params);
  }

  if (srccaps)
    gst_caps_unref(srccaps);

  if (query)
    gst_query_unref(query);

  buffers = NULL;
  return bmvideoenc;
}

static void
cleanup_bmvideoenc(GstElement* bmvideoenc)
{
  /* Free parsed buffers */
  gst_check_drop_buffers();

  gst_pad_set_active(bmsrcpad, FALSE);
  gst_pad_set_active(bmsinkpad, FALSE);
  gst_check_teardown_src_pad(bmvideoenc);
  gst_check_teardown_sink_pad(bmvideoenc);
  gst_check_teardown_element(bmvideoenc);
}

// check element factory make, this equals to gst_check_setup_element
GST_START_TEST(test_bmvideoenc_create_destroy)
{
  GstElement* bmvideoenc;

  g_print("test_bmvideoenc_create_destroy()\n");
  g_print("cwd: %s\n", GetExePath());

  bmvideoenc = gst_element_factory_make("bmh264enc", NULL);
  gst_object_unref(bmvideoenc);

  bmvideoenc = gst_element_factory_make("bmh265enc", NULL);
  gst_object_unref(bmvideoenc);
}
GST_END_TEST;

GST_START_TEST(test_bmvideoenc_property)
{
  GstElement* bmvideoenc;
  GstAllocator *allocator = NULL;
  g_print("test_bmvideoenc_property()\n");

  // setup the element for testing
  bmvideoenc = setup_bmvideoenc("video/x-raw,format=(string)I420,"
                            "width=(int)720,height=(int)480,"
                            "framerate=(fraction)25/1",
                            0, &allocator);

  gint prof = 0, level = 0, gop = 0, gop_preset = 0;
  guint br = 0, max_qp = 0, min_qp = 0;
  g_object_set(G_OBJECT(bmvideoenc), "bps", 1000, "profile", 66, "level", 40,
               "gop", 30, "qp-max", 50, "qp-min", 20, "gop-preset", 3, NULL);

  g_object_get(G_OBJECT(bmvideoenc), "gop", &gop, "profile", &prof,
               "level", &level,  "bps", &br, "qp-max", &max_qp, "qp-min", &min_qp, "gop-preset", &gop_preset, NULL);

  fail_unless_equals_int(prof, 66);
  fail_unless_equals_int(level, 40);
  fail_unless_equals_int(gop, 30);
  fail_unless_equals_int(gop_preset, 3);
  fail_unless_equals_int(br, 1000);
  fail_unless_equals_int(max_qp, 50);
  fail_unless_equals_int(min_qp, 20);
  // tear down the element
  cleanup_bmvideoenc(bmvideoenc);
  if (allocator) {
    gst_object_unref(allocator);
  }
}
GST_END_TEST;

GST_START_TEST(test_bmvideoenc_NV21_H264)
{
  GstElement* bmvideoenc;
  GstAllocator *allocator = NULL;
  g_print("test_bmvideoenc_NV21_H264()\n");

  // setup the element for testing
  bmvideoenc = setup_bmvideoenc("video/x-raw,format=(string)NV21,"
                            "width=(int)352,height=(int)288,"
                            "framerate=(fraction)25/1",
                            0, &allocator);

  setup_stream("../tests/data/coastguard_352x288_nv21.yuv",
              "coastguard_352x288_nv21.h264");

  feed_stream(GST_VIDEO_FORMAT_NV21, 352, 288, 10, allocator);

  // wait encoder finish encoding work
  while (got_eos == FALSE) {
    usleep(10000);
  }

  // g_print("encoder finished encoding!\n");

  cleanup_stream();

  if (allocator)
    gst_object_unref(allocator);

  // tear down the element
  cleanup_bmvideoenc(bmvideoenc);
}
GST_END_TEST;

GST_START_TEST(test_bmvideoenc_NV12_H264)
{
  GstElement* bmvideoenc;
  GstAllocator *allocator = NULL;

  g_print("test_bmvideoenc_NV12_H264()\n");

  // setup the element for testing
  bmvideoenc = setup_bmvideoenc("video/x-raw,format=(string)NV12,"
                            "width=(int)720,height=(int)480,"
                            "framerate=(fraction)25/1",
                            0, &allocator);

  setup_stream("../tests/data/sintel_trailer_720x480_nv12.yuv",
              "sintel_trailer_720x480_nv12.h264");
  feed_stream(GST_VIDEO_FORMAT_NV12, 720, 480, 10, allocator);

  // wait encoder finish encoding work
  while (got_eos == FALSE) {
    usleep(10000);
  }

  // g_print("encoder finished encoding!\n");

  cleanup_stream();

  if (allocator)
    gst_object_unref(allocator);

  // tear down the element
  cleanup_bmvideoenc(bmvideoenc);
}
GST_END_TEST;

GST_START_TEST(test_bmvideoenc_I420_H264)
{
  GstElement* bmvideoenc;
  GstAllocator *allocator = NULL;

  g_print("test_bmvideoenc_I420_H264()\n");

  // setup the element for testing
  bmvideoenc = setup_bmvideoenc("video/x-raw,format=(string)I420,"
                            "width=(int)352,height=(int)288,"
                            "framerate=(fraction)25/1",
                            0, &allocator);

  setup_stream("../tests/data/coastguard_352x288_i420.yuv",
              "coastguard_352x288_i420.h264");
  feed_stream(GST_VIDEO_FORMAT_I420, 352, 288, 10, allocator);

  // wait encoder finish encoding work
  while (got_eos == FALSE) {
    usleep(10000);
  }

  // g_print("encoder finished encoding!\n");

  cleanup_stream();

  if (allocator)
    gst_object_unref(allocator);

  // tear down the element
  cleanup_bmvideoenc(bmvideoenc);
}
GST_END_TEST;

GST_START_TEST(test_bmvideoenc_NV12_H265)
{
  GstElement* bmvideoenc;
  GstAllocator *allocator = NULL;

  g_print("test_bmvideoenc_NV12_H265()\n");

  // setup the element for testing
  bmvideoenc = setup_bmvideoenc("video/x-raw,format=(string)NV12,"
                            "width=(int)720,height=(int)480,"
                            "framerate=(fraction)25/1",
                            1, &allocator);

  setup_stream("../tests/data/sintel_trailer_720x480_nv12.yuv",
              "sintel_trailer_720x480_nv12.h265");
  feed_stream(GST_VIDEO_FORMAT_NV12, 720, 480, 10, allocator);

  // wait encoder finish encoding work
  while (got_eos == FALSE) {
    usleep(10000);
  }

  // g_print("encoder finished encoding!\n");

  cleanup_stream();

  if (allocator)
    gst_object_unref(allocator);

  // tear down the element
  cleanup_bmvideoenc(bmvideoenc);
}
GST_END_TEST;

GST_START_TEST(test_bmvideoenc_NV21_H265)
{
  GstElement* bmvideoenc;
  GstAllocator *allocator = NULL;

  g_print("test_bmvideoenc_NV21_H265()\n");

  // setup the element for testing
  bmvideoenc = setup_bmvideoenc("video/x-raw,format=(string)NV21,"
                            "width=(int)352,height=(int)288,"
                            "framerate=(fraction)25/1",
                            1, &allocator);

  setup_stream("../tests/data/coastguard_352x288_nv21.yuv",
               "coastguard_352x288_nv21.h265");
  feed_stream(GST_VIDEO_FORMAT_NV21, 352, 288, 10, allocator);

  // wait encoder finish encoding work
  while (got_eos == FALSE) {
    usleep(10000);
  }

  // g_print("encoder finished encoding!\n");

  cleanup_stream();

  if (allocator)
    gst_object_unref(allocator);

  // tear down the element
  cleanup_bmvideoenc(bmvideoenc);
}
GST_END_TEST;

GST_START_TEST(test_bmvideoenc_I420_H265)
{
  GstElement* bmvideoenc;
  GstAllocator *allocator = NULL;

  g_print("test_bmvideoenc_NV21_H265()\n");

  // setup the element for testing
  bmvideoenc = setup_bmvideoenc("video/x-raw,format=(string)I420,"
                            "width=(int)352,height=(int)288,"
                            "framerate=(fraction)25/1",
                            1, &allocator);

  setup_stream("../tests/data/coastguard_352x288_i420.yuv",
               "coastguard_352x288_i420.h265");
  feed_stream(GST_VIDEO_FORMAT_I420, 352, 288, 10, allocator);

  // wait encoder finish encoding work
  while (got_eos == FALSE) {
    usleep(10000);
  }

  // g_print("encoder finished encoding!\n");

  cleanup_stream();

  if (allocator)
    gst_object_unref(allocator);

  // tear down the element
  cleanup_bmvideoenc(bmvideoenc);
}
GST_END_TEST;

GST_START_TEST(test_bmvideoenc_NV12_JPEG)
{
  GstElement* bmvideoenc;
  GstAllocator *allocator = NULL;

  g_print("test_bmvideoenc_NV12_JPEG()\n");

  // setup the element for testing
  bmvideoenc = setup_bmvideoenc("video/x-raw,format=(string)NV12,"
                            "width=(int)720,height=(int)480,"
                            "framerate=(fraction)25/1",
                            2, &allocator);

  setup_stream("../tests/data/sintel_trailer_720x480_nv12.yuv",
              "sintel_trailer_720x480_nv12.jpg");
  feed_stream(GST_VIDEO_FORMAT_NV12, 720, 480, 1, allocator);

  // wait encoder finish encoding work
  while (got_eos == FALSE) {
    usleep(10000);
  }

  // g_print("encoder finished encoding!\n");

  cleanup_stream();

  if (allocator)
    gst_object_unref(allocator);

  // tear down the element
  cleanup_bmvideoenc(bmvideoenc);
}
GST_END_TEST;

GST_START_TEST(test_bmvideoenc_NV21_JPEG)
{
  GstElement* bmvideoenc;
  GstAllocator *allocator = NULL;

  g_print("test_bmvideoenc_NV21_JPEG()\n");

  // setup the element for testing
  bmvideoenc = setup_bmvideoenc("video/x-raw,format=(string)NV21,"
                            "width=(int)352,height=(int)288,"
                            "framerate=(fraction)25/1",
                            2, &allocator);

  setup_stream("../tests/data/coastguard_352x288_nv21.yuv",
               "coastguard_352x288_nv21.jpeg");
  feed_stream(GST_VIDEO_FORMAT_NV21, 352, 288, 1, allocator);

  // wait encoder finish encoding works
  while (got_eos == FALSE) {
    usleep(10000);
  }

  // g_print("encoder finished encoding!\n");

  cleanup_stream();

  if (allocator)
    gst_object_unref(allocator);

  // tear down the element
  cleanup_bmvideoenc(bmvideoenc);
}
GST_END_TEST;

GST_START_TEST(test_bmvideoenc_I420_JPEG)
{
  GstElement* bmvideoenc;
  GstAllocator *allocator = NULL;

  g_print("test_bmvideoenc_I420_JPEG()\n");

  // setup the element for testing
  bmvideoenc = setup_bmvideoenc("video/x-raw,format=(string)I420,"
                            "width=(int)352,height=(int)288,"
                            "framerate=(fraction)25/1",
                            2, &allocator);

  setup_stream("../tests/data/coastguard_352x288_i420.yuv",
               "coastguard_352x288_i420.jpeg");
  feed_stream(GST_VIDEO_FORMAT_NV21, 352, 288, 1, allocator);

  // wait encoder finish encoding works
  while (got_eos == FALSE) {
    usleep(10000);
  }

  // g_print("encoder finished encoding!\n");

  cleanup_stream();

  if (allocator)
    gst_object_unref(allocator);

  // tear down the element
  cleanup_bmvideoenc(bmvideoenc);
}
GST_END_TEST;

Suite*
bmvideoenc_suite(void)
{
  Suite* s = suite_create("bmvideo_enc");
  TCase* tc_chain = tcase_create("general");

  suite_add_tcase(s, tc_chain);

  tcase_add_test(tc_chain, test_bmvideoenc_create_destroy);
  tcase_add_test(tc_chain, test_bmvideoenc_property);
  tcase_add_test(tc_chain, test_bmvideoenc_NV12_H264);
  tcase_add_test(tc_chain, test_bmvideoenc_NV21_H264);
  tcase_add_test(tc_chain, test_bmvideoenc_I420_H264);
  tcase_add_test(tc_chain, test_bmvideoenc_NV12_H265);
  tcase_add_test(tc_chain, test_bmvideoenc_NV21_H265);
  tcase_add_test(tc_chain, test_bmvideoenc_I420_H265);
  tcase_add_test(tc_chain, test_bmvideoenc_NV12_JPEG);
  tcase_add_test(tc_chain, test_bmvideoenc_NV21_JPEG);
  tcase_add_test(tc_chain, test_bmvideoenc_I420_JPEG);
  return s;
}
