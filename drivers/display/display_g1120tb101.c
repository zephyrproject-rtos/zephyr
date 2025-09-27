/*
 * Copyright (c) 2025 STMicroelectronics
 * Inspired from display_otm8009a.c
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gvo_g1120tb101

#include <zephyr/kernel.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(g1120tb101, CONFIG_DISPLAY_LOG_LEVEL);

struct g1120tb101_config {
	const struct device *mipi_dsi;
	const struct gpio_dt_spec reset;
	uint8_t data_lanes;
	uint16_t width;
	uint16_t height;
	uint8_t channel;
	uint8_t dsi_pixel_format;
};

struct g1120tb101_cmd_data {
	uint8_t cmd;
	uint8_t param;
};

static const struct g1120tb101_cmd_data g1120tb101_init_data[] = {
	/* Go to command 2 */
	{ 0xfe, 0x01 },
	/* IC Frame rate control, set power, sw mapping, mux switch timing command */
	{ 0x06, 0x62 }, { 0x0e, 0x80 }, { 0x0f, 0x80 }, { 0x10, 0x71 }, { 0x13, 0x81 },
	{ 0x14, 0x81 }, { 0x15, 0x82 }, { 0x16, 0x82 }, { 0x18, 0x88 }, { 0x19, 0x55 },
	{ 0x1a, 0x10 }, { 0x1c, 0x99 }, { 0x1d, 0x03 }, { 0x1e, 0x03 }, { 0x1f, 0x03 },
	{ 0x20, 0x03 }, { 0x25, 0x03 }, { 0x26, 0x8d }, { 0x2a, 0x03 }, { 0x2b, 0x8d },
	{ 0x36, 0x00 }, { 0x37, 0x10 }, { 0x3a, 0x00 }, { 0x3b, 0x00 }, { 0x3d, 0x20 },
	{ 0x3f, 0x3a }, { 0x40, 0x30 }, { 0x41, 0x1a }, { 0x42, 0x33 }, { 0x43, 0x22 },
	{ 0x44, 0x11 }, { 0x45, 0x66 }, { 0x46, 0x55 }, { 0x47, 0x44 }, { 0x4c, 0x33 },
	{ 0x4d, 0x22 }, { 0x4e, 0x11 }, { 0x4f, 0x66 }, { 0x50, 0x55 }, { 0x51, 0x44 },
	{ 0x57, 0x33 }, { 0x6b, 0x1b }, { 0x70, 0x55 }, { 0x74, 0x0c },
	/* Go to command 3 */
	{ 0xfe, 0x02 },
	/* Set the VGMP/VGSP voltage control */
	{ 0x9b, 0x40 }, { 0x9c, 0x00 }, { 0x9d, 0x20 },
	/* Go to command 4 */
	{ 0xfe, 0x03 },
	/* Set the VGMP/VGSP voltage control */
	{ 0x9b, 0x40 }, { 0x9c, 0x00 }, { 0x9d, 0x20 },
	/* Go to command 5 */
	{ 0xfe, 0x04 },
	/* VSR command */
	{ 0x5d, 0x10 },
	/* VSR1 timing set */
	{ 0x00, 0x8d }, { 0x01, 0x00 }, { 0x02, 0x01 }, { 0x03, 0x01 }, { 0x04, 0x10 },
	{ 0x05, 0x01 }, { 0x06, 0xa7 }, { 0x07, 0x20 }, { 0x08, 0x00 },
	/* VSR2 timing set */
	{ 0x09, 0xc2 }, { 0x0a, 0x00 }, { 0x0b, 0x02 }, { 0x0c, 0x01 }, { 0x0d, 0x40 },
	{ 0x0e, 0x06 }, { 0x0f, 0x01 }, { 0x10, 0xa7 }, { 0x11, 0x00 },
	/* VSR3 timing set */
	{ 0x12, 0xc2 }, { 0x13, 0x00 }, { 0x14, 0x02 }, { 0x15, 0x01 }, { 0x16, 0x40 },
	{ 0x17, 0x07 }, { 0x18, 0x01 }, { 0x19, 0xa7 }, { 0x1a, 0x00 },
	/* VSR4 timing set */
	{ 0x1B, 0x82 }, { 0x1C, 0x00 }, { 0x1D, 0xFF }, { 0x1E, 0x05 }, { 0x1F, 0x60 },
	{ 0x20, 0x02 }, { 0x21, 0x01 }, { 0x22, 0x7C }, { 0x23, 0x00 },
	/* VSR5 timing set */
	{ 0x24, 0xC2 }, { 0x25, 0x00 }, { 0x26, 0x04 }, { 0x27, 0x02 }, { 0x28, 0x70 },
	{ 0x29, 0x05 }, { 0x2A, 0x74 }, { 0x2B, 0x8D }, { 0x2D, 0x00 },
	/* VSR6 timing set */
	{ 0x2F, 0xC2 }, { 0x30, 0x00 }, { 0x31, 0x04 }, { 0x32, 0x02 }, { 0x33, 0x70 },
	{ 0x34, 0x07 }, { 0x35, 0x74 }, { 0x36, 0x8D }, { 0x37, 0x00 },
	/* VSR marping command */
	{ 0x5E, 0x20 }, { 0x5F, 0x31 }, { 0x60, 0x54 }, { 0x61, 0x76 }, { 0x62, 0x98 },
	/* Go to command 6 */
	{ 0xfe, 0x05 },
	/* Set the ELVSS voltage */
	{ 0x05, 0x17 }, { 0x2A, 0x04 }, { 0x91, 0x00 },
	/* Go back in standard commands */
	{ 0xfe, 0x00 },
	/* Set the Pixel format */
	{ MIPI_DCS_SET_PIXEL_FORMAT, 0x07},
	/* Set tear off */
	{ MIPI_DCS_SET_TEAR_OFF, 0x00 },
	/* Set DSI mode to internal timing added vs ORIGINAL for Command mode */
	{ 0xc2, 0x00 },
};

