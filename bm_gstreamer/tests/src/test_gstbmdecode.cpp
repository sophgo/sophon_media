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

#include <gst/app/gstappsink.h>
#include <gst/check/gstbufferstraw.h>
#include <gst/check/gstcheck.h>
#include <gst/video/video.h>
#include <unistd.h>
#include <atomic>
#include <cstdint>

static void
src_handle_pad_added(GstElement G_GNUC_UNUSED *src, GstPad* new_pad, GstElement* sink)
{
  GstPad* sink_pad = gst_element_get_static_pad(sink, "sink");
  fail_if(!sink_pad);

  if (!gst_pad_is_linked(sink_pad)) {
    gst_pad_link(new_pad, sink_pad);
  }
  gst_object_unref(sink_pad);
}

static std::atomic<bool> eos(false);

/* Verify h264 is working when explictly requested by a pipeline. */
GST_START_TEST(test_h264dec_I420)
{
  GstElement *pipeline, *source, *parser, *dec, *appsink, *caps;
  guint test_frame_nums = 3;
  gchar current_path[128];
  memset(current_path, 0x00, sizeof(current_path));
  fail_unless(getcwd(current_path, sizeof(current_path) - 1));
  gchar* video_file = g_strconcat("file://", current_path, "/../samples/data/videos/test_1080p.h264", NULL);

  g_print("test h264 I420 \n");
  /* construct a pipeline that explicitly uses bmvideodec h264 */
  pipeline = gst_pipeline_new(NULL);

  source = gst_check_setup_element("uridecodebin");
  parser = gst_check_setup_element("h264parse");
  dec = gst_check_setup_element("bmdec");
  appsink = gst_check_setup_element("appsink");
  caps = gst_check_setup_element("capsfilter");

  GstCaps* h264_caps = gst_caps_new_empty_simple("video/x-h264");
  g_object_set(G_OBJECT(source), "caps", h264_caps, "uri", video_file, NULL);
  gst_caps_unref(h264_caps);
  g_free(video_file);

  GstCaps* i420_caps = gst_caps_from_string("video/x-raw, format=(string)I420");
  g_object_set(G_OBJECT(caps), "caps", i420_caps, NULL);
  gst_caps_unref(i420_caps);

  g_signal_connect(source, "pad-added", G_CALLBACK(src_handle_pad_added), parser);

  gst_bin_add_many(GST_BIN(pipeline), source, parser, dec, caps, appsink, NULL);
  fail_unless(gst_element_link_many(parser, dec, caps, appsink, NULL));
  GstPad* sinkpad = gst_element_get_static_pad(appsink, "sink");
  gst_buffer_straw_start_pipeline(pipeline, sinkpad);

  for (guint i = 0; i < test_frame_nums; i++) {
    GstBuffer* buf;
    buf = gst_buffer_straw_get_buffer(pipeline, sinkpad);
    gst_buffer_unref(buf);
  }
  gst_buffer_straw_stop_pipeline(pipeline, sinkpad);
  gst_object_unref(pipeline);
  gst_object_unref(sinkpad);
}
GST_END_TEST;

