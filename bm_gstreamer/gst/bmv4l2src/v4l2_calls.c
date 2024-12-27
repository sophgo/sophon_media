/* GStreamer
 *
 * Copyright (C) 2002 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *               2006 Edgard Lima <edgard.lima@gmail.com>
 *
 * v4l2_calls.c - generic V4L2 calls handling
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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "gstv4l2object.h"

#include "gstbmv4l2src.h"

GST_DEBUG_CATEGORY_EXTERN (bm_v4l2_debug);
#define GST_CAT_DEFAULT bm_v4l2_debug

/******************************************************
 * gst_v4l2_get_capabilities():
 *   get the device's capturing capabilities
 * return value: TRUE on success, FALSE on error
 ******************************************************/
static gboolean
gst_v4l2_get_capabilities (GstV4l2Object * v4l2object)
{
  GstElement *e;

  e = v4l2object->element;

  GST_DEBUG_OBJECT (e, "getting capabilities");

  if (!GST_V4L2_IS_OPEN (v4l2object))
    return FALSE;

  if (v4l2object->ioctl (v4l2object->video_fd, VIDIOC_QUERYCAP,
          &v4l2object->vcap) < 0)
    goto cap_failed;

  if (v4l2object->vcap.capabilities & V4L2_CAP_DEVICE_CAPS)
    v4l2object->device_caps = v4l2object->vcap.device_caps;
  else
    v4l2object->device_caps = v4l2object->vcap.capabilities;

  GST_LOG_OBJECT (e, "driver:      '%s'", v4l2object->vcap.driver);
  GST_LOG_OBJECT (e, "card:        '%s'", v4l2object->vcap.card);
  GST_LOG_OBJECT (e, "bus_info:    '%s'", v4l2object->vcap.bus_info);
  GST_LOG_OBJECT (e, "version:     %08x", v4l2object->vcap.version);
  GST_LOG_OBJECT (e, "capabilities: %08x", v4l2object->device_caps);

  return TRUE;

  /* ERRORS */
cap_failed:
  {
    GST_ELEMENT_ERROR (v4l2object->element, RESOURCE, SETTINGS,
        ("Error getting capabilities for device '%s': "
                "It isn't a v4l2 driver. Check if it is a v4l1 driver.",
            v4l2object->videodev), GST_ERROR_SYSTEM);
    return FALSE;
  }
}

/******************************************************
 * The video4linux command line tool v4l2-ctrl
 * normalises the names of the controls received from
 * the kernel like:
 *
 *     "Exposure (absolute)" -> "exposure_absolute"
 *
 * We follow their lead here.  @name is modified
 * in-place.
 ******************************************************/
static void __attribute__((unused))
gst_v4l2_normalise_control_name (gchar * name)
{
  int i, j;
  for (i = 0, j = 0; name[j]; ++j) {
    if (g_ascii_isalnum (name[j])) {
      if (i > 0 && !g_ascii_isalnum (name[j - 1]))
        name[i++] = '_';
      name[i++] = g_ascii_tolower (name[j]);
    }
  }
  name[i++] = '\0';
}

/******************************************************
 * gst_v4l2_empty_lists() and gst_v4l2_fill_lists():
 *   fill/empty the lists of enumerations
 * return value: TRUE on success, FALSE on error
 ******************************************************/
static gboolean
gst_v4l2_fill_lists (GstV4l2Object * v4l2object)
{
  GST_V4L2_CHECK_OPEN (v4l2object);

  return TRUE;
}


static void
gst_v4l2_empty_lists (GstV4l2Object * v4l2object)
{
  g_datalist_clear (&v4l2object->controls);
}

