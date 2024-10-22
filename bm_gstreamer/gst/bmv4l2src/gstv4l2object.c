/* GStreamer
 *
 * Copyright (C) 2001-2002 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *               2006 Edgard Lima <edgard.lima@gmail.com>
 *
 * gstv4l2object.c: base class for V4L2 elements
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version. This library is distributed in the hope
 * that it will be useful, but WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Library General Public License for more details.
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/ioctl.h>


#ifdef HAVE_GUDEV
#include <gudev/gudev.h>
#endif

#include "ext/videodev2.h"
#include "gstv4l2object.h"

#include <gst/video/video.h>
#include <gst/allocators/gstdmabuf.h>

GST_DEBUG_CATEGORY_EXTERN (bm_v4l2_debug);
#define GST_CAT_DEFAULT bm_v4l2_debug

#define DEFAULT_PROP_DEVICE_NAME        NULL
#define DEFAULT_PROP_DEVICE_FD          -1
#define DEFAULT_PROP_FLAGS              0
#define DEFAULT_PROP_TV_NORM            0
#define DEFAULT_PROP_IO_MODE            GST_V4L2_IO_MMAP

#define ENCODED_BUFFER_MIN_SIZE         (256 * 1024)
#define GST_V4L2_DEFAULT_WIDTH          320
#define GST_V4L2_DEFAULT_HEIGHT         240

enum
{
  PROP_0,
  V4L2_STD_OBJECT_PROPS,
};

/*
 * common format / caps utilities:
 */
typedef enum
{
  GST_V4L2_RAW = 1 << 0,
  GST_V4L2_CODEC = 1 << 1,
  GST_V4L2_TRANSPORT = 1 << 2,
  GST_V4L2_NO_PARSE = 1 << 3,
  GST_V4L2_ALL = 0xffff
} GstV4L2FormatFlags;

typedef struct
{
  guint32 format;
  gboolean dimensions;
  GstV4L2FormatFlags flags;
} GstV4L2FormatDesc;

static const GstV4L2FormatDesc gst_v4l2_formats[] = {
  /* RGB formats */
  {V4L2_PIX_FMT_RGB332, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_ARGB555, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_XRGB555, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_ARGB555X, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_XRGB555X, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_RGB565, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_RGB565X, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_BGR666, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_BGR24, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_RGB24, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_ABGR32, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_XBGR32, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_BGRA32, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_BGRX32, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_RGBA32, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_RGBX32, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_ARGB32, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_XRGB32, TRUE, GST_V4L2_RAW},

  /* Deprecated Packed RGB Image Formats (alpha ambiguity) */
  {V4L2_PIX_FMT_RGB444, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_RGB555, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_RGB555X, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_BGR32, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_RGB32, TRUE, GST_V4L2_RAW},

  /* Grey formats */
  {V4L2_PIX_FMT_GREY, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_Y4, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_Y6, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_Y10, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_Y12, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_Y16, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_Y16_BE, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_Y10BPACK, TRUE, GST_V4L2_RAW},

  /* Palette formats */
  {V4L2_PIX_FMT_PAL8, TRUE, GST_V4L2_RAW},

  /* Chrominance formats */
  {V4L2_PIX_FMT_UV8, TRUE, GST_V4L2_RAW},

  /* Luminance+Chrominance formats */
  {V4L2_PIX_FMT_YVU410, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_YVU420, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_YVU420M, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_YUYV, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_YYUV, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_YVYU, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_UYVY, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_VYUY, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_YUV422P, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_YUV422M, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_YUV411P, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_Y41P, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_YUV444, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_YUV555, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_YUV565, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_YUV32, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_YUV410, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_YUV420, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_YUV420M, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_HI240, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_HM12, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_M420, TRUE, GST_V4L2_RAW},

  /* two planes -- one Y, one Cr + Cb interleaved  */
  {V4L2_PIX_FMT_NV12, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_NV12M, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_NV12MT, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_NV12MT_16X16, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_NV12M_8L128, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_NV12M_10BE_8L128, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_NV21, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_NV21M, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_NV16, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_NV16M, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_NV61, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_NV61M, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_NV24, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_NV42, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_MM21, TRUE, GST_V4L2_RAW},

  /* Bayer formats - see http://www.siliconimaging.com/RGB%20Bayer.htm */
  {V4L2_PIX_FMT_SBGGR8, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_SGBRG8, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_SGRBG8, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_SRGGB8, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_SBGGR10, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_SGBRG10, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_SGRBG10, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_SRGGB10, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_SBGGR12, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_SGBRG12, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_SGRBG12, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_SRGGB12, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_SBGGR14, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_SGBRG14, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_SGRBG14, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_SRGGB14, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_SBGGR16, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_SGBRG16, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_SGRBG16, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_SRGGB16, TRUE, GST_V4L2_RAW},

  /* compressed formats */
  {V4L2_PIX_FMT_MJPEG, FALSE, GST_V4L2_CODEC},
  {V4L2_PIX_FMT_JPEG, FALSE, GST_V4L2_CODEC},
  {V4L2_PIX_FMT_PJPG, FALSE, GST_V4L2_CODEC},
  {V4L2_PIX_FMT_DV, FALSE, GST_V4L2_TRANSPORT},
  {V4L2_PIX_FMT_MPEG, FALSE, GST_V4L2_TRANSPORT},
  {V4L2_PIX_FMT_FWHT, FALSE, GST_V4L2_CODEC},
  {V4L2_PIX_FMT_H264, FALSE, GST_V4L2_CODEC},
  {V4L2_PIX_FMT_H264_NO_SC, FALSE, GST_V4L2_CODEC},
  {V4L2_PIX_FMT_H264_MVC, FALSE, GST_V4L2_CODEC},
  {V4L2_PIX_FMT_HEVC, FALSE, GST_V4L2_CODEC},
  {V4L2_PIX_FMT_H263, FALSE, GST_V4L2_CODEC},
  {V4L2_PIX_FMT_MPEG1, FALSE, GST_V4L2_CODEC},
  {V4L2_PIX_FMT_MPEG2, FALSE, GST_V4L2_CODEC},
  {V4L2_PIX_FMT_MPEG4, FALSE, GST_V4L2_CODEC},
  {V4L2_PIX_FMT_XVID, FALSE, GST_V4L2_CODEC},
  {V4L2_PIX_FMT_VC1_ANNEX_G, FALSE, GST_V4L2_CODEC},
  {V4L2_PIX_FMT_VC1_ANNEX_L, FALSE, GST_V4L2_CODEC},
  {V4L2_PIX_FMT_VP8, FALSE, GST_V4L2_CODEC | GST_V4L2_NO_PARSE},
  {V4L2_PIX_FMT_VP9, FALSE, GST_V4L2_CODEC | GST_V4L2_NO_PARSE},

  /*  Vendor-specific formats   */
  {V4L2_PIX_FMT_WNVA, TRUE, GST_V4L2_CODEC},
  {V4L2_PIX_FMT_SN9C10X, TRUE, GST_V4L2_CODEC},
  {V4L2_PIX_FMT_PWC1, TRUE, GST_V4L2_CODEC},
  {V4L2_PIX_FMT_PWC2, TRUE, GST_V4L2_CODEC},
};

#define GST_V4L2_FORMAT_COUNT (G_N_ELEMENTS (gst_v4l2_formats))

static GSList *gst_v4l2_object_get_format_list (GstV4l2Object * v4l2object);


#define GST_TYPE_V4L2_DEVICE_FLAGS (gst_v4l2_device_get_type ())
static GType
gst_v4l2_device_get_type (void)
{
  static GType v4l2_device_type = 0;

  if (v4l2_device_type == 0) {
    static const GFlagsValue values[] = {
      {V4L2_CAP_VIDEO_CAPTURE, "Device supports video capture", "capture"},
      {V4L2_CAP_VIDEO_OUTPUT, "Device supports video playback", "output"},
      {0, NULL, NULL}
    };

    v4l2_device_type =
        g_flags_register_static ("GstV4l2DeviceTypeFlags", values);
  }

  return v4l2_device_type;
}

GType
gst_v4l2_io_mode_get_type (void)
{
  static GType v4l2_io_mode = 0;

  if (!v4l2_io_mode) {
    static const GEnumValue io_modes[] = {
      {GST_V4L2_IO_AUTO, "GST_V4L2_IO_AUTO", "auto"},
      {GST_V4L2_IO_RW, "GST_V4L2_IO_RW", "rw"},
      {GST_V4L2_IO_MMAP, "GST_V4L2_IO_MMAP", "mmap"},
      {GST_V4L2_IO_USERPTR, "GST_V4L2_IO_USERPTR", "userptr"},
      {GST_V4L2_IO_DMABUF, "GST_V4L2_IO_DMABUF", "dmabuf"},
      {GST_V4L2_IO_DMABUF_IMPORT, "GST_V4L2_IO_DMABUF_IMPORT",
          "dmabuf-import"},

      {0, NULL, NULL}
    };
    v4l2_io_mode = g_enum_register_static ("GstV4l2IOMode", io_modes);
  }
  return v4l2_io_mode;
}

