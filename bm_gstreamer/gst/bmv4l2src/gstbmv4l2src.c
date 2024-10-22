#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "gstbmv4l2src.h"
#include "v4l2-utils.h"
#include "cvi_isp_v4l2.h"

enum
{
  PROP_0,
  PROP_DEVICE_PATH,
  PROP_CAST_DMABUF,
  PROP_LAST,
};

struct PreferredCapsInfo
{
  gint width;
  gint height;
  gint fps_n;
  gint fps_d;
};

/* signals and args */
enum
{
  SIGNAL_PRE_SET_FORMAT,
  LAST_SIGNAL
};

static guint gst_v4l2_signals[LAST_SIGNAL] = { 0 };

#define DEFAULT_PROP_DEVICE   "/dev/video0"

static void gst_bmv4l2src_uri_handler_init (gpointer g_iface, gpointer iface_data);

G_DEFINE_TYPE_WITH_CODE (GstBmV4l2Src, gst_bmv4l2src, GST_TYPE_PUSH_SRC,
    G_IMPLEMENT_INTERFACE (GST_TYPE_URI_HANDLER, gst_bmv4l2src_uri_handler_init););

static void
gst_bmv4l2src_open_device(GstBmV4l2Src *v4l2src)
{
	gchar dev_path[16];
	gint index = 0;
	gint strLen = sizeof(DEFAULT_PROP_DEVICE);

	memcpy(dev_path, v4l2src->device_path, strLen);
	index = dev_path[strLen - 2] - '0';

  /* fixme: give an update_fps_function */
  v4l2src->v4l2object = gst_v4l2_object_new (GST_ELEMENT (v4l2src),
      GST_OBJECT (GST_BASE_SRC_PAD (v4l2src)), V4L2_BUF_TYPE_VIDEO_CAPTURE,
      dev_path, gst_v4l2_get_input, gst_v4l2_set_input, NULL);

  v4l2src->index_id = index;

  GST_INFO_OBJECT (v4l2src, "open:%s as src%d", dev_path, index);

  /* Avoid the slow probes */
  v4l2src->v4l2object->skip_try_fmt_probes = TRUE;

  gst_base_src_set_format (GST_BASE_SRC (v4l2src), GST_FORMAT_TIME);
  gst_base_src_set_live (GST_BASE_SRC (v4l2src), TRUE);
}

static void
gst_bmv4l2src_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GstBmV4l2Src *v4l2src = GST_BM_V4L2SRC (object);

  switch (prop_id) {
    case PROP_DEVICE_PATH:
      v4l2src->device_path = g_value_dup_string (value);
      gst_bmv4l2src_open_device(v4l2src);
      break;
    case PROP_CAST_DMABUF:
      v4l2src->cast_dmabuf = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      return;
  }
}

