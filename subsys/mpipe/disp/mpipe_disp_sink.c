/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/display.h>
#include <zephyr/drivers/video.h>
#include <zephyr/logging/log.h>

#include <zephyr/mpipe/mpipe_buffer.h>
#include <zephyr/mpipe/mpipe_element.h>
#include <zephyr/mpipe/mpipe_structure.h>
#include <zephyr/mpipe/mpipe_value.h>

#include <zephyr/mpipe/disp/mpipe_disp_sink.h>

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

int mpipe_disp_sink_chain_fn(struct mpipe_pad *pad, struct net_buf *in_buf,
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
	struct net_buf *cur;
	struct net_buf *next;
	int ret;

	if (value != NULL) {
		disp_fmt = vid_to_disp_pix_fmt(mpipe_value_get_uint(value));
	}

	value = mpipe_structure_get_value(&pad->caps, MPIPE_CAPS_IMAGE_WIDTH);
	if (value != NULL) {
		width = mpipe_value_get_uint(value);
	}

	/* Sink returns NULL for output buffer as it is at the end of the chain */
	*out_buf = NULL;

	if (in_buf == NULL) {
		return 0;
	}

	/*
	 * The height of a fragment is derived from its size, the frame width and
	 * the pixel depth. Caps that never got negotiated leave both the format
	 * and the width unset, so report that instead of dividing by zero.
	 */
	bytes_per_line = width * DISPLAY_BITS_PER_PIXEL(disp_fmt) / BITS_PER_BYTE;
	if (bytes_per_line == 0) {
		LOG_ERR("No negotiated pixel format and width to display a buffer with");
		net_buf_unref(in_buf);
		return -ENODATA;
	}

	/* Input may be a fragment chain. Display each fragment then unref it */
	cur = in_buf;
	while (cur != NULL) {
		/*
		 * The buffer is assumed to carry a video buffer.
		 * TODO: Add support for other buffer types
		 */
		struct video_buffer *vbuf =
			(struct video_buffer *)mpipe_buffer_get_meta(cur)->driver_buf;
		struct display_buffer_descriptor buf_desc = {
			.buf_size = mpipe_buffer_get_meta(cur)->bytes_used,
		};

		next = cur->frags;
		cur->frags = NULL;

		buf_desc.width = width;
		buf_desc.pitch = buf_desc.width;
		/* Do not get height from caps as sometimes buffer is just a partial frame */
		buf_desc.height = buf_desc.buf_size / bytes_per_line;

		if (buf_desc.height == 0U) {
			LOG_ERR("Buffer of %u bytes is shorter than a %u-byte line",
				buf_desc.buf_size, bytes_per_line);
			ret = -ENODATA;
		} else if (vbuf != NULL) {
			/* line_offset is only used to support partial video frame */
			ret = display_write(disp_sink->display_dev, 0, vbuf->line_offset, &buf_desc,
					    vbuf->buffer);
		} else {
			/* Fallback to net_buf data if no video_buffer metadata */
			ret = display_write(disp_sink->display_dev, 0, 0, &buf_desc, cur->data);
		}

		if (ret != 0) {
			if (ret != -ENODATA) {
				LOG_ERR("display_write failed (%d)", ret);
			}
			net_buf_unref(cur);
			if (next != NULL) {
				net_buf_unref(next);
			}
			return ret;
		}

		net_buf_unref(cur);
		cur = next;
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

	self->object.get_property = mpipe_disp_sink_get_property;
	self->object.set_property = mpipe_disp_sink_set_property;

	sink->sink_pad.chain_fn = mpipe_disp_sink_chain_fn;
	sink->sink_pad.enum_caps_fn = mpipe_disp_sink_enum_caps;
	sink->set_caps = mpipe_disp_sink_set_caps;

	return 0;
}