void
gst_v4l2_object_install_properties_helper (GObjectClass * gobject_class,
    const char *default_device)
{
  g_object_class_install_property (gobject_class, PROP_DEVICE,
      g_param_spec_string ("device", "Device", "Device location",
          default_device, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_DEVICE_NAME,
      g_param_spec_string ("device-name", "Device name",
          "Name of the device", DEFAULT_PROP_DEVICE_NAME,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_DEVICE_FD,
      g_param_spec_int ("device-fd", "File descriptor",
          "File descriptor of the device", -1, G_MAXINT, DEFAULT_PROP_DEVICE_FD,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_FLAGS,
      g_param_spec_flags ("flags", "Flags", "Device type flags",
          GST_TYPE_V4L2_DEVICE_FLAGS, DEFAULT_PROP_FLAGS,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  // gst_type_mark_as_plugin_api (GST_TYPE_V4L2_DEVICE_FLAGS, 0);
  // gst_type_mark_as_plugin_api (GST_TYPE_V4L2_TV_NORM, 0);
  // gst_type_mark_as_plugin_api (GST_TYPE_V4L2_IO_MODE, 0);
}

void
gst_v4l2_object_install_m2m_properties_helper (GObjectClass * gobject_class)
{
  g_object_class_install_property (gobject_class, PROP_DEVICE,
      g_param_spec_string ("device", "Device", "Device location",
          NULL, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DEVICE_NAME,
      g_param_spec_string ("device-name", "Device name",
          "Name of the device", DEFAULT_PROP_DEVICE_NAME,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DEVICE_FD,
      g_param_spec_int ("device-fd", "File descriptor",
          "File descriptor of the device", -1, G_MAXINT, DEFAULT_PROP_DEVICE_FD,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_OUTPUT_IO_MODE,
      g_param_spec_enum ("output-io-mode", "Output IO mode",
          "Output side I/O mode (matches sink pad)",
          GST_TYPE_V4L2_IO_MODE, DEFAULT_PROP_IO_MODE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_CAPTURE_IO_MODE,
      g_param_spec_enum ("capture-io-mode", "Capture IO mode",
          "Capture I/O mode (matches src pad)",
          GST_TYPE_V4L2_IO_MODE, DEFAULT_PROP_IO_MODE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_EXTRA_CONTROLS,
      g_param_spec_boxed ("extra-controls", "Extra Controls",
          "Extra v4l2 controls (CIDs) for the device",
          GST_TYPE_STRUCTURE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

/* Support for 32bit off_t, this wrapper is casting off_t to gint64 */
#ifdef HAVE_LIBV4L2
#if SIZEOF_OFF_T < 8

static gpointer
v4l2_mmap_wrapper (gpointer start, gsize length, gint prot, gint flags, gint fd,
    off_t offset)
{
  return v4l2_mmap (start, length, prot, flags, fd, (gint64) offset);
}

#define v4l2_mmap v4l2_mmap_wrapper

#endif /* SIZEOF_OFF_T < 8 */
#endif /* HAVE_LIBV4L2 */

GstV4l2Object *
gst_v4l2_object_new (GstElement * element,
    GstObject * debug_object,
    enum v4l2_buf_type type,
    const char *default_device,
    GstV4l2GetInOutFunction get_in_out_func,
    GstV4l2SetInOutFunction set_in_out_func,
    GstV4l2UpdateFpsFunction update_fps_func)
{
  GstV4l2Object *v4l2object;

  /*
   * some default values
   */
  v4l2object = g_new0 (GstV4l2Object, 1);

  v4l2object->type = type;
  v4l2object->formats = NULL;
  v4l2object->mode = GST_V4L2_IO_MMAP;

  v4l2object->element = element;
  v4l2object->dbg_obj = debug_object;
  v4l2object->get_in_out_func = get_in_out_func;
  v4l2object->set_in_out_func = set_in_out_func;
  v4l2object->update_fps_func = update_fps_func;

  v4l2object->video_fd = -1;
  v4l2object->active = FALSE;
  v4l2object->videodev = g_strdup (default_device);

  v4l2object->norms = NULL;
  v4l2object->channels = NULL;
  v4l2object->colors = NULL;

  v4l2object->keep_aspect = TRUE;

  v4l2object->n_v4l2_planes = 0;

  v4l2object->no_initial_format = FALSE;

  v4l2object->poll = gst_poll_new (TRUE);
  v4l2object->can_poll_device = TRUE;

  /* We now disable libv4l2 by default, but have an env to enable it. */
#ifdef HAVE_LIBV4L2
  if (g_getenv ("GST_V4L2_USE_LIBV4L2")) {
    v4l2object->fd_open = v4l2_fd_open;
    v4l2object->close = v4l2_close;
    v4l2object->dup = v4l2_dup;
    v4l2object->ioctl = v4l2_ioctl;
    v4l2object->read = v4l2_read;
    v4l2object->mmap = v4l2_mmap;
    v4l2object->munmap = v4l2_munmap;
  } else
#endif
  {
    v4l2object->fd_open = NULL;
    v4l2object->close = close;
    v4l2object->dup = dup;
    v4l2object->ioctl = ioctl;
    v4l2object->read = read;
    v4l2object->mmap = mmap;
    v4l2object->munmap = munmap;
  }

  return v4l2object;
}


static gboolean
gst_v4l2_object_clear_format_list (GstV4l2Object * v4l2object)
{
  g_slist_foreach (v4l2object->formats, (GFunc) g_free, NULL);
  g_slist_free (v4l2object->formats);
  v4l2object->formats = NULL;
  v4l2object->fmtdesc = NULL;

  return TRUE;
}


void
gst_v4l2_object_destroy (GstV4l2Object * v4l2object)
{
  g_return_if_fail (v4l2object != NULL);

  g_free (v4l2object->videodev);
  g_free (v4l2object->par);
  g_free (v4l2object->channel);

  gst_poll_free (v4l2object->poll);

  if (v4l2object->formats) {
    gst_v4l2_object_clear_format_list (v4l2object);
  }

  if (v4l2object->probed_caps) {
    gst_caps_unref (v4l2object->probed_caps);
  }

  if (v4l2object->extra_controls) {
    gst_structure_free (v4l2object->extra_controls);
  }

  g_free (v4l2object);
}

gboolean
gst_v4l2_object_set_property_helper (GstV4l2Object * v4l2object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  switch (prop_id) {
    case PROP_DEVICE:
      g_free (v4l2object->videodev);
      v4l2object->videodev = g_value_dup_string (value);
      break;
    case PROP_IO_MODE:
      v4l2object->req_mode = g_value_get_enum (value);
      break;
    case PROP_CAPTURE_IO_MODE:
      g_return_val_if_fail (!V4L2_TYPE_IS_OUTPUT (v4l2object->type), FALSE);
      v4l2object->req_mode = g_value_get_enum (value);
      break;
    default:
      return FALSE;
      break;
  }
  return TRUE;
}


gboolean
gst_v4l2_object_get_property_helper (GstV4l2Object * v4l2object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  switch (prop_id) {
    case PROP_DEVICE:
      g_value_set_string (value, v4l2object->videodev);
      break;
    case PROP_DEVICE_NAME:
    {
      const guchar *name = NULL;

      if (GST_V4L2_IS_OPEN (v4l2object))
        name = v4l2object->vcap.card;

      g_value_set_string (value, (gchar *) name);
      break;
    }
    case PROP_DEVICE_FD:
    {
      if (GST_V4L2_IS_OPEN (v4l2object))
        g_value_set_int (value, v4l2object->video_fd);
      else
        g_value_set_int (value, DEFAULT_PROP_DEVICE_FD);
      break;
    }
    case PROP_FLAGS:
    {
      guint flags = 0;

      if (GST_V4L2_IS_OPEN (v4l2object)) {
        flags |= v4l2object->device_caps &
            (V4L2_CAP_VIDEO_CAPTURE |
            V4L2_CAP_VIDEO_OUTPUT);

        if (v4l2object->device_caps & V4L2_CAP_VIDEO_CAPTURE_MPLANE)
          flags |= V4L2_CAP_VIDEO_CAPTURE;

        if (v4l2object->device_caps & V4L2_CAP_VIDEO_OUTPUT_MPLANE)
          flags |= V4L2_CAP_VIDEO_OUTPUT;
      }
      g_value_set_flags (value, flags);
      break;
    }

    default:
      return FALSE;
      break;
  }
  return TRUE;
}

static void
gst_v4l2_get_driver_min_buffers (GstV4l2Object * v4l2object)
{
  struct v4l2_control control = { 0, };

  g_return_if_fail (GST_V4L2_IS_OPEN (v4l2object));

  if (V4L2_TYPE_IS_OUTPUT (v4l2object->type))
    control.id = V4L2_CID_MIN_BUFFERS_FOR_OUTPUT;
  else
    control.id = V4L2_CID_MIN_BUFFERS_FOR_CAPTURE;

  if (v4l2object->ioctl (v4l2object->video_fd, VIDIOC_G_CTRL, &control) == 0) {
    GST_DEBUG_OBJECT (v4l2object->dbg_obj,
        "driver requires a minimum of %d buffers", control.value);
    v4l2object->min_buffers = control.value;
  } else {
    v4l2object->min_buffers = 0;
  }
}

static void
gst_v4l2_set_defaults (GstV4l2Object * v4l2object)
{
 
}

static void
gst_v4l2_object_init_poll (GstV4l2Object * v4l2object)
{
  gst_poll_fd_init (&v4l2object->pollfd);
  v4l2object->pollfd.fd = v4l2object->video_fd;
  gst_poll_add_fd (v4l2object->poll, &v4l2object->pollfd);
  if (V4L2_TYPE_IS_OUTPUT (v4l2object->type))
    gst_poll_fd_ctl_write (v4l2object->poll, &v4l2object->pollfd, TRUE);
  else
    gst_poll_fd_ctl_read (v4l2object->poll, &v4l2object->pollfd, TRUE);

  v4l2object->can_poll_device = TRUE;
}

gboolean
gst_v4l2_object_open (GstV4l2Object * v4l2object, GstV4l2Error * error)
{
  if (gst_v4l2_open (v4l2object, error))
    gst_v4l2_set_defaults (v4l2object);
  else
    return FALSE;

  gst_v4l2_object_init_poll (v4l2object);

  return TRUE;
}

gboolean
gst_v4l2_object_open_shared (GstV4l2Object * v4l2object, GstV4l2Object * other)
{
  if (gst_v4l2_dup (v4l2object, other)) {
    gst_v4l2_object_init_poll (v4l2object);
    return TRUE;
  }

  return FALSE;
}

gboolean
gst_v4l2_object_close (GstV4l2Object * v4l2object)
{
  if (!gst_v4l2_close (v4l2object))
    return FALSE;

  gst_caps_replace (&v4l2object->probed_caps, NULL);

  /* reset our copy of the device caps */
  v4l2object->device_caps = 0;

  if (v4l2object->formats) {
    gst_v4l2_object_clear_format_list (v4l2object);
  }

  if (v4l2object->par) {
    g_value_unset (v4l2object->par);
    g_free (v4l2object->par);
    v4l2object->par = NULL;
  }

  if (v4l2object->channel) {
    g_free (v4l2object->channel);
    v4l2object->channel = NULL;
  }

  /* remove old fd from poll */
  if (v4l2object->poll)
    gst_poll_remove_fd (v4l2object->poll, &v4l2object->pollfd);

  return TRUE;
}

static struct v4l2_fmtdesc *
gst_v4l2_object_get_format_from_fourcc (GstV4l2Object * v4l2object,
    guint32 fourcc)
{
  struct v4l2_fmtdesc *fmt;
  GSList *walk;

  if (fourcc == 0)
    return NULL;

  walk = gst_v4l2_object_get_format_list (v4l2object);
  while (walk) {
    fmt = (struct v4l2_fmtdesc *) walk->data;
    if (fmt->pixelformat == fourcc)
      return fmt;
    /* special case for jpeg */
    if (fmt->pixelformat == V4L2_PIX_FMT_MJPEG ||
        fmt->pixelformat == V4L2_PIX_FMT_JPEG ||
        fmt->pixelformat == V4L2_PIX_FMT_PJPG) {
      if (fourcc == V4L2_PIX_FMT_JPEG || fourcc == V4L2_PIX_FMT_MJPEG ||
          fourcc == V4L2_PIX_FMT_PJPG) {
        return fmt;
      }
    }
    walk = g_slist_next (walk);
  }

  return NULL;
}



/* complete made up ranking, the values themselves are meaningless */
/* These ranks MUST be X such that X<<15 fits on a signed int - see
   the comment at the end of gst_v4l2_object_format_get_rank. */
#define YUV_BASE_RANK     1000
#define JPEG_BASE_RANK     500
#define DV_BASE_RANK       200
#define RGB_BASE_RANK      100
#define YUV_ODD_BASE_RANK   50
#define RGB_ODD_BASE_RANK   25
#define BAYER_BASE_RANK     15
#define S910_BASE_RANK      10
#define GREY_BASE_RANK       5
#define PWC_BASE_RANK        1

static gint
gst_v4l2_object_format_get_rank (const struct v4l2_fmtdesc *fmt)
{
  guint32 fourcc = fmt->pixelformat;
  gboolean emulated = ((fmt->flags & V4L2_FMT_FLAG_EMULATED) != 0);
  gint rank = 0;

  switch (fourcc) {
    case V4L2_PIX_FMT_MJPEG:
    case V4L2_PIX_FMT_PJPG:
      rank = JPEG_BASE_RANK;
      break;
    case V4L2_PIX_FMT_JPEG:
      rank = JPEG_BASE_RANK + 1;
      break;
    case V4L2_PIX_FMT_MPEG:    /* MPEG          */
      rank = JPEG_BASE_RANK + 2;
      break;

    case V4L2_PIX_FMT_RGB332:
    case V4L2_PIX_FMT_ARGB555:
    case V4L2_PIX_FMT_XRGB555:
    case V4L2_PIX_FMT_RGB555:
    case V4L2_PIX_FMT_ARGB555X:
    case V4L2_PIX_FMT_XRGB555X:
    case V4L2_PIX_FMT_RGB555X:
    case V4L2_PIX_FMT_BGR666:
    case V4L2_PIX_FMT_RGB565:
    case V4L2_PIX_FMT_RGB565X:
    case V4L2_PIX_FMT_RGB444:
    case V4L2_PIX_FMT_Y4:
    case V4L2_PIX_FMT_Y6:
    case V4L2_PIX_FMT_Y10:
    case V4L2_PIX_FMT_Y12:
    case V4L2_PIX_FMT_Y10BPACK:
    case V4L2_PIX_FMT_YUV555:
    case V4L2_PIX_FMT_YUV565:
    case V4L2_PIX_FMT_YUV32:
    case V4L2_PIX_FMT_NV12MT_16X16:
    case V4L2_PIX_FMT_NV42:
    case V4L2_PIX_FMT_H264_MVC:
      rank = RGB_ODD_BASE_RANK;
      break;

    case V4L2_PIX_FMT_RGB24:
    case V4L2_PIX_FMT_BGR24:
      rank = RGB_BASE_RANK - 1;
      break;

    case V4L2_PIX_FMT_RGB32:
    case V4L2_PIX_FMT_BGR32:
    case V4L2_PIX_FMT_ABGR32:
    case V4L2_PIX_FMT_XBGR32:
    case V4L2_PIX_FMT_BGRA32:
    case V4L2_PIX_FMT_BGRX32:
    case V4L2_PIX_FMT_RGBA32:
    case V4L2_PIX_FMT_RGBX32:
    case V4L2_PIX_FMT_ARGB32:
    case V4L2_PIX_FMT_XRGB32:
      rank = RGB_BASE_RANK;
      break;

    case V4L2_PIX_FMT_GREY:    /*  8  Greyscale     */
      rank = GREY_BASE_RANK;
      break;

    case V4L2_PIX_FMT_NV12MT:  /* NV12 64x32 tile   */
    case V4L2_PIX_FMT_NV21:    /* 12  Y/CrCb 4:2:0  */
    case V4L2_PIX_FMT_NV21M:   /* Same as NV21      */
    case V4L2_PIX_FMT_YYUV:    /* 16  YUV 4:2:2     */
    case V4L2_PIX_FMT_HI240:   /*  8  8-bit color   */
    case V4L2_PIX_FMT_NV16:    /* 16  Y/CbCr 4:2:2  */
    case V4L2_PIX_FMT_NV16M:   /* Same as NV16      */
    case V4L2_PIX_FMT_NV61:    /* 16  Y/CrCb 4:2:2  */
    case V4L2_PIX_FMT_NV61M:   /* Same as NV61      */
    case V4L2_PIX_FMT_NV24:    /* 24  Y/CrCb 4:4:4  */
    case V4L2_PIX_FMT_MM21:    /* NV12 Y 16x32, UV 16x16 tile */
    case V4L2_PIX_FMT_NV12M_8L128:
    case V4L2_PIX_FMT_NV12M_10BE_8L128:
      rank = YUV_ODD_BASE_RANK;
      break;

    case V4L2_PIX_FMT_YVU410:  /* YVU9,  9 bits per pixel */
      rank = YUV_BASE_RANK + 3;
      break;
    case V4L2_PIX_FMT_YUV410:  /* YUV9,  9 bits per pixel */
      rank = YUV_BASE_RANK + 2;
      break;
    case V4L2_PIX_FMT_YUV420:  /* I420, 12 bits per pixel */
    case V4L2_PIX_FMT_YUV420M:
      rank = YUV_BASE_RANK + 7;
      break;
    case V4L2_PIX_FMT_NV12:    /* Y/CbCr 4:2:0, 12 bits per pixel */
    case V4L2_PIX_FMT_NV12M:   /* Same as NV12      */
      rank = YUV_BASE_RANK + 8;
      break;
    case V4L2_PIX_FMT_YUYV:    /* YUY2, 16 bits per pixel */
      rank = YUV_BASE_RANK + 10;
      break;
    case V4L2_PIX_FMT_YVU420:  /* YV12, 12 bits per pixel */
    case V4L2_PIX_FMT_YVU420M:
      rank = YUV_BASE_RANK + 6;
      break;
    case V4L2_PIX_FMT_UYVY:    /* UYVY, 16 bits per pixel */
      rank = YUV_BASE_RANK + 9;
      break;
    case V4L2_PIX_FMT_YUV444:
      rank = YUV_BASE_RANK + 6;
      break;
    case V4L2_PIX_FMT_Y41P:    /* Y41P, 12 bits per pixel */
      rank = YUV_BASE_RANK + 5;
      break;
    case V4L2_PIX_FMT_YUV411P: /* Y41B, 12 bits per pixel */
      rank = YUV_BASE_RANK + 4;
      break;
    case V4L2_PIX_FMT_YUV422P: /* Y42B, 16 bits per pixel */
    case V4L2_PIX_FMT_YUV422M:
      rank = YUV_BASE_RANK + 8;
      break;

    case V4L2_PIX_FMT_DV:
      rank = DV_BASE_RANK;
      break;

    case V4L2_PIX_FMT_WNVA:    /* Winnov hw compress */
      rank = 0;
      break;

    case V4L2_PIX_FMT_SBGGR8:
    case V4L2_PIX_FMT_SGBRG8:
    case V4L2_PIX_FMT_SGRBG8:
    case V4L2_PIX_FMT_SRGGB8:
    case V4L2_PIX_FMT_SBGGR10:
    case V4L2_PIX_FMT_SGBRG10:
    case V4L2_PIX_FMT_SGRBG10:
    case V4L2_PIX_FMT_SRGGB10:
    case V4L2_PIX_FMT_SBGGR12:
    case V4L2_PIX_FMT_SGBRG12:
    case V4L2_PIX_FMT_SGRBG12:
    case V4L2_PIX_FMT_SRGGB12:
    case V4L2_PIX_FMT_SBGGR14:
    case V4L2_PIX_FMT_SGBRG14:
    case V4L2_PIX_FMT_SGRBG14:
    case V4L2_PIX_FMT_SRGGB14:
    case V4L2_PIX_FMT_SBGGR16:
    case V4L2_PIX_FMT_SGBRG16:
    case V4L2_PIX_FMT_SGRBG16:
    case V4L2_PIX_FMT_SRGGB16:
      rank = BAYER_BASE_RANK;
      break;

    case V4L2_PIX_FMT_SN9C10X:
      rank = S910_BASE_RANK;
      break;

    case V4L2_PIX_FMT_PWC1:
      rank = PWC_BASE_RANK;
      break;
    case V4L2_PIX_FMT_PWC2:
      rank = PWC_BASE_RANK;
      break;

    default:
      rank = 0;
      break;
  }

  /* All ranks are below 1<<15 so a shift by 15
   * will a) make all non-emulated formats larger
   * than emulated and b) will not overflow
   */
  if (!emulated)
    rank <<= 15;

  return rank;
}



static gint
format_cmp_func (gconstpointer a, gconstpointer b)
{
  const struct v4l2_fmtdesc *fa = a;
  const struct v4l2_fmtdesc *fb = b;

  if (fa->pixelformat == fb->pixelformat)
    return 0;

  return gst_v4l2_object_format_get_rank (fb) -
      gst_v4l2_object_format_get_rank (fa);
}

/******************************************************
 * gst_v4l2_object_fill_format_list():
 *   create list of supported capture formats
 * return value: TRUE on success, FALSE on error
 ******************************************************/
static gboolean
gst_v4l2_object_fill_format_list (GstV4l2Object * v4l2object,
    enum v4l2_buf_type type)
{
  gint n;
  struct v4l2_fmtdesc *format;

  GST_DEBUG_OBJECT (v4l2object->dbg_obj, "getting src format enumerations");

  /* format enumeration */
  for (n = 0;; n++) {
    format = g_new0 (struct v4l2_fmtdesc, 1);

    format->index = n;
    format->type = type;

    if (v4l2object->ioctl (v4l2object->video_fd, VIDIOC_ENUM_FMT, format) < 0) {
      if (errno == EINVAL) {
        g_free (format);
        break;                  /* end of enumeration */
      } else {
        goto failed;
      }
    }

    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "index:       %u", format->index);
    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "type:        %d", format->type);
    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "flags:       %08x", format->flags);
    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "description: '%s'", format->description);
    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "pixelformat: %" GST_FOURCC_FORMAT,
        GST_FOURCC_ARGS (format->pixelformat));

    /* sort formats according to our preference;  we do this, because caps
     * are probed in the order the formats are in the list, and the order of
     * formats in the final probed caps matters for things like fixation */
    v4l2object->formats = g_slist_insert_sorted (v4l2object->formats, format,
        (GCompareFunc) format_cmp_func);
  }

#ifndef GST_DISABLE_GST_DEBUG
  {
    GSList *l;

    GST_INFO_OBJECT (v4l2object->dbg_obj, "got %d format(s):", n);
    for (l = v4l2object->formats; l != NULL; l = l->next) {
      format = l->data;

      GST_INFO_OBJECT (v4l2object->dbg_obj,
          "  %" GST_FOURCC_FORMAT "%s", GST_FOURCC_ARGS (format->pixelformat),
          ((format->flags & V4L2_FMT_FLAG_EMULATED)) ? " (emulated)" : "");
    }
  }
#endif

  return TRUE;

  /* ERRORS */
failed:
  {
    g_free (format);

    if (v4l2object->element)
      return FALSE;

    return FALSE;
  }
}

/*
  * Get the list of supported capture formats, a list of
  * `struct v4l2_fmtdesc`.
  */
static GSList *
gst_v4l2_object_get_format_list (GstV4l2Object * v4l2object)
{
  if (!v4l2object->formats) {

    /* check usual way */
    gst_v4l2_object_fill_format_list (v4l2object, v4l2object->type);

    /* if our driver supports multi-planar
     * and if formats are still empty then we can workaround driver bug
     * by also looking up formats as if our device was not supporting
     * multiplanar */
    if (!v4l2object->formats) {
      switch (v4l2object->type) {
        case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
          gst_v4l2_object_fill_format_list (v4l2object,
              V4L2_BUF_TYPE_VIDEO_CAPTURE);
          break;

        case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
          gst_v4l2_object_fill_format_list (v4l2object,
              V4L2_BUF_TYPE_VIDEO_OUTPUT);
          break;

        default:
          break;
      }
    }
  }
  return v4l2object->formats;
}

static GstVideoFormat
gst_v4l2_object_v4l2fourcc_to_video_format (guint32 fourcc)
{
  GstVideoFormat format;

  switch (fourcc) {
    case V4L2_PIX_FMT_GREY:    /*  8  Greyscale     */
      format = GST_VIDEO_FORMAT_GRAY8;
      break;
    case V4L2_PIX_FMT_Y16:
      format = GST_VIDEO_FORMAT_GRAY16_LE;
      break;
    case V4L2_PIX_FMT_Y16_BE:
      format = GST_VIDEO_FORMAT_GRAY16_BE;
      break;
    case V4L2_PIX_FMT_XRGB555:
    case V4L2_PIX_FMT_RGB555:
      format = GST_VIDEO_FORMAT_RGB15;
      break;
    case V4L2_PIX_FMT_XRGB555X:
    case V4L2_PIX_FMT_RGB555X:
      format = GST_VIDEO_FORMAT_BGR15;
      break;
    case V4L2_PIX_FMT_RGB565:
      format = GST_VIDEO_FORMAT_RGB16;
      break;
    case V4L2_PIX_FMT_RGB24:
      format = GST_VIDEO_FORMAT_RGB;
      break;
    case V4L2_PIX_FMT_BGR24:
      format = GST_VIDEO_FORMAT_BGR;
      break;
    case V4L2_PIX_FMT_XRGB32:
    case V4L2_PIX_FMT_RGB32:
      format = GST_VIDEO_FORMAT_xRGB;
      break;
    case V4L2_PIX_FMT_RGBX32:
      format = GST_VIDEO_FORMAT_RGBx;
      break;
    case V4L2_PIX_FMT_XBGR32:
    case V4L2_PIX_FMT_BGR32:
      format = GST_VIDEO_FORMAT_BGRx;
      break;
    case V4L2_PIX_FMT_BGRX32:
      format = GST_VIDEO_FORMAT_xBGR;
      break;
    case V4L2_PIX_FMT_ABGR32:
      format = GST_VIDEO_FORMAT_BGRA;
      break;
    case V4L2_PIX_FMT_BGRA32:
      format = GST_VIDEO_FORMAT_ABGR;
      break;
    case V4L2_PIX_FMT_RGBA32:
      format = GST_VIDEO_FORMAT_RGBA;
      break;
    case V4L2_PIX_FMT_ARGB32:
      format = GST_VIDEO_FORMAT_ARGB;
      break;
    case V4L2_PIX_FMT_NV12:
    case V4L2_PIX_FMT_NV12M:
      format = GST_VIDEO_FORMAT_NV12;
      break;
    case V4L2_PIX_FMT_NV21:
    case V4L2_PIX_FMT_NV21M:
      format = GST_VIDEO_FORMAT_NV21;
      break;
    case V4L2_PIX_FMT_YVU410:
      format = GST_VIDEO_FORMAT_YVU9;
      break;
    case V4L2_PIX_FMT_YUV410:
      format = GST_VIDEO_FORMAT_YUV9;
      break;
    case V4L2_PIX_FMT_YUV420:
    case V4L2_PIX_FMT_YUV420M:
      format = GST_VIDEO_FORMAT_I420;
      break;
    case V4L2_PIX_FMT_YUYV:
      format = GST_VIDEO_FORMAT_YUY2;
      break;
    case V4L2_PIX_FMT_YVU420:
    case V4L2_PIX_FMT_YVU420M:
      format = GST_VIDEO_FORMAT_YV12;
      break;
    case V4L2_PIX_FMT_UYVY:
      format = GST_VIDEO_FORMAT_UYVY;
      break;
    case V4L2_PIX_FMT_YUV411P:
      format = GST_VIDEO_FORMAT_Y41B;
      break;
    case V4L2_PIX_FMT_YUV422P:
    case V4L2_PIX_FMT_YUV422M:
      format = GST_VIDEO_FORMAT_Y42B;
      break;
    case V4L2_PIX_FMT_YVYU:
      format = GST_VIDEO_FORMAT_YVYU;
      break;
    case V4L2_PIX_FMT_NV16:
    case V4L2_PIX_FMT_NV16M:
      format = GST_VIDEO_FORMAT_NV16;
      break;
    case V4L2_PIX_FMT_NV61:
    case V4L2_PIX_FMT_NV61M:
      format = GST_VIDEO_FORMAT_NV61;
      break;
    case V4L2_PIX_FMT_NV24:
      format = GST_VIDEO_FORMAT_NV24;
      break;
    default:
      format = GST_VIDEO_FORMAT_UNKNOWN;
      break;
  }

  return format;
}

static gboolean
gst_v4l2_object_v4l2fourcc_is_rgb (guint32 fourcc)
{
  gboolean ret = FALSE;

  switch (fourcc) {
    case V4L2_PIX_FMT_XRGB555:
    case V4L2_PIX_FMT_RGB555:
    case V4L2_PIX_FMT_XRGB555X:
    case V4L2_PIX_FMT_RGB555X:
    case V4L2_PIX_FMT_RGB565:
    case V4L2_PIX_FMT_RGB24:
    case V4L2_PIX_FMT_BGR24:
    case V4L2_PIX_FMT_XRGB32:
    case V4L2_PIX_FMT_RGB32:
    case V4L2_PIX_FMT_RGBA32:
    case V4L2_PIX_FMT_RGBX32:
    case V4L2_PIX_FMT_XBGR32:
    case V4L2_PIX_FMT_BGR32:
    case V4L2_PIX_FMT_BGRA32:
    case V4L2_PIX_FMT_BGRX32:
    case V4L2_PIX_FMT_ABGR32:
    case V4L2_PIX_FMT_ARGB32:
    case V4L2_PIX_FMT_SBGGR8:
    case V4L2_PIX_FMT_SGBRG8:
    case V4L2_PIX_FMT_SGRBG8:
    case V4L2_PIX_FMT_SRGGB8:
    case V4L2_PIX_FMT_SBGGR10:
    case V4L2_PIX_FMT_SGBRG10:
    case V4L2_PIX_FMT_SGRBG10:
    case V4L2_PIX_FMT_SRGGB10:
    case V4L2_PIX_FMT_SBGGR12:
    case V4L2_PIX_FMT_SGBRG12:
    case V4L2_PIX_FMT_SGRBG12:
    case V4L2_PIX_FMT_SRGGB12:
    case V4L2_PIX_FMT_SBGGR14:
    case V4L2_PIX_FMT_SGBRG14:
    case V4L2_PIX_FMT_SGRBG14:
    case V4L2_PIX_FMT_SRGGB14:
    case V4L2_PIX_FMT_SBGGR16:
    case V4L2_PIX_FMT_SGBRG16:
    case V4L2_PIX_FMT_SGRBG16:
    case V4L2_PIX_FMT_SRGGB16:
      ret = TRUE;
      break;
    default:
      break;
  }

  return ret;
}

static GstStructure *
gst_v4l2_object_v4l2fourcc_to_bare_struct (guint32 fourcc)
{
  GstStructure *structure = NULL;
  const gchar *bayer_format = NULL;

  switch (fourcc) {
    case V4L2_PIX_FMT_MJPEG:   /* Motion-JPEG */
    case V4L2_PIX_FMT_PJPG:    /* Progressive-JPEG */
    case V4L2_PIX_FMT_JPEG:    /* JFIF JPEG */
      structure = gst_structure_new ("image/jpeg",
          "parsed", G_TYPE_BOOLEAN, TRUE, NULL);
      break;
    case V4L2_PIX_FMT_MPEG1:
      structure = gst_structure_new ("video/mpeg",
          "mpegversion", G_TYPE_INT, 1, NULL);
      break;
    case V4L2_PIX_FMT_MPEG2:
      structure = gst_structure_new ("video/mpeg",
          "mpegversion", G_TYPE_INT, 2, NULL);
      break;
    case V4L2_PIX_FMT_MPEG4:
    case V4L2_PIX_FMT_XVID:
      structure = gst_structure_new ("video/mpeg",
          "mpegversion", G_TYPE_INT, 4, "systemstream",
          G_TYPE_BOOLEAN, FALSE, NULL);
      break;
    case V4L2_PIX_FMT_FWHT:
      structure = gst_structure_new_empty ("video/x-fwht");
      break;
    case V4L2_PIX_FMT_H263:
      structure = gst_structure_new ("video/x-h263",
          "variant", G_TYPE_STRING, "itu", NULL);
      break;
    case V4L2_PIX_FMT_H264:    /* H.264 */
      structure = gst_structure_new ("video/x-h264",
          "stream-format", G_TYPE_STRING, "byte-stream", "alignment",
          G_TYPE_STRING, "au", NULL);
      break;
    case V4L2_PIX_FMT_H264_NO_SC:
      structure = gst_structure_new ("video/x-h264",
          "stream-format", G_TYPE_STRING, "avc", "alignment",
          G_TYPE_STRING, "au", NULL);
      break;
    case V4L2_PIX_FMT_HEVC:    /* H.265 */
      structure = gst_structure_new ("video/x-h265",
          "stream-format", G_TYPE_STRING, "byte-stream", "alignment",
          G_TYPE_STRING, "au", NULL);
      break;
    case V4L2_PIX_FMT_VC1_ANNEX_G:
    case V4L2_PIX_FMT_VC1_ANNEX_L:
      structure = gst_structure_new ("video/x-wmv",
          "wmvversion", G_TYPE_INT, 3, "format", G_TYPE_STRING, "WVC1", NULL);
      break;
    case V4L2_PIX_FMT_VP8:
      structure = gst_structure_new_empty ("video/x-vp8");
      break;
    case V4L2_PIX_FMT_VP9:
      structure = gst_structure_new_empty ("video/x-vp9");
      break;
    case V4L2_PIX_FMT_GREY:    /*  8  Greyscale     */
    case V4L2_PIX_FMT_Y16:
    case V4L2_PIX_FMT_Y16_BE:
    case V4L2_PIX_FMT_XRGB555:
    case V4L2_PIX_FMT_RGB555:
    case V4L2_PIX_FMT_XRGB555X:
    case V4L2_PIX_FMT_RGB555X:
    case V4L2_PIX_FMT_RGB565:
    case V4L2_PIX_FMT_RGB24:
    case V4L2_PIX_FMT_BGR24:
    case V4L2_PIX_FMT_RGB32:
    case V4L2_PIX_FMT_XRGB32:
    case V4L2_PIX_FMT_ARGB32:
    case V4L2_PIX_FMT_RGBX32:
    case V4L2_PIX_FMT_RGBA32:
    case V4L2_PIX_FMT_BGR32:
    case V4L2_PIX_FMT_BGRX32:
    case V4L2_PIX_FMT_BGRA32:
    case V4L2_PIX_FMT_XBGR32:
    case V4L2_PIX_FMT_ABGR32:
    case V4L2_PIX_FMT_NV12:    /* 12  Y/CbCr 4:2:0  */
    case V4L2_PIX_FMT_NV12M:
    case V4L2_PIX_FMT_NV12MT:
    case V4L2_PIX_FMT_MM21:
    case V4L2_PIX_FMT_NV12M_8L128:
    case V4L2_PIX_FMT_NV12M_10BE_8L128:
    case V4L2_PIX_FMT_NV21:    /* 12  Y/CrCb 4:2:0  */
    case V4L2_PIX_FMT_NV21M:
    case V4L2_PIX_FMT_NV16:    /* 16  Y/CbCr 4:2:2  */
    case V4L2_PIX_FMT_NV16M:
    case V4L2_PIX_FMT_NV61:    /* 16  Y/CrCb 4:2:2  */
    case V4L2_PIX_FMT_NV61M:
    case V4L2_PIX_FMT_NV24:    /* 24  Y/CrCb 4:4:4  */
    case V4L2_PIX_FMT_YVU410:
    case V4L2_PIX_FMT_YUV410:
    case V4L2_PIX_FMT_YUV420:  /* I420/IYUV */
    case V4L2_PIX_FMT_YUV420M:
    case V4L2_PIX_FMT_YUYV:
    case V4L2_PIX_FMT_YVU420:
    case V4L2_PIX_FMT_YVU420M:
    case V4L2_PIX_FMT_UYVY:
    case V4L2_PIX_FMT_YUV422P:
    case V4L2_PIX_FMT_YUV422M:
    case V4L2_PIX_FMT_YVYU:
    case V4L2_PIX_FMT_YUV411P:{
      GstVideoFormat format;
      format = gst_v4l2_object_v4l2fourcc_to_video_format (fourcc);
      if (format != GST_VIDEO_FORMAT_UNKNOWN)
        structure = gst_structure_new ("video/x-raw",
            "format", G_TYPE_STRING, gst_video_format_to_string (format), NULL);
      break;
    }
    case V4L2_PIX_FMT_DV:
      structure =
          gst_structure_new ("video/x-dv", "systemstream", G_TYPE_BOOLEAN, TRUE,
          NULL);
      break;
    case V4L2_PIX_FMT_MPEG:    /* MPEG          */
      structure = gst_structure_new ("video/mpegts",
          "systemstream", G_TYPE_BOOLEAN, TRUE, NULL);
      break;
    case V4L2_PIX_FMT_WNVA:    /* Winnov hw compress */
      break;
    case V4L2_PIX_FMT_SBGGR8:
      bayer_format = "bggr";
      break;
    case V4L2_PIX_FMT_SBGGR10:
      bayer_format = "bggr10le";
      break;
    case V4L2_PIX_FMT_SBGGR12:
      bayer_format = "bggr12le";
      break;
    case V4L2_PIX_FMT_SBGGR14:
      bayer_format = "bggr14le";
      break;
    case V4L2_PIX_FMT_SBGGR16:
      bayer_format = "bggr16le";
      break;
    case V4L2_PIX_FMT_SGBRG8:
      bayer_format = "gbrgle";
      break;
    case V4L2_PIX_FMT_SGBRG10:
      bayer_format = "gbrg10le";
      break;
    case V4L2_PIX_FMT_SGBRG12:
      bayer_format = "gbrg12le";
      break;
    case V4L2_PIX_FMT_SGBRG14:
      bayer_format = "gbrg14le";
      break;
    case V4L2_PIX_FMT_SGBRG16:
      bayer_format = "gbrg16le";
      break;
    case V4L2_PIX_FMT_SGRBG8:
      bayer_format = "grbgle";
      break;
    case V4L2_PIX_FMT_SGRBG10:
      bayer_format = "grbg10le";
      break;
    case V4L2_PIX_FMT_SGRBG12:
      bayer_format = "grbg12le";
      break;
    case V4L2_PIX_FMT_SGRBG14:
      bayer_format = "grbg14le";
      break;
    case V4L2_PIX_FMT_SGRBG16:
      bayer_format = "grbg16le";
      break;
    case V4L2_PIX_FMT_SRGGB8:
      bayer_format = "rggble";
      break;
    case V4L2_PIX_FMT_SRGGB10:
      bayer_format = "rggb10le";
      break;
    case V4L2_PIX_FMT_SRGGB12:
      bayer_format = "rggb12le";
      break;
    case V4L2_PIX_FMT_SRGGB14:
      bayer_format = "rggb14le";
      break;
    case V4L2_PIX_FMT_SRGGB16:
      bayer_format = "rggb16le";
      break;
    case V4L2_PIX_FMT_SN9C10X:
      structure = gst_structure_new_empty ("video/x-sonix");
      break;
    case V4L2_PIX_FMT_PWC1:
      structure = gst_structure_new_empty ("video/x-pwc1");
      break;
    case V4L2_PIX_FMT_PWC2:
      structure = gst_structure_new_empty ("video/x-pwc2");
      break;
    case V4L2_PIX_FMT_RGB332:
    case V4L2_PIX_FMT_BGR666:
    case V4L2_PIX_FMT_ARGB555X:
    case V4L2_PIX_FMT_RGB565X:
    case V4L2_PIX_FMT_RGB444:
    case V4L2_PIX_FMT_YYUV:    /* 16  YUV 4:2:2     */
    case V4L2_PIX_FMT_HI240:   /*  8  8-bit color   */
    case V4L2_PIX_FMT_Y4:
    case V4L2_PIX_FMT_Y6:
    case V4L2_PIX_FMT_Y10:
    case V4L2_PIX_FMT_Y12:
    case V4L2_PIX_FMT_Y10BPACK:
    case V4L2_PIX_FMT_YUV444:
    case V4L2_PIX_FMT_YUV555:
    case V4L2_PIX_FMT_YUV565:
    case V4L2_PIX_FMT_Y41P:
    case V4L2_PIX_FMT_YUV32:
    case V4L2_PIX_FMT_NV12MT_16X16:
    case V4L2_PIX_FMT_NV42:
    case V4L2_PIX_FMT_H264_MVC:
    default:
      GST_DEBUG ("Unsupported fourcc 0x%08x %" GST_FOURCC_FORMAT,
          fourcc, GST_FOURCC_ARGS (fourcc));
      break;
  }

  if (bayer_format)
    structure = gst_structure_new ("video/x-bayer", "format", G_TYPE_STRING,
        bayer_format, NULL);

  return structure;
}

GstStructure *
gst_v4l2_object_v4l2fourcc_to_structure (guint32 fourcc)
{
  GstStructure *template;
  guint i;

  template = gst_v4l2_object_v4l2fourcc_to_bare_struct (fourcc);

  if (template == NULL)
    goto done;

  for (i = 0; i < GST_V4L2_FORMAT_COUNT; i++) {
    if (gst_v4l2_formats[i].format != fourcc)
      continue;

    if (gst_v4l2_formats[i].dimensions) {
      gst_structure_set (template,
          "width", GST_TYPE_INT_RANGE, 1, GST_V4L2_MAX_SIZE,
          "height", GST_TYPE_INT_RANGE, 1, GST_V4L2_MAX_SIZE,
          "framerate", GST_TYPE_FRACTION_RANGE, 0, 1, G_MAXINT, 1, NULL);
    }
    break;
  }

done:
  return template;
}

gboolean
gst_v4l2_object_is_raw (GstV4l2Object * v4l2object)
{
  guint i;

  if (GST_VIDEO_INFO_FORMAT (&v4l2object->info) != GST_VIDEO_FORMAT_ENCODED)
    return TRUE;

  for (i = 0; i < GST_V4L2_FORMAT_COUNT; i++) {
    if (gst_v4l2_formats[i].format == GST_V4L2_PIXELFORMAT (v4l2object)) {
      return !!(gst_v4l2_formats[i].flags & GST_V4L2_RAW);
    }
  }
  return FALSE;
}

/* Add an 'alternate' variant of the caps with the feature */
static void
add_alternate_variant (GstV4l2Object * v4l2object, GstCaps * caps,
    GstStructure * structure)
{
  GstStructure *alt_s;

  if (v4l2object && v4l2object->never_interlaced)
    return;

  if (!gst_structure_has_name (structure, "video/x-raw"))
    return;

  alt_s = gst_structure_copy (structure);
  gst_structure_set (alt_s, "interlace-mode", G_TYPE_STRING, "alternate", NULL);

  gst_caps_append_structure_full (caps, alt_s,
      gst_caps_features_new (GST_CAPS_FEATURE_FORMAT_INTERLACED, NULL));
}

static GstCaps *
gst_v4l2_object_get_caps_helper (GstV4L2FormatFlags flags)
{
  GstStructure *structure;
  GstCaps *caps, *caps_interlaced;
  guint i;

  caps = gst_caps_new_empty ();
  caps_interlaced = gst_caps_new_empty ();
  for (i = 0; i < GST_V4L2_FORMAT_COUNT; i++) {

    if ((gst_v4l2_formats[i].flags & flags) == 0)
      continue;

    structure =
        gst_v4l2_object_v4l2fourcc_to_bare_struct (gst_v4l2_formats[i].format);

    if (structure) {
      GstStructure *alt_s = NULL;

      if (gst_v4l2_formats[i].dimensions) {
        gst_structure_set (structure,
            "width", GST_TYPE_INT_RANGE, 1, GST_V4L2_MAX_SIZE,
            "height", GST_TYPE_INT_RANGE, 1, GST_V4L2_MAX_SIZE,
            "framerate", GST_TYPE_FRACTION_RANGE, 0, 1, G_MAXINT, 1, NULL);
      }

      switch (gst_v4l2_formats[i].format) {
        case V4L2_PIX_FMT_RGB32:
          alt_s = gst_structure_copy (structure);
          gst_structure_set (alt_s, "format", G_TYPE_STRING, "ARGB", NULL);
          break;
        case V4L2_PIX_FMT_BGR32:
          alt_s = gst_structure_copy (structure);
          gst_structure_set (alt_s, "format", G_TYPE_STRING, "BGRA", NULL);
        default:
          break;
      }

      gst_caps_append_structure (caps, structure);

      if (alt_s) {
        gst_caps_append_structure (caps, alt_s);
        add_alternate_variant (NULL, caps_interlaced, alt_s);
      }

      add_alternate_variant (NULL, caps_interlaced, structure);
    }
  }

  caps = gst_caps_simplify (caps);
  caps_interlaced = gst_caps_simplify (caps_interlaced);

  return gst_caps_merge (caps, caps_interlaced);
}

GstCaps *
gst_v4l2_object_get_all_caps (void)
{
  static GstCaps *caps = NULL;

  if (g_once_init_enter (&caps)) {
    GstCaps *all_caps = gst_v4l2_object_get_caps_helper (GST_V4L2_ALL);
    GST_MINI_OBJECT_FLAG_SET (all_caps, GST_MINI_OBJECT_FLAG_MAY_BE_LEAKED);
    g_once_init_leave (&caps, all_caps);
  }

  return caps;
}

GstCaps *
gst_v4l2_object_get_raw_caps (void)
{
  static GstCaps *caps = NULL;

  if (g_once_init_enter (&caps)) {
    GstCaps *raw_caps = gst_v4l2_object_get_caps_helper (GST_V4L2_RAW);
    GST_MINI_OBJECT_FLAG_SET (raw_caps, GST_MINI_OBJECT_FLAG_MAY_BE_LEAKED);
    g_once_init_leave (&caps, raw_caps);
  }

  return caps;
}

GstCaps *
gst_v4l2_object_get_codec_caps (void)
{
  static GstCaps *caps = NULL;

  if (g_once_init_enter (&caps)) {
    GstCaps *codec_caps = gst_v4l2_object_get_caps_helper (GST_V4L2_CODEC);
    GST_MINI_OBJECT_FLAG_SET (codec_caps, GST_MINI_OBJECT_FLAG_MAY_BE_LEAKED);
    g_once_init_leave (&caps, codec_caps);
  }

  return caps;
}

/* collect data for the given caps
 * @caps: given input caps
 * @format: location for the v4l format
 * @w/@h: location for width and height
 * @fps_n/@fps_d: location for framerate
 * @size: location for expected size of the frame or 0 if unknown
 */
static gboolean
gst_v4l2_object_get_caps_info (GstV4l2Object * v4l2object, GstCaps * caps,
    struct v4l2_fmtdesc **format, GstVideoInfo * info)
{
  GstStructure *structure;
  guint32 fourcc = 0, fourcc_nc = 0;
  const gchar *mimetype;
  struct v4l2_fmtdesc *fmt = NULL;

  structure = gst_caps_get_structure (caps, 0);

  mimetype = gst_structure_get_name (structure);

  if (!gst_video_info_from_caps (info, caps))
    goto invalid_format;

  if (g_str_equal (mimetype, "video/x-raw")) {
    switch (GST_VIDEO_INFO_FORMAT (info)) {
      case GST_VIDEO_FORMAT_I420:
        fourcc = V4L2_PIX_FMT_YUV420;
        fourcc_nc = V4L2_PIX_FMT_YUV420M;
        break;
      case GST_VIDEO_FORMAT_YUY2:
        fourcc = V4L2_PIX_FMT_YUYV;
        break;
      case GST_VIDEO_FORMAT_UYVY:
        fourcc = V4L2_PIX_FMT_UYVY;
        break;
      case GST_VIDEO_FORMAT_YV12:
        fourcc = V4L2_PIX_FMT_YVU420;
        fourcc_nc = V4L2_PIX_FMT_YVU420M;
        break;
      case GST_VIDEO_FORMAT_Y41B:
        fourcc = V4L2_PIX_FMT_YUV411P;
        break;
      case GST_VIDEO_FORMAT_Y42B:
        fourcc = V4L2_PIX_FMT_YUV422P;
        fourcc_nc = V4L2_PIX_FMT_YUV422M;
        break;
      case GST_VIDEO_FORMAT_NV12:
        fourcc = V4L2_PIX_FMT_NV12;
        fourcc_nc = V4L2_PIX_FMT_NV12M;
        break;
      case GST_VIDEO_FORMAT_NV21:
        fourcc = V4L2_PIX_FMT_NV21;
        fourcc_nc = V4L2_PIX_FMT_NV21M;
        break;
      case GST_VIDEO_FORMAT_NV16:
        fourcc = V4L2_PIX_FMT_NV16;
        fourcc_nc = V4L2_PIX_FMT_NV16M;
        break;
      case GST_VIDEO_FORMAT_NV61:
        fourcc = V4L2_PIX_FMT_NV61;
        fourcc_nc = V4L2_PIX_FMT_NV61M;
        break;
      case GST_VIDEO_FORMAT_NV24:
        fourcc = V4L2_PIX_FMT_NV24;
        break;
      case GST_VIDEO_FORMAT_YVYU:
        fourcc = V4L2_PIX_FMT_YVYU;
        break;
      case GST_VIDEO_FORMAT_RGB15:
        fourcc = V4L2_PIX_FMT_RGB555;
        fourcc_nc = V4L2_PIX_FMT_XRGB555;
        break;
      case GST_VIDEO_FORMAT_RGB16:
        fourcc = V4L2_PIX_FMT_RGB565;
        break;
      case GST_VIDEO_FORMAT_RGB:
        fourcc = V4L2_PIX_FMT_RGB24;
        break;
      case GST_VIDEO_FORMAT_BGR:
        fourcc = V4L2_PIX_FMT_BGR24;
        break;
      case GST_VIDEO_FORMAT_xRGB:
        fourcc = V4L2_PIX_FMT_RGB32;
        fourcc_nc = V4L2_PIX_FMT_XRGB32;
        break;
      case GST_VIDEO_FORMAT_RGBx:
        fourcc = V4L2_PIX_FMT_RGBX32;
        break;
      case GST_VIDEO_FORMAT_ARGB:
        fourcc = V4L2_PIX_FMT_RGB32;
        fourcc_nc = V4L2_PIX_FMT_ARGB32;
        break;
      case GST_VIDEO_FORMAT_RGBA:
        fourcc = V4L2_PIX_FMT_RGBA32;
        break;
      case GST_VIDEO_FORMAT_BGRx:
        fourcc = V4L2_PIX_FMT_BGR32;
        fourcc_nc = V4L2_PIX_FMT_XBGR32;
        break;
      case GST_VIDEO_FORMAT_xBGR:
        fourcc = V4L2_PIX_FMT_BGRX32;
        break;
      case GST_VIDEO_FORMAT_BGRA:
        fourcc = V4L2_PIX_FMT_BGR32;
        fourcc_nc = V4L2_PIX_FMT_ABGR32;
        break;
      case GST_VIDEO_FORMAT_ABGR:
        fourcc = V4L2_PIX_FMT_BGRA32;
        break;
      case GST_VIDEO_FORMAT_GRAY8:
        fourcc = V4L2_PIX_FMT_GREY;
        break;
      case GST_VIDEO_FORMAT_GRAY16_LE:
        fourcc = V4L2_PIX_FMT_Y16;
        break;
      case GST_VIDEO_FORMAT_GRAY16_BE:
        fourcc = V4L2_PIX_FMT_Y16_BE;
        break;
      case GST_VIDEO_FORMAT_BGR15:
        fourcc = V4L2_PIX_FMT_RGB555X;
        fourcc_nc = V4L2_PIX_FMT_XRGB555X;
        break;
      default:
        break;
    }
  }

  /* Prefer the non-contiguous if supported */
  v4l2object->prefered_non_contiguous = TRUE;

  if (fourcc_nc)
    fmt = gst_v4l2_object_get_format_from_fourcc (v4l2object, fourcc_nc);
  else if (fourcc == 0)
    goto unhandled_format;

  if (fmt == NULL) {
    fmt = gst_v4l2_object_get_format_from_fourcc (v4l2object, fourcc);
    v4l2object->prefered_non_contiguous = FALSE;
  }

  if (fmt == NULL)
    goto unsupported_format;

  *format = fmt;

  return TRUE;

  /* ERRORS */
invalid_format:
  {
    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "invalid format");
    return FALSE;
  }
unhandled_format:
  {
    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "unhandled format");
    return FALSE;
  }
unsupported_format:
  {
    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "unsupported format");
    return FALSE;
  }
}

static gboolean
gst_v4l2_object_get_nearest_size (GstV4l2Object * v4l2object,
    guint32 pixelformat, gint * width, gint * height);

/* returns TRUE if the value was changed in place, otherwise FALSE */
static gboolean
gst_v4l2src_value_simplify (GValue * val)
{
  /* simplify list of one value to one value */
  if (GST_VALUE_HOLDS_LIST (val) && gst_value_list_get_size (val) == 1) {
    const GValue *list_val;
    GValue new_val = G_VALUE_INIT;

    list_val = gst_value_list_get_value (val, 0);
    g_value_init (&new_val, G_VALUE_TYPE (list_val));
    g_value_copy (list_val, &new_val);
    g_value_unset (val);
    *val = new_val;
    return TRUE;
  }

  return FALSE;
}

static gboolean
gst_v4l2_object_get_streamparm (GstV4l2Object * v4l2object, GstVideoInfo * info)
{
  struct v4l2_streamparm streamparm;
  memset (&streamparm, 0x00, sizeof (struct v4l2_streamparm));
  streamparm.type = v4l2object->type;
  if (v4l2object->ioctl (v4l2object->video_fd, VIDIOC_G_PARM, &streamparm) < 0) {
    GST_WARNING_OBJECT (v4l2object->dbg_obj, "VIDIOC_G_PARM failed");
    return FALSE;
  }
  if ((streamparm.parm.capture.timeperframe.numerator != 0)
      && (v4l2object->type == V4L2_BUF_TYPE_VIDEO_CAPTURE
          || v4l2object->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)) {
    GST_VIDEO_INFO_FPS_N (info) =
        streamparm.parm.capture.timeperframe.denominator;
    GST_VIDEO_INFO_FPS_D (info) =
        streamparm.parm.capture.timeperframe.numerator;
  }
  return TRUE;
}

static int
gst_v4l2_object_try_fmt (GstV4l2Object * v4l2object,
    struct v4l2_format *try_fmt)
{
  int fd = v4l2object->video_fd;
  struct v4l2_format fmt;
  int r;

  memcpy (&fmt, try_fmt, sizeof (fmt));
  r = v4l2object->ioctl (fd, VIDIOC_TRY_FMT, &fmt);

  if (r < 0 && errno == ENOTTY) {
    /* The driver might not implement TRY_FMT, in which case we will try
       S_FMT to probe */
    if (GST_V4L2_IS_ACTIVE (v4l2object))
      goto error;

    memcpy (&fmt, try_fmt, sizeof (fmt));
    r = v4l2object->ioctl (fd, VIDIOC_S_FMT, &fmt);
  }
  memcpy (try_fmt, &fmt, sizeof (fmt));

  return r;

error:
  memcpy (try_fmt, &fmt, sizeof (fmt));
  GST_WARNING_OBJECT (v4l2object->dbg_obj,
      "Unable to try format: %s", g_strerror (errno));
  return r;
}

/* The frame interval enumeration code first appeared in Linux 2.6.19. */
static GstStructure *
gst_v4l2_object_probe_caps_for_format_and_size (GstV4l2Object * v4l2object,
    guint32 pixelformat,
    guint32 width, guint32 height, const GstStructure * template)
{
  gint fd = v4l2object->video_fd;
  struct v4l2_frmivalenum ival;
  guint32 num, denom;
  GstStructure *s;
  GValue rates = { 0, };

  memset (&ival, 0, sizeof (struct v4l2_frmivalenum));
  ival.index = 0;
  ival.pixel_format = pixelformat;
  ival.width = width;
  ival.height = height;

  GST_LOG_OBJECT (v4l2object->dbg_obj,
      "get frame interval for %ux%u, %" GST_FOURCC_FORMAT, width, height,
      GST_FOURCC_ARGS (pixelformat));

  /* keep in mind that v4l2 gives us frame intervals (durations); we invert the
   * fraction to get framerate */
  if (v4l2object->ioctl (fd, VIDIOC_ENUM_FRAMEINTERVALS, &ival) < 0)
    goto enum_frameintervals_failed;

  if (ival.type == V4L2_FRMIVAL_TYPE_DISCRETE) {
    GValue rate = { 0, };

    g_value_init (&rates, GST_TYPE_LIST);
    g_value_init (&rate, GST_TYPE_FRACTION);

    do {
      num = ival.discrete.numerator;
      denom = ival.discrete.denominator;

      if (num > G_MAXINT || denom > G_MAXINT) {
        /* let us hope we don't get here... */
        num >>= 1;
        denom >>= 1;
      }

      GST_INFO_OBJECT (v4l2object->dbg_obj, "adding discrete framerate: %d/%d",
          denom, num);

      /* swap to get the framerate */
      gst_value_set_fraction (&rate, denom, num);
      gst_value_list_append_value (&rates, &rate);

      ival.index++;
    } while (v4l2object->ioctl (fd, VIDIOC_ENUM_FRAMEINTERVALS, &ival) >= 0);
  } else if (ival.type == V4L2_FRMIVAL_TYPE_STEPWISE) {
    GValue min = { 0, };
    GValue step = { 0, };
    GValue max = { 0, };
    gboolean added = FALSE;
    guint32 minnum, mindenom;
    guint32 maxnum, maxdenom;

    g_value_init (&rates, GST_TYPE_LIST);

    g_value_init (&min, GST_TYPE_FRACTION);
    g_value_init (&step, GST_TYPE_FRACTION);
    g_value_init (&max, GST_TYPE_FRACTION);

    /* get the min */
    minnum = ival.stepwise.min.numerator;
    mindenom = ival.stepwise.min.denominator;
    if (minnum > G_MAXINT || mindenom > G_MAXINT) {
      minnum >>= 1;
      mindenom >>= 1;
    }
    GST_LOG_OBJECT (v4l2object->dbg_obj, "stepwise min frame interval: %d/%d",
        minnum, mindenom);
    gst_value_set_fraction (&min, minnum, mindenom);

    /* get the max */
    maxnum = ival.stepwise.max.numerator;
    maxdenom = ival.stepwise.max.denominator;
    if (maxnum > G_MAXINT || maxdenom > G_MAXINT) {
      maxnum >>= 1;
      maxdenom >>= 1;
    }

    GST_LOG_OBJECT (v4l2object->dbg_obj, "stepwise max frame interval: %d/%d",
        maxnum, maxdenom);
    gst_value_set_fraction (&max, maxnum, maxdenom);

    /* get the step */
    num = ival.stepwise.step.numerator;
    denom = ival.stepwise.step.denominator;
    if (num > G_MAXINT || denom > G_MAXINT) {
      num >>= 1;
      denom >>= 1;
    }

    if (num == 0 || denom == 0) {
      /* in this case we have a wrong fraction or no step, set the step to max
       * so that we only add the min value in the loop below */
      num = maxnum;
      denom = maxdenom;
    }

    /* since we only have gst_value_fraction_subtract and not add, negate the
     * numerator */
    GST_LOG_OBJECT (v4l2object->dbg_obj, "stepwise step frame interval: %d/%d",
        num, denom);
    gst_value_set_fraction (&step, -num, denom);

    while (gst_value_compare (&min, &max) != GST_VALUE_GREATER_THAN) {
      GValue rate = { 0, };

      num = gst_value_get_fraction_numerator (&min);
      denom = gst_value_get_fraction_denominator (&min);
      GST_LOG_OBJECT (v4l2object->dbg_obj, "adding stepwise framerate: %d/%d",
          denom, num);

      /* invert to get the framerate */
      g_value_init (&rate, GST_TYPE_FRACTION);
      gst_value_set_fraction (&rate, denom, num);
      gst_value_list_append_value (&rates, &rate);
      added = TRUE;

      /* we're actually adding because step was negated above. This is because
       * there is no _add function... */
      if (!gst_value_fraction_subtract (&min, &min, &step)) {
        GST_WARNING_OBJECT (v4l2object->dbg_obj, "could not step fraction!");
        break;
      }
    }
    if (!added) {
      /* no range was added, leave the default range from the template */
      GST_WARNING_OBJECT (v4l2object->dbg_obj,
          "no range added, leaving default");
      g_value_unset (&rates);
    }
  } else if (ival.type == V4L2_FRMIVAL_TYPE_CONTINUOUS) {
    guint32 maxnum, maxdenom;

    g_value_init (&rates, GST_TYPE_FRACTION_RANGE);

    num = ival.stepwise.min.numerator;
    denom = ival.stepwise.min.denominator;
    if (num > G_MAXINT || denom > G_MAXINT) {
      num >>= 1;
      denom >>= 1;
    }

    maxnum = ival.stepwise.max.numerator;
    maxdenom = ival.stepwise.max.denominator;
    if (maxnum > G_MAXINT || maxdenom > G_MAXINT) {
      maxnum >>= 1;
      maxdenom >>= 1;
    }

    GST_LOG_OBJECT (v4l2object->dbg_obj,
        "continuous frame interval %d/%d to %d/%d", maxdenom, maxnum, denom,
        num);

    gst_value_set_fraction_range_full (&rates, maxdenom, maxnum, denom, num);
  } else {
    goto unknown_type;
  }

return_data:
  s = gst_structure_copy (template);
  gst_structure_set (s, "width", G_TYPE_INT, (gint) width,
      "height", G_TYPE_INT, (gint) height, NULL);

  if (G_IS_VALUE (&rates)) {
    gst_v4l2src_value_simplify (&rates);
    /* only change the framerate on the template when we have a valid probed new
     * value */
    gst_structure_take_value (s, "framerate", &rates);
  } else if (v4l2object->type == V4L2_BUF_TYPE_VIDEO_CAPTURE ||
      v4l2object->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
    gst_structure_set (s, "framerate", GST_TYPE_FRACTION_RANGE, 0, 1, G_MAXINT,
        1, NULL);
  }
  return s;

  /* ERRORS */
enum_frameintervals_failed:
  {
    GST_DEBUG_OBJECT (v4l2object->dbg_obj,
        "Unable to enumerate intervals for %" GST_FOURCC_FORMAT "@%ux%u",
        GST_FOURCC_ARGS (pixelformat), width, height);
    goto return_data;
  }
unknown_type:
  {
    /* I don't see how this is actually an error, we ignore the format then */
    GST_WARNING_OBJECT (v4l2object->dbg_obj,
        "Unknown frame interval type at %" GST_FOURCC_FORMAT "@%ux%u: %u",
        GST_FOURCC_ARGS (pixelformat), width, height, ival.type);
    return NULL;
  }
}

static gint
sort_by_frame_size (GstStructure * s1, GstStructure * s2)
{
  int w1, h1, w2, h2;

  gst_structure_get_int (s1, "width", &w1);
  gst_structure_get_int (s1, "height", &h1);
  gst_structure_get_int (s2, "width", &w2);
  gst_structure_get_int (s2, "height", &h2);

  /* I think it's safe to assume that this won't overflow for a while */
  return ((w2 * h2) - (w1 * h1));
}

static void
check_alternate_and_append_struct (GstCaps * caps, GstStructure * s)
{
  const GValue *mode;

  mode = gst_structure_get_value (s, "interlace-mode");
  if (!mode)
    goto done;

  if (G_VALUE_HOLDS_STRING (mode)) {
    /* Add the INTERLACED feature if the mode is alternate */
    if (!g_strcmp0 (gst_structure_get_string (s, "interlace-mode"),
            "alternate")) {
      GstCapsFeatures *feat;

      feat = gst_caps_features_new (GST_CAPS_FEATURE_FORMAT_INTERLACED, NULL);
      gst_caps_set_features (caps, gst_caps_get_size (caps) - 1, feat);
    }
  } else if (GST_VALUE_HOLDS_LIST (mode)) {
    /* If the mode is a list containing alternate, remove it from the list and add a
     * variant with interlace-mode=alternate and the INTERLACED feature. */
    GValue alter = G_VALUE_INIT;
    GValue inter = G_VALUE_INIT;

    g_value_init (&alter, G_TYPE_STRING);
    g_value_set_string (&alter, "alternate");

    /* Cannot use gst_value_can_intersect() as it requires args to have the
     * same type. */
    if (gst_value_intersect (&inter, mode, &alter)) {
      GValue minus_alter = G_VALUE_INIT;
      GstStructure *copy;

      gst_value_subtract (&minus_alter, mode, &alter);
      gst_structure_take_value (s, "interlace-mode", &minus_alter);

      copy = gst_structure_copy (s);
      gst_structure_take_value (copy, "interlace-mode", &inter);
      gst_caps_append_structure_full (caps, copy,
          gst_caps_features_new (GST_CAPS_FEATURE_FORMAT_INTERLACED, NULL));
    }
    g_value_unset (&alter);
  }

done:
  gst_caps_append_structure (caps, s);
}

static void
gst_v4l2_object_update_and_append (GstV4l2Object * v4l2object,
    guint32 format, GstCaps * caps, GstStructure * s)
{
  GstStructure *alt_s = NULL;

  /* Encoded stream on output buffer need to be parsed */
  if (v4l2object->type == V4L2_BUF_TYPE_VIDEO_OUTPUT ||
      v4l2object->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
    guint i = 0;

    for (; i < GST_V4L2_FORMAT_COUNT; i++) {
      if (format == gst_v4l2_formats[i].format &&
          gst_v4l2_formats[i].flags & GST_V4L2_CODEC &&
          !(gst_v4l2_formats[i].flags & GST_V4L2_NO_PARSE)) {
        gst_structure_set (s, "parsed", G_TYPE_BOOLEAN, TRUE, NULL);
        break;
      }
    }
  }

  if (v4l2object->has_alpha_component &&
      (v4l2object->type == V4L2_BUF_TYPE_VIDEO_CAPTURE ||
          v4l2object->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)) {
    switch (format) {
      case V4L2_PIX_FMT_RGB32:
        alt_s = gst_structure_copy (s);
        gst_structure_set (alt_s, "format", G_TYPE_STRING, "ARGB", NULL);
        break;
      case V4L2_PIX_FMT_BGR32:
        alt_s = gst_structure_copy (s);
        gst_structure_set (alt_s, "format", G_TYPE_STRING, "BGRA", NULL);
        break;
      default:
        break;
    }
  }

  check_alternate_and_append_struct (caps, s);

  if (alt_s) {
    check_alternate_and_append_struct (caps, alt_s);
  }
}

static GstCaps *
gst_v4l2_object_probe_caps_for_format (GstV4l2Object * v4l2object,
    guint32 pixelformat, const GstStructure * template)
{
  GstCaps *ret = gst_caps_new_empty ();
  GstStructure *tmp;
  gint fd = v4l2object->video_fd;
  struct v4l2_frmsizeenum size;
  GList *results = NULL;
  guint32 w, h;

  if (pixelformat == GST_MAKE_FOURCC ('M', 'P', 'E', 'G')) {
    gst_caps_append_structure (ret, gst_structure_copy (template));
    return ret;
  }

  memset (&size, 0, sizeof (struct v4l2_frmsizeenum));
  size.index = 0;
  size.pixel_format = pixelformat;

  GST_DEBUG_OBJECT (v4l2object->dbg_obj,
      "Enumerating frame sizes for %" GST_FOURCC_FORMAT,
      GST_FOURCC_ARGS (pixelformat));

  if (v4l2object->ioctl (fd, VIDIOC_ENUM_FRAMESIZES, &size) < 0)
    goto enum_framesizes_failed;

  if (size.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
    guint32 maxw = 0, maxh = 0;

    do {
      GST_LOG_OBJECT (v4l2object->dbg_obj, "got discrete frame size %dx%d",
          size.discrete.width, size.discrete.height);

      w = MIN (size.discrete.width, G_MAXINT);
      h = MIN (size.discrete.height, G_MAXINT);

      if (w && h) {
        tmp =
            gst_v4l2_object_probe_caps_for_format_and_size (v4l2object,
            pixelformat, w, h, template);

        if (tmp)
          results = g_list_prepend (results, tmp);
      }

      if (w > maxw && h > maxh) {
        maxw = w;
        maxh = h;
      }

      size.index++;
    } while (v4l2object->ioctl (fd, VIDIOC_ENUM_FRAMESIZES, &size) >= 0);

    v4l2object->max_width = maxw;
    v4l2object->max_height = maxh;
    GST_DEBUG_OBJECT (v4l2object->dbg_obj,
        "done iterating discrete frame sizes");
  } else if (size.type == V4L2_FRMSIZE_TYPE_STEPWISE) {
    guint32 maxw, maxh, step_w, step_h;

    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "we have stepwise frame sizes:");
    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "min width:   %d",
        size.stepwise.min_width);
    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "min height:  %d",
        size.stepwise.min_height);
    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "max width:   %d",
        size.stepwise.max_width);
    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "max height:  %d",
        size.stepwise.max_height);
    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "step width:  %d",
        size.stepwise.step_width);
    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "step height: %d",
        size.stepwise.step_height);

    step_w = MAX (size.stepwise.step_width, 1);
    step_h = MAX (size.stepwise.step_height, 1);
    w = MAX (size.stepwise.min_width, step_w);
    h = MAX (size.stepwise.min_height, step_h);
    maxw = MIN (size.stepwise.max_width + 1, G_MAXINT);
    maxh = MIN (size.stepwise.max_height + 1, G_MAXINT);

    /* FIXME: check for sanity and that min/max are multiples of the steps */

    /* we only query details for the max width/height since it's likely the
     * most restricted if there are any resolution-dependent restrictions */
    tmp = gst_v4l2_object_probe_caps_for_format_and_size (v4l2object,
        pixelformat, maxw, maxh, template);

    if (tmp) {
      GValue step_range = G_VALUE_INIT;

      g_value_init (&step_range, GST_TYPE_INT_RANGE);
      gst_value_set_int_range_step (&step_range, w, maxw, step_w);
      gst_structure_set_value (tmp, "width", &step_range);

      gst_value_set_int_range_step (&step_range, h, maxh, step_h);
      gst_structure_take_value (tmp, "height", &step_range);

      /* no point using the results list here, since there's only one struct */
      gst_v4l2_object_update_and_append (v4l2object, pixelformat, ret, tmp);

      v4l2object->max_width = maxw;
      v4l2object->max_height = maxh;
    }
  } else if (size.type == V4L2_FRMSIZE_TYPE_CONTINUOUS) {
    guint32 maxw, maxh;

    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "we have continuous frame sizes:");
    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "min width:   %d",
        size.stepwise.min_width);
    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "min height:  %d",
        size.stepwise.min_height);
    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "max width:   %d",
        size.stepwise.max_width);
    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "min height:  %d",
        size.stepwise.max_height);

    w = MAX (size.stepwise.min_width, 1);
    h = MAX (size.stepwise.min_height, 1);
    maxw = MIN (size.stepwise.max_width, G_MAXINT);
    maxh = MIN (size.stepwise.max_height, G_MAXINT);

    tmp =
        gst_v4l2_object_probe_caps_for_format_and_size (v4l2object, pixelformat,
        w, h, template);
    if (tmp) {
      gst_structure_set (tmp, "width", GST_TYPE_INT_RANGE, (gint) w,
          (gint) maxw, "height", GST_TYPE_INT_RANGE, (gint) h, (gint) maxh,
          NULL);

      /* no point using the results list here, since there's only one struct */
      gst_v4l2_object_update_and_append (v4l2object, pixelformat, ret, tmp);

      v4l2object->max_width = maxw;
      v4l2object->max_height = maxh;
    }
  } else {
    goto unknown_type;
  }

  /* we use an intermediary list to store and then sort the results of the
   * probing because we can't make any assumptions about the order in which
   * the driver will give us the sizes, but we want the final caps to contain
   * the results starting with the highest resolution and having the lowest
   * resolution last, since order in caps matters for things like fixation. */
  results = g_list_sort (results, (GCompareFunc) sort_by_frame_size);
  while (results != NULL) {
    gst_v4l2_object_update_and_append (v4l2object, pixelformat, ret,
        results->data);
    results = g_list_delete_link (results, results);
  }

  if (gst_caps_is_empty (ret))
    goto enum_framesizes_no_results;

  return ret;

  /* ERRORS */
