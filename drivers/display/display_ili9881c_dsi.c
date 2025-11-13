/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ilitek_ili9881c_dsi

#include <zephyr/kernel.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include "display_ili9881c_dsi.h"
LOG_MODULE_REGISTER(display_ili9881c_dsi, CONFIG_DISPLAY_LOG_LEVEL);

#define ILITEK_ILI9881C_COLMOD_RGB565 0x50
#define ILITEK_ILI9881C_COLMOD_RGB888 0x70
struct ili9881c_config {
	const struct device *mipi_dsi;
	const struct gpio_dt_spec reset;
	const struct gpio_dt_spec backlight;
	enum display_pixel_format pixel_format;
	uint8_t data_lanes;
	uint16_t width;
	uint16_t height;
	uint8_t channel;
};

struct ili9881c_init_cmd {
	uint8_t reg;
	uint8_t cmd_len;
	uint8_t cmd[3];
} __packed;

static const struct ili9881c_init_cmd init_cmds[] = {
	/* Change to Page 3 CMD */
	{.reg = 0xff, .cmd_len = 3, .cmd = {0x98, 0x81, 0x03}},
	/* GIP_1 */
	{.reg = 0x01, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x02, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x03, .cmd_len = 1, .cmd = {0x72}},
	{.reg = 0x04, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x05, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x06, .cmd_len = 1, .cmd = {0x09}},
	{.reg = 0x07, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x08, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x09, .cmd_len = 1, .cmd = {0x01}},
	{.reg = 0x0a, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x0b, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x0c, .cmd_len = 1, .cmd = {0x01}},
	{.reg = 0x0d, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x0e, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x0f, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x10, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x11, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x12, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x13, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x14, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x15, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x16, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x17, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x18, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x19, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x1a, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x1b, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x1c, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x1d, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x1e, .cmd_len = 1, .cmd = {0x40}},
	{.reg = 0x1f, .cmd_len = 1, .cmd = {0x80}},
	{.reg = 0x20, .cmd_len = 1, .cmd = {0x05}},
	{.reg = 0x21, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x22, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x23, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x24, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x25, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x26, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x27, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x28, .cmd_len = 1, .cmd = {0x33}},
	{.reg = 0x29, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x2a, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x2b, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x2c, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x2d, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x2e, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x2f, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x30, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x31, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x32, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x32, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x33, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x34, .cmd_len = 1, .cmd = {0x04}},
	{.reg = 0x35, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x36, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x37, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x38, .cmd_len = 1, .cmd = {0x3C}},
	{.reg = 0x39, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x3a, .cmd_len = 1, .cmd = {0x40}},
	{.reg = 0x3b, .cmd_len = 1, .cmd = {0x40}},
	{.reg = 0x3c, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x3d, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x3e, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x3f, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x40, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x41, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x42, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x43, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x44, .cmd_len = 1, .cmd = {0x00}},
	/* GIP_2 */
	{.reg = 0x50, .cmd_len = 1, .cmd = {0x01}},
	{.reg = 0x51, .cmd_len = 1, .cmd = {0x23}},
	{.reg = 0x52, .cmd_len = 1, .cmd = {0x45}},
	{.reg = 0x53, .cmd_len = 1, .cmd = {0x67}},
	{.reg = 0x54, .cmd_len = 1, .cmd = {0x89}},
	{.reg = 0x55, .cmd_len = 1, .cmd = {0xab}},
	{.reg = 0x56, .cmd_len = 1, .cmd = {0x01}},
	{.reg = 0x57, .cmd_len = 1, .cmd = {0x23}},
	{.reg = 0x58, .cmd_len = 1, .cmd = {0x45}},
	{.reg = 0x59, .cmd_len = 1, .cmd = {0x67}},
	{.reg = 0x5a, .cmd_len = 1, .cmd = {0x89}},
	{.reg = 0x5b, .cmd_len = 1, .cmd = {0xab}},
	{.reg = 0x5c, .cmd_len = 1, .cmd = {0xcd}},
	{.reg = 0x5d, .cmd_len = 1, .cmd = {0xef}},
	/* GIP_3 */
	{.reg = 0x5e, .cmd_len = 1, .cmd = {0x11}},
	{.reg = 0x5f, .cmd_len = 1, .cmd = {0x01}},
	{.reg = 0x60, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x61, .cmd_len = 1, .cmd = {0x15}},
	{.reg = 0x62, .cmd_len = 1, .cmd = {0x14}},
	{.reg = 0x63, .cmd_len = 1, .cmd = {0x0E}},
	{.reg = 0x64, .cmd_len = 1, .cmd = {0x0F}},
	{.reg = 0x65, .cmd_len = 1, .cmd = {0x0C}},
	{.reg = 0x66, .cmd_len = 1, .cmd = {0x0D}},
	{.reg = 0x67, .cmd_len = 1, .cmd = {0x06}},
	{.reg = 0x68, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x69, .cmd_len = 1, .cmd = {0x07}},
	{.reg = 0x6a, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x6b, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x6c, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x6d, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x6e, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x6f, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x70, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x71, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x72, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x73, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x74, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x75, .cmd_len = 1, .cmd = {0x01}},
	{.reg = 0x76, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x77, .cmd_len = 1, .cmd = {0x14}},
	{.reg = 0x78, .cmd_len = 1, .cmd = {0x15}},
	{.reg = 0x79, .cmd_len = 1, .cmd = {0x0E}},
	{.reg = 0x7a, .cmd_len = 1, .cmd = {0x0F}},
	{.reg = 0x7b, .cmd_len = 1, .cmd = {0x0C}},
	{.reg = 0x7c, .cmd_len = 1, .cmd = {0x0D}},
	{.reg = 0x7d, .cmd_len = 1, .cmd = {0x06}},
	{.reg = 0x7e, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x7f, .cmd_len = 1, .cmd = {0x07}},
	{.reg = 0x80, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x81, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x83, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x84, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x85, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x86, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x87, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x88, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x89, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x8A, .cmd_len = 1, .cmd = {0x02}},
	/* Change to Page 4 CMD */
	{.reg = 0xff, .cmd_len = 3, .cmd = {0x98, 0x81, 0x04}},
	{.reg = 0x6C, .cmd_len = 1, .cmd = {0x15}},
	{.reg = 0x6E, .cmd_len = 1, .cmd = {0x2A}},
	{.reg = 0x6F, .cmd_len = 1, .cmd = {0x33}},
	{.reg = 0x3A, .cmd_len = 1, .cmd = {0x94}},
	{.reg = 0x8D, .cmd_len = 1, .cmd = {0x15}},
	{.reg = 0x87, .cmd_len = 1, .cmd = {0xBA}},
	{.reg = 0x26, .cmd_len = 1, .cmd = {0x76}},
	{.reg = 0xB2, .cmd_len = 1, .cmd = {0xD1}},
	{.reg = 0xB5, .cmd_len = 1, .cmd = {0x06}},
	/* Change to Page 1 CMD */
	{.reg = 0xff, .cmd_len = 3, .cmd = {0x98, 0x81, 0x01}},
	{.reg = 0x22, .cmd_len = 1, .cmd = {0x0A}},
	{.reg = 0x31, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x53, .cmd_len = 1, .cmd = {0xA5}},
	{.reg = 0x55, .cmd_len = 1, .cmd = {0xA2}},
	{.reg = 0x50, .cmd_len = 1, .cmd = {0xB7}},
	{.reg = 0x51, .cmd_len = 1, .cmd = {0xB7}},
	{.reg = 0x60, .cmd_len = 1, .cmd = {0x22}},
	{.reg = 0x61, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x62, .cmd_len = 1, .cmd = {0x19}},
	{.reg = 0x63, .cmd_len = 1, .cmd = {0x10}},
	/* Gamma Start */
	{.reg = 0xA0, .cmd_len = 1, .cmd = {0x08}},
	{.reg = 0xA1, .cmd_len = 1, .cmd = {0x17}},
	{.reg = 0xA2, .cmd_len = 1, .cmd = {0x1E}},
	{.reg = 0xA3, .cmd_len = 1, .cmd = {0x0E}},
	{.reg = 0xA4, .cmd_len = 1, .cmd = {0x13}},
	{.reg = 0xA5, .cmd_len = 1, .cmd = {0x24}},
	{.reg = 0xA6, .cmd_len = 1, .cmd = {0x1B}},
	{.reg = 0xA7, .cmd_len = 1, .cmd = {0x1B}},
	{.reg = 0xA8, .cmd_len = 1, .cmd = {0x53}},
	{.reg = 0xA9, .cmd_len = 1, .cmd = {0x1B}},
	{.reg = 0xAA, .cmd_len = 1, .cmd = {0x28}},
	{.reg = 0xAB, .cmd_len = 1, .cmd = {0x45}},
	{.reg = 0xAC, .cmd_len = 1, .cmd = {0x1A}},
	{.reg = 0xAD, .cmd_len = 1, .cmd = {0x1A}},
	{.reg = 0xAE, .cmd_len = 1, .cmd = {0x50}},
	{.reg = 0xAF, .cmd_len = 1, .cmd = {0x21}},
	{.reg = 0xB0, .cmd_len = 1, .cmd = {0x2C}},
	{.reg = 0xB1, .cmd_len = 1, .cmd = {0x3B}},
	{.reg = 0xB2, .cmd_len = 1, .cmd = {0x63}},
	{.reg = 0xB3, .cmd_len = 1, .cmd = {0x39}},
	/* Neg Register */
	{.reg = 0xC0, .cmd_len = 1, .cmd = {0x08}},
	{.reg = 0xC1, .cmd_len = 1, .cmd = {0x0C}},
	{.reg = 0xC2, .cmd_len = 1, .cmd = {0x17}},
	{.reg = 0xC3, .cmd_len = 1, .cmd = {0x0F}},
	{.reg = 0xC4, .cmd_len = 1, .cmd = {0x0B}},
	{.reg = 0xC5, .cmd_len = 1, .cmd = {0x1C}},
	{.reg = 0xC6, .cmd_len = 1, .cmd = {0x10}},
	{.reg = 0xC7, .cmd_len = 1, .cmd = {0x16}},
	{.reg = 0xC8, .cmd_len = 1, .cmd = {0x5B}},
	{.reg = 0xC9, .cmd_len = 1, .cmd = {0x1A}},
	{.reg = 0xCA, .cmd_len = 1, .cmd = {0x26}},
	{.reg = 0xCB, .cmd_len = 1, .cmd = {0x55}},
	{.reg = 0xCC, .cmd_len = 1, .cmd = {0x1D}},
	{.reg = 0xCD, .cmd_len = 1, .cmd = {0x1E}},
	{.reg = 0xCE, .cmd_len = 1, .cmd = {0x52}},
	{.reg = 0xCF, .cmd_len = 1, .cmd = {0x26}},
	{.reg = 0xD0, .cmd_len = 1, .cmd = {0x29}},
	{.reg = 0xD1, .cmd_len = 1, .cmd = {0x45}},
	{.reg = 0xD2, .cmd_len = 1, .cmd = {0x63}},
	{.reg = 0xD3, .cmd_len = 1, .cmd = {0x39}},
	/* Change to Page 0 CMD */
	{.reg = 0xff, .cmd_len = 3, .cmd = {0x98, 0x81, 0x00}}};

