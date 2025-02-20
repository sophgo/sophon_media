
#include <stdio.h>
#include <stdlib.h>
#include <gst/base/gstbasetransform.h>
#include <gst/allocators/gstdmabuf.h>

#include "gstbmive.h"
#include "bmlib_runtime.h"
#include "gstbmallocator.h"
#include "bmcv_api_ext_c.h"

#define DEFAULT_PROP_TYPE 5
#define DEFAULT_PROP_MODE1 0
#define DEFAULT_PROP_MODE2 0
#define DEFAULT_PROC_IVE_TYPE 5
#define DEFAULT_PROP_FORMAT 14
#define DEFAULT_PROP_VALUE 0


GST_DEBUG_CATEGORY (gst_bm_ive_debug);
#define GST_CAT_DEFAULT gst_bm_ive_debug

extern bm_status_t bm_ive_image_calc_stride(bm_handle_t handle, int img_h, int img_w,
    bm_image_format_ext image_format, bm_image_data_format_ext data_type, int *stride);

static void gst_bm_ive_init(GstBmIVE *self);
// static gboolean gst_bm_ive_deinit(GstBaseTransform *transform);
static gboolean gst_bm_ive_start (GstBaseTransform *transform);
static gboolean gst_bm_ive_stop (GstBaseTransform *transform);
static GstCaps* gst_bmive_transform_caps(GstBaseTransform *transform,
                          GstPadDirection direction,
                          GstCaps *caps, GstCaps *filter);
static GstFlowReturn
gst_bm_ive_transform_frame (GstVideoFilter * filter, GstVideoFrame * inframe,
    GstVideoFrame * outframe);
static int gst_gstformat_to_bmformat(GstVideoFormat gst_format);
static GstVideoFormat gst_bmformat_to_gstformat(int bm_format);
static void
gst_bm_ive_get_property(GObject *object,
	guint prop_id, GValue *value, GParamSpec *pspec);
static void
gst_bm_ive_set_property(GObject *object,
	guint prop_id, const GValue *value, GParamSpec *pspec);

G_DEFINE_TYPE(GstBmIVE, gst_bm_ive, GST_TYPE_VIDEO_FILTER);



