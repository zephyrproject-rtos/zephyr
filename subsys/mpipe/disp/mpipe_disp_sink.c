/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/display.h>
#include <zephyr/drivers/video.h>
#include <zephyr/logging/log.h>

#include <zephyr/mpipe/mpipe_buffer.h>
#include <zephyr/mpipe/mpipe_dispatch.h>
#include <zephyr/mpipe/mpipe_element.h>
#include <zephyr/mpipe/mpipe_structure.h>
#include <zephyr/mpipe/mpipe_value.h>

#include <zephyr/mpipe/disp/mpipe_disp_sink.h>

/* Investigation aid, see mpipe_stage_stats.h */
#define MPIPE_STAGE_STATS IS_ENABLED(CONFIG_MPIPE_STAGE_STATS)
#if MPIPE_STAGE_STATS
#include "../mpipe_stage_stats.h"
static struct mpipe_stage_stats disp_stats;
#endif

LOG_MODULE_REGISTER(mpipe_disp_sink, CONFIG_MPIPE_LOG_LEVEL);

#define DEFAULT_PROP_DEVICE DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_display))

/* Default supported minimum width/height may depend on the HW but currently no way to get it */
#define DEFAULT_WIDTH_MIN  1
#define DEFAULT_HEIGHT_MIN 1

struct mpipe_disp_vid_pix_fmt {
	enum display_pixel_format disp_fmt;
	uint32_t vid_fmt;
};

/*
 * Pixel format mapping between video and display
 * Note: video fourcc pixel format is the standard used for caps
 */
static const struct mpipe_disp_vid_pix_fmt mpipe_disp_vid_pix_fmt_map[] = {
	{PIXEL_FORMAT_RGB_565, VIDEO_PIX_FMT_RGB565},
	{PIXEL_FORMAT_RGB_565X, VIDEO_PIX_FMT_RGB565X},
	{PIXEL_FORMAT_RGB_888, VIDEO_PIX_FMT_BGR24},
	{PIXEL_FORMAT_ARGB_8888, VIDEO_PIX_FMT_BGRA32},
	{PIXEL_FORMAT_XRGB_8888, VIDEO_PIX_FMT_BGRX32},
	{PIXEL_FORMAT_L_8, VIDEO_PIX_FMT_GREY},
};

static uint32_t disp_to_vid_pix_fmt(enum display_pixel_format disp_fmt)
{
	for (uint8_t i = 0; i < ARRAY_SIZE(mpipe_disp_vid_pix_fmt_map); i++) {
		if (mpipe_disp_vid_pix_fmt_map[i].disp_fmt == disp_fmt) {
			return mpipe_disp_vid_pix_fmt_map[i].vid_fmt;
		}
	}

	return 0;
}

static enum display_pixel_format vid_to_disp_pix_fmt(uint32_t vid_fmt)
{
	for (uint8_t i = 0; i < ARRAY_SIZE(mpipe_disp_vid_pix_fmt_map); i++) {
		if (mpipe_disp_vid_pix_fmt_map[i].vid_fmt == vid_fmt) {
			return mpipe_disp_vid_pix_fmt_map[i].disp_fmt;
		}
	}

	return 0;
}

static int mpipe_disp_sink_setup(struct mpipe_disp_sink *disp_sink,
				 const enum display_pixel_format pixfmt)
{
	int ret = 0;

	ret = display_set_pixel_format(disp_sink->display_dev, pixfmt);
	if (ret != 0) {
		LOG_ERR("Unable to set display format");
		return ret;
	}

	/* Turn off blanking if driver supports it */
	ret = display_blanking_off(disp_sink->display_dev);
	if (ret == -ENOSYS) {
		LOG_WRN("Display blanking off not available");
		ret = 0;
	}

	return ret;
}

/*
 * The display advertises the formats it accepts as a bitmask, so the caps of
 * the n-th supported format can be produced directly from it and none of the
 * others has to exist.
 */
static int mpipe_disp_sink_enum_caps(struct mpipe_pad *pad, uint32_t index,
				     const struct mpipe_structure *filter,
				     struct mpipe_structure *out)
{
	struct mpipe_disp_sink *disp = (struct mpipe_disp_sink *)pad->object.container;
	struct display_capabilities display_caps;
	struct mpipe_structure candidate;
	uint32_t vid_fmt = 0;
	uint32_t matched = 0;
	int ret;

	display_get_capabilities(disp->display_dev, &display_caps);

	for (uint8_t i = 0; i < ARRAY_SIZE(mpipe_disp_vid_pix_fmt_map); i++) {
		/* Use video formats as reference formats for caps */
		uint32_t fmt = disp_to_vid_pix_fmt(display_caps.supported_pixel_formats &
						   mpipe_disp_vid_pix_fmt_map[i].disp_fmt);

		if (fmt == 0) {
			continue;
		}

		if (matched == index) {
			vid_fmt = fmt;
			break;
		}

		matched++;
	}

