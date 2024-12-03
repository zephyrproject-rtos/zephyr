/*
 * Copyright (c) 2024 Savoir-faire Linux
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ssd1327, CONFIG_DISPLAY_LOG_LEVEL);

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/kernel.h>

#include "ssd1327_regs.h"

#define SSD1327_ENABLE_VDD				0x01
#define SSD1327_ENABLE_SECOND_PRECHARGE			0x62
#define SSD1327_VCOMH_VOLTAGE				0x0f
#define SSD1327_PHASES_VALUE				0xf1
#define SSD1327_DEFAULT_PRECHARGE_V			0x08
#define SSD1327_UNLOCK_COMMAND				0x12

struct ssd1327_config {
	const struct device *mipi_dev;
	const struct mipi_dbi_config dbi_config;
	uint16_t height;
	uint16_t width;
	uint8_t oscillator_freq;
	uint8_t start_line;
	uint8_t display_offset;
	uint8_t multiplex_ratio;
	uint8_t prechargep;
	uint8_t remap_value;
	bool color_inversion;
};

struct ssd1327_data {
	uint8_t contrast;
	uint8_t scan_mode;
};

static inline int ssd1327_write_bus_cmd(const struct device *dev, const uint8_t cmd,
					const uint8_t *data, size_t len)
{
	const struct ssd1327_config *config = dev->config;
	int err;

	/* Values given after the memory register must be sent with pin D/C set to 0. */
	/* Data is sent as a command following the mipi_cbi api */
	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config, cmd, NULL, 0);
	if (err) {
		return err;
	}
	for (size_t i = 0; i < len; i++) {
		err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config,
					     data[i], NULL, 0);
		if (err) {
			return err;
		}
	}
	mipi_dbi_release(config->mipi_dev, &config->dbi_config);

	return 0;
}

static inline int ssd1327_set_timing_setting(const struct device *dev)
{
	const struct ssd1327_config *config = dev->config;
	uint8_t buf;

	buf = SSD1327_PHASES_VALUE;
	if (ssd1327_write_bus_cmd(dev, SSD1327_SET_PHASE_LENGTH, &buf, 1)) {
		return -EIO;
	}
	if (ssd1327_write_bus_cmd(dev, SSD1327_SET_OSC_FREQ, &config->oscillator_freq, 1)) {
		return -EIO;
	}
	if (ssd1327_write_bus_cmd(dev, SSD1327_SET_PRECHARGE_PERIOD, &config->prechargep, 1)) {
		return -EIO;
	}
	if (ssd1327_write_bus_cmd(dev, SSD1327_LINEAR_LUT, NULL, 0)) {
		return -EIO;
	}
	buf = SSD1327_DEFAULT_PRECHARGE_V;
	if (ssd1327_write_bus_cmd(dev, SSD1327_SET_PRECHARGE_VOLTAGE, &buf, 1)) {
		return -EIO;
	}
	buf = SSD1327_VCOMH_VOLTAGE;
	if (ssd1327_write_bus_cmd(dev, SSD1327_SET_VCOMH, &buf, 1)) {
		return -EIO;
	}
	buf = SSD1327_ENABLE_SECOND_PRECHARGE;
	if (ssd1327_write_bus_cmd(dev, SSD1327_FUNCTION_SELECTION_B, &buf, 1)) {
		return -EIO;
	}
	buf = SSD1327_UNLOCK_COMMAND;
	if (ssd1327_write_bus_cmd(dev, SSD1327_SET_COMMAND_LOCK, &buf, 1)) {
		return -EIO;
	}

	return 0;
}

static inline int ssd1327_set_hardware_config(const struct device *dev)
{
	const struct ssd1327_config *config = dev->config;
	uint8_t buf;

	if (ssd1327_write_bus_cmd(dev, SSD1327_SET_DISPLAY_START_LINE, &config->start_line, 1)) {
		return -EIO;
	}
	if (ssd1327_write_bus_cmd(dev, SSD1327_SET_DISPLAY_OFFSET, &config->display_offset, 1)) {
		return -EIO;
	}
	if (ssd1327_write_bus_cmd(dev, SSD1327_SET_NORMAL_DISPLAY, NULL, 0)) {
		return -EIO;
	}
	if (ssd1327_write_bus_cmd(dev, SSD1327_SET_SEGMENT_MAP_REMAPED, &config->remap_value, 1)) {
		return -EIO;
	}
	if (ssd1327_write_bus_cmd(dev, SSD1327_SET_MULTIPLEX_RATIO, &config->multiplex_ratio, 1)) {
		return -EIO;
	}
	buf = SSD1327_ENABLE_VDD;
	if (ssd1327_write_bus_cmd(dev, SSD1327_SET_FUNCTION_A, &buf, 1)) {
		return -EIO;
	}

	return 0;
}

