/*
 * Copyright (c) 2024 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT led_strip_matrix

#include <zephyr/drivers/display.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(led_strip_matrix, CONFIG_DISPLAY_LOG_LEVEL);

struct led_strip_buffer {
	const struct device *const dev;
	const size_t chain_length;
	struct led_rgb *pixels;
};

struct led_strip_matrix_config {
	size_t num_of_strips;
	const struct led_strip_buffer *strips;
	uint16_t height;
	uint16_t width;
	uint16_t module_width;
	uint16_t module_height;
	bool circulative;
	bool start_from_right;
	bool start_from_bottom;
	bool modules_circulative;
	bool modules_start_from_right;
	bool modules_start_from_bottom;
	enum display_pixel_format pixel_format;
};

static size_t pixel_index(const struct led_strip_matrix_config *config, uint16_t x, uint16_t y)
{
	const size_t mods_per_row = config->width / config->module_width;
	const size_t mod_w = config->module_width;
	const size_t mod_h = config->module_height;
	const size_t mod_pixels = mod_w * mod_h;
	const size_t mod_row =
		config->modules_start_from_bottom ? (mod_h - 1) - (y / mod_h) : y / mod_h;
	const size_t y_in_mod = config->start_from_bottom ? (mod_h - 1) - (y % mod_h) : y % mod_h;
	size_t mod_col = x / mod_w;
	size_t x_in_mod = x % mod_w;

	if (config->modules_circulative) {
		if (config->modules_start_from_right) {
			mod_col = mods_per_row - 1 - mod_col;
		}
	} else {
		if ((mod_row % 2) == !config->modules_start_from_right) {
			mod_col = mods_per_row - 1 - mod_col;
		}
	}

	if (config->circulative) {
		if (config->start_from_right) {
			x_in_mod = (mod_w - 1) - (x % mod_w);
		}
	} else {
		if ((y_in_mod % 2) == !config->start_from_right) {
			x_in_mod = (mod_w - 1) - (x % mod_w);
		}
	}

	return (mods_per_row * mod_row + mod_col) * mod_pixels + y_in_mod * mod_w + x_in_mod;
}

static struct led_rgb *pixel_address(const struct led_strip_matrix_config *config, uint16_t x,
				     uint16_t y)
{
	size_t idx = pixel_index(config, x, y);

	for (size_t i = 0; i < config->num_of_strips; i++) {
		if (idx < config->strips[i].chain_length) {
			return &config->strips[i].pixels[idx];
		}
		idx -= config->strips[i].chain_length;
	}

	return NULL;
}

static inline int check_descriptor(const struct led_strip_matrix_config *config, const uint16_t x,
				   const uint16_t y, const struct display_buffer_descriptor *desc)
{
	__ASSERT(desc->width <= desc->pitch, "Pitch is smaller then width");
	__ASSERT(desc->pitch <= config->width, "Pitch in descriptor is larger than screen size");
	__ASSERT(desc->height <= config->height, "Height in descriptor is larger than screen size");
	__ASSERT(x + desc->pitch <= config->width,
		 "Writing outside screen boundaries in horizontal direction");
	__ASSERT(y + desc->height <= config->height,
		 "Writing outside screen boundaries in vertical direction");

	if (desc->width > desc->pitch || x + desc->pitch > config->width ||
	    y + desc->height > config->height) {
		return -EINVAL;
	}

	return 0;
}

static int led_strip_matrix_write(const struct device *dev, const uint16_t x, const uint16_t y,
				  const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct led_strip_matrix_config *config = dev->config;
	const uint8_t *buf_ptr = buf;
	int rc;

	rc = check_descriptor(config, x, y, desc);
	if (rc) {
		LOG_ERR("Invalid descriptor: %d", rc);
		return rc;
	}

	for (size_t ypos = y; ypos < (y + desc->height); ypos++) {
		for (size_t xpos = x; xpos < (x + desc->width); xpos++) {
			struct led_rgb *pix = pixel_address(config, xpos, ypos);

			if (config->pixel_format == PIXEL_FORMAT_ARGB_8888) {
				uint32_t color = *((uint32_t *)buf_ptr);

				pix->r = (color >> 16) & 0xFF;
				pix->g = (color >> 8) & 0xFF;
				pix->b = (color) & 0xFF;

				buf_ptr += 4;
			} else {
				pix->r = *buf_ptr;
				buf_ptr++;
				pix->g = *buf_ptr;
				buf_ptr++;
				pix->b = *buf_ptr;
				buf_ptr++;
			}
		}
		buf_ptr += (desc->pitch - desc->width) *
			   (config->pixel_format == PIXEL_FORMAT_ARGB_8888 ? 4 : 3);
	}

	for (size_t i = 0; i < config->num_of_strips; i++) {
		rc = led_strip_update_rgb(config->strips[i].dev, config->strips[i].pixels,
					  config->width * config->height);
		if (rc) {
			LOG_ERR("couldn't update strip: %d", rc);
		}
	}

	return rc;
}

static int led_strip_matrix_read(const struct device *dev, const uint16_t x, const uint16_t y,
				 const struct display_buffer_descriptor *desc, void *buf)
{
	const struct led_strip_matrix_config *config = dev->config;
	uint8_t *buf_ptr = buf;
	int rc;

	rc = check_descriptor(config, x, y, desc);
	if (rc) {
		LOG_ERR("Invalid descriptor: %d", rc);
		return rc;
	}

	for (size_t ypos = y; ypos < (y + desc->height); ypos++) {
		for (size_t xpos = x; xpos < (x + desc->width); xpos++) {
			struct led_rgb *pix = pixel_address(config, xpos, ypos);

			if (config->pixel_format == PIXEL_FORMAT_ARGB_8888) {
				uint32_t *pix_ptr = (uint32_t *)buf_ptr;

				*pix_ptr = 0xFF000000 | pix->r << 16 | pix->g << 8 | pix->b;
			} else {
				*buf_ptr = pix->r;
				buf_ptr++;
				*buf_ptr = pix->g;
				buf_ptr++;
				*buf_ptr = pix->b;
				buf_ptr++;
			}
		}
		buf_ptr += (desc->pitch - desc->width) *
			   (config->pixel_format == PIXEL_FORMAT_ARGB_8888 ? 4 : 3);
	}

	return 0;
}

static void led_strip_matrix_get_capabilities(const struct device *dev,
					      struct display_capabilities *caps)
{
	const struct led_strip_matrix_config *config = dev->config;

	memset(caps, 0, sizeof(struct display_capabilities));
	caps->x_resolution = config->width;
	caps->y_resolution = config->height;
	caps->supported_pixel_formats = PIXEL_FORMAT_ARGB_8888 | PIXEL_FORMAT_RGB_888;
	caps->current_pixel_format = config->pixel_format;
	caps->screen_info = 0;
}

static const struct display_driver_api led_strip_matrix_api = {
	.write = led_strip_matrix_write,
	.read = led_strip_matrix_read,
	.get_capabilities = led_strip_matrix_get_capabilities,
};

static int led_strip_matrix_init(const struct device *dev)
{
	const struct led_strip_matrix_config *config = dev->config;

	for (size_t i = 0; i < config->num_of_strips; i++) {
		if (!device_is_ready(config->strips[i].dev)) {
			LOG_ERR("LED strip device %s is not ready", config->strips[i].dev->name);
			return -EINVAL;
		}
	}

	return 0;
}

#define CHAIN_LENGTH(idx, inst)                                                                    \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, chain_lengths),                                    \
		    (DT_INST_PROP_BY_IDX(inst, chain_lengths, idx)),                               \
		    (DT_INST_PROP_BY_PHANDLE_IDX(inst, led_strips, idx, chain_length)))

#define STRIP_BUFFER_INITIALIZER(idx, inst)                                                        \
	{                                                                                          \
		.dev = DEVICE_DT_GET(DT_INST_PROP_BY_IDX(inst, led_strips, idx)),                  \
		.chain_length = CHAIN_LENGTH(idx, inst),                                           \
		.pixels = pixels##inst##_##idx,                                                    \
	}

#define DECLARE_PIXELS(idx, inst)                                                                  \
	static struct led_rgb pixels##inst##_##idx[CHAIN_LENGTH(idx, inst)];

#define AMOUNT_OF_LEDS(inst) LISTIFY(DT_INST_PROP_LEN(inst, led_strips), CHAIN_LENGTH, (+), inst)

#define VALIDATE_CHAIN_LENGTH(idx, inst)                                                           \
	BUILD_ASSERT(                                                                              \
		CHAIN_LENGTH(idx, inst) %                                                          \
			(DT_INST_PROP(inst, width) / DT_INST_PROP(inst, horizontal_modules) *      \
			 (DT_INST_PROP(inst, height) / DT_INST_PROP(inst, vertical_modules))) ==   \
		0);

#define LED_STRIP_MATRIX_DEFINE(inst)                                                              \
	LISTIFY(DT_INST_PROP_LEN(inst, led_strips), DECLARE_PIXELS, (;), inst);                    \
	static const struct led_strip_buffer strip_buffer##inst[] = {                              \
		LISTIFY(DT_INST_PROP_LEN(inst, led_strips), STRIP_BUFFER_INITIALIZER, (,), inst),  \
	};                                                                                         \
	static const struct led_strip_matrix_config dd_config_##inst = {                           \
		.num_of_strips = DT_INST_PROP_LEN(inst, led_strips),                               \
		.strips = strip_buffer##inst,                                                      \
		.width = DT_INST_PROP(inst, width),                                                \
		.height = DT_INST_PROP(inst, height),                                              \
		.module_width =                                                                    \
			DT_INST_PROP(inst, width) / DT_INST_PROP(inst, horizontal_modules),        \
		.module_height =                                                                   \
			DT_INST_PROP(inst, height) / DT_INST_PROP(inst, vertical_modules),         \
		.circulative = DT_INST_PROP(inst, circulative),                                    \
		.start_from_right = DT_INST_PROP(inst, start_from_right),                          \
		.modules_circulative = DT_INST_PROP(inst, modules_circulative),                    \
		.modules_start_from_right = DT_INST_PROP(inst, modules_start_from_right),          \
		.pixel_format = DT_INST_PROP(inst, pixel_format),                                  \
	};                                                                                         \
                                                                                                   \
	BUILD_ASSERT((DT_INST_PROP(inst, pixel_format) == PIXEL_FORMAT_RGB_888) ||                 \
		     (DT_INST_PROP(inst, pixel_format) == PIXEL_FORMAT_ARGB_8888));                \
	BUILD_ASSERT((DT_INST_PROP(inst, width) * DT_INST_PROP(inst, height)) ==                   \
		     AMOUNT_OF_LEDS(inst));                                                        \
	BUILD_ASSERT((DT_INST_PROP(inst, width) % DT_INST_PROP(inst, horizontal_modules)) == 0);   \
	BUILD_ASSERT((DT_INST_PROP(inst, height) % DT_INST_PROP(inst, vertical_modules)) == 0);    \
	LISTIFY(DT_INST_PROP_LEN(inst, led_strips), VALIDATE_CHAIN_LENGTH, (;), inst);             \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, led_strip_matrix_init, NULL, NULL, &dd_config_##inst,          \
			      POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY,                       \
			      &led_strip_matrix_api);

DT_INST_FOREACH_STATUS_OKAY(LED_STRIP_MATRIX_DEFINE)
