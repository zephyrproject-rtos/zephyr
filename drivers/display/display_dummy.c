/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <drivers/display.h>

struct dummy_display_data {
	enum display_pixel_format current_pixel_format;
};

static struct dummy_display_data dummy_display_data;

static int dummy_display_init(struct device *dev)
{
	struct dummy_display_data *disp_data =
	    (struct dummy_display_data *)dev->driver_data;

	disp_data->current_pixel_format = PIXEL_FORMAT_ARGB_8888;

	return 0;
}

static int dummy_display_write(const struct device *dev, const u16_t x,
			       const u16_t y,
			       const struct display_buffer_descriptor *desc,
			       const void *buf)
{
	return 0;
}

static int dummy_display_read(const struct device *dev, const u16_t x,
			      const u16_t y,
			      const struct display_buffer_descriptor *desc,
			      void *buf)
{
	return -ENOTSUP;
}

static void *dummy_display_get_framebuffer(const struct device *dev)
{
	return NULL;
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
					const u8_t brightness)
{
	return 0;
}

static int dummy_display_set_contrast(const struct device *dev,
				      const u8_t contrast)
{
	return 0;
}

static void dummy_display_get_capabilities(const struct device *dev,
		struct display_capabilities *capabilities)
{
	struct dummy_display_data *disp_data =
		(struct dummy_display_data *)dev->driver_data;

	memset(capabilities, 0, sizeof(struct display_capabilities));
	capabilities->x_resolution = CONFIG_DUMMY_DISPLAY_X_RES;
	capabilities->y_resolution = CONFIG_DUMMY_DISPLAY_Y_RES;
	capabilities->supported_pixel_formats = PIXEL_FORMAT_ARGB_8888 |
		PIXEL_FORMAT_RGB_888 |
		PIXEL_FORMAT_MONO01 |
		PIXEL_FORMAT_MONO10;
	capabilities->current_pixel_format = disp_data->current_pixel_format;
	capabilities->screen_info = SCREEN_INFO_MONO_VTILED |
		SCREEN_INFO_MONO_MSB_FIRST;
}

static int dummy_display_set_pixel_format(const struct device *dev,
		const enum display_pixel_format pixel_format)
{
	struct dummy_display_data *disp_data =
		(struct dummy_display_data *)dev->driver_data;

	disp_data->current_pixel_format = pixel_format;
	return 0;
}

static const struct display_driver_api dummy_display_api = {
	.blanking_on = dummy_display_blanking_on,
	.blanking_off = dummy_display_blanking_off,
	.write = dummy_display_write,
	.read = dummy_display_read,
	.get_framebuffer = dummy_display_get_framebuffer,
	.set_brightness = dummy_display_set_brightness,
	.set_contrast = dummy_display_set_contrast,
	.get_capabilities = dummy_display_get_capabilities,
	.set_pixel_format = dummy_display_set_pixel_format,
};

DEVICE_AND_API_INIT(dummy_display, CONFIG_DUMMY_DISPLAY_DEV_NAME,
		    &dummy_display_init, &dummy_display_data, NULL,
		    APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY,
		    &dummy_display_api);

