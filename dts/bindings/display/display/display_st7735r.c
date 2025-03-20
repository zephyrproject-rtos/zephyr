/*
 * Copyright (c) 2017 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 * Copyright (c) 2019 Nordic Semiconductor ASA
 * Copyright (c) 2019 Marc Reilly
 * Copyright (c) 2019 PHYTEC Messtechnik GmbH
 * Copyright (c) 2020 Endian Technologies AB
 * Copyright (c) 2020 Kim Bøndergaard <kim@fam-boendergaard.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sitronix_st7735r

#include "display_st7735r.h"

#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/display.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(display_st7735r, CONFIG_DISPLAY_LOG_LEVEL);

#define ST7735R_RESET_TIME              K_MSEC(1)
#define ST7735R_EXIT_SLEEP_TIME K_MSEC(120)

#define ST7735R_PIXEL_SIZE 2u

struct st7735r_config {
	struct spi_dt_spec bus;
	struct gpio_dt_spec cmd_data;
	struct gpio_dt_spec reset;
	uint16_t height;
	uint16_t width;
	uint8_t madctl;
	uint8_t colmod;
	uint8_t caset[4];
	uint8_t raset[4];
	uint8_t vmctr1;
	uint8_t invctr;
	uint8_t pwctr1[3];
	uint8_t pwctr2[1];
	uint8_t pwctr3[2];
	uint8_t pwctr4[2];
	uint8_t pwctr5[2];
	uint8_t frmctr1[3];
	uint8_t frmctr2[3];
	uint8_t frmctr3[6];
	uint8_t gamctrp1[16];
	uint8_t gamctrn1[16];
	bool inversion_on;
	bool rgb_is_inverted;
};

struct st7735r_data {
	uint16_t x_offset;
	uint16_t y_offset;
};

static void st7735r_set_lcd_margins(const struct device *dev,
				    uint16_t x_offset, uint16_t y_offset)
{
	struct st7735r_data *data = dev->data;

	data->x_offset = x_offset;
	data->y_offset = y_offset;
}

static void st7735r_set_cmd(const struct device *dev, int is_cmd)
{
	const struct st7735r_config *config = dev->config;

	gpio_pin_set_dt(&config->cmd_data, is_cmd);
}

static int st7735r_transmit_hold(const struct device *dev, uint8_t cmd,
				 const uint8_t *tx_data, size_t tx_count)
{
	const struct st7735r_config *config = dev->config;
	struct spi_buf tx_buf = { .buf = &cmd, .len = 1 };
	struct spi_buf_set tx_bufs = { .buffers = &tx_buf, .count = 1 };
	int ret;

	st7735r_set_cmd(dev, 1);
	ret = spi_write_dt(&config->bus, &tx_bufs);
	if (ret < 0) {
		return ret;
	}

	if (tx_data != NULL) {
		tx_buf.buf = (void *)tx_data;
		tx_buf.len = tx_count;
		st7735r_set_cmd(dev, 0);
		ret = spi_write_dt(&config->bus, &tx_bufs);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int st7735r_transmit(const struct device *dev, uint8_t cmd,
			    const uint8_t *tx_data, size_t tx_count)
{
	const struct st7735r_config *config = dev->config;
	int ret;

	ret = st7735r_transmit_hold(dev, cmd, tx_data, tx_count);
	spi_release_dt(&config->bus);
	return ret;
}

static int st7735r_exit_sleep(const struct device *dev)
{
	int ret;

	ret = st7735r_transmit(dev, ST7735R_CMD_SLEEP_OUT, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	k_sleep(ST7735R_EXIT_SLEEP_TIME);

	return 0;
}

static int st7735r_reset_display(const struct device *dev)
{
	const struct st7735r_config *config = dev->config;
	int ret;

	LOG_DBG("Resetting display");
	if (config->reset.port != NULL) {
		gpio_pin_set_dt(&config->reset, 1);
		k_sleep(ST7735R_RESET_TIME);
		gpio_pin_set_dt(&config->reset, 0);
	} else {
		ret = st7735r_transmit(dev, ST7735R_CMD_SW_RESET, NULL, 0);
		if (ret < 0) {
			return ret;
		}
	}

	k_sleep(ST7735R_EXIT_SLEEP_TIME);

	return 0;
}

static int st7735r_blanking_on(const struct device *dev)
{
	return st7735r_transmit(dev, ST7735R_CMD_DISP_OFF, NULL, 0);
}

static int st7735r_blanking_off(const struct device *dev)
{
	return st7735r_transmit(dev, ST7735R_CMD_DISP_ON, NULL, 0);
}

static int st7735r_set_mem_area(const struct device *dev,
				const uint16_t x, const uint16_t y,
				const uint16_t w, const uint16_t h)
{
	const struct st7735r_config *config = dev->config;
	struct st7735r_data *data = dev->data;
	uint16_t spi_data[2];

	int ret;

	/* ST7735S requires repeating COLMOD for each transfer */
	ret = st7735r_transmit_hold(dev, ST7735R_CMD_COLMOD, &config->colmod, 1);
	if (ret < 0) {
		return ret;
	}

	uint16_t ram_x = x + data->x_offset;
	uint16_t ram_y = y + data->y_offset;

	spi_data[0] = sys_cpu_to_be16(ram_x);
	spi_data[1] = sys_cpu_to_be16(ram_x + w - 1);
	ret = st7735r_transmit_hold(dev, ST7735R_CMD_CASET, (uint8_t *)&spi_data[0], 4);
	if (ret < 0) {
		return ret;
	}

	spi_data[0] = sys_cpu_to_be16(ram_y);
	spi_data[1] = sys_cpu_to_be16(ram_y + h - 1);
	ret = st7735r_transmit_hold(dev, ST7735R_CMD_RASET, (uint8_t *)&spi_data[0], 4);
	if (ret < 0) {
		return ret;
	}

	/* NB: CS still held - data transfer coming next */
	return 0;
}

static int st7735r_write(const struct device *dev,
			 const uint16_t x,
			 const uint16_t y,
			 const struct display_buffer_descriptor *desc,
			 const void *buf)
{
	const struct st7735r_config *config = dev->config;
	const uint8_t *write_data_start = (uint8_t *) buf;
	struct spi_buf tx_buf;
	struct spi_buf_set tx_bufs;
	uint16_t write_cnt;
	uint16_t nbr_of_writes;
	uint16_t write_h;
	int ret;

	__ASSERT(desc->width <= desc->pitch, "Pitch is smaller than width");
	__ASSERT((desc->pitch * ST7735R_PIXEL_SIZE * desc->height)
		 <= desc->buf_size, "Input buffer too small");

	LOG_DBG("Writing %dx%d (w,h) @ %dx%d (x,y)",
		desc->width, desc->height, x, y);
	ret = st7735r_set_mem_area(dev, x, y, desc->width, desc->height);
	if (ret < 0) {
		goto out;
	}

	if (desc->pitch > desc->width) {
		write_h = 1U;
		nbr_of_writes = desc->height;
	} else {
		write_h = desc->height;
		nbr_of_writes = 1U;
	}

	ret = st7735r_transmit_hold(dev, ST7735R_CMD_RAMWR,
				    (void *) write_data_start,
				    desc->width * ST7735R_PIXEL_SIZE * write_h);
	if (ret < 0) {
		goto out;
	}

	tx_bufs.buffers = &tx_buf;
	tx_bufs.count = 1;

	write_data_start += (desc->pitch * ST7735R_PIXEL_SIZE);
	for (write_cnt = 1U; write_cnt < nbr_of_writes; ++write_cnt) {
		tx_buf.buf = (void *)write_data_start;
		tx_buf.len = desc->width * ST7735R_PIXEL_SIZE * write_h;
		ret = spi_write_dt(&config->bus, &tx_bufs);
		if (ret < 0) {
			goto out;
		}

		write_data_start += (desc->pitch * ST7735R_PIXEL_SIZE);
	}

	ret = 0;
out:
	spi_release_dt(&config->bus);
	return ret;
}

static void st7735r_get_capabilities(const struct device *dev,
				     struct display_capabilities *capabilities)
{
	const struct st7735r_config *config = dev->config;

	memset(capabilities, 0, sizeof(struct display_capabilities));
	capabilities->x_resolution = config->width;
	capabilities->y_resolution = config->height;

	/*
	 * Invert the pixel format if rgb_is_inverted is enabled.
	 * Report pixel format as the same format set in the MADCTL
	 * if disabling the rgb_is_inverted option.
	 * Or not so, reporting pixel format as RGB if MADCTL setting
	 * is BGR. And also vice versa.
	 * It is a workaround for supporting buggy modules that display RGB as BGR.
	 */
	if (!(config->madctl & ST7735R_MADCTL_BGR) != !config->rgb_is_inverted) {
		capabilities->supported_pixel_formats = PIXEL_FORMAT_BGR_565;
		capabilities->current_pixel_format = PIXEL_FORMAT_BGR_565;
	} else {
		capabilities->supported_pixel_formats = PIXEL_FORMAT_RGB_565;
		capabilities->current_pixel_format = PIXEL_FORMAT_RGB_565;
	}

	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static int st7735r_set_pixel_format(const struct device *dev,
				    const enum display_pixel_format pixel_format)
{
	const struct st7735r_config *config = dev->config;

	if ((pixel_format == PIXEL_FORMAT_RGB_565) &&
	    (~config->madctl & ST7735R_MADCTL_BGR)) {
		return 0;
	}

	if ((pixel_format == PIXEL_FORMAT_BGR_565) &&
	    (config->madctl & ST7735R_MADCTL_BGR)) {
		return 0;
	}

	LOG_ERR("Pixel format change not implemented");

	return -ENOTSUP;
}

static int st7735r_set_orientation(const struct device *dev,
				   const enum display_orientation orientation)
{
	if (orientation == DISPLAY_ORIENTATION_NORMAL) {
		return 0;
	}

	LOG_ERR("Changing display orientation not implemented");

	return -ENOTSUP;
}

static int st7735r_lcd_init(const struct device *dev)
{
	const struct st7735r_config *config = dev->config;
	struct st7735r_data *data = dev->data;
	int ret;

	st7735r_set_lcd_margins(dev, data->x_offset, data->y_offset);

	ret = st7735r_transmit(dev, ST7735R_CMD_FRMCTR1, config->frmctr1,
			       sizeof(config->frmctr1));
	if (ret < 0) {
		return ret;
	}

	ret = st7735r_transmit(dev, ST7735R_CMD_FRMCTR2, config->frmctr2,
			       sizeof(config->frmctr2));
	if (ret < 0) {
		return ret;
	}

	ret = st7735r_transmit(dev, ST7735R_CMD_FRMCTR3, config->frmctr3,
			       sizeof(config->frmctr3));
	if (ret < 0) {
		return ret;
	}

	ret = st7735r_transmit(dev, ST7735R_CMD_INVCTR, &config->invctr, 1);
	if (ret < 0) {
		return ret;
	}

	ret = st7735r_transmit(dev, ST7735R_CMD_PWCTR1, config->pwctr1,
			       sizeof(config->pwctr1));
	if (ret < 0) {
		return ret;
	}

	ret = st7735r_transmit(dev, ST7735R_CMD_PWCTR2, config->pwctr2,
			       sizeof(config->pwctr2));
	if (ret < 0) {
		return ret;
	}

	ret = st7735r_transmit(dev, ST7735R_CMD_PWCTR3, config->pwctr3,
			       sizeof(config->pwctr3));
	if (ret < 0) {
		return ret;
	}

	ret = st7735r_transmit(dev, ST7735R_CMD_PWCTR4, config->pwctr4,
			       sizeof(config->pwctr4));
	if (ret < 0) {
		return ret;
	}

	ret = st7735r_transmit(dev, ST7735R_CMD_PWCTR5, config->pwctr5,
			       sizeof(config->pwctr5));
	if (ret < 0) {
		return ret;
	}

	ret = st7735r_transmit(dev, ST7735R_CMD_VMCTR1, &config->vmctr1, 1);
	if (ret < 0) {
		return ret;
	}

	if (config->inversion_on) {
		ret = st7735r_transmit(dev, ST7735R_CMD_INV_ON, NULL, 0);
	} else {
		ret = st7735r_transmit(dev, ST7735R_CMD_INV_OFF, NULL, 0);
	}
	if (ret < 0) {
		return ret;
	}

	ret = st7735r_transmit(dev, ST7735R_CMD_MADCTL, &config->madctl, 1);
	if (ret < 0) {
		return ret;
	}

	ret = st7735r_transmit(dev, ST7735R_CMD_COLMOD, &config->colmod, 1);
	if (ret < 0) {
		return ret;
	}

	ret = st7735r_transmit(dev, ST7735R_CMD_CASET, config->caset,
			       sizeof(config->caset));
	if (ret < 0) {
		return ret;
	}

	ret = st7735r_transmit(dev, ST7735R_CMD_RASET, config->raset,
			       sizeof(config->raset));
	if (ret < 0) {
		return ret;
	}

	ret = st7735r_transmit(dev, ST7735R_CMD_GAMCTRP1, config->gamctrp1,
			       sizeof(config->gamctrp1));
	if (ret < 0) {
		return ret;
	}

	ret = st7735r_transmit(dev, ST7735R_CMD_GAMCTRN1, config->gamctrn1,
			       sizeof(config->gamctrn1));
	if (ret < 0) {
		return ret;
	}

	ret = st7735r_transmit(dev, ST7735R_CMD_NORON, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	ret = st7735r_transmit(dev, ST7735R_CMD_DISP_ON, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int st7735r_init(const struct device *dev)
{
	const struct st7735r_config *config = dev->config;
	int ret;

	if (!spi_is_ready_dt(&config->bus)) {
		LOG_ERR("SPI bus %s not ready", config->bus.bus->name);
		return -ENODEV;
	}

	if (config->reset.port != NULL) {
		if (!gpio_is_ready_dt(&config->reset)) {
			LOG_ERR("Reset GPIO port for display not ready");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&config->reset,
					    GPIO_OUTPUT_INACTIVE);
		if (ret) {
			LOG_ERR("Couldn't configure reset pin");
			return ret;
		}
	}

	if (!gpio_is_ready_dt(&config->cmd_data)) {
		LOG_ERR("cmd/DATA GPIO port not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->cmd_data, GPIO_OUTPUT);
	if (ret) {
		LOG_ERR("Couldn't configure cmd/DATA pin");
		return ret;
	}

	ret = st7735r_reset_display(dev);
	if (ret < 0) {
		LOG_ERR("Couldn't reset display");
		return ret;
	}

	ret = st7735r_exit_sleep(dev);
	if (ret < 0) {
		LOG_ERR("Couldn't exit sleep");
		return ret;
	}

	ret = st7735r_lcd_init(dev);
	if (ret < 0) {
		LOG_ERR("Couldn't init LCD");
		return ret;
	}

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int st7735r_pm_action(const struct device *dev,
			     enum pm_device_action action)
{
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = st7735r_exit_sleep(dev);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		ret = st7735r_transmit(dev, ST7735R_CMD_SLEEP_IN, NULL, 0);
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

static const struct display_driver_api st7735r_api = {
	.blanking_on = st7735r_blanking_on,
	.blanking_off = st7735r_blanking_off,
	.write = st7735r_write,
	.get_capabilities = st7735r_get_capabilities,
	.set_pixel_format = st7735r_set_pixel_format,
	.set_orientation = st7735r_set_orientation,
};


#define ST7735R_INIT(inst)							\
	const static struct st7735r_config st7735r_config_ ## inst = {		\
		.bus = SPI_DT_SPEC_INST_GET(					\
			inst, SPI_OP_MODE_MASTER | SPI_WORD_SET(8) |		\
			SPI_HOLD_ON_CS | SPI_LOCK_ON, 0),			\
		.cmd_data = GPIO_DT_SPEC_INST_GET(inst, cmd_data_gpios),	\
		.reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {}),	\
		.width = DT_INST_PROP(inst, width),				\
		.height = DT_INST_PROP(inst, height),				\
		.madctl = DT_INST_PROP(inst, madctl),				\
		.colmod = DT_INST_PROP(inst, colmod),				\
		.caset = DT_INST_PROP(inst, caset),				\
		.raset = DT_INST_PROP(inst, raset),				\
		.vmctr1 = DT_INST_PROP(inst, vmctr1),				\
		.invctr = DT_INST_PROP(inst, invctr),				\
		.pwctr1 = DT_INST_PROP(inst, pwctr1),				\
		.pwctr2 = DT_INST_PROP(inst, pwctr2),				\
		.pwctr3 = DT_INST_PROP(inst, pwctr3),				\
		.pwctr4 = DT_INST_PROP(inst, pwctr4),				\
		.pwctr5 = DT_INST_PROP(inst, pwctr5),				\
		.frmctr1 = DT_INST_PROP(inst, frmctr1),				\
		.frmctr2 = DT_INST_PROP(inst, frmctr2),				\
		.frmctr3 = DT_INST_PROP(inst, frmctr3),				\
		.gamctrp1 = DT_INST_PROP(inst, gamctrp1),			\
		.gamctrn1 = DT_INST_PROP(inst, gamctrn1),			\
		.inversion_on = DT_INST_PROP(inst, inversion_on),		\
		.rgb_is_inverted = DT_INST_PROP(inst, rgb_is_inverted),		\
	};									\
										\
	static struct st7735r_data st7735r_data_ ## inst = {			\
		.x_offset = DT_INST_PROP(inst, x_offset),			\
		.y_offset = DT_INST_PROP(inst, y_offset),			\
	};									\
										\
	PM_DEVICE_DT_INST_DEFINE(inst, st7735r_pm_action);			\
										\
	DEVICE_DT_INST_DEFINE(inst, st7735r_init, PM_DEVICE_DT_INST_GET(inst),	\
			      &st7735r_data_ ## inst, &st7735r_config_ ## inst,	\
			      POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY,	\
			      &st7735r_api);

DT_INST_FOREACH_STATUS_OKAY(ST7735R_INIT)