static int ili9881c_write_reg(const struct device *dev, uint8_t reg, const uint8_t *buf, size_t len)
{
	int ret;
	const struct ili9881c_config *cfg = dev->config;

	struct mipi_dsi_msg msg = {
		.cmd = reg,
		.tx_buf = buf,
		.tx_len = len,
		.flags = MIPI_DSI_MSG_USE_LPM,
	};

	switch (len) {
	case 0U:
		msg.type = MIPI_DSI_DCS_SHORT_WRITE;
		break;

	case 1U:
		msg.type = MIPI_DSI_DCS_SHORT_WRITE_PARAM;
		break;

	default:
		msg.type = MIPI_DSI_DCS_LONG_WRITE;
		break;
	}

	ret = mipi_dsi_transfer(cfg->mipi_dsi, cfg->channel, &msg);
	if (ret < 0) {
		LOG_ERR("Failed writing reg: 0x%x result: (%d)", reg, ret);
		return ret;
	}

	return 0;
}

static int ili9881c_write_reg_val(const struct device *dev, uint8_t reg, uint8_t value)
{
	return ili9881c_write_reg(dev, reg, &value, 1);
}

static int ili9881c_write_sequence(const struct device *dev, const struct ili9881c_init_cmd *cmd,
				   uint8_t nr_cmds)
{
	int ret = 0;

	/* Loop through all commands as long as writes are successful */
	for (int i = 0; i < nr_cmds && ret == 0; i++) {
		ret = ili9881c_write_reg(dev, cmd->reg, cmd->cmd, cmd->cmd_len);
		if (ret < 0) {
			LOG_ERR("Failed writing sequence: 0x%x result: (%d)", cmd->reg, ret);
			return ret;
		}
		cmd++;
	}

	return ret;
}