enum_framesizes_failed:
  {
    /* I don't see how this is actually an error */
    GST_DEBUG_OBJECT (v4l2object->dbg_obj,
        "Failed to enumerate frame sizes for pixelformat %" GST_FOURCC_FORMAT
        " (%s)", GST_FOURCC_ARGS (pixelformat), g_strerror (errno));
    goto default_frame_sizes;
  }
enum_framesizes_no_results:
  {
    /* it's possible that VIDIOC_ENUM_FRAMESIZES is defined but the driver in
     * question doesn't actually support it yet */
    GST_DEBUG_OBJECT (v4l2object->dbg_obj,
        "No results for pixelformat %" GST_FOURCC_FORMAT
        " enumerating frame sizes, trying fallback",
        GST_FOURCC_ARGS (pixelformat));
    goto default_frame_sizes;
  }
unknown_type:
  {
    GST_WARNING_OBJECT (v4l2object->dbg_obj,
        "Unknown frame sizeenum type for pixelformat %" GST_FOURCC_FORMAT
        ": %u", GST_FOURCC_ARGS (pixelformat), size.type);
    goto default_frame_sizes;
  }

default_frame_sizes:
  {
    gint min_w, max_w, min_h, max_h, fix_num = 0, fix_denom = 0;

    /* This code is for Linux < 2.6.19 */
    min_w = min_h = 1;
    max_w = max_h = GST_V4L2_MAX_SIZE;
    if (!gst_v4l2_object_get_nearest_size (v4l2object, pixelformat, &min_w,
            &min_h)) {
      GST_WARNING_OBJECT (v4l2object->dbg_obj,
          "Could not probe minimum capture size for pixelformat %"
          GST_FOURCC_FORMAT, GST_FOURCC_ARGS (pixelformat));
    }
    if (!gst_v4l2_object_get_nearest_size (v4l2object, pixelformat, &max_w,
            &max_h)) {
      GST_WARNING_OBJECT (v4l2object->dbg_obj,
          "Could not probe maximum capture size for pixelformat %"
          GST_FOURCC_FORMAT, GST_FOURCC_ARGS (pixelformat));
    }
    if (min_w == 0 || min_h == 0)
      min_w = min_h = 1;
    if (max_w == 0 || max_h == 0)
      max_w = max_h = GST_V4L2_MAX_SIZE;
    v4l2object->max_width = max_w;
    v4l2object->max_height = max_h;

    tmp = gst_structure_copy (template);
    if (fix_num) {
      gst_structure_set (tmp, "framerate", GST_TYPE_FRACTION, fix_num,
          fix_denom, NULL);
    } else if (v4l2object->type == V4L2_BUF_TYPE_VIDEO_CAPTURE ||
        v4l2object->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
      /* if norm can't be used, copy the template framerate */
      gst_structure_set (tmp, "framerate", GST_TYPE_FRACTION_RANGE, 0, 1,
          G_MAXINT, 1, NULL);
    }

    if (min_w == max_w)
      gst_structure_set (tmp, "width", G_TYPE_INT, max_w, NULL);
    else
      gst_structure_set (tmp, "width", GST_TYPE_INT_RANGE, min_w, max_w, NULL);

    if (min_h == max_h)
      gst_structure_set (tmp, "height", G_TYPE_INT, max_h, NULL);
    else
      gst_structure_set (tmp, "height", GST_TYPE_INT_RANGE, min_h, max_h, NULL);

    gst_v4l2_object_update_and_append (v4l2object, pixelformat, ret, tmp);
    return ret;
  }
}

