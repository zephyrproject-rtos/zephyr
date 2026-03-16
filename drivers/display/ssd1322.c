/*
 * Copyright (c) 2024 Lukasz Hawrylko
 * Copyright (c) 2025 MASSDRIVER EI (massdriver.space)
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
#define SSD1322_BLANKING_OFF_INVERSE 0xA7
#define SSD1322_EXIT_PARTIAL         0xA9
#define SSD1322_DISPLAY_OFF          0xAE
#define SSD1322_DISPLAY_ON           0xAF
#define SSD1322_SET_PHASE_LENGTH     0xB1
#define SSD1322_SET_CLOCK_DIV        0xB3
#define SSD1322_SET_GPIO             0xB5
#define SSD1322_SET_SECOND_PRECHARGE 0xB6
#define SSD1322_DEFAULT_GRAYSCALE    0xB9
#define SSD1322_SET_PRECHARGE        0xBB
#define SSD1322_SET_VCOMH            0xBE
#define SSD1322_SET_CONTRAST         0xC1
#define SSD1322_SET_MUX_RATIO        0xCA

#define SSD1322_SET_ENHANCE         0xD1
#define SSD1322_SET_ENHANCE_ENABLE  0x82
#define SSD1322_SET_ENHANCE_DISABLE 0xA2
#define SSD1322_SET_ENHANCE_END     0x20

#define SSD1322_COMMAND_LOCK        0xFD
#define SSD1322_COMMAND_LOCK_UNLOCK 0x12
#define SSD1322_COMMAND_LOCK_LOCK   0x16

/* 2 pixels per byte */
#define SSD1322_2PPB 2
/* 8 pixels per byte */
#define SSD1322_8PPB 8
/* 2 pixels per real pixel*/
#define SSD1322_2PPP 2

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
	bool grayscale_enhancement;
	bool color_inversion;
	uint8_t segments_per_pixel;
	uint8_t oscillator_freq;
	uint8_t precharge_voltage;
	uint8_t vcomh_voltage;
	uint8_t phase_length;
	uint8_t precharge_period;
	uint8_t *conversion_buf;
	size_t conversion_buf_size;
};

struct ssd1322_data {
	int current_pixel_format;
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
	const struct ssd1322_config *config = dev->config;

	return ssd1322_write_command(
		dev, config->color_inversion ? SSD1322_BLANKING_OFF_INVERSE : SSD1322_BLANKING_OFF,
		NULL, 0);
}

/* Convert what the conversion buffer can hold to pixelx (3:0) and pixelx+1 (7:4) */
static int ssd1322_convert_L_8(const struct device *dev, const uint8_t *buf, int cur_offset,
			       uint32_t pixel_count)
{
	const struct ssd1322_config *config = dev->config;
	int i = 0;

	if (config->segments_per_pixel == SSD1322_2PPP) {
		for (; i < config->conversion_buf_size && pixel_count > cur_offset + i; i += 1) {
			config->conversion_buf[i ^ 1] = (buf[cur_offset + i] >> 4) << 4;
			config->conversion_buf[i ^ 1] |= buf[cur_offset + i] >> 4;
		}
	} else {
		for (; i / 2 < config->conversion_buf_size && pixel_count > cur_offset + i;
		     i += 2) {
			config->conversion_buf[i / 2] = buf[cur_offset + i + 1] >> 4;
			config->conversion_buf[i / 2] |= (buf[cur_offset + i] >> 4) << 4;
		}
	}

	return i;
}