static int ili9881c_regs_init(const struct device *dev)
{
	const struct ili9881c_config *cfg = dev->config;
	int ret;

	ret = ili9881c_write_sequence(dev, init_cmds, ARRAY_SIZE(init_cmds));
	if (ret < 0) {
		return ret;
	}
	/* Add a delay, otherwise MADCTL not taken */
	k_msleep(120);

	/* Exit sleep mode */
	ret = ili9881c_write_reg(dev, MIPI_DCS_EXIT_SLEEP_MODE, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	/* Wait for sleep out exit */
	k_msleep(5);

	/* Set color mode */
	ret = ili9881c_write_reg_val(dev, MIPI_DCS_SET_PIXEL_FORMAT,
				     cfg->pixel_format == PIXEL_FORMAT_RGB_565
					     ? ILITEK_ILI9881C_COLMOD_RGB565
					     : ILITEK_ILI9881C_COLMOD_RGB888);
	if (ret < 0) {
		return ret;
	}

	/* Turn on display */
	ret = ili9881c_write_reg(dev, MIPI_DCS_SET_DISPLAY_ON, NULL, 0);

	/* Set Tearing Effect Line On */
	ret = ili9881c_write_reg_val(dev, MIPI_DCS_SET_TEAR_ON, 0);

	return ret;
}

static int ili9881c_blanking_on(const struct device *dev)
{
	const struct ili9881c_config *cfg = dev->config;
	int ret;

	if (cfg->backlight.port != NULL) {
		ret = gpio_pin_set_dt(&cfg->backlight, 0);
		if (ret) {
			LOG_ERR("Disable backlight failed! (%d)", ret);
			return ret;
		}
	}

	return ili9881c_write_reg(dev, MIPI_DCS_SET_DISPLAY_OFF, NULL, 0);
}

static int ili9881c_blanking_off(const struct device *dev)
{
	const struct ili9881c_config *cfg = dev->config;
	int ret;

	if (cfg->backlight.port != NULL) {
		ret = gpio_pin_set_dt(&cfg->backlight, 1);
		if (ret) {
			LOG_ERR("Enable backlight failed! (%d)", ret);
			return ret;
		}
	}

	return ili9881c_write_reg(dev, MIPI_DCS_SET_DISPLAY_ON, NULL, 0);
}

static void ili9881c_get_capabilities(const struct device *dev,
				      struct display_capabilities *capabilities)
{
	const struct ili9881c_config *cfg = dev->config;

	memset(capabilities, 0, sizeof(struct display_capabilities));
	capabilities->x_resolution = cfg->width;
	capabilities->y_resolution = cfg->height;
	capabilities->supported_pixel_formats = cfg->pixel_format;
	capabilities->current_pixel_format = cfg->pixel_format;
}

static int ili9881c_pixel_format(const struct device *dev,
				 const enum display_pixel_format pixel_format)
{
	const struct ili9881c_config *config = dev->config;

	LOG_WRN("Pixel format change not implemented");
	if (pixel_format == config->pixel_format) {
		return 0;
	}

	return -ENOTSUP;
}

static DEVICE_API(display, ili9881c_api) = {
	.blanking_on = ili9881c_blanking_on,
	.blanking_off = ili9881c_blanking_off,
	.set_pixel_format = ili9881c_pixel_format,
	.get_capabilities = ili9881c_get_capabilities,
};

static int ili9881c_init(const struct device *dev)
{
	const struct ili9881c_config *cfg = dev->config;
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

	/* Attach to MIPI-DSI host */
	if (cfg->pixel_format == PIXEL_FORMAT_RGB_565) {
		mdev.pixfmt = MIPI_DSI_PIXFMT_RGB565;
	} else {
		mdev.pixfmt = MIPI_DSI_PIXFMT_RGB888;
	}
	mdev.data_lanes = cfg->data_lanes;
	mdev.mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_LPM;
	mdev.timings.hactive = cfg->width;
	mdev.timings.hbp = ILITEK_ILI9881C_HBP;
	mdev.timings.hfp = ILITEK_ILI9881C_HFP;
	mdev.timings.hsync = ILITEK_ILI9881C_HSYNC;
	mdev.timings.vactive = cfg->height;
	mdev.timings.vbp = ILITEK_ILI9881C_VBP;
	mdev.timings.vfp = ILITEK_ILI9881C_VFP;
	mdev.timings.vsync = ILITEK_ILI9881C_VSYNC;

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

	ret = ili9881c_regs_init(dev);
	if (ret) {
		LOG_ERR("DSI init sequence failed! (%d)", ret);
		return ret;
	}

	return 0;
}

#define ILITEK_ILI9881C_DEFINE(n)                                                                  \
	static const struct ili9881c_config ili9881c_config_##n = {                                \
		.mipi_dsi = DEVICE_DT_GET(DT_INST_BUS(n)),                                         \
		.reset = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {0}),                            \
		.backlight = GPIO_DT_SPEC_INST_GET_OR(n, bl_gpios, {0}),                           \
		.data_lanes = DT_INST_PROP_BY_IDX(n, data_lanes, 0),                               \
		.width = DT_INST_PROP(n, width),                                                   \
		.height = DT_INST_PROP(n, height),                                                 \
		.channel = DT_INST_REG_ADDR(n),                                                    \
		.pixel_format = DT_INST_PROP(n, pixel_format),                                     \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, &ili9881c_init, NULL, NULL, &ili9881c_config_##n, POST_KERNEL,    \
			      CONFIG_DISPLAY_ILI9881C_DSI_INIT_PRIORITY, &ili9881c_api);

DT_INST_FOREACH_STATUS_OKAY(ILITEK_ILI9881C_DEFINE)