/* Verify h264 is working when explictly requested by a pipeline. */
GST_START_TEST(test_h264dec_NV12)
{
  GstElement *pipeline, *source, *parser, *dec, *appsink, *caps;
  guint test_frame_nums = 3;
  gchar current_path[128];
  memset(current_path, 0x00, sizeof(current_path));
  fail_unless(getcwd(current_path, sizeof(current_path) - 1));
  gchar* video_file = g_strconcat("file://", current_path, "/../samples/data/videos/test_1080p.h264", NULL);

  g_print("test h264 NV12 \n");
  /* construct a pipeline that explicitly uses bmvideodec h264 */
  pipeline = gst_pipeline_new(NULL);

  source = gst_check_setup_element("uridecodebin");
  parser = gst_check_setup_element("h264parse");
  dec = gst_check_setup_element("bmdec");
  appsink = gst_check_setup_element("appsink");
  caps = gst_check_setup_element("capsfilter");

  GstCaps* h264_caps = gst_caps_new_empty_simple("video/x-h264");
  g_object_set(G_OBJECT(source), "caps", h264_caps, "uri", video_file, NULL);
  gst_caps_unref(h264_caps);
  g_free(video_file);

  GstCaps* nv12_caps = gst_caps_from_string("video/x-raw, format=(string)NV12");
  g_object_set(G_OBJECT(caps), "caps", nv12_caps, NULL);
  gst_caps_unref(nv12_caps);

  g_signal_connect(source, "pad-added", G_CALLBACK(src_handle_pad_added), parser);

  gst_bin_add_many(GST_BIN(pipeline), source, parser, dec, caps, appsink, NULL);
  fail_unless(gst_element_link_many(parser, dec, caps, appsink, NULL));
  GstPad* sinkpad = gst_element_get_static_pad(appsink, "sink");
  gst_buffer_straw_start_pipeline(pipeline, sinkpad);

  for (guint i = 0; i < test_frame_nums; i++) {
    GstBuffer* buf;
    buf = gst_buffer_straw_get_buffer(pipeline, sinkpad);
    gst_buffer_unref(buf);
  }
  gst_buffer_straw_stop_pipeline(pipeline, sinkpad);
  gst_object_unref(pipeline);
  gst_object_unref(sinkpad);
}
GST_END_TEST;

/* Verify h264 is working when explictly requested by a pipeline. */
GST_START_TEST(test_h264dec_NV21)
{
  GstElement *pipeline, *source, *parser, *dec, *appsink, *caps;
  guint test_frame_nums = 3;
  gchar current_path[128];
  memset(current_path, 0x00, sizeof(current_path));
  fail_unless(getcwd(current_path, sizeof(current_path) - 1));
  gchar* video_file = g_strconcat("file://", current_path, "/../samples/data/videos/test_1080p.h264", NULL);

  g_print("test h264 NV21 \n");
  /* construct a pipeline that explicitly uses bmvideodec h264 */
  pipeline = gst_pipeline_new(NULL);

  source = gst_check_setup_element("uridecodebin");
  parser = gst_check_setup_element("h264parse");
  dec = gst_check_setup_element("bmdec");
  appsink = gst_check_setup_element("appsink");
  caps = gst_check_setup_element("capsfilter");

  GstCaps* h264_caps = gst_caps_new_empty_simple("video/x-h264");
  g_object_set(G_OBJECT(source), "caps", h264_caps, "uri", video_file, NULL);
  gst_caps_unref(h264_caps);
  g_free(video_file);

  GstCaps* nv21_caps = gst_caps_from_string("video/x-raw, format=(string)NV21");
  g_object_set(G_OBJECT(caps), "caps", nv21_caps, NULL);
  gst_caps_unref(nv21_caps);

  g_signal_connect(source, "pad-added", G_CALLBACK(src_handle_pad_added), parser);

  gst_bin_add_many(GST_BIN(pipeline), source, parser, dec, caps, appsink, NULL);
  fail_unless(gst_element_link_many(parser, dec, caps, appsink, NULL));
  GstPad* sinkpad = gst_element_get_static_pad(appsink, "sink");
  gst_buffer_straw_start_pipeline(pipeline, sinkpad);

  for (guint i = 0; i < test_frame_nums; i++) {
    GstBuffer* buf;
    buf = gst_buffer_straw_get_buffer(pipeline, sinkpad);
    gst_buffer_unref(buf);
  }
  gst_buffer_straw_stop_pipeline(pipeline, sinkpad);
  gst_object_unref(pipeline);
  gst_object_unref(sinkpad);
}
GST_END_TEST;

