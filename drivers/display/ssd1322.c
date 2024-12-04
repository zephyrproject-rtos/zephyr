/*
 * Copyright (c) 2024 Lukasz Hawrylko
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/sys/util.h"
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

#define SSD1322_SET_COLUMN_ADDR      0x15
#define SSD1322_ENABLE_RAM_WRITE     0x5C
#define SSD1322_SET_ROW_ADDR         0x75
#define SSD1322_SET_REMAP            0xA0
#define SSD1322_SET_START_LINE       0xA1
#define SSD1322_SET_DISPLAY_OFFSET   0xA2
#define SSD1322_BLANKING_ON          0xA4
#define SSD1322_BLANKING_OFF         0xA6
#define SSD1322_EXIT_PARTIAL         0xA9
#define SSD1322_DISPLAY_OFF          0xAE
#define SSD1322_DISPLAY_ON           0xAF
#define SSD1322_SET_PHASE_LENGTH     0xB1
#define SSD1322_SET_CLOCK_DIV        0xB3
#define SSD1322_SET_GPIO             0xB5
#define SSD1322_SET_SECOND_PRECHARGE 0xB6
#define SSD1322_DEFAULT_GREYSCALE    0xB9
#define SSD1322_SET_PRECHARGE        0xBB
#define SSD1322_SET_VCOMH            0xBE
#define SSD1322_SET_CONTRAST         0xC1
#define SSD1322_SET_MUX_RATIO        0xCA
#define SSD1322_COMMAND_LOCK         0xFD

#define BITS_PER_SEGMENT  4
#define SEGMENTS_PER_BYTE (8 / BITS_PER_SEGMENT)

struct ssd1322_config {
	const struct device *mipi_dev;
	struct mipi_dbi_config dbi_config;
	uint16_t height;
	uint16_t width;
	uint16_t column_offset;
	uint8_t row_offset;
	uint8_t start_line;
	uint8_t mux_ratio;
	bool remap_row_first;
	bool remap_columns;
	bool remap_rows;
	bool remap_nibble;
	bool remap_com_odd_even_split;
	bool remap_com_dual;
	uint8_t segments_per_pixel;
	uint8_t *conversion_buf;
	size_t conversion_buf_size;
};

static inline int ssd1322_write_command(const struct device *dev, uint8_t cmd, const uint8_t *buf,
					size_t len)
{
	const struct ssd1322_config *config = dev->config;

	return mipi_dbi_command_write(config->mipi_dev, &config->dbi_config, cmd, buf, len);
}

static int ssd1322_blanking_on(const struct device *dev)
{
	return ssd1322_write_command(dev, SSD1322_BLANKING_ON, NULL, 0);
}

static int ssd1322_blanking_off(const struct device *dev)
{
	return ssd1322_write_command(dev, SSD1322_BLANKING_OFF, NULL, 0);
}

/*
 * The controller uses 4-bit grayscale format, so one pixel is represented by 4 bits.
 * Zephyr's display API does not support this format, so this uses mono01, and converts each 1-bit
 * pixel to 1111 or 0000.
 *
 * buf_in: pointer to input buffer in mono01 format. This value will bel updated to point to
 * the first byte after the last converted pixel.
 * pixel_count: pointer to the total number of pixels in buf_in to convert. The number of
 *   converted pixels will be subtracted.
 * returns the number of bytes written to buf_out.
 */