static void
gst_v4l2_adjust_buf_type (GstV4l2Object * v4l2object)
{
  /* when calling gst_v4l2_object_new the user decides the initial type
   * so adjust it if multi-planar is supported
   * the driver should make it exclusive. So the driver should
   * not support both MPLANE and non-PLANE.
   * Because even when using MPLANE it still possibles to use it
   * in a contiguous manner. In this case the first v4l2 plane
   * contains all the gst planes.
   */
  switch (v4l2object->type) {
    case V4L2_BUF_TYPE_VIDEO_OUTPUT:
      if (v4l2object->device_caps &
          (V4L2_CAP_VIDEO_OUTPUT_MPLANE | V4L2_CAP_VIDEO_M2M_MPLANE)) {
        GST_DEBUG ("adjust type to multi-planar output");
        v4l2object->type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
      }
      break;
    case V4L2_BUF_TYPE_VIDEO_CAPTURE:
      if (v4l2object->device_caps &
          (V4L2_CAP_VIDEO_CAPTURE_MPLANE | V4L2_CAP_VIDEO_M2M_MPLANE)) {
        GST_DEBUG ("adjust type to multi-planar capture");
        v4l2object->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
      }
      break;
    default:
      break;
  }
}

/******************************************************
 * gst_v4l2_open():
 *   open the video device (v4l2object->videodev)
 * return value: TRUE on success, FALSE on error
 ******************************************************/
gboolean
gst_v4l2_open (GstV4l2Object * v4l2object, GstV4l2Error * error)
{
  struct stat st;
  int libv4l2_fd = -1;

  GST_INFO_OBJECT (v4l2object->dbg_obj, "Trying to open device %s",
      v4l2object->videodev);

  GST_V4L2_CHECK_NOT_OPEN (v4l2object);
  GST_V4L2_CHECK_NOT_ACTIVE (v4l2object);

  /* be sure we have a device */
  if (!v4l2object->videodev)
    v4l2object->videodev = g_strdup ("/dev/video");

  /* check if it is a device */
  if (stat (v4l2object->videodev, &st) == -1)
    goto stat_failed;

  if (!S_ISCHR (st.st_mode))
    goto no_device;

  /* open the device */
  v4l2object->video_fd =
      open (v4l2object->videodev, O_RDWR /* | O_NONBLOCK */ );

  if (!GST_V4L2_IS_OPEN (v4l2object))
    goto not_open;

#ifdef HAVE_LIBV4L2
  if (v4l2object->fd_open)
    libv4l2_fd = v4l2object->fd_open (v4l2object->video_fd,
        V4L2_ENABLE_ENUM_FMT_EMULATION);
#endif

  /* Note the v4l2_xxx functions are designed so that if they get passed an
     unknown fd, the will behave exactly as their regular xxx counterparts, so
     if v4l2_fd_open fails, we continue as normal (missing the libv4l2 custom
     cam format to normal formats conversion). Chances are big we will still
     fail then though, as normally v4l2_fd_open only fails if the device is not
     a v4l2 device. */
  if (libv4l2_fd != -1)
    v4l2object->video_fd = libv4l2_fd;

  /* get capabilities, error will be posted */
  if (!gst_v4l2_get_capabilities (v4l2object))
    goto error;

  /* do we need to be a capture device? */
  if (GST_IS_BM_V4L2SRC (v4l2object->element) &&
      !(v4l2object->device_caps & (V4L2_CAP_VIDEO_CAPTURE |
              V4L2_CAP_VIDEO_CAPTURE_MPLANE)))
    goto not_capture;

  gst_v4l2_adjust_buf_type (v4l2object);

  /* create enumerations, posts errors. */
  if (!gst_v4l2_fill_lists (v4l2object))
    goto error;

  GST_INFO_OBJECT (v4l2object->dbg_obj,
      "Opened device '%s' (%s) successfully",
      v4l2object->vcap.card, v4l2object->videodev);

  return TRUE;

  /* ERRORS */
stat_failed:
  {
    GST_V4L2_ERROR (error, RESOURCE, NOT_FOUND,
        ("Cannot identify device '%s'.", v4l2object->videodev),
        GST_ERROR_SYSTEM);
    goto error;
  }
no_device:
  {
    GST_V4L2_ERROR (error, RESOURCE, NOT_FOUND,
        ("This isn't a device '%s'.", v4l2object->videodev),
        GST_ERROR_SYSTEM);
    goto error;
  }
not_open:
  {
    GST_V4L2_ERROR (error, RESOURCE, OPEN_READ_WRITE,
        ("Could not open device '%s' for reading and writing.",
            v4l2object->videodev), GST_ERROR_SYSTEM);
    goto error;
  }
not_capture:
  {
    GST_V4L2_ERROR (error, RESOURCE, NOT_FOUND,
        ("Device '%s' is not a capture device.", v4l2object->videodev),
        ("Capabilities: 0x%x", v4l2object->device_caps));
    goto error;
  }
error:
  {
    if (GST_V4L2_IS_OPEN (v4l2object)) {
      /* close device */
      v4l2object->close (v4l2object->video_fd);
      v4l2object->video_fd = -1;
    }
    /* empty lists */
    gst_v4l2_empty_lists (v4l2object);

    return FALSE;
  }
}