/* Verify h265 is working when explictly requested by a pipeline. */
GST_START_TEST(test_h265dec_I420)
{
  GstElement *pipeline, *source, *parser, *dec, *appsink, *caps;
  guint test_frame_nums = 3;
  gchar current_path[128];
  memset(current_path, 0x00, sizeof(current_path));
  fail_unless(getcwd(current_path, sizeof(current_path) - 1));
  gchar* video_file = g_strconcat("file://", current_path, "/../samples/data/videos/test_1080p.h265", NULL);

  g_print("test h265 I420 \n");
  /* construct a pipeline that explicitly uses bmvideodec h265*/
  pipeline = gst_pipeline_new(NULL);

  source = gst_check_setup_element("uridecodebin");
  parser = gst_check_setup_element("h265parse");
  dec = gst_check_setup_element("bmdec");
  appsink = gst_check_setup_element("appsink");
  caps = gst_check_setup_element("capsfilter");

  GstCaps* h265_caps = gst_caps_new_empty_simple("video/x-h265");
  g_object_set(G_OBJECT(source), "caps", h265_caps, "uri", video_file, NULL);
  gst_caps_unref(h265_caps);
  g_free(video_file);

  GstCaps* i420_caps = gst_caps_from_string("video/x-raw, format=(string)I420");
  g_object_set(G_OBJECT(caps), "caps", i420_caps, NULL);
  gst_caps_unref(i420_caps);

  g_signal_connect(source, "pad-added", G_CALLBACK(src_handle_pad_added), parser);

  gst_bin_add_many(GST_BIN(pipeline), source, parser, dec, caps, appsink, NULL);
  fail_unless(gst_element_link_many(parser, dec, caps, appsink, NULL));
  GstPad* sinkpad = gst_element_get_static_pad(appsink, "sink");
  gst_buffer_straw_start_pipeline(pipeline, sinkpad);

  for (guint i = 0; i < test_frame_nums; i++) {
    GstBuffer* buf;
    buf = gst_buffer_straw_get_buffer(pipeline, sinkpad);
    gst_buffer_unref(buf);
  }

  gst_buffer_straw_stop_pipeline(pipeline, sinkpad);
  gst_object_unref(pipeline);
  gst_object_unref(sinkpad);
}
GST_END_TEST;
GST_START_TEST(test_h265dec_NV12)
{
  GstElement *pipeline, *source, *parser, *dec, *appsink, *caps;
  guint test_frame_nums = 3;
  gchar current_path[128];
  memset(current_path, 0x00, sizeof(current_path));
  fail_unless(getcwd(current_path, sizeof(current_path) - 1));
  gchar* video_file = g_strconcat("file://", current_path, "/../samples/data/videos/test_1080p.h265", NULL);

  g_print("test h265 NV12 \n");
  /* construct a pipeline that explicitly uses bmvideodec h265*/
  pipeline = gst_pipeline_new(NULL);

  source = gst_check_setup_element("uridecodebin");
  parser = gst_check_setup_element("h265parse");
  dec = gst_check_setup_element("bmdec");
  appsink = gst_check_setup_element("appsink");
  caps = gst_check_setup_element("capsfilter");

  GstCaps* h265_caps = gst_caps_new_empty_simple("video/x-h265");
  g_object_set(G_OBJECT(source), "caps", h265_caps, "uri", video_file, NULL);
  gst_caps_unref(h265_caps);
  g_free(video_file);

  GstCaps* nv12_caps = gst_caps_from_string("video/x-raw, format=(string)NV12");
  g_object_set(G_OBJECT(caps), "caps", nv12_caps, NULL);
  gst_caps_unref(nv12_caps);

  g_signal_connect(source, "pad-added", G_CALLBACK(src_handle_pad_added), parser);

  gst_bin_add_many(GST_BIN(pipeline), source, parser, dec, caps, appsink, NULL);
  fail_unless(gst_element_link_many(parser, dec, caps, appsink, NULL));
  GstPad* sinkpad = gst_element_get_static_pad(appsink, "sink");
  gst_buffer_straw_start_pipeline(pipeline, sinkpad);

  for (guint i = 0; i < test_frame_nums; i++) {
    GstBuffer* buf;
    buf = gst_buffer_straw_get_buffer(pipeline, sinkpad);
    gst_buffer_unref(buf);
  }

  gst_buffer_straw_stop_pipeline(pipeline, sinkpad);
  gst_object_unref(pipeline);
  gst_object_unref(sinkpad);
}
GST_END_TEST;
GST_START_TEST(test_h265dec_NV21)
{
  GstElement *pipeline, *source, *parser, *dec, *appsink, *caps;
  guint test_frame_nums = 3;
  gchar current_path[128];
  memset(current_path, 0x00, sizeof(current_path));
  fail_unless(getcwd(current_path, sizeof(current_path) - 1));
  gchar* video_file = g_strconcat("file://", current_path, "/../samples/data/videos/test_1080p.h265", NULL);

  g_print("test h265 NV21 \n");
  /* construct a pipeline that explicitly uses bmvideodec h265*/
  pipeline = gst_pipeline_new(NULL);

  source = gst_check_setup_element("uridecodebin");
  parser = gst_check_setup_element("h265parse");
  dec = gst_check_setup_element("bmdec");
  appsink = gst_check_setup_element("appsink");
  caps = gst_check_setup_element("capsfilter");

  GstCaps* h265_caps = gst_caps_new_empty_simple("video/x-h265");
  g_object_set(G_OBJECT(source), "caps", h265_caps, "uri", video_file, NULL);
  gst_caps_unref(h265_caps);
  g_free(video_file);

  GstCaps* nv21_caps = gst_caps_from_string("video/x-raw, format=(string)NV21");
  g_object_set(G_OBJECT(caps), "caps", nv21_caps, NULL);
  gst_caps_unref(nv21_caps);

  g_signal_connect(source, "pad-added", G_CALLBACK(src_handle_pad_added), parser);

  gst_bin_add_many(GST_BIN(pipeline), source, parser, dec, caps, appsink, NULL);
  fail_unless(gst_element_link_many(parser, dec, caps, appsink, NULL));
  GstPad* sinkpad = gst_element_get_static_pad(appsink, "sink");
  gst_buffer_straw_start_pipeline(pipeline, sinkpad);

  for (guint i = 0; i < test_frame_nums; i++) {
    GstBuffer* buf;
    buf = gst_buffer_straw_get_buffer(pipeline, sinkpad);
    gst_buffer_unref(buf);
  }

  gst_buffer_straw_stop_pipeline(pipeline, sinkpad);
  gst_object_unref(pipeline);
  gst_object_unref(sinkpad);
}
GST_END_TEST;