static gboolean
gst_v4l2_object_get_nearest_size (GstV4l2Object * v4l2object,
    guint32 pixelformat, gint * width, gint * height)
{
  struct v4l2_format fmt;
  gboolean ret = FALSE;

  g_return_val_if_fail (width != NULL, FALSE);
  g_return_val_if_fail (height != NULL, FALSE);

  GST_LOG_OBJECT (v4l2object->dbg_obj,
      "getting nearest size to %dx%d with format %" GST_FOURCC_FORMAT,
      *width, *height, GST_FOURCC_ARGS (pixelformat));

  memset (&fmt, 0, sizeof (struct v4l2_format));

  /* get size delimiters */
  memset (&fmt, 0, sizeof (fmt));
  fmt.type = v4l2object->type;
  fmt.fmt.pix.width = *width;
  fmt.fmt.pix.height = *height;
  fmt.fmt.pix.pixelformat = pixelformat;
  fmt.fmt.pix.field = V4L2_FIELD_ANY;

  if (gst_v4l2_object_try_fmt (v4l2object, &fmt) < 0)
    goto error;

  GST_LOG_OBJECT (v4l2object->dbg_obj,
      "got nearest size %dx%d", fmt.fmt.pix.width, fmt.fmt.pix.height);

  *width = fmt.fmt.pix.width;
  *height = fmt.fmt.pix.height;

  ret = TRUE;

error:
  if (!ret) {
    GST_WARNING_OBJECT (v4l2object->dbg_obj,
        "Unable to try format: %s", g_strerror (errno));
  }

  return ret;
}

