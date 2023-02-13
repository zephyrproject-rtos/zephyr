/*
 * Copyright (c) 2019 Intel Corp.
 *
 * This code attempts to be endian-agnostic. It manipulates the framebuffer
 * address space only in 32-bit words (and assumes those words are 0xAARRGGBB).
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT intel_multiboot_framebuffer

#include <errno.h>

#include <zephyr/arch/x86/multiboot.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <string.h>

struct framebuf_dev_config {
	uint16_t width;
	uint16_t height;
};

struct framebuf_dev_data {
	void *buffer;
	uint32_t pitch;
};

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
	const struct framebuf_dev_config *config = dev->config;

	caps->x_resolution = config->width;
	caps->y_resolution = config->height;
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
	struct framebuf_dev_data *data = dev->data;
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
	struct framebuf_dev_data *data = dev->data;
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

static int multiboot_framebuf_init(const struct device *dev)
{
	const struct framebuf_dev_config *config = dev->config;
	struct framebuf_dev_data *data = dev->data;
	struct multiboot_info *info = &multiboot_info;

	if ((info->flags & MULTIBOOT_INFO_FLAGS_FB) &&
	    (info->fb_width >= config->width) &&
	    (info->fb_height >= config->height) &&
	    (info->fb_bpp == 32) && (info->fb_addr_hi == 0)) {
		/*
		 * We have a usable multiboot framebuffer - it is 32 bpp
		 * and at least as large as the requested dimensions. Compute
		 * the pitch and adjust the start address center our canvas.
		 */

		uint16_t adj_x;
		uint16_t adj_y;
		uint32_t *buffer;

		adj_x = info->fb_width - config->width;
		adj_y = info->fb_height - config->height;
		data->pitch = (info->fb_pitch / 4) + adj_x;
		adj_x /= 2U;
		adj_y /= 2U;
		buffer = (uint32_t *) (uintptr_t) info->fb_addr_lo;
		buffer += adj_x;
		buffer += adj_y * data->pitch;
		data->buffer = buffer;
		return 0;
	} else {
		return -ENOTSUP;
	}
}

static const struct framebuf_dev_config config = {
	.width = DT_INST_PROP(0, width),
	.height = DT_INST_PROP(0, height),
};

static struct framebuf_dev_data data;

DEVICE_DT_INST_DEFINE(0, multiboot_framebuf_init, NULL, &data, &config,
		      PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		      &framebuf_display_api);
