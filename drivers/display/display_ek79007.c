/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT fitipower_ek79007

#include <zephyr/drivers/display.h>
#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/display/mipi_display.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ek79007, CONFIG_DISPLAY_LOG_LEVEL);

#define EK79007_PAD_CONTROL 0xB2
#define EK79007_DSI_2_LANE  0x10

struct ek79007_init_cmd {
	uint8_t cmd;
	uint8_t data;
	uint8_t len;
	uint16_t delay_ms;
};

static const struct ek79007_init_cmd ek79007_init_cmds[] = {
	{0x80, 0x8B, 1, 0}, {0x81, 0x78, 1, 0},
	{0x82, 0x84, 1, 0}, {0x83, 0x88, 1, 0},
	{0x84, 0xA8, 1, 0}, {0x85, 0xE3, 1, 0},
	{0x86, 0x88, 1, 0}, {MIPI_DCS_EXIT_SLEEP_MODE, 0x00, 0, 120},
};

struct ek79007_config {
	const struct device *mipi_dsi;
	const struct gpio_dt_spec reset_gpio;
	const struct gpio_dt_spec bl_gpio;
	uint8_t num_of_lanes;
	uint8_t pixel_format;
	uint16_t panel_width;
	uint16_t panel_height;
	uint8_t channel;
	uint32_t hbp;
	uint32_t hsync;
	uint32_t hfp;
	uint32_t vbp;
	uint32_t vsync;
	uint32_t vfp;
};

static int ek79007_blanking_off(const struct device *dev)
{
	const struct ek79007_config *config = dev->config;

	if (config->bl_gpio.port != NULL) {
		return gpio_pin_set_dt(&config->bl_gpio, 1);
	}
	return 0;
}

static int ek79007_blanking_on(const struct device *dev)
{
	const struct ek79007_config *config = dev->config;

	if (config->bl_gpio.port != NULL) {
		return gpio_pin_set_dt(&config->bl_gpio, 0);
	}
	return 0;
}

/* The devicetree carries the MIPI DSI pixel format, which the display API
 * reports under its own enumeration.
 */
static enum display_pixel_format ek79007_display_format(uint8_t pixfmt)
{
	switch (pixfmt) {
	case MIPI_DSI_PIXFMT_RGB565:
		return PIXEL_FORMAT_RGB_565;
	case MIPI_DSI_PIXFMT_RGB888:
	default:
		return PIXEL_FORMAT_RGB_888;
	}
}

static void ek79007_get_capabilities(const struct device *dev,
				     struct display_capabilities *capabilities)
{
	const struct ek79007_config *config = dev->config;
	enum display_pixel_format format = ek79007_display_format(config->pixel_format);

	memset(capabilities, 0, sizeof(struct display_capabilities));
	capabilities->x_resolution = config->panel_width;
	capabilities->y_resolution = config->panel_height;
	capabilities->supported_pixel_formats = format;
	capabilities->current_pixel_format = format;
	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static int ek79007_set_pixel_format(const struct device *dev,
				    const enum display_pixel_format pixel_format)
{
	const struct ek79007_config *config = dev->config;

	if (pixel_format == ek79007_display_format(config->pixel_format)) {
		return 0;
	}

	return -ENOTSUP;
}

static int ek79007_set_orientation(const struct device *dev,
				   const enum display_orientation orientation)
{
	if (orientation == DISPLAY_ORIENTATION_NORMAL) {
		return 0;
	}
	return -ENOTSUP;
}

static DEVICE_API(display, ek79007_api) = {
	.blanking_on = ek79007_blanking_on,
	.blanking_off = ek79007_blanking_off,
	.get_capabilities = ek79007_get_capabilities,
	.set_pixel_format = ek79007_set_pixel_format,
	.set_orientation = ek79007_set_orientation,
};

static int ek79007_init(const struct device *dev)
{
	const struct ek79007_config *config = dev->config;
	struct mipi_dsi_device mdev;
	int ret;

	if (!device_is_ready(config->mipi_dsi)) {
		LOG_ERR("MIPI DSI host not ready");
		return -ENODEV;
	}

	if (config->reset_gpio.port != NULL) {
		ret = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure reset GPIO (%d)", ret);
			return ret;
		}
		k_msleep(10);
		gpio_pin_set_dt(&config->reset_gpio, 0);
		k_msleep(50);
	}