static gboolean
gst_v4l2_object_is_dmabuf_supported (GstV4l2Object * v4l2object)
{
  gboolean ret = TRUE;
  struct v4l2_exportbuffer expbuf = {
    .type = v4l2object->type,
    .index = -1,
    .plane = -1,
    .flags = FD_CLOEXEC | O_RDWR,
  };

  if (v4l2object->fmtdesc &&
      v4l2object->fmtdesc->flags & V4L2_FMT_FLAG_EMULATED) {
    GST_WARNING_OBJECT (v4l2object->dbg_obj,
        "libv4l2 converter detected, disabling DMABuf");
    ret = FALSE;
  }

  /* Expected to fail, but ENOTTY tells us that it is not implemented. */
  v4l2object->ioctl (v4l2object->video_fd, VIDIOC_EXPBUF, &expbuf);
  if (errno == ENOTTY)
    ret = FALSE;

  return ret;
}

static gboolean
gst_v4l2_object_setup_pool (GstV4l2Object * v4l2object, GstCaps * caps)
{
  GstV4l2IOMode mode;

  GST_INFO_OBJECT (v4l2object->dbg_obj, "initializing the %s system",
      V4L2_TYPE_IS_OUTPUT (v4l2object->type) ? "output" : "capture");

  GST_V4L2_CHECK_OPEN (v4l2object);
  GST_V4L2_CHECK_NOT_ACTIVE (v4l2object);

  /* hard code */
  mode = GST_V4L2_IO_MMAP;

  /* if still no transport selected, error out */
  if (mode == GST_V4L2_IO_AUTO)
    goto no_supported_capture_method;

  GST_INFO_OBJECT (v4l2object->dbg_obj, "accessing buffers via mode %d", mode);
  v4l2object->mode = mode;

  /* If min_buffers is not set, the driver either does not support the control or
     it has not been asked yet via propose_allocation/decide_allocation. */
  if (!v4l2object->min_buffers)
    gst_v4l2_get_driver_min_buffers (v4l2object);

  /* Map the buffers */
  GST_INFO_OBJECT (v4l2object->dbg_obj, "initial buffer pool");

  {
    GstBufferPool *pool = gst_v4l2_buffer_pool_new (v4l2object, caps);
    GST_OBJECT_LOCK (v4l2object->element);
    v4l2object->pool = pool;
    GST_OBJECT_UNLOCK (v4l2object->element);
    if (!pool)
      goto buffer_pool_new_failed;
  }

  GST_V4L2_SET_ACTIVE (v4l2object);

  GST_INFO_OBJECT (v4l2object->dbg_obj, "set obj active success");

  return TRUE;

  /* ERRORS */
buffer_pool_new_failed:
  {
    GST_ELEMENT_ERROR (v4l2object->element, RESOURCE, READ,
        ("Could not map buffers from device '%s'",
            v4l2object->videodev),
        ("Failed to create buffer pool: %s", g_strerror (errno)));
    return FALSE;
  }
no_supported_capture_method:
  {
    GST_ELEMENT_ERROR (v4l2object->element, RESOURCE, READ,
        ("The driver of device '%s' does not support any known IO "
                "method.", v4l2object->videodev), (NULL));
    return FALSE;
  }
}

static gboolean
gst_v4l2_object_reset_compose_region (GstV4l2Object * obj)
{
  struct v4l2_selection sel = { 0 };

  GST_V4L2_CHECK_OPEN (obj);

  sel.type = obj->type;
  sel.target = V4L2_SEL_TGT_COMPOSE_DEFAULT;

  if (obj->ioctl (obj->video_fd, VIDIOC_G_SELECTION, &sel) < 0) {
    if (errno == ENOTTY) {
      /* No-op when selection API is not supported */
      return TRUE;
    } else {
      GST_WARNING_OBJECT (obj->dbg_obj,
          "Failed to get default compose rectangle with VIDIOC_G_SELECTION: %s",
          g_strerror (errno));
      return FALSE;
    }
  }

  sel.target = V4L2_SEL_TGT_COMPOSE;

  if (obj->ioctl (obj->video_fd, VIDIOC_S_SELECTION, &sel) < 0) {
    GST_WARNING_OBJECT (obj->dbg_obj,
        "Failed to set default compose rectangle with VIDIOC_S_SELECTION: %s",
        g_strerror (errno));
    return FALSE;
  }

  return TRUE;
}

static const gchar *
field_to_str (enum v4l2_field f)
{
  switch (f) {
    case V4L2_FIELD_ANY:
      return "any";
    case V4L2_FIELD_NONE:
      return "none";
    case V4L2_FIELD_TOP:
      return "top";
    case V4L2_FIELD_BOTTOM:
      return "bottom";
    case V4L2_FIELD_INTERLACED:
      return "interlaced";
    case V4L2_FIELD_SEQ_TB:
      return "seq-tb";
    case V4L2_FIELD_SEQ_BT:
      return "seq-bt";
    case V4L2_FIELD_ALTERNATE:
      return "alternate";
    case V4L2_FIELD_INTERLACED_TB:
      return "interlaced-tb";
    case V4L2_FIELD_INTERLACED_BT:
      return "interlaced-bt";
  }

  return "unknown";
}

static guint
calculate_max_sizeimage (GstV4l2Object * v4l2object, guint pixel_bitdepth)
{
  guint max_width, max_height;
  guint sizeimage;

  max_width = v4l2object->max_width;
  max_height = v4l2object->max_height;
  sizeimage = max_width * max_height * pixel_bitdepth / 8 / 2;

  return MAX (ENCODED_BUFFER_MIN_SIZE, sizeimage);
}

static gboolean
gst_v4l2_object_set_format_full (GstV4l2Object * v4l2object, GstCaps * caps,
    gboolean try_only, GstV4l2Error * error)
{
  gint fd = v4l2object->video_fd;
  struct v4l2_format format;
  struct v4l2_streamparm streamparm;
  enum v4l2_field field;
  guint32 pixelformat;
  struct v4l2_fmtdesc *fmtdesc;
  GstVideoInfo info;
  GstVideoFormat gstformat;
  GstVideoAlignment align;
  guint width, height, fps_n, fps_d;
  guint pixel_bitdepth = 8;
  gint n_v4l_planes;
  gint i = 0;
  gboolean is_mplane;
  enum v4l2_colorspace colorspace = 0;
  enum v4l2_quantization range = 0;
  enum v4l2_ycbcr_encoding matrix = 0;
  enum v4l2_xfer_func transfer = 0;
  // GstStructure *s;

  g_return_val_if_fail (!v4l2object->skip_try_fmt_probes ||
      gst_caps_is_writable (caps), FALSE);

  GST_V4L2_CHECK_OPEN (v4l2object);
  if (!try_only)
    GST_V4L2_CHECK_NOT_ACTIVE (v4l2object);

  is_mplane = V4L2_TYPE_IS_MULTIPLANAR (v4l2object->type);

  gst_video_info_init (&info);
  gst_video_alignment_reset (&align);
  v4l2object->transfer = GST_VIDEO_TRANSFER_UNKNOWN;

  if (!gst_v4l2_object_get_caps_info (v4l2object, caps, &fmtdesc, &info))
    goto invalid_caps;

  pixelformat = fmtdesc->pixelformat;
  width = GST_VIDEO_INFO_WIDTH (&info);
  height = GST_VIDEO_INFO_FIELD_HEIGHT (&info);
  /* if caps has no width and height info, use default value */
  if (V4L2_TYPE_IS_OUTPUT (v4l2object->type) && width == 0 && height == 0) {
    width = GST_V4L2_DEFAULT_WIDTH;
    height = GST_V4L2_DEFAULT_HEIGHT;
  }
  fps_n = GST_VIDEO_INFO_FPS_N (&info);
  fps_d = GST_VIDEO_INFO_FPS_D (&info);

  gstformat = gst_v4l2_object_v4l2fourcc_to_video_format (pixelformat);
  gst_video_info_set_format (&v4l2object->info, gstformat, width, height);
  GST_INFO_OBJECT (v4l2object->dbg_obj, "set w_%d, h_%d format_%d",
    width, height, gstformat);

  /* if encoded format (GST_VIDEO_INFO_N_PLANES return 0)
   * or if contiguous is preferred */
  n_v4l_planes = GST_VIDEO_INFO_N_PLANES (&info);
  if (!n_v4l_planes || !v4l2object->prefered_non_contiguous)
    n_v4l_planes = 1;

  // field = get_v4l2_field_for_info (&info);
  // if (field != V4L2_FIELD_NONE)
  //   GST_DEBUG_OBJECT (v4l2object->dbg_obj, "interlaced video");
  // else
  //   GST_DEBUG_OBJECT (v4l2object->dbg_obj, "progressive video");

  // GST_DEBUG_OBJECT (v4l2object->dbg_obj, "Desired format %dx%d, format "
  //     "%" GST_FOURCC_FORMAT " stride: %d", width, height,
  //     GST_FOURCC_ARGS (pixelformat), GST_VIDEO_INFO_PLANE_STRIDE (&info, 0));

  // s = gst_caps_get_structure (caps, 0);

  // if (gst_structure_has_field (s, "bit-depth-chroma")) {
  //   gst_structure_get_uint (s, "bit-depth-chroma", &pixel_bitdepth);
  //   GST_DEBUG_OBJECT (v4l2object->element, "Got pixel bit depth %u from caps",
  //       pixel_bitdepth);
  // }

  memset (&format, 0x00, sizeof (struct v4l2_format));
  format.type = v4l2object->type;

  if (is_mplane) {
    format.type = v4l2object->type;
    format.fmt.pix_mp.pixelformat = pixelformat;
    format.fmt.pix_mp.width = width;
    format.fmt.pix_mp.height = height;
    // format.fmt.pix_mp.field = field;
    format.fmt.pix_mp.num_planes = n_v4l_planes;

    /* try to ask our preferred stride but it's not a failure if not
     * accepted */
    for (i = 0; i < n_v4l_planes; i++) {
      gint stride = GST_VIDEO_INFO_PLANE_STRIDE (&info, i);

      format.fmt.pix_mp.plane_fmt[i].bytesperline = stride;
    }

    if (GST_VIDEO_INFO_FORMAT (&info) == GST_VIDEO_FORMAT_ENCODED)
      format.fmt.pix_mp.plane_fmt[0].sizeimage =
          calculate_max_sizeimage (v4l2object, pixel_bitdepth);
  } else {
    gint stride = GST_VIDEO_INFO_PLANE_STRIDE (&info, 0);

    format.type = v4l2object->type;

    format.fmt.pix.width = width;
    format.fmt.pix.height = height;
    format.fmt.pix.pixelformat = pixelformat;
    format.fmt.pix.field = field;

    /* try to ask our preferred stride */
    format.fmt.pix.bytesperline = stride;

    if (GST_VIDEO_INFO_FORMAT (&info) == GST_VIDEO_FORMAT_ENCODED)
      format.fmt.pix.sizeimage =
          calculate_max_sizeimage (v4l2object, pixel_bitdepth);
  }

  GST_DEBUG_OBJECT (v4l2object->dbg_obj, "Desired format is %dx%d, format "
      "%" GST_FOURCC_FORMAT ", nb planes %d", format.fmt.pix.width,
      format.fmt.pix_mp.height,
      GST_FOURCC_ARGS (format.fmt.pix.pixelformat),
      is_mplane ? format.fmt.pix_mp.num_planes : 1);

#ifndef GST_DISABLE_GST_DEBUG
  if (is_mplane) {
    for (i = 0; i < format.fmt.pix_mp.num_planes; i++)
      GST_DEBUG_OBJECT (v4l2object->dbg_obj, "  stride %d",
          format.fmt.pix_mp.plane_fmt[i].bytesperline);
  } else {
    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "  stride %d",
        format.fmt.pix.bytesperline);
  }