	if (vid_fmt == 0) {
		return -ENOENT;
	}

	ret = mpipe_structure_init_fields(
		&candidate, MPIPE_MEDIA_VIDEO, MPIPE_CAPS_PIXEL_FORMAT, MPIPE_TYPE_UINT, vid_fmt,
		MPIPE_CAPS_IMAGE_WIDTH, MPIPE_TYPE_UINT_RANGE, DEFAULT_WIDTH_MIN,
		display_caps.x_resolution, 1, MPIPE_CAPS_IMAGE_HEIGHT, MPIPE_TYPE_UINT_RANGE,
		DEFAULT_HEIGHT_MIN, display_caps.y_resolution, 1, MPIPE_CAPS_END);
	if (ret != 0) {
		return ret;
	}

	return mpipe_pad_enum_filter(&candidate, filter, out);
}

static int mpipe_disp_sink_set_caps(struct mpipe_sink *sink, const struct mpipe_structure *caps)
{
	struct mpipe_disp_sink *disp_sink = (struct mpipe_disp_sink *)sink;
	const struct mpipe_value *value = mpipe_structure_get_value(caps, MPIPE_CAPS_PIXEL_FORMAT);
	enum display_pixel_format disp_fmt;

	if (value == NULL) {
		return -EINVAL;
	}

	disp_fmt = vid_to_disp_pix_fmt(mpipe_value_get_uint(value));
	if (disp_fmt == 0 || mpipe_disp_sink_setup(disp_sink, disp_fmt) != 0) {
		return -EINVAL;
	}

	return mpipe_pad_set_caps(&sink->sink_pad, caps);
}

static int mpipe_disp_sink_set_property(struct mpipe_object *obj, uint32_t key, const void *val)
{
	struct mpipe_disp_sink *disp_sink = (struct mpipe_disp_sink *)obj;

	switch (key) {
	case MPIPE_PROP_DISP_SINK_DEVICE:
		disp_sink->display_dev = val;

		return 0;
	default:
		return -ENOTSUP;
	}
}

static int mpipe_disp_sink_get_property(struct mpipe_object *obj, uint32_t key, void *val)
{
	struct mpipe_disp_sink *disp_sink = (struct mpipe_disp_sink *)obj;

	switch (key) {
	case MPIPE_PROP_DISP_SINK_DEVICE:
		*(const struct device **)val = disp_sink->display_dev;

		return 0;
	default:
		return -ENOTSUP;
	}
}

/* Whether the display reads the buffer it is given until the next frame replaces it */
static bool mpipe_disp_sink_holds_buffer(const struct mpipe_disp_sink *disp_sink)
{
	struct display_capabilities display_caps;

	if (disp_sink->display_dev == NULL) {
		return false;
	}

	display_get_capabilities(disp_sink->display_dev, &display_caps);

	return (display_caps.screen_info & SCREEN_INFO_WRITE_HOLDS_BUFFER) != 0U;
}

/* The frame on screen goes back to its pool with the run, before the pool itself does */
static int mpipe_disp_sink_change_state(struct mpipe_element *self,
					enum mpipe_state_change transition)
{
	struct mpipe_disp_sink *disp_sink = (struct mpipe_disp_sink *)self;

	if (transition == MPIPE_STATE_CHANGE_READY_TO_PAUSED) {
		disp_sink->holds_buffer = mpipe_disp_sink_holds_buffer(disp_sink);
	} else if (transition == MPIPE_STATE_CHANGE_PAUSED_TO_READY && disp_sink->shown != NULL) {
		net_buf_unref(disp_sink->shown);
		disp_sink->shown = NULL;
	}

	return mpipe_sink_change_state(self, transition);
}

/* A frame kept on screen is one buffer upstream cannot have back */
static int mpipe_disp_sink_propose_buffer_pool(struct mpipe_sink *self,
					       struct mpipe_dispatch *query)
{
	const struct mpipe_disp_sink *disp_sink = (const struct mpipe_disp_sink *)self;

	query->pool_cfg.min_buffers = mpipe_disp_sink_holds_buffer(disp_sink) ? 1U : 0U;

	return 0;
}

int mpipe_disp_sink_process_fn(struct mpipe_pad *pad, struct net_buf *in_buf,
			       struct net_buf **out_buf)
{
	__ASSERT_NO_MSG(pad != NULL);
	__ASSERT_NO_MSG(out_buf != NULL);

	struct mpipe_disp_sink *disp_sink =
		CONTAINER_OF(pad->object.container, struct mpipe_disp_sink, sink.element.object);
	const struct mpipe_value *value =
		mpipe_structure_get_value(&pad->caps, MPIPE_CAPS_PIXEL_FORMAT);
	enum display_pixel_format disp_fmt = 0;
	uint32_t bytes_per_line;
	uint32_t width = 0;
	struct net_buf *prev;
	int ret;