static void
gst_bmv4l2src_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  GstBmV4l2Src *v4l2src = GST_BM_V4L2SRC (object);

  switch (prop_id) {
    case PROP_DEVICE_PATH:
      g_value_set_string (value, v4l2src->device_path);
      break;
    case PROP_CAST_DMABUF:
      g_value_set_uint (value, v4l2src->cast_dmabuf);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
gst_bmv4l2src_start (GstBaseSrc * src)
{
  GstBmV4l2Src *v4l2src = GST_BM_V4L2SRC (src);
  GstV4l2Object *obj = v4l2src->v4l2object;

  v4l2src->offset = 0;
  v4l2src->next_offset_same = FALSE;
  v4l2src->renegotiation_adjust = 0;

  CVI_ISP_V4L2_Init(v4l2src->index_id, obj->video_fd);

  /* activate settings for first frame */
  v4l2src->ctrl_time = 0;
  gst_object_sync_values (GST_OBJECT (src), v4l2src->ctrl_time);

  v4l2src->has_bad_timestamp = FALSE;
  v4l2src->last_timestamp = 0;

  GST_INFO_OBJECT (v4l2src, "start");

  return TRUE;
}

static gboolean
gst_bmv4l2src_stop (GstBaseSrc * src)
{
  GstBmV4l2Src *v4l2src = GST_BM_V4L2SRC (src);
  GstV4l2Object *obj = v4l2src->v4l2object;

  CVI_ISP_V4L2_Exit(obj->video_fd);

  if (GST_V4L2_IS_ACTIVE (obj)) {
    if (!gst_v4l2_object_stop (obj))
      return FALSE;
  }

  GST_INFO_OBJECT (v4l2src, "stop");

  return TRUE;
}

static void
gst_v4l2_src_parse_fixed_struct (GstStructure * s,
    gint * width, gint * height, gint * fps_n, gint * fps_d)
{
  if (gst_structure_has_field (s, "width") && width)
    gst_structure_get_int (s, "width", width);

  if (gst_structure_has_field (s, "height") && height)
    gst_structure_get_int (s, "height", height);

  if (gst_structure_has_field (s, "framerate") && fps_n && fps_d)
    gst_structure_get_fraction (s, "framerate", fps_n, fps_d);
}

static void
gst_v4l2_src_fixate_struct_with_preference (GstStructure * s,
    struct PreferredCapsInfo *pref)
{
  if (gst_structure_has_field (s, "width"))
    gst_structure_fixate_field_nearest_int (s, "width", pref->width);

  if (gst_structure_has_field (s, "height"))
    gst_structure_fixate_field_nearest_int (s, "height", pref->height);

  if (gst_structure_has_field (s, "framerate"))
    gst_structure_fixate_field_nearest_fraction (s, "framerate", pref->fps_n,
        pref->fps_d);
}

gst_v4l2src_fixed_caps_compare (GstCaps * caps_a, GstCaps * caps_b,
    struct PreferredCapsInfo *pref)
{
  GstStructure *a, *b;
  gint aw = G_MAXINT, ah = G_MAXINT;
  gint bw = G_MAXINT, bh = G_MAXINT;
  gint a_fps_n = G_MAXINT, a_fps_d = 1;
  gint b_fps_n = G_MAXINT, b_fps_d = 1;
  gint a_distance, b_distance;
  gint ret = 0;

  a = gst_caps_get_structure (caps_a, 0);
  b = gst_caps_get_structure (caps_b, 0);

  gst_v4l2_src_parse_fixed_struct (a, &aw, &ah, &a_fps_n, &a_fps_d);
  gst_v4l2_src_parse_fixed_struct (b, &bw, &bh, &b_fps_n, &b_fps_d);

  // Sort first the one with closest framerate to preference. Note that any
  // framerate lower then 1 frame per second will be considered the same. In
  // practice this should be fine considering that these framerate only exists
  // for still picture, in which case the resolution is most likely the key.
  a_distance = ABS ((a_fps_n / a_fps_d) - (pref->fps_n / pref->fps_d));
  b_distance = ABS ((b_fps_n / b_fps_d) - (pref->fps_n / pref->fps_d));
  if (a_distance != b_distance)
    return a_distance - b_distance;

  // If same framerate, sort first the one with closest resolution to preference
  a_distance = ABS (aw * ah - pref->width * pref->height);
  b_distance = ABS (bw * bh - pref->width * pref->height);

  /* If the distance are equivalent, maintain the order */
  if (a_distance == b_distance)
    ret = 1;
  else
    ret = a_distance - b_distance;

  GST_TRACE ("Placing %" GST_PTR_FORMAT " %s %" GST_PTR_FORMAT,
      caps_a, ret > 0 ? "after" : "before", caps_b);

  return ret;
}

static GstCaps *
gst_v4l2src_fixate (GstBaseSrc * basesrc, GstCaps * caps,
    struct PreferredCapsInfo *pref)
{
  GstBmV4l2Src *v4l2src = GST_BM_V4L2SRC (basesrc);
  GstV4l2Object *obj = v4l2src->v4l2object;
  GList *caps_list = NULL;
  GstStructure *s;
  gint i = G_MAXINT;
  GstV4l2Error error = GST_V4L2_ERROR_INIT;
  GstCaps *fcaps = NULL;

  GST_DEBUG_OBJECT (basesrc, "Fixating caps %" GST_PTR_FORMAT, caps);
  GST_INFO_OBJECT (basesrc, "Preferred size %ix%i", pref->width, pref->height);

  /* Sort the structures to get the caps that is nearest to our preferences,
   * first. Use single struct caps for sorting so we preserve the features.  */
  for (i = 0; i < gst_caps_get_size (caps); i++) {
    GstCaps *tmp = gst_caps_copy_nth (caps, i);

    s = gst_caps_get_structure (tmp, 0);
    gst_v4l2_src_fixate_struct_with_preference (s, pref);

    caps_list = g_list_insert_sorted_with_data (caps_list, tmp,
        (GCompareDataFunc) gst_v4l2src_fixed_caps_compare, pref);
  }

  gst_caps_unref (caps);
  caps = gst_caps_new_empty ();

  while (caps_list) {
    GstCaps *tmp = caps_list->data;
    caps_list = g_list_delete_link (caps_list, caps_list);
    gst_caps_append (caps, tmp);
  }

  GST_INFO_OBJECT (basesrc, "sorted and normalized caps %" GST_PTR_FORMAT,
      caps);

  /* Each structure in the caps has been fixated, except for the
   * interlace-mode and colorimetry. Now normalize the caps so we can
   * enumerate the possibilities */
  caps = gst_caps_normalize (caps);

  /* try hard to avoid TRY_FMT since some UVC camera just crash when this
   * is called at run-time. */
  if (gst_v4l2_object_caps_is_subset (obj, caps)) {
    fcaps = gst_v4l2_object_get_current_caps (obj);
    GST_DEBUG_OBJECT (basesrc, "reuse current caps %" GST_PTR_FORMAT, fcaps);
    goto out;
  }

  for (i = 0; i < gst_caps_get_size (caps); ++i) {
    gst_v4l2_clear_error (&error);
    if (fcaps)
      gst_caps_unref (fcaps);

    fcaps = gst_caps_copy_nth (caps, i);

    /* Just check if the format is acceptable, once we know
     * no buffers should be outstanding we try S_FMT.
     *
     * Basesrc will do an allocation query that
     * should indirectly reclaim buffers, after that we can
     * set the format and then configure our pool */
    if (gst_v4l2_object_try_format (obj, fcaps, &error)) {
      /* make sure the caps changed before doing anything */
      if (gst_v4l2_object_caps_equal (obj, fcaps))
        break;

      // v4l2src->renegotiation_adjust = v4l2src->offset + 1;
      v4l2src->pending_set_fmt = TRUE;
      break;
    }

    /* Only EIVAL make sense, report any other errors, this way we don't keep
     * probing if the device got disconnected, or if it's firmware stopped
     * responding */
    if (error.error->code != GST_RESOURCE_ERROR_SETTINGS) {
      i = G_MAXINT;
      break;
    }
  }

  if (i >= gst_caps_get_size (caps)) {
    gst_v4l2_error (v4l2src, &error);
    if (fcaps)
      gst_caps_unref (fcaps);
    gst_caps_unref (caps);
    return NULL;
  }

out:
  gst_caps_unref (caps);

  GST_INFO_OBJECT (basesrc, "fixated caps %" GST_PTR_FORMAT, fcaps);

  return fcaps;
}

static gboolean
gst_bmv4l2src_negotiate (GstBaseSrc * basesrc)
{
  GstBmV4l2Src *v4l2src = GST_BM_V4L2SRC (basesrc);
  GstCaps *thiscaps;
  GstCaps *caps = NULL;
  GstCaps *peercaps = NULL;
  gboolean result = TRUE;
  gboolean have_pref;
  /* Let's prefer a good resolution as of today's standard. */
  struct PreferredCapsInfo pref = {
    1920, 1080, 30, 1
  };

  /* first see what is possible on our source pad */
  thiscaps = gst_pad_query_caps (GST_BASE_SRC_PAD (basesrc), NULL);
  GST_INFO_OBJECT (basesrc, "caps of src: %" GST_PTR_FORMAT, thiscaps);

  /* nothing or anything is allowed, we're done */
  if (thiscaps == NULL || gst_caps_is_any (thiscaps))
    goto no_nego_needed;

  // /* get the peer caps without a filter as we'll filter ourselves later on */
  // peercaps = gst_pad_peer_query_caps (GST_BASE_SRC_PAD (basesrc), NULL);
  // GST_INFO_OBJECT (basesrc, "caps of peer: %" GST_PTR_FORMAT, peercaps);
  // if (peercaps && !gst_caps_is_any (peercaps)) {
  //   /* Prefer the first caps we are compatible with that the peer proposed */
  //   caps = gst_caps_intersect_full (peercaps, thiscaps,
  //       GST_CAPS_INTERSECT_FIRST);

  //   GST_INFO_OBJECT (basesrc, "intersect: %" GST_PTR_FORMAT, caps);

  //   gst_caps_unref (thiscaps);
  // } else {
    /* no peer or peer have ANY caps, work with our own caps then */
    caps = thiscaps;
  // }

  if (caps) {
    /* now fixate */
    if (!gst_caps_is_empty (caps)) {

      /* otherwise consider the first structure from peercaps to be a
       * preference. This is useful for matching a reported native display,
       * or simply to avoid transformation to happen downstream. */
      if (!have_pref && peercaps && !gst_caps_is_any (peercaps)) {
        GstStructure *pref_s = gst_caps_get_structure (peercaps, 0);
        pref_s = gst_structure_copy (pref_s);
        // gst_v4l2_src_fixate_struct_with_preference (pref_s, &pref);
        // gst_v4l2_src_parse_fixed_struct (pref_s, &pref.width, &pref.height,
        //     &pref.fps_n, &pref.fps_d);
        gst_structure_free (pref_s);
      }

      caps = gst_v4l2src_fixate (basesrc, caps, &pref);

      /* Fixating may fail as we now set the selected format */
      if (!caps) {
        result = FALSE;
        goto done;
      }

      GST_INFO_OBJECT (basesrc, "fixated to: %" GST_PTR_FORMAT, caps);

      if (gst_caps_is_any (caps)) {
        /* hmm, still anything, so element can do anything and
         * nego is not needed */
        result = TRUE;
      } else if (gst_caps_is_fixed (caps)) {
        /* yay, fixed caps, use those then */
        result = gst_base_src_set_caps (basesrc, caps);
      }
    }
    gst_caps_unref (caps);
  }

done:
  if (peercaps)
    gst_caps_unref (peercaps);

  GST_INFO_OBJECT (v4l2src, "negotiate done");

  return result;

no_nego_needed:
  {
    GST_INFO_OBJECT (basesrc, "no negotiation needed");
    if (thiscaps)
      gst_caps_unref (thiscaps);
    return TRUE;
  }
}

static gboolean
gst_v4l2src_set_format (GstBmV4l2Src * v4l2src, GstCaps * caps)
{
  GstV4l2Object *obj;
  GstV4l2Error error;
  obj = v4l2src->v4l2object;

  /* make sure we stop capturing and dealloc buffers */
  if (!gst_v4l2_object_stop (obj))
    return FALSE;

  g_signal_emit (v4l2src, gst_v4l2_signals[SIGNAL_PRE_SET_FORMAT], 0,
      v4l2src->v4l2object->video_fd, caps);

  // if (!gst_v4l2src_do_source_crop (v4l2src))
  //   return FALSE;

  return gst_v4l2_object_set_format (obj, caps, &error);
}

static gboolean
gst_bmv4l2src_decide_allocation (GstBaseSrc * bsrc, GstQuery * query)
{
  GstBmV4l2Src *v4l2src = GST_BM_V4L2SRC (bsrc);
  GstBufferPool *bpool = gst_v4l2_object_get_buffer_pool (v4l2src->v4l2object);
  gboolean ret = TRUE;

  if (!bpool)
    GST_INFO_OBJECT (v4l2src, "buffer_pool is NULL!");

  if (v4l2src->pending_set_fmt) {
    GstCaps *caps = gst_pad_get_current_caps (GST_BASE_SRC_PAD (bsrc));
    GstV4l2Error error = GST_V4L2_ERROR_INIT;

    /* Setting the format replaces the current pool */
    gst_clear_object (&bpool);

    caps = gst_caps_make_writable (caps);

    ret = gst_v4l2src_set_format (v4l2src, caps);
    if (ret) {
      GstV4l2BufferPool *pool;
      bpool = gst_v4l2_object_get_buffer_pool (v4l2src->v4l2object);
      pool = GST_V4L2_BUFFER_POOL (bpool);
      gst_v4l2_buffer_pool_enable_resolution_change (pool);
    } else {
      gst_v4l2_error (v4l2src, &error);
    }

    gst_caps_unref (caps);
    v4l2src->pending_set_fmt = FALSE;
  } else if (gst_buffer_pool_is_active (bpool)) {
    /* Trick basesrc into not deactivating the active pool. Renegotiating here
     * would otherwise turn off and on the camera. */
    GstAllocator *allocator;
    GstAllocationParams params;
    GstBufferPool *pool;

    gst_base_src_get_allocator (bsrc, &allocator, &params);
    pool = gst_base_src_get_buffer_pool (bsrc);

    if (gst_query_get_n_allocation_params (query))
      gst_query_set_nth_allocation_param (query, 0, allocator, &params);
    else
      gst_query_add_allocation_param (query, allocator, &params);

    if (gst_query_get_n_allocation_pools (query))
      gst_query_set_nth_allocation_pool (query, 0, pool,
        v4l2src->v4l2object->info.size, 1, 0);
    else
      gst_query_add_allocation_pool (query, pool, v4l2src->v4l2object->info.size, 1, 0);

    if (pool)
      gst_object_unref (pool);
    if (allocator)
      gst_object_unref (allocator);
    if (bpool)
      gst_object_unref (bpool);

    return GST_BASE_SRC_CLASS (parent_class)->decide_allocation (bsrc, query);
  }

  if (ret) {
    ret = gst_v4l2_object_decide_allocation (v4l2src->v4l2object, query);
    if (ret)
      ret = GST_BASE_SRC_CLASS (parent_class)->decide_allocation (bsrc, query);
  }

  if (ret) {
    if (!gst_buffer_pool_set_active (bpool, TRUE))
      goto activate_failed;
  }

  if (bpool)
    gst_object_unref (bpool);

  GST_INFO_OBJECT (v4l2src, "allocation done");

  return ret;

activate_failed:
  {
    GST_ELEMENT_ERROR (v4l2src, RESOURCE, SETTINGS,
        ("Failed to allocate required memory."),
        ("Buffer pool activation failed"));
    if (bpool)
      gst_object_unref (bpool);
    return FALSE;
  }
}

static gboolean
gst_bmv4l2src_handle_resolution_change (GstBmV4l2Src * v4l2src)
{
  GST_INFO_OBJECT (v4l2src, "Resolution change detected.");

  /* It is required to always cycle through streamoff, we also need to
   * streamoff in order to allow locking a new DV_TIMING which will
   * influence the output of TRY_FMT */
  gst_bmv4l2src_stop (GST_BASE_SRC (v4l2src));

  /* Force renegotiation */
  v4l2src->renegotiation_adjust = v4l2src->offset + 1;
  v4l2src->pending_set_fmt = TRUE;

  // return gst_base_src_negotiate (GST_BASE_SRC (v4l2src));
  return true;
}

static GstFlowReturn
gst_bmv4l2src_create (GstPushSrc * src, GstBuffer ** buf)
{
  GstBmV4l2Src *v4l2src = GST_BM_V4L2SRC (src);
  GstV4l2Object *obj = v4l2src->v4l2object;
  GstFlowReturn ret;
  GstClock *clock;
  GstClockTime abs_time, base_time, timestamp, duration;
  GstClockTime delay;
  GstMessage *qos_msg;
  gboolean half_frame;

  do {
    ret = GST_BASE_SRC_CLASS (parent_class)->alloc (GST_BASE_SRC (src), 0,
        obj->info.size, buf);

    if (G_UNLIKELY (ret != GST_FLOW_OK)) {
      if (ret == GST_V4L2_FLOW_RESOLUTION_CHANGE) {
        if (!gst_bmv4l2src_handle_resolution_change (v4l2src)) {
          ret = GST_FLOW_NOT_NEGOTIATED;
          goto error;
        }

        continue;
      }
      goto alloc_failed;
    }

    {
      GstV4l2BufferPool *obj_pool =
          GST_V4L2_BUFFER_POOL_CAST (gst_v4l2_object_get_buffer_pool (obj));

      ret = gst_v4l2_buffer_pool_process (obj_pool, buf, NULL);

      if (obj_pool)
        gst_object_unref (obj_pool);

      if (G_UNLIKELY (ret == GST_V4L2_FLOW_RESOLUTION_CHANGE)) {
        if (!gst_bmv4l2src_handle_resolution_change (v4l2src)) {
          ret = GST_FLOW_NOT_NEGOTIATED;
          goto error;
        }
      }
    }
  } while (ret == GST_V4L2_FLOW_CORRUPTED_BUFFER ||
      ret == GST_V4L2_FLOW_RESOLUTION_CHANGE);

  if (G_UNLIKELY (ret != GST_FLOW_OK))
    goto error;

  timestamp = GST_BUFFER_TIMESTAMP (*buf);
  duration = obj->duration;

  /* timestamps, LOCK to get clock and base time. */
  /* FIXME: element clock and base_time is rarely changing */
  GST_OBJECT_LOCK (v4l2src);
  if ((clock = GST_ELEMENT_CLOCK (v4l2src))) {
    /* we have a clock, get base time and ref clock */
    base_time = GST_ELEMENT (v4l2src)->base_time;
    gst_object_ref (clock);
  } else {
    /* no clock, can't set timestamps */
    base_time = GST_CLOCK_TIME_NONE;
  }
  GST_OBJECT_UNLOCK (v4l2src);

  /* sample pipeline clock */
  if (clock) {
    abs_time = gst_clock_get_time (clock);
    gst_object_unref (clock);
  } else {
    abs_time = GST_CLOCK_TIME_NONE;
  }

retry:
  if (!v4l2src->has_bad_timestamp && timestamp != GST_CLOCK_TIME_NONE) {
    struct timespec now;
    GstClockTime gstnow;

    /* v4l2 specs say to use the system time although many drivers switched to
     * the more desirable monotonic time. We first try to use the monotonic time
     * and see how that goes */
    clock_gettime (CLOCK_MONOTONIC, &now);
    gstnow = GST_TIMESPEC_TO_TIME (now);

    if (timestamp > gstnow || (gstnow - timestamp) > (10 * GST_SECOND)) {
      /* very large diff, fall back to system time */
      gstnow = g_get_real_time () * GST_USECOND;
    }

    /* Detect buggy drivers here, and stop using their timestamp. Failing any
     * of these condition would imply a very buggy driver:
     *   - Timestamp in the future
     *   - Timestamp is going backward compare to last seen timestamp
     *   - Timestamp is jumping forward for less then a frame duration
     *   - Delay is bigger then the actual timestamp
     * */
    if (timestamp > gstnow) {
      GST_WARNING_OBJECT (v4l2src,
          "Timestamp in the future detected, ignoring driver timestamps");
      v4l2src->has_bad_timestamp = TRUE;
      goto retry;
    }

    if (v4l2src->last_timestamp > timestamp) {
      GST_WARNING_OBJECT (v4l2src,
          "Timestamp going backward, ignoring driver timestamps");
      v4l2src->has_bad_timestamp = TRUE;
      goto retry;
    }

    delay = gstnow - timestamp;

    if (delay > timestamp) {
      GST_WARNING_OBJECT (v4l2src,
          "Timestamp does not correlate with any clock, ignoring driver timestamps");
      v4l2src->has_bad_timestamp = TRUE;
      goto retry;
    }

    /* Save last timestamp for sanity checks */
    v4l2src->last_timestamp = timestamp;

    GST_DEBUG_OBJECT (v4l2src, "ts: %" GST_TIME_FORMAT " now %" GST_TIME_FORMAT
        " delay %" GST_TIME_FORMAT, GST_TIME_ARGS (timestamp),
        GST_TIME_ARGS (gstnow), GST_TIME_ARGS (delay));
  } else {
    /* we assume 1 frame/field latency otherwise */
    if (GST_CLOCK_TIME_IS_VALID (duration))
      delay = duration;
    else
      delay = 0;
  }

  /* set buffer metadata */

  if (G_LIKELY (abs_time != GST_CLOCK_TIME_NONE)) {
    /* the time now is the time of the clock minus the base time */
    timestamp = abs_time - base_time;

    /* adjust for delay in the device */
    if (timestamp > delay)
      timestamp -= delay;
    else
      timestamp = 0;
  } else {
    timestamp = GST_CLOCK_TIME_NONE;
  }

  /* activate settings for next frame */
  if (GST_CLOCK_TIME_IS_VALID (duration)) {
    v4l2src->ctrl_time += duration;
  } else {
    /* this is not very good (as it should be the next timestamp),
     * still good enough for linear fades (as long as it is not -1)
     */
    v4l2src->ctrl_time = timestamp;
  }
  gst_object_sync_values (GST_OBJECT (src), v4l2src->ctrl_time);

  GST_LOG_OBJECT (src, "sync to %" GST_TIME_FORMAT " out ts %" GST_TIME_FORMAT,
      GST_TIME_ARGS (v4l2src->ctrl_time), GST_TIME_ARGS (timestamp));

  if (v4l2src->next_offset_same &&
      GST_BUFFER_OFFSET_IS_VALID (*buf) &&
      GST_BUFFER_OFFSET (*buf) != v4l2src->offset) {
    /* Probably had a lost field then, best to forget about last field. */
    GST_WARNING_OBJECT (v4l2src,
        "lost field detected - ts: %" GST_TIME_FORMAT,
        GST_TIME_ARGS (timestamp));
    v4l2src->next_offset_same = FALSE;
  }

  half_frame = (GST_BUFFER_FLAG_IS_SET (*buf, GST_VIDEO_BUFFER_FLAG_ONEFIELD));
  if (half_frame)
    v4l2src->next_offset_same = !v4l2src->next_offset_same;

  /* use generated offset values only if there are not already valid ones
   * set by the v4l2 device */
  if (!GST_BUFFER_OFFSET_IS_VALID (*buf)
      || !GST_BUFFER_OFFSET_END_IS_VALID (*buf)
      || GST_BUFFER_OFFSET (*buf) <=
      (v4l2src->offset - v4l2src->renegotiation_adjust)) {
    GST_BUFFER_OFFSET (*buf) = v4l2src->offset;
    GST_BUFFER_OFFSET_END (*buf) = v4l2src->offset + 1;
    if (!half_frame || !v4l2src->next_offset_same)
      v4l2src->offset++;
  } else {
    /* adjust raw v4l2 device sequence, will restart at null in case of renegotiation
     * (streamoff/streamon) */
    GST_BUFFER_OFFSET (*buf) += v4l2src->renegotiation_adjust;
    GST_BUFFER_OFFSET_END (*buf) += v4l2src->renegotiation_adjust;
    /* check for frame loss with given (from v4l2 device) buffer offset */
    if ((v4l2src->offset != 0)
        && (!half_frame || v4l2src->next_offset_same)
        && (GST_BUFFER_OFFSET (*buf) != (v4l2src->offset + 1))) {
      guint64 lost_frame_count = GST_BUFFER_OFFSET (*buf) - v4l2src->offset - 1;
      GST_WARNING_OBJECT (v4l2src,
          "lost frames detected: count = %" G_GUINT64_FORMAT " - ts: %"
          GST_TIME_FORMAT, lost_frame_count, GST_TIME_ARGS (timestamp));

      qos_msg = gst_message_new_qos (GST_OBJECT_CAST (v4l2src), TRUE,
          GST_CLOCK_TIME_NONE, GST_CLOCK_TIME_NONE, timestamp,
          GST_CLOCK_TIME_IS_VALID (duration) ? lost_frame_count *
          duration : GST_CLOCK_TIME_NONE);
      gst_element_post_message (GST_ELEMENT_CAST (v4l2src), qos_msg);

    }
    v4l2src->offset = GST_BUFFER_OFFSET (*buf);
  }

  GST_BUFFER_TIMESTAMP (*buf) = timestamp;
  GST_BUFFER_DURATION (*buf) = duration;

  return ret;

  /* ERROR */
alloc_failed:
  {
    if (ret != GST_FLOW_FLUSHING)
      GST_ELEMENT_ERROR (src, RESOURCE, NO_SPACE_LEFT,
          ("Failed to allocate a buffer"), (NULL));
    return ret;
  }
error:
  {
    gst_buffer_replace (buf, NULL);
    if (ret == GST_V4L2_FLOW_LAST_BUFFER) {
      GST_ELEMENT_ERROR (src, RESOURCE, FAILED,
          ("Driver returned a buffer with no payload, this most likely "
              "indicate a bug in the driver."), (NULL));
      ret = GST_FLOW_ERROR;
    } else {
      GST_DEBUG_OBJECT (src, "error processing buffer %d (%s)", ret,
          gst_flow_get_name (ret));
    }
    return ret;
  }
}

static void
gst_bmv4l2src_finalize (GstBmV4l2Src * v4l2src)
{
  gst_v4l2_object_destroy (v4l2src->v4l2object);

  G_OBJECT_CLASS (parent_class)->finalize ((GObject *) (v4l2src));
}

static GstStateChangeReturn
gst_bmv4l2src_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
  GstBmV4l2Src *v4l2src = GST_BM_V4L2SRC (element);
  GstV4l2Object *obj = v4l2src->v4l2object;
  GstV4l2Error error = GST_V4L2_ERROR_INIT;

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      /* open the device */
      if (!gst_v4l2_object_open (obj, &error)) {
        gst_v4l2_error (v4l2src, &error);
        return GST_STATE_CHANGE_FAILURE;
      }
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_NULL:
      /* close the device */
      if (!gst_v4l2_object_close (obj)) {
        GST_ERROR_OBJECT (v4l2src, "Fail to close v4l2_obj");
        return GST_STATE_CHANGE_FAILURE;
      }

      break;
    default:
      break;
  }

  return ret;
}