static
void gst_bm_ive_class_init (GstBmIVEClass *klass)
{
	GstBaseTransformClass *transform_class = GST_BASE_TRANSFORM_CLASS (klass);
	GstVideoFilterClass *video_filter_class = GST_VIDEO_FILTER_CLASS (klass);
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "bmive", 0, "sophon ive");

	transform_class->start = GST_DEBUG_FUNCPTR(gst_bm_ive_start);
	transform_class->stop  = GST_DEBUG_FUNCPTR(gst_bm_ive_stop);
	transform_class->transform_caps = GST_DEBUG_FUNCPTR(gst_bmive_transform_caps);
	video_filter_class->transform_frame = GST_DEBUG_FUNCPTR(gst_bm_ive_transform_frame);
	// transform_class->transform = GST_DEBUG_FUNCPTR(gst_bm_ive_transform);
	// transform_class->sink_event = GST_DEBUG_FUNCPTR(gst_bm_ive_sink_event);
	// transform_class->src_event = GST_DEBUG_FUNCPTR(gst_bm_ive_src_event);
	// transform_class->src_event = GST_DEBUG_FUNCPTR(gst_bm_ive_src_event);
	// transform_class->flush = GST_DEBUG_FUNCPTR(gst_bm_ive_flush);
	// transform_class->finish = GST_DEBUG_FUNCPTR(gst_bm_ive_finish);
	// transform_class->set_format = GST_DEBUG_FUNCPTR(gst_bm_ive_set_format);
	// transform_class->propose_allocation =
	// 		GST_DEBUG_FUNCPTR(gst_bm_ive_propose_allocation);
	// ive_class->transform = GST_DEBUG_FUNCPTR(gst_bm_ive_transform);

	gobject_class->set_property = GST_DEBUG_FUNCPTR (gst_bm_ive_set_property);
	gobject_class->get_property = GST_DEBUG_FUNCPTR (gst_bm_ive_get_property);


	// TODO: 注册属性，先设计好ive属性
	g_object_class_install_property (gobject_class, PROP_TYPE,
		g_param_spec_uint ("Type", "type of case",
			"type of case",
			0, G_MAXINT, DEFAULT_PROC_IVE_TYPE,
			G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (gobject_class, PROP_MODE1,
		g_param_spec_uint ("Mode1", "case mode1",
			"case mode1(depend on which case, like 0x1, 0x2)",
			0, G_MAXINT, DEFAULT_PROP_MODE1,
			G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (gobject_class, PROP_MODE2,
		g_param_spec_uint ("Mode2", "case mode2",
			"case mode2(depend on which case, like 0x1, 0x2)",
			0, G_MAXINT, DEFAULT_PROP_MODE2,
			G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (gobject_class, PROP_LOW_THR,
		g_param_spec_uint ("LowThr", "thresh low_thr value",
			"value low_thr(such as 0x20, 0x255)",
			0, G_MAXINT, DEFAULT_PROP_VALUE,
			G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (gobject_class, PROP_HIGHT_THR,
		g_param_spec_uint ("HightThr", "thresh high_thr value",
			"value low_thr(such as 0x20, 0x255)",
			0, G_MAXINT, DEFAULT_PROP_VALUE,
			G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (gobject_class, PROP_MIN_VAL,
		g_param_spec_uint ("MinVal", "min value",
			"min value(such as 0x20, 0x255)",
			0, G_MAXINT, DEFAULT_PROP_VALUE,
			G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (gobject_class, PROP_MID_VAL,
		g_param_spec_uint ("MidVal", "mid value",
			"mid value(such as 0x20, 0x255)",
			0, G_MAXINT, DEFAULT_PROP_VALUE,
			G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (gobject_class, PROP_MAX_VAL,
		g_param_spec_uint ("MaxVal", "max_val",
			"max val(such as 0x20, 0x255)",
			0, G_MAXINT, DEFAULT_PROP_VALUE,
			G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (gobject_class, PROP_NORM,
		g_param_spec_uint ("Norm", "value of norm",
			"norm used by normlized case(such as 0x1, 0x3)",
			0, G_MAXINT, DEFAULT_PROP_VALUE,
			G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (gobject_class, PROP_INIT_AREA_THR,
		g_param_spec_uint ("InitAreaThr", "value of area thresh",
			"area thr used by thresh case(such as 0x20, 0x255)",
			0, G_MAXINT, DEFAULT_PROP_VALUE,
			G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (gobject_class, PROP_STEP,
		g_param_spec_uint ("Step", "value of step",
			"step used by xxxx case(such as 0x1, 0x3)",
			0, G_MAXINT, DEFAULT_PROP_VALUE,
			G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (gobject_class, PROP_THR_VALUE,
		g_param_spec_uint ("ThrValue", "value of thresh",
			"thr value used by xxxx case(such as 0x1, 0x3)",
			0, G_MAXINT, DEFAULT_PROP_VALUE,
			G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (gobject_class, PROP_X,
		g_param_spec_uint ("X", "value of X",
			"thr X used by xxxx case(such as 0x1, 0x3)",
			0, G_MAXINT, DEFAULT_PROP_VALUE,
			G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (gobject_class, PROP_Y,
		g_param_spec_uint ("Y", "value of Y",
			"thr Y used by xxxx case(such as 0x1, 0x3)",
			0, G_MAXINT, DEFAULT_PROP_VALUE,
			G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	// element_class->change_state = GST_DEBUG_FUNCPTR (gst_bm_ive_change_state);
}


static void
gst_bm_ive_subclass_init(gpointer glass, gpointer class_data)
{
  /* subclass initialization logic */
  GstBmIVEClass *bmive_class = GST_BM_IVE_CLASS(glass);
  GstElementClass *element_class = GST_ELEMENT_CLASS(glass);
  GstBmIVEClassData *cdata = class_data;

  gst_element_class_add_pad_template(
      element_class, gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
                                          cdata->sink_caps));
  gst_element_class_add_pad_template(
      element_class, gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                                          cdata->src_caps));

  gst_element_class_set_static_metadata(element_class,
                                        "SOPHGO IVE",
                                        "Filter/Effect/Video",
                                        "SOPHGO Intelligent Video Engine",
                                        "ShiHuai Zhang <shihuai.zhang@sophgo.com>");

  bmive_class->soc_index = cdata->soc_index;

  gst_caps_unref(cdata->sink_caps);
  gst_caps_unref(cdata->src_caps);
  g_free(cdata);
}

static void
gst_bm_ive_subinstance_init(GTypeInstance G_GNUC_UNUSED *instance,
                             gpointer G_GNUC_UNUSED g_class)
{
  // TODO: do init
}
static void
gst_bm_ive_set_property(GObject *object,
	guint prop_id, const GValue *value, GParamSpec *pspec)
{
	GstBaseTransform *transform = GST_BASE_TRANSFORM(object);
	GstBmIVE *self = GST_BM_IVE (transform);
	gint v;

	v = g_value_get_uint (value);
	switch (prop_id) {
		case PROP_MODE1: {
			gint mode1 = v;
			if (self->mode1 == mode1)
				return;
			self->mode1 = mode1;
			break;
		}
		case PROP_MODE2: {
			gint mode2 = v;
			if (self->mode2 == mode2)
				return;
			self->mode2 = mode2;
			break;
		}
		case PROP_TYPE: {
			gint type = v;
			if (self->type == type)
				return;
			self->type = type;
			break;
		}
		case PROP_LOW_THR: {
			gint low_thr = v;
			if (self->low_thr == low_thr)
				return;
			self->low_thr = low_thr;
			break;
		}
		case PROP_HIGHT_THR: {
			gint hight_thr = v;
			if (self->hight_thr == hight_thr)
				return;
			self->hight_thr = hight_thr;
			break;
		}
		case PROP_MIN_VAL: {
			gint min_val = v;
			if (self->min_val == min_val)
				return;
			self->min_val = min_val;
			break;
		}
		case PROP_MID_VAL: {
			gint mid_val = v;
			if (self->mid_val == mid_val)
				return;
			self->mid_val = mid_val;
			break;
		}
		case PROP_MAX_VAL: {
			gint max_val = v;
			if (self->max_val == max_val)
				return;
			self->max_val = max_val;
			break;
		}
		case PROP_NORM: {
			gint norm = v;
			if (self->norm == norm)
				return;
			self->norm = norm;
			break;
		}
		case PROP_INIT_AREA_THR: {
			gint init_area_thr = v;
			if (self->init_area_thr == init_area_thr)
				return;
			self->init_area_thr = init_area_thr;
			break;
		}
		case PROP_STEP: {
			gint step = v;
			if (self->step == step)
				return;
			self->step = step;
			break;
		}
		case PROP_THR_VALUE: {
			gint thr_value = v;
			if (self->thr_value == thr_value)
				return;
			self->thr_value = thr_value;
			break;
		}
		case PROP_X: {
			gint x = v;
			if (self->x == x)
				return;
			self->x = x;
			break;
		}
		case PROP_Y: {
			gint y = v;
			if (self->y == y)
				return;
			self->y = y;
			break;
		}
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			return;
	}
	self->prop_dirty = TRUE;
	self->init_mem = FALSE;
}
static void
gst_bm_ive_get_property(GObject *object,
	guint prop_id, GValue *value, GParamSpec *pspec)
{
	GstBaseTransform *transform = GST_BASE_TRANSFORM(object);
	GstBmIVE *self = GST_BM_IVE (transform);
	switch (prop_id) {
		case PROP_MODE1:
			g_value_set_uint (value, self->mode1);
			break;
		case PROP_MODE2:
			g_value_set_uint (value, self->mode2);
			break;
		case PROP_TYPE:
			g_value_set_uint (value, self->type);
			break;
		case PROP_LOW_THR:
			g_value_set_uint (value, self->low_thr);
			break;
		case PROP_HIGHT_THR:
			g_value_set_uint (value, self->hight_thr);
			break;
		case PROP_MIN_VAL:
			g_value_set_uint (value, self->min_val);
			break;
		case PROP_MID_VAL:
			g_value_set_uint (value, self->mid_val);
			break;
		case PROP_MAX_VAL:
			g_value_set_uint (value, self->max_val);
			break;
		case PROP_NORM:
			g_value_set_uint (value, self->norm);
			break;
		case PROP_INIT_AREA_THR:
			g_value_set_uint (value, self->init_area_thr);
			break;
		case PROP_STEP:
			g_value_set_uint (value, self->step);
			break;
		case PROP_THR_VALUE:
			g_value_set_uint (value, self->thr_value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			return;
	}

}
#if 0
static void
gst_bm_ive_stop_task (GstBaseTransform *transform, gboolean drain)
{
	GstBmIve *self = GST_BM_IVE (transform);
	GstTask *task = transform->srcpad->task;

	if (!GST_BM_IVE_TASK_STARTED (transform))
		return;

	GST_DEBUG_OBJECT (self, "stopping ive thread");

	/* Discard pending frames */
	if (!drain)
		self->pending_frames = 0;

	GST_BM_IVE_BROADCAST (transform);
  	/* Wait for task thread to pause */
	if (task) {
		GST_OBJECT_LOCK (task);
		while (GST_TASK_STATE (task) == GST_TASK_STARTED)
		GST_TASK_WAIT (task);
		GST_OBJECT_UNLOCK (task);
	}
	gst_pad_stop_task (transform->srcpad);

}
static void
gst_bm_ive_reset (GstBaseTransform *transform, gboolean drain, gboolean final)
{
	GstBmIve *self = GST_BM_IVE (transform);
	GST_BM_ENC_LOCK (encoder);

	GST_DEBUG_OBJECT (self, "resetting");

	self->flushing = TRUE;
	self->draining = drain;

	gst_bm_ive_stop_task (transform, drain);

	self->flushing = TRUE;
	self->draining = drain;

	self->task_ret = GST_FLOW_OK;
	self->pending_frames = 0;

	if (self->frames) {
		g_list_free (self->frames);
		self->frames = NULL;
	}

	/* Force re-apply prop */
	self->prop_dirty = TRUE;

	GST_BM_IVE_UNLOCK (transform);

}

static GstStateChangeReturn
gst_bm_ive_change_state (GstElement *element, GstStateChange transition)
{
	GstBaseTransform *transform = GST_BASE_TRANSFORM(object);

	if (transition == GST_STATE_CHANGE_PAUSED_TO_READY) {
	GST_VIDEO_ENCODER_STREAM_LOCK (transform);
	gst_bm_ive_reset (transform, FALSE, TRUE);
	GST_VIDEO_ENCODER_STREAM_UNLOCK (transform);
	}

	return GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
}
#endif

static void
gst_bm_ive_init(GstBmIVE *self)
{

	self->allocator = gst_bm_allocator_new();
	GST_DEBUG_OBJECT (self, "ive init");

}

// static gboolean
// gst_bm_ive_deinit(GstBaseTransform *transform)
// {
// 	GstBmIVE *self = GST_BM_IVE (transform);

// 	GST_DEBUG_OBJECT (self, "ive deiniting.....");

// 	return TRUE;
// }

static gboolean
gst_bm_ive_stop (GstBaseTransform *transform)
{
	GstBmIVE *self = GST_BM_IVE (transform);
	gboolean ret = TRUE;

	GST_DEBUG_OBJECT (self, "stopping");

	self->init_mem = FALSE;

	if (!ret) {
		GST_ERROR("ive deinit failed\n");
		return FALSE;
	}

	return TRUE;
}
static gboolean
gst_bm_ive_start (GstBaseTransform *transform)
{
	GstBmIVE *self = GST_BM_IVE (transform);
	gboolean ret = TRUE;

	GST_DEBUG_OBJECT (self, "ive starting");


	self->allocator = gst_bm_allocator_new();
	if (!self->allocator) {
		GST_ERROR_OBJECT (self, "gst_bm_allocator_new fail\n");
		return FALSE;
	}

	if (!ret) {
		GST_ERROR("ive init failed\n");
		return FALSE;
	}


	GST_DEBUG_OBJECT (self, "started");

	return TRUE;
}
static GstCaps *
gst_bmive_transform_caps(GstBaseTransform *transform,
                          GstPadDirection direction,
                          GstCaps *caps, GstCaps *filter)
{
	GstCaps *ret = NULL;
	GstBmIVE *self = GST_BM_IVE(transform);

	if (direction == GST_PAD_SRC) {
		GstPad *sinkpad = GST_BASE_TRANSFORM_SINK_PAD(transform);
		if (sinkpad) {
			GstCaps *upstream_caps = gst_pad_peer_query_caps(sinkpad, filter);
			if (upstream_caps) {
				ret = gst_caps_copy(upstream_caps);
				gst_caps_unref(upstream_caps);
				GST_DEBUG_OBJECT(self, "sinkpad caps:%s\n", gst_caps_to_string(ret));
			}
		}
	} else if (direction == GST_PAD_SINK) {
		GstPad *srcpad = GST_BASE_TRANSFORM_SRC_PAD(transform);
		if (srcpad) {
			GstCaps *downstream_caps = gst_pad_peer_query_caps(srcpad, filter);
			if (downstream_caps) {
				ret = gst_caps_copy(downstream_caps);
				gst_caps_unref(downstream_caps);
				GST_DEBUG_OBJECT(self, "srcpad caps: %s\n", gst_caps_to_string(ret));
			}
		}
	}
	return ret;
}
static gboolean
gst_bm_ive_dma(GstVideoFilter * filter, struct bm_image *src, struct bm_image *dst)
{
	gboolean ret;
	bm_handle_t bm_handle = NULL;
	GstBmIVE *self = GST_BM_IVE (filter);
	bm_dev_request(&bm_handle, 0);
	if (self->mode1 == 0) {

		ret = bmcv_ive_dma(bm_handle, *src, *dst, IVE_DMA_DIRECT_COPY, NULL);

		if (ret) {
			GST_ERROR("ive_dma fail\n");
		}

	}else {
		GST_ERROR("other mode no support now\n");
		return FALSE;
	}

	return ret;
}
static gboolean
gst_bm_ive_hist(GstVideoFilter * filter, struct bm_image *src, struct bm_image *dst)
{
	gboolean ret;
	bm_handle_t bm_handle = NULL;
	GstBmIVE *self = GST_BM_IVE (filter);
	bm_device_mem_t *dst_mem;

	bm_dev_request(&bm_handle, 0);
	if (!self->init_mem) {

		ret = bm_malloc_device_byte(bm_handle, &self->mem, 1024);
		if (ret) {
			GST_ERROR("HIST malloc mem fail\n");
			return FALSE;
		}
		self->init_mem = TRUE;
	}
	dst_mem = &self->mem;

	ret = bmcv_ive_hist(bm_handle, *src, *dst_mem);

	if (ret) {
		GST_ERROR("ive_hist fail\n");
		return FALSE;
	}
	return ret;

}
static gboolean
gst_bm_ive_erode(GstVideoFilter * filter, struct bm_image *src, struct bm_image *dst)
{
	gboolean ret;
	bm_handle_t bm_handle = NULL;
	//GstBmIVE *self = GST_BM_IVE (filter);
	//bm_device_mem_t dst_mem;
	unsigned char arr3by3[25] = { 0, 0, 0, 0, 0, 0, 0, 255, 0, 0, 0, 255, 255,
                 255, 0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0 };

	bm_dev_request(&bm_handle, 0);

	ret = bmcv_ive_erode(bm_handle, *src, *dst, arr3by3);

	if (ret) {
		GST_ERROR("ive_erode fail\n");
		return FALSE;
	}
	return ret;

}
static gboolean
gst_bm_ive_dilate(GstVideoFilter * filter, struct bm_image *src, struct bm_image *dst)
{
	gboolean ret;
	bm_handle_t bm_handle = NULL;
	//GstBmIVE *self = GST_BM_IVE (filter);
	//bm_device_mem_t dst_mem;
	unsigned char arr3by3[25] = { 0, 0, 0, 0, 0, 0, 0, 255, 0, 0, 0, 255, 255,
                 255, 0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0 };

	bm_dev_request(&bm_handle, 0);

	ret = bmcv_ive_dilate(bm_handle, *src, *dst, arr3by3);

	if (ret) {
		GST_ERROR("ive_dilate fail\n");
		return FALSE;
	}
	return ret;

}
static gboolean
gst_bm_ive_thresh(GstVideoFilter * filter, struct bm_image *src, struct bm_image *dst)
{
	gboolean ret;
	bm_handle_t bm_handle = NULL;
	bmcv_ive_thresh_attr thresh_attr;
	GstBmIVE *self = GST_BM_IVE (filter);
	bmcv_ive_thresh_mode thresh_mode;

	thresh_attr.low_thr = self->low_thr;
	thresh_attr.high_thr = self->hight_thr;
	thresh_attr.min_val = self->min_val;
	thresh_attr.mid_val = self->mid_val;
	thresh_attr.max_val = self->max_val;
	thresh_mode = self->mode1;

	bm_dev_request(&bm_handle, 0);

	ret = bmcv_ive_thresh(bm_handle, *src, *dst, thresh_mode, thresh_attr);

	if (ret) {
		GST_ERROR("ive_thresh fail\n");
		return FALSE;
	}
	return ret;

}

static gboolean
gst_bm_ive_map(GstVideoFilter * filter, struct bm_image *src, struct bm_image *dst)
{
	gboolean ret;
	bm_handle_t bm_handle = NULL;
	//GstBmIVE *self = GST_BM_IVE (filter);
	bm_device_mem_t map_table;

	static unsigned char FixMap[256] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x03,
		0x03, 0x04, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
		0x0C, 0x0D, 0x0F, 0x10, 0x11, 0x12, 0x14, 0x15, 0x17, 0x18,
		0x1A, 0x1B, 0x1D, 0x1E, 0x20, 0x21, 0x23, 0x24, 0x26, 0x27,
		0x29, 0x2A, 0x2C, 0x2D, 0x2F, 0x31, 0x32, 0x34, 0x35, 0x37,
		0x38, 0x3A, 0x3B, 0x3D, 0x3E, 0x40, 0x41, 0x43, 0x44, 0x45,
		0x47, 0x48, 0x4A, 0x4B, 0x4D, 0x4E, 0x50, 0x51, 0x52, 0x54,
		0x55, 0x56, 0x58, 0x59, 0x5A, 0x5B, 0x5D, 0x5E, 0x5F, 0x60,
		0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x69, 0x6A, 0x6B, 0x6D,
		0x6E, 0x70, 0x71, 0x73, 0x75, 0x76, 0x78, 0x7A, 0x7B, 0x7D,
		0x7E, 0x80, 0x81, 0x83, 0x84, 0x86, 0x87, 0x88, 0x89, 0x8B,
		0x8C, 0x8D, 0x8E, 0x90, 0x92, 0x94, 0x97, 0x9A, 0x9C, 0x9E,
		0xA1, 0xA3, 0xA5, 0xA6, 0xA7, 0xA9, 0xAA, 0xAB, 0xAC, 0xAC,
		0xAD, 0xAE, 0xAF, 0xB0, 0xB1, 0xB3, 0xB4, 0xB5, 0xB7, 0xB9,
		0xBB, 0xBD, 0xBF, 0xC1, 0xC4, 0xC7, 0xCC, 0xD1, 0xD5, 0xDA,
		0xDE, 0xE0, 0xE2, 0xE3, 0xE4, 0xE5, 0xE5, 0xE6, 0xE6, 0xE6,
		0xE6, 0xE6, 0xE7, 0xE7, 0xE7, 0xE8, 0xE8, 0xE9, 0xEA, 0xEC,
		0xED, 0xEE, 0xF0, 0xF2, 0xF4, 0xF5, 0xF7, 0xF8, 0xFA, 0xFB,
		0xFD, 0xFE, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
	};

	bm_dev_request(&bm_handle, 0);

    ret = bm_malloc_device_byte(bm_handle, &map_table, MAP_TABLE_SIZE);
    if (ret != BM_SUCCESS) {
        GST_ERROR("bm_memcpy_s2d failed . ret = %d\n", ret);
		return FALSE;
    }
    ret = bm_memcpy_s2d(bm_handle, map_table, FixMap);
    if(ret != BM_SUCCESS){
        GST_ERROR("bm_memcpy_s2d failed . ret = %d\n", ret);
		return FALSE;
    }
	// if (!self->mem) {

	// 	ret = bm_malloc_device_byte(bm_handle, &map_table, 256);
	// 	if (ret) {
	// 		GST_ERROR("HIST malloc mem fail\n");
	// 		return FALSE;
	// 	}
	// 	self->mem = &map_table;
	// 	ret = bm_memcpy_s2d(bm_handle, map_table, FixMap);
	// 	if(ret){
	// 		printf("bm_memcpy_s2d failed . ret = %d\n", ret);
	// 		return FALSE;
	// 	}
	// }

	ret = bmcv_ive_map(bm_handle, *src, *dst, map_table);

	if (ret) {
		GST_ERROR("ive_map fail\n");
		return FALSE;
	}
	return ret;

}
static gboolean
gst_bm_ive_lbp(GstVideoFilter * filter, struct bm_image *src, struct bm_image *dst)
{
	gboolean ret;
	bm_handle_t bm_handle = NULL;
	GstBmIVE *self = GST_BM_IVE (filter);
	//bm_device_mem_t dst_mem;
	bmcv_ive_lbp_ctrl_attr lbp_ctrl;

	bm_dev_request(&bm_handle, 0);

	lbp_ctrl.en_mode = self->mode1;
	lbp_ctrl.un8_bit_thr.s8_val = (self->mode1 == BM_IVE_LBP_CMP_MODE_ABS ? 35 : 41);

	ret = bmcv_ive_lbp(bm_handle, *src, *dst, lbp_ctrl);
	if (ret) {
		GST_ERROR("ive_lbp fail\n");
		return FALSE;
	}
	return ret;

}
static gboolean
gst_bm_ive_filter(GstVideoFilter * filter, struct bm_image *src, struct bm_image *dst)
{
	gboolean ret;
	bm_handle_t bm_handle = NULL;
	GstBmIVE *self = GST_BM_IVE (filter);
	//bm_device_mem_t dst_mem;
	signed char arr3by3[25] = { 0, 0, 0, 0, 0, 0, 1, 2, 1, 0, 0, 2, 4,
                    2, 0, 0, 1, 2, 1, 0, 0, 0, 0, 0, 0 };
	bmcv_ive_filter_ctrl filterAttr;

	bm_dev_request(&bm_handle, 0);

	filterAttr.u8_norm = self->norm;
	memcpy(filterAttr.as8_mask, arr3by3, 5 * 5 * sizeof(signed char));

	ret = bmcv_ive_filter(bm_handle, *src, *dst, filterAttr);

	if (ret) {
		GST_ERROR("ive_filter fail\n");
		return FALSE;
	}
	return ret;

}
static gboolean
gst_bm_ive_ccl(GstVideoFilter * filter, struct bm_image *src, struct bm_image *dst)
{
	gboolean ret;
	bm_handle_t bm_handle = NULL;
	GstBmIVE *self = GST_BM_IVE (filter);
	bmcv_ive_ccl_attr ccl_attr;
	bmcv_ive_ccblob *ccblob = NULL;

	bm_dev_request(&bm_handle, 0);
	ccblob = (bmcv_ive_ccblob *)malloc(sizeof(bmcv_ive_ccblob));
	memset(ccblob, 0, sizeof(bmcv_ive_ccblob));
	if (!self->init_mem) {
		ret = bm_malloc_device_byte(bm_handle, &self->mem, sizeof(bmcv_ive_ccblob));
		if (ret) {
			GST_ERROR("CCL malloc mem fail\n");
			return FALSE;
		}
		self->init_mem = TRUE;
	}

	ret = bm_memcpy_s2d(bm_handle, self->mem, (void *)ccblob);

    ccl_attr.en_mode = self->mode1;
    ccl_attr.u16_init_area_thr = self->init_area_thr;
    ccl_attr.u16_step = self->step;

	ret = bmcv_ive_ccl(bm_handle, *src, self->mem, ccl_attr);

	if (ret) {
		GST_ERROR("ive_ccl fail\n");
		return FALSE;
	}
	return ret;

}
static gboolean
gst_bm_ive_intg(GstVideoFilter * filter, struct bm_image *src, struct bm_image *dst)
{
	gboolean ret;
	bm_handle_t bm_handle = NULL;
	bm_device_mem_t *dst_mem;
	GstBmIVE *self = GST_BM_IVE (filter);
	bmcv_ive_integ_ctrl_s intg_attr;

	intg_attr.en_out_ctrl = self->mode1;

	bm_dev_request(&bm_handle, 0);


	if (!self->init_mem) {
		bm_malloc_device_byte(bm_handle, &self->mem, 1024);
		self->init_mem = TRUE;
	}
	dst_mem = &self->mem;

	ret = bmcv_ive_integ(bm_handle, *src, *dst_mem, intg_attr);

	if (ret) {
		GST_ERROR("ive_intg fail\n");
		return FALSE;
	}

	return ret;
}
static gboolean
gst_bm_ive_ncc(GstVideoFilter * filter, struct bm_image *src, struct bm_image *dst)
{
	gboolean ret;
	bm_handle_t bm_handle = NULL;
	GstBmIVE *self = GST_BM_IVE (filter);

	bm_dev_request(&bm_handle, 0);

	if (!self->init_mem) {
		bm_malloc_device_byte(bm_handle, &self->mem, sizeof(bmcv_ive_ncc_dst_mem_t));
		self->init_mem = TRUE;
	}

	ret = bmcv_ive_ncc(bm_handle, *src, *src, self->mem);
	if (ret) {
		GST_ERROR("ive_ncc fail\n");
		return FALSE;
	}

	return ret;

}
static gboolean
gst_bm_ive_ordstatafilter(GstVideoFilter * filter, struct bm_image *src, struct bm_image *dst)
{
	gboolean ret;
	bm_handle_t bm_handle = NULL;
	GstBmIVE *self = GST_BM_IVE (filter);
	bmcv_ive_ord_stat_filter_mode ordstatfilter_mode;

	ordstatfilter_mode = self->mode1;
	bm_dev_request(&bm_handle, 0);

	ret = bmcv_ive_ord_stat_filter(bm_handle, *src, *dst, ordstatfilter_mode);

	if (ret) {
		GST_ERROR("ive_ordstatafilter fail\n");
		return FALSE;
	}

	return ret;
}
static gboolean
gst_bm_ive_magandang(GstVideoFilter * filter, struct bm_image *src, struct bm_image *dst)
{
	gboolean ret;
	bm_handle_t bm_handle = NULL;
	GstBmIVE *self = GST_BM_IVE (filter);
	bmcv_ive_mag_and_ang_ctrl magandang_attr;

	memset(&magandang_attr, 0, sizeof(bmcv_ive_mag_and_ang_ctrl));
	magandang_attr.en_out_ctrl = self->mode1;
	magandang_attr.u16_thr = self->thr_value;
	bm_dev_request(&bm_handle, 0);

	if(magandang_attr.en_out_ctrl == BM_IVE_MAG_AND_ANG_OUT_ALL) {
		GST_ERROR(" BM_IVE_MAG_AND_ANG_OUT_ALL not support!\n");
		return FALSE;
	}

	ret = bmcv_ive_mag_and_ang(bm_handle, src, dst, NULL, magandang_attr);

	if (ret) {
		GST_ERROR("ive_magandang fail\n");
		return FALSE;
	}

	return ret;

}
static gboolean
gst_bm_ive_normgrad(GstVideoFilter * filter, struct bm_image *src, struct bm_image *dst)
{
	gboolean ret;
	bm_handle_t bm_handle = NULL;
	GstBmIVE *self = GST_BM_IVE (filter);
	bmcv_ive_normgrad_ctrl normgrad_attr;

	signed char arr3by3[25] = { 0, 0, 0, 0,  0, 0, -1, 0, 1, 0, 0, -2, 0,
                   2, 0, 0, -1, 0, 1, 0,  0, 0, 0, 0, 0 };
	normgrad_attr.en_mode = self->mode1;
	normgrad_attr.u8_norm = 8;
	memcpy(normgrad_attr.as8_mask, arr3by3, 5 * 5 * sizeof(signed char));

	bm_dev_request(&bm_handle, 0);

	switch (normgrad_attr.en_mode)
	{
		case BM_IVE_NORM_GRAD_OUT_HOR_AND_VER:
			GST_ERROR(" mode BM_IVE_NORM_GRAD_OUT_HOR_AND_VER not support!\n");
			break;
		case BM_IVE_NORM_GRAD_OUT_HOR:
			ret = bmcv_ive_norm_grad(bm_handle, src, dst, NULL, NULL, normgrad_attr);
			break;
		case BM_IVE_NORM_GRAD_OUT_VER:
			ret = bmcv_ive_norm_grad(bm_handle, src, NULL, dst, NULL, normgrad_attr);
			break;
		case BM_IVE_NORM_GRAD_OUT_COMBINE:
			ret = bmcv_ive_norm_grad(bm_handle, src, NULL, NULL, dst, normgrad_attr);
			break;
		default:
			GST_ERROR("normgrad not support mode: %d\n", normgrad_attr.en_mode);
			break;
	}

	return ret;
}
static gboolean
gst_bm_ive_cannyhysedge(GstVideoFilter * filter, struct bm_image *src, struct bm_image *dst)
{
	gboolean ret;
	bm_handle_t bm_handle = NULL;
	GstBmIVE *self = GST_BM_IVE (filter);
	bmcv_ive_canny_hys_edge_ctrl cannyHysEdgeAttr;
	memset(&cannyHysEdgeAttr, 0, sizeof(bmcv_ive_canny_hys_edge_ctrl));
	unsigned char *edge_res;

	signed char arr3by3[25] = { 0, 0, 0, 0,  0, 0, -1, 0, 1, 0, 0, -2, 0,
                   2, 0, 0, -1, 0, 1, 0,  0, 0, 0, 0, 0 };

	cannyHysEdgeAttr.u16_low_thr =  42;
	cannyHysEdgeAttr.u16_high_thr = 3 * cannyHysEdgeAttr.u16_low_thr;
	memcpy(cannyHysEdgeAttr.as8_mask, arr3by3, 5 * 5 * sizeof(signed char));

	if (!self->init_mem) {
		edge_res = malloc(src->height * src->width * sizeof(unsigned char));
		memcpy(&self->mem, edge_res, src->height * src->width * sizeof(unsigned char));
		self->init_mem = TRUE;
	}
	edge_res = (unsigned char *)&self->mem;

	bm_dev_request(&bm_handle, 0);

	ret = bmcv_ive_canny(bm_handle, *src, bm_mem_from_system((void *)edge_res), cannyHysEdgeAttr);

	return ret;
}
static gboolean
gst_bm_ive_stcandicorner(GstVideoFilter * filter, struct bm_image *src, struct bm_image *dst)
{
	gboolean ret;
	bm_handle_t bm_handle = NULL;
	GstBmIVE *self = GST_BM_IVE (filter);
	bmcv_ive_stcandicorner_attr stCandiCorner_attr;
	memset(&stCandiCorner_attr, 0, sizeof(bmcv_ive_stcandicorner_attr));
	int attr_len;

	bm_dev_request(&bm_handle, 0);
	if (!self->init_mem) {
		int stride;
		bm_ive_image_calc_stride(bm_handle, src->height, src->height, src->image_format, DATA_TYPE_EXT_1N_BYTE, &stride);
		attr_len = 4 * src->height * stride + sizeof(bmcv_ive_st_max_eig);
		ret = bm_malloc_device_byte(bm_handle, &self->mem, attr_len * sizeof(unsigned char));
		self->init_mem = TRUE;
	}

	memcpy(&stCandiCorner_attr.st_mem, &self->mem, attr_len * sizeof(unsigned char));

	ret = bmcv_ive_stcandicorner(bm_handle, *src, *dst, stCandiCorner_attr);

	return ret;
}
static gboolean
gst_bm_ive_gradfg(GstVideoFilter * filter, struct bm_image *src, struct bm_image *dst)
{
	gboolean ret;
	bm_handle_t bm_handle = NULL;
	GstBmIVE *self = GST_BM_IVE (filter);
	bmcv_ive_gradfg_attr gradFgAttr;
	bm_image stCurGrad, stBgGrad;
	guint height = src->height;
	guint width = src->width;
	bm_image_format_ext fmt = FORMAT_GRAY;
	int u16Stride;

	bm_dev_request(&bm_handle, 0);
	signed char arr3by3[25] = { 0, 0, 0, 0, 0, 0, -1, 0, 1, 0, 0, -2, 0,
                2, 0, 0, -1, 0, 1, 0, 0, 0, 0, 0, 0 };

    bmcv_ive_normgrad_ctrl normGradAttr;
    normGradAttr.en_mode = BM_IVE_NORM_GRAD_OUT_COMBINE;
    normGradAttr.u8_norm = 8;
    memcpy(&normGradAttr.as8_mask, arr3by3, 5 * 5 * sizeof(signed char));

    gradFgAttr.en_mode = self->mode1;
    gradFgAttr.u16_edw_factor = 1000;
    gradFgAttr.u8_crl_coef_thr = 80;
    gradFgAttr.u8_mag_crl_thr = 4;
    gradFgAttr.u8_min_mag_diff = 2;
    gradFgAttr.u8_noise_val = 1;
    gradFgAttr.u8_edw_dark = 1;

    // calc ive image stride && create bm image struct
    bm_ive_image_calc_stride(bm_handle, height, width, fmt, DATA_TYPE_EXT_U16, &u16Stride);

    bm_image_create(bm_handle, height, width, fmt, DATA_TYPE_EXT_U16, &stCurGrad, &u16Stride);
    bm_image_create(bm_handle, height, width, fmt, DATA_TYPE_EXT_U16, &stBgGrad, &u16Stride);

	bm_image_alloc_dev_mem(stCurGrad, BMCV_HEAP_ANY);
	bm_image_alloc_dev_mem(stBgGrad, BMCV_HEAP_ANY);

	bmcv_ive_norm_grad(bm_handle, src, NULL, NULL, &stCurGrad, normGradAttr);


	ret = bmcv_ive_gradfg(bm_handle, *src, stCurGrad, stBgGrad, *dst, gradFgAttr);

	if (ret) {
		GST_ERROR("ive_gradfg fail\n");
		return FALSE;
	}

	bm_image_destroy(&stCurGrad);
	bm_image_destroy(&stBgGrad);
	free(&stCurGrad);
	free(&stBgGrad);
	return ret;
}
static gboolean
gst_bm_ive_sad(GstVideoFilter * filter, struct bm_image *src, struct bm_image *dst)
{
	gboolean ret;
	bm_handle_t bm_handle = NULL;
	GstBmIVE *self = GST_BM_IVE (filter);
	//bmcv_ive_sad_mode sadMode = self->mode1;
	bmcv_ive_sad_out_ctrl sadOutCtrl = self->mode2;
	bmcv_ive_sad_attr sadAttr;
    sadAttr.en_out_ctrl = sadOutCtrl;
	bmcv_ive_sad_thresh_attr sadthreshAttr;
    sadthreshAttr.u16_thr = self->thr_value;
    sadthreshAttr.u8_min_val = self->min_val;
    sadthreshAttr.u8_max_val = self->max_val;

	bm_dev_request(&bm_handle, 0);

	ret = bmcv_ive_sad(bm_handle, src, dst, NULL, &sadAttr, &sadthreshAttr);

	if (ret) {
		GST_ERROR("ive_sad fail\n");
		return FALSE;
	}
	return ret;
}
static gboolean
gst_bm_ive_bgmodel(GstVideoFilter * filter, struct bm_image *src, struct bm_image *dst)
{
	gboolean ret;
	bm_handle_t bm_handle = NULL;
	//GstBmIVE *self = GST_BM_IVE (filter);

	bm_dev_request(&bm_handle, 0);
	return ret;
}
static gboolean
gst_bm_ive_gmm(GstVideoFilter * filter, struct bm_image *src, struct bm_image *dst)
{
	gboolean ret;
	bm_handle_t bm_handle = NULL;
	//GstBmIVE *self = GST_BM_IVE (filter);
	bm_device_mem_t dst_model;
	guint width = src->width;
	guint height = src->height;
    bmcv_ive_gmm_ctrl gmmAttr;
	bm_image dst_fg, dst_bg;
    gmmAttr.u0q16_bg_ratio = 45875;
    gmmAttr.u0q16_init_weight = 3277;
    gmmAttr.u22q10_noise_var = 225 * 1024;
    gmmAttr.u22q10_max_var = 2000 * 1024;
    gmmAttr.u22q10_min_var = 200 * 1024;
    gmmAttr.u8q8_var_thr = (unsigned short)(256 * 6.25);
    gmmAttr.u8_model_num = 3;
	bm_image_format_ext src_fmt = FORMAT_GRAY;
	int stride[4];
    guint u32FrameNumMax = 32;
    guint u32FrmCnt = 0;
	guint model_len = width * height * gmmAttr.u8_model_num * 8;
	guint inputdata[u32FrameNumMax * 352 * 288];

	bm_dev_request(&bm_handle, 0);

    unsigned char *srcData = malloc(width * height * sizeof(unsigned char));
    unsigned char *ive_fg_res = malloc(width * height * sizeof(unsigned char));
    unsigned char *ive_bg_res = malloc(width * height * sizeof(unsigned char));
    unsigned char *model_data = malloc(model_len * sizeof(unsigned char));
    memset(srcData, 0, width * height * sizeof(unsigned char));
    memset(ive_fg_res, 0, width * height * sizeof(unsigned char));
    memset(ive_bg_res, 0, width * height * sizeof(unsigned char));
    memset(model_data, 0, model_len * sizeof(unsigned char));

    bm_ive_image_calc_stride(bm_handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, stride);

    bm_image_create(bm_handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, src, stride);
    bm_image_create(bm_handle, height, width, FORMAT_GRAY, DATA_TYPE_EXT_1N_BYTE, &dst_fg, stride);
    bm_image_create(bm_handle, height, width, FORMAT_GRAY, DATA_TYPE_EXT_1N_BYTE, &dst_bg, stride);

	bm_image_alloc_dev_mem(dst_fg, BMCV_HEAP_ANY);
	bm_image_alloc_dev_mem(dst_bg, BMCV_HEAP_ANY);
	bm_malloc_device_byte(bm_handle, &dst_model, model_len);
	bm_memcpy_s2d(bm_handle, dst_model, model_data);

	for(u32FrmCnt = 0; u32FrmCnt < u32FrameNumMax; u32FrmCnt++){
		if(width > 480 ){
			for(int j = 0; j < 288; j++){
				memcpy(srcData + (j * width),
						inputdata + (u32FrmCnt * 352 * 288 + j * 352), 352);
				memcpy(srcData + (j * width + 352),
						inputdata + (u32FrmCnt * 352 * 288 + j * 352), 352);
			}
		} else {
			for(int j = 0; j < 288; j++){
				memcpy(srcData + j * stride[0],
						inputdata + u32FrmCnt * width * 288 + j * width, width);
				int s = stride[0] - width;
				memset(srcData + j * stride[0] + width, 0, s);
			}
		}

		ret = bm_image_copy_host_to_device(*src, (void**)&srcData);
		if(ret != BM_SUCCESS){
			GST_ERROR("bm_image copy src h2d failed. ret = %d \n", ret);
			free(ive_fg_res);
			free(srcData);
			free(ive_bg_res);
			free(model_data);
		}

		if(u32FrmCnt >= 500)
			gmmAttr.u0q16_learn_rate = 131;  //0.02
		else
			gmmAttr.u0q16_learn_rate = 65535 / (u32FrmCnt + 1);

		ret = bmcv_ive_gmm(bm_handle, *src, dst_fg, dst_bg, dst_model, gmmAttr);
	}
	free(srcData);
	free(ive_fg_res);
	free(ive_bg_res);
	free(model_data);
    bm_image_destroy(&dst_fg);
    bm_image_destroy(&dst_bg);
    bm_free_device(bm_handle, dst_model);
	return ret;
}
static gboolean
gst_bm_ive_gmm2(GstVideoFilter * filter, struct bm_image *src, struct bm_image *dst)
{
	gboolean ret;
	bm_handle_t bm_handle = NULL;
	//GstBmIVE *self = GST_BM_IVE (filter);

	bm_dev_request(&bm_handle, 0);

	return ret;
}
static gboolean
gst_bm_ive_add(GstVideoFilter * filter, struct bm_image *src, struct bm_image *dst)
{
	gboolean ret;
	bm_handle_t bm_handle = NULL;
	GstBmIVE *self = GST_BM_IVE (filter);
    bmcv_ive_add_attr add_attr;
    memset(&add_attr, 0, sizeof(bmcv_ive_add_attr));

	bm_dev_request(&bm_handle, 0);

    add_attr.param_x = self->x;
    add_attr.param_y = self->y;
	ret = bmcv_ive_add(bm_handle, *src, *src, *dst, add_attr);
	if (ret) {
		GST_ERROR("ive_add fail\n");
		return FALSE;
	}

	return ret;
}
static gboolean
gst_bm_ive_and(GstVideoFilter * filter, struct bm_image *src, struct bm_image *dst)
{
	gboolean ret;
	bm_handle_t bm_handle = NULL;
	//GstBmIVE *self = GST_BM_IVE (filter);


	bm_dev_request(&bm_handle, 0);

	ret = bmcv_ive_and(bm_handle, *src, *src, *dst);
	if (ret) {
		GST_ERROR("ive_and fail\n");
		return FALSE;
	}
	return ret;
}
static gboolean
gst_bm_ive_sub(GstVideoFilter * filter, struct bm_image *src, struct bm_image *dst)
{
	gboolean ret;
	bm_handle_t bm_handle = NULL;
	GstBmIVE *self = GST_BM_IVE (filter);
	bmcv_ive_sub_attr sub_attr;
    memset(&sub_attr, 0, sizeof(bmcv_ive_sub_attr));

    sub_attr.en_mode = self->mode1;

	bm_dev_request(&bm_handle, 0);

	ret = bmcv_ive_sub(bm_handle, *src, *src, *dst, sub_attr);
	if (ret) {
		GST_ERROR("ive_sub fail\n");
		return FALSE;
	}
	return ret;
}
static gboolean
gst_bm_ive_or(GstVideoFilter * filter, struct bm_image *src, struct bm_image *dst)
{
	gboolean ret;
	bm_handle_t bm_handle = NULL;
	//GstBmIVE *self = GST_BM_IVE (filter);

	bm_dev_request(&bm_handle, 0);

	ret = bmcv_ive_or(bm_handle, *src, *src, *dst);
	if (ret) {
		GST_ERROR("ive_or fail\n");
		return FALSE;
	}
	return ret;
}
static gboolean
gst_bm_ive_xor(GstVideoFilter * filter, struct bm_image *src, struct bm_image *dst)
{
	gboolean ret;
	bm_handle_t bm_handle = NULL;
	//GstBmIVE *self = GST_BM_IVE (filter);

	bm_dev_request(&bm_handle, 0);

	ret = bmcv_ive_xor(bm_handle, *src, *src, *dst);
	if (ret) {
		GST_ERROR("ive_xor fail\n");
		return FALSE;
	}
	return ret;
}
static gboolean
gst_bm_ive_sobel(GstVideoFilter * filter, struct bm_image *src, struct bm_image *dst)
{
	gboolean ret;
	bm_handle_t bm_handle = NULL;
	GstBmIVE *self = GST_BM_IVE (filter);
    bmcv_ive_sobel_ctrl sobelAtt;
    sobelAtt.sobel_mode = self->mode1;

	/* 5 by 5*/
	//signed char arr5by5[25] = { -1, -2, 0,  2,  1, -4, -8, 0,  8,  4, -6, -12, 0,
	//				12, 6,  -4, -8, 0, 8,  4,  -1, -2, 0, 2, 1 };
	/* 3 by 3*/
	signed char arr3by3[25] = { 0, 0, 0, 0,  0, 0, -1, 0, 1, 0, 0, -2, 0,
					2, 0, 0, -1, 0, 1, 0,  0, 0, 0, 0, 0 };

    memcpy(sobelAtt.as8_mask, arr3by3, 5 * 5 * sizeof(signed char));

	bm_dev_request(&bm_handle, 0);

	ret = bmcv_ive_sobel(bm_handle, src, NULL, dst, sobelAtt);
	if (ret) {
		GST_ERROR("ive_sobel fail\n");
		return FALSE;
	}
	return ret;
}
static void __attribute__((unused))
gst_bm_ive_transform_frame_handle(GstVideoFilter *filter, bm_image *src
		, bm_image *dst)
{
	GstBmIVE *self = GST_BM_IVE (filter);
	guint type = self->type;
	switch (type)
	{
	case IVE_ADD:
		gst_bm_ive_add(filter, src, dst);
		break;
	case IVE_AND:
		gst_bm_ive_and(filter, src, dst);
		break;
	case IVE_SUB:
		gst_bm_ive_sub(filter, src, dst);
		break;
	case IVE_XOR:
		gst_bm_ive_xor(filter, src, dst);
		break;
	case IVE_OR:
		gst_bm_ive_or(filter, src, dst);
		break;
	case IVE_DMA:
		gst_bm_ive_dma(filter, src, dst);
		break;
	case IVE_HIST:
		gst_bm_ive_hist(filter, src, dst);
		break;
	case IVE_SOBEL:
		gst_bm_ive_sobel(filter, src, dst);
		break;
	case IVE_CCL:
		gst_bm_ive_ccl(filter, src, dst);
		break;
	case IVE_FILTER:
		gst_bm_ive_filter(filter, src, dst);
		break;
	case IVE_ERODE:
		gst_bm_ive_erode(filter, src, dst);
		break;
	case IVE_DILATE:
		gst_bm_ive_dilate(filter, src, dst);
		break;
	case IVE_THRESH:
		gst_bm_ive_thresh(filter, src, dst);
		break;
	case IVE_MAP:
		gst_bm_ive_map(filter, src, dst);
		break;
	case IVE_THRESH_U16:
		gst_bm_ive_thresh(filter, src, dst);
		break;
	case IVE_THRESH_S16:
		gst_bm_ive_thresh(filter, src, dst);
		break;
	case IVE_LBP:
		gst_bm_ive_lbp(filter, src, dst);
		break;
	case IVE_INTG:
		gst_bm_ive_intg(filter, src, dst);
		break;
	case IVE_NCC:
		gst_bm_ive_ncc(filter, src, dst);
		break;
	case IVE_ORDSATAFILTER:
		gst_bm_ive_ordstatafilter(filter, src, dst);
		break;
	case IVE_MAGANDANG:
		gst_bm_ive_magandang(filter, src, dst);
		break;
	case IVE_NORMGRAD:
		gst_bm_ive_normgrad(filter, src, dst);
		break;
	case IVE_CANNYHYSEDGE:
		gst_bm_ive_cannyhysedge(filter, src, dst);
		break;
	case IVE_GMM:
		gst_bm_ive_gmm(filter, src, dst);
		break;
	case IVE_GMM2:
		gst_bm_ive_gmm2(filter, src, dst);
		break;
	case IVE_STCANDICORNER:
		gst_bm_ive_stcandicorner(filter, src, dst);
		break;
	case IVE_GRADFG:
		gst_bm_ive_gradfg(filter, src, dst);
		break;
	case IVE_SAD:
		gst_bm_ive_sad(filter, src, dst);
		break;
	case IVE_BGMODEL:
		gst_bm_ive_bgmodel(filter, src, dst);
		break;
	default:
		GST_ERROR("OP type[%d] not support\n", type);
		break;
	}
}
static GstFlowReturn
gst_bm_ive_transform_frame (GstVideoFilter * filter, GstVideoFrame * inframe,
    GstVideoFrame * outframe)
{
	GstFlowReturn ret = GST_FLOW_OK;
	bm_image_format_ext bm_in_format, bm_out_format;
	int in_height, in_width, out_height, out_width;
	//guint *src_in_ptr[4], *dst_in_ptr[4];
	bm_handle_t bm_handle = NULL;
	GstBmIVE *self = GST_BM_IVE (filter);

	bm_dev_request(&bm_handle, 0);

	bm_image *src = NULL;
	bm_image *dst = NULL;

	src = (bm_image *) malloc(sizeof(bm_image));
	dst = (bm_image *) malloc(sizeof(bm_image));

	if ((!src) || (!dst))
	{
		GST_ERROR_OBJECT(self, "Failed to allocate memory for bm_image");
		return GST_FLOW_ERROR;
	}
	bm_in_format =
		(bm_image_format_ext)gst_gstformat_to_bmformat(inframe->info.finfo->format);
	bm_out_format =
		(bm_image_format_ext)gst_gstformat_to_bmformat(outframe->info.finfo->format);
	in_height = inframe->info.height;
	in_width = inframe->info.width;
	out_height = outframe->info.height;
	out_width = outframe->info.width;

	GstBuffer *inbuf, *outbuf;
	GstMemory *inmem, *outmem;
	bm_device_mem_t *fb_dma_buffer;

	inbuf = inframe->buffer;
	inmem = gst_buffer_peek_memory(inbuf, 0);
	outbuf = outframe->buffer;
	outmem = gst_buffer_peek_memory(outbuf, 0);

	int in_stride[4] = {0};
	for (guint i = 0; i < GST_VIDEO_FRAME_N_PLANES(inframe); i++) {
		in_stride[i] = GST_VIDEO_FRAME_PLANE_STRIDE(inframe, i);
	}

#if 1

	// bm_ive_image_calc_stride(bm_handle, in_height, in_width, bm_in_format, DATA_TYPE_EXT_1N_BYTE, stride);
	bm_image_create(bm_handle, in_height, in_width, bm_in_format, DATA_TYPE_EXT_1N_BYTE, src, in_stride);
	bm_image_create(bm_handle, out_height, out_width, bm_out_format, DATA_TYPE_EXT_1N_BYTE, dst, NULL);

	if (g_type_is_a(G_OBJECT_TYPE(inmem->allocator), GST_TYPE_BM_ALLOCATOR)) {
		#if define USE_BM_ALLOCATOR
		fb_dma_buffer = gst_bm_allocator_get_bm_buffer(inmem);
		unsigned long long base_addr = bm_mem_get_device_addr(*fb_dma_buffer);
		int plane_num = GST_VIDEO_FRAME_N_PLANES(inframe);
		int plane_size[3] = {0};
		unsigned long long input_addr_phy[3] = {0};
		bm_device_mem_t input_addr[3] = {0};

		for (int i = 0; i < plane_num; i++) {
		plane_size[i] = GST_VIDEO_FRAME_COMP_STRIDE(inframe, i) *
						GST_VIDEO_FRAME_COMP_HEIGHT(inframe, i);
		input_addr_phy[i] = base_addr + GST_VIDEO_FRAME_PLANE_OFFSET(inframe, i);
		input_addr[i] = bm_mem_from_device(input_addr_phy[i], plane_size[i]);
		}
		#endif
	} else {
		GST_DEBUG_OBJECT(self, "inframe->buffer isn't bm_allocator");
		bm_image_alloc_dev_mem(*src, BMCV_HEAP_ANY);
		guint8 *src_in_ptr[4];
		for (guint i = 0; i < GST_VIDEO_FRAME_N_PLANES(inframe); i++) {
			src_in_ptr[i] = GST_VIDEO_FRAME_PLANE_DATA(inframe, i);
		}
		bm_image_copy_host_to_device(*src, (void **)src_in_ptr);
	}

	if (g_type_is_a(G_OBJECT_TYPE(outmem->allocator), GST_TYPE_BM_ALLOCATOR)) {
		fb_dma_buffer = gst_bm_allocator_get_bm_buffer(outmem);
		unsigned long long base_addr = bm_mem_get_device_addr(*fb_dma_buffer);
		int plane_num = GST_VIDEO_FRAME_N_PLANES(outframe);
		int plane_size[3] = {0};
		unsigned long long output_addr_phy[3] = {0};
		bm_device_mem_t output_addr[3] = {0};

		for (int i = 0; i < plane_num; i++) {
		plane_size[i] = GST_VIDEO_FRAME_COMP_STRIDE(outframe, i) *
						GST_VIDEO_FRAME_COMP_HEIGHT(outframe, i);
		output_addr_phy[i] = base_addr + GST_VIDEO_FRAME_PLANE_OFFSET(outframe, i);
		output_addr[i] = bm_mem_from_device(output_addr_phy[i], plane_size[i]);
		}
		bm_image_attach(*dst, output_addr);
	} else if (gst_is_dmabuf_memory(outmem)) {
		gint fd = gst_dmabuf_memory_get_fd(outmem);
		GST_DEBUG_OBJECT(self, "outmem fd: %d", fd);
		int plane_num = GST_VIDEO_FRAME_N_PLANES(outframe);
		int plane_size[3] = {0};
		bm_device_mem_t output_addr[3] = {0};

		for (int i = 0; i < plane_num; i++) {
		plane_size[i] = GST_VIDEO_FRAME_COMP_STRIDE(outframe, i) *
						GST_VIDEO_FRAME_COMP_HEIGHT(outframe, i);
		output_addr[i] = bm_mem_from_device(fd, plane_size[i]);
		}
		bm_image_attach(*dst, output_addr);
	} else {
		GST_DEBUG_OBJECT(self, "outframe->buffer isn't dmabuffer");
		bm_image_alloc_dev_mem(*dst, BMCV_HEAP_ANY);
	}

#endif

	if (!g_type_is_a(G_OBJECT_TYPE(outmem->allocator), GST_TYPE_BM_ALLOCATOR) &&
		!gst_is_dmabuf_memory(outmem)) {
		GST_DEBUG_OBJECT(self, "outframe is copy");
		guint8 *dst_in_ptr[4];
		for (guint i = 0; i < GST_VIDEO_FRAME_N_PLANES(outframe); i++) {
		dst_in_ptr[i] = GST_VIDEO_FRAME_PLANE_DATA(outframe, i);
		}
		bm_image_copy_device_to_host(*dst, (void **)dst_in_ptr);
	} else {
		GST_DEBUG_OBJECT(self, "outframe is zerocopy");
	}

	bm_image_destroy(src);
	bm_image_destroy(dst);
	free(src);
	free(dst);

	return ret;
}

static int
gst_gstformat_to_bmformat(GstVideoFormat gst_format)
{
  int format;
  switch (gst_format)
  {
  case GST_VIDEO_FORMAT_GRAY8:
    format = FORMAT_GRAY;
    break;
  case GST_VIDEO_FORMAT_NV12:
    format = FORMAT_NV12;
    break;
  case GST_VIDEO_FORMAT_RGB:
    format = FORMAT_RGB_PACKED;
    break;
  default:
    GST_DEBUG("Error: Unsupported GstVideoFormat %d\n", gst_format);
    return -1;
  }
  return format;
}


static GstVideoFormat __attribute__((unused)) gst_bmformat_to_gstformat(int bm_format)
{
  GstVideoFormat format;
  switch (bm_format)
  {
  case FORMAT_GRAY:
    format = GST_VIDEO_FORMAT_GRAY8;
    break;
  case FORMAT_NV12:
    format = GST_VIDEO_FORMAT_NV12;
    break;
  case FORMAT_RGB_PACKED:
    format = GST_VIDEO_FORMAT_RGB;
    break;
  default:
    g_warning("Unsupported BM format %d", bm_format);
    format = GST_VIDEO_FORMAT_UNKNOWN;
    break;
  }
  return format;
}
void gst_bm_ive_register(GstPlugin *plugin, guint soc_idx, GstCaps *sink_caps,
                          GstCaps *src_caps)
{
	GType type;
	gchar *type_name;
	GstBmIVEClassData *class_data;
	GTypeInfo type_info = {
	sizeof (GstBmIVEClass),
	NULL,
	NULL,
	(GClassInitFunc) gst_bm_ive_subclass_init,
	NULL,
	NULL,
	sizeof (GstBmIVE),
	0,
	(GInstanceInitFunc) gst_bm_ive_subinstance_init,
	NULL,
	};

	GST_DEBUG_CATEGORY_INIT (gst_bm_ive_debug, "bmive", 0, "BM IVE Plugin");

	class_data = g_new0 (GstBmIVEClassData, 1);
	class_data->sink_caps = gst_caps_ref (sink_caps);
	class_data->src_caps = gst_caps_ref (src_caps);
	class_data->soc_index = soc_idx;

	type_name = g_strdup (GST_BM_IVE_TYPE_NAME);
	type_info.class_data = class_data;
	type = g_type_register_static(GST_TYPE_BM_IVE, type_name, &type_info, 0);

	if (!gst_element_register(plugin, type_name, GST_RANK_PRIMARY, type)) {
	GST_WARNING("Failed to register IVE plugin '%s'", type_name);
	}

	g_free (type_name);
}
