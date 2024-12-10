/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ilitek_ili9806e_dsi

#include <zephyr/kernel.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include "display_ili9806e_dsi.h"
LOG_MODULE_REGISTER(display_ili9806e_dsi, CONFIG_DISPLAY_LOG_LEVEL);

#define ILITEK_ILI9806E_COLMOD_RGB565 0x50
#define ILITEK_ILI9806E_COLMOD_RGB888 0x70
struct ili9806e_config {
	const struct device *mipi_dsi;
	const struct gpio_dt_spec reset;
	const struct gpio_dt_spec backlight;
	enum display_pixel_format pixel_format;
	uint8_t data_lanes;
	uint16_t width;
	uint16_t height;
	uint8_t channel;
};

struct ili9806e_init_cmd {
	uint8_t reg;
	uint8_t cmd_len;
	uint8_t cmd[5];
} __packed;

static const struct ili9806e_init_cmd init_cmds[] = {
	/* Change to Page 1 CMD */
	{.reg = 0xff, .cmd_len = 5, .cmd = {0xFF, 0x98, 0x06, 0x04, 0x01}},
	/* Output SDA */
	{.reg = 0x08, .cmd_len = 1, .cmd = {0x10}},
	/* DE = 1 Active */
	{.reg = 0x21, .cmd_len = 1, .cmd = {0x01}},
	/* Resolution setting 480 X 800 */
	{.reg = 0x30, .cmd_len = 1, .cmd = {0x01}},
	/* Inversion setting */
	{.reg = 0x31, .cmd_len = 1, .cmd = {0x00}},
	/* BT 15 */
	{.reg = 0x40, .cmd_len = 1, .cmd = {0x14}},
	/* avdd +5.2v,avee-5.2v */
	{.reg = 0x41, .cmd_len = 1, .cmd = {0x33}},
	/* VGL=DDVDL+VCL-VCIP,VGH=2DDVDH-DDVDL */
	{.reg = 0x42, .cmd_len = 1, .cmd = {0x02}},
	/* Set VGH clamp level */
	{.reg = 0x43, .cmd_len = 1, .cmd = {0x09}},
	/* Set VGL clamp level */
	{.reg = 0x44, .cmd_len = 1, .cmd = {0x06}},
	/* Set VREG1 */
	{.reg = 0x50, .cmd_len = 1, .cmd = {0x70}},
	/* Set VREG2 */
	{.reg = 0x51, .cmd_len = 1, .cmd = {0x70}},
	/* Flicker MSB */
	{.reg = 0x52, .cmd_len = 1, .cmd = {0x00}},
	/* Flicker LSB */
	{.reg = 0x53, .cmd_len = 1, .cmd = {0x48}},
	/* Timing Adjust */
	{.reg = 0x60, .cmd_len = 1, .cmd = {0x07}},
	{.reg = 0x61, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x62, .cmd_len = 1, .cmd = {0x08}},
	{.reg = 0x63, .cmd_len = 1, .cmd = {0x00}},
	/* Positive Gamma Control 1 */
	{.reg = 0xa0, .cmd_len = 1, .cmd = {0x00}},
	/* Positive Gamma Control 2 */
	{.reg = 0xa1, .cmd_len = 1, .cmd = {0x03}},
	/* Positive Gamma Control 3 */
	{.reg = 0xa2, .cmd_len = 1, .cmd = {0x09}},
	/* Positive Gamma Control 4 */
	{.reg = 0xa3, .cmd_len = 1, .cmd = {0x0d}},
	/* Positive Gamma Control 5 */
	{.reg = 0xa4, .cmd_len = 1, .cmd = {0x06}},
	/* Positive Gamma Control 6 */
	{.reg = 0xa5, .cmd_len = 1, .cmd = {0x16}},
	/* Positive Gamma Control 7 */
	{.reg = 0xa6, .cmd_len = 1, .cmd = {0x09}},
	/* Positive Gamma Control 8 */
	{.reg = 0xa7, .cmd_len = 1, .cmd = {0x08}},
	/* Positive Gamma Control 9 */
	{.reg = 0xa8, .cmd_len = 1, .cmd = {0x03}},
	/* Positive Gamma Control 10 */
	{.reg = 0xa9, .cmd_len = 1, .cmd = {0x07}},
	/* Positive Gamma Control 11 */
	{.reg = 0xaa, .cmd_len = 1, .cmd = {0x06}},
	/* Positive Gamma Control 12 */
	{.reg = 0xab, .cmd_len = 1, .cmd = {0x05}},
	/* Positive Gamma Control 13 */
	{.reg = 0xac, .cmd_len = 1, .cmd = {0x0d}},
	/* Positive Gamma Control 14 */
	{.reg = 0xad, .cmd_len = 1, .cmd = {0x2c}},
	/* Positive Gamma Control 15 */
	{.reg = 0xae, .cmd_len = 1, .cmd = {0x26}},
	/* Positive Gamma Control 16 */
	{.reg = 0xaf, .cmd_len = 1, .cmd = {0x00}},
	/* Negative Gamma Correction 1 */
	{.reg = 0xc0, .cmd_len = 1, .cmd = {0x00}},
	/* Negative Gamma Correction 2 */
	{.reg = 0xc1, .cmd_len = 1, .cmd = {0x04}},
	/* Negative Gamma Correction 3 */
	{.reg = 0xc2, .cmd_len = 1, .cmd = {0x0b}},
	/* Negative Gamma Correction 4 */
	{.reg = 0xc3, .cmd_len = 1, .cmd = {0x0f}},
	/* Negative Gamma Correction 5 */
	{.reg = 0xc4, .cmd_len = 1, .cmd = {0x09}},
	/* Negative Gamma Correction 6 */
	{.reg = 0xc5, .cmd_len = 1, .cmd = {0x18}},
	/* Negative Gamma Correction 7 */
	{.reg = 0xc6, .cmd_len = 1, .cmd = {0x07}},
	/* Negative Gamma Correction 8 */
	{.reg = 0xc7, .cmd_len = 1, .cmd = {0x08}},
	/* Negative Gamma Correction 9 */
	{.reg = 0xc8, .cmd_len = 1, .cmd = {0x05}},
	/* Negative Gamma Correction 10 */
	{.reg = 0xc9, .cmd_len = 1, .cmd = {0x09}},
	/* Negative Gamma Correction 11 */
	{.reg = 0xca, .cmd_len = 1, .cmd = {0x07}},
	/* Negative Gamma Correction 12 */
	{.reg = 0xcb, .cmd_len = 1, .cmd = {0x05}},
	/* Negative Gamma Correction 13 */
	{.reg = 0xcc, .cmd_len = 1, .cmd = {0x0c}},
	/* Negative Gamma Correction 14 */
	{.reg = 0xcd, .cmd_len = 1, .cmd = {0x2d}},
	/* Negative Gamma Correction 15 */
	{.reg = 0xce, .cmd_len = 1, .cmd = {0x28}},
	/* Negative Gamma Correction 16 */
	{.reg = 0xcf, .cmd_len = 1, .cmd = {0x00}},

	/* Change to Page 6 CMD for GIP timing */
	{.reg = 0xff, .cmd_len = 5, .cmd = {0xFF, 0x98, 0x06, 0x04, 0x06}},
	/* GIP Control 1 */
	{.reg = 0x00, .cmd_len = 1, .cmd = {0x21}},
	{.reg = 0x01, .cmd_len = 1, .cmd = {0x09}},
	{.reg = 0x02, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x03, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x04, .cmd_len = 1, .cmd = {0x01}},
	{.reg = 0x05, .cmd_len = 1, .cmd = {0x01}},
	{.reg = 0x06, .cmd_len = 1, .cmd = {0x80}},
	{.reg = 0x07, .cmd_len = 1, .cmd = {0x05}},
	{.reg = 0x08, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x09, .cmd_len = 1, .cmd = {0x80}},
	{.reg = 0x0a, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x0b, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x0c, .cmd_len = 1, .cmd = {0x0a}},
	{.reg = 0x0d, .cmd_len = 1, .cmd = {0x0a}},
	{.reg = 0x0e, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x0f, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x10, .cmd_len = 1, .cmd = {0xe0}},
	{.reg = 0x11, .cmd_len = 1, .cmd = {0xe4}},
	{.reg = 0x12, .cmd_len = 1, .cmd = {0x04}},
	{.reg = 0x13, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x14, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x15, .cmd_len = 1, .cmd = {0xc0}},
	{.reg = 0x16, .cmd_len = 1, .cmd = {0x08}},
	{.reg = 0x17, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x18, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x19, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x1a, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x1b, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x1c, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x1d, .cmd_len = 1, .cmd = {0x00}},
	/* GIP Control 2 */
	{.reg = 0x20, .cmd_len = 1, .cmd = {0x01}},
	{.reg = 0x21, .cmd_len = 1, .cmd = {0x23}},
	{.reg = 0x22, .cmd_len = 1, .cmd = {0x45}},
	{.reg = 0x23, .cmd_len = 1, .cmd = {0x67}},
	{.reg = 0x24, .cmd_len = 1, .cmd = {0x01}},
	{.reg = 0x25, .cmd_len = 1, .cmd = {0x23}},
	{.reg = 0x26, .cmd_len = 1, .cmd = {0x45}},
	{.reg = 0x27, .cmd_len = 1, .cmd = {0x67}},
	/* GIP Control 3 */
	{.reg = 0x30, .cmd_len = 1, .cmd = {0x01}},
	{.reg = 0x31, .cmd_len = 1, .cmd = {0x11}},
	{.reg = 0x32, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x33, .cmd_len = 1, .cmd = {0xee}},
	{.reg = 0x34, .cmd_len = 1, .cmd = {0xff}},
	{.reg = 0x35, .cmd_len = 1, .cmd = {0xcb}},
	{.reg = 0x36, .cmd_len = 1, .cmd = {0xda}},
	{.reg = 0x37, .cmd_len = 1, .cmd = {0xad}},
	{.reg = 0x38, .cmd_len = 1, .cmd = {0xbc}},
	{.reg = 0x39, .cmd_len = 1, .cmd = {0x76}},
	{.reg = 0x3a, .cmd_len = 1, .cmd = {0x67}},
	{.reg = 0x3b, .cmd_len = 1, .cmd = {0x22}},
	{.reg = 0x3c, .cmd_len = 1, .cmd = {0x22}},
	{.reg = 0x3d, .cmd_len = 1, .cmd = {0x22}},
	{.reg = 0x3e, .cmd_len = 1, .cmd = {0x22}},
	{.reg = 0x3f, .cmd_len = 1, .cmd = {0x22}},
	{.reg = 0x40, .cmd_len = 1, .cmd = {0x22}},
	/* GOUT VGLO Control */
	{.reg = 0x53, .cmd_len = 1, .cmd = {0x10}},
	{.reg = 0x54, .cmd_len = 1, .cmd = {0x10}},
	/* Change to Page 7 CMD for Normal command */
	{.reg = 0xff, .cmd_len = 5, .cmd = {0xff, 0x98, 0x06, 0x04, 0x07}},
	/* VREG1/2OUT ENABLE */
	{.reg = 0x18, .cmd_len = 1, .cmd = {0x1d}},
	{.reg = 0x26, .cmd_len = 1, .cmd = {0xb2}},
	{.reg = 0x02, .cmd_len = 1, .cmd = {0x77}},
	{.reg = 0xe1, .cmd_len = 1, .cmd = {0x79}},
	{.reg = 0x17, .cmd_len = 1, .cmd = {0x22}},
	/* Change to Page 0 CMD for Normal command */
	{.reg = 0xff, .cmd_len = 5, .cmd = {0xff, 0x98, 0x06, 0x04, 0x00}}};

