/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/display.h>
#include <zephyr/drivers/video.h>
#include <zephyr/logging/log.h>

#include <zephyr/mp/core/mp_buffer.h>
#include <zephyr/mp/core/mp_caps.h>
#include <zephyr/mp/core/mp_element.h>
#include <zephyr/mp/core/mp_structure.h>
#include <zephyr/mp/core/mp_value.h>

#include <zephyr/mp/zdisp/mp_zdisp_property.h>
#include <zephyr/mp/zdisp/mp_zdisp_sink.h>

LOG_MODULE_REGISTER(mp_zdisp_sink, CONFIG_MP_LOG_LEVEL);

#define DEFAULT_PROP_DEVICE DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_display))

/* Default supported minimum width/height may depend on the HW but currently no way to get it */
#define DEFAULT_WIDTH_MIN  1
#define DEFAULT_HEIGHT_MIN 1

struct mp_disp_vid_pix_fmt {
	enum display_pixel_format disp_fmt;
	uint32_t vid_fmt;
};

/*
 * Pixel formats mapping betwen video and display
 * Note: video fourcc pixel format is the standard used for caps
 */
static const struct mp_disp_vid_pix_fmt mp_disp_vid_pix_fmt_map[] = {
	{PIXEL_FORMAT_RGB_565, VIDEO_PIX_FMT_RGB565},
	{PIXEL_FORMAT_RGB_565X, VIDEO_PIX_FMT_RGB565X},
	{PIXEL_FORMAT_RGB_888, VIDEO_PIX_FMT_BGR24},
	{PIXEL_FORMAT_ARGB_8888, VIDEO_PIX_FMT_BGRA32},
	{PIXEL_FORMAT_XRGB_8888, VIDEO_PIX_FMT_BGRX32},
	{PIXEL_FORMAT_L_8, VIDEO_PIX_FMT_GREY},
};

static const uint32_t disp_to_vid_pix_fmt(enum display_pixel_format disp_fmt)
{
	for (uint8_t i = 0; i < ARRAY_SIZE(mp_disp_vid_pix_fmt_map); i++) {
		if (mp_disp_vid_pix_fmt_map[i].disp_fmt == disp_fmt) {
			return mp_disp_vid_pix_fmt_map[i].vid_fmt;
		}
	}

	return 0;
}

static const enum display_pixel_format vid_to_disp_pix_fmt(uint32_t vid_fmt)
{
	for (uint8_t i = 0; i < ARRAY_SIZE(mp_disp_vid_pix_fmt_map); i++) {
		if (mp_disp_vid_pix_fmt_map[i].vid_fmt == vid_fmt) {
			return mp_disp_vid_pix_fmt_map[i].disp_fmt;
		}
	}

	return 0;
}

static int mp_zdisp_sink_setup(struct mp_zdisp_sink *zdisp_sink,
			       const enum display_pixel_format pixfmt)
{
	int ret = 0;

	ret = display_set_pixel_format(zdisp_sink->display_dev, pixfmt);
	if (ret != 0) {
		LOG_ERR("Unable to set display format");
		return ret;
	}

	/* Turn off blanking if driver supports it */
	ret = display_blanking_off(zdisp_sink->display_dev);
	if (ret == -ENOSYS) {
		LOG_WRN("Display blanking off not available");
		ret = 0;
	}

	return ret;
}

static struct mp_caps *mp_zdisp_sink_supported_caps(struct mp_sink *sink)
{
	uint32_t vid_fmt;
	struct display_capabilities display_caps;
	struct mp_value *supported_fmt = mp_value_new(MP_TYPE_LIST, NULL);
	struct mp_zdisp_sink *zdisp = (struct mp_zdisp_sink *)sink;

	display_get_capabilities(zdisp->display_dev, &display_caps);

	for (uint8_t i = 0; i < ARRAY_SIZE(mp_disp_vid_pix_fmt_map); i++) {
		vid_fmt = disp_to_vid_pix_fmt(display_caps.supported_pixel_formats &
					      mp_disp_vid_pix_fmt_map[i].disp_fmt);
		if (vid_fmt != 0) {
			/* Use video formats as reference formats for caps */
			mp_value_list_append(supported_fmt, mp_value_new(MP_TYPE_UINT, vid_fmt));
		}
	}

	return mp_caps_new(MP_MEDIA_VIDEO, MP_CAPS_PIXEL_FORMAT, MP_TYPE_LIST, supported_fmt,
			   MP_CAPS_IMAGE_WIDTH, MP_TYPE_UINT_RANGE, DEFAULT_WIDTH_MIN,
			   display_caps.x_resolution, 1, MP_CAPS_IMAGE_HEIGHT, MP_TYPE_UINT_RANGE,
			   DEFAULT_HEIGHT_MIN, display_caps.y_resolution, 1, MP_CAPS_END);
}

static void mp_zdisp_sink_update_caps(struct mp_sink *sink)
{
	struct mp_caps *caps = mp_zdisp_sink_supported_caps(sink);

	mp_sink_update_caps(sink, caps);
	mp_caps_unref(caps);
}

