/*
 * Copyright (c) 2017 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 * Copyright (c) 2019 Nordic Semiconductor ASA
 * Copyright (c) 2020 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ilitek_ili9340

#include "display_ili9340.h"

#include <string.h>

#include <dt-bindings/display/ili9340.h>
#include <drivers/display.h>
#include <drivers/gpio.h>
#include <drivers/spi.h>
#include <sys/byteorder.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(display_ili9340, CONFIG_DISPLAY_LOG_LEVEL);

struct ili9340_config {
	const char *spi_name;
	uint16_t spi_addr;
	uint32_t spi_max_freq;
	const char *spi_cs_label;
	gpio_pin_t spi_cs_pin;
	gpio_dt_flags_t spi_cs_flags;
	const char *cmd_data_label;
	gpio_pin_t cmd_data_pin;
	gpio_dt_flags_t cmd_data_flags;
	const char *reset_label;
	gpio_pin_t reset_pin;
	gpio_dt_flags_t reset_flags;
	uint8_t pixel_format;
	uint16_t rotation;
	uint8_t gamset[ILI9340_GAMSET_LEN];
	uint8_t frmctr1[ILI9340_FRMCTR1_LEN];
	uint8_t disctrl[ILI9340_DISCTRL_LEN];
	uint8_t pwctrl1[ILI9340_PWCTRL1_LEN];
	uint8_t pwctrl2[ILI9340_PWCTRL2_LEN];
	uint8_t vmctrl1[ILI9340_VMCTRL1_LEN];
	uint8_t vmctrl2[ILI9340_VMCTRL2_LEN];
	uint8_t pgamctrl[ILI9340_PGAMCTRL_LEN];
	uint8_t ngamctrl[ILI9340_NGAMCTRL_LEN];
};

struct ili9340_data {
	const struct device *reset_gpio;
	const struct device *command_data_gpio;
	const struct device *spi_dev;
	struct spi_config spi_config;
	struct spi_cs_control cs_ctrl;
	uint8_t bytes_per_pixel;
	enum display_pixel_format pixel_format;
	enum display_orientation orientation;
};

static int ili9340_transmit(const struct device *dev, uint8_t cmd,
			    const void *tx_data, size_t tx_len)
{
	const struct ili9340_config *config = (struct ili9340_config *)dev->config;
	struct ili9340_data *data = (struct ili9340_data *)dev->data;

	int r;
	struct spi_buf tx_buf;
	struct spi_buf_set tx_bufs = { .buffers = &tx_buf, .count = 1U };

	/* send command */
	tx_buf.buf = &cmd;
	tx_buf.len = 1U;

	gpio_pin_set(data->command_data_gpio, config->cmd_data_pin, ILI9340_CMD);
	r = spi_write(data->spi_dev, &data->spi_config, &tx_bufs);
	if (r < 0) {
		return r;
	}

	/* send data (if any) */
	if (tx_data != NULL) {
		tx_buf.buf = (void *)tx_data;
		tx_buf.len = tx_len;

		gpio_pin_set(data->command_data_gpio, config->cmd_data_pin, ILI9340_DATA);
		r = spi_write(data->spi_dev, &data->spi_config, &tx_bufs);
		if (r < 0) {
			return r;
		}
	}

	return 0;
}

static int ili9340_exit_sleep(const struct device *dev)
{
	int r;

	r = ili9340_transmit(dev, ILI9340_CMD_EXIT_SLEEP, NULL, 0);
	if (r < 0) {
		return r;
	}

	k_sleep(K_MSEC(ILI9340_SLEEP_OUT_TIME));

	return 0;
}

static void ili9340_hw_reset(const struct device *dev)
{
	const struct ili9340_config *config = (struct ili9340_config *)dev->config;
	struct ili9340_data *data = (struct ili9340_data *)dev->data;

	if (data->reset_gpio == NULL) {
		return;
	}

	gpio_pin_set(data->reset_gpio, config->reset_pin, 1);
	k_sleep(K_MSEC(ILI9340_RESET_PULSE_TIME));
	gpio_pin_set(data->reset_gpio, config->reset_pin, 0);

	k_sleep(K_MSEC(ILI9340_RESET_WAIT_TIME));
}