static int ili9806e_write_reg(const struct device *dev, uint8_t reg, const uint8_t *buf, size_t len)
{
	int ret;
	const struct ili9806e_config *cfg = dev->config;

	ret = mipi_dsi_dcs_write(cfg->mipi_dsi, cfg->channel, reg, buf, len);
	if (ret < 0) {
		LOG_ERR("Failed writing reg: 0x%x result: (%d)", reg, ret);
		return ret;
	}

	return 0;
}

static int ili9806e_write_reg_val(const struct device *dev, uint8_t reg, uint8_t value)
{
	return ili9806e_write_reg(dev, reg, &value, 1);
}

static int ili9806e_write_sequence(const struct device *dev, const struct ili9806e_init_cmd *cmd,
				   uint8_t nr_cmds)
{
	int ret = 0;

	/* Loop through all commands as long as writes are successful */
	for (int i = 0; i < nr_cmds && ret == 0; i++) {
		ret = ili9806e_write_reg(dev, cmd->reg, cmd->cmd, cmd->cmd_len);
		if (ret < 0) {
			LOG_ERR("Failed writing sequence: 0x%x result: (%d)", cmd->reg, ret);
			return ret;
		}
		cmd++;
	}

	return ret;
}

static int ili9806e_config(const struct device *dev)
{
	const struct ili9806e_config *cfg = dev->config;
	int ret;

	ret = ili9806e_write_sequence(dev, init_cmds, ARRAY_SIZE(init_cmds));
	if (ret < 0) {
		return ret;
	}
	/* Add a delay, otherwise MADCTL not taken */
	k_msleep(120);

	/* Exit sleep mode */
	ret = ili9806e_write_reg(dev, MIPI_DCS_EXIT_SLEEP_MODE, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	/* Wait for sleep out exit */
	k_msleep(5);

	/* Set color mode */
	ret = ili9806e_write_reg_val(dev, MIPI_DCS_SET_PIXEL_FORMAT,
				     cfg->pixel_format == PIXEL_FORMAT_RGB_565
					     ? ILITEK_ILI9806E_COLMOD_RGB565
					     : ILITEK_ILI9806E_COLMOD_RGB888);
	if (ret < 0) {
		return ret;
	}

	/* Turn on display */
	ret = ili9806e_write_reg(dev, MIPI_DCS_SET_DISPLAY_ON, NULL, 0);
	return ret;
}

static int ili9806e_blanking_on(const struct device *dev)
{
	const struct ili9806e_config *cfg = dev->config;
	int ret;

	if (cfg->backlight.port != NULL) {
		ret = gpio_pin_set_dt(&cfg->backlight, 0);
		if (ret) {
			LOG_ERR("Disable backlight failed! (%d)", ret);
			return ret;
		}
	}

	return ili9806e_write_reg(dev, MIPI_DCS_SET_DISPLAY_OFF, NULL, 0);
}

static int ili9806e_blanking_off(const struct device *dev)
{
	const struct ili9806e_config *cfg = dev->config;
	int ret;

	if (cfg->backlight.port != NULL) {
		ret = gpio_pin_set_dt(&cfg->backlight, 1);
		if (ret) {
			LOG_ERR("Enable backlight failed! (%d)", ret);
			return ret;
		}
	}

	return ili9806e_write_reg(dev, MIPI_DCS_SET_DISPLAY_ON, NULL, 0);
}

static void ili9806e_get_capabilities(const struct device *dev,
				      struct display_capabilities *capabilities)
{
	const struct ili9806e_config *cfg = dev->config;

	memset(capabilities, 0, sizeof(struct display_capabilities));
	capabilities->x_resolution = cfg->width;
	capabilities->y_resolution = cfg->height;
	capabilities->supported_pixel_formats = cfg->pixel_format;
	capabilities->current_pixel_format = cfg->pixel_format;
}

static int ili9806e_pixel_format(const struct device *dev,
				 const enum display_pixel_format pixel_format)
{
	const struct ili9806e_config *config = dev->config;

	LOG_WRN("Pixel format change not implemented");
	if (pixel_format == config->pixel_format) {
		return 0;
	}

	return -ENOTSUP;
}

static DEVICE_API(display, ili9806e_api) = {
	.blanking_on = ili9806e_blanking_on,
	.blanking_off = ili9806e_blanking_off,
	.set_pixel_format = ili9806e_pixel_format,
	.get_capabilities = ili9806e_get_capabilities,
};

static int ili9806e_init(const struct device *dev)
{
	const struct ili9806e_config *cfg = dev->config;
	struct mipi_dsi_device mdev;
	int ret;

	if (cfg->reset.port) {
		if (!gpio_is_ready_dt(&cfg->reset)) {
			LOG_ERR("Reset GPIO device is not ready!");
			return -ENODEV;
		}
		k_sleep(K_MSEC(1));

		ret = gpio_pin_configure_dt(&cfg->reset, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Reset display failed! (%d)", ret);
			return ret;
		}

		ret = gpio_pin_set_dt(&cfg->reset, 0);
		if (ret < 0) {
			LOG_ERR("Reset display failed! (%d)", ret);
			return ret;
		}
		k_sleep(K_MSEC(1));

		ret = gpio_pin_set_dt(&cfg->reset, 1);
		if (ret < 0) {
			LOG_ERR("Enable display failed! (%d)", ret);
			return ret;
		}
		k_sleep(K_MSEC(50));
	}

	/* attach to MIPI-DSI host */
	if (cfg->pixel_format == PIXEL_FORMAT_RGB_565) {
		mdev.pixfmt = MIPI_DSI_PIXFMT_RGB565;
	} else {
		mdev.pixfmt = MIPI_DSI_PIXFMT_RGB888;
	}
	mdev.data_lanes = cfg->data_lanes;
	mdev.mode_flags = MIPI_DSI_MODE_VIDEO;
	mdev.timings.hactive = cfg->width;
	mdev.timings.hbp = ILITEK_ILI9806E_HBP;
	mdev.timings.hfp = ILITEK_ILI9806E_HFP;
	mdev.timings.hsync = ILITEK_ILI9806E_HSYNC;
	mdev.timings.vactive = cfg->height;
	mdev.timings.vbp = ILITEK_ILI9806E_VBP;
	mdev.timings.vfp = ILITEK_ILI9806E_VFP;
	mdev.timings.vsync = ILITEK_ILI9806E_VSYNC;

	ret = mipi_dsi_attach(cfg->mipi_dsi, cfg->channel, &mdev);
	if (ret < 0) {
		LOG_ERR("Could not attach to MIPI-DSI host");
		return ret;
	}

	if (cfg->backlight.port != NULL) {
		ret = gpio_pin_configure_dt(&cfg->backlight, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not configure backlight GPIO (%d)", ret);
			return ret;
		}
	}

	ret = ili9806e_config(dev);
	if (ret) {
		LOG_ERR("DSI init sequence failed! (%d)", ret);
		return ret;
	}

	return 0;
}

#define ILITEK_ILI9806E_DEFINE(n)                                                                  \
	static const struct ili9806e_config ili9806e_config_##n = {                                \
		.mipi_dsi = DEVICE_DT_GET(DT_INST_BUS(n)),                                         \
		.reset = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {0}),                            \
		.backlight = GPIO_DT_SPEC_INST_GET_OR(n, bl_gpios, {0}),                           \
		.data_lanes = DT_INST_PROP_BY_IDX(n, data_lanes, 0),                               \
		.width = DT_INST_PROP(n, width),                                                   \
		.height = DT_INST_PROP(n, height),                                                 \
		.channel = DT_INST_REG_ADDR(n),                                                    \
		.pixel_format = DT_INST_PROP(n, pixel_format),                                     \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, &ili9806e_init, NULL, NULL, &ili9806e_config_##n, POST_KERNEL,    \
			      CONFIG_DISPLAY_ILI9806E_DSI_INIT_PRIORITY, &ili9806e_api);

DT_INST_FOREACH_STATUS_OKAY(ILITEK_ILI9806E_DEFINE)