static int ssd1327_resume(const struct device *dev)
{
	return ssd1327_write_bus_cmd(dev, SSD1327_DISPLAY_ON, NULL, 0);
}

static int ssd1327_suspend(const struct device *dev)
{
	return ssd1327_write_bus_cmd(dev, SSD1327_DISPLAY_OFF, NULL, 0);
}

static int ssd1327_set_display(const struct device *dev)
{
	const struct ssd1327_config *config = dev->config;
	uint8_t x_position[] = {
		0,
		config->width - 1
	};
	uint8_t y_position[] = {
		0,
		config->height - 1
	};

	if (ssd1327_write_bus_cmd(dev, SSD1327_SET_COLUMN_ADDR, x_position, sizeof(x_position))) {
		return -EIO;
	}
	if (ssd1327_write_bus_cmd(dev, SSD1327_SET_ROW_ADDR, y_position, sizeof(y_position))) {
		return -EIO;
	}
	if (ssd1327_write_bus_cmd(dev, SSD1327_SET_SEGMENT_MAP_REMAPED, &config->remap_value, 1)) {
		return -EIO;
	}

	return 0;
}

static int ssd1327_write(const struct device *dev, const uint16_t x, const uint16_t y,
			 const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct ssd1327_config *config = dev->config;
	struct display_buffer_descriptor mipi_desc;
	int err;
	size_t buf_len;
	uint8_t x_position[] = { x, x + desc->width - 1 };
	uint8_t y_position[] = { y, y + desc->height - 1 };

	if (desc->pitch < desc->width) {
		LOG_ERR("Pitch is smaller than width");
		return -1;
	}
	mipi_desc.pitch = desc->pitch;

	/* Following the datasheet, in the GDDRAM, two segment are split in one register */
	buf_len = MIN(desc->buf_size, desc->height * desc->width / 2);
	if (buf == NULL || buf_len == 0U) {
		LOG_ERR("Display buffer is not available");
		return -1;
	}
	mipi_desc.buf_size = buf_len;

	if (desc->pitch > desc->width) {
		LOG_ERR("Unsupported mode");
		return -1;
	}

	if ((y & 0x7) != 0U) {
		LOG_ERR("Unsupported origin");
		return -1;
	}
	mipi_desc.height = desc->height;
	mipi_desc.width = desc->width;

	LOG_DBG("x %u, y %u, pitch %u, width %u, height %u, buf_len %u", x, y, desc->pitch,
		desc->width, desc->height, buf_len);

	err = ssd1327_write_bus_cmd(dev, SSD1327_SET_COLUMN_ADDR, x_position, sizeof(x_position));
	if (err) {
		return err;
	}

	err = ssd1327_write_bus_cmd(dev, SSD1327_SET_ROW_ADDR, y_position, sizeof(y_position));
	if (err) {
		return err;
	}

	err = mipi_dbi_write_display(config->mipi_dev, &config->dbi_config, buf, &mipi_desc,
				     PIXEL_FORMAT_MONO10);
	if (err) {
		return err;
	}
	return mipi_dbi_release(config->mipi_dev, &config->dbi_config);
}

static int ssd1327_set_contrast(const struct device *dev, const uint8_t contrast)
{
	return ssd1327_write_bus_cmd(dev, SSD1327_SET_CONTRAST_CTRL, &contrast, 1);
}

static void ssd1327_get_capabilities(const struct device *dev,
				     struct display_capabilities *caps)
{
	const struct ssd1327_config *config = dev->config;

