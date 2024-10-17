/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_dummy_dc

#include <errno.h>
#include <string.h>

#include <zephyr/drivers/display.h>
#include <zephyr/device.h>

struct dummy_display_config {
	uint16_t height;
	uint16_t width;
};

struct dummy_display_data {
	enum display_pixel_format current_pixel_format;
};

static int dummy_display_init(const struct device *dev)
{
	struct dummy_display_data *disp_data = dev->data;

	disp_data->current_pixel_format = PIXEL_FORMAT_ARGB_8888;

	return 0;
}

static int dummy_display_write(const struct device *dev, const uint16_t x,
			       const uint16_t y,
			       const struct display_buffer_descriptor *desc,
			       const void *buf)
{
	const struct dummy_display_config *config = dev->config;

	__ASSERT(desc->width <= desc->pitch, "Pitch is smaller then width");
	__ASSERT(desc->pitch <= config->width,
		"Pitch in descriptor is larger than screen size");
	__ASSERT(desc->height <= config->height,
		"Height in descriptor is larger than screen size");
	__ASSERT(x + desc->pitch <= config->width,
		 "Writing outside screen boundaries in horizontal direction");
	__ASSERT(y + desc->height <= config->height,
		 "Writing outside screen boundaries in vertical direction");

	if (desc->width > desc->pitch ||
	    x + desc->pitch > config->width ||
	    y + desc->height > config->height) {
		return -EINVAL;
	}

	return 0;
}

static int dummy_display_show(const struct device *dev)
{
	return 0;
}

static int dummy_display_blanking_off(const struct device *dev)
{
	return 0;
}

static int dummy_display_blanking_on(const struct device *dev)
{
	return 0;
}

static int dummy_display_set_brightness(const struct device *dev,
					const uint8_t brightness)
{
	return 0;
}

static int dummy_display_set_contrast(const struct device *dev,
				      const uint8_t contrast)
{
	return 0;
}

static void dummy_display_get_capabilities(const struct device *dev,
		struct display_capabilities *capabilities)
{
	const struct dummy_display_config *config = dev->config;
	struct dummy_display_data *disp_data = dev->data;

	memset(capabilities, 0, sizeof(struct display_capabilities));
	capabilities->x_resolution = config->width;
	capabilities->y_resolution = config->height;
	capabilities->supported_pixel_formats = PIXEL_FORMAT_ARGB_8888 |
		PIXEL_FORMAT_RGB_888 |
		PIXEL_FORMAT_MONO01 |
		PIXEL_FORMAT_MONO10;
	capabilities->current_pixel_format = disp_data->current_pixel_format;
	capabilities->screen_info =
		SCREEN_INFO_MONO_VTILED | SCREEN_INFO_MONO_MSB_FIRST | SCREEN_INFO_REQUIRES_SHOW;
}

static int dummy_display_set_pixel_format(const struct device *dev,
		const enum display_pixel_format pixel_format)
{
	struct dummy_display_data *disp_data = dev->data;

	disp_data->current_pixel_format = pixel_format;
	return 0;
}

static const struct display_driver_api dummy_display_api = {
	.blanking_on = dummy_display_blanking_on,
	.blanking_off = dummy_display_blanking_off,
	.write = dummy_display_write,
	.show = dummy_display_show,
	.set_brightness = dummy_display_set_brightness,
	.set_contrast = dummy_display_set_contrast,
	.get_capabilities = dummy_display_get_capabilities,
	.set_pixel_format = dummy_display_set_pixel_format,
};

#define DISPLAY_DUMMY_DEFINE(n)						\
	static const struct dummy_display_config dd_config_##n = {	\
		.height = DT_INST_PROP(n, height),			\
		.width = DT_INST_PROP(n, width),			\
	};								\
									\
	static struct dummy_display_data dd_data_##n;			\
									\
	DEVICE_DT_INST_DEFINE(n, &dummy_display_init, NULL,		\
			      &dd_data_##n,				\
			      &dd_config_##n,				\
			      POST_KERNEL,				\
			      CONFIG_DISPLAY_INIT_PRIORITY,		\
			      &dummy_display_api);			\

DT_INST_FOREACH_STATUS_OKAY(DISPLAY_DUMMY_DEFINE)