/* Convert what the conversion buffer can hold to pixelx (3:0) and pixelx+1 (7:4) */
static int ssd1322_convert_MONO01(const struct device *dev, const uint8_t *buf, int cur_offset,
				  uint32_t pixel_count)
{
	const struct ssd1322_config *config = dev->config;
	int i = 0;

	if (config->segments_per_pixel == SSD1322_2PPP) {
		for (; i * 8 < config->conversion_buf_size && pixel_count > cur_offset + i;
		     i += 1) {
			config->conversion_buf[i * 8] = buf[cur_offset + i] & BIT(0) ? 0xFF : 0;
			config->conversion_buf[i * 8 + 1] = buf[cur_offset + i] & BIT(1) ? 0xFF : 0;
			config->conversion_buf[i * 8 + 2] = buf[cur_offset + i] & BIT(2) ? 0xFF : 0;
			config->conversion_buf[i * 8 + 3] = buf[cur_offset + i] & BIT(3) ? 0xFF : 0;
			config->conversion_buf[i * 8 + 4] = buf[cur_offset + i] & BIT(4) ? 0xFF : 0;
			config->conversion_buf[i * 8 + 5] = buf[cur_offset + i] & BIT(5) ? 0xFF : 0;
			config->conversion_buf[i * 8 + 6] = buf[cur_offset + i] & BIT(6) ? 0xFF : 0;
			config->conversion_buf[i * 8 + 7] = buf[cur_offset + i] & BIT(7) ? 0xFF : 0;
		}
	} else {
		for (; i * 4 < config->conversion_buf_size && pixel_count > cur_offset + i;
		     i += 1) {
			config->conversion_buf[i * 4] = buf[cur_offset + i] & BIT(1) ? 0xF : 0;
			config->conversion_buf[i * 4] |= (buf[cur_offset + i] & BIT(0) ? 0xF : 0)
							 << 4;
			config->conversion_buf[i * 4 + 1] = buf[cur_offset + i] & BIT(3) ? 0xF : 0;
			config->conversion_buf[i * 4 + 1] |=
				(buf[cur_offset + i] & BIT(2) ? 0xF : 0) << 4;
			config->conversion_buf[i * 4 + 2] = buf[cur_offset + i] & BIT(5) ? 0xF : 0;
			config->conversion_buf[i * 4 + 2] |=
				(buf[cur_offset + i] & BIT(4) ? 0xF : 0) << 4;
			config->conversion_buf[i * 4 + 3] = buf[cur_offset + i] & BIT(7) ? 0xF : 0;
			config->conversion_buf[i * 4 + 3] |=
				(buf[cur_offset + i] & BIT(6) ? 0xF : 0) << 4;
		}
	}
	return i;
}

static int ssd1322_write_pixels_MONO01(const struct device *dev, const uint8_t *buf,
				       uint32_t pixel_count,
				       const struct display_buffer_descriptor *desc)
{
	const struct ssd1322_config *config = dev->config;
	struct display_buffer_descriptor mipi_desc;
	int ret, i;
	int total = 0;

	mipi_desc.pitch = desc->pitch;

	while (pixel_count > total) {
		i = ssd1322_convert_MONO01(dev, buf, total, pixel_count);

		mipi_desc.buf_size = i * 4 * config->segments_per_pixel;
		mipi_desc.width = mipi_desc.buf_size / desc->height;
		mipi_desc.height = mipi_desc.buf_size / desc->width;

		/* This is the wrong format, but it doesn't matter to almost all mipi drivers */
		ret = mipi_dbi_write_display(config->mipi_dev, &config->dbi_config,
					     config->conversion_buf, &mipi_desc,
					     PIXEL_FORMAT_MONO01);
		if (ret < 0) {
			return ret;
		}
		total += i;
	}
	mipi_dbi_release(config->mipi_dev, &config->dbi_config);
	return 0;
}

static int ssd1322_write_pixels_L_8(const struct device *dev, const uint8_t *buf,
				    uint32_t pixel_count,
				    const struct display_buffer_descriptor *desc)
{
	const struct ssd1322_config *config = dev->config;
	struct display_buffer_descriptor mipi_desc;
	int ret, i;
	int total = 0;

	mipi_desc.pitch = desc->pitch;

	while (pixel_count > total) {
		i = ssd1322_convert_L_8(dev, buf, total, pixel_count);

		mipi_desc.buf_size = i * (uint32_t)config->segments_per_pixel / SSD1322_2PPB;
		mipi_desc.width = mipi_desc.buf_size / desc->height;
		mipi_desc.height = mipi_desc.buf_size / desc->width;

		/* This is the wrong format, but it doesn't matter to almost all mipi drivers */
		ret = mipi_dbi_write_display(config->mipi_dev, &config->dbi_config,
					     config->conversion_buf, &mipi_desc, PIXEL_FORMAT_L_8);
		if (ret < 0) {
			return ret;
		}
		total += i;
	}
	mipi_dbi_release(config->mipi_dev, &config->dbi_config);
	return 0;
}

