/*
 * Copyright (c) 2024 Erik Andersson <erian747@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Based on the NT35510 driver provided by STMicroelectronics at
 * https://github.com/STMicroelectronics/stm32-nt35510/blob/main/nt35510.c
 */

#define DT_DRV_COMPAT frida_nt35510

#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include "display_nt35510.h"

LOG_MODULE_REGISTER(nt35510, CONFIG_DISPLAY_LOG_LEVEL);

/**
 * @brief  Possible values of COLMOD parameter corresponding to used pixel formats
 */
#define NT35510_COLMOD_RGB565 0x55
#define NT35510_COLMOD_RGB888 0x77

/**
 * @brief  NT35510_480X800 Timing parameters for Portrait orientation mode
 */
#define NT35510_480X800_HSYNC ((uint16_t)2)  /* Horizontal synchronization */
#define NT35510_480X800_HBP   ((uint16_t)34) /* Horizontal back porch      */
#define NT35510_480X800_HFP   ((uint16_t)34) /* Horizontal front porch     */

#define NT35510_480X800_VSYNC ((uint16_t)120) /* Vertical synchronization   */
#define NT35510_480X800_VBP   ((uint16_t)150) /* Vertical back porch        */
#define NT35510_480X800_VFP   ((uint16_t)150) /* Vertical front porch       */

/**
 * @brief  NT35510_800X480 Timing parameters for Landscape orientation mode
 *         Same values as for Portrait mode in fact.
 */
#define NT35510_800X480_HSYNC NT35510_480X800_VSYNC /* Horizontal synchronization */
#define NT35510_800X480_HBP   NT35510_480X800_VBP   /* Horizontal back porch      */
#define NT35510_800X480_HFP   NT35510_480X800_VFP   /* Horizontal front porch     */
#define NT35510_800X480_VSYNC NT35510_480X800_HSYNC /* Vertical synchronization   */
#define NT35510_800X480_VBP   NT35510_480X800_HBP   /* Vertical back porch        */
#define NT35510_800X480_VFP   NT35510_480X800_HFP   /* Vertical front porch       */

struct nt35510_config {
	const struct device *mipi_dsi;
	const struct gpio_dt_spec reset;
	const struct gpio_dt_spec backlight;
	uint8_t data_lanes;
	uint16_t width;
	uint16_t height;
	uint8_t channel;
	uint16_t rotation;
};

struct nt35510_data {
	enum display_pixel_format pixel_format;
	enum display_orientation orientation;
	uint16_t xres;
	uint16_t yres;
};

struct nt35510_init_cmd {
	uint8_t reg;
	uint8_t cmd_len;
	uint8_t cmd[5];
} __packed;

static const struct nt35510_init_cmd init_cmds[] = {
	/* LV2:  Page 1 enable */
	{.reg = 0xf0, .cmd_len = 5, .cmd = {0x55, 0xaa, 0x52, 0x08, 0x01}},
	/* AVDD: 5.2V */
	{.reg = 0xb0, .cmd_len = 3, .cmd = {0x03, 0x03, 0x03}},
	/* AVDD: Ratio */
	{.reg = 0xb6, .cmd_len = 3, .cmd = {0x46, 0x46, 0x46}},
	/* AVEE: -5.2V */
	{.reg = 0xb1, .cmd_len = 3, .cmd = {0x03, 0x03, 0x03}},
	/* AVEE: Ratio */
	{.reg = 0xb7, .cmd_len = 3, .cmd = {0x36, 0x36, 0x36}},
	/* VCL: -2.5V */
	{.reg = 0xb2, .cmd_len = 3, .cmd = {0x00, 0x00, 0x02}},
	/* VCL: Ratio */
	{.reg = 0xb8, .cmd_len = 3, .cmd = {0x26, 0x26, 0x26}},
	/* VGH: 15V (Free Pump) */
	{.reg = 0xbf, .cmd_len = 1, .cmd = {0x01}},
	/* Frida LCD MFR specific */
	{.reg = 0xb3, .cmd_len = 3, .cmd = {0x09, 0x09, 0x09}},
	/* VGH: Ratio */
	{.reg = 0xb9, .cmd_len = 3, .cmd = {0x36, 0x36, 0x36}},
	/* VGL_REG: -10V */
	{.reg = 0xb5, .cmd_len = 3, .cmd = {0x08, 0x08, 0x08}},
	/* VGLX: Ratio */
	{.reg = 0xba, .cmd_len = 3, .cmd = {0x26, 0x26, 0x26}},
	/* VGMP/VGSP: 4.5V/0V */
	{.reg = 0xbc, .cmd_len = 3, .cmd = {0x00, 0x80, 0x00}},
	/* VGMN/VGSN:-4.5V/0V */
	{.reg = 0xbd, .cmd_len = 3, .cmd = {0x00, 0x80, 0x00}},
	/* VCOM: -1.325V */
	{.reg = 0xbe, .cmd_len = 2, .cmd = {0x00, 0x50}},

	/* LV2: Page 0 enable */
	{.reg = 0xf0, .cmd_len = 5, .cmd = {0x55, 0xaa, 0x52, 0x08, 0x00}},
	/* Display optional control */
	{.reg = 0xb1, .cmd_len = 2, .cmd = {0xfc, 0x00}},
	/* Set source output data hold time */
	{.reg = 0xb6, .cmd_len = 1, .cmd = {0x03}},
	/* Display resolution control */
	{.reg = 0xb5, .cmd_len = 1, .cmd = {0x51}},
	/* Gate EQ control */
	{.reg = 0xb7, .cmd_len = 2, .cmd = {0x00, 0x00}},
	/* Src EQ control(Mode2) */
	{.reg = 0xb8, .cmd_len = 4, .cmd = {0x01, 0x02, 0x02, 0x02}},
	/* Frida LCD MFR specific */
	{.reg = 0xbc, .cmd_len = 3, .cmd = {0x00, 0x00, 0x00}},
	/* Frida LCD MFR specific */
	{.reg = 0xcc, .cmd_len = 3, .cmd = {0x03, 0x00, 0x00}},
	/* Frida LCD MFR specific */
	{.reg = 0xba, .cmd_len = 1, .cmd = {0x01}}};