static int ili9340_set_mem_area(const struct device *dev, const uint16_t x,
				const uint16_t y, const uint16_t w, const uint16_t h)
{
	int r;
	uint16_t spi_data[2];

	spi_data[0] = sys_cpu_to_be16(x);
	spi_data[1] = sys_cpu_to_be16(x + w - 1U);
	r = ili9340_transmit(dev, ILI9340_CMD_COLUMN_ADDR, &spi_data[0], 4U);
	if (r < 0) {
		return r;
	}

	spi_data[0] = sys_cpu_to_be16(y);
	spi_data[1] = sys_cpu_to_be16(y + h - 1U);
	r = ili9340_transmit(dev, ILI9340_CMD_PAGE_ADDR, &spi_data[0], 4U);
	if (r < 0) {
		return r;
	}

	return 0;
}

static int ili9340_write(const struct device *dev, const uint16_t x,
			 const uint16_t y,
			 const struct display_buffer_descriptor *desc,
			 const void *buf)
{
	struct ili9340_data *data = (struct ili9340_data *)dev->data;

	int r;
	const uint8_t *write_data_start = (const uint8_t *)buf;
	struct spi_buf tx_buf;
	struct spi_buf_set tx_bufs;
	uint16_t write_cnt;
	uint16_t nbr_of_writes;
	uint16_t write_h;

	__ASSERT(desc->width <= desc->pitch, "Pitch is smaller than width");
	__ASSERT((desc->pitch * data->bytes_per_pixel * desc->height) <= desc->buf_size,
		 "Input buffer to small");

	LOG_DBG("Writing %dx%d (w,h) @ %dx%d (x,y)", desc->width, desc->height,
			x, y);
	r = ili9340_set_mem_area(dev, x, y, desc->width, desc->height);
	if (r < 0) {
		return r;
	}

	if (desc->pitch > desc->width) {
		write_h = 1U;
		nbr_of_writes = desc->height;
	} else {
		write_h = desc->height;
		nbr_of_writes = 1U;
	}

	r = ili9340_transmit(dev, ILI9340_CMD_MEM_WRITE,
			     write_data_start,
			     desc->width * data->bytes_per_pixel * write_h);
	if (r < 0) {
		return r;
	}

	tx_bufs.buffers = &tx_buf;
	tx_bufs.count = 1;

	write_data_start += desc->pitch * data->bytes_per_pixel;
	for (write_cnt = 1U; write_cnt < nbr_of_writes; ++write_cnt) {
		tx_buf.buf = (void *)write_data_start;
		tx_buf.len = desc->width * data->bytes_per_pixel * write_h;

		r = spi_write(data->spi_dev, &data->spi_config, &tx_bufs);
		if (r < 0) {
			return r;
		}

		write_data_start += desc->pitch * data->bytes_per_pixel;
	}

	return 0;
}

static int ili9340_read(const struct device *dev, const uint16_t x,
			const uint16_t y,
			const struct display_buffer_descriptor *desc,
			void *buf)
{
	LOG_ERR("Reading not supported");
	return -ENOTSUP;
}

static void *ili9340_get_framebuffer(const struct device *dev)
{
	LOG_ERR("Direct framebuffer access not supported");
	return NULL;
}

static int ili9340_display_blanking_off(const struct device *dev)
{
	LOG_DBG("Turning display blanking off");
	return ili9340_transmit(dev, ILI9340_CMD_DISPLAY_ON, NULL, 0);
}

static int ili9340_display_blanking_on(const struct device *dev)
{
	LOG_DBG("Turning display blanking on");
	return ili9340_transmit(dev, ILI9340_CMD_DISPLAY_OFF, NULL, 0);
}

static int ili9340_set_brightness(const struct device *dev,
				  const uint8_t brightness)
{
	LOG_ERR("Set brightness not implemented");
	return -ENOTSUP;
}

static int ili9340_set_contrast(const struct device *dev, const uint8_t contrast)
{
	LOG_ERR("Set contrast not supported");
	return -ENOTSUP;
}

static int ili9340_set_pixel_format(const struct device *dev,
				    const enum display_pixel_format
				    pixel_format)
{
	struct ili9340_data *data = (struct ili9340_data *)dev->data;

	int r;
	uint8_t tx_data;
	uint8_t bytes_per_pixel;

	if (pixel_format == PIXEL_FORMAT_RGB_565) {
		bytes_per_pixel = 2U;
		tx_data = ILI9340_DATA_PIXEL_FORMAT_MCU_16_BIT |
			  ILI9340_DATA_PIXEL_FORMAT_RGB_16_BIT;
	} else if (pixel_format == PIXEL_FORMAT_RGB_888) {
		bytes_per_pixel = 3U;
		tx_data = ILI9340_DATA_PIXEL_FORMAT_MCU_18_BIT |
			  ILI9340_DATA_PIXEL_FORMAT_RGB_18_BIT;
	} else {
		LOG_ERR("Unsupported pixel format");
		return -ENOTSUP;
	}

	r = ili9340_transmit(dev, ILI9340_CMD_PIXEL_FORMAT_SET, &tx_data, 1U);
	if (r < 0) {
		return r;
	}

	data->pixel_format = pixel_format;
	data->bytes_per_pixel = bytes_per_pixel;

	return 0;
}