static int ssd1322_write(const struct device *dev, const uint16_t x, const uint16_t y,
			 const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct ssd1322_config *config = dev->config;
	struct ssd1322_data *data = dev->data;
	size_t buf_len;
	int ret;
	uint8_t cmd_data[2];

	if (desc->pitch != desc->width) {
		LOG_ERR("Pitch is different from width");
		return -EINVAL;
	}

	switch (data->current_pixel_format) {
	case PIXEL_FORMAT_MONO01:
		buf_len = MIN(desc->buf_size, desc->height * desc->width / SSD1322_8PPB);
	break;
	case PIXEL_FORMAT_L_8:
		buf_len = MIN(desc->buf_size, desc->height * desc->width / SSD1322_2PPB);
	break;
	default:
		return -EINVAL;
	}

	if (buf == NULL || buf_len == 0U) {
		LOG_ERR("Display buffer is not available");
		return -EINVAL;
	}

	if ((x & 1) != 0U) {
		LOG_ERR("Unsupported origin");
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

	if (data->current_pixel_format == PIXEL_FORMAT_L_8) {
		return ssd1322_write_pixels_L_8(dev, buf, desc->width * desc->height, desc);
	}
	return ssd1322_write_pixels_MONO01(dev, buf, desc->width * desc->height / SSD1322_8PPB,
					   desc);
}

static int ssd1322_set_contrast(const struct device *dev, const uint8_t contrast)
{
	return ssd1322_write_command(dev, SSD1322_SET_CONTRAST, &contrast, 1);
}

static void ssd1322_get_capabilities(const struct device *dev, struct display_capabilities *caps)
{
	const struct ssd1322_config *config = dev->config;
	struct ssd1322_data *data = dev->data;

	memset(caps, 0, sizeof(struct display_capabilities));
	caps->x_resolution = config->width;
	caps->y_resolution = config->height;
	caps->supported_pixel_formats = PIXEL_FORMAT_MONO01 | PIXEL_FORMAT_L_8;
	caps->current_pixel_format = data->current_pixel_format;
	caps->screen_info = 0;
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

	/* Unlock display */
	data[0] = SSD1322_COMMAND_LOCK_UNLOCK;
	ret = ssd1322_write_command(dev, SSD1322_COMMAND_LOCK, data, 1);
	if (ret < 0) {
		return ret;
	}

	ret = ssd1322_write_command(dev, SSD1322_DISPLAY_OFF, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	ret = ssd1322_write_command(dev, SSD1322_SET_CLOCK_DIV, &config->oscillator_freq, 1);
	if (ret < 0) {
		return ret;
	}

	ret = ssd1322_write_command(dev, SSD1322_SET_MUX_RATIO, &config->mux_ratio, 1);
	if (ret < 0) {
		return ret;
	}

	ret = ssd1322_write_command(dev, SSD1322_SET_START_LINE, &config->start_line, 1);
	if (ret < 0) {
		return ret;
	}

	ret = ssd1322_write_command(dev, SSD1322_SET_DISPLAY_OFFSET, &config->row_offset, 1);
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

	ret = ssd1322_write_command(dev, SSD1322_DEFAULT_GRAYSCALE, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	ret = ssd1322_write_command(dev, SSD1322_SET_PHASE_LENGTH, &config->phase_length, 1);
	if (ret < 0) {
		return ret;
	}

	ret = ssd1322_write_command(dev, SSD1322_SET_PRECHARGE, &config->precharge_voltage, 1);
	if (ret < 0) {
		return ret;
	}

	ret = ssd1322_write_command(dev, SSD1322_SET_SECOND_PRECHARGE, &config->precharge_period,
				    1);
	if (ret < 0) {
		return ret;
	}

	ret = ssd1322_write_command(dev, SSD1322_SET_VCOMH, &config->vcomh_voltage, 1);
	if (ret < 0) {
		return ret;
	}

	ret = ssd1322_write_command(dev, SSD1322_EXIT_PARTIAL, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	data[0] = config->grayscale_enhancement ? SSD1322_SET_ENHANCE_ENABLE
						: SSD1322_SET_ENHANCE_DISABLE;
	data[1] = SSD1322_SET_ENHANCE_END;
	ret = ssd1322_write_command(dev, SSD1322_SET_ENHANCE, data, 2);
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

static int ssd1322_set_pixel_format(const struct device *dev,
				    const enum display_pixel_format pixel_format)
{
	struct ssd1322_data *data = dev->data;

	if (pixel_format == PIXEL_FORMAT_MONO01) {
		data->current_pixel_format = PIXEL_FORMAT_MONO01;
	} else if (pixel_format == PIXEL_FORMAT_L_8) {
		data->current_pixel_format = PIXEL_FORMAT_L_8;
	} else {
		LOG_ERR("Unsupported Pixel format");
		return -EINVAL;
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
	.set_pixel_format = ssd1322_set_pixel_format,
};

#define SSD1322_WORD_SIZE(inst)                                                                    \
	((DT_STRING_UPPER_TOKEN(inst, mipi_mode) == MIPI_DBI_MODE_SPI_4WIRE) ? SPI_WORD_SET(8)     \
									     : SPI_WORD_SET(9))

#ifdef CONFIG_SSD1322_DEFAULT_GRAYSCALE
#define SSD1322_CURRENT_PIXEL_FORMAT PIXEL_FORMAT_L_8
#else
#define SSD1322_CURRENT_PIXEL_FORMAT PIXEL_FORMAT_MONO01
#endif

#define SSD1322_CONV_BUFFER_SIZE(node_id)                                                          \
	DIV_ROUND_UP(DT_PROP(node_id, width) * CONFIG_SSD1322_CONV_BUFFER_LINES *                  \
			     DT_PROP(node_id, segments_per_pixel),                                 \
		     SSD1322_2PPB)

#define SSD1322_DEFINE(node_id)                                                                    \
	static uint8_t conversion_buf##node_id[SSD1322_CONV_BUFFER_SIZE(node_id)];                 \
	static struct ssd1322_data data##node_id = {                                               \
		.current_pixel_format = SSD1322_CURRENT_PIXEL_FORMAT,                              \
	};                                                                                         \
	static const struct ssd1322_config config##node_id = {                                     \
		.height = DT_PROP(node_id, height),                                                \
		.width = DT_PROP(node_id, width),                                                  \
		.column_offset = DT_PROP(node_id, column_offset),                                  \
		.row_offset = DT_PROP(node_id, row_offset),                                        \
		.start_line = DT_PROP(node_id, start_line),                                        \
		.mux_ratio = DT_PROP(node_id, mux_ratio),                                          \
		.grayscale_enhancement = DT_PROP(node_id, grayscale_enhancement),                  \
		.remap_row_first = DT_PROP(node_id, remap_row_first),                              \
		.remap_columns = DT_PROP(node_id, remap_columns),                                  \
		.remap_rows = DT_PROP(node_id, remap_rows),                                        \
		.remap_nibble = DT_PROP(node_id, remap_nibble),                                    \
		.remap_com_odd_even_split = DT_PROP(node_id, remap_com_odd_even_split),            \
		.remap_com_dual = DT_PROP(node_id, remap_com_dual),                                \
		.segments_per_pixel = DT_PROP(node_id, segments_per_pixel),                        \
		.oscillator_freq = DT_PROP(node_id, oscillator_freq),                              \
		.precharge_voltage = DT_PROP(node_id, precharge_voltage),                          \
		.vcomh_voltage = DT_PROP(node_id, vcomh_voltage),                                  \
		.phase_length = DT_PROP(node_id, phase_length),                                    \
		.precharge_period = DT_PROP(node_id, precharge_period),                            \
		.color_inversion = DT_PROP(node_id, inversion_on),                                 \
		.mipi_dev = DEVICE_DT_GET(DT_PARENT(node_id)),                                     \
		.dbi_config = MIPI_DBI_CONFIG_DT(                                                  \
			node_id, SSD1322_WORD_SIZE(node_id) | SPI_OP_MODE_MASTER, 0),              \
		.conversion_buf = conversion_buf##node_id,                                         \
		.conversion_buf_size = sizeof(conversion_buf##node_id),                            \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(node_id, ssd1322_init, NULL, &data##node_id, &config##node_id,            \
			 POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY, &ssd1322_driver_api);

DT_FOREACH_STATUS_OKAY(solomon_ssd1322, SSD1322_DEFINE)
