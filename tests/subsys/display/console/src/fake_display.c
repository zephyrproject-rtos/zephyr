/*
 * SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT vnd_display_fake

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/sys/atomic.h>

#include "fake_display.h"

static atomic_t fake_display_write_count;

uint32_t fake_display_writes(void)
{
	return (uint32_t)atomic_get(&fake_display_write_count);
}

static int fake_display_write(const struct device *dev, const uint16_t x, const uint16_t y,
			      const struct display_buffer_descriptor *desc, const void *buf)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(x);
	ARG_UNUSED(y);
	ARG_UNUSED(desc);
	ARG_UNUSED(buf);

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
		.supported_pixel_formats = PIXEL_FORMAT_RGB_565,
		.current_pixel_format = PIXEL_FORMAT_RGB_565,
	};
}

static int fake_display_set_pixel_format(const struct device *dev,
					 const enum display_pixel_format pixel_format)
{
	ARG_UNUSED(dev);

	return (pixel_format == PIXEL_FORMAT_RGB_565) ? 0 : -ENOTSUP;
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