static GstCaps *
gst_bmv4l2src_get_caps (GstBaseSrc * src, GstCaps * filter)
{
  GstBmV4l2Src *v4l2src;
  GstV4l2Object *obj;

  v4l2src = GST_BM_V4L2SRC (src);
  obj = v4l2src->v4l2object;

  if (!GST_V4L2_IS_OPEN (obj)) {
    return gst_pad_get_pad_template_caps (GST_BASE_SRC_PAD (v4l2src));
  }

  return gst_v4l2_object_get_caps (obj, filter);
}

static gboolean
gst_bmv4l2src_query (GstBaseSrc * bsrc, GstQuery * query)
{
  GstBmV4l2Src *v4l2src;
  GstV4l2Object *obj;
  gboolean res = FALSE;

  v4l2src = GST_BM_V4L2SRC (bsrc);
  obj = v4l2src->v4l2object;

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_LATENCY:{
      GstClockTime min_latency, max_latency;
      guint32 fps_n, fps_d;
      guint num_buffers = 0;

      /* device must be open */
      if (!GST_V4L2_IS_OPEN (obj)) {
        GST_WARNING_OBJECT (v4l2src, "Can't give latency since device isn't open !");
        goto done;
      }

      fps_n = GST_V4L2_FPS_N (obj);
      fps_d = GST_V4L2_FPS_D (obj);

      /* we must have a framerate */
      if (fps_n <= 0 || fps_d <= 0) {
        GST_WARNING_OBJECT (v4l2src,
            "Can't give latency since framerate isn't fixated !");
        goto done;
      }

      /* min latency is the time to capture one frame/field */
      min_latency = gst_util_uint64_scale_int (GST_SECOND, fps_d, fps_n);
      if (GST_VIDEO_INFO_INTERLACE_MODE (&obj->info) ==
          GST_VIDEO_INTERLACE_MODE_ALTERNATE)
        min_latency /= 2;

      /* max latency is total duration of the frame buffer */
      {
        GstBufferPool *obj_pool = gst_v4l2_object_get_buffer_pool (obj);
        if (obj_pool != NULL) {
          num_buffers = GST_V4L2_BUFFER_POOL_CAST (obj_pool)->max_latency;
          gst_object_unref (obj_pool);
        }
      }

      if (num_buffers == 0)
        max_latency = -1;
      else
        max_latency = num_buffers * min_latency;

      GST_INFO_OBJECT (bsrc,
          "report latency min %" GST_TIME_FORMAT " max %" GST_TIME_FORMAT,
          GST_TIME_ARGS (min_latency), GST_TIME_ARGS (max_latency));

      /* we are always live, the min latency is 1 frame and the max latency is
       * the complete buffer of frames. */
      gst_query_set_latency (query, TRUE, min_latency, max_latency);

      res = TRUE;
      break;
    }
    default:
      res = GST_BASE_SRC_CLASS (parent_class)->query (bsrc, query);
      break;
  }

