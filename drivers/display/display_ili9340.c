/*
 * Copyright (c) 2017 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ilitek_ili9340

#include "display_ili9340.h"
#include <drivers/display.h>

#define LOG_LEVEL CONFIG_DISPLAY_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(display_ili9340);

#include <drivers/gpio.h>
#include <sys/byteorder.h>
#include <drivers/spi.h>
#include <string.h>

struct ili9340_data {
#if DT_INST_NODE_HAS_PROP(0, reset_gpios)
	struct device *reset_gpio;
#endif
	struct device *command_data_gpio;
	struct device *spi_dev;
	struct spi_config spi_config;
#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	struct spi_cs_control cs_ctrl;
#endif
};

#define ILI9340_CMD_DATA_PIN_COMMAND 1
#define ILI9340_CMD_DATA_PIN_DATA 0

/* The number of bytes taken by a RGB pixel */
#ifdef CONFIG_ILI9340_RGB565
#define ILI9340_RGB_SIZE 2U
#else
#define ILI9340_RGB_SIZE 3U
#endif

static void ili9340_exit_sleep(struct ili9340_data *data)
{
	ili9340_transmit(data, ILI9340_CMD_EXIT_SLEEP, NULL, 0);
	k_sleep(K_MSEC(120));
}

static int ili9340_init(struct device *dev)
{
	struct ili9340_data *data = (struct ili9340_data *)dev->data;

	LOG_DBG("Initializing display driver");

	data->spi_dev = device_get_binding(DT_INST_BUS_LABEL(0));
	if (data->spi_dev == NULL) {
		LOG_ERR("Could not get SPI device for ILI9340");
		return -EPERM;
	}

	data->spi_config.frequency = DT_INST_PROP(0, spi_max_frequency);
	data->spi_config.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8);
	data->spi_config.slave = DT_INST_REG_ADDR(0);

#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	data->cs_ctrl.gpio_dev =
		device_get_binding(DT_INST_SPI_DEV_CS_GPIOS_LABEL(0));
	data->cs_ctrl.gpio_pin = DT_INST_SPI_DEV_CS_GPIOS_PIN(0);
	data->cs_ctrl.gpio_dt_flags = DT_INST_SPI_DEV_CS_GPIOS_FLAGS(0);
	data->cs_ctrl.delay = 0U;
	data->spi_config.cs = &(data->cs_ctrl);
#else
	data->spi_config.cs = NULL;
#endif

#if DT_INST_NODE_HAS_PROP(0, reset_gpios)
	data->reset_gpio =
		device_get_binding(DT_INST_GPIO_LABEL(0, reset_gpios));
	if (data->reset_gpio == NULL) {
		LOG_ERR("Could not get GPIO port for ILI9340 reset");
		return -EPERM;
	}

	gpio_pin_configure(data->reset_gpio, DT_INST_GPIO_PIN(0, reset_gpios),
			   GPIO_OUTPUT_INACTIVE |
			   DT_INST_GPIO_FLAGS(0, reset_gpios));
#endif

	data->command_data_gpio =
		device_get_binding(DT_INST_GPIO_LABEL(0, cmd_data_gpios));
	if (data->command_data_gpio == NULL) {
		LOG_ERR("Could not get GPIO port for ILI9340 command/data");
		return -EPERM;
	}

	gpio_pin_configure(data->command_data_gpio, DT_INST_GPIO_PIN(0, cmd_data_gpios),
			   GPIO_OUTPUT |
			   DT_INST_GPIO_FLAGS(0, cmd_data_gpios));

#if DT_INST_NODE_HAS_PROP(0, reset_gpios)
	LOG_DBG("Resetting display driver");
	k_sleep(K_MSEC(1));
	gpio_pin_set(data->reset_gpio,
		     DT_INST_GPIO_PIN(0, reset_gpios), 1);
	k_sleep(K_MSEC(1));
	gpio_pin_set(data->reset_gpio,
		     DT_INST_GPIO_PIN(0, reset_gpios), 0);
	k_sleep(K_MSEC(5));
#endif

	LOG_DBG("Initializing LCD");
	ili9340_lcd_init(data);

	LOG_DBG("Exiting sleep mode");
	ili9340_exit_sleep(data);

	return 0;
}

static void ili9340_set_mem_area(struct ili9340_data *data, const uint16_t x,
				 const uint16_t y, const uint16_t w, const uint16_t h)
{
	uint16_t spi_data[2];

	spi_data[0] = sys_cpu_to_be16(x);
	spi_data[1] = sys_cpu_to_be16(x + w - 1);
	ili9340_transmit(data, ILI9340_CMD_COLUMN_ADDR, &spi_data[0], 4);

	spi_data[0] = sys_cpu_to_be16(y);
	spi_data[1] = sys_cpu_to_be16(y + h - 1);
	ili9340_transmit(data, ILI9340_CMD_PAGE_ADDR, &spi_data[0], 4);
}