	memset(caps, 0, sizeof(struct display_capabilities));
	caps->x_resolution = config->width;
	caps->y_resolution = config->height;
	caps->supported_pixel_formats = PIXEL_FORMAT_MONO10;
	caps->current_pixel_format = PIXEL_FORMAT_MONO10;
	caps->screen_info = SCREEN_INFO_MONO_VTILED;
}

static int ssd1327_set_pixel_format(const struct device *dev,
				    const enum display_pixel_format pf)
{
	if (pf == PIXEL_FORMAT_MONO10) {
		return 0;
	}
	LOG_ERR("Unsupported pixel format");
	return -ENOTSUP;
}

static int ssd1327_init_device(const struct device *dev)
{
	const struct ssd1327_config *config = dev->config;
	uint8_t buf;

	/* Turn display off */
	if (ssd1327_suspend(dev)) {
		return -EIO;
	}

	if (ssd1327_set_display(dev)) {
		return -EIO;
	}

	if (ssd1327_set_contrast(dev, CONFIG_SSD1327_DEFAULT_CONTRAST)) {
		return -EIO;
	}

	if (ssd1327_set_hardware_config(dev)) {
		return -EIO;
	}

	buf = (config->color_inversion ?
	       SSD1327_SET_REVERSE_DISPLAY : SSD1327_SET_NORMAL_DISPLAY);
	if (ssd1327_write_bus_cmd(dev, SSD1327_SET_ENTIRE_DISPLAY_OFF, &buf, 1)) {
		return -EIO;
	}

	if (ssd1327_set_timing_setting(dev)) {
		return -EIO;
	}

	if (ssd1327_resume(dev)) {
		return -EIO;
	}

	return 0;
}

static int ssd1327_init(const struct device *dev)
{
	const struct ssd1327_config *config = dev->config;

	LOG_DBG("Initializing device");

	if (!device_is_ready(config->mipi_dev)) {
		LOG_ERR("MIPI Device not ready!");
		return -EINVAL;
	}

	if (mipi_dbi_reset(config->mipi_dev, SSD1327_RESET_DELAY)) {
		LOG_ERR("Failed to reset device!");
		return -EIO;
	}
	k_msleep(SSD1327_RESET_DELAY);

	if (ssd1327_init_device(dev)) {
		LOG_ERR("Failed to initialize device!");
		return -EIO;
	}

	return 0;
}

static DEVICE_API(display, ssd1327_driver_api) = {
	.blanking_on = ssd1327_suspend,
	.blanking_off = ssd1327_resume,
	.write = ssd1327_write,
	.set_contrast = ssd1327_set_contrast,
	.get_capabilities = ssd1327_get_capabilities,
	.set_pixel_format = ssd1327_set_pixel_format,
};

#define SSD1327_DEFINE(node_id)                                                                    \
	static struct ssd1327_data data##node_id;                                                  \
	static const struct ssd1327_config config##node_id = {                                     \
		.mipi_dev = DEVICE_DT_GET(DT_PARENT(node_id)),                                     \
		.dbi_config = { .mode = MIPI_DBI_MODE_SPI_4WIRE,                                   \
				.config = MIPI_DBI_SPI_CONFIG_DT(node_id,                          \
							SPI_OP_MODE_MASTER | SPI_WORD_SET(8) |     \
							SPI_HOLD_ON_CS | SPI_LOCK_ON, 0),          \
				},                                                                 \
		.height = DT_PROP(node_id, height),                                                \
		.width = DT_PROP(node_id, width),                                                  \
		.oscillator_freq = DT_PROP(node_id, oscillator_freq),                              \
		.display_offset = DT_PROP(node_id, display_offset),                                \
		.start_line = DT_PROP(node_id, start_line),                                        \
		.multiplex_ratio = DT_PROP(node_id, multiplex_ratio),                              \
		.prechargep = DT_PROP(node_id, prechargep),                                        \
		.remap_value = DT_PROP(node_id, remap_value),                                      \
		.color_inversion = DT_PROP(node_id, inversion_on),                                 \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(node_id, ssd1327_init, NULL, &data##node_id, &config##node_id,            \
			 POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY, &ssd1327_driver_api);

DT_FOREACH_STATUS_OKAY(solomon_ssd1327fb, SSD1327_DEFINE)