GST_START_TEST(test_jpegdec_I420)
{
  GstElement *pipeline, *source, *parser, *dec, *appsink, *caps;
  guint test_frame_nums = 1;
  gchar current_path[128];
  memset(current_path, 0x00, sizeof(current_path));
  fail_unless(getcwd(current_path, sizeof(current_path) - 1));
  gchar* video_file = g_strconcat("file://", current_path, "/../samples/data/videos/test_1080p.jpg", NULL);

  g_print("test jpeg i420 \n");
  /* construct a pipeline that explicitly uses bmvideodec*/
  pipeline = gst_pipeline_new(NULL);

  source = gst_check_setup_element("uridecodebin");
  parser = gst_check_setup_element("jpegparse");
  dec = gst_check_setup_element("bmdec");
  appsink = gst_check_setup_element("appsink");
  caps = gst_check_setup_element("capsfilter");

  GstCaps* jpeg_caps = gst_caps_new_empty_simple("image/jpeg");
  g_object_set(G_OBJECT(source), "caps", jpeg_caps, "uri", video_file, NULL);
  gst_caps_unref(jpeg_caps);
  g_free(video_file);

  GstCaps* i420_caps = gst_caps_from_string("video/x-raw, format=(string)I420");
  g_object_set(G_OBJECT(caps), "caps", i420_caps, NULL);
  gst_caps_unref(i420_caps);

  g_signal_connect(source, "pad-added", G_CALLBACK(src_handle_pad_added), parser);

  gst_bin_add_many(GST_BIN(pipeline), source, parser, dec, caps, appsink, NULL);
  fail_unless(gst_element_link_many(parser, dec, caps, appsink, NULL));
  GstPad* sinkpad = gst_element_get_static_pad(appsink, "sink");
  gst_buffer_straw_start_pipeline(pipeline, sinkpad);

  for (guint i = 0; i < test_frame_nums; i++) {
    GstBuffer* buf;
    buf = gst_buffer_straw_get_buffer(pipeline, sinkpad);
    gst_buffer_unref(buf);
  }

  gst_buffer_straw_stop_pipeline(pipeline, sinkpad);
  gst_object_unref(pipeline);
  gst_object_unref(sinkpad);
}
GST_END_TEST;

GST_START_TEST(test_jpegdec_NV12)
{
  GstElement *pipeline, *source, *parser, *dec, *appsink, *caps;
  guint test_frame_nums = 1;
  gchar current_path[128];
  memset(current_path, 0x00, sizeof(current_path));
  fail_unless(getcwd(current_path, sizeof(current_path) - 1));
  gchar* video_file = g_strconcat("file://", current_path, "/../samples/data/videos/test_1080p.jpg", NULL);

  g_print("test jpeg nv12 \n");
  /* construct a pipeline that explicitly uses bmvideodec*/
  pipeline = gst_pipeline_new(NULL);

  source = gst_check_setup_element("uridecodebin");
  parser = gst_check_setup_element("jpegparse");
  dec = gst_check_setup_element("bmdec");
  appsink = gst_check_setup_element("appsink");
  caps = gst_check_setup_element("capsfilter");

  GstCaps* jpeg_caps = gst_caps_new_empty_simple("image/jpeg");
  g_object_set(G_OBJECT(source), "caps", jpeg_caps, "uri", video_file, NULL);
  gst_caps_unref(jpeg_caps);
  g_free(video_file);

  GstCaps* nv12_caps = gst_caps_from_string("video/x-raw, format=(string)NV12");
  g_object_set(G_OBJECT(caps), "caps", nv12_caps, NULL);
  gst_caps_unref(nv12_caps);

  g_signal_connect(source, "pad-added", G_CALLBACK(src_handle_pad_added), parser);

  gst_bin_add_many(GST_BIN(pipeline), source, parser, dec, caps, appsink, NULL);
  fail_unless(gst_element_link_many(parser, dec, caps, appsink, NULL));
  GstPad* sinkpad = gst_element_get_static_pad(appsink, "sink");
  gst_buffer_straw_start_pipeline(pipeline, sinkpad);

  for (guint i = 0; i < test_frame_nums; i++) {
    GstBuffer* buf;
    buf = gst_buffer_straw_get_buffer(pipeline, sinkpad);
    gst_buffer_unref(buf);
  }

  gst_buffer_straw_stop_pipeline(pipeline, sinkpad);
  gst_object_unref(pipeline);
  gst_object_unref(sinkpad);
}
GST_END_TEST;
GST_START_TEST(test_jpegdec_NV21)
{
  GstElement *pipeline, *source, *parser, *dec, *appsink, *caps;
  guint test_frame_nums = 1;
  gchar current_path[128];
  memset(current_path, 0x00, sizeof(current_path));
  fail_unless(getcwd(current_path, sizeof(current_path) - 1));
  gchar* video_file = g_strconcat("file://", current_path, "/../samples/data/videos/test_1080p.jpg", NULL);

  g_print("test jpeg nv21 \n");
  /* construct a pipeline that explicitly uses bmvideodec*/
  pipeline = gst_pipeline_new(NULL);

  source = gst_check_setup_element("uridecodebin");
  parser = gst_check_setup_element("jpegparse");
  dec = gst_check_setup_element("bmdec");
  appsink = gst_check_setup_element("appsink");
  caps = gst_check_setup_element("capsfilter");

  GstCaps* jpeg_caps = gst_caps_new_empty_simple("image/jpeg");
  g_object_set(G_OBJECT(source), "caps", jpeg_caps, "uri", video_file, NULL);
  gst_caps_unref(jpeg_caps);
  g_free(video_file);

  GstCaps* nv21_caps = gst_caps_from_string("video/x-raw, format=(string)NV21");
  g_object_set(G_OBJECT(caps), "caps", nv21_caps, NULL);
  gst_caps_unref(nv21_caps);

  g_signal_connect(source, "pad-added", G_CALLBACK(src_handle_pad_added), parser);

  gst_bin_add_many(GST_BIN(pipeline), source, parser, dec, caps, appsink, NULL);

  fail_unless(gst_element_link_many(parser, dec, caps, appsink, NULL));
  GstPad* sinkpad = gst_element_get_static_pad(appsink, "sink");
  gst_buffer_straw_start_pipeline(pipeline, sinkpad);

  for (guint i = 0; i < test_frame_nums; i++) {
    GstBuffer* buf;
    buf = gst_buffer_straw_get_buffer(pipeline, sinkpad);
    gst_buffer_unref(buf);
  }

  gst_buffer_straw_stop_pipeline(pipeline, sinkpad);
  gst_object_unref(pipeline);
  gst_object_unref(sinkpad);
}
GST_END_TEST;
GST_START_TEST(test_jpegdec_NV16)
{
  GstElement *pipeline, *source, *parser, *dec, *appsink, *caps;
  guint test_frame_nums = 1;
  gchar current_path[128];
  memset(current_path, 0x00, sizeof(current_path));
  fail_unless(getcwd(current_path, sizeof(current_path) - 1));
  gchar* video_file = g_strconcat("file://", current_path, "/../samples/data/videos/test_1080p.jpg", NULL);

  g_print("test jpeg nv16 \n");
  /* construct a pipeline that explicitly uses bmvideodec*/
  pipeline = gst_pipeline_new(NULL);

  source = gst_check_setup_element("uridecodebin");
  parser = gst_check_setup_element("jpegparse");
  dec = gst_check_setup_element("bmdec");
  appsink = gst_check_setup_element("appsink");
  caps = gst_check_setup_element("capsfilter");

  GstCaps* jpeg_caps = gst_caps_new_empty_simple("image/jpeg");
  g_object_set(G_OBJECT(source), "caps", jpeg_caps, "uri", video_file, NULL);
  gst_caps_unref(jpeg_caps);
  g_free(video_file);

  GstCaps* nv16_caps = gst_caps_from_string("video/x-raw, format=(string)NV16");
  g_object_set(G_OBJECT(caps), "caps", nv16_caps, NULL);
  gst_caps_unref(nv16_caps);

  g_signal_connect(source, "pad-added", G_CALLBACK(src_handle_pad_added), parser);

  gst_bin_add_many(GST_BIN(pipeline), source, parser, dec, caps, appsink, NULL);

  fail_unless(gst_element_link_many(parser, dec, caps, appsink, NULL));
  GstPad* sinkpad = gst_element_get_static_pad(appsink, "sink");
  gst_buffer_straw_start_pipeline(pipeline, sinkpad);

  for (guint i = 0; i < test_frame_nums; i++) {
    GstBuffer* buf;
    buf = gst_buffer_straw_get_buffer(pipeline, sinkpad);
    gst_buffer_unref(buf);
  }

  gst_buffer_straw_stop_pipeline(pipeline, sinkpad);
  gst_object_unref(pipeline);
  gst_object_unref(sinkpad);
}
GST_END_TEST;
#if 0
GST_START_TEST(test_jpegdec_YUV422)
{
  GstElement *pipeline, *source, *parser, *dec, *appsink, *caps;
  guint test_frame_nums = 1;
  gchar current_path[128];
  memset(current_path, 0x00, sizeof(current_path));
  fail_unless(getcwd(current_path, sizeof(current_path) - 1));
  gchar* video_file = g_strconcat("file://", current_path, "/../samples/data/videos/test_1080p.jpg", NULL);

  g_print("test jpeg YUV422 \n");
  /* construct a pipeline that explicitly uses bmvideodec*/
  pipeline = gst_pipeline_new(NULL);

  source = gst_check_setup_element("uridecodebin");
  parser = gst_check_setup_element("jpegparse");
  dec = gst_check_setup_element("bmdec");
  appsink = gst_check_setup_element("appsink");
  caps = gst_check_setup_element("capsfilter");

  GstCaps* jpeg_caps = gst_caps_new_empty_simple("image/jpeg");
  g_object_set(G_OBJECT(source), "caps", jpeg_caps, "uri", video_file, NULL);
  gst_caps_unref(jpeg_caps);
  g_free(video_file);

  GstCaps* y42b_caps = gst_caps_from_string("video/x-raw, format=(string)Y42B");
  g_object_set(G_OBJECT(caps), "caps", y42b_caps, NULL);
  gst_caps_unref(y42b_caps);

  g_signal_connect(source, "pad-added", G_CALLBACK(src_handle_pad_added), parser);

  gst_bin_add_many(GST_BIN(pipeline), source, parser, dec, caps, appsink, NULL);

  fail_unless(gst_element_link_many(parser, dec, caps, appsink, NULL));
  GstPad* sinkpad = gst_element_get_static_pad(appsink, "sink");
  gst_buffer_straw_start_pipeline(pipeline, sinkpad);

  for (guint i = 0; i < test_frame_nums; i++) {
    GstBuffer* buf;
    buf = gst_buffer_straw_get_buffer(pipeline, sinkpad);
    gst_buffer_unref(buf);
  }

  gst_buffer_straw_stop_pipeline(pipeline, sinkpad);
  gst_object_unref(pipeline);
  gst_object_unref(sinkpad);
}
GST_END_TEST;
GST_START_TEST(test_jpegdec_YUV444)
{
  GstElement *pipeline, *source, *parser, *dec, *appsink, *caps;
  guint test_frame_nums = 1;
  gchar current_path[128];
  memset(current_path, 0x00, sizeof(current_path));
  fail_unless(getcwd(current_path, sizeof(current_path) - 1));
  gchar* video_file = g_strconcat("file://", current_path, "/../samples/data/videos/test_1080p.jpg", NULL);

  g_print("test jpeg YUV444 \n");
  /* construct a pipeline that explicitly uses bmvideodec*/
  pipeline = gst_pipeline_new(NULL);

  source = gst_check_setup_element("uridecodebin");
  parser = gst_check_setup_element("jpegparse");
  dec = gst_check_setup_element("bmdec");
  appsink = gst_check_setup_element("appsink");
  caps = gst_check_setup_element("capsfilter");

  GstCaps* jpeg_caps = gst_caps_new_empty_simple("image/jpeg");
  g_object_set(G_OBJECT(source), "caps", jpeg_caps, "uri", video_file, NULL);
  gst_caps_unref(jpeg_caps);
  g_free(video_file);

  GstCaps* y444_caps = gst_caps_from_string("video/x-raw, format=(string)Y444");
  g_object_set(G_OBJECT(caps), "caps", y444_caps, NULL);
  gst_caps_unref(y444_caps);

  g_signal_connect(source, "pad-added", G_CALLBACK(src_handle_pad_added), parser);

  gst_bin_add_many(GST_BIN(pipeline), source, parser, dec, caps, appsink, NULL);

  fail_unless(gst_element_link_many(parser, dec, caps, appsink, NULL));
  GstPad* sinkpad = gst_element_get_static_pad(appsink, "sink");
  gst_buffer_straw_start_pipeline(pipeline, sinkpad);

  for (guint i = 0; i < test_frame_nums; i++) {
    GstBuffer* buf;
    buf = gst_buffer_straw_get_buffer(pipeline, sinkpad);
    gst_buffer_unref(buf);
  }

  gst_buffer_straw_stop_pipeline(pipeline, sinkpad);
  gst_object_unref(pipeline);
  gst_object_unref(sinkpad);
}
GST_END_TEST;
#endif
Suite*
bmvideodec_suite(void)
{
  Suite* s = suite_create("bmdec");
  TCase* tc_chain = tcase_create("general");

  suite_add_tcase(s, tc_chain);
  tcase_add_test(tc_chain, test_h264dec_I420);
  tcase_add_test(tc_chain, test_h264dec_NV12);
  tcase_add_test(tc_chain, test_h264dec_NV21);
  tcase_add_test(tc_chain, test_h265dec_I420);
  tcase_add_test(tc_chain, test_h265dec_NV12);
  tcase_add_test(tc_chain, test_h265dec_NV21);
  tcase_add_test(tc_chain, test_jpegdec_I420);
  tcase_add_test(tc_chain, test_jpegdec_NV12);
  tcase_add_test(tc_chain, test_jpegdec_NV21);
  tcase_add_test(tc_chain, test_jpegdec_NV16);
  //tcase_add_test(tc_chain, test_jpegdec_YUV422);
  //tcase_add_test(tc_chain, test_jpegdec_YUV444);

  return s;
}