static int ili9340_write(const struct device *dev, const uint16_t x,
			 const uint16_t y,
			 const struct display_buffer_descriptor *desc,
			 const void *buf)
{
	struct ili9340_data *data = (struct ili9340_data *)dev->data;
	const uint8_t *write_data_start = (uint8_t *) buf;
	struct spi_buf tx_buf;
	struct spi_buf_set tx_bufs;
	uint16_t write_cnt;
	uint16_t nbr_of_writes;
	uint16_t write_h;

	__ASSERT(desc->width <= desc->pitch, "Pitch is smaller then width");
	__ASSERT((desc->pitch * ILI9340_RGB_SIZE * desc->height)
		 <= desc->buf_size, "Input buffer to small");

	LOG_DBG("Writing %dx%d (w,h) @ %dx%d (x,y)", desc->width, desc->height,
			x, y);
	ili9340_set_mem_area(data, x, y, desc->width, desc->height);

	if (desc->pitch > desc->width) {
		write_h = 1U;
		nbr_of_writes = desc->height;
	} else {
		write_h = desc->height;
		nbr_of_writes = 1U;
	}

	ili9340_transmit(data, ILI9340_CMD_MEM_WRITE,
			 (void *) write_data_start,
			 desc->width * ILI9340_RGB_SIZE * write_h);

	tx_bufs.buffers = &tx_buf;
	tx_bufs.count = 1;

	write_data_start += (desc->pitch * ILI9340_RGB_SIZE);
	for (write_cnt = 1U; write_cnt < nbr_of_writes; ++write_cnt) {
		tx_buf.buf = (void *)write_data_start;
		tx_buf.len = desc->width * ILI9340_RGB_SIZE * write_h;
		spi_write(data->spi_dev, &data->spi_config, &tx_bufs);
		write_data_start += (desc->pitch * ILI9340_RGB_SIZE);
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
	struct ili9340_data *data = (struct ili9340_data *)dev->data;

	LOG_DBG("Turning display blanking off");
	ili9340_transmit(data, ILI9340_CMD_DISPLAY_ON, NULL, 0);
	return 0;
}

static int ili9340_display_blanking_on(const struct device *dev)
{
	struct ili9340_data *data = (struct ili9340_data *)dev->data;

	LOG_DBG("Turning display blanking on");
	ili9340_transmit(data, ILI9340_CMD_DISPLAY_OFF, NULL, 0);
	return 0;
}

static int ili9340_set_brightness(const struct device *dev,
				  const uint8_t brightness)
{
	LOG_WRN("Set brightness not implemented");
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
#ifdef CONFIG_ILI9340_RGB565
	if (pixel_format == PIXEL_FORMAT_RGB_565) {
#else
	if (pixel_format == PIXEL_FORMAT_RGB_888) {
#endif
		return 0;
	}
	LOG_ERR("Pixel format change not implemented");
	return -ENOTSUP;
}

static int ili9340_set_orientation(const struct device *dev,
				   const enum display_orientation orientation)
{
	if (orientation == DISPLAY_ORIENTATION_NORMAL) {
		return 0;
	}
	LOG_ERR("Changing display orientation not implemented");
	return -ENOTSUP;
}

static void ili9340_get_capabilities(const struct device *dev,
				     struct display_capabilities *capabilities)
{
	memset(capabilities, 0, sizeof(struct display_capabilities));
	capabilities->x_resolution = 320U;
	capabilities->y_resolution = 240U;
#ifdef CONFIG_ILI9340_RGB565
	capabilities->supported_pixel_formats = PIXEL_FORMAT_RGB_565;
	capabilities->current_pixel_format = PIXEL_FORMAT_RGB_565;
#else
	capabilities->supported_pixel_formats = PIXEL_FORMAT_RGB_888;
	capabilities->current_pixel_format = PIXEL_FORMAT_RGB_888;
#endif
	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

void ili9340_transmit(struct ili9340_data *data, uint8_t cmd, void *tx_data,
		      size_t tx_len)
{
	struct spi_buf tx_buf = { .buf = &cmd, .len = 1 };
	struct spi_buf_set tx_bufs = { .buffers = &tx_buf, .count = 1 };

	gpio_pin_set(data->command_data_gpio,
		     DT_INST_GPIO_PIN(0, cmd_data_gpios),
		     ILI9340_CMD_DATA_PIN_COMMAND);
	spi_write(data->spi_dev, &data->spi_config, &tx_bufs);

	if (tx_data != NULL) {
		tx_buf.buf = tx_data;
		tx_buf.len = tx_len;
		gpio_pin_set(data->command_data_gpio,
			     DT_INST_GPIO_PIN(0, cmd_data_gpios),
			     ILI9340_CMD_DATA_PIN_DATA);
		spi_write(data->spi_dev, &data->spi_config, &tx_bufs);
	}
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

static struct ili9340_data ili9340_data;

DEVICE_AND_API_INIT(ili9340, DT_INST_LABEL(0), &ili9340_init,
		    &ili9340_data, NULL, APPLICATION,
		    CONFIG_APPLICATION_INIT_PRIORITY, &ili9340_api);