static int ssd1322_conv_mono01_grayscale(const uint8_t **buf_in, uint32_t *pixel_count,
					 uint8_t *buf_out, size_t buf_out_size,
					 uint8_t segments_per_pixel)
{
	/* Output buffer size gets rounded down to avoid splitting chunks in the middle of input
	 * bytes
	 */
	uint16_t pixels_in_chunk =
		MIN(*pixel_count,
		    ROUND_DOWN((buf_out_size * SEGMENTS_PER_BYTE) / segments_per_pixel, 8));

	for (uint16_t in_idx = 0; in_idx < pixels_in_chunk; in_idx++) {
		uint8_t color = ((*buf_in)[in_idx / 8] & BIT(in_idx % 8)) ? 0xF : 0;

		for (size_t i = 0; i < segments_per_pixel; i++) {
			size_t seg_idx = in_idx * segments_per_pixel + i;
			size_t shift = BITS_PER_SEGMENT * (seg_idx % SEGMENTS_PER_BYTE);

			if (shift == 0) {
				buf_out[seg_idx / SEGMENTS_PER_BYTE] = color;
			} else {
				buf_out[seg_idx / SEGMENTS_PER_BYTE] |= color << shift;
			}
		}
	}

	buf_in += pixels_in_chunk / 8;
	*pixel_count -= pixels_in_chunk;
	return pixels_in_chunk * segments_per_pixel / SEGMENTS_PER_BYTE;
}

static int ssd1322_write_pixels(const struct device *dev, const uint8_t *buf, uint32_t pixel_count)
{
	const struct ssd1322_config *config = dev->config;
	struct display_buffer_descriptor mipi_desc;

	while (pixel_count > 0) {
		size_t len;
		int ret;

		/* Other formats, such as RGB888 to grayscale could be added by switching here */
		len = ssd1322_conv_mono01_grayscale(&buf, &pixel_count, config->conversion_buf,
						    config->conversion_buf_size,
						    config->segments_per_pixel);

		/* As the MIPI DBI interface also does not support 4bit grayscale, it is disguised
		 * as a single row of mono01 pixels. While this could theoretically cause issues
		 * with some mipi-dbi implementations, the SPI-based driver ignores this metadata,
		 * and is likely the most relevant in practice.
		 */
		mipi_desc.buf_size = len;
		mipi_desc.width = len * 8;
		mipi_desc.height = 1;
		mipi_desc.pitch = mipi_desc.width;
		ret = mipi_dbi_write_display(config->mipi_dev, &config->dbi_config,
					     config->conversion_buf, &mipi_desc,
					     PIXEL_FORMAT_MONO01);
		if (ret < 0) {
			return ret;
		}
	}
	return 0;
}

