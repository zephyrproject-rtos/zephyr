/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>

#include <src/core/mp_buffer.h>
#include <src/core/mp_pixel_format.h>
#include <src/core/mp_plugin.h>

#include "mp_zdisp_sink.h"

LOG_MODULE_REGISTER(mp_zdisp_sink, CONFIG_LIBMP_LOG_LEVEL);

#define DEFAULT_PROP_DEVICE DEVICE_DT_GET(DT_CHOSEN(zephyr_display))

/* Default supported minimum width/height may depend on the HW but currently no way to get it */
#define DEFAULT_WIDTH_MIN  1
#define DEFAULT_HEIGHT_MIN 1

enum {
	PROP_DEVICE,
};

typedef struct {
	MpPixelFormat mp_fmt;
	enum display_pixel_format zdisp_fmt;
} MpZdispPixfmtDesc;

/* Keep this array in the same order and in sync with Zephyr display pixel formats */
static const MpZdispPixfmtDesc mp_zdisp_pixfmt_map[] = {
	{MP_PIXEL_FORMAT_RGB24, PIXEL_FORMAT_RGB_888},
	{MP_PIXEL_FORMAT_MONO01, PIXEL_FORMAT_MONO01},
	{MP_PIXEL_FORMAT_MONO10, PIXEL_FORMAT_MONO10},
	{MP_PIXEL_FORMAT_ARGB32, PIXEL_FORMAT_ARGB_8888},
	{MP_PIXEL_FORMAT_RGB565, PIXEL_FORMAT_RGB_565},
	{MP_PIXEL_FORMAT_BGR565, PIXEL_FORMAT_BGR_565},
	{MP_PIXEL_FORMAT_GREY8, PIXEL_FORMAT_L_8},
	{MP_PIXEL_FORMAT_AL88, PIXEL_FORMAT_AL_88},
	{MP_PIXEL_FORMAT_XRGB32, PIXEL_FORMAT_XRGB_8888},
};

static const MpPixelFormat zdisp2mp_pixfmt(enum display_pixel_format zdisp_fmt)
{
	for (uint8_t i = 0; i < ARRAY_SIZE(mp_zdisp_pixfmt_map); i++) {
		if (mp_zdisp_pixfmt_map[i].zdisp_fmt == zdisp_fmt) {
			return mp_zdisp_pixfmt_map[i].mp_fmt;
		}
	}

	return MP_PIXEL_FORMAT_UNKNOWN;
}

static const enum display_pixel_format mp2zdisp_pixfmt(MpPixelFormat mp_fmt)
{
	for (uint8_t i = 0; i < ARRAY_SIZE(mp_zdisp_pixfmt_map); i++) {
		if (mp_zdisp_pixfmt_map[i].mp_fmt == mp_fmt) {
			return mp_zdisp_pixfmt_map[i].zdisp_fmt;
		}
	}

	return 0;
}

static int mp_zdisp_sink_set_property(MpObject *obj, uint32_t key, const void *val)
{
	return 0;
}

static int mp_zdisp_sink_get_property(MpObject *obj, uint32_t key, void *val)
{
	return 0;
}

static int mp_zdisp_sink_setup(MpZdispSink *zdisp_sink, const enum display_pixel_format pixfmt)
{
	int ret = 0;

	LOG_INF("Display device: %s", zdisp_sink->display_dev->name);

	ret = display_set_pixel_format(zdisp_sink->display_dev, pixfmt);
	if (ret) {
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

bool mp_zdisp_sink_chainfn(MpPad *pad, MpBuffer *buffer)
{
	MpZdispSink *zdisp_sink = MP_ZDISP_SINK(pad->object.container);

	/* Get width / height from pad's caps */
	MpStructure *first_structure = mp_caps_get_structure(pad->caps, 0);
	MpValue *value;
	struct display_buffer_descriptor buf_desc = {
		.buf_size = buffer->bytesused,
	};

	value = mp_structure_get_value(first_structure, "width");
	if (value) {
		buf_desc.width = mp_value_get_int(value);
		buf_desc.pitch = mp_value_get_int(value);
	}

	value = mp_structure_get_value(first_structure, "height");
	if (value) {
		buf_desc.height = mp_value_get_int(value);
	}

	display_write(zdisp_sink->display_dev, 0, buffer->line_offset, &buf_desc, buffer->data);

	/* Done with the buffer, unref it */
	mp_buffer_unref(buffer);

	return true;
}

static MpCaps *mp_zdisp_sink_get_caps(MpSink *sink)
{
	MpPixelFormat mp_fmt;
	struct display_capabilities display_caps;
	MpValue *supported_fmt = mp_value_new(MP_TYPE_LIST, NULL);

	display_get_capabilities(MP_ZDISP_SINK(sink)->display_dev, &display_caps);

	for (uint8_t i = 0; i < ARRAY_SIZE(mp_zdisp_pixfmt_map); i++) {
		mp_fmt = zdisp2mp_pixfmt(display_caps.supported_pixel_formats &
					 mp_zdisp_pixfmt_map[i].zdisp_fmt);
		if (mp_fmt != MP_PIXEL_FORMAT_UNKNOWN) {
			mp_value_list_append(supported_fmt, mp_value_new(MP_TYPE_UINT, mp_fmt));
		}
	}

	return mp_caps_new("video/x-raw", "format", MP_TYPE_LIST, supported_fmt, "width",
			   MP_TYPE_INT_RANGE, DEFAULT_WIDTH_MIN, display_caps.x_resolution, 1,
			   "height", MP_TYPE_INT_RANGE, DEFAULT_HEIGHT_MIN,
			   display_caps.y_resolution, 1, NULL);
}

static bool mp_zdisp_sink_set_caps(MpSink *sink, MpCaps *caps)
{
	MpStructure *first_structure = mp_caps_get_structure(caps, 0);
	MpValue *value = mp_structure_get_value(first_structure, "format");
	enum display_pixel_format zdisp_fmt = mp2zdisp_pixfmt(mp_value_get_uint(value));

	if (zdisp_fmt == 0 || mp_zdisp_sink_setup(MP_ZDISP_SINK(sink), zdisp_fmt) != 0) {
		return false;
	}

	mp_caps_replace(&sink->sinkpad.caps, caps);

	return true;
}

void mp_zdisp_sink_init(MpElement *self)
{
	MpSink *sink = MP_SINK(self);
	MpZdispSink *zdisp_sink = MP_ZDISP_SINK(self);

	/* Init base class */
	mp_sink_init(self);

	zdisp_sink->display_dev = DEFAULT_PROP_DEVICE;

	self->object.get_property = mp_zdisp_sink_get_property;
	self->object.set_property = mp_zdisp_sink_set_property;

	sink->sinkpad.chainfn = mp_zdisp_sink_chainfn;
	sink->set_caps = mp_zdisp_sink_set_caps;
	sink->get_caps = mp_zdisp_sink_get_caps;
	sink->sinkpad.caps = mp_zdisp_sink_get_caps(sink);

	LOG_DBG("mp_zdisp_sink_init called !!!");
}

static void plugin_init()
{
	MP_ELEMENTFACTORY_DEFINE(zdisp_sink, sizeof(MpZdispSink), mp_zdisp_sink_init);
}

MP_PLUGIN_DEFINE(zdisp, plugin_init);
