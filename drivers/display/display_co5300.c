/*
 * Copyright (c) 2025 Ambiq Micro Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT chipone_co5300

#include <zephyr/kernel.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(co5300, CONFIG_DISPLAY_LOG_LEVEL);

struct co5300_config {
	const struct device *mipi_dsi;
	const struct gpio_dt_spec reset;
	const struct mipi_dsi_device device;
	uint8_t channel;
	uint32_t rotation;
};
struct co5300_data {
	uint16_t xstart;
	uint16_t ystart;
	uint16_t width;
	uint16_t height;
	enum display_pixel_format pixel_format;
	enum display_orientation orientation;
};

static inline int co5300_dcs_write(const struct device *dev, uint8_t cmd, const void *buf,
				   size_t len)
{
	const struct co5300_config *cfg = dev->config;
	int ret;

	ret = mipi_dsi_dcs_write(cfg->mipi_dsi, cfg->channel, cmd, buf, len);
	if (ret < 0) {
		LOG_ERR("DCS 0x%x write failed! (%d)", cmd, ret);
		return ret;
	}

	return 0;
}

static inline int co5300_generic_write(const struct device *dev, const void *buf, size_t len)
{
	const struct co5300_config *cfg = dev->config;
	int ret;

	ret = mipi_dsi_generic_write(cfg->mipi_dsi, cfg->channel, buf, len);
	if (ret < 0) {
		LOG_ERR("Generic write failed!");
		return ret;
	}

	return 0;
}

static int co5300_blanking_on(const struct device *dev)
{
	co5300_dcs_write(dev, MIPI_DCS_SET_DISPLAY_OFF, NULL, 0);
	return 0;
}

static int co5300_blanking_off(const struct device *dev)
{
	co5300_dcs_write(dev, MIPI_DCS_SET_DISPLAY_ON, NULL, 0);
	return 0;
}

static int co5300_write(const struct device *dev, uint16_t x, uint16_t y,
			const struct display_buffer_descriptor *desc, const void *buf)
{
	struct co5300_data *data = dev->data;
	uint8_t cmd[4];

	if (data->xstart != x || data->width != desc->width) {
		data->xstart = x;
		data->width = desc->width;

		cmd[0] = data->xstart >> 8U;
		cmd[1] = data->xstart & 0xFFU;
		cmd[2] = (data->xstart + data->width - 1) >> 8U;
		cmd[3] = (data->xstart + data->width - 1) & 0xFFU;
		co5300_dcs_write(dev, MIPI_DCS_SET_COLUMN_ADDRESS, cmd, 4);
	}
	if (data->ystart != y || data->height != desc->height) {
		data->ystart = y;
		data->height = desc->height;

		cmd[0] = data->ystart >> 8U;
		cmd[1] = data->ystart & 0xFFU;
		cmd[2] = (data->ystart + data->height - 1) >> 8U;
		cmd[3] = (data->ystart + data->height - 1) & 0xFFU;
		co5300_dcs_write(dev, MIPI_DCS_SET_PAGE_ADDRESS, cmd, 4);
	}

	co5300_dcs_write(dev, MIPI_DCS_WRITE_MEMORY_START, buf, desc->buf_size);
	return 0;
}

static int co5300_set_brightness(const struct device *dev, uint8_t brightness)
{
	uint8_t cmd[4];

	cmd[0] = MIPI_DCS_SET_DISPLAY_BRIGHTNESS;
	cmd[1] = brightness;
	co5300_generic_write(dev, cmd, 2);
	return 0;
}

static void co5300_get_capabilities(const struct device *dev,
				    struct display_capabilities *capabilities)
{
	const struct co5300_config *config = dev->config;

	memset(capabilities, 0, sizeof(struct display_capabilities));
	capabilities->x_resolution = config->device.timings.hactive;
	capabilities->y_resolution = config->device.timings.vactive;
	capabilities->supported_pixel_formats = PIXEL_FORMAT_RGB_888 | PIXEL_FORMAT_RGB_565;
#if DT_INST_NODE_HAS_PROP(0, pixel_format)
#if DT_INST_PROP(0, pixel_format) == PANEL_PIXEL_FORMAT_RGB_888
	capabilities->current_pixel_format = PIXEL_FORMAT_RGB_888;
#elif DT_INST_PROP(0, pixel_format) == PANEL_PIXEL_FORMAT_RGB_565
	capabilities->current_pixel_format = PIXEL_FORMAT_RGB_565;
#endif
#endif
	capabilities->current_orientation = config->rotation;
	capabilities->screen_info = SCREEN_INFO_MONO_VTILED;
}

static DEVICE_API(display, co5300_api) = {
	.blanking_on = co5300_blanking_on,
	.blanking_off = co5300_blanking_off,
	.write = co5300_write,
	.set_brightness = co5300_set_brightness,
	.get_capabilities = co5300_get_capabilities,
};

static int co5300_configure(const struct device *dev)
{
	const struct co5300_config *config = dev->config;
	struct co5300_data *data = dev->data;
	uint8_t cmd[4];
	int ret;

	cmd[0] = MIPI_DCS_SET_DISPLAY_BRIGHTNESS;
	cmd[1] = 0xff;
	ret = co5300_generic_write(dev, cmd, 2);

	if (config->device.pixfmt == MIPI_DSI_PIXFMT_RGB888) {
		cmd[0] = MIPI_DCS_PIXEL_FORMAT_24BIT;
	} else if (config->device.pixfmt == MIPI_DSI_PIXFMT_RGB565) {
		cmd[0] = MIPI_DCS_PIXEL_FORMAT_16BIT;
	}
	ret = co5300_dcs_write(dev, MIPI_DCS_SET_PIXEL_FORMAT, cmd, 1);

	const int MIPI_set_hsifopctr = 0x0A;

	const int MIPI_set_cmd_page = 0xFE;

	cmd[0] = MIPI_set_cmd_page;
	cmd[1] = 0x01;
	ret = co5300_generic_write(dev, cmd, 2);

	cmd[0] = MIPI_set_hsifopctr;
	cmd[1] = 0xF8;
	ret = co5300_generic_write(dev, cmd, 2);

	cmd[0] = MIPI_set_cmd_page;
	cmd[1] = 0x00;
	ret = co5300_generic_write(dev, cmd, 2);

	cmd[0] = 0x00;
	ret = co5300_dcs_write(dev, MIPI_DCS_SET_ADDRESS_MODE, cmd, 1);

	cmd[0] = MIPI_set_cmd_page;
	cmd[1] = 0x20;
	ret = co5300_generic_write(dev, cmd, 2);

	cmd[0] = 0xF4;
	cmd[1] = 0x5A;
	ret = co5300_generic_write(dev, cmd, 2);

	cmd[0] = 0xF5;
	cmd[1] = 0x59;
	ret = co5300_generic_write(dev, cmd, 2);

	cmd[0] = MIPI_set_cmd_page;
	cmd[1] = 0x80;
	ret = co5300_generic_write(dev, cmd, 2);

	cmd[0] = 0x00;
	cmd[1] = 0xF8;
	ret = co5300_generic_write(dev, cmd, 2);

	cmd[0] = MIPI_set_cmd_page;
	cmd[1] = 0x00;
	ret = co5300_generic_write(dev, cmd, 2);

	ret = co5300_dcs_write(dev, MIPI_DCS_EXIT_SLEEP_MODE, cmd, 0);

	ret = co5300_dcs_write(dev, MIPI_DCS_SET_DISPLAY_ON, cmd, 0);

	cmd[0] = 0x02;
	ret = co5300_dcs_write(dev, MIPI_DCS_SET_TEAR_ON, cmd, 1);

	data->xstart = 0;
	data->width = config->device.timings.hactive;

	cmd[0] = data->xstart >> 8U;
	cmd[1] = data->xstart & 0xFFU;
	cmd[2] = (data->xstart + data->width - 1) >> 8U;
	cmd[3] = (data->xstart + data->width - 1) & 0xFFU;
	ret = co5300_dcs_write(dev, MIPI_DCS_SET_COLUMN_ADDRESS, cmd, 4);

	data->ystart = 0;
	data->height = config->device.timings.vactive;

	cmd[0] = data->ystart >> 8U;
	cmd[1] = data->ystart & 0xFFU;
	cmd[2] = (data->ystart + data->height - 1) >> 8U;
	cmd[3] = (data->ystart + data->height - 1) & 0xFFU;
	ret = co5300_dcs_write(dev, MIPI_DCS_SET_PAGE_ADDRESS, cmd, 4);

	return 0;
}

static int co5300_init(const struct device *dev)
{
	const struct co5300_config *cfg = dev->config;
	int ret;

	LOG_DBG("%s", __func__);
	if (cfg->reset.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->reset)) {
			LOG_ERR("Reset GPIO device is not ready!");
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&cfg->reset, GPIO_OUTPUT_HIGH);
		if (ret < 0) {
			LOG_ERR("Could not pull reset high! (%d)", ret);
			return ret;
		}
		k_msleep(5);
		ret = gpio_pin_set_dt(&cfg->reset, 0);
		if (ret < 0) {
			LOG_ERR("Could not pull reset low! (%d)", ret);
			return ret;
		}
		k_msleep(20);
		ret = gpio_pin_set_dt(&cfg->reset, 1);
		if (ret < 0) {
			LOG_ERR("Could not toggle reset pin from low to high! (%d)", ret);
			return ret;
		}
		k_msleep(150);
	}

	/* attach to MIPI-DSI host */

	ret = mipi_dsi_attach(cfg->mipi_dsi, cfg->channel, &cfg->device);
	if (ret < 0) {
		LOG_ERR("MIPI-DSI attach failed! (%d)", ret);
		return ret;
	}

	ret = co5300_configure(dev);
	if (ret) {
		LOG_ERR("DSI init sequence failed! (%d)", ret);
		return ret;
	}

	return 0;
}