static const struct nt35510_init_cmd portrait_cmds[] = {
	{.reg = NT35510_CMD_MADCTL, .cmd_len = 1, .cmd = {0x00}},
	{.reg = NT35510_CMD_CASET, .cmd_len = 4, .cmd = {0x00, 0x00, 0x01, 0xdf}},
	{.reg = NT35510_CMD_RASET, .cmd_len = 4, .cmd = {0x00, 0x00, 0x03, 0x1f}}};

static const struct nt35510_init_cmd landscape_cmds[] = {
	{.reg = NT35510_CMD_MADCTL, .cmd_len = 1, .cmd = {0x60}},
	{.reg = NT35510_CMD_CASET, .cmd_len = 4, .cmd = {0x00, 0x00, 0x03, 0x1f}},
	{.reg = NT35510_CMD_RASET, .cmd_len = 4, .cmd = {0x00, 0x00, 0x01, 0xdf}}};

static const struct nt35510_init_cmd turn_on_cmds[] = {
	/* Content Adaptive Backlight Control section start */
	{.reg = NT35510_CMD_WRDISBV, .cmd_len = 1, .cmd = {0x7f}},
	/* Brightness Control Block, Display Dimming & BackLight on */
	{.reg = NT35510_CMD_WRCTRLD, .cmd_len = 1, .cmd = {0x2c}},
	/* Image Content based Adaptive Brightness [Still Picture] */
	{.reg = NT35510_CMD_WRCABC, .cmd_len = 1, .cmd = {0x02}},
	/* Brightness, use maximum as default */
	{.reg = NT35510_CMD_WRCABCMB, .cmd_len = 1, .cmd = {0xff}},
	/* Turn on display */
	{.reg = MIPI_DCS_SET_DISPLAY_ON, .cmd_len = 0, .cmd = {}},
	/* Send Command GRAM memory write (no parameters)
	 * this initiates frame write via other DSI commands sent by
	 * DSI host from LTDC incoming pixels in video mode
	 */
	{.reg = NT35510_CMD_RAMWR, .cmd_len = 0, .cmd = {}},
};

/* Write data buffer to LCD register */
static int nt35510_write_reg(const struct device *dev, uint8_t reg, const uint8_t *buf, size_t len)
{
	int ret;
	const struct nt35510_config *cfg = dev->config;

	ret = mipi_dsi_dcs_write(cfg->mipi_dsi, cfg->channel, reg, buf, len);
	if (ret < 0) {
		LOG_ERR("Failed writing reg: 0x%x result: (%d)", reg, ret);
		return ret;
	}
	return 0;
}

/* Write an 8-bit value to a register */
static int nt35510_write_reg_val(const struct device *dev, uint8_t reg, uint8_t value)
{
	return nt35510_write_reg(dev, reg, &value, 1);
}

/* Write a list of commands to registers */
static int nt35510_write_sequence(const struct device *dev, const struct nt35510_init_cmd *cmd,
				  uint8_t nr_cmds)
{
	int ret = 0;

	/* Loop trough all commands as long as writes are successful*/
	for (int i = 0; i < nr_cmds && ret == 0; i++) {
		ret = nt35510_write_reg(dev, cmd->reg, cmd->cmd, cmd->cmd_len);
		cmd++;
	}
	return ret;
}

