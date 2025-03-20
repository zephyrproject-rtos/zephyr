/*
 * Copyright 2023, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sitronix_st7796s

#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/display.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/mipi_dbi.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(display_st7796s, CONFIG_DISPLAY_LOG_LEVEL);

#include "display_st7796s.h"

/* Magic numbers used to lock/unlock command settings */
#define ST7796S_UNLOCK_1 0xC3
#define ST7796S_UNLOCK_2 0x96

#define ST7796S_LOCK_1 0x3C
#define ST7796S_LOCK_2 0x69

#define ST7796S_PIXEL_SIZE 2 /* Only 16 bit color mode supported with this driver */

struct st7796s_config {
	const struct device *mipi_dbi;
	const struct mipi_dbi_config dbi_config;
	const struct gpio_dt_spec cmd_data_gpio;
	const struct gpio_dt_spec reset_gpio;
	uint16_t width;
	uint16_t height;
	bool inverted; /* Display color inversion */
	/* Display configuration parameters */
	uint8_t dic; /* Display inversion control */
	uint8_t frmctl1[2]; /* Frame rate control, normal mode */
	uint8_t frmctl2[2]; /* Frame rate control, idle mode */
	uint8_t frmctl3[2]; /* Frame rate control, partial mode */
	uint8_t bpc[4]; /* Blanking porch control */
	uint8_t dfc[4]; /* Display function control */
	uint8_t pwr1[2]; /* Power control 1 */
	uint8_t pwr2; /* Power control 2 */
	uint8_t pwr3; /* Power control 3 */
	uint8_t vcmpctl; /* VCOM control */
	uint8_t doca[8]; /* Display output ctrl */
	uint8_t pgc[14]; /* Positive gamma control */
	uint8_t ngc[14]; /* Negative gamma control */
	uint8_t madctl; /* Memory data access control */
};

static int st7796s_send_cmd(const struct device *dev,
			uint8_t cmd, const uint8_t *data, size_t len)
{
	const struct st7796s_config *config = dev->config;

	return mipi_dbi_command_write(config->mipi_dbi, &config->dbi_config,
				      cmd, data, len);
}

static int st7796s_set_cursor(const struct device *dev,
			      const uint16_t x, const uint16_t y,
			      const uint16_t width, const uint16_t height)
{
	uint16_t addr_data[2];
	int ret;

	/* Column address */
	addr_data[0] = sys_cpu_to_be16(x);
	addr_data[1] = sys_cpu_to_be16(x + width - 1);

	ret = st7796s_send_cmd(dev, ST7796S_CMD_CASET,
			       (uint8_t *)addr_data, sizeof(addr_data));
	if (ret < 0) {
		return ret;
	}

	/* Row address */
	addr_data[0] = sys_cpu_to_be16(y);
	addr_data[1] = sys_cpu_to_be16(y + height - 1);
	ret = st7796s_send_cmd(dev, ST7796S_CMD_RASET,
			       (uint8_t *)addr_data, sizeof(addr_data));
	return ret;
}

static int st7796s_blanking_on(const struct device *dev)
{
	return st7796s_send_cmd(dev, ST7796S_CMD_DISPOFF, NULL, 0);
}

static int st7796s_blanking_off(const struct device *dev)
{
	return st7796s_send_cmd(dev, ST7796S_CMD_DISPON, NULL, 0);
}


static int st7796s_write(const struct device *dev,
			 const uint16_t x,
			 const uint16_t y,
			 const struct display_buffer_descriptor *desc,
			 const void *buf)
{
	const struct st7796s_config *config = dev->config;
	int ret;
	struct display_buffer_descriptor mipi_desc;
	enum display_pixel_format pixfmt;

	ret = st7796s_set_cursor(dev, x, y, desc->width, desc->height);
	if (ret < 0) {
		return ret;
	}

	mipi_desc.buf_size = desc->width * desc->height * ST7796S_PIXEL_SIZE;

	ret =  mipi_dbi_command_write(config->mipi_dbi,
				      &config->dbi_config, ST7796S_CMD_RAMWR,
				      NULL, 0);
	if (ret < 0) {
		return ret;
	}

	if (config->madctl & ST7796S_MADCTL_BGR) {
		/* Zephyr treats RGB565 as BGR565 */
		pixfmt = PIXEL_FORMAT_RGB_565;
	} else {
		pixfmt = PIXEL_FORMAT_BGR_565;
	}

	return mipi_dbi_write_display(config->mipi_dbi,
				      &config->dbi_config, buf,
				      &mipi_desc, pixfmt);
}

