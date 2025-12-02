/*
 * Copyright (c) 2025 Ambiq Micro Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sharp_ls014b7dd01

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/jdi.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <string.h>

LOG_MODULE_REGISTER(ls014b7dd01, CONFIG_DISPLAY_LOG_LEVEL);

#if !DT_NODE_EXISTS(DT_CHOSEN(zephyr_display))
#error "Unsupported board: zephyr,display is not assigned"
#endif

struct ls014b7dd01_config {
	const struct device *jdi;
	struct pwm_dt_spec va;
	struct pwm_dt_spec vcom;
	const struct jdi_device device;
	uint32_t rotation;
};

struct ls014b7dd01_data {
	enum display_pixel_format pixel_format;
	enum display_orientation orientation;
};

static int ls014b7dd01_blanking_on(const struct device *dev)
{
	const struct ls014b7dd01_config *config = dev->config;
	int ret;

	ret = pwm_set_dt(&config->va, 0, 0);
	if (ret < 0) {
		LOG_ERR("PWM set failed: %d", ret);
		return ret;
	}

	ret = pwm_set_dt(&config->vcom, 0, 0);
	if (ret < 0) {
		LOG_ERR("PWM set failed: %d", ret);
		return ret;
	}

	return 0;
}

static int ls014b7dd01_blanking_off(const struct device *dev)
{
	const struct ls014b7dd01_config *config = dev->config;
	int ret;

	LOG_DBG("Turning display blanking off");
	ret = pwm_set_dt(&config->va, config->va.period, config->va.period / 2);
	if (ret < 0) {
		LOG_ERR("PWM set failed: %d", ret);
		return ret;
	}

	ret = pwm_set_dt(&config->vcom, config->vcom.period, config->vcom.period / 2);
	if (ret < 0) {
		LOG_ERR("PWM set failed: %d", ret);
		return ret;
	}

	return 0;
}

static int ls014b7dd01_write(const struct device *dev, const uint16_t x, const uint16_t y,
			     const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct ls014b7dd01_config *config = dev->config;
	struct jdi_msg msg;
	int ret;

	msg.x = x;
	msg.y = y;
	msg.w = desc->width;
	msg.h = desc->height;
	msg.tx_buf = (void *)buf;
	msg.tx_len = desc->buf_size;

	ret = jdi_transfer(config->jdi, &msg);
	if (ret < 0) {
		LOG_ERR("Failed to transfer: %d", ret);
		return ret;
	}

	return 0;
}

static void ls014b7dd01_get_capabilities(const struct device *dev,
					 struct display_capabilities *capabilities)
{
	const struct ls014b7dd01_config *config = dev->config;

	memset(capabilities, 0, sizeof(struct display_capabilities));
	capabilities->x_resolution = config->device.width;
	capabilities->y_resolution = config->device.height;
	capabilities->supported_pixel_formats = PIXEL_FORMAT_RGB_888 | PIXEL_FORMAT_ARGB_8888 |
						PIXEL_FORMAT_RGB_565 | PIXEL_FORMAT_BGR_565 |
						PIXEL_FORMAT_L_8 | PIXEL_FORMAT_AL_88;

	if (config->device.input_pixfmt == JDI_PIXFMT_RGB888) {
		capabilities->current_pixel_format = PIXEL_FORMAT_RGB_888;
	} else if (config->device.input_pixfmt == JDI_PIXFMT_ARGB8888) {
		capabilities->current_pixel_format = PIXEL_FORMAT_ARGB_8888;
	} else if (config->device.input_pixfmt == JDI_PIXFMT_RGB565) {
		capabilities->current_pixel_format = PIXEL_FORMAT_RGB_565;
	} else if (config->device.input_pixfmt == JDI_PIXFMT_BGR565) {
		capabilities->current_pixel_format = PIXEL_FORMAT_BGR_565;
	} else if (config->device.input_pixfmt == JDI_PIXFMT_L8) {
		capabilities->current_pixel_format = PIXEL_FORMAT_L_8;
	} else if (config->device.input_pixfmt == JDI_PIXFMT_AL88) {
		capabilities->current_pixel_format = PIXEL_FORMAT_AL_88;
	}

	capabilities->current_orientation = config->rotation;
	capabilities->screen_info = SCREEN_INFO_X_ALIGNMENT_WIDTH | SCREEN_INFO_MONO_VTILED;

	/* Enable PWM */
	ls014b7dd01_blanking_off(dev);
}

static int ls014b7dd01_set_orientation(const struct device *dev,
				       const enum display_orientation orientation)
{
	if (orientation != DISPLAY_ORIENTATION_NORMAL &&
	    orientation != DISPLAY_ORIENTATION_ROTATED_180) {
		LOG_ERR("Unsupported orientation");
		return -ENOTSUP;
	}

	return 0;
}

/* Display Driver API Structure */
static DEVICE_API(display, ls014b7dd01_api) = {
	.blanking_on = ls014b7dd01_blanking_on,
	.blanking_off = ls014b7dd01_blanking_off,
	.write = ls014b7dd01_write,
	.get_capabilities = ls014b7dd01_get_capabilities,
	.set_orientation = ls014b7dd01_set_orientation,
};

static int ls014b7dd01_init(const struct device *dev)
{
	const struct ls014b7dd01_config *config = dev->config;
	int ret;

	if (!device_is_ready(config->va.dev)) {
		LOG_ERR("PWM VA device not ready\n");
		return -ENODEV;
	}

	if (!device_is_ready(config->vcom.dev)) {
		LOG_ERR("PWM VCOM device not ready\n");
		return -ENODEV;
	}

	/* Attach to JDI */
	ret = jdi_attach(config->jdi, &config->device);
	if (ret < 0) {
		LOG_ERR("Failed to attach to JDI Host: %d", ret);
		return ret;
	}

	return 0;
}

/* Device Definition Macros */
#define LS014B7DD01_DEVICE(n)                                                                      \
	static struct ls014b7dd01_data ls014b7dd01_data_##n;                                       \
	static const struct ls014b7dd01_config ls014b7dd01_config_##n = {                          \
		.jdi = DEVICE_DT_GET(DT_INST_BUS(n)),                                              \
		.va = PWM_DT_SPEC_GET_BY_IDX(DT_DRV_INST(n), 0),                                   \
		.vcom = PWM_DT_SPEC_GET_BY_IDX(DT_DRV_INST(n), 1),                                 \
		.rotation = DT_INST_PROP_OR(n, rotation, 0),                                       \
		.device =                                                                          \
			{                                                                          \
				.input_pixfmt =                                                    \
					DT_INST_PROP_OR(n, input_pixel_format, JDI_PIXFMT_RGB565), \
				.width = DT_INST_PROP_OR(n, width, 280),                           \
				.height = DT_INST_PROP_OR(n, height, 280),                         \
				.mode_flags = DT_INST_PROP_OR(n, mode_flags, 0),                   \
			},                                                                         \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, ls014b7dd01_init, NULL, &ls014b7dd01_data_##n,                    \
			      &ls014b7dd01_config_##n, POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY,  \
			      &ls014b7dd01_api);

DT_INST_FOREACH_STATUS_OKAY(LS014B7DD01_DEVICE)
