/*
 * Copyright (c) 2026 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "display_framebuffer.h"

int display_framebuffer_read(const struct device *dev, uint16_t x, uint16_t y,
			     const struct display_buffer_descriptor *desc, void *buf)
{
	const struct display_framebuffer_common_config *cfg = dev->config;
	struct display_framebuffer_common_data *data = dev->data;
	uint32_t *src = (uint32_t *)data->fb_addr;
	uint32_t *dst = (uint32_t *)buf;

	if ((x + desc->width > cfg->width) || (y + desc->height > cfg->height) ||
	    desc->pitch < desc->width) {
		return -EINVAL;
	}

	if (desc->buf_size < ((size_t)desc->pitch * desc->height * data->bytes_per_pixel)) {
		return -EINVAL;
	}

	src += x + (y * data->pitch);

	for (uint32_t row = 0; row < desc->height; row++) {
		memcpy(dst, src, desc->width * sizeof(uint32_t));
		src += data->pitch;
		dst += desc->pitch;
	}

	return 0;
}

int display_framebuffer_write(const struct device *dev, uint16_t x, uint16_t y,
			      const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct display_framebuffer_common_config *cfg = dev->config;
	struct display_framebuffer_common_data *data = dev->data;
	uint32_t *dst = (uint32_t *)data->fb_addr;
	const uint32_t *src = (const uint32_t *)buf;

	if ((x + desc->width > cfg->width) || (y + desc->height > cfg->height) ||
	    desc->pitch < desc->width) {
		return -EINVAL;
	}

	if (desc->buf_size < ((size_t)desc->pitch * desc->height * data->bytes_per_pixel)) {
		return -EINVAL;
	}

	dst += x + (y * data->pitch);

	for (uint32_t row = 0; row < desc->height; row++) {
		memcpy(dst, src, desc->width * sizeof(uint32_t));
		dst += data->pitch;
		src += desc->pitch;
	}

	return 0;
}
