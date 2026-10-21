/*
 * SPDX-FileCopyrightText: 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "display_ili98xx_dsi.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(display_ili98xx_dsi, CONFIG_DISPLAY_LOG_LEVEL);

int ili98xx_dsi_write_reg(const struct device *dev, uint8_t reg, const uint8_t *buf, size_t len)
{
	int ret;
	const struct ili98xx_dsi_config *cfg = dev->config;

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

int ili98xx_dsi_write_reg_val(const struct device *dev, uint8_t reg, uint8_t value)
{
	return ili98xx_dsi_write_reg(dev, reg, &value, 1);
}

int ili98xx_dsi_write_sequence(const struct device *dev, const struct ili98xx_dsi_cmd *cmds,
			       size_t nr_cmds)
{
	int ret;

	/* Loop through all commands as long as writes are successful */
	for (size_t i = 0; i < nr_cmds; i++) {
		ret = ili98xx_dsi_write_reg(dev, cmds[i].reg, cmds[i].cmd, cmds[i].cmd_len);
		if (ret < 0) {
			LOG_ERR("Failed writing sequence: 0x%x result: (%d)", cmds[i].reg, ret);
			return ret;
		}
	}

	return 0;
}

static int ili98xx_dsi_configure(const struct device *dev)
{
	const struct ili98xx_dsi_config *cfg = dev->config;
	int ret;

	ret = cfg->regs_init_fn(dev);
	if (ret < 0) {
		return ret;
	}
	/* Add a delay, otherwise MADCTL not taken */
	k_msleep(120);

	/* Exit sleep mode */
	ret = ili98xx_dsi_write_reg(dev, MIPI_DCS_EXIT_SLEEP_MODE, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	/* Wait for sleep out exit */
	k_msleep(5);

	/* Set color mode */
	ret = ili98xx_dsi_write_reg_val(dev, MIPI_DCS_SET_PIXEL_FORMAT,
					cfg->pixel_format == PIXEL_FORMAT_RGB_565
						? ILI98XX_DSI_COLMOD_RGB565
						: ILI98XX_DSI_COLMOD_RGB888);
	if (ret < 0) {
		return ret;
	}

	/* Turn on display */
	ret = ili98xx_dsi_write_reg(dev, MIPI_DCS_SET_DISPLAY_ON, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	/* Set Tearing Effect Line On */
	ret = ili98xx_dsi_write_reg_val(dev, MIPI_DCS_SET_TEAR_ON, 0);

	return ret;
}

static int ili98xx_dsi_blanking_on(const struct device *dev)
{
	const struct ili98xx_dsi_config *cfg = dev->config;
	int ret;

	if (cfg->backlight.port != NULL) {
		ret = gpio_pin_set_dt(&cfg->backlight, 0);
		if (ret) {
			LOG_ERR("Disable backlight failed! (%d)", ret);
			return ret;
		}
	}

	return ili98xx_dsi_write_reg(dev, MIPI_DCS_SET_DISPLAY_OFF, NULL, 0);
}

static int ili98xx_dsi_blanking_off(const struct device *dev)
{
	const struct ili98xx_dsi_config *cfg = dev->config;
	int ret;

	if (cfg->backlight.port != NULL) {
		ret = gpio_pin_set_dt(&cfg->backlight, 1);
		if (ret) {
			LOG_ERR("Enable backlight failed! (%d)", ret);
			return ret;
		}
	}

	return ili98xx_dsi_write_reg(dev, MIPI_DCS_SET_DISPLAY_ON, NULL, 0);
}

static void ili98xx_dsi_get_capabilities(const struct device *dev,
					 struct display_capabilities *capabilities)
{
	const struct ili98xx_dsi_config *cfg = dev->config;

	capabilities->x_resolution = cfg->width;
	capabilities->y_resolution = cfg->height;
	capabilities->supported_pixel_formats = cfg->pixel_format;
	capabilities->current_pixel_format = cfg->pixel_format;
}

static int ili98xx_dsi_set_pixel_format(const struct device *dev,
					const enum display_pixel_format pixel_format)
{
	const struct ili98xx_dsi_config *cfg = dev->config;

	LOG_WRN("Pixel format change not implemented");
	if (pixel_format == cfg->pixel_format) {
		return 0;
	}

	return -ENOTSUP;
}

static DEVICE_API(display, ili98xx_dsi_api) = {
	.blanking_on = ili98xx_dsi_blanking_on,
	.blanking_off = ili98xx_dsi_blanking_off,
	.set_pixel_format = ili98xx_dsi_set_pixel_format,
	.get_capabilities = ili98xx_dsi_get_capabilities,
};

static int ili98xx_dsi_init(const struct device *dev)
{
	const struct ili98xx_dsi_config *cfg = dev->config;
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

	mdev.data_lanes = cfg->data_lanes;
	mdev.mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_LPM;
	mdev.timings.hactive = cfg->width;
	mdev.timings.hbp = ILI98XX_DSI_HBP;
	mdev.timings.hfp = ILI98XX_DSI_HFP;
	mdev.timings.hsync = ILI98XX_DSI_HSYNC;
	mdev.timings.vactive = cfg->height;
	mdev.timings.vbp = ILI98XX_DSI_VBP;
	mdev.timings.vfp = ILI98XX_DSI_VFP;
	mdev.timings.vsync = ILI98XX_DSI_VSYNC;
	mdev.pixfmt = cfg->pixel_format;

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

	ret = ili98xx_dsi_configure(dev);
	if (ret) {
		LOG_ERR("DSI init sequence failed! (%d)", ret);
		return ret;
	}

	return 0;
}

#define INST_DT_ILI98XX_DSI(n, t) DT_INST(n, ilitek_ili##t)

#define ILI98XX_DSI_INIT(n, t)                                                                     \
	static const struct ili98xx_dsi_config ili##t##_dsi_config_##n = {                         \
		.mipi_dsi = DEVICE_DT_GET(DT_PARENT(INST_DT_ILI98XX_DSI(n, t))),                   \
		.reset = GPIO_DT_SPEC_GET_OR(INST_DT_ILI98XX_DSI(n, t), reset_gpios, {0}),         \
		.backlight = GPIO_DT_SPEC_GET_OR(INST_DT_ILI98XX_DSI(n, t), bl_gpios, {0}),        \
		.data_lanes = DT_PROP_BY_IDX(INST_DT_ILI98XX_DSI(n, t), data_lanes, 0),            \
		.width = DT_PROP(INST_DT_ILI98XX_DSI(n, t), width),                                \
		.height = DT_PROP(INST_DT_ILI98XX_DSI(n, t), height),                              \
		.channel = DT_REG_ADDR(INST_DT_ILI98XX_DSI(n, t)),                                 \
		.pixel_format = DT_PROP(INST_DT_ILI98XX_DSI(n, t), pixel_format),                  \
		.regs_init_fn = ili##t##_dsi_regs_init,                                            \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(INST_DT_ILI98XX_DSI(n, t), ili98xx_dsi_init, NULL, NULL,                  \
			 &ili##t##_dsi_config_##n, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY,  \
			 &ili98xx_dsi_api)

#define DT_INST_FOREACH_ILI98XX_DSI_STATUS_OKAY(t)                                                 \
	LISTIFY(DT_NUM_INST_STATUS_OKAY(ilitek_ili##t), ILI98XX_DSI_INIT, (;), t)

#ifdef CONFIG_ILI9806E_DSI
#include "display_ili9806e_dsi.h"
DT_INST_FOREACH_ILI98XX_DSI_STATUS_OKAY(9806e);
#endif
