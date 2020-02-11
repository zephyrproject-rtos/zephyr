/*
 * Copyright (c) 2017 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "display_ili9340.h"
#include <drivers/display.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(display_ili9340, CONFIG_DISPLAY_LOG_LEVEL);

#include <drivers/gpio.h>
#include <sys/byteorder.h>
#include <drivers/spi.h>
#include <string.h>

#define RES_X_MAX 320U
#define RES_Y_MAX 240U

#define DT_CS_PIN         DT_INST_0_ILITEK_ILI9340_CS_GPIOS_PIN
#define DT_CMD_DATA_PIN   DT_INST_0_ILITEK_ILI9340_CMD_DATA_GPIOS_PIN
#define DT_CMD_DATA_FLAGS DT_INST_0_ILITEK_ILI9340_CMD_DATA_GPIOS_FLAGS
#define DT_RESET_PIN      DT_INST_0_ILITEK_ILI9340_RESET_GPIOS_PIN
#define DT_RESET_FLAGS    DT_INST_0_ILITEK_ILI9340_RESET_GPIOS_FLAGS

#define RESET_FLAGS (GPIO_OUTPUT_INACTIVE | DT_RESET_FLAGS)
#define CMD_DATA_FLAGS (GPIO_OUTPUT_ACTIVE | DT_CMD_DATA_FLAGS)

struct ili9340_data {
#ifdef DT_INST_0_ILITEK_ILI9340_RESET_GPIOS_CONTROLLER
	struct device *rst;
#endif
	struct device *cmd;
	struct device *spi_dev;
	struct spi_config spi_config;
#ifdef DT_INST_0_ILITEK_ILI9340_CS_GPIOS_CONTROLLER
	struct spi_cs_control cs;
#endif
	enum display_pixel_format pixel_format;
	enum display_orientation orientation;
};

void ili9340_transmit(struct ili9340_data *data, u8_t cmd, void *tx_data,
		      size_t tx_len)
{
	struct spi_buf tx_buf = { .buf = &cmd, .len = 1 };
	struct spi_buf_set tx_bufs = { .buffers = &tx_buf, .count = 1 };

	gpio_pin_set(data->cmd, DT_CMD_DATA_PIN, 1);
	spi_write(data->spi_dev, &data->spi_config, &tx_bufs);

	if (tx_data != NULL) {
		tx_buf.buf = tx_data;
		tx_buf.len = tx_len;
		gpio_pin_set(data->cmd, DT_CMD_DATA_PIN, 0);
		spi_write(data->spi_dev, &data->spi_config, &tx_bufs);
	}
}

static void ili9340_reset(struct ili9340_data *data)
{
	LOG_DBG("Resetting display");
#ifdef DT_INST_0_ILITEK_ILI9340_RESET_GPIOS_CONTROLLER
	k_sleep(K_MSEC(1));
	gpio_pin_set(cmd->rst, DT_RESET_PIN, 1);
	k_sleep(K_MSEC(1));
	gpio_pin_set(cmd->rst, DT_RESET_PIN, 0);
#else
	ili9340_transmit(data, ILI9340_CMD_SOFTWARE_RESET, NULL, 0);
#endif
	k_sleep(K_MSEC(5));
}


static void ili9340_exit_sleep(struct ili9340_data *data)
{
	ili9340_transmit(data, ILI9340_CMD_EXIT_SLEEP, NULL, 0);
	k_sleep(K_MSEC(120));
}

static void ili9340_set_mem_area(struct ili9340_data *data, const u16_t x,
				 const u16_t y, const u16_t w, const u16_t h)
{
	u16_t spi_data[2];

	spi_data[0] = sys_cpu_to_be16(x);
	spi_data[1] = sys_cpu_to_be16(x + w - 1);
	ili9340_transmit(data, ILI9340_CMD_COLUMN_ADDR, &spi_data[0], 4);

	spi_data[0] = sys_cpu_to_be16(y);
	spi_data[1] = sys_cpu_to_be16(y + h - 1);
	ili9340_transmit(data, ILI9340_CMD_PAGE_ADDR, &spi_data[0], 4);
}

static int ili9340_write(const struct device *dev, const u16_t x,
			 const u16_t y,
			 const struct display_buffer_descriptor *desc,
			 const void *buf)
{
	struct ili9340_data *data = (struct ili9340_data *)dev->driver_data;
	const u8_t *write_data_start = (u8_t *) buf;
	struct spi_buf tx_buf;
	struct spi_buf_set tx_bufs;
	u16_t write_cnt;
	u16_t nbr_of_writes;
	u16_t write_h;
	const u8_t pix_size = 2 + (data->pixel_format == PIXEL_FORMAT_RGB_888);

	__ASSERT(desc->width <= desc->pitch, "Pitch is smaller then width");
	__ASSERT((desc->pitch * pix_size * desc->height) <= desc->bu_size,
			"Input buffer to small");

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
			 desc->width * pix_size * write_h);

	tx_bufs.buffers = &tx_buf;
	tx_bufs.count = 1;

	write_data_start += (desc->pitch * pix_size);
	for (write_cnt = 1U; write_cnt < nbr_of_writes; ++write_cnt) {
		tx_buf.buf = (void *)write_data_start;
		tx_buf.len = desc->width * pix_size * write_h;
		spi_write(data->spi_dev, &data->spi_config, &tx_bufs);
		write_data_start += (desc->pitch * pix_size);
	}

	return 0;
}

static int ili9340_read(const struct device *dev, const u16_t x,
			const u16_t y,
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

static int ili9340_blanking_off(const struct device *dev)
{
	struct ili9340_data *data = (struct ili9340_data *)dev->driver_data;

	LOG_DBG("Turning display blanking off");
	ili9340_transmit(data, ILI9340_CMD_DISPLAY_ON, NULL, 0);
	return 0;
}

static int ili9340_blanking_on(const struct device *dev)
{
	struct ili9340_data *data = (struct ili9340_data *)dev->driver_data;

	LOG_DBG("Turning display blanking on");
	ili9340_transmit(data, ILI9340_CMD_DISPLAY_OFF, NULL, 0);
	return 0;
}

static int ili9340_set_brightness(const struct device *dev,
				  const u8_t brightness)
{
	LOG_WRN("Set brightness not implemented");
	return -ENOTSUP;
}

static int ili9340_set_contrast(const struct device *dev, const u8_t contrast)
{
	LOG_ERR("Set contrast not supported");
	return -ENOTSUP;
}

static int ili9340_set_pixel_format(const struct device *dev,
				    const enum display_pixel_format
				    pixel_format)
{
	struct ili9340_data *data = (struct ili9340_data *)dev->driver_data;

	LOG_DBG("change request %d -> %d", data->pixel_format, pixel_format);
	if (data->pixel_format == pixel_format) {
		return 0;
	}
	LOG_ERR("Pixel format change not implemented");
	return -ENOTSUP;
}

static int ili9340_set_orientation(const struct device *dev,
				   const enum display_orientation orientation)
{
	struct ili9340_data *data = (struct ili9340_data *)dev->driver_data;

	LOG_DBG("change request %d -> %d", data->orientation, orientation);
	if (data->orientation == orientation) {
		return 0;
	}
	LOG_ERR("Changing display orientation not implemented");
	return -ENOTSUP;
}

static void ili9340_get_capabilities(const struct device *dev,
				     struct display_capabilities *capabilities)
{
	struct ili9340_data *data = (struct ili9340_data *)dev->driver_data;

	memset(capabilities, 0, sizeof(struct display_capabilities));
	capabilities->x_resolution = RES_X_MAX;
	capabilities->y_resolution = RES_Y_MAX;
	if (IS_ENABLED(CONFIG_ILI9340_RGB565)) {
		capabilities->supported_pixel_formats = PIXEL_FORMAT_RGB_565;
		capabilities->current_pixel_format = PIXEL_FORMAT_RGB_565;
	} else if (IS_ENABLED(CONFIG_ILI9340_RGB888)) {
		capabilities->supported_pixel_formats = PIXEL_FORMAT_RGB_888;
		capabilities->current_pixel_format = PIXEL_FORMAT_RGB_888;
	} else {
		LOG_ERR("Unsupported pixel format");
	}
	capabilities->current_orientation = data->orientation;
}

static int ili9340_init(struct device *dev)
{
	struct ili9340_data *data = (struct ili9340_data *)dev->driver_data;

	LOG_DBG("Initializing display driver");

	data->orientation = DISPLAY_ORIENTATION_NORMAL;
	if (IS_ENABLED(CONFIG_ILI9340_RGB565)) {
		data->pixel_format = PIXEL_FORMAT_RGB_565;
	} else if (IS_ENABLED(CONFIG_ILI9340_RGB888)) {
		data->pixel_format = PIXEL_FORMAT_RGB_888;
	} else {
		LOG_ERR("Unsupported pixel format");
	}

	data->spi_dev = device_get_binding(DT_INST_0_ILITEK_ILI9340_BUS_NAME);
	if (data->spi_dev == NULL) {
		LOG_ERR("No '%s' device", DT_INST_0_ILITEK_ILI9340_BUS_NAME);
		return -EPERM;
	}

	data->spi_config.frequency = DT_INST_0_ILITEK_ILI9340_SPI_MAX_FREQUENCY;
	data->spi_config.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8);
	data->spi_config.slave = DT_INST_0_ILITEK_ILI9340_BASE_ADDRESS;

#ifdef DT_INST_0_ILITEK_ILI9340_CS_GPIOS_CONTROLLER
	data->cs.gpio_dev = device_get_binding(
			DT_INST_0_ILITEK_ILI9340_CS_GPIOS_CONTROLLER);
	if (!data->cs.gpio_dev) {
		LOG_ERR("No '%s' device",
				DT_INST_0_ILITEK_ILI9340_CMD_DATA_GPIOS_CONTROLLER);
		return -EPERM;
	}
	data->cs.gpio_pin = DT_CS_PIN;
	data->cs.delay = 0U;
	data->spi_config.cs = &(data->cs);
#else
	data->spi_config.cs = NULL;
#endif

#ifdef DT_INST_0_ILITEK_ILI9340_RESET_GPIOS_CONTROLLER
	data->rst = device_get_binding(
			DT_INST_0_ILITEK_ILI9340_RESET_GPIOS_CONTROLLER);
	if (data->rst == NULL) {
		LOG_ERR("No '%s' device",
				DT_INST_0_ILITEK_ILI9340_RESET_GPIOS_CONTROLLER);
		return -EPERM;
	}

	if (gpio_pin_configure(data->rst, DT_RESET_PIN, RESET_FLAGS)) {
		LOG_ERR("Couldn't configure reset pin");
		return -EIO;
	}
#endif

	data->cmd = device_get_binding(
			DT_INST_0_ILITEK_ILI9340_CMD_DATA_GPIOS_CONTROLLER);
	if (data->cmd == NULL) {
		LOG_ERR("No '%s' device",
				DT_INST_0_ILITEK_ILI9340_CMD_DATA_GPIOS_CONTROLLER);
		return -EPERM;
	}

	if (gpio_pin_configure(data->cmd, DT_CMD_DATA_PIN, CMD_DATA_FLAGS)) {
		LOG_ERR("Couldn't configure data/command pin");
		return -EIO;
	}

	LOG_DBG("Initializing LCD");

	ili9340_reset(data);

	ili9340_blanking_on(dev);

	ili9340_lcd_init(data);

	ili9340_exit_sleep(data);

	return 0;
}

static const struct display_driver_api ili9340_api = {
	.blanking_on = ili9340_blanking_on,
	.blanking_off = ili9340_blanking_off,
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

DEVICE_AND_API_INIT(ili9340, DT_INST_0_ILITEK_ILI9340_LABEL, &ili9340_init,
		    &ili9340_data, NULL, APPLICATION,
		    CONFIG_APPLICATION_INIT_PRIORITY, &ili9340_api);