/* mapping pixel-format to pixfmt */
#define CO5300_GET_PIXFMT(inst)                                                                    \
	(DT_INST_NODE_HAS_PROP(inst, pixel_format)                                                 \
		 ? (DT_INST_PROP(inst, pixel_format) == PANEL_PIXEL_FORMAT_RGB_888                 \
			    ? MIPI_DSI_PIXFMT_RGB888                                               \
		    : DT_INST_PROP(inst, pixel_format) == PANEL_PIXEL_FORMAT_RGB_565               \
			    ? MIPI_DSI_PIXFMT_RGB565                                               \
			    : MIPI_DSI_PIXFMT_RGB888)                                              \
		 : MIPI_DSI_PIXFMT_RGB888)

#define CO5300_DEVICE(inst)                                                                        \
	static const struct co5300_config co5300_config_##inst = {                                 \
		.mipi_dsi = DEVICE_DT_GET(DT_INST_BUS(inst)),                                      \
		.reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),                         \
		.channel = 0,                                                                      \
		.rotation = DT_INST_PROP_OR(inst, rotation, 0),                                    \
		.device =                                                                          \
			{                                                                          \
				.data_lanes = DT_INST_PROP_BY_IDX(inst, data_lanes, 0),            \
				.pixfmt = CO5300_GET_PIXFMT(inst),                                 \
				.mode_flags =                                                      \
					DT_INST_PROP_OR(inst, mode_flags, MIPI_DSI_MODE_LPM),      \
				.timings =                                                         \
					{                                                          \
						.hactive = DT_INST_PROP_OR(inst, width, 0),        \
						.hfp = DT_INST_PROP_OR(inst, hfp, 1),              \
						.hbp = DT_INST_PROP_OR(inst, hbp, 1),              \
						.hsync = DT_INST_PROP_OR(inst, hsync, 1),          \
						.vactive = DT_INST_PROP_OR(inst, height, 0),       \
						.vfp = DT_INST_PROP_OR(inst, vfp, 1),              \
						.vbp = DT_INST_PROP_OR(inst, vbp, 1),              \
						.vsync = DT_INST_PROP_OR(inst, vsync, 1),          \
					},                                                         \
			},                                                                         \
	};                                                                                         \
	static struct co5300_data co5300_data_##inst;                                              \
	DEVICE_DT_INST_DEFINE(inst, &co5300_init, NULL, &co5300_data_##inst,                       \
			      &co5300_config_##inst, POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY,    \
			      &co5300_api);

DT_INST_FOREACH_STATUS_OKAY(CO5300_DEVICE)