	if (config->bl_gpio.port != NULL) {
		ret = gpio_pin_configure_dt(&config->bl_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure backlight GPIO (%d)", ret);
			return ret;
		}
	}

	mdev.pixfmt = config->pixel_format;
	mdev.data_lanes = config->num_of_lanes;
	mdev.mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST;
	mdev.timings.hactive = config->panel_width;
	mdev.timings.hfp = config->hfp;
	mdev.timings.hbp = config->hbp;
	mdev.timings.hsync = config->hsync;
	mdev.timings.vactive = config->panel_height;
	mdev.timings.vfp = config->vfp;
	mdev.timings.vbp = config->vbp;
	mdev.timings.vsync = config->vsync;

	if (config->num_of_lanes != 2) {
		LOG_ERR("Unsupported lane count %u, panel is configured for 2",
			config->num_of_lanes);
		return -ENOTSUP;
	}

	uint8_t lane_cmd = EK79007_DSI_2_LANE;

	ret = mipi_dsi_dcs_write(config->mipi_dsi, config->channel, EK79007_PAD_CONTROL, &lane_cmd,
				 1);
	if (ret < 0) {
		LOG_ERR("Failed to set panel lane configuration (%d)", ret);
		return ret;
	}

	for (size_t i = 0; i < ARRAY_SIZE(ek79007_init_cmds); i++) {
		const struct ek79007_init_cmd *c = &ek79007_init_cmds[i];

		ret = mipi_dsi_dcs_write(config->mipi_dsi, config->channel, c->cmd, &c->data,
					 c->len);
		if (ret < 0) {
			LOG_ERR("Panel init command 0x%02x failed (%d)", c->cmd, ret);
			return ret;
		}
		if (c->delay_ms > 0) {
			k_msleep(c->delay_ms);
		}
	}

	ret = mipi_dsi_dcs_write(config->mipi_dsi, config->channel, MIPI_DCS_SET_DISPLAY_ON, NULL,
				 0);
	if (ret < 0) {
		LOG_ERR("Failed to turn panel display on (%d)", ret);
		return ret;
	}
	k_msleep(50);

	ret = mipi_dsi_attach(config->mipi_dsi, config->channel, &mdev);
	if (ret < 0) {
		LOG_ERR("Failed to attach to MIPI DSI host (%d)", ret);
		return ret;
	}

	LOG_INF("EK79007 panel initialized (%ux%u)", config->panel_width, config->panel_height);

	return 0;
}

#define EK79007_PANEL(id)                                                                          \
	static const struct ek79007_config ek79007_config_##id = {                                 \
		.mipi_dsi = DEVICE_DT_GET(DT_INST_BUS(id)),                                        \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(id, reset_gpios, {0}),                      \
		.bl_gpio = GPIO_DT_SPEC_INST_GET_OR(id, bl_gpios, {0}),                            \
		.num_of_lanes = DT_INST_PROP_BY_IDX(id, data_lanes, 0),                            \
		.pixel_format = DT_INST_PROP(id, pixel_format),                                    \
		.panel_width = DT_INST_PROP(id, width),                                            \
		.panel_height = DT_INST_PROP(id, height),                                          \
		.channel = DT_INST_REG_ADDR(id),                                                   \
		.hbp = DT_PROP(DT_INST_CHILD(id, display_timings), hback_porch),                   \
		.hsync = DT_PROP(DT_INST_CHILD(id, display_timings), hsync_len),                   \
		.hfp = DT_PROP(DT_INST_CHILD(id, display_timings), hfront_porch),                  \
		.vbp = DT_PROP(DT_INST_CHILD(id, display_timings), vback_porch),                   \
		.vsync = DT_PROP(DT_INST_CHILD(id, display_timings), vsync_len),                   \
		.vfp = DT_PROP(DT_INST_CHILD(id, display_timings), vfront_porch),                  \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(id, ek79007_init, NULL, NULL, &ek79007_config_##id, POST_KERNEL,     \
			      CONFIG_DISPLAY_EK79007_INIT_PRIORITY, &ek79007_api);

DT_INST_FOREACH_STATUS_OKAY(EK79007_PANEL)