static int ssd1322_write(const struct device *dev, const uint16_t x, const uint16_t y,
			 const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct ssd1322_config *config = dev->config;
	size_t buf_len;
	int ret;
	uint8_t cmd_data[2];
	int32_t pixel_count = desc->width * desc->height;

	if (desc->pitch < desc->width) {
		LOG_ERR("Pitch is smaller than width");
		return -EINVAL;
	}

	buf_len = MIN(desc->buf_size, desc->height * desc->width / 8);
	if (buf == NULL || buf_len == 0U) {
		LOG_ERR("Display buffer is not available");
		return -EINVAL;
	}

	if (desc->pitch > desc->width) {
		LOG_ERR("Unsupported mode");
		return -EINVAL;
	}

	LOG_DBG("x %u, y %u, pitch %u, width %u, height %u, buf_len %u", x, y, desc->pitch,
		desc->width, desc->height, buf_len);

	cmd_data[0] = config->column_offset + (x >> 2) * config->segments_per_pixel;
	cmd_data[1] =
		config->column_offset + ((x + desc->width) >> 2) * config->segments_per_pixel - 1;
	ret = ssd1322_write_command(dev, SSD1322_SET_COLUMN_ADDR, cmd_data, 2);
	if (ret < 0) {
		return ret;
	}

	cmd_data[0] = y;
	cmd_data[1] = y + desc->height - 1;
	ret = ssd1322_write_command(dev, SSD1322_SET_ROW_ADDR, cmd_data, 2);
	if (ret < 0) {
		return ret;
	}

	ret = ssd1322_write_command(dev, SSD1322_ENABLE_RAM_WRITE, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	return ssd1322_write_pixels(dev, buf, pixel_count);
}

static int ssd1322_set_contrast(const struct device *dev, const uint8_t contrast)
{
	return ssd1322_write_command(dev, SSD1322_SET_CONTRAST, &contrast, 1);
}

static void ssd1322_get_capabilities(const struct device *dev, struct display_capabilities *caps)
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

	data[0] = config->mux_ratio - 1;
	ret = ssd1322_write_command(dev, SSD1322_SET_MUX_RATIO, data, 1);
	if (ret < 0) {
		return ret;
	}

	data[0] = config->start_line;
	ret = ssd1322_write_command(dev, SSD1322_SET_START_LINE, data, 1);
	if (ret < 0) {
		return ret;
	}

	data[0] = config->row_offset;
	ret = ssd1322_write_command(dev, SSD1322_SET_DISPLAY_OFFSET, data, 1);
	if (ret < 0) {
		return ret;
	}

	data[0] = 0x00;
	data[1] = 0x01;
	WRITE_BIT(data[0], 0, config->remap_row_first);
	WRITE_BIT(data[0], 1, config->remap_columns);
	WRITE_BIT(data[0], 2, config->remap_nibble);
	WRITE_BIT(data[0], 4, config->remap_rows);
	WRITE_BIT(data[0], 5, config->remap_com_odd_even_split);
	WRITE_BIT(data[1], 4, config->remap_com_dual);
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

static DEVICE_API(display, ssd1322_driver_api) = {
	.blanking_on = ssd1322_blanking_on,
	.blanking_off = ssd1322_blanking_off,
	.write = ssd1322_write,
	.set_contrast = ssd1322_set_contrast,
	.get_capabilities = ssd1322_get_capabilities,
};

#define SSD1322_CONV_BUFFER_SIZE(node_id)                                                          \
	DIV_ROUND_UP(DT_PROP(node_id, width) * DT_PROP(node_id, height) *                          \
			     DT_PROP(node_id, segments_per_pixel),                                 \
		     SEGMENTS_PER_BYTE)

#define SSD1322_DEFINE(node_id)                                                                    \
	static uint8_t conversion_buf##node_id[SSD1322_CONV_BUFFER_SIZE(node_id)];                 \
	static const struct ssd1322_config config##node_id = {                                     \
		.height = DT_PROP(node_id, height),                                                \
		.width = DT_PROP(node_id, width),                                                  \
		.column_offset = DT_PROP(node_id, column_offset),                                  \
		.row_offset = DT_PROP(node_id, row_offset),                                        \
		.start_line = DT_PROP(node_id, start_line),                                        \
		.mux_ratio = DT_PROP(node_id, mux_ratio),                                          \
		.remap_row_first = DT_PROP(node_id, remap_row_first),                              \
		.remap_columns = DT_PROP(node_id, remap_columns),                                  \
		.remap_rows = DT_PROP(node_id, remap_rows),                                        \
		.remap_nibble = DT_PROP(node_id, remap_nibble),                                    \
		.remap_com_odd_even_split = DT_PROP(node_id, remap_com_odd_even_split),            \
		.remap_com_dual = DT_PROP(node_id, remap_com_dual),                                \
		.segments_per_pixel = DT_PROP(node_id, segments_per_pixel),                        \
		.mipi_dev = DEVICE_DT_GET(DT_PARENT(node_id)),                                     \
		.dbi_config = {.mode = MIPI_DBI_MODE_SPI_4WIRE,                                    \
			       .config = MIPI_DBI_SPI_CONFIG_DT(                                   \
				       node_id, SPI_OP_MODE_MASTER | SPI_WORD_SET(8), 0)},         \
		.conversion_buf = conversion_buf##node_id,                                         \
		.conversion_buf_size = sizeof(conversion_buf##node_id),                            \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(node_id, ssd1322_init, NULL, NULL, &config##node_id, POST_KERNEL,         \
			 CONFIG_DISPLAY_INIT_PRIORITY, &ssd1322_driver_api);

DT_FOREACH_STATUS_OKAY(solomon_ssd1322, SSD1322_DEFINE)