done:
  return res;
}

static void
gst_bmv4l2src_class_init (GstBmV4l2SrcClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *element_class;
  GstBaseSrcClass *basesrc_class;
  GstPushSrcClass *pushsrc_class;
  GError *err = NULL;

  gobject_class = G_OBJECT_CLASS (klass);
  element_class = GST_ELEMENT_CLASS (klass);
  basesrc_class = GST_BASE_SRC_CLASS (klass);
  pushsrc_class = GST_PUSH_SRC_CLASS (klass);

  gobject_class->set_property = GST_DEBUG_FUNCPTR (gst_bmv4l2src_set_property);
  gobject_class->get_property = GST_DEBUG_FUNCPTR (gst_bmv4l2src_get_property);
  gobject_class->finalize = (GObjectFinalizeFunc) GST_DEBUG_FUNCPTR (gst_bmv4l2src_finalize);

  element_class->change_state = GST_DEBUG_FUNCPTR (gst_bmv4l2src_change_state);

  basesrc_class->get_caps = GST_DEBUG_FUNCPTR (gst_bmv4l2src_get_caps);
  basesrc_class->start = GST_DEBUG_FUNCPTR(gst_bmv4l2src_start);
  basesrc_class->stop = GST_DEBUG_FUNCPTR(gst_bmv4l2src_stop);
  basesrc_class->query = GST_DEBUG_FUNCPTR (gst_bmv4l2src_query);
  basesrc_class->negotiate = GST_DEBUG_FUNCPTR (gst_bmv4l2src_negotiate);
  basesrc_class->decide_allocation = GST_DEBUG_FUNCPTR (gst_bmv4l2src_decide_allocation);

  pushsrc_class->create = GST_DEBUG_FUNCPTR (gst_bmv4l2src_create);

  klass->v4l2_class_devices = NULL;

  g_object_class_install_property (gobject_class, PROP_DEVICE_PATH,
      g_param_spec_string ("device", "Device Path",
          "The Path of the device node", "",
          G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_CAST_DMABUF,
      g_param_spec_uint ("dmabuf", "dmabuf mode",
          "set to need cast buffer to dmabuf",
          0, G_MAXUINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  /**
   * GstV4l2Src::prepare-format:
   * @v4l2src: the v4l2src instance
   * @fd: the file descriptor of the current device
   * @caps: the caps of the format being set
   *
   * This signal gets emitted before calling the v4l2 VIDIOC_S_FMT ioctl
   * (set format). This allows for any custom configuration of the device to
   * happen prior to the format being set.
   * This is mostly useful for UVC H264 encoding cameras which need the H264
   * Probe & Commit to happen prior to the normal Probe & Commit.
   */
  gst_v4l2_signals[SIGNAL_PRE_SET_FORMAT] = g_signal_new ("prepare-format",
      G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST,
      0, NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_INT, GST_TYPE_CAPS);
}

static void
gst_bmv4l2src_subclass_init(gpointer glass, gpointer class_data)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS(glass);
  GstBmV4l2SrcClassData *cdata = class_data;

  gst_element_class_add_pad_template(
      element_class, gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                                          cdata->src_caps));

  gst_element_class_set_static_metadata(element_class,
                                        "SOPHGO V4l2Src",
                                        "Video/Source",
                                        "Reads frames from sophgo vi/v4l2 device",
                                        "xx <xx@sophgo.com>");

  gst_caps_unref(cdata->sink_caps);
  gst_caps_unref(cdata->src_caps);
  g_free(cdata);
}

static void
gst_bmv4l2src_init(GstBmV4l2Src *v4l2src)
{
  //TODO
}

static void
gst_bmv4l2src_subinstance_init(GTypeInstance G_GNUC_UNUSED *instance,
                             gpointer G_GNUC_UNUSED g_class)
{
  // TODO: do init
}

void gst_bmv4l2src_register(GstPlugin *plugin,
                             GstCaps *sink_caps, GstCaps *src_caps)
{
  GType type;
  gchar *type_name;
  GstBmV4l2SrcClassData *class_data;
  GTypeInfo type_info = {
    sizeof (GstBmV4l2SrcClass),
    NULL,
    NULL,
    (GClassInitFunc) gst_bmv4l2src_subclass_init,
    NULL,
    NULL,
    sizeof (GstBmV4l2Src),
    0,
    (GInstanceInitFunc) gst_bmv4l2src_subinstance_init,
    NULL,
  };

  class_data = g_new0 (GstBmV4l2SrcClassData, 1);
  class_data->sink_caps = gst_caps_ref (sink_caps);
  class_data->src_caps = gst_caps_ref (src_caps);

  type_name = g_strdup (GST_BM_V4L2SRC_TYPE_NAME);
  type_info.class_data = class_data;
  type = g_type_register_static(GST_TYPE_BM_V4L2SRC, type_name, &type_info, 0);

  if (!gst_element_register(plugin, type_name, GST_RANK_PRIMARY + 1, type)) {
    GST_WARNING("Failed to register V4l2Src plugin '%s'", type_name);
  }

  g_free (type_name);
}

/* GstURIHandler interface */
static GstURIType
gst_bmv4l2src_uri_get_type (GType type)
{
  return GST_URI_SRC;
}

static const gchar *const *
gst_bmv4l2src_uri_get_protocols (GType type)
{
  static const gchar *protocols[] = { "v4l2", NULL };

  return protocols;
}

static gchar *
gst_bmv4l2src_uri_get_uri (GstURIHandler * handler)
{
  GstBmV4l2Src *v4l2src = GST_BM_V4L2SRC (handler);

  if (v4l2src->v4l2object->videodev != NULL) {
    return g_strdup_printf ("v4l2://%s", v4l2src->v4l2object->videodev);
  }

  return g_strdup ("v4l2://");
}

static gboolean
gst_bmv4l2src_uri_set_uri (GstURIHandler * handler, const gchar * uri, GError ** error)
{
  GstBmV4l2Src *v4l2src = GST_BM_V4L2SRC (handler);
  const gchar *device = DEFAULT_PROP_DEVICE;

  if (strcmp (uri, "v4l2://") != 0) {
    device = uri + 7;
  }
  g_object_set (v4l2src, "device", device, NULL);

  return TRUE;
}


static void
gst_bmv4l2src_uri_handler_init (gpointer g_iface, gpointer iface_data)
{
  GstURIHandlerInterface *iface = (GstURIHandlerInterface *) g_iface;

  iface->get_type = gst_bmv4l2src_uri_get_type;
  iface->get_protocols = gst_bmv4l2src_uri_get_protocols;
  iface->get_uri = gst_bmv4l2src_uri_get_uri;
  iface->set_uri = gst_bmv4l2src_uri_set_uri;
}