static inline int g1120tb101_dcs_write(const struct device *dev, uint8_t cmd, const void *buf,
				       size_t len)
{
	const struct g1120tb101_config *cfg = dev->config;
	int ret;

	ret = mipi_dsi_dcs_write(cfg->mipi_dsi, cfg->channel, cmd, buf, len);
	if (ret < 0) {
		LOG_ERR("DCS 0x%x write failed! (%d)", cmd, ret);
		return ret;
	}

	return 0;
}

static int g1120tb101_configure(const struct device *dev)
{
	uint8_t InitParam1[4] = {0x00, 0x04, 0x01, 0x89};
	uint8_t InitParam2[4] = {0x00, 0x00, 0x01, 0x85};
	int ret, i;

	/* Configure common commands */
	for (i = 0; i < ARRAY_SIZE(g1120tb101_init_data); i++) {
		ret = g1120tb101_dcs_write(dev, g1120tb101_init_data[i].cmd,
					   &g1120tb101_init_data[i].param, 1);
		if (ret) {
			LOG_ERR("Failed to write cmd:0x%x, param:0x%x (ret:0x%x)",
				g1120tb101_init_data[i].cmd, g1120tb101_init_data[i].param, ret);
			return ret;
		}
	}

	ret = g1120tb101_dcs_write(dev, MIPI_DCS_SET_COLUMN_ADDRESS, InitParam1, 4);
	if (ret) {
		LOG_ERR("Failed to write COLUMN_ADDRESS (ret:0x%x)", ret);
		return ret;
	}

	ret = g1120tb101_dcs_write(dev, MIPI_DCS_SET_PAGE_ADDRESS, InitParam2, 4);
	if (ret) {
		LOG_ERR("Failed to write PAGE_ADDRESS (ret:0x%x)", ret);
		return ret;
	}

	ret = g1120tb101_dcs_write(dev, MIPI_DCS_EXIT_SLEEP_MODE, NULL, 0);
	if (ret) {
		LOG_ERR("Failed to write EXIT_SLEEP_MODE (ret:0x%x)", ret);
		return ret;
	}

	k_msleep(120);

	return 0;
}

static int g1120tb101_blanking_on(const struct device *dev)
{
	return g1120tb101_dcs_write(dev, MIPI_DCS_SET_DISPLAY_OFF, NULL, 0);
}

static int g1120tb101_blanking_off(const struct device *dev)
{
	return g1120tb101_dcs_write(dev, MIPI_DCS_SET_DISPLAY_ON, NULL, 0);
}