#endif

  if (is_mplane) {
    format.fmt.pix_mp.colorspace = colorspace;
    format.fmt.pix_mp.quantization = range;
    format.fmt.pix_mp.ycbcr_enc = matrix;
    format.fmt.pix_mp.xfer_func = transfer;
  } else {
    format.fmt.pix.priv = V4L2_PIX_FMT_PRIV_MAGIC;
    format.fmt.pix.colorspace = colorspace;
    format.fmt.pix.quantization = range;
    format.fmt.pix.ycbcr_enc = matrix;
    format.fmt.pix.xfer_func = transfer;
  }

  GST_DEBUG_OBJECT (v4l2object->dbg_obj, "Desired colorspace is %d:%d:%d:%d",
      colorspace, range, matrix, transfer);

  if (try_only) {
    if (v4l2object->ioctl (fd, VIDIOC_TRY_FMT, &format) < 0)
      goto try_fmt_failed;
  } else {
    if (v4l2object->ioctl (fd, VIDIOC_S_FMT, &format) < 0)
      goto set_fmt_failed;
  }

  if (is_mplane) {
    colorspace = format.fmt.pix_mp.colorspace;
    range = format.fmt.pix_mp.quantization;
    matrix = format.fmt.pix_mp.ycbcr_enc;
    transfer = format.fmt.pix_mp.xfer_func;
  } else {
    colorspace = format.fmt.pix.colorspace;
    range = format.fmt.pix.quantization;
    matrix = format.fmt.pix.ycbcr_enc;
    transfer = format.fmt.pix.xfer_func;
  }

  GST_DEBUG_OBJECT (v4l2object->dbg_obj, "Got format of %dx%d, format "
      "%" GST_FOURCC_FORMAT ", nb planes %d, colorspace %d:%d:%d:%d field: %s",
      format.fmt.pix.width, format.fmt.pix_mp.height,
      GST_FOURCC_ARGS (format.fmt.pix.pixelformat),
      is_mplane ? format.fmt.pix_mp.num_planes : 1,
      colorspace, range, matrix, transfer, field_to_str (format.fmt.pix.field));

#ifndef GST_DISABLE_GST_DEBUG
  if (is_mplane) {
    for (i = 0; i < format.fmt.pix_mp.num_planes; i++)
      GST_DEBUG_OBJECT (v4l2object->dbg_obj, "  stride %d, sizeimage %d",
          format.fmt.pix_mp.plane_fmt[i].bytesperline,
          format.fmt.pix_mp.plane_fmt[i].sizeimage);
  } else {
    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "  stride %d, sizeimage %d",
        format.fmt.pix.bytesperline, format.fmt.pix.sizeimage);
  }
#endif

  if (format.fmt.pix.pixelformat != pixelformat)
    goto invalid_pixelformat;

  /* Only negotiate size with raw data.
   * For some codecs the dimensions are *not* in the bitstream, IIRC VC1
   * in ASF mode for example, there is also not reason for a driver to
   * change the size. */
  if (info.finfo->format != GST_VIDEO_FORMAT_ENCODED) {
    /* We can crop larger images */
    if (format.fmt.pix.width < width || format.fmt.pix.height < height)
      goto invalid_dimensions;

    /* Note, this will be adjusted if upstream has non-centered cropping. */
    align.padding_top = 0;
    align.padding_bottom = format.fmt.pix.height - height;
    align.padding_left = 0;
    align.padding_right = format.fmt.pix.width - width;
  }

  if (is_mplane && format.fmt.pix_mp.num_planes != n_v4l_planes)
    goto invalid_planes;

  if (try_only)                 /* good enough for trying only */
    return TRUE;

  /* Is there a reason we require the caller to always specify a framerate? */
  GST_DEBUG_OBJECT (v4l2object->dbg_obj, "Desired framerate: %u/%u", fps_n,
      fps_d);

  memset (&streamparm, 0x00, sizeof (struct v4l2_streamparm));
  streamparm.type = v4l2object->type;

  if (v4l2object->ioctl (fd, VIDIOC_G_PARM, &streamparm) < 0)
    goto get_parm_failed;

  if (v4l2object->type == V4L2_BUF_TYPE_VIDEO_CAPTURE
      || v4l2object->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
    GST_VIDEO_INFO_FPS_N (&info) =
        streamparm.parm.capture.timeperframe.denominator;
    GST_VIDEO_INFO_FPS_D (&info) =
        streamparm.parm.capture.timeperframe.numerator;

    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "Got capture framerate: %u/%u",
        streamparm.parm.capture.timeperframe.denominator,
        streamparm.parm.capture.timeperframe.numerator);

    /* We used to skip frame rate setup if the camera was already setup
     * with the requested frame rate. This breaks some cameras though,
     * causing them to not output data (several models of Thinkpad cameras
     * have this problem at least).
     * So, don't skip. */
    GST_LOG_OBJECT (v4l2object->dbg_obj, "Setting capture framerate to %u/%u",
        fps_n, fps_d);
    /* We want to change the frame rate, so check whether we can. Some cheap USB
     * cameras don't have the capability */
    if ((streamparm.parm.capture.capability & V4L2_CAP_TIMEPERFRAME) == 0) {
      GST_DEBUG_OBJECT (v4l2object->dbg_obj,
          "Not setting capture framerate (not supported)");
      goto done;
    }

    /* Note: V4L2 wants the frame interval, we have the frame rate */
    streamparm.parm.capture.timeperframe.numerator = fps_d;
    streamparm.parm.capture.timeperframe.denominator = fps_n;

    /* some cheap USB cam's won't accept any change */
    if (v4l2object->ioctl (fd, VIDIOC_S_PARM, &streamparm) < 0)
      goto set_parm_failed;

    if (streamparm.parm.capture.timeperframe.numerator > 0 &&
        streamparm.parm.capture.timeperframe.denominator > 0) {
      /* get new values */
      fps_d = streamparm.parm.capture.timeperframe.numerator;
      fps_n = streamparm.parm.capture.timeperframe.denominator;

      GST_INFO_OBJECT (v4l2object->dbg_obj, "Set capture framerate to %u/%u",
          fps_n, fps_d);
    } else {
      /* fix v4l2 capture driver to provide framerate values */
      GST_WARNING_OBJECT (v4l2object->dbg_obj,
          "Reuse caps framerate %u/%u - fix v4l2 capture driver", fps_n, fps_d);
    }

    GST_VIDEO_INFO_FPS_N (&info) = fps_n;
    GST_VIDEO_INFO_FPS_D (&info) = fps_d;
  } else if (v4l2object->type == V4L2_BUF_TYPE_VIDEO_OUTPUT
      || v4l2object->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
    GST_VIDEO_INFO_FPS_N (&info) =
        streamparm.parm.output.timeperframe.denominator;
    GST_VIDEO_INFO_FPS_D (&info) =
        streamparm.parm.output.timeperframe.numerator;

    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "Got output framerate: %u/%u",
        streamparm.parm.output.timeperframe.denominator,
        streamparm.parm.output.timeperframe.numerator);

    GST_LOG_OBJECT (v4l2object->dbg_obj, "Setting output framerate to %u/%u",
        fps_n, fps_d);
    if ((streamparm.parm.output.capability & V4L2_CAP_TIMEPERFRAME) == 0) {
      GST_DEBUG_OBJECT (v4l2object->dbg_obj,
          "Not setting output framerate (not supported)");
      goto done;
    }

    /* Note: V4L2 wants the frame interval, we have the frame rate */
    streamparm.parm.output.timeperframe.numerator = fps_d;
    streamparm.parm.output.timeperframe.denominator = fps_n;

    if (v4l2object->ioctl (fd, VIDIOC_S_PARM, &streamparm) < 0)
      goto set_parm_failed;

    if (streamparm.parm.output.timeperframe.numerator > 0 &&
        streamparm.parm.output.timeperframe.denominator > 0) {
      /* get new values */
      fps_d = streamparm.parm.output.timeperframe.numerator;
      fps_n = streamparm.parm.output.timeperframe.denominator;

      GST_INFO_OBJECT (v4l2object->dbg_obj, "Set output framerate to %u/%u",
          fps_n, fps_d);
    } else {
      /* fix v4l2 output driver to provide framerate values */
      GST_WARNING_OBJECT (v4l2object->dbg_obj,
          "Reuse caps framerate %u/%u - fix v4l2 output driver", fps_n, fps_d);
    }

    GST_VIDEO_INFO_FPS_N (&info) = fps_n;
    GST_VIDEO_INFO_FPS_D (&info) = fps_d;
  }

done:
  /* add boolean return, so we can fail on drivers bugs */
  //TODO
  // gst_v4l2_object_save_format (v4l2object, fmtdesc, &format, &info, &align);

  /* reset composition region to match the S_FMT size */
  gst_v4l2_object_reset_compose_region (v4l2object);

  /* now configure the pool */
  if (!gst_v4l2_object_setup_pool (v4l2object, caps))
    goto pool_failed;

  return TRUE;

  /* ERRORS */
invalid_caps:
  {
    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "can't parse caps %" GST_PTR_FORMAT,
        caps);

    GST_V4L2_ERROR (error, RESOURCE, SETTINGS,
        ("Invalid caps"), ("Can't parse caps %" GST_PTR_FORMAT, caps));
    return FALSE;
  }
try_fmt_failed:
  {
    if (errno == EINVAL) {
      GST_V4L2_ERROR (error, RESOURCE, SETTINGS,
          ("Device '%s' has no supported format", v4l2object->videodev),
          ("Call to TRY_FMT failed for %" GST_FOURCC_FORMAT " @ %dx%d: %s",
              GST_FOURCC_ARGS (pixelformat), width, height,
              g_strerror (errno)));
    } else {
      GST_V4L2_ERROR (error, RESOURCE, FAILED,
          ("Device '%s' failed during initialization",
              v4l2object->videodev),
          ("Call to TRY_FMT failed for %" GST_FOURCC_FORMAT " @ %dx%d: %s",
              GST_FOURCC_ARGS (pixelformat), width, height,
              g_strerror (errno)));
    }
    return FALSE;
  }
set_fmt_failed:
  {
    if (errno == EBUSY) {
      GST_V4L2_ERROR (error, RESOURCE, BUSY,
          ("Device '%s' is busy", v4l2object->videodev),
          ("Call to S_FMT failed for %" GST_FOURCC_FORMAT " @ %dx%d: %s",
              GST_FOURCC_ARGS (pixelformat), width, height,
              g_strerror (errno)));
    } else if (errno == EINVAL) {
      GST_V4L2_ERROR (error, RESOURCE, SETTINGS,
          ("Device '%s' has no supported format", v4l2object->videodev),
          ("Call to S_FMT failed for %" GST_FOURCC_FORMAT " @ %dx%d: %s",
              GST_FOURCC_ARGS (pixelformat), width, height,
              g_strerror (errno)));
    } else {
      GST_V4L2_ERROR (error, RESOURCE, FAILED,
          ("Device '%s' failed during initialization",
              v4l2object->videodev),
          ("Call to S_FMT failed for %" GST_FOURCC_FORMAT " @ %dx%d: %s",
              GST_FOURCC_ARGS (pixelformat), width, height,
              g_strerror (errno)));
    }
    return FALSE;
  }
invalid_dimensions:
  {
    GST_V4L2_ERROR (error, RESOURCE, SETTINGS,
        ("Device '%s' cannot capture at %dx%d",
            v4l2object->videodev, width, height),
        ("Tried to capture at %dx%d, but device returned size %dx%d",
            width, height, format.fmt.pix.width, format.fmt.pix.height));
    return FALSE;
  }
invalid_pixelformat:
  {
    GST_V4L2_ERROR (error, RESOURCE, SETTINGS,
        ("Device '%s' cannot capture in the specified format",
            v4l2object->videodev),
        ("Tried to capture in %" GST_FOURCC_FORMAT
            ", but device returned format" " %" GST_FOURCC_FORMAT,
            GST_FOURCC_ARGS (pixelformat),
            GST_FOURCC_ARGS (format.fmt.pix.pixelformat)));
    return FALSE;
  }
invalid_planes:
  {
    GST_V4L2_ERROR (error, RESOURCE, SETTINGS,
        ("Device '%s' does support non-contiguous planes",
            v4l2object->videodev),
        ("Device wants %d planes", format.fmt.pix_mp.num_planes));
    return FALSE;
  }
get_parm_failed:
  {
    /* it's possible that this call is not supported */
    if (errno != EINVAL && errno != ENOTTY) {
      GST_V4L2_ERROR (error, RESOURCE, SETTINGS,
          ("Could not get parameters on device '%s'",
              v4l2object->videodev), GST_ERROR_SYSTEM);
    }
    goto done;
  }
set_parm_failed:
  {
    GST_V4L2_ERROR (error, RESOURCE, SETTINGS,
        ("Video device did not accept new frame rate setting."),
        GST_ERROR_SYSTEM);
    goto done;
  }
pool_failed:
  {
    /* setup_pool already send the error */
    return FALSE;
  }
}

gboolean
gst_v4l2_object_set_format (GstV4l2Object * v4l2object, GstCaps * caps,
    GstV4l2Error * error)
{
  GST_INFO_OBJECT (v4l2object->dbg_obj, "Setting format to %" GST_PTR_FORMAT,
      caps);
  return gst_v4l2_object_set_format_full (v4l2object, caps, FALSE, error);
}

gboolean
gst_v4l2_object_try_format (GstV4l2Object * v4l2object, GstCaps * caps,
    GstV4l2Error * error)
{
  GST_DEBUG_OBJECT (v4l2object->dbg_obj, "Trying format %" GST_PTR_FORMAT,
      caps);
  return gst_v4l2_object_set_format_full (v4l2object, caps, TRUE, error);
}

/**
 * gst_v4l2_object_acquire_format:
 * @v4l2object: the object
 * @info: a GstVideoInfo to be filled
 *
 * Acquire the driver chosen format. This is useful in decoder or encoder elements where
 * the output format is chosen by the HW.
 *
 * Returns: %TRUE on success, %FALSE on failure.
 */
gboolean
gst_v4l2_object_acquire_format (GstV4l2Object * v4l2object, GstVideoInfo * info)
{
  struct v4l2_fmtdesc *fmtdesc;
  struct v4l2_format fmt;
  struct v4l2_crop crop;
  struct v4l2_selection sel;
  struct v4l2_rect *r = NULL;
  GstVideoFormat format;
  guint width, height;
  GstVideoAlignment align;

  gst_video_info_init (info);
  gst_video_alignment_reset (&align);
  v4l2object->transfer = GST_VIDEO_TRANSFER_UNKNOWN;

  memset (&fmt, 0x00, sizeof (struct v4l2_format));
  fmt.type = v4l2object->type;
  if (v4l2object->ioctl (v4l2object->video_fd, VIDIOC_G_FMT, &fmt) < 0)
    goto get_fmt_failed;

  fmtdesc = gst_v4l2_object_get_format_from_fourcc (v4l2object,
      fmt.fmt.pix.pixelformat);
  if (fmtdesc == NULL)
    goto unsupported_format;

  /* No need to care about mplane, the four first params are the same */
  format = gst_v4l2_object_v4l2fourcc_to_video_format (fmt.fmt.pix.pixelformat);

  /* fails if we do no translate the fmt.pix.pixelformat to GstVideoFormat */
  if (format == GST_VIDEO_FORMAT_UNKNOWN)
    goto unsupported_format;

  if (fmt.fmt.pix.width == 0 || fmt.fmt.pix.height == 0)
    goto invalid_dimensions;

  width = fmt.fmt.pix.width;
  height = fmt.fmt.pix.height;

  /* Use the default compose rectangle */
  memset (&sel, 0, sizeof (struct v4l2_selection));
  sel.type = v4l2object->type;
  sel.target = V4L2_SEL_TGT_COMPOSE_DEFAULT;
  if (v4l2object->ioctl (v4l2object->video_fd, VIDIOC_G_SELECTION, &sel) >= 0) {
    r = &sel.r;
  } else {
    /* For ancient kernels, fall back to G_CROP */
    memset (&crop, 0, sizeof (struct v4l2_crop));
    crop.type = v4l2object->type;
    if (v4l2object->ioctl (v4l2object->video_fd, VIDIOC_G_CROP, &crop) >= 0)
      r = &crop.c;
  }
  if (r) {
    align.padding_left = r->left;
    align.padding_top = r->top;
    align.padding_right = width - r->width - r->left;
    align.padding_bottom = height - r->height - r->top;
    width = r->width;
    height = r->height;
  }

  gst_v4l2_object_get_streamparm (v4l2object, info);
  if ((info->fps_n == 0 && v4l2object->info.fps_d != 0)
      && (v4l2object->type == V4L2_BUF_TYPE_VIDEO_CAPTURE
          || v4l2object->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)) {
    info->fps_d = v4l2object->info.fps_d;
    info->fps_n = v4l2object->info.fps_n;
    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "Set capture fps to %d/%d",
        info->fps_n, info->fps_d);
  }

  // gst_v4l2_object_save_format (v4l2object, fmtdesc, &fmt, info, &align);

  /* Shall we setup the pool ? */

  return TRUE;

get_fmt_failed:
  {
    GST_ELEMENT_WARNING (v4l2object->element, RESOURCE, SETTINGS,
        ("Video device did not provide output format."), GST_ERROR_SYSTEM);
    return FALSE;
  }
invalid_dimensions:
  {
    GST_ELEMENT_WARNING (v4l2object->element, RESOURCE, SETTINGS,
        ("Video device returned invalid dimensions."),
        ("Expected non 0 dimensions, got %dx%d", fmt.fmt.pix.width,
            fmt.fmt.pix.height));
    return FALSE;
  }
unsupported_format:
  {
    GST_ELEMENT_ERROR (v4l2object->element, RESOURCE, SETTINGS,
        ("Video device uses an unsupported pixel format."),
        ("V4L2 format %" GST_FOURCC_FORMAT " not supported",
            GST_FOURCC_ARGS (fmt.fmt.pix.pixelformat)));
    return FALSE;
  }
}

/**
 * gst_v4l2_object_set_crop:
 * @obj: the object
 * @crop_rect: the region to crop
 *
 * Crop the video data to the regions specified in the @crop_rect.
 *
 * For capture devices, this crop the image sensor / video stream provided by
 * the V4L2 device.
 * For output devices, this crops the memory buffer that GStreamer passed to
 * the V4L2 device.
 *
 * The crop_rect may be modified by the V4L2 device to a region that
 * fulfills H/W requirements.
 *
 * Returns: %TRUE on success, %FALSE on failure.
 */
gboolean
gst_v4l2_object_set_crop (GstV4l2Object * obj, struct v4l2_rect *crop_rect)
{
  struct v4l2_selection sel = { 0 };
  struct v4l2_crop crop = { 0 };

  GST_V4L2_CHECK_OPEN (obj);

  sel.type = obj->type;
  sel.target = V4L2_SEL_TGT_CROP;
  sel.flags = 0;
  sel.r = *crop_rect;

  crop.type = obj->type;
  crop.c = sel.r;

  GST_DEBUG_OBJECT (obj->dbg_obj,
      "Desired cropping left %u, top %u, size %ux%u", crop.c.left, crop.c.top,
      crop.c.width, crop.c.height);

  if (obj->ioctl (obj->video_fd, VIDIOC_S_SELECTION, &sel) < 0) {
    if (errno != ENOTTY) {
      GST_WARNING_OBJECT (obj->dbg_obj,
          "Failed to set crop rectangle with VIDIOC_S_SELECTION: %s",
          g_strerror (errno));
      return FALSE;
    } else {
      if (obj->ioctl (obj->video_fd, VIDIOC_S_CROP, &crop) < 0) {
        GST_WARNING_OBJECT (obj->dbg_obj, "VIDIOC_S_CROP failed");
        return FALSE;
      }

      if (obj->ioctl (obj->video_fd, VIDIOC_G_CROP, &crop) < 0) {
        GST_WARNING_OBJECT (obj->dbg_obj, "VIDIOC_G_CROP failed");
        return FALSE;
      }

      sel.r = crop.c;
    }
  }

  GST_DEBUG_OBJECT (obj->dbg_obj,
      "Got cropping left %u, top %u, size %ux%u", crop.c.left, crop.c.top,
      crop.c.width, crop.c.height);

  return TRUE;
}

