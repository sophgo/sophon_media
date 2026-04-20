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
#include "bmcv_api_ext.h"

#define MAX_NAME_LENGTH 20

typedef enum {
  CODEC_NONE,
  CODEC_H264,
  CODEC_H265,
  CODEC_JPEG,
  CODEC_BIN, /*for input format no specified user decodebin pad */
  CODEC_BUTT,
}CODEC_TYPE;

typedef struct {
    FILE *fp;
    int frame_count;
} CaptureContext;

typedef struct {
    const char *name;
    GstClockTime start_time;
    GstClockTime last_time;
    guint frame_count;
} FpsContext;

GstElement *pipeline = NULL, *src = NULL, *parse = NULL, *dec = NULL, *sink = NULL;

typedef struct _ElementGroup {
    GstElement *pipeline;
    GstElement *dec;
    GstElement *vpss;
    GstElement *encode;
    GstElement *sink;
}ElementGroup;

#define CHECK(ret, output)           \
  if (!ret) {                        \
    GST_ERROR("ERROR: %s", output);  \
    return -1;                       \
  }

gboolean
bus_call (GstBus G_GNUC_UNUSED *bus, GstMessage * msg, gpointer data)
{
  gchar *debug;
  GError *error;
  GMainLoop *loop = reinterpret_cast < GMainLoop * >(data);
  switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_EOS:
      GST_INFO ("End of stream\n");
      g_main_loop_quit (loop);
      break;
    case GST_MESSAGE_WARNING:
      gst_message_parse_warning(msg, &error, &debug);
      // handle warning message from uridecodebin
      if (g_str_equal(GST_OBJECT_NAME (msg->src), "src")) {
        GST_ERROR ("WARNING from element %s: %s\n",
          GST_OBJECT_NAME (msg->src), error->message);
      }
      g_error_free (error);
      // g_main_loop_quit (loop);
      break;
    case GST_MESSAGE_ERROR:
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

static GstPadProbeReturn
capture_probe(GstPad *pad, GstPadProbeInfo *info, gpointer user_data) {
  CaptureContext *ctx = (CaptureContext *)user_data;
  GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);
  if (!buffer) return GST_PAD_PROBE_OK;

  buffer = gst_buffer_ref(buffer);

  GstCaps *caps = gst_pad_get_current_caps(pad);
  GstStructure *str = gst_caps_get_structure(caps, 0);
  const gchar *format = gst_structure_get_string(str, "format");
  gint width = 0, height = 0;
  gst_structure_get_int(str, "width", &width);
  gst_structure_get_int(str, "height", &height);

  //g_print("[Frame %d] Format: %s, Resolution: %dx%d\n",
  //      ctx->frame_count, format ? format : "unknown", width, height);

  GstMapInfo map;
  if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
    fwrite(map.data, 1, map.size, ctx->fp);
    gst_buffer_unmap(buffer, &map);
  }

  gst_buffer_unref(buffer);
  gst_caps_unref(caps);

  ctx->frame_count++;
  return GST_PAD_PROBE_OK;
}

static GstPadProbeReturn fps_probe(GstPad *pad, GstPadProbeInfo *info, gpointer user_data) {
  FpsContext *ctx = (FpsContext *)user_data;
  GstBuffer *buf = GST_PAD_PROBE_INFO_BUFFER(info);
  if (!buf) return GST_PAD_PROBE_OK;

  GstClockTime now = gst_util_get_timestamp();

  if (ctx->frame_count == 0) {
      ctx->start_time = now;
      ctx->last_time = now;
  }

  ctx->frame_count++;

  if ((now - ctx->last_time) >= GST_SECOND) {
    gdouble elapsed = (now - ctx->start_time) / (gdouble)GST_SECOND;
    gdouble fps = ctx->frame_count / elapsed;
    const char* env = getenv("DISPLAY_FRAMERATE");
    if (env && strcmp(env, "1") == 0) {
      g_print("[%s] FPS: %.2f\n", ctx->name, fps);
    }
    ctx->last_time = now;
  }

  return GST_PAD_PROBE_OK;
}

FpsContext* attach_fps_probe(GstElement *element, const char *pad_name, const char *stage_name) {
  GstPad *pad = gst_element_get_static_pad(element, pad_name);
  if (!pad) {
      g_printerr("Failed to get pad %s of element %s\n", pad_name, GST_ELEMENT_NAME(element));
      return NULL;
  }

  FpsContext *ctx = g_new0(FpsContext, 1);
  ctx->name = stage_name;

  gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, fps_probe, ctx, NULL);
  gst_object_unref(pad);

  return ctx;
}

