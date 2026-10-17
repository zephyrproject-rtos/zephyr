/*
 * SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT vnd_display_fake

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#include "fake_display.h"

static atomic_t fake_display_write_count;

static const enum display_pixel_format fake_display_formats[] = {
	PIXEL_FORMAT_RGB_565, PIXEL_FORMAT_RGB_888, PIXEL_FORMAT_ARGB_8888,
	PIXEL_FORMAT_L_8,     PIXEL_FORMAT_MONO01,  PIXEL_FORMAT_MONO10,
};

#define FAKE_DISPLAY_FORMAT fake_display_formats[DT_INST_ENUM_IDX(0, pixel_format)]
#define FAKE_DISPLAY_VTILED DT_INST_PROP(0, mono_vtiled)

/* Bytes a write of the given size takes in the configured format. */
static size_t fake_display_buf_size(uint16_t w, uint16_t h)
{
	switch (FAKE_DISPLAY_FORMAT) {
	case PIXEL_FORMAT_RGB_565:
		return (size_t)w * h * 2U;
	case PIXEL_FORMAT_RGB_888:
		return (size_t)w * h * 3U;
	case PIXEL_FORMAT_ARGB_8888:
		return (size_t)w * h * 4U;
	case PIXEL_FORMAT_L_8:
		return (size_t)w * h;
	default:
		return FAKE_DISPLAY_VTILED ? (size_t)w * DIV_ROUND_UP(h, 8U)
					   : DIV_ROUND_UP(w, 8U) * h;
	}
}

uint32_t fake_display_writes(void)
{
	return (uint32_t)atomic_get(&fake_display_write_count);
}

static int fake_display_write(const struct device *dev, const uint16_t x, const uint16_t y,
			      const struct display_buffer_descriptor *desc, const void *buf)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(y);
	ARG_UNUSED(buf);

	if (DT_INST_PROP(0, x_alignment_width) &&
	    (x != 0U || desc->width != DT_INST_PROP(0, width))) {
		return -ENOTSUP;
	}

	if (desc->buf_size < fake_display_buf_size(desc->width, desc->height)) {
		return -EINVAL;
	}

	atomic_inc(&fake_display_write_count);

	return 0;
}

static int fake_display_blanking(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static void fake_display_get_capabilities(const struct device *dev,
					  struct display_capabilities *caps)
{
	ARG_UNUSED(dev);

	*caps = (struct display_capabilities){
		.x_resolution = DT_INST_PROP(0, width),
		.y_resolution = DT_INST_PROP(0, height),
		.supported_pixel_formats = FAKE_DISPLAY_FORMAT,
		.current_pixel_format = FAKE_DISPLAY_FORMAT,
		.screen_info =
			(DT_INST_PROP(0, x_alignment_width) ? SCREEN_INFO_X_ALIGNMENT_WIDTH : 0) |
			(FAKE_DISPLAY_VTILED ? SCREEN_INFO_MONO_VTILED : 0),
	};
}

static int fake_display_set_pixel_format(const struct device *dev,
					 const enum display_pixel_format pixel_format)
{
	ARG_UNUSED(dev);

	return (pixel_format == FAKE_DISPLAY_FORMAT) ? 0 : -ENOTSUP;
}

static DEVICE_API(display, fake_display_api) = {
	.blanking_on = fake_display_blanking,
	.blanking_off = fake_display_blanking,
	.write = fake_display_write,
	.get_capabilities = fake_display_get_capabilities,
	.set_pixel_format = fake_display_set_pixel_format,
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY,
		      &fake_display_api);