static int mp_zdisp_sink_set_caps(struct mp_sink *sink, struct mp_caps *caps)
{
	struct mp_zdisp_sink *zdisp_sink = (struct mp_zdisp_sink *)sink;
	struct mp_structure *first_structure = mp_caps_get_structure(caps, 0);
	struct mp_value *value = mp_structure_get_value(first_structure, MP_CAPS_PIXEL_FORMAT);
	enum display_pixel_format disp_fmt = vid_to_disp_pix_fmt(mp_value_get_uint(value));

	if (disp_fmt == 0 || mp_zdisp_sink_setup(zdisp_sink, disp_fmt) != 0) {
		return -EINVAL;
	}

	mp_caps_replace(&sink->sinkpad.caps, caps);

	return 0;
}

static int mp_zdisp_sink_set_property(struct mp_object *obj, uint32_t key, const void *val)
{
	struct mp_zdisp_sink *zdisp_sink = (struct mp_zdisp_sink *)obj;
	struct mp_sink *sink = &zdisp_sink->sink;

	switch (key) {
	case PROP_ZDISP_SINK_DEVICE:
		zdisp_sink->display_dev = val;
		/* Device set, update caps */
		mp_zdisp_sink_update_caps(sink);

		return 0;
	default:
		return -ENOTSUP;
	}
}

static int mp_zdisp_sink_get_property(struct mp_object *obj, uint32_t key, void *val)
{
	struct mp_zdisp_sink *zdisp_sink = (struct mp_zdisp_sink *)obj;

	switch (key) {
	case PROP_ZDISP_SINK_DEVICE:
		*(const struct device **)val = zdisp_sink->display_dev;

		return 0;
	default:
		return -ENOTSUP;
	}
}

int mp_zdisp_sink_chainfn(struct mp_pad *pad, struct net_buf *in_buf, struct net_buf **out_buf)
{
	struct mp_zdisp_sink *zdisp_sink =
		CONTAINER_OF(pad->object.container, struct mp_zdisp_sink, sink.element.object);
	/* Get width / height from pad's caps */
	struct mp_structure *first_structure = mp_caps_get_structure(pad->caps, 0);
	struct mp_value *value = mp_structure_get_value(first_structure, MP_CAPS_PIXEL_FORMAT);
	enum display_pixel_format disp_fmt = 0;
	struct net_buf *cur;
	struct net_buf *next;

	if (value != NULL) {
		disp_fmt = vid_to_disp_pix_fmt(mp_value_get_uint(value));
	}

	/* Sink returns NULL for output buffer as it is at the end of the chain */
	*out_buf = NULL;

	if (in_buf == NULL) {
		return 0;
	}

	/* Input may be a fragment chain. Display each fragment then unref it */
	cur = in_buf;
	while (cur != NULL) {
		/*
		 * Currently, we assumme that in_buf is always a video buffer.
		 * TODO: Add support for other buffer types
		 */
		struct video_buffer *vbuf = (struct video_buffer *)mp_buffer_get_meta(cur)->priv;
		struct display_buffer_descriptor buf_desc = {
			.buf_size = mp_buffer_get_meta(cur)->bytes_used,
		};

		next = cur->frags;
		cur->frags = NULL;

		value = mp_structure_get_value(first_structure, MP_CAPS_IMAGE_WIDTH);
		if (value != NULL) {
			buf_desc.width = mp_value_get_int(value);
		}

		buf_desc.pitch = buf_desc.width;
		/* Do not get height from caps as sometimes buffer is just a partial frame */
		buf_desc.height =
			buf_desc.buf_size /
			(buf_desc.width * DISPLAY_BITS_PER_PIXEL(disp_fmt) / BITS_PER_BYTE);

		/* line_offset is only used to support partial video frame */
		if (vbuf != NULL) {
			display_write(zdisp_sink->display_dev, 0, vbuf->line_offset, &buf_desc,
				      vbuf->buffer);
		} else {
			/* Fallback to net_buf data if no video_buffer metadata */
			display_write(zdisp_sink->display_dev, 0, 0, &buf_desc, cur->data);
		}

		net_buf_unref(cur);
		cur = next;
	}

	return 0;
}

void mp_zdisp_sink_init(struct mp_element *self)
{
	struct mp_zdisp_sink *zdisp_sink = (struct mp_zdisp_sink *)self;
	struct mp_sink *sink = &zdisp_sink->sink;

	/* Init base class */
	mp_sink_init(self);

	zdisp_sink->display_dev = DEFAULT_PROP_DEVICE;

	self->object.get_property = mp_zdisp_sink_get_property;
	self->object.set_property = mp_zdisp_sink_set_property;

	sink->sinkpad.chainfn = mp_zdisp_sink_chainfn;
	sink->set_caps = mp_zdisp_sink_set_caps;

	mp_zdisp_sink_update_caps(sink);
}