void print_total_fps(FpsContext *ctx) {
  if (!ctx || ctx->frame_count == 0) return;

  GstClockTime now = gst_util_get_timestamp();
  gdouble elapsed = (now - ctx->start_time) / (gdouble)GST_SECOND;

  gdouble avg_fps = ctx->frame_count / elapsed;
  g_print("[%s] Total Frames: %u, Time Elapsed: %.2f sec, Average FPS: %.2f\n",
            ctx->name, ctx->frame_count, elapsed, avg_fps);
}

int main (int argc, char **argv) {
  gchar *video_path = NULL, *output_path = NULL;
  GOptionContext *ctx;
  char *absolute_path;
  GError *err = NULL;

  gint  dec_type = 0, enc_type = 0;
  gint vpss_left = 0, vpss_right = 0, vpss_top = 0, vpss_bottom = 0;
  gint out_width = 0, out_height = 0;
  gchar *out_format = NULL;
  gchar *save_decoded = NULL;
  gchar *save_vpss = NULL;

  GOptionEntry entries[] = {
    { "video_path", 'v', 0, G_OPTION_ARG_STRING, &video_path,
      "The path of the video file.", "FILE" },
    { "output_path", 'o', 0, G_OPTION_ARG_STRING, &output_path,
      "The path to save output transcode video file", "FILE" },
    { "dec_type", 'd', 0, G_OPTION_ARG_INT, &dec_type,
      "h26x input codec format type 1:h264 2:h265 3:JPEG 4: decode bin chose by gst", "type" },
    { "enc_type", 'e', 0, G_OPTION_ARG_INT, &enc_type,
      "h26x the output codec format type 1:h264 2:h265 3:JPEG", "type" },

    { "vpss_left", 'l', 0, G_OPTION_ARG_INT, &vpss_left, "VPSS crop left", "int" },
    { "vpss_right", 'r', 0, G_OPTION_ARG_INT, &vpss_right, "VPSS crop right", "int" },
    { "vpss_top", 't', 0, G_OPTION_ARG_INT, &vpss_top, "VPSS crop top", "int" },
    { "vpss_bottom", 'b', 0, G_OPTION_ARG_INT, &vpss_bottom, "VPSS crop bottom", "int" },
    { "out_width", 'w', 0, G_OPTION_ARG_INT, &out_width, "Output width", "int" },
    { "out_height", 'h', 0, G_OPTION_ARG_INT, &out_height, "Output height", "int" },
    { "out_format", 'f', 0, G_OPTION_ARG_STRING, &out_format, "Output format (e.g., I420, NV12, NV16, RGB)", "STRING" },
    { "save_decoded", 's', 0, G_OPTION_ARG_STRING, &save_decoded, "Save decoded raw video (YUV)", "FILE"},
    { "save_vpss", 'S', 0, G_OPTION_ARG_STRING, &save_vpss, "Save VPSS output raw video (YUV)", "FILE" },
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
  absolute_path = realpath(video_path, NULL);
  vpath += absolute_path;
  std::string opath = output_path;
  if (absolute_path) {
    free(absolute_path);
  }

  g_print("create in \n");
  GstElement *pipeline = NULL, *src = NULL, *parse = NULL, *dec = NULL, *vpss = NULL, *encode = NULL, *sink = NULL;
  GstCaps *caps = NULL, *out_caps = NULL;
  GstElement *capsfilter = NULL;
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
    caps = gst_caps_from_string("video/x-h264; "
                                "video/x-h265; "
                                "image/jpeg;");
    break;
  default:
      g_print("no support in dec codec type \n");
    break;
  }

  dec = gst_element_factory_make ("bmdec", "dec");
  vpss = gst_element_factory_make("bmvpss", "vpss");

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

  CHECK ((pipeline && src && dec && vpss && encode && sink),
      "create element failed\n");

  elem_group.pipeline = pipeline;
  elem_group.dec = dec;
  elem_group.vpss = vpss;
  elem_group.encode = encode;
  elem_group.sink = sink;

  if (parse && dec)
    gst_bin_add_many (reinterpret_cast < GstBin * >(pipeline),
        src, parse, dec, vpss, encode, sink, NULL);
  else
    gst_bin_add_many(reinterpret_cast < GstBin * >(pipeline), src, dec, vpss, encode, sink, NULL);

  if (caps)
    g_object_set (G_OBJECT (src), "caps", caps, "uri", vpath.c_str (), NULL);
  else
    g_object_set (G_OBJECT (src), "uri", vpath.c_str (), NULL);

  g_object_set (G_OBJECT (sink), "location", opath.c_str(), NULL);

  g_object_set(G_OBJECT(vpss),
    "left", vpss_left,
    "right", vpss_right,
    "top", vpss_top,
    "bottom", vpss_bottom,
    NULL);

  if (out_width <= 0 || out_height <= 0 || !out_format) {
     g_printerr("Parameters error, should set out_width, out_height and out_format\n");
  } else {
    out_caps = gst_caps_new_simple("video/x-raw",
        "width", G_TYPE_INT, out_width,
        "height", G_TYPE_INT, out_height,
        "format", G_TYPE_STRING, out_format,
        NULL);
    capsfilter = gst_element_factory_make("capsfilter", "filter");
    g_object_set(capsfilter, "caps", out_caps, NULL);
    gst_caps_unref(out_caps);
    gst_bin_add(GST_BIN(pipeline), capsfilter);
  }

  if (parse)
    g_signal_connect (src, "pad-added", G_CALLBACK (src_handle_pad_added), parse);
  else
    g_signal_connect (src, "pad-added", G_CALLBACK (src_pad_added), &elem_group);

  if (caps)
    gst_caps_unref (caps);

  gboolean ret = FALSE;
    if (capsfilter) {
        if (parse) ret = gst_element_link_many(parse, dec, vpss, capsfilter, encode, sink, NULL);
        else ret = gst_element_link_many(dec, vpss, capsfilter, encode, sink, NULL);
    } else {
        if (parse) ret = gst_element_link_many(parse, dec, vpss, encode, sink, NULL);
        else ret = gst_element_link_many(dec, vpss, encode, sink, NULL);
    }
    if (!ret) {
        g_printerr("Link failed\n");
        return -1;
    }

  FpsContext *ctx_decoder = attach_fps_probe(dec, "src", "DECODER");
  FpsContext *ctx_vpss = attach_fps_probe(vpss, "src", "VPSS");
  FpsContext *ctx_encoder = attach_fps_probe(encode, "src", "ENCODER");

  CaptureContext *capture_ctx = NULL;
  if (save_decoded) {
      capture_ctx = g_new0(CaptureContext, 1);
      capture_ctx->fp = fopen(save_decoded, "wb");
      if (!capture_ctx->fp) {
          g_printerr("Failed to open %s for writing\n", save_decoded);
          return -1;
      }
      GstPad *dec_src_pad = gst_element_get_static_pad(dec, "src");
      if (dec_src_pad) {
          gst_pad_add_probe(dec_src_pad, GST_PAD_PROBE_TYPE_BUFFER, capture_probe, capture_ctx, NULL);
          gst_object_unref(dec_src_pad);
      }
  }

  CaptureContext *capture_vpss_ctx = NULL;
  if (save_vpss) {
      capture_vpss_ctx = g_new0(CaptureContext, 1);
      capture_vpss_ctx->fp = fopen(save_vpss, "wb");
      if (!capture_vpss_ctx->fp) {
        g_printerr("Failed to open %s\n", save_vpss);
        return -1;
      }
      GstPad *vpss_src_pad = gst_element_get_static_pad(vpss, "src");
      if (vpss_src_pad) {
          gst_pad_add_probe(vpss_src_pad, GST_PAD_PROBE_TYPE_BUFFER, capture_probe, capture_vpss_ctx, NULL);
          gst_object_unref(vpss_src_pad);
      }
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

  print_total_fps(ctx_decoder);
  print_total_fps(ctx_vpss);
  print_total_fps(ctx_encoder);

  if (capture_ctx) {
      fclose(capture_ctx->fp);
      g_print("Saved %d frames to %s\n", capture_ctx->frame_count, save_decoded);
      g_free(capture_ctx);
  }

  if (capture_vpss_ctx) {
      fclose(capture_vpss_ctx->fp);
      g_print("Saved %d frames to %s\n", capture_vpss_ctx->frame_count, save_vpss);
      g_free(capture_vpss_ctx);
  }

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