	if (value != NULL) {
		disp_fmt = vid_to_disp_pix_fmt(mpipe_value_get_uint(value));
	}

	value = mpipe_structure_get_value(&pad->caps, MPIPE_CAPS_IMAGE_WIDTH);
	if (value != NULL) {
		width = mpipe_value_get_uint(value);
	}

	/* A sink consumes the buffer: no output buffer */
	*out_buf = NULL;

	if (in_buf == NULL) {
		return 0;
	}

	/*
	 * The height of a buffer is derived from its size, the frame width and
	 * the pixel depth. Caps that never got negotiated leave both the format
	 * and the width unset, so report that instead of dividing by zero.
	 */
	bytes_per_line = width * DISPLAY_BITS_PER_PIXEL(disp_fmt) / BITS_PER_BYTE;
	if (bytes_per_line == 0) {
		LOG_ERR("No negotiated pixel format and width to display a buffer with");
		net_buf_unref(in_buf);
		return -ENODATA;
	}

	/*
	 * The buffer is assumed to carry a video buffer.
	 * TODO: Add support for other buffer types
	 */
	struct video_buffer *vbuf =
		(struct video_buffer *)mpipe_buffer_get_meta(in_buf)->driver_buf;
	struct display_buffer_descriptor buf_desc = {
		.buf_size = mpipe_buffer_get_meta(in_buf)->bytes_used,
	};

	buf_desc.width = width;
	buf_desc.pitch = buf_desc.width;
	/* Do not get height from caps as sometimes buffer is just a partial frame */
	buf_desc.height = buf_desc.buf_size / bytes_per_line;

#if MPIPE_STAGE_STATS
	uint32_t t0 = k_cycle_get_32();
#endif

	if (buf_desc.height == 0U) {
		LOG_ERR("Buffer of %u bytes is shorter than a %u-byte line", buf_desc.buf_size,
			bytes_per_line);
		ret = -ENODATA;
	} else if (vbuf != NULL) {
		/* line_offset is only used to support partial video frame */
		ret = display_write(disp_sink->display_dev, 0, vbuf->line_offset, &buf_desc,
				    vbuf->buffer);
	} else {
		/* Fallback to net_buf data if no video_buffer metadata */
		ret = display_write(disp_sink->display_dev, 0, 0, &buf_desc, in_buf->data);
	}

#if MPIPE_STAGE_STATS
	if (mpipe_stage_stats_add(&disp_stats, 1, buf_desc.buf_size, buf_desc.buf_size,
				  mpipe_stage_stats_elapsed_us(t0))) {
		LOG_INF("stats disp_sink: %u fps, %ux%u, write %u/%u/%u ms min/avg/max (%u ms)",
			mpipe_stage_stats_per_s(&disp_stats, disp_stats.count), buf_desc.width,
			buf_desc.height, disp_stats.us_min / 1000U,
			mpipe_stage_stats_us_avg(&disp_stats) / 1000U, disp_stats.us_max / 1000U,
			disp_stats.elapsed_ms);
		mpipe_stage_stats_reset(&disp_stats);
	}
#endif

	if (ret != 0) {
		if (ret != -ENODATA) {
			LOG_ERR("display_write failed (%d)", ret);
		}
		net_buf_unref(in_buf);
		return ret;
	}

	/*
	 * A display that scans the frame out of this very buffer reads it until
	 * the next frame replaces it: keep it and let go of the one it replaced.
	 * Released at once, the buffer would be the next one the decoder writes
	 * into, under the viewer's eyes.
	 */
	if (disp_sink->holds_buffer) {
		prev = disp_sink->shown;
		disp_sink->shown = in_buf;
		if (prev != NULL) {
			net_buf_unref(prev);
		}
	} else {
		net_buf_unref(in_buf);
	}

	return 0;
}

int mpipe_disp_sink_init(struct mpipe_disp_sink *disp_sink, uint8_t id)
{
	__ASSERT_NO_MSG(disp_sink != NULL);

	struct mpipe_element *self = &disp_sink->sink.element;
	struct mpipe_sink *sink = &disp_sink->sink;
	int ret = mpipe_sink_init(sink, id);

	if (ret != 0) {
		return ret;
	}

	mpipe_element_set_name(self, "disp_sink");

	disp_sink->display_dev = DEFAULT_PROP_DEVICE;
	disp_sink->holds_buffer = false;
	disp_sink->shown = NULL;

	self->object.get_property = mpipe_disp_sink_get_property;
	self->object.set_property = mpipe_disp_sink_set_property;
	self->change_state = mpipe_disp_sink_change_state;

	sink->sink_pad.process_fn = mpipe_disp_sink_process_fn;
	sink->sink_pad.enum_caps_fn = mpipe_disp_sink_enum_caps;
	sink->set_caps = mpipe_disp_sink_set_caps;
	sink->propose_buffer_pool = mpipe_disp_sink_propose_buffer_pool;

	return 0;
}