/**
 * gst_v4l2_object_setup_padding:
 * @obj: v4l2 object
 *
 * Crop away the padding around the video data as specified
 * in GstVideoAlignement data stored in @obj.
 *
 * For capture devices, this crop the image sensor / video stream provided by
 * the V4L2 device.
 * For output devices, this crops the memory buffer that GStreamer passed to
 * the V4L2 device.
 *
 * Returns: %TRUE on success, %FALSE on failure.
 */
gboolean
gst_v4l2_object_setup_padding (GstV4l2Object * obj)
{
  GstVideoAlignment *align = &obj->align;
  struct v4l2_rect crop;

  if (align->padding_left + align->padding_top
      + align->padding_right + align->padding_bottom == 0) {
    GST_DEBUG_OBJECT (obj->dbg_obj, "no cropping needed");
    return TRUE;
  }

  crop.left = align->padding_left;
  crop.top = align->padding_top;
  crop.width = obj->info.width;
  crop.height = GST_VIDEO_INFO_FIELD_HEIGHT (&obj->info);

  return gst_v4l2_object_set_crop (obj, &crop);
}

static gboolean
gst_v4l2_object_get_crop_rect (GstV4l2Object * obj, guint target,
    struct v4l2_rect *result)
{
  struct v4l2_rect *res_rect;

  struct v4l2_selection sel = { 0 };
  struct v4l2_cropcap cropcap = { 0 };

  GST_V4L2_CHECK_OPEN (obj);

  if (target != V4L2_SEL_TGT_CROP_BOUNDS && target != V4L2_SEL_TGT_CROP_DEFAULT)
    return FALSE;

  sel.type = obj->type;
  sel.target = target;

  res_rect = &sel.r;

  if (obj->ioctl (obj->video_fd, VIDIOC_G_SELECTION, &sel) < 0) {
    if (errno != ENOTTY) {
      GST_WARNING_OBJECT (obj->dbg_obj,
          "Failed to get default crop rectangle with VIDIOC_G_SELECTION: %s",
          g_strerror (errno));
      return FALSE;
    } else {
      if (obj->ioctl (obj->video_fd, VIDIOC_CROPCAP, &cropcap) < 0) {
        GST_WARNING_OBJECT (obj->dbg_obj, "VIDIOC_CROPCAP failed");
        return FALSE;
      }
      if (target == V4L2_SEL_TGT_CROP_BOUNDS)
        res_rect = &cropcap.bounds;
      else if (target == V4L2_SEL_TGT_CROP_DEFAULT)
        res_rect = &cropcap.defrect;
    }
  }

  *result = *res_rect;
  return TRUE;
}

gboolean
gst_v4l2_object_get_crop_bounds (GstV4l2Object * obj, struct v4l2_rect *result)
{
  return gst_v4l2_object_get_crop_rect (obj, V4L2_SEL_TGT_CROP_BOUNDS, result);
}

gboolean
gst_v4l2_object_get_crop_default (GstV4l2Object * obj, struct v4l2_rect *result)
{
  return gst_v4l2_object_get_crop_rect (obj, V4L2_SEL_TGT_CROP_DEFAULT, result);
}

gboolean
gst_v4l2_object_caps_equal (GstV4l2Object * v4l2object, GstCaps * caps)
{
  GstStructure *config;
  GstCaps *oldcaps;
  gboolean ret;
  GstBufferPool *pool = gst_v4l2_object_get_buffer_pool (v4l2object);

  if (!pool)
    return FALSE;

  config = gst_buffer_pool_get_config (pool);
  gst_buffer_pool_config_get_params (config, &oldcaps, NULL, NULL, NULL);

  ret = oldcaps && gst_caps_is_equal (caps, oldcaps);

  gst_structure_free (config);

  gst_object_unref (pool);
  return ret;
}

gboolean
gst_v4l2_object_caps_is_subset (GstV4l2Object * v4l2object, GstCaps * caps)
{
  GstStructure *config;
  GstCaps *oldcaps;
  gboolean ret;
  GstBufferPool *pool = gst_v4l2_object_get_buffer_pool (v4l2object);

  if (!pool)
    return FALSE;

  config = gst_buffer_pool_get_config (pool);
  gst_buffer_pool_config_get_params (config, &oldcaps, NULL, NULL, NULL);

  ret = oldcaps && gst_caps_is_subset (oldcaps, caps);

  gst_structure_free (config);

  gst_object_unref (pool);
  return ret;
}

GstCaps *
gst_v4l2_object_get_current_caps (GstV4l2Object * v4l2object)
{
  GstStructure *config;
  GstCaps *oldcaps;
  GstBufferPool *pool = gst_v4l2_object_get_buffer_pool (v4l2object);

  if (!pool)
    return NULL;

  config = gst_buffer_pool_get_config (pool);
  gst_buffer_pool_config_get_params (config, &oldcaps, NULL, NULL, NULL);

  if (oldcaps)
    gst_caps_ref (oldcaps);

  gst_structure_free (config);

  gst_object_unref (pool);
  return oldcaps;
}

gboolean
gst_v4l2_object_unlock (GstV4l2Object * v4l2object)
{
  gboolean ret = TRUE;
  GstBufferPool *pool = gst_v4l2_object_get_buffer_pool (v4l2object);

  GST_LOG_OBJECT (v4l2object->dbg_obj, "start flushing");

  gst_poll_set_flushing (v4l2object->poll, TRUE);

  if (!pool)
    return ret;

  if (gst_buffer_pool_is_active (pool))
    gst_buffer_pool_set_flushing (pool, TRUE);

  gst_object_unref (pool);
  return ret;
}

gboolean
gst_v4l2_object_unlock_stop (GstV4l2Object * v4l2object)
{
  gboolean ret = TRUE;
  GstBufferPool *pool = gst_v4l2_object_get_buffer_pool (v4l2object);

  GST_LOG_OBJECT (v4l2object->dbg_obj, "stop flushing");

  gst_poll_set_flushing (v4l2object->poll, FALSE);

  if (!pool)
    return ret;

  if (gst_buffer_pool_is_active (pool))
    gst_buffer_pool_set_flushing (pool, FALSE);

  gst_object_unref (pool);
  return ret;
}

gboolean
gst_v4l2_object_stop (GstV4l2Object * v4l2object)
{
  GstBufferPool *pool;
  GST_DEBUG_OBJECT (v4l2object->dbg_obj, "stopping");

  if (!GST_V4L2_IS_OPEN (v4l2object))
    goto done;
  if (!GST_V4L2_IS_ACTIVE (v4l2object))
    goto done;

  gst_poll_set_flushing (v4l2object->poll, TRUE);

  pool = gst_v4l2_object_get_buffer_pool (v4l2object);
  if (pool)
    gst_object_unref (pool);

  GST_V4L2_SET_INACTIVE (v4l2object);

done:
  return TRUE;
}

GstCaps *
gst_v4l2_object_probe_caps (GstV4l2Object * v4l2object, GstCaps * filter)
{
  GstCaps *ret;
  GSList *walk;
  GSList *formats;
  guint32 fourcc = 0;

  if (v4l2object->fmtdesc)
    fourcc = GST_V4L2_PIXELFORMAT (v4l2object);

  gst_v4l2_object_clear_format_list (v4l2object);
  formats = gst_v4l2_object_get_format_list (v4l2object);

  /* Recover the fmtdesc, it may no longer exist, in which case it will be set
   * to null */
  if (fourcc)
    v4l2object->fmtdesc =
        gst_v4l2_object_get_format_from_fourcc (v4l2object, fourcc);

  ret = gst_caps_new_empty ();

  if (v4l2object->keep_aspect && !v4l2object->par) {
    struct v4l2_cropcap cropcap;

    memset (&cropcap, 0, sizeof (cropcap));

    cropcap.type = v4l2object->type;
    if (v4l2object->ioctl (v4l2object->video_fd, VIDIOC_CROPCAP, &cropcap) < 0) {

      switch (errno) {
        case ENODATA:
        case ENOTTY:
          GST_INFO_OBJECT (v4l2object->dbg_obj,
              "Driver does not support VIDIOC_CROPCAP (%s), assuming pixel aspect ratio 1/1",
              g_strerror (errno));
          break;

        default:
          GST_WARNING_OBJECT (v4l2object->dbg_obj,
              "Failed to probe pixel aspect ratio with VIDIOC_CROPCAP: %s",
              g_strerror (errno));
      }
      v4l2object->par = g_new0 (GValue, 1);
      g_value_init (v4l2object->par, GST_TYPE_FRACTION);
      gst_value_set_fraction (v4l2object->par, 1, 1);

    } else if (cropcap.pixelaspect.numerator && cropcap.pixelaspect.denominator) {
      v4l2object->par = g_new0 (GValue, 1);
      g_value_init (v4l2object->par, GST_TYPE_FRACTION);
      gst_value_set_fraction (v4l2object->par, cropcap.pixelaspect.numerator,
          cropcap.pixelaspect.denominator);
    }
  }

  for (walk = formats; walk; walk = walk->next) {
    struct v4l2_fmtdesc *format;
    GstStructure *template;
    GstCaps *tmp;

    format = (struct v4l2_fmtdesc *) walk->data;

    template = gst_v4l2_object_v4l2fourcc_to_bare_struct (format->pixelformat);

    if (!template) {
      GST_DEBUG_OBJECT (v4l2object->dbg_obj,
          "unknown format %" GST_FOURCC_FORMAT,
          GST_FOURCC_ARGS (format->pixelformat));
      continue;
    }

    /* If we have a filter, check if we need to probe this format or not */
    if (filter) {
      GstCaps *format_caps = gst_caps_new_empty ();

      gst_caps_append_structure (format_caps, gst_structure_copy (template));

      if (!gst_caps_can_intersect (format_caps, filter)) {
        gst_caps_unref (format_caps);
        gst_structure_free (template);
        continue;
      }

      gst_caps_unref (format_caps);
    }

    tmp = gst_v4l2_object_probe_caps_for_format (v4l2object,
        format->pixelformat, template);
    if (tmp) {
      gst_caps_append (ret, tmp);

      /* Add a variant of the caps with the Interlaced feature so we can negotiate it if needed */
      add_alternate_variant (v4l2object, ret, gst_caps_get_structure (ret,
              gst_caps_get_size (ret) - 1));
    }

    gst_structure_free (template);
  }

  if (filter) {
    GstCaps *tmp;

    tmp = ret;
    ret = gst_caps_intersect_full (filter, ret, GST_CAPS_INTERSECT_FIRST);
    gst_caps_unref (tmp);
  }

  GST_INFO_OBJECT (v4l2object->dbg_obj, "probed caps: %" GST_PTR_FORMAT, ret);

  return ret;
}

GstCaps *
gst_v4l2_object_get_caps (GstV4l2Object * v4l2object, GstCaps * filter)
{
  GstCaps *ret;

  if (v4l2object->probed_caps == NULL)
    v4l2object->probed_caps = gst_v4l2_object_probe_caps (v4l2object, NULL);

  if (filter) {
    ret = gst_caps_intersect_full (filter, v4l2object->probed_caps,
        GST_CAPS_INTERSECT_FIRST);
  } else {
    ret = gst_caps_ref (v4l2object->probed_caps);
  }

  return ret;
}

static gboolean
gst_v4l2_object_match_buffer_layout (GstV4l2Object * obj, guint n_planes,
    gsize offset[GST_VIDEO_MAX_PLANES], gint stride[GST_VIDEO_MAX_PLANES],
    gsize buffer_size, guint padded_height)
{
  guint p;
  gboolean need_fmt_update = FALSE;

  GST_DEBUG_OBJECT (obj->dbg_obj, "buffer size %lu", buffer_size);

  if (n_planes != GST_VIDEO_INFO_N_PLANES (&obj->info)) {
    GST_WARNING_OBJECT (obj->dbg_obj,
        "Cannot match buffers with different number planes");
    return FALSE;
  }

  for (p = 0; p < n_planes; p++) {
    if (stride[p] < obj->info.stride[p]) {
      GST_DEBUG_OBJECT (obj->dbg_obj,
          "Not matching as remote stride %i is smaller than %i on plane %u",
          stride[p], obj->info.stride[p], p);
      return FALSE;
    } else if (stride[p] > obj->info.stride[p]) {
      GST_LOG_OBJECT (obj->dbg_obj,
          "remote stride %i is higher than %i on plane %u",
          stride[p], obj->info.stride[p], p);
      need_fmt_update = TRUE;
    }

    if (offset[p] < obj->info.offset[p]) {
      GST_DEBUG_OBJECT (obj->dbg_obj,
          "Not matching as offset %" G_GSIZE_FORMAT
          " is smaller than %" G_GSIZE_FORMAT " on plane %u",
          offset[p], obj->info.offset[p], p);
      return FALSE;
    } else if (offset[p] > obj->info.offset[p]) {
      GST_LOG_OBJECT (obj->dbg_obj,
          "Remote offset %" G_GSIZE_FORMAT
          " is higher than %" G_GSIZE_FORMAT " on plane %u",
          offset[p], obj->info.offset[p], p);
      need_fmt_update = TRUE;
    }

    if (padded_height) {
      guint fmt_height;

      if (V4L2_TYPE_IS_MULTIPLANAR (obj->type))
        fmt_height = obj->format.fmt.pix_mp.height;
      else
        fmt_height = obj->format.fmt.pix.height;

      if (padded_height > fmt_height)
        need_fmt_update = TRUE;
    }
  }

  if (need_fmt_update) {
    struct v4l2_format format;
    guint wanted_stride[GST_VIDEO_MAX_PLANES] = { 0, };

    format = obj->format;

    if (padded_height) {
      GST_DEBUG_OBJECT (obj->dbg_obj, "Padded height %u", padded_height);

      obj->align.padding_bottom =
          padded_height - GST_VIDEO_INFO_FIELD_HEIGHT (&obj->info);
    } else {
      GST_WARNING_OBJECT (obj->dbg_obj,
          "Failed to compute padded height; keep the default one");
      padded_height = format.fmt.pix_mp.height;
    }

    // /* update the current format with the stride we want to import from */
    // if (V4L2_TYPE_IS_MULTIPLANAR (obj->type)) {
    //   guint i;

    //   GST_DEBUG_OBJECT (obj->dbg_obj, "Wanted strides:");

    //   for (i = 0; i < obj->n_v4l2_planes; i++) {
    //     gint plane_stride = stride[i];

    //     if (GST_VIDEO_FORMAT_INFO_IS_TILED (obj->info.finfo))
    //       plane_stride = GST_VIDEO_TILE_X_TILES (plane_stride) *
    //           GST_VIDEO_FORMAT_INFO_TILE_STRIDE (obj->info.finfo, i);

    //     format.fmt.pix_mp.plane_fmt[i].bytesperline = plane_stride;
    //     format.fmt.pix_mp.height = padded_height;
    //     wanted_stride[i] = plane_stride;
    //     GST_DEBUG_OBJECT (obj->dbg_obj, "    [%u] %i", i, wanted_stride[i]);
    //   }
    // } else {
    //   gint plane_stride = stride[0];

    //   GST_DEBUG_OBJECT (obj->dbg_obj, "Wanted stride: %i", plane_stride);

    //   if (GST_VIDEO_FORMAT_INFO_IS_TILED (obj->info.finfo))
    //     plane_stride = GST_VIDEO_TILE_X_TILES (plane_stride) *
    //         GST_VIDEO_FORMAT_INFO_TILE_STRIDE (obj->info.finfo, 0);

    //   format.fmt.pix.bytesperline = plane_stride;
    //   format.fmt.pix.height = padded_height;
    //   wanted_stride[0] = plane_stride;
    // }

    if (obj->ioctl (obj->video_fd, VIDIOC_S_FMT, &format) < 0) {
      GST_WARNING_OBJECT (obj->dbg_obj,
          "Something went wrong trying to update current format: %s",
          g_strerror (errno));
      return FALSE;
    }

    if (V4L2_TYPE_IS_MULTIPLANAR (obj->type)) {\
      gint i;

      for (i = 0; i < obj->n_v4l2_planes; i++) {
        if (format.fmt.pix_mp.plane_fmt[i].bytesperline != wanted_stride[i]) {
          GST_DEBUG_OBJECT (obj->dbg_obj,
              "[%i] Driver did not accept the new stride (wants %i, got %i)",
              i, wanted_stride[i], format.fmt.pix_mp.plane_fmt[i].bytesperline);
          return FALSE;
        }
      }

      if (format.fmt.pix_mp.height != padded_height) {
        GST_DEBUG_OBJECT (obj->dbg_obj,
            "Driver did not accept the padded height (wants %i, got %i)",
            padded_height, format.fmt.pix_mp.height);
      }
    } else {
      if (format.fmt.pix.bytesperline != wanted_stride[0]) {
        GST_DEBUG_OBJECT (obj->dbg_obj,
            "Driver did not accept the new stride (wants %i, got %i)",
            wanted_stride[0], format.fmt.pix.bytesperline);
        return FALSE;
      }

      if (format.fmt.pix.height != padded_height) {
        GST_DEBUG_OBJECT (obj->dbg_obj,
            "Driver did not accept the padded height (wants %i, got %i)",
            padded_height, format.fmt.pix.height);
      }
    }
  }

  if (obj->align.padding_bottom) {
    /* Crop because of vertical padding */
    GST_DEBUG_OBJECT (obj->dbg_obj, "crop because of bottom padding of %d",
        obj->align.padding_bottom);
    gst_v4l2_object_setup_padding (obj);
  }

  return TRUE;
}

static gboolean
validate_video_meta_struct (GstV4l2Object * obj, const GstStructure * s)
{
  gint i;

  for (i = 0; i < gst_structure_n_fields (s); i++) {
    const gchar *name = gst_structure_nth_field_name (s, i);

    if (!g_str_equal (name, "padding-top")
        && !g_str_equal (name, "padding-bottom")
        && !g_str_equal (name, "padding-left")
        && !g_str_equal (name, "padding-right")) {
      GST_WARNING_OBJECT (obj->dbg_obj, "Unknown video meta field: '%s'", name);
      return FALSE;
    }
  }

  return TRUE;
}

static gboolean
gst_v4l2_object_match_buffer_layout_from_struct (GstV4l2Object * obj,
    const GstStructure * s, GstCaps * caps, guint buffer_size)
{
  GstVideoInfo info;
  GstVideoAlignment align;
  gsize plane_size[GST_VIDEO_MAX_PLANES];

  if (!validate_video_meta_struct (obj, s))
    return FALSE;

  if (!gst_video_info_from_caps (&info, caps)) {
    GST_WARNING_OBJECT (obj->dbg_obj, "Failed to create video info");
    return FALSE;
  }

  gst_video_alignment_reset (&align);

  gst_structure_get_uint (s, "padding-top", &align.padding_top);
  gst_structure_get_uint (s, "padding-bottom", &align.padding_bottom);
  gst_structure_get_uint (s, "padding-left", &align.padding_left);
  gst_structure_get_uint (s, "padding-right", &align.padding_right);

  if (align.padding_top || align.padding_bottom || align.padding_left ||
      align.padding_right) {
    GST_DEBUG_OBJECT (obj->dbg_obj,
        "Upstream requested padding (top: %d bottom: %d left: %d right: %d)",
        align.padding_top, align.padding_bottom, align.padding_left,
        align.padding_right);
  }

  // if (!gst_video_info_align_full (&info, &align, plane_size)) {
  //   GST_WARNING_OBJECT (obj->dbg_obj, "Failed to align video info");
  //   return FALSE;
  // }

  if (GST_VIDEO_INFO_SIZE (&info) != buffer_size) {
    GST_WARNING_OBJECT (obj->dbg_obj,
        "Requested buffer size (%d) doesn't match video info size (%"
        G_GSIZE_FORMAT ")", buffer_size, GST_VIDEO_INFO_SIZE (&info));
    return FALSE;
  }

  GST_DEBUG_OBJECT (obj->dbg_obj,
      "try matching buffer layout requested by downstream");

  // gst_v4l2_object_match_buffer_layout (obj, GST_VIDEO_INFO_N_PLANES (&info),
  //     info.offset, info.stride, buffer_size,
  //     GST_VIDEO_INFO_PLANE_HEIGHT (&info, 0, plane_size));

  return TRUE;
}