static int ili9340_set_orientation(const struct device *dev,
				   const enum display_orientation orientation)
{
	struct ili9340_data *data = (struct ili9340_data *)dev->data;

	int r;
	uint8_t tx_data = ILI9340_DATA_MEM_ACCESS_CTRL_BGR;

	if (orientation == DISPLAY_ORIENTATION_NORMAL) {
		tx_data |= ILI9340_DATA_MEM_ACCESS_CTRL_MX;
	} else if (orientation == DISPLAY_ORIENTATION_ROTATED_90) {
		tx_data |= ILI9340_DATA_MEM_ACCESS_CTRL_MV;
	} else if (orientation == DISPLAY_ORIENTATION_ROTATED_180) {
		tx_data |= ILI9340_DATA_MEM_ACCESS_CTRL_MY;
	} else if (orientation == DISPLAY_ORIENTATION_ROTATED_270) {
		tx_data |= ILI9340_DATA_MEM_ACCESS_CTRL_MV |
			   ILI9340_DATA_MEM_ACCESS_CTRL_MX |
			   ILI9340_DATA_MEM_ACCESS_CTRL_MY;
	}

	r = ili9340_transmit(dev, ILI9340_CMD_MEM_ACCESS_CTRL, &tx_data, 1U);
	if (r < 0) {
		return r;
	}

	data->orientation = orientation;

	return 0;
}

static void ili9340_get_capabilities(const struct device *dev,
				     struct display_capabilities *capabilities)
{
	struct ili9340_data *data = (struct ili9340_data *)dev->data;

	memset(capabilities, 0, sizeof(struct display_capabilities));

	capabilities->supported_pixel_formats = PIXEL_FORMAT_RGB_565 |
						PIXEL_FORMAT_RGB_888;
	capabilities->current_pixel_format = data->pixel_format;

	if (data->orientation == DISPLAY_ORIENTATION_NORMAL ||
	    data->orientation == DISPLAY_ORIENTATION_ROTATED_180) {
		capabilities->x_resolution = ILI9340_X_RES;
		capabilities->y_resolution = ILI9340_Y_RES;
	} else {
		capabilities->x_resolution = ILI9340_Y_RES;
		capabilities->y_resolution = ILI9340_X_RES;
	}

	capabilities->current_orientation = data->orientation;
}