gboolean
gst_v4l2_dup (GstV4l2Object * v4l2object, GstV4l2Object * other)
{
  GST_DEBUG_OBJECT (v4l2object->dbg_obj, "Trying to dup device %s",
      other->videodev);

  GST_V4L2_CHECK_OPEN (other);
  GST_V4L2_CHECK_NOT_OPEN (v4l2object);
  GST_V4L2_CHECK_NOT_ACTIVE (other);
  GST_V4L2_CHECK_NOT_ACTIVE (v4l2object);

  v4l2object->vcap = other->vcap;
  v4l2object->device_caps = other->device_caps;
  gst_v4l2_adjust_buf_type (v4l2object);

  v4l2object->video_fd = v4l2object->dup (other->video_fd);
  if (!GST_V4L2_IS_OPEN (v4l2object))
    goto not_open;

  g_free (v4l2object->videodev);
  v4l2object->videodev = g_strdup (other->videodev);

  GST_INFO_OBJECT (v4l2object->dbg_obj,
      "Cloned device '%s' (%s) successfully",
      v4l2object->vcap.card, v4l2object->videodev);

  v4l2object->never_interlaced = other->never_interlaced;
  v4l2object->no_initial_format = other->no_initial_format;

  return TRUE;

not_open:
  {
    GST_ELEMENT_ERROR (v4l2object->element, RESOURCE, OPEN_READ_WRITE,
        ("Could not dup device '%s' for reading and writing.",
            v4l2object->videodev), GST_ERROR_SYSTEM);

    return FALSE;
  }
}


/******************************************************
 * gst_v4l2_close():
 *   close the video device (v4l2object->video_fd)
 * return value: TRUE on success, FALSE on error
 ******************************************************/
gboolean
gst_v4l2_close (GstV4l2Object * v4l2object)
{
  GST_DEBUG_OBJECT (v4l2object->dbg_obj, "Trying to close %s",
      v4l2object->videodev);

  GST_V4L2_CHECK_OPEN (v4l2object);
  GST_V4L2_CHECK_NOT_ACTIVE (v4l2object);

  /* close device */
  v4l2object->close (v4l2object->video_fd);
  v4l2object->video_fd = -1;

  /* empty lists */
  gst_v4l2_empty_lists (v4l2object);

  return TRUE;
}

gboolean
gst_v4l2_get_input (GstV4l2Object * v4l2object, guint32 * input)
{
  guint32 n;

  GST_DEBUG_OBJECT (v4l2object->dbg_obj, "trying to get input");

  if (!GST_V4L2_IS_OPEN (v4l2object))
    return FALSE;

  if (v4l2object->ioctl (v4l2object->video_fd, VIDIOC_G_INPUT, &n) < 0)
    goto input_failed;

  *input = n;

  GST_DEBUG_OBJECT (v4l2object->dbg_obj, "input: %u", n);

  return TRUE;

  /* ERRORS */
input_failed:
  if (v4l2object->device_caps & V4L2_CAP_TUNER) {
    /* only give a warning message if driver actually claims to have tuner
     * support
     */
    GST_ELEMENT_WARNING (v4l2object->element, RESOURCE, SETTINGS,
        ("Failed to get current input on device '%s'. May be it is a radio device", v4l2object->videodev), GST_ERROR_SYSTEM);
  }
  return FALSE;
}

