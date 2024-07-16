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

#include <gst/gst.h>
#include <iostream>
#include <string>

#define MAX_NAME_LENGTH 20

typedef enum {
  CODEC_NONE,
  CODEC_H264,
  CODEC_H265,
  CODEC_JPEG,
  CODEC_BIN, /*for input format no specified user decodebin pad */
  CODEC_BUTT,
}CODEC_TYPE;

#define CHECK(ret, output)           \
  if (!ret) {                        \
    GST_ERROR("ERROR: %s", output);  \
    return -1;                       \
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


static void
src_handle_pad_added (GstElement G_GNUC_UNUSED *src, GstPad * new_pad, GstElement * sink)
{
  GstPad *sink_pad = gst_element_get_static_pad (sink, "sink");
  if (!sink_pad) {
    GST_INFO ("have not get the pad\n");
  }
  GstPadLinkReturn ret;

  GstCaps *new_pad_caps = NULL;
  GstStructure *new_pad_structure = NULL;
  const gchar *new_pad_type = NULL;
  new_pad_caps = gst_pad_get_current_caps (new_pad);
  new_pad_structure = gst_caps_get_structure (new_pad_caps, 0);
  new_pad_type = gst_structure_get_name (new_pad_structure);
  g_print("new_pad_type %s\n", new_pad_type);

  if (gst_pad_is_linked (sink_pad)) {
    GST_INFO ("already linked, Ignoring.\n");
    gst_object_unref (sink_pad);
    return;
  }

  ret = gst_pad_link (new_pad, sink_pad);
  if (GST_PAD_LINK_FAILED (ret)) {
    GST_INFO ("link failed\n");
  } else {
    GST_INFO ("link succeed\n");
  }
  gst_object_unref (sink_pad);
  gst_caps_unref(new_pad_caps);
}

GstElement *pipeline = NULL, *src = NULL, *parse = NULL, *dec = NULL, *encode = NULL, *sink = NULL;

typedef struct _ElementGroup {
    GstElement *pipeline;
    GstElement *dec;
    GstElement *encode;
    GstElement *sink;
}ElementGroup;

static void
src_pad_added (GstElement G_GNUC_UNUSED *src, GstPad * new_pad, ElementGroup *group)
{
  GstCaps *new_pad_caps = NULL;
  GstStructure *new_pad_structure = NULL;
  const gchar *new_pad_type = NULL;
  new_pad_caps = gst_pad_get_current_caps (new_pad);
  new_pad_structure = gst_caps_get_structure (new_pad_caps, 0);
  new_pad_type = gst_structure_get_name (new_pad_structure);
  g_print("new_pad_type %s\n", new_pad_type);
  GstElement *parser = NULL;
  GstElement *pipeline = group->pipeline;

  if (g_str_has_prefix(new_pad_type, "video/x-h264")) {
    parser = gst_element_factory_make("h264parse", "h264-parser");
  } else if (g_str_has_prefix(new_pad_type, "video/x-h265")) {
    parser = gst_element_factory_make("h265parse", "h265-parser");
  } else if (g_str_has_prefix(new_pad_type, "image/jpeg")) {
    parser = gst_element_factory_make ("jpegparse", "parse");
  }

  if (parser) {
    gst_bin_add(GST_BIN(pipeline), parser);
    // 连接 pad
    gst_element_sync_state_with_parent(parser);
    gst_element_sync_state_with_parent(group->dec);

    GstPad *sink_pad = gst_element_get_static_pad(parser, "sink");
    if (gst_pad_link(new_pad, sink_pad) != GST_PAD_LINK_OK) {
        g_print("Failed to link pads\n");
    }
    gst_object_unref(sink_pad);

    if (!gst_element_link(parser, group->dec)) {
        g_print("Failed to link parser and decoder\n");
    }
  } else {
    GstPad *sink_pad = gst_element_get_static_pad(group->dec, "sink");
    if (gst_pad_is_linked (sink_pad)) {
      GST_INFO ("already linked, Ignoring.\n");
      gst_object_unref (sink_pad);
      return;
    }
    if (gst_pad_link(new_pad, sink_pad) != GST_PAD_LINK_OK) {
        g_print("Failed to link pads\n");
    }
    gst_object_unref(sink_pad);
    gst_caps_unref(new_pad_caps);
  }
}

int main (int argc, char **argv) {
  gchar *video_path = NULL, *output_path = NULL;
  GOptionContext *ctx;
  char *absolute_path;
  GError *err = NULL;

  gint  cmd_gop = -1, cmd_gop_preset = -1;
  gint  cmd_bps = -1, cmd_max_qp = -1, cmd_min_qp = -1, cmd_cqp = -1;
  gint  cmd_qfactor = 0;

  gint  gop = 0, gop_preset = 0;
  gint  dec_type = 0, enc_type = 0;
  guint br = 0, max_qp = 0, min_qp = 0, cqp = -1, qfactor;
  GOptionEntry entries[] = {
    { "video_path", 'v', 0, G_OPTION_ARG_STRING, &video_path,
      "The path of the video file.", "FILE" },
    { "output_path", 'o', 0, G_OPTION_ARG_STRING, &output_path,
      "The path to save output transcode video file", "FILE" },
    { "dec_type", 'd', 0, G_OPTION_ARG_INT, &dec_type,
      "h26x input codec format type 1:h264 2:h265 3:JPEG 4: decode bin chose by gst", "type" },
    { "enc_type", 'e', 0, G_OPTION_ARG_INT, &enc_type,
      "h26x the output codec format type 1:h264 2:h265 3:JPEG", "type" },
    { "bps", 'b', 0, G_OPTION_ARG_INT, &cmd_bps,
      "enc target bitrate ", "bps" },
    { "gop", 'g', 0, G_OPTION_ARG_INT, &cmd_gop,
      "h26x enc Group of pictures starting with I frame", "gop" },
    { "gop_preset", 'p', 0, G_OPTION_ARG_INT, &cmd_gop_preset,
      "h26x enc A GOP structure preset option [1-7] ", "gop_preset" },
    { "cqp", 'q', 0, G_OPTION_ARG_INT, &cmd_cqp,
      "h26x enc constant quality mode set >=0 bsp invalid", "cqp"},
    { "qp_min", 'm', 0, G_OPTION_ARG_INT, &cmd_min_qp,
      "h26x enc Min QP", "qp_min"},
    { "qp_max", 'x', 0, G_OPTION_ARG_INT, &cmd_max_qp,
      "h26x enc Max QP", "qp_max"},
    { "q_factor", 'f', 0, G_OPTION_ARG_INT, &cmd_qfactor,
      "jpeg quality", "q_factor"},
    { NULL, ' ', 0, G_OPTION_ARG_STRING, NULL, NULL, NULL}
  };

  g_print("g_option_context_new in \n");
  ctx = g_option_context_new ("demo");
  g_option_context_add_main_entries (ctx, entries, NULL);
  g_option_context_add_group (ctx, gst_init_get_option_group ());
  if (!g_option_context_parse (ctx, &argc, &argv, &err)) {
    g_print ("Failed to initialize: %s\n", err->message);
    g_error_free (err);
    return 1;
  }

  gst_init (&argc, &argv);

  if (!dec_type || !enc_type || dec_type > CODEC_BIN || enc_type > CODEC_JPEG) {
    g_print (" codec type set fail dec_type %d enc_type %d \n", dec_type, enc_type);
    return 1;
  }

  std::cout << "video path: " << video_path << std::endl;
  std::cout << "output path: " << output_path << std::endl;
  std::string vpath = "file://";
    // 使用 realpath 获取绝对路径
  absolute_path = realpath(video_path, NULL);
  vpath += absolute_path;
  std::string opath = output_path;
  if (absolute_path) {
    free(absolute_path);
  }

  g_print("create in \n");
  GstElement *pipeline = NULL, *src = NULL, *parse = NULL, *dec = NULL, *encode = NULL, *sink = NULL;
  GstCaps *caps = NULL;
  ElementGroup elem_group;

  GST_INFO ("create pipeline\n");
  pipeline = gst_pipeline_new ("pipeline");

  src = gst_element_factory_make ("uridecodebin", "src");
  switch(dec_type) {
  case CODEC_H264:
    caps = gst_caps_new_empty_simple ("video/x-h264");
    parse = gst_element_factory_make ("h264parse", "parse");
    break;
  case CODEC_H265:
    caps = gst_caps_new_empty_simple ("video/x-h265");
    parse = gst_element_factory_make ("h265parse", "parse");
    break;
  case CODEC_JPEG:
    parse = gst_element_factory_make ("jpegparse", "parse");
    caps = gst_caps_new_empty_simple ("image/jpeg");
    break;
  case CODEC_BIN:
    caps = gst_caps_from_string("video/x-h264, stream-format=(string)byte-stream, alignment=(string)au; "
                                "video/x-h265, stream-format=(string)byte-stream, alignment=(string)au; "
                                "image/jpeg;");
    break;
  default:
      g_print("no support in dec codec type \n");
    break;
  }

  dec = gst_element_factory_make ("bmdec", "dec");

  switch(enc_type) {
  case CODEC_H264:
    encode = gst_element_factory_make ("bmh264enc", "enc");
    break;
  case CODEC_H265:
    encode = gst_element_factory_make ("bmh265enc", "enc");
    break;
  case CODEC_JPEG:
     encode = gst_element_factory_make ("bmjpegenc", "enc");
    break;
  default:
    g_print("no support enc type set \n");
    break;
  }

  sink = gst_element_factory_make("filesink", "sink");

  CHECK ((pipeline && sink &&
          src && encode),
      "create element failed\n");

  elem_group.pipeline = pipeline;
  elem_group.dec = dec;
  elem_group.encode = encode;
  elem_group.sink = sink;

  if (parse && dec)
    gst_bin_add_many (reinterpret_cast < GstBin * >(pipeline),
        src, parse, dec, encode, sink, NULL);
  else
    gst_bin_add_many(reinterpret_cast < GstBin * >(pipeline), src, dec, encode, sink, NULL);

  if (caps)
    g_object_set (G_OBJECT (src), "caps", caps, "uri", vpath.c_str (), NULL);
  else
    g_object_set (G_OBJECT (src), "uri", vpath.c_str (), NULL);

  g_object_set (G_OBJECT (sink), "location", opath.c_str(), NULL);

  if (enc_type == CODEC_H264 || enc_type == CODEC_H265) {
    g_object_get(G_OBJECT(encode), "gop", &gop, "bps", &br, "qp-max", &max_qp, "qp-min", &min_qp, "gop-preset", &gop_preset,"cqp", &cqp, NULL);
    if (cmd_bps >= 0) {
      br = cmd_bps;
    }
    if (cmd_gop >= 0) {
      gop = cmd_gop;
    }
    if (cmd_gop_preset > 0) {
      gop_preset = cmd_gop_preset;
    }
    if (cmd_cqp > 0) {
      cqp = cmd_cqp;
    }

    if (cmd_min_qp >= 0) {
        min_qp = cmd_min_qp;
    }

    if (cmd_max_qp >= 0) {
        max_qp = cmd_max_qp;
    }
    g_object_set(G_OBJECT(encode), "gop", gop, "bps", br, "qp-max", max_qp, "qp-min", min_qp, "gop-preset", gop_preset, "cqp", cqp, NULL);
  } else if (enc_type == CODEC_JPEG) {
    g_print("qfactor set in cmd_qfactor %d \n", cmd_qfactor);
    g_object_get(G_OBJECT(encode), "q-factor", &qfactor,NULL);
    if (cmd_qfactor > 0) {
      qfactor = cmd_qfactor;
    }
    g_object_set(G_OBJECT(encode), "q-factor", qfactor, NULL);
  }

  // common
  if (parse)
    g_signal_connect (src, "pad-added", G_CALLBACK (src_handle_pad_added), parse);
  else
    g_signal_connect (src, "pad-added", G_CALLBACK (src_pad_added), &elem_group);

  if (caps)
    gst_caps_unref (caps);

  auto ret = 0;
  if (parse) {
    ret = gst_element_link_many (parse, dec, encode, sink, NULL);
    if (!ret) {
      GST_ERROR ("link failed\n");
      return -1;
    }
  } else {
    gst_element_link_many (dec, encode, sink, NULL);
  }

  auto loop = g_main_loop_new (NULL, false);
  auto bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  guint bus_watch_id = gst_bus_add_watch (bus, bus_call, loop);
  gst_object_unref (bus);

  GST_INFO ("Pipeline playing...\n");
  gst_element_set_state (pipeline, GST_STATE_PLAYING);
  GST_INFO ("Running...\n");
  g_main_loop_run (loop);
  GST_INFO ("Stop playback\n");
  gst_element_set_state (pipeline, GST_STATE_NULL);

  // release resources
  /* Release the request pads from the Tiler, and unref them */
  gst_object_unref (GST_OBJECT (pipeline));
  g_source_remove (bus_watch_id);
  g_main_loop_unref (loop);
  g_free(video_path);
  g_free(output_path);
  g_option_context_free(ctx);
  return 0;
}