static int ili9340_configure(const struct device *dev)
{
	const struct ili9340_config *config = (struct ili9340_config *)dev->config;

	int r;
	enum display_pixel_format pixel_format;
	enum display_orientation orientation;
	uint8_t tx_data[15];

	/* pixel format */
	if (config->pixel_format == ILI9340_PIXEL_FORMAT_RGB565) {
		pixel_format = PIXEL_FORMAT_RGB_565;
	} else {
		pixel_format = PIXEL_FORMAT_RGB_888;
	}

	r = ili9340_set_pixel_format(dev, pixel_format);
	if (r < 0) {
		return r;
	}

	/* orientation */
	if (config->rotation == 0U) {
		orientation = DISPLAY_ORIENTATION_NORMAL;
	} else if (config->rotation == 90U) {
		orientation = DISPLAY_ORIENTATION_ROTATED_90;
	} else if (config->rotation == 180U) {
		orientation = DISPLAY_ORIENTATION_ROTATED_180;
	} else {
		orientation = DISPLAY_ORIENTATION_ROTATED_270;
	}

	r = ili9340_set_orientation(dev, orientation);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(config->gamset, ILI9340_GAMSET_LEN, "GAMSET");
	memcpy(tx_data, config->gamset, ILI9340_GAMSET_LEN);
	r = ili9340_transmit(dev, ILI9340_CMD_GAMMA_SET, tx_data,
			     ILI9340_GAMSET_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(config->frmctr1, ILI9340_FRMCTR1_LEN, "FRMCTR1");
	memcpy(tx_data, config->frmctr1, ILI9340_FRMCTR1_LEN);
	r = ili9340_transmit(dev, ILI9340_CMD_FRAME_CTRL_NORMAL_MODE, tx_data,
			     ILI9340_FRMCTR1_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(config->disctrl, ILI9340_DISCTRL_LEN, "DISCTRL");
	memcpy(tx_data, config->disctrl, ILI9340_DISCTRL_LEN);
	r = ili9340_transmit(dev, ILI9340_CMD_DISPLAY_FUNCTION_CTRL, tx_data,
			     ILI9340_DISCTRL_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(config->pwctrl1, ILI9340_PWCTRL1_LEN, "PWCTRL1");
	memcpy(tx_data, config->pwctrl1, ILI9340_PWCTRL1_LEN);
	r = ili9340_transmit(dev, ILI9340_CMD_POWER_CTRL_1, tx_data,
			     ILI9340_PWCTRL1_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(config->pwctrl2, ILI9340_PWCTRL2_LEN, "PWCTRL2");
	memcpy(tx_data, config->pwctrl2, ILI9340_PWCTRL2_LEN);
	r = ili9340_transmit(dev, ILI9340_CMD_POWER_CTRL_2, tx_data,
			     ILI9340_PWCTRL2_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(config->vmctrl1, ILI9340_VMCTRL1_LEN, "VMCTRL1");
	memcpy(tx_data, config->vmctrl1, ILI9340_VMCTRL1_LEN);
	r = ili9340_transmit(dev, ILI9340_CMD_VCOM_CTRL_1, tx_data,
			     ILI9340_VMCTRL1_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(config->vmctrl2, ILI9340_VMCTRL2_LEN, "VMCTRL2");
	memcpy(tx_data, config->vmctrl2, ILI9340_VMCTRL2_LEN);
	r = ili9340_transmit(dev, ILI9340_CMD_VCOM_CTRL_2, tx_data,
			     ILI9340_VMCTRL2_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(config->pgamctrl, ILI9340_PGAMCTRL_LEN, "PGAMCTRL");
	memcpy(tx_data, config->pgamctrl, ILI9340_PGAMCTRL_LEN);
	r = ili9340_transmit(dev, ILI9340_CMD_POSITIVE_GAMMA_CORRECTION,
			     tx_data, ILI9340_PGAMCTRL_LEN);
	if (r < 0) {
		return r;
	}

	LOG_HEXDUMP_DBG(config->ngamctrl, ILI9340_NGAMCTRL_LEN, "NGAMCTRL");
	memcpy(tx_data, config->ngamctrl, ILI9340_NGAMCTRL_LEN);
	r = ili9340_transmit(dev, ILI9340_CMD_NEGATIVE_GAMMA_CORRECTION,
			     tx_data, ILI9340_NGAMCTRL_LEN);
	if (r < 0) {
		return r;
	}

	return 0;
}

static int ili9340_init(const struct device *dev)
{
	const struct ili9340_config *config = (struct ili9340_config *)dev->config;
	struct ili9340_data *data = (struct ili9340_data *)dev->data;

	int r;

	data->spi_dev = device_get_binding(config->spi_name);
	if (data->spi_dev == NULL) {
		LOG_ERR("Could not get SPI device %s", config->spi_name);
		return -ENODEV;
	}

	data->spi_config.frequency = config->spi_max_freq;
	data->spi_config.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8U);
	data->spi_config.slave = config->spi_addr;

	data->cs_ctrl.gpio_dev = device_get_binding(config->spi_cs_label);
	if (data->cs_ctrl.gpio_dev != NULL) {
		data->cs_ctrl.gpio_pin = config->spi_cs_pin;
		data->cs_ctrl.gpio_dt_flags = config->spi_cs_flags;
		data->cs_ctrl.delay = 0U;
		data->spi_config.cs = &data->cs_ctrl;
	}

	data->command_data_gpio = device_get_binding(config->cmd_data_label);
	if (data->command_data_gpio == NULL) {
		LOG_ERR("Could not get command/data GPIO port %s", config->cmd_data_label);
		return -ENODEV;
	}

	r = gpio_pin_configure(data->command_data_gpio, config->cmd_data_pin,
			       GPIO_OUTPUT | config->cmd_data_flags);
	if (r < 0) {
		LOG_ERR("Could not configure command/data GPIO (%d)", r);
		return r;
	}

	data->reset_gpio = device_get_binding(config->reset_label);
	if (data->reset_gpio != NULL) {
		r = gpio_pin_configure(data->reset_gpio, config->reset_pin,
				       GPIO_OUTPUT_INACTIVE | config->reset_flags);
		if (r < 0) {
			LOG_ERR("Could not configure reset GPIO (%d)", r);
			return r;
		}
	}

	ili9340_hw_reset(dev);

	r = ili9340_configure(dev);
	if (r < 0) {
		LOG_ERR("Could not configure display (%d)", r);
		return r;
	}

	r = ili9340_exit_sleep(dev);
	if (r < 0) {
		LOG_ERR("Could not exit sleep mode (%d)", r);
		return r;
	}

	return 0;
}

static const struct display_driver_api ili9340_api = {
	.blanking_on = ili9340_display_blanking_on,
	.blanking_off = ili9340_display_blanking_off,
	.write = ili9340_write,
	.read = ili9340_read,
	.get_framebuffer = ili9340_get_framebuffer,
	.set_brightness = ili9340_set_brightness,
	.set_contrast = ili9340_set_contrast,
	.get_capabilities = ili9340_get_capabilities,
	.set_pixel_format = ili9340_set_pixel_format,
	.set_orientation = ili9340_set_orientation,
};

#define ILI9340_INIT(index)                                                    \
	static const struct ili9340_config ili9340_config_##index = {          \
		.spi_name = DT_INST_BUS_LABEL(index),                          \
		.spi_addr = DT_INST_REG_ADDR(index),                           \
		.spi_max_freq = UTIL_AND(                                      \
			DT_INST_HAS_PROP(index, spi_max_frequency),            \
			DT_INST_PROP(index, spi_max_frequency)                 \
		),                                                             \
		.spi_cs_label = UTIL_AND(                                      \
			DT_INST_SPI_DEV_HAS_CS_GPIOS(index),                   \
			DT_INST_SPI_DEV_CS_GPIOS_LABEL(index)),                \
		.spi_cs_pin = UTIL_AND(                                        \
			DT_INST_SPI_DEV_HAS_CS_GPIOS(index),                   \
			DT_INST_SPI_DEV_CS_GPIOS_PIN(index)),                  \
		.spi_cs_flags = UTIL_AND(                                      \
			DT_INST_SPI_DEV_HAS_CS_GPIOS(index),                   \
			DT_INST_SPI_DEV_CS_GPIOS_FLAGS(index)),                \
		.cmd_data_label = DT_INST_GPIO_LABEL(index, cmd_data_gpios),   \
		.cmd_data_pin = DT_INST_GPIO_PIN(index, cmd_data_gpios),       \
		.cmd_data_flags = DT_INST_GPIO_FLAGS(index, cmd_data_gpios),   \
		.reset_label = UTIL_AND(                                       \
			DT_INST_NODE_HAS_PROP(index, reset_gpios),             \
			DT_INST_GPIO_LABEL(index, reset_gpios)),               \
		.reset_pin = UTIL_AND(                                         \
			DT_INST_NODE_HAS_PROP(index, reset_gpios),             \
			DT_INST_GPIO_PIN(index, reset_gpios)),                 \
		.reset_flags = UTIL_AND(                                       \
			DT_INST_NODE_HAS_PROP(index, reset_gpios),             \
			DT_INST_GPIO_FLAGS(index, reset_gpios)),               \
		.pixel_format = DT_INST_PROP(index, pixel_format),             \
		.rotation = DT_INST_PROP(index, rotation),                     \
		.gamset = DT_INST_PROP(index, gamset),                         \
		.frmctr1 = DT_INST_PROP(index, frmctr1),                       \
		.disctrl = DT_INST_PROP(index, disctrl),                       \
		.pwctrl1 = DT_INST_PROP(index, pwctrl1),                       \
		.pwctrl2 = DT_INST_PROP(index, pwctrl2),                       \
		.vmctrl1 = DT_INST_PROP(index, vmctrl1),                       \
		.vmctrl2 = DT_INST_PROP(index, vmctrl2),                       \
		.pgamctrl = DT_INST_PROP(index, pgamctrl),                     \
		.ngamctrl = DT_INST_PROP(index, ngamctrl),                     \
	};                                                                     \
									       \
	static struct ili9340_data ili9340_data_##index;                       \
									       \
	DEVICE_AND_API_INIT(ili9340_##index, DT_INST_LABEL(index),             \
			    ili9340_init, &ili9340_data_##index,               \
			    &ili9340_config_##index, POST_KERNEL,              \
			    CONFIG_APPLICATION_INIT_PRIORITY, &ili9340_api);

DT_INST_FOREACH_STATUS_OKAY(ILI9340_INIT)
