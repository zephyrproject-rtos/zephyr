/*
 * Copyright (c) 2019 Intel Corp.
 *
 * This is most of the display driver for a "standard" 32-bpp framebuffer.
 * Device-specific drivers must still create the device instance and initialize
 * it accordingly, but this driver implements most/all of the API functions.
 * This code attempts to be endian-agnostic. It manipulates the framebuffer
 * address space only in 32-bit words (and assumes those words are 0xAARRGGBB).
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/display.h>
#include <display/framebuf.h>
#include <string.h>

static int framebuf_blanking_on(const struct device *dev)
{
	return -ENOTSUP;
}

static int framebuf_blanking_off(const struct device *dev)
{
	return -ENOTSUP;
}

static void *framebuf_get_framebuffer(const struct device *dev)
{
	return NULL;
}

static int framebuf_set_brightness(const struct device *dev,
				   const uint8_t brightness)
{
	return -ENOTSUP;
}

static int framebuf_set_contrast(const struct device *dev,
				 const uint8_t contrast)
{
	return -ENOTSUP;
}

static int framebuf_set_pixel_format(const struct device *dev,
				     const enum display_pixel_format format)
{
	switch (format) {
	case PIXEL_FORMAT_ARGB_8888:
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int framebuf_set_orientation(const struct device *dev,
				    const enum display_orientation orientation)
{
	switch (orientation) {
	case DISPLAY_ORIENTATION_NORMAL:
		return 0;
	default:
		return -ENOTSUP;
	}
}

static void framebuf_get_capabilities(const struct device *dev,
				      struct display_capabilities *caps)
{
	struct framebuf_dev_data *data = FRAMEBUF_DATA(dev);

	caps->x_resolution = data->width;
	caps->y_resolution = data->height;
	caps->supported_pixel_formats = PIXEL_FORMAT_ARGB_8888;
	caps->screen_info = 0;
	caps->current_pixel_format = PIXEL_FORMAT_ARGB_8888;
	caps->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static int framebuf_write(const struct device *dev, const uint16_t x,
			  const uint16_t y,
			  const struct display_buffer_descriptor *desc,
			  const void *buf)
{
	struct framebuf_dev_data *data = FRAMEBUF_DATA(dev);
	uint32_t *dst = data->buffer;
	const uint32_t *src = buf;
	uint32_t row;

	dst += x;
	dst += (y * data->pitch);

	for (row = 0; row < desc->height; ++row) {
		(void) memcpy(dst, src, desc->width * sizeof(uint32_t));
		dst += data->pitch;
		src += desc->pitch;
	}

	return 0;
}

static int framebuf_read(const struct device *dev, const uint16_t x,
			 const uint16_t y,
			 const struct display_buffer_descriptor *desc,
			 void *buf)
{
	struct framebuf_dev_data *data = FRAMEBUF_DATA(dev);
	uint32_t *src = data->buffer;
	uint32_t *dst = buf;
	uint32_t row;

	src += x;
	src += (y * data->pitch);

	for (row = 0; row < desc->height; ++row) {
		(void) memcpy(dst, src, desc->width * sizeof(uint32_t));
		src += data->pitch;
		dst += desc->pitch;
	}

	return 0;
}

const struct display_driver_api framebuf_display_api = {
	.blanking_on = framebuf_blanking_on,
	.blanking_off = framebuf_blanking_off,
	.write = framebuf_write,
	.read = framebuf_read,
	.get_framebuffer = framebuf_get_framebuffer,
	.set_brightness = framebuf_set_brightness,
	.set_contrast = framebuf_set_contrast,
	.get_capabilities = framebuf_get_capabilities,
	.set_pixel_format = framebuf_set_pixel_format,
	.set_orientation = framebuf_set_orientation
};