static int g1120tb101_write(const struct device *dev, uint16_t x, uint16_t y,
			    const struct display_buffer_descriptor *desc, const void *buf)
{
	return -ENOTSUP;
}

static int g1120tb101_set_brightness(const struct device *dev, uint8_t brightness)
{
	return g1120tb101_dcs_write(dev, MIPI_DCS_SET_DISPLAY_BRIGHTNESS, &brightness, 1);
}

static void g1120tb101_get_capabilities(const struct device *dev,
					struct display_capabilities *capabilities)
{
	const struct g1120tb101_config *cfg = dev->config;

	memset(capabilities, 0, sizeof(struct display_capabilities));
	capabilities->x_resolution = cfg->width;
	capabilities->y_resolution = cfg->height;
	capabilities->supported_pixel_formats = PIXEL_FORMAT_RGB_888;
	capabilities->current_pixel_format = PIXEL_FORMAT_RGB_888;
	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static const struct display_driver_api g1120tb101_api = {
	.blanking_on = g1120tb101_blanking_on,
	.blanking_off = g1120tb101_blanking_off,
	.write = g1120tb101_write,
	.set_brightness = g1120tb101_set_brightness,
	.get_capabilities = g1120tb101_get_capabilities,
};

static int g1120tb101_init(const struct device *dev)
{
	const struct g1120tb101_config *cfg = dev->config;
	struct mipi_dsi_device mdev = { 0 };
	int ret;

	if (cfg->dsi_pixel_format != MIPI_DSI_PIXFMT_RGB888) {
		LOG_ERR("Unsupported pixel-format 0x%x\n", cfg->dsi_pixel_format);
		return -EINVAL;
	}

	if (cfg->data_lanes != 1) {
		LOG_ERR("Only MIPI 1 lane is supported\n");
		return -EINVAL;
	}

	if (cfg->reset.port) {
		ret = gpio_pin_configure_dt(&cfg->reset, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Reset configure failed! (%d)", ret);
			return ret;
		}

		k_msleep(100);

		ret = gpio_pin_set_dt(&cfg->reset, 0);
		if (ret < 0) {
			LOG_ERR("Disable reset failed! (%d)", ret);
			return ret;
		}

		k_msleep(120);
	}

	/* attach to MIPI-DSI host */
	mdev.data_lanes = cfg->data_lanes;
	mdev.pixfmt = cfg->dsi_pixel_format;
	mdev.mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST | MIPI_DSI_MODE_LPM;

	mdev.timings.hactive = cfg->width;
	mdev.timings.hbp = 1;
	mdev.timings.hfp = 1;
	mdev.timings.hsync = 1;
	mdev.timings.vactive = cfg->height;
	mdev.timings.vbp = 1;
	mdev.timings.vfp = 1;
	mdev.timings.vsync = 1;

	ret = mipi_dsi_attach(cfg->mipi_dsi, cfg->channel, &mdev);
	if (ret < 0) {
		LOG_ERR("MIPI-DSI attach failed! (%d)", ret);
		return ret;
	}

	ret = g1120tb101_configure(dev);
	if (ret) {
		LOG_ERR("DSI init sequence failed! (%d)", ret);
		return ret;
	}

	ret = g1120tb101_blanking_off(dev);
	if (ret) {
		LOG_ERR("Display blanking off failed! (%d)", ret);
		return ret;
	}

	return 0;
}

#define G1120TB101_DEVICE(inst)									\
	static const struct g1120tb101_config g1120tb101_config_##inst = {			\
		.mipi_dsi = DEVICE_DT_GET(DT_INST_BUS(inst)),					\
		.reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),			\
		.data_lanes = DT_INST_PROP_BY_IDX(inst, data_lanes, 0),				\
		.width = DT_INST_PROP(inst, width),						\
		.height = DT_INST_PROP(inst, height),						\
		.channel = DT_INST_REG_ADDR(inst),						\
		.dsi_pixel_format = DT_INST_PROP(inst, pixel_format),				\
	};											\
	DEVICE_DT_INST_DEFINE(inst, &g1120tb101_init, NULL, NULL,				\
			      &g1120tb101_config_##inst, POST_KERNEL,				\
			      CONFIG_DISPLAY_G1120TB101_INIT_PRIORITY, &g1120tb101_api);	\

DT_INST_FOREACH_STATUS_OKAY(G1120TB101_DEVICE)