gboolean
gst_v4l2_object_decide_allocation (GstV4l2Object * obj, GstQuery * query)
{
  GstCaps *caps;
  GstBufferPool *pool = NULL, *other_pool = NULL, *obj_pool = NULL;
  GstStructure *config;
  guint size, min, max, own_min = 0;
  gboolean update;
  gboolean has_video_meta;
  gboolean can_share_own_pool, pushing_from_our_pool = FALSE;
  GstAllocator *allocator = NULL;
  GstAllocationParams params = { 0 };
  guint video_idx;

  GST_DEBUG_OBJECT (obj->dbg_obj, "decide allocation");

  g_return_val_if_fail (obj->type == V4L2_BUF_TYPE_VIDEO_CAPTURE ||
      obj->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, FALSE);

  gst_query_parse_allocation (query, &caps, NULL);

  obj_pool = gst_v4l2_object_get_buffer_pool (obj);
  if (obj_pool == NULL) {
    if (!gst_v4l2_object_setup_pool (obj, caps))
      goto pool_failed;
    obj_pool = gst_v4l2_object_get_buffer_pool (obj);
    if (obj_pool == NULL)
      goto pool_failed;
  }

  if (gst_query_get_n_allocation_params (query) > 0)
    gst_query_parse_nth_allocation_param (query, 0, &allocator, &params);

  if (gst_query_get_n_allocation_pools (query) > 0) {
    gst_query_parse_nth_allocation_pool (query, 0, &pool, &size, &min, &max);
    update = TRUE;
  } else {
    pool = NULL;
    min = max = 0;
    size = 0;
    update = FALSE;
  }

  GST_DEBUG_OBJECT (obj->dbg_obj, "allocation: size:%u min:%u max:%u pool:%"
      GST_PTR_FORMAT, size, min, max, pool);

  has_video_meta =
      gst_query_find_allocation_meta (query, GST_VIDEO_META_API_TYPE,
      &video_idx);

  if (has_video_meta) {
    const GstStructure *params;
    gst_query_parse_nth_allocation_meta (query, video_idx, &params);

    if (params)
      gst_v4l2_object_match_buffer_layout_from_struct (obj, params, caps, size);
  }

  can_share_own_pool = (has_video_meta || !obj->need_video_meta);

  gst_v4l2_get_driver_min_buffers (obj);
  /* We can't share our own pool, if it exceed V4L2 capacity */
  if (min + obj->min_buffers + 1 > VIDEO_MAX_FRAME)
    can_share_own_pool = FALSE;

  /* select a pool */
  switch (obj->mode) {
    case GST_V4L2_IO_USERPTR:
    case GST_V4L2_IO_DMABUF_IMPORT:
      /* in importing mode, prefer our own pool, and pass the other pool to
       * our own, so it can serve itself */
      if (pool == NULL)
        goto no_downstream_pool;
      gst_v4l2_buffer_pool_set_other_pool (GST_V4L2_BUFFER_POOL (obj_pool),
          pool);
      other_pool = pool;
      gst_object_unref (pool);
      pool = gst_object_ref (obj_pool);
      size = obj->info.size;
      break;

    case GST_V4L2_IO_MMAP:
    case GST_V4L2_IO_DMABUF:
      /* in streaming mode, prefer our own pool */
      /* Check if we can use it ... */
      if (can_share_own_pool) {
        if (pool)
          gst_object_unref (pool);
        pool = gst_object_ref (obj_pool);
        size = obj->info.size;
        GST_DEBUG_OBJECT (obj->dbg_obj,
            "streaming mode: using our own pool %" GST_PTR_FORMAT, pool);
        pushing_from_our_pool = TRUE;
      } else if (pool) {
        GST_DEBUG_OBJECT (obj->dbg_obj,
            "streaming mode: copying to downstream pool %" GST_PTR_FORMAT,
            pool);
      } else {
        GST_DEBUG_OBJECT (obj->dbg_obj,
            "streaming mode: no usable pool, copying to generic pool");
        size = MAX (size, obj->info.size);
      }
      break;
    case GST_V4L2_IO_AUTO:
    default:
      GST_WARNING_OBJECT (obj->dbg_obj, "unhandled mode");
      break;
  }

  if (size == 0)
    goto no_size;

  /* If pushing from our own pool, configure it with queried minimum,
   * otherwise use the minimum required */
  if (pushing_from_our_pool) {
    /* When pushing from our own pool, we need what downstream one, to be able
     * to fill the pipeline, the minimum required to decoder according to the
     * driver and 2 more, so we don't endup up with everything downstream or
     * held by the decoder. We account 2 buffers for v4l2 so when one is being
     * pushed downstream the other one can already be queued for the next
     * frame. */
    own_min = min + obj->min_buffers + 2;

    /* If no allocation parameters where provided, allow for a little more
     * buffers and enable copy threshold */
    if (!update) {
      own_min += 2;
      gst_v4l2_buffer_pool_copy_at_threshold (GST_V4L2_BUFFER_POOL (pool),
          TRUE);
    } else {
      gst_v4l2_buffer_pool_copy_at_threshold (GST_V4L2_BUFFER_POOL (pool),
          FALSE);
    }

  } else {
    /* In this case we'll have to configure two buffer pool. For our buffer
     * pool, we'll need what the driver one, and one more, so we can dequeu */
    own_min = obj->min_buffers + 1;
    own_min = MAX (own_min, GST_V4L2_MIN_BUFFERS (obj));

    /* for the downstream pool, we keep what downstream wants, though ensure
     * at least a minimum if downstream didn't suggest anything (we are
     * expecting the base class to create a default one for the context) */
    min = MAX (min, GST_V4L2_MIN_BUFFERS (obj));

    /* To import we need the other pool to hold at least own_min */
    if (obj_pool == pool)
      min += own_min;
  }

  /* Request a bigger max, if one was suggested but it's too small */
  if (max != 0)
    max = MAX (min, max);

  /* First step, configure our own pool */
  config = gst_buffer_pool_get_config (obj_pool);

  if (obj->need_video_meta || has_video_meta) {
    GST_DEBUG_OBJECT (obj->dbg_obj, "activate Video Meta");
    gst_buffer_pool_config_add_option (config,
        GST_BUFFER_POOL_OPTION_VIDEO_META);
  }

  gst_buffer_pool_config_set_allocator (config, allocator, &params);
  gst_buffer_pool_config_set_params (config, caps, size, own_min, 0);

  GST_DEBUG_OBJECT (obj->dbg_obj, "setting own pool config to %"
      GST_PTR_FORMAT, config);

  /* Our pool often need to adjust the value */
  if (!gst_buffer_pool_set_config (obj_pool, config)) {
    config = gst_buffer_pool_get_config (obj_pool);

    GST_DEBUG_OBJECT (obj->dbg_obj, "own pool config changed to %"
        GST_PTR_FORMAT, config);

    /* our pool will adjust the maximum buffer, which we are fine with */
    if (!gst_buffer_pool_set_config (obj_pool, config))
      goto config_failed;
  }

  /* Now configure the other pool if different */
  if (obj_pool != pool)
    other_pool = pool;

  if (other_pool) {
    config = gst_buffer_pool_get_config (other_pool);
    gst_buffer_pool_config_set_allocator (config, allocator, &params);
    gst_buffer_pool_config_set_params (config, caps, size, min, max);

    GST_DEBUG_OBJECT (obj->dbg_obj, "setting other pool config to %"
        GST_PTR_FORMAT, config);

    /* if downstream supports video metadata, add this to the pool config */
    if (has_video_meta) {
      GST_DEBUG_OBJECT (obj->dbg_obj, "activate Video Meta");
      gst_buffer_pool_config_add_option (config,
          GST_BUFFER_POOL_OPTION_VIDEO_META);
    }

    if (!gst_buffer_pool_set_config (other_pool, config)) {
      config = gst_buffer_pool_get_config (other_pool);

      if (!gst_buffer_pool_config_validate_params (config, caps, size, min,
              max)) {
        gst_structure_free (config);
        goto config_failed;
      }

      if (!gst_buffer_pool_set_config (other_pool, config))
        goto config_failed;
    }
  }

  if (pool) {
    /* For simplicity, simply read back the active configuration, so our base
     * class get the right information */
    config = gst_buffer_pool_get_config (pool);
    gst_buffer_pool_config_get_params (config, NULL, &size, &min, &max);
    gst_structure_free (config);
  }

  if (update)
    gst_query_set_nth_allocation_pool (query, 0, pool, size, min, max);
  else
    gst_query_add_allocation_pool (query, pool, size, min, max);

  if (allocator)
    gst_object_unref (allocator);

  if (pool)
    gst_object_unref (pool);

  if (obj_pool)
    gst_object_unref (obj_pool);

  return TRUE;

pool_failed:
  {
    /* setup_pool already send the error */
    GST_ERROR_OBJECT (obj->dbg_obj, "setup obj_pool fail!");
    goto cleanup;
  }
config_failed:
  {
    GST_ELEMENT_ERROR (obj->element, RESOURCE, SETTINGS,
        ("Failed to configure internal buffer pool."), (NULL));
    goto cleanup;
  }
no_size:
  {
    GST_ELEMENT_ERROR (obj->element, RESOURCE, SETTINGS,
        ("Video device did not suggest any buffer size."), (NULL));
    goto cleanup;
  }
no_downstream_pool:
  {
    GST_ELEMENT_ERROR (obj->element, RESOURCE, SETTINGS,
        ("No downstream pool to import from."),
        ("When importing DMABUF or USERPTR, we need a pool to import from"));
    goto cleanup;
  }
cleanup:
  {
    if (allocator)
      gst_object_unref (allocator);

    if (pool)
      gst_object_unref (pool);

    if (obj_pool)
      gst_object_unref (obj_pool);
    return FALSE;
  }
}

gboolean
gst_v4l2_object_propose_allocation (GstV4l2Object * obj, GstQuery * query)
{
  GstBufferPool *pool = NULL;
  /* we need at least 2 buffers to operate */
  guint size, min, max;
  GstCaps *caps;
  gboolean need_pool;

  /* Set defaults allocation parameters */
  size = obj->info.size;
  min = GST_V4L2_MIN_BUFFERS (obj);
  max = VIDEO_MAX_FRAME;

  gst_query_parse_allocation (query, &caps, &need_pool);

  if (caps == NULL)
    goto no_caps;

  switch (obj->mode) {
    case GST_V4L2_IO_MMAP:
    case GST_V4L2_IO_DMABUF:
      if (need_pool) {
        GstBufferPool *obj_pool = gst_v4l2_object_get_buffer_pool (obj);
        if (obj_pool) {
          if (!gst_buffer_pool_is_active (obj_pool))
            pool = gst_object_ref (obj_pool);

          gst_object_unref (obj_pool);
        }
      }
      break;
    default:
      break;
  }

  if (pool != NULL) {
    GstCaps *pcaps;
    GstStructure *config;

    /* we had a pool, check caps */
    config = gst_buffer_pool_get_config (pool);
    gst_buffer_pool_config_get_params (config, &pcaps, NULL, NULL, NULL);

    GST_DEBUG_OBJECT (obj->dbg_obj,
        "we had a pool with caps %" GST_PTR_FORMAT, pcaps);
    if (!gst_caps_is_equal (caps, pcaps)) {
      gst_structure_free (config);
      gst_object_unref (pool);
      goto different_caps;
    }
    gst_structure_free (config);
  }
  gst_v4l2_get_driver_min_buffers (obj);

  min = MAX (obj->min_buffers, GST_V4L2_MIN_BUFFERS (obj));

  gst_query_add_allocation_pool (query, pool, size, min, max);

  /* we also support various metadata */
  gst_query_add_allocation_meta (query, GST_VIDEO_META_API_TYPE, NULL);

  if (pool)
    gst_object_unref (pool);

  return TRUE;

  /* ERRORS */
no_caps:
  {
    GST_DEBUG_OBJECT (obj->dbg_obj, "no caps specified");
    return FALSE;
  }
different_caps:
  {
    /* different caps, we can't use this pool */
    GST_DEBUG_OBJECT (obj->dbg_obj, "pool has different caps");
    return FALSE;
  }
}

gboolean
gst_v4l2_object_try_import (GstV4l2Object * obj, GstBuffer * buffer)
{
  GstVideoMeta *vmeta;
  gint n_mem = gst_buffer_n_memory (buffer);

  /* only import if requested */
  switch (obj->mode) {
    case GST_V4L2_IO_USERPTR:
    case GST_V4L2_IO_DMABUF_IMPORT:
      break;
    default:
      GST_DEBUG_OBJECT (obj->dbg_obj,
          "The io-mode does not enable importation");
      return FALSE;
  }

  vmeta = gst_buffer_get_video_meta (buffer);
  if (!vmeta && obj->need_video_meta) {
    GST_DEBUG_OBJECT (obj->dbg_obj, "Downstream buffer uses standard "
        "stride/offset while the driver does not.");
    return FALSE;
  }

  /* we need matching strides/offsets and size */
  if (vmeta) {
    guint plane_height[GST_VIDEO_MAX_PLANES] = { 0, };

    // gst_video_meta_get_plane_height (vmeta, plane_height);

    if (!gst_v4l2_object_match_buffer_layout (obj, vmeta->n_planes,
            vmeta->offset, vmeta->stride, gst_buffer_get_size (buffer),
            plane_height[0]))
      return FALSE;
  }

  /* we can always import single memory buffer, but otherwise we need the same
   * amount of memory object. */
  if (n_mem != 1 && n_mem != obj->n_v4l2_planes) {
    GST_DEBUG_OBJECT (obj->dbg_obj, "Can only import %i memory, "
        "buffers contains %u memory", obj->n_v4l2_planes, n_mem);
    return FALSE;
  }

  /* For DMABuf importation we need DMABuf of course */
  if (obj->mode == GST_V4L2_IO_DMABUF_IMPORT) {
    gint i;

    for (i = 0; i < n_mem; i++) {
      GstMemory *mem = gst_buffer_peek_memory (buffer, i);

      if (!gst_is_dmabuf_memory (mem)) {
        GST_DEBUG_OBJECT (obj->dbg_obj, "Cannot import non-DMABuf memory.");
        return FALSE;
      }
    }
  }

  /* for the remaining, only the kernel driver can tell */
  return TRUE;
}

/**
 * gst_v4l2_object_get_buffer_pool:
 * @src: a #GstV4l2Object
 *
 * Returns: (nullable) (transfer full): the instance of the #GstBufferPool used
 * by the v4l2object; unref it after usage.
 */
GstBufferPool *
gst_v4l2_object_get_buffer_pool (GstV4l2Object * v4l2object)
{
  GstBufferPool *ret = NULL;

  g_return_val_if_fail (v4l2object != NULL, NULL);

  GST_OBJECT_LOCK (v4l2object->element);
  if (v4l2object->pool)
    ret = gst_object_ref (v4l2object->pool);
  GST_OBJECT_UNLOCK (v4l2object->element);

  return ret;
}

/**
 * gst_v4l2_object_poll:
 * @v4l2object: a #GstV4l2Object
 * @timeout: timeout of type #GstClockTime
 *
 * Poll the video file descriptor for read when this is a capture, write when
 * this is an output. It will also watch for errors and source change events.
 * If a source change event is received, %GST_V4L2_FLOW_RESOLUTION_CHANGE will
 * be returned. If the poll was interrupted, %GST_FLOW_FLUSHING is returned.
 * If there was no read or write indicator, %GST_V4L2_FLOW_LAST_BUFFER is
 * returned. It may also return %GST_FLOW_ERROR if some unexpected error
 * occured.
 *
 * Returns: GST_FLOW_OK if buffers are ready to be queued or dequeued.
 */
GstFlowReturn
gst_v4l2_object_poll (GstV4l2Object * v4l2object, GstClockTime timeout)
{
  gint ret;

  if (!v4l2object->can_poll_device) {
    if (timeout != 0)
      goto done;
    else
      goto no_buffers;
  }

  GST_LOG_OBJECT (v4l2object->dbg_obj, "polling device");

again:
  ret = gst_poll_wait (v4l2object->poll, timeout);
  if (G_UNLIKELY (ret < 0)) {
    switch (errno) {
      case EBUSY:
        goto stopped;
      case EAGAIN:
      case EINTR:
        goto again;
      case ENXIO:
        GST_WARNING_OBJECT (v4l2object->dbg_obj,
            "v4l2 device doesn't support polling. Disabling"
            " using libv4l2 in this case may cause deadlocks");
        v4l2object->can_poll_device = FALSE;
        goto done;
      default:
        goto select_error;
    }
  }

  if (gst_poll_fd_has_error (v4l2object->poll, &v4l2object->pollfd))
    goto select_error;

  /* PRI is used to signal that events are available */
  if (gst_poll_fd_has_pri (v4l2object->poll, &v4l2object->pollfd)) {
    struct v4l2_event event = { 0, };

    if (!gst_v4l2_dequeue_event (v4l2object, &event))
      goto dqevent_failed;

    if (event.type != V4L2_EVENT_SOURCE_CHANGE) {
      GST_INFO_OBJECT (v4l2object->dbg_obj,
          "Received unhandled event, ignoring.");
      goto again;
    }

    if ((event.u.src_change.changes & V4L2_EVENT_SRC_CH_RESOLUTION) == 0) {
      GST_DEBUG_OBJECT (v4l2object->dbg_obj,
          "Received non-resolution source-change, ignoring.");
      goto again;
    }

    if (v4l2object->formats)
      gst_v4l2_object_clear_format_list (v4l2object);

    return GST_V4L2_FLOW_RESOLUTION_CHANGE;
  }

  if (ret == 0)
    goto no_buffers;

done:
  return GST_FLOW_OK;

  /* ERRORS */
stopped:
  {
    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "stop called");
    return GST_FLOW_FLUSHING;
  }
select_error:
  {
    GST_ELEMENT_ERROR (v4l2object->element, RESOURCE, READ, (NULL),
        ("poll error %d: %s (%d)", ret, g_strerror (errno), errno));
    return GST_FLOW_ERROR;
  }
no_buffers:
  {
    return GST_V4L2_FLOW_LAST_BUFFER;
  }
dqevent_failed:
  {
    GST_ELEMENT_ERROR (v4l2object->element, RESOURCE, READ, (NULL),
        ("dqevent error: %s (%d)", g_strerror (errno), errno));
    return GST_FLOW_ERROR;
  }
}

/**
 * gst_v4l2_object_subscribe_event:
 * @v4l2object: a #GstV4l2Object
 * @event: the event ID
 *
 * Subscribe to an event, and enable polling for these. Note that only
 * %V4L2_EVENT_SOURCE_CHANGE is currently supported by the poll helper.
 *
 * Returns: %TRUE if the driver supports this event
 */
gboolean
gst_v4l2_object_subscribe_event (GstV4l2Object * v4l2object, guint32 event)
{
  guint32 id = 0;

  g_return_val_if_fail (v4l2object != NULL, FALSE);
  g_return_val_if_fail (GST_V4L2_IS_OPEN (v4l2object), FALSE);

  v4l2object->get_in_out_func (v4l2object, &id);

  if (gst_v4l2_subscribe_event (v4l2object, event, id)) {
    gst_poll_fd_ctl_pri (v4l2object->poll, &v4l2object->pollfd, TRUE);
    return TRUE;
  }

  return FALSE;
}