/* Initialization, configuration turn on sequence */
static int nt35510_config(const struct device *dev)
{
	struct nt35510_data *data = dev->data;
	int ret;

	ret = nt35510_write_sequence(dev, init_cmds, ARRAY_SIZE(init_cmds));
	if (ret < 0) {
		return ret;
	}
	/* Add a delay, otherwise MADCTL not taken */
	k_msleep(200);

	/* Configure orientation */
	if (data->orientation == DISPLAY_ORIENTATION_NORMAL) {
		ret = nt35510_write_sequence(dev, portrait_cmds, ARRAY_SIZE(portrait_cmds));
	} else {
		ret = nt35510_write_sequence(dev, landscape_cmds, ARRAY_SIZE(landscape_cmds));
	}
	if (ret < 0) {
		return ret;
	}
	/* Exit sleep mode */
	ret = nt35510_write_reg(dev, NT35510_CMD_SLPOUT, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	/* Wait for sleep out exit */
	k_msleep(20);

	/* Set color mode */
	ret = nt35510_write_reg_val(dev, NT35510_CMD_COLMOD,
				    data->pixel_format == PIXEL_FORMAT_RGB_565
					    ? NT35510_COLMOD_RGB565
					    : NT35510_COLMOD_RGB888);
	if (ret < 0) {
		return ret;
	}

	/* Adjust brightness and turn on display */
	ret = nt35510_write_sequence(dev, turn_on_cmds, ARRAY_SIZE(turn_on_cmds));
	return ret;
}

static int nt35510_blanking_on(const struct device *dev)
{
	const struct nt35510_config *cfg = dev->config;
	int ret;

	if (cfg->backlight.port != NULL) {
		ret = gpio_pin_set_dt(&cfg->backlight, 0);
		if (ret) {
			LOG_ERR("Disable backlight failed! (%d)", ret);
			return ret;
		}
	}
	return nt35510_write_reg(dev, MIPI_DCS_SET_DISPLAY_OFF, NULL, 0);
}

static int nt35510_blanking_off(const struct device *dev)
{
	const struct nt35510_config *cfg = dev->config;
	int ret;

	if (cfg->backlight.port != NULL) {
		ret = gpio_pin_set_dt(&cfg->backlight, 1);
		if (ret) {
			LOG_ERR("Enable backlight failed! (%d)", ret);
			return ret;
		}
	}
	return nt35510_write_reg(dev, MIPI_DCS_SET_DISPLAY_ON, NULL, 0);
}

static int nt35510_set_brightness(const struct device *dev, const uint8_t brightness)
{
	return nt35510_write_reg(dev, MIPI_DCS_SET_DISPLAY_BRIGHTNESS, &brightness, 1);
}

static void nt35510_get_capabilities(const struct device *dev,
				     struct display_capabilities *capabilities)
{
	const struct nt35510_config *cfg = dev->config;
	struct nt35510_data *data = dev->data;

	memset(capabilities, 0, sizeof(struct display_capabilities));
	capabilities->x_resolution = cfg->width;
	capabilities->y_resolution = cfg->height;
	capabilities->supported_pixel_formats = PIXEL_FORMAT_RGB_565 | PIXEL_FORMAT_RGB_888;
	capabilities->current_pixel_format = data->pixel_format;
	capabilities->current_orientation = data->orientation;
}

static int nt35510_set_pixel_format(const struct device *dev,
				    const enum display_pixel_format pixel_format)
{
	struct nt35510_data *data = dev->data;

	if (pixel_format == PIXEL_FORMAT_RGB_565 || pixel_format == PIXEL_FORMAT_RGB_888) {
		data->pixel_format = pixel_format;
		return 0;
	}
	LOG_ERR("Pixel format not supported");
	return -ENOTSUP;
}

static int nt35510_check_id(const struct device *dev)
{
	const struct nt35510_config *cfg = dev->config;
	uint8_t id = 0;
	int ret;

	ret = mipi_dsi_dcs_read(cfg->mipi_dsi, cfg->channel, NT35510_CMD_RDID2, &id, 1);
	if (ret != sizeof(id)) {
		LOG_ERR("Failed reading ID (%d)", ret);
		return -EIO;
	}

	if (id != NT35510_ID) {
		LOG_ERR("ID 0x%x, expected: 0x%x)", id, NT35510_ID);
		return -EINVAL;
	}
	return 0;
}

static int nt35510_init(const struct device *dev)
{
	const struct nt35510_config *cfg = dev->config;
	struct nt35510_data *data = dev->data;
	struct mipi_dsi_device mdev;
	int ret;

	if (cfg->reset.port) {
		if (!gpio_is_ready_dt(&cfg->reset)) {
			LOG_ERR("Reset GPIO device is not ready!");
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&cfg->reset, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Reset display failed! (%d)", ret);
			return ret;
		}
		k_msleep(20);
		ret = gpio_pin_set_dt(&cfg->reset, 1);
		if (ret < 0) {
			LOG_ERR("Enable display failed! (%d)", ret);
			return ret;
		}
		k_msleep(200);
	}

	/* Store x/y resolution & rotation */
	if (cfg->rotation == 0) {
		data->xres = cfg->width;
		data->yres = cfg->height;
		data->orientation = DISPLAY_ORIENTATION_NORMAL;
	} else if (cfg->rotation == 90) {
		data->xres = cfg->height;
		data->yres = cfg->width;
		data->orientation = DISPLAY_ORIENTATION_ROTATED_90;
	} else if (cfg->rotation == 180) {
		data->xres = cfg->width;
		data->yres = cfg->height;
		data->orientation = DISPLAY_ORIENTATION_ROTATED_180;
	} else if (cfg->rotation == 270) {
		data->xres = cfg->height;
		data->yres = cfg->width;
		data->orientation = DISPLAY_ORIENTATION_ROTATED_270;
	}

	/* Attach to MIPI-DSI host */
	mdev.data_lanes = cfg->data_lanes;
	mdev.mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST | MIPI_DSI_MODE_LPM;

	if (data->pixel_format == PIXEL_FORMAT_RGB_565) {
		mdev.pixfmt = MIPI_DSI_PIXFMT_RGB565;
	} else {
		mdev.pixfmt = MIPI_DSI_PIXFMT_RGB888;
	}

	mdev.timings.hactive = data->xres;
	mdev.timings.hbp = NT35510_480X800_HBP;
	mdev.timings.hfp = NT35510_480X800_HFP;
	mdev.timings.hsync = NT35510_480X800_HSYNC;
	mdev.timings.vactive = data->yres;
	mdev.timings.vbp = NT35510_480X800_VBP;
	mdev.timings.vfp = NT35510_480X800_VFP;
	mdev.timings.vsync = NT35510_480X800_VSYNC;

	ret = mipi_dsi_attach(cfg->mipi_dsi, cfg->channel, &mdev);
	if (ret < 0) {
		LOG_ERR("MIPI-DSI attach failed! (%d)", ret);
		return ret;
	}

	ret = nt35510_check_id(dev);
	if (ret) {
		LOG_ERR("Panel ID check failed! (%d)", ret);
		return ret;
	}

	ret = gpio_pin_configure_dt(&cfg->backlight, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Backlight pin init fail (%d)", ret);
		return ret;
	}

	ret = nt35510_config(dev);
	if (ret) {
		LOG_ERR("DSI init sequence failed! (%d)", ret);
		return ret;
	}

	ret = nt35510_blanking_off(dev);
	if (ret) {
		LOG_ERR("Display blanking off failed! (%d)", ret);
		return ret;
	}

	LOG_INF("Init complete(%d)", ret);
	return 0;
}

static const struct display_driver_api nt35510_api = {
	.blanking_on = nt35510_blanking_on,
	.blanking_off = nt35510_blanking_off,
	.set_brightness = nt35510_set_brightness,
	.get_capabilities = nt35510_get_capabilities,
	.set_pixel_format = nt35510_set_pixel_format,
};

#define NT35510_DEFINE(n)                                                                          \
	static const struct nt35510_config nt35510_config_##n = {                                  \
		.mipi_dsi = DEVICE_DT_GET(DT_INST_BUS(n)),                                         \
		.reset = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {0}),                            \
		.backlight = GPIO_DT_SPEC_INST_GET_OR(n, bl_gpios, {0}),                           \
		.data_lanes = DT_INST_PROP_BY_IDX(n, data_lanes, 0),                               \
		.width = DT_INST_PROP(n, width),                                                   \
		.height = DT_INST_PROP(n, height),                                                 \
		.channel = DT_INST_REG_ADDR(n),                                                    \
		.rotation = DT_INST_PROP(n, rotation),                                             \
	};                                                                                         \
                                                                                                   \
	static struct nt35510_data nt35510_data_##n = {                                            \
		.pixel_format = DT_INST_PROP(n, pixel_format),                                     \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, &nt35510_init, NULL, &nt35510_data_##n, &nt35510_config_##n,      \
			      POST_KERNEL, CONFIG_DISPLAY_NT35510_INIT_PRIORITY, &nt35510_api);

DT_INST_FOREACH_STATUS_OKAY(NT35510_DEFINE)