gboolean
gst_v4l2_set_input (GstV4l2Object * v4l2object, guint32 input)
{
  GST_DEBUG_OBJECT (v4l2object->dbg_obj, "trying to set input to %u", input);

  if (!GST_V4L2_IS_OPEN (v4l2object))
    return FALSE;

  if (v4l2object->ioctl (v4l2object->video_fd, VIDIOC_S_INPUT, &input) < 0)
    goto input_failed;

  return TRUE;

  /* ERRORS */
input_failed:
  if (v4l2object->device_caps & V4L2_CAP_TUNER) {
    /* only give a warning message if driver actually claims to have tuner
     * support
     */
    GST_ELEMENT_WARNING (v4l2object->element, RESOURCE, SETTINGS,
        ("Failed to set input %u on device %s.",
            input, v4l2object->videodev), GST_ERROR_SYSTEM);
  }
  return FALSE;
}

gboolean
gst_v4l2_query_input (GstV4l2Object * obj, struct v4l2_input *input)
{
  gint ret;

  ret = obj->ioctl (obj->video_fd, VIDIOC_ENUMINPUT, input);
  if (ret < 0) {
    GST_WARNING_OBJECT (obj->dbg_obj, "Failed to read input state: %s (%i)",
        g_strerror (errno), errno);
    return FALSE;
  }

  return TRUE;
}

static const gchar *
gst_v4l2_event_to_string (guint32 event)
{
  switch (event) {
    case V4L2_EVENT_ALL:
      return "ALL";
    case V4L2_EVENT_VSYNC:
      return "VSYNC";
    case V4L2_EVENT_EOS:
      return "EOS";
    case V4L2_EVENT_CTRL:
      return "CTRL";
    case V4L2_EVENT_FRAME_SYNC:
      return "FRAME_SYNC";
    case V4L2_EVENT_SOURCE_CHANGE:
      return "SOURCE_CHANGE";
    case V4L2_EVENT_MOTION_DET:
      return "MOTION_DET";
    default:
      break;
  }

  return "UNKNOWN";
}

gboolean
gst_v4l2_subscribe_event (GstV4l2Object * v4l2object, guint32 event, guint32 id)
{
  struct v4l2_event_subscription sub = {.type = event,.id = id, };
  gint ret;

  GST_DEBUG_OBJECT (v4l2object->dbg_obj, "Subscribing to '%s' event",
      gst_v4l2_event_to_string (event));

  if (!GST_V4L2_IS_OPEN (v4l2object))
    return FALSE;

  ret = v4l2object->ioctl (v4l2object->video_fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
  if (ret < 0)
    goto failed;

  return TRUE;

  /* ERRORS */
failed:
  {
    if (errno == ENOTTY || errno == EINVAL) {
      GST_DEBUG_OBJECT (v4l2object->dbg_obj,
          "Cannot subscribe to '%s' event: %s",
          gst_v4l2_event_to_string (event), "not supported");
    } else {
      GST_ERROR_OBJECT (v4l2object->dbg_obj,
          "Cannot subscribe to '%s' event: %s",
          gst_v4l2_event_to_string (event), g_strerror (errno));
    }
    return FALSE;
  }
}

gboolean
gst_v4l2_dequeue_event (GstV4l2Object * v4l2object, struct v4l2_event *event)
{
  gint ret;

  if (!GST_V4L2_IS_OPEN (v4l2object))
    return FALSE;

  ret = v4l2object->ioctl (v4l2object->video_fd, VIDIOC_DQEVENT, event);

  if (ret < 0) {
    GST_ERROR_OBJECT (v4l2object->dbg_obj, "DQEVENT failed: %s",
        g_strerror (errno));
    return FALSE;
  }

  GST_DEBUG_OBJECT (v4l2object->dbg_obj, "Dequeued a '%s' event.",
      gst_v4l2_event_to_string (event->type));

  return TRUE;
}