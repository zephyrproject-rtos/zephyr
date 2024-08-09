/*
 * Copyright (c) 2024 Lukasz Hawrylko
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT solomon_ssd1322

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ssd1322, CONFIG_DISPLAY_LOG_LEVEL);

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/kernel.h>

#include "ssd1322_regs.h"

#define BITS_PER_PIXEL 4
#define PIXELS_IN_BYTE (8 / BITS_PER_PIXEL)

struct ssd1322_config {
	const struct device *mipi_dev;
	struct mipi_dbi_config dbi_config;
	uint16_t height;
	uint16_t width;
	uint16_t column_offset;
};

static inline int ssd1322_write_command(const struct device *dev, uint8_t cmd,
					const uint8_t *buf, size_t len)
{
	const struct ssd1322_config *config = dev->config;

	return mipi_dbi_command_write(config->mipi_dev, &config->dbi_config, cmd, buf, len);
}

static inline int ssd1322_write_data(const struct device *dev, uint8_t *buf, size_t len)
{
	const struct ssd1322_config *config = dev->config;
	struct display_buffer_descriptor mipi_desc;

	mipi_desc.buf_size = len;
	mipi_desc.width = len * PIXELS_IN_BYTE;
	mipi_desc.height = 1;
	mipi_desc.pitch = mipi_desc.width;
	return mipi_dbi_write_display(config->mipi_dev, &config->dbi_config, buf,
				      &mipi_desc, PIXEL_FORMAT_MONO01);
}

static int ssd1322_blanking_on(const struct device *dev)
{
	return ssd1322_write_command(dev, SSD1322_BLANKING_ON, NULL, 0);
}

static int ssd1322_blanking_off(const struct device *dev)
{
	return ssd1322_write_command(dev, SSD1322_BLANKING_OFF, NULL, 0);
}

static int ssd1322_write(const struct device *dev, const uint16_t x, const uint16_t y,
			 const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct ssd1322_config *config = dev->config;
	size_t buf_len;
	int ret;

	if (desc->pitch < desc->width) {
		LOG_ERR("Pitch is smaller than width");
		return -1;
	}

	buf_len = MIN(desc->buf_size, desc->height * desc->width / 8);
	if (buf == NULL || buf_len == 0U) {
		LOG_ERR("Display buffer is not available");
		return -1;
	}

	if (desc->pitch > desc->width) {
		LOG_ERR("Unsupported mode");
		return -1;
	}

	LOG_DBG("x %u, y %u, pitch %u, width %u, height %u, buf_len %u", x, y, desc->pitch,
			desc->width, desc->height, buf_len);

	uint8_t cmd_data[2];

	cmd_data[0] = config->column_offset + (x >> 2);
	cmd_data[1] = config->column_offset + (((x + desc->width) >> 2) - 1);
	ret = ssd1322_write_command(dev, SSD1322_SET_COLUMN_ADDR, cmd_data, 2);
	if (ret < 0) {
		return ret;
	}

	cmd_data[0] = y;
	cmd_data[1] = (y + desc->height - 1);
	ret = ssd1322_write_command(dev, SSD1322_SET_ROW_ADDR, cmd_data, 2);
	if (ret < 0) {
		return ret;
	}

	ret = ssd1322_write_command(dev, SSD1322_ENABLE_RAM_WRITE, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	uint8_t *buff_ptr = (uint8_t *)buf;

	/*
	 * Controller uses 4-bit grayscale format, so one pixel is represented by 4 bits.
	 * Zephyr's display API does not support this format, so I am using 1-bit b/w mode and
	 * convert each 1-bit pixel to 1111 or 0000.
	 */
	for (uint16_t j = 0; j < buf_len; j++) {
		uint8_t pixel_buff[4] = {0};

		for (uint8_t i = 0; i < 8; ++i) {
			if (*buff_ptr & (1 << i)) {
				uint8_t nibble_pos = (i & 1) ? 0 : 4;

				pixel_buff[i >> 1] |= 0x0F << nibble_pos;
			}
		}
		ret = ssd1322_write_data(dev, pixel_buff, sizeof(pixel_buff));
		if (ret < 0) {
			return ret;
		}
		buff_ptr++;
	}

	return 0;
}

static int ssd1322_set_contrast(const struct device *dev, const uint8_t contrast)
{
	return ssd1322_write_command(dev, SSD1322_SET_CONTRAST, &contrast, 1);
}

static void ssd1322_get_capabilities(const struct device *dev,
				     struct display_capabilities *caps)
{
	const struct ssd1322_config *config = dev->config;

	memset(caps, 0, sizeof(struct display_capabilities));
	caps->x_resolution = config->width;
	caps->y_resolution = config->height;
	caps->supported_pixel_formats = PIXEL_FORMAT_MONO01;
	caps->current_pixel_format = PIXEL_FORMAT_MONO01;
}

static int ssd1322_init_device(const struct device *dev)
{
	int ret;
	uint8_t data[2];
	const struct ssd1322_config *config = dev->config;

	ret = mipi_dbi_reset(config->mipi_dev, 1);
	if (ret < 0) {
		return ret;
	}
	k_usleep(100);

	ret = ssd1322_write_command(dev, SSD1322_DISPLAY_OFF, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	data[0] = 0x91;
	ret = ssd1322_write_command(dev, SSD1322_SET_CLOCK_DIV, data, 1);
	if (ret < 0) {
		return ret;
	}

	data[0] = 0x3F;
	ret = ssd1322_write_command(dev, SSD1322_SET_MUX_RATIO, data, 1);
	if (ret < 0) {
		return ret;
	}

	data[0] = 0x14; data[1] = 0x11;
	ret = ssd1322_write_command(dev, SSD1322_SET_REMAP, data, 2);
	if (ret < 0) {
		return ret;
	}

	data[0] = 0x00;
	ret = ssd1322_write_command(dev, SSD1322_SET_GPIO, data, 1);
	if (ret < 0) {
		return ret;
	}

	ret = ssd1322_write_command(dev, SSD1322_DEFAULT_GREYSCALE, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	data[0] = 0xE2;
	ret = ssd1322_write_command(dev, SSD1322_SET_PHASE_LENGTH, data, 1);
	if (ret < 0) {
		return ret;
	}

	data[0] = 0x1F;
	ret = ssd1322_write_command(dev, SSD1322_SET_PRECHARGE, data, 1);
	if (ret < 0) {
		return ret;
	}

	data[0] = 0x08;
	ret = ssd1322_write_command(dev, SSD1322_SET_SECOND_PRECHARGE, data, 1);
	if (ret < 0) {
		return ret;
	}

	data[0] = 0x07;
	ret = ssd1322_write_command(dev, SSD1322_SET_VCOMH, data, 1);
	if (ret < 0) {
		return ret;
	}

	ret = ssd1322_write_command(dev, SSD1322_EXIT_PARTIAL, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	ret = ssd1322_blanking_on(dev);
	if (ret < 0) {
		return ret;
	}

	ret = ssd1322_write_command(dev, SSD1322_DISPLAY_ON, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int ssd1322_init(const struct device *dev)
{
	const struct ssd1322_config *config = dev->config;

	if (!device_is_ready(config->mipi_dev)) {
		LOG_ERR("MIPI not ready!");
		return -ENODEV;
	}

	int ret = ssd1322_init_device(dev);

	if (ret < 0) {
		LOG_ERR("Failed to initialize device, err = %d", ret);
		return -EIO;
	}

	return 0;
}

static struct display_driver_api ssd1322_driver_api = {
	.blanking_on = ssd1322_blanking_on,
	.blanking_off = ssd1322_blanking_off,
	.write = ssd1322_write,
	.set_contrast = ssd1322_set_contrast,
	.get_capabilities = ssd1322_get_capabilities,
};

#define SSD1322_DEFINE(node_id)								\
	static const struct ssd1322_config config##node_id = {				\
		.height = DT_PROP(node_id, height),					\
		.width = DT_PROP(node_id, width),					\
		.column_offset = DT_PROP(node_id, column_offset),			\
		.mipi_dev = DEVICE_DT_GET(DT_PARENT(node_id)),\
		.dbi_config = {								\
			.mode = MIPI_DBI_MODE_SPI_4WIRE,				\
			.config = MIPI_DBI_SPI_CONFIG_DT(				\
				node_id, SPI_OP_MODE_MASTER | SPI_WORD_SET(8), 0)	\
		},									\
	};										\
											\
	DEVICE_DT_DEFINE(node_id, ssd1322_init, NULL, NULL, &config##node_id,		\
			POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY, &ssd1322_driver_api);

DT_FOREACH_STATUS_OKAY(solomon_ssd1322, SSD1322_DEFINE)