static void st7796s_get_capabilities(const struct device *dev,
				     struct display_capabilities *capabilities)
{
	const struct st7796s_config *config = dev->config;

	memset(capabilities, 0, sizeof(struct display_capabilities));

	if (config->madctl & ST7796S_MADCTL_BGR) {
		/* Zephyr treats RGB565 as BGR565 */
		capabilities->current_pixel_format = PIXEL_FORMAT_RGB_565;
	} else {
		capabilities->current_pixel_format = PIXEL_FORMAT_BGR_565;
	}
	capabilities->x_resolution = config->width;
	capabilities->y_resolution = config->height;
	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static int st7796s_lcd_config(const struct device *dev)
{
	const struct st7796s_config *config = dev->config;
	int ret;
	uint8_t param;

	/* Unlock display configuration */
	param = ST7796S_UNLOCK_1;
	ret = st7796s_send_cmd(dev, ST7796S_CMD_CSCON, &param, sizeof(param));
	if (ret < 0) {
		return ret;
	}

	param = ST7796S_UNLOCK_2;
	ret = st7796s_send_cmd(dev, ST7796S_CMD_CSCON, &param, sizeof(param));
	if (ret < 0) {
		return ret;
	}

	ret = st7796s_send_cmd(dev, ST7796S_CMD_DIC, &config->dic, sizeof(config->dic));
	if (ret < 0) {
		return ret;
	}

	ret = st7796s_send_cmd(dev, ST7796S_CMD_FRMCTR1, config->frmctl1,
			       sizeof(config->frmctl1));
	if (ret < 0) {
		return ret;
	}

	ret = st7796s_send_cmd(dev, ST7796S_CMD_FRMCTR2, config->frmctl2,
			       sizeof(config->frmctl2));
	if (ret < 0) {
		return ret;
	}

	ret = st7796s_send_cmd(dev, ST7796S_CMD_FRMCTR3, config->frmctl3,
			       sizeof(config->frmctl3));
	if (ret < 0) {
		return ret;
	}

	ret = st7796s_send_cmd(dev, ST7796S_CMD_BPC, config->bpc, sizeof(config->bpc));
	if (ret < 0) {
		return ret;
	}

	ret = st7796s_send_cmd(dev, ST7796S_CMD_DFC, config->dfc, sizeof(config->dfc));
	if (ret < 0) {
		return ret;
	}

	ret = st7796s_send_cmd(dev, ST7796S_CMD_PWR1, config->pwr1, sizeof(config->pwr1));
	if (ret < 0) {
		return ret;
	}

	ret = st7796s_send_cmd(dev, ST7796S_CMD_PWR2, &config->pwr2, sizeof(config->pwr2));
	if (ret < 0) {
		return ret;
	}

	ret = st7796s_send_cmd(dev, ST7796S_CMD_PWR3, &config->pwr3, sizeof(config->pwr3));
	if (ret < 0) {
		return ret;
	}

	ret = st7796s_send_cmd(dev, ST7796S_CMD_VCMPCTL, &config->vcmpctl,
			       sizeof(config->vcmpctl));
	if (ret < 0) {
		return ret;
	}

	ret = st7796s_send_cmd(dev, ST7796S_CMD_DOCA, config->doca,
			       sizeof(config->doca));
	if (ret < 0) {
		return ret;
	}

	ret = st7796s_send_cmd(dev, ST7796S_CMD_PGC, config->pgc, sizeof(config->pgc));
	if (ret < 0) {
		return ret;
	}

	ret = st7796s_send_cmd(dev, ST7796S_CMD_NGC, config->ngc, sizeof(config->ngc));
	if (ret < 0) {
		return ret;
	}

	/* Lock display configuration */
	param = ST7796S_LOCK_1;
	ret = st7796s_send_cmd(dev, ST7796S_CMD_CSCON, &param, sizeof(param));
	if (ret < 0) {
		return ret;
	}

	param = ST7796S_LOCK_2;
	return st7796s_send_cmd(dev, ST7796S_CMD_CSCON, &param, sizeof(param));
}

static int st7796s_init(const struct device *dev)
{
	const struct st7796s_config *config = dev->config;
	int ret;
	uint8_t param;

	/* Since VDDI comes up before reset pin is low, we must reset display
	 * state. Pulse for 100 MS, per datasheet
	 */
	ret = mipi_dbi_reset(config->mipi_dbi, 100);
	if (ret < 0) {
		return ret;
	}
	/* Delay an additional 100ms after reset */
	k_msleep(100);

	/* Configure controller parameters */
	ret = st7796s_lcd_config(dev);
	if (ret < 0) {
		LOG_ERR("Could not set LCD configuration (%d)", ret);
		return ret;
	}

	if (config->inverted) {
		ret = st7796s_send_cmd(dev, ST7796S_CMD_INVON, NULL, 0);
	} else {
		ret = st7796s_send_cmd(dev, ST7796S_CMD_INVOFF, NULL, 0);
	}
	if (ret < 0) {
		return ret;
	}

	param = ST7796S_CONTROL_16BIT;
	ret = st7796s_send_cmd(dev, ST7796S_CMD_COLMOD, &param, sizeof(param));
	if (ret < 0) {
		return ret;
	}

	param = config->madctl;
	ret = st7796s_send_cmd(dev, ST7796S_CMD_MADCTL, &param, sizeof(param));
	if (ret < 0) {
		return ret;
	}

	/* Exit sleep */
	st7796s_send_cmd(dev, ST7796S_CMD_SLPOUT, NULL, 0);
	/* Delay 5ms after sleep out command, per datasheet */
	k_msleep(5);
	/* Turn on display */
	st7796s_send_cmd(dev, ST7796S_CMD_DISPON, NULL, 0);

	return 0;
}

static const struct display_driver_api st7796s_api = {
	.blanking_on = st7796s_blanking_on,
	.blanking_off = st7796s_blanking_off,
	.write = st7796s_write,
	.get_capabilities = st7796s_get_capabilities,
};


#define ST7796S_INIT(n)								\
	static const struct st7796s_config st7796s_config_##n = {		\
		.mipi_dbi = DEVICE_DT_GET(DT_INST_PARENT(n)),			\
		.dbi_config = {							\
			.config = MIPI_DBI_SPI_CONFIG_DT(			\
						DT_DRV_INST(n),			\
						SPI_OP_MODE_MASTER |		\
						SPI_WORD_SET(8),		\
						0),				\
			.mode = MIPI_DBI_MODE_SPI_4WIRE,			\
		},								\
		.width = DT_INST_PROP(n, width),				\
		.height = DT_INST_PROP(n, height),				\
		.inverted = DT_INST_PROP(n, color_invert),			\
		.dic = DT_INST_ENUM_IDX(n, invert_mode),			\
		.frmctl1 = DT_INST_PROP(n, frmctl1),				\
		.frmctl2 = DT_INST_PROP(n, frmctl2),				\
		.frmctl3 = DT_INST_PROP(n, frmctl3),				\
		.bpc = DT_INST_PROP(n, bpc),					\
		.dfc = DT_INST_PROP(n, dfc),					\
		.pwr1 = DT_INST_PROP(n, pwr1),					\
		.pwr2 = DT_INST_PROP(n, pwr2),					\
		.pwr3 = DT_INST_PROP(n, pwr3),					\
		.vcmpctl = DT_INST_PROP(n, vcmpctl),				\
		.doca = DT_INST_PROP(n, doca),					\
		.pgc = DT_INST_PROP(n, pgc),					\
		.ngc = DT_INST_PROP(n, ngc),					\
		.madctl = DT_INST_PROP(n, madctl),				\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n, st7796s_init,					\
			NULL,							\
			NULL,							\
			&st7796s_config_##n,					\
			POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY,		\
			&st7796s_api);

DT_INST_FOREACH_STATUS_OKAY(ST7796S_INIT)
