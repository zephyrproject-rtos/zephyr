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
#include <zephyr/kernel.h>

struct dummy_display_config {
	uint16_t height;
	uint16_t width;
};

#define DUMMY_DISPLAY_PIXEL_FORMATS                                                                \
	(PIXEL_FORMAT_ARGB_8888 | PIXEL_FORMAT_RGB_888 | PIXEL_FORMAT_MONO01 | PIXEL_FORMAT_MONO10)

#if CONFIG_DUMMY_DISPLAY_PLANES > 0
struct dummy_display_plane {
	struct display_plane_config staged;
	bool is_staged;
	bool enabled;
};
#endif

struct dummy_display_data {
	enum display_pixel_format current_pixel_format;
#if CONFIG_DUMMY_DISPLAY_PLANES > 0
	struct dummy_display_plane planes[CONFIG_DUMMY_DISPLAY_PLANES];
	struct k_mutex lock;
#endif
};

static int dummy_display_init(const struct device *dev)
{
	struct dummy_display_data *disp_data = dev->data;

	disp_data->current_pixel_format = PIXEL_FORMAT_ARGB_8888;
#if CONFIG_DUMMY_DISPLAY_PLANES > 0
	k_mutex_init(&disp_data->lock);
#endif

	return 0;
}

static int dummy_display_write(const struct device *dev, const uint16_t x,
			       const uint16_t y,
			       const struct display_buffer_descriptor *desc,
			       const void *buf)
{
	const struct dummy_display_config *config = dev->config;

	__ASSERT(desc->width <= desc->pitch, "Pitch is smaller than width");
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

	capabilities->x_resolution = config->width;
	capabilities->y_resolution = config->height;
	capabilities->supported_pixel_formats = DUMMY_DISPLAY_PIXEL_FORMATS;
	capabilities->current_pixel_format = disp_data->current_pixel_format;
	capabilities->screen_info = SCREEN_INFO_MONO_VTILED |
		SCREEN_INFO_MONO_MSB_FIRST;
	capabilities->num_planes = CONFIG_DUMMY_DISPLAY_PLANES;
}

#if CONFIG_DUMMY_DISPLAY_PLANES > 0
static int dummy_display_get_plane_capabilities(const struct device *dev, uint8_t plane,
						struct display_plane_capabilities *caps)
{
	const struct dummy_display_config *config = dev->config;

	if (plane >= CONFIG_DUMMY_DISPLAY_PLANES) {
		return -EINVAL;
	}

	caps->supported_pixel_formats = DUMMY_DISPLAY_PIXEL_FORMATS;
	caps->supported_blend_modes = (plane == 0U) ? 0U
						    : (DISPLAY_BLEND_NONE | DISPLAY_BLEND_COVERAGE |
						       DISPLAY_BLEND_PREMULTIPLIED);
	caps->max_width = config->width;
	caps->max_height = config->height;
	caps->width_align = 1U;

	return 0;
}

static int dummy_display_set_plane(const struct device *dev, uint8_t plane,
				   const struct display_plane_config *cfg)
{
	const struct dummy_display_config *config = dev->config;
	struct dummy_display_data *disp_data = dev->data;
	const struct display_buffer_descriptor *desc = &cfg->desc;
	uint32_t row_bytes;

	if (plane >= CONFIG_DUMMY_DISPLAY_PLANES) {
		return -EINVAL;
	}

	if (cfg->buf != NULL) {
		if ((cfg->pixel_format & DUMMY_DISPLAY_PIXEL_FORMATS) == 0U ||
		    (plane != 0U &&
		     (cfg->blend_mode & (DISPLAY_BLEND_NONE | DISPLAY_BLEND_COVERAGE |
					 DISPLAY_BLEND_PREMULTIPLIED)) == 0U)) {
			return -ENOTSUP;
		}

		row_bytes = (uint32_t)desc->pitch * DISPLAY_BITS_PER_PIXEL(cfg->pixel_format) /
			    BITS_PER_BYTE;
		if (desc->width == 0U || desc->height == 0U || desc->pitch < desc->width ||
		    cfg->x + desc->width > config->width ||
		    cfg->y + desc->height > config->height ||
		    desc->buf_size < row_bytes * desc->height) {
			return -EINVAL;
		}
	}

	k_mutex_lock(&disp_data->lock, K_FOREVER);
	disp_data->planes[plane].staged = *cfg;
	disp_data->planes[plane].is_staged = true;
	k_mutex_unlock(&disp_data->lock);

	return 0;
}

static int dummy_display_commit(const struct device *dev)
{
	struct dummy_display_data *disp_data = dev->data;

	k_mutex_lock(&disp_data->lock, K_FOREVER);
	for (uint8_t i = 0; i < CONFIG_DUMMY_DISPLAY_PLANES; i++) {
		struct dummy_display_plane *p = &disp_data->planes[i];

		if (p->is_staged) {
			p->enabled = p->staged.buf != NULL;
			p->is_staged = false;
		}
	}
	k_mutex_unlock(&disp_data->lock);

	return 0;
}
#endif /* CONFIG_DUMMY_DISPLAY_PLANES > 0 */

static int dummy_display_set_pixel_format(const struct device *dev,
		const enum display_pixel_format pixel_format)
{
	struct dummy_display_data *disp_data = dev->data;

	disp_data->current_pixel_format = pixel_format;
	return 0;
}

static DEVICE_API(display, dummy_display_api) = {
	.blanking_on = dummy_display_blanking_on,
	.blanking_off = dummy_display_blanking_off,
	.write = dummy_display_write,
	.set_brightness = dummy_display_set_brightness,
	.set_contrast = dummy_display_set_contrast,
	.get_capabilities = dummy_display_get_capabilities,
	.set_pixel_format = dummy_display_set_pixel_format,
#if CONFIG_DUMMY_DISPLAY_PLANES > 0
	.get_plane_capabilities = dummy_display_get_plane_capabilities,
	.set_plane = dummy_display_set_plane,
	.commit = dummy_display_commit,
#endif
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
