/*
 * Copyright (c) 2025 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ssd135x, CONFIG_DISPLAY_LOG_LEVEL);

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/kernel.h>

#define SSD135X_DISPLAY_OFF         0xAE
#define SSD135X_DISPLAY_ON          0xAF
#define SSD135X_SET_NORMAL_DISPLAY  0xA6
#define SSD135X_SET_REVERSE_DISPLAY 0xA7

#define SSD135X_SET_COLUMN_ADDR 0x15
#define SSD135X_SET_ROW_ADDR    0x75

#define SSD135X_SET_DISPLAY_START_LINE 0xA1
#define SSD135X_SET_DISPLAY_OFFSET     0xA2
#define SSD135X_SET_MULTIPLEX_RATIO    0xCA
#define SSD135X_SET_PHASE_LENGTH       0xB1
#define SSD135X_SET_OSC_FREQ           0xB3
#define SSD135X_SET_PRECHARGE_V        0xBB
#define SSD135X_SET_VCOMH              0xBE
#define SSD135X_SET_CURRENT_ATT        0xC7
#define SSD135X_SET_PRECHARGE_P        0xB6
#define SSD135X_SET_REMAP              0xA0
#define SSD135X_STOP_SCROLL            0x9E
#define SSD135X_SET_LINEAR_LUT         0xB9

#define SSD135X_CONTRAST 0xC1

#define SSD135X_SET_LOCK 0xFD
#define SSD135X_UNLOCK_1 0x12
#define SSD135X_UNLOCK_2 0xB1
#define SSD135X_LOCK_1   0x16
#define SSD135X_LOCK_2   0xB0

#define SSD135X_WRITE 0x5C

#define SSD135X_RESET_DELAY 10

struct ssd135x_config {
	const struct device *mipi_dev;
	const struct mipi_dbi_config dbi_config;
	uint16_t height;
	uint16_t width;
	uint8_t start_line;
	uint8_t display_offset;
	uint8_t multiplex_ratio;
	uint8_t phase_length;
	uint8_t oscillator_freq;
	uint8_t precharge_voltage;
	uint8_t precharge_time;
	uint8_t vcomh_voltage;
	uint8_t current_att;
	uint8_t remap_value;
	uint8_t column_offset;
	bool color_inversion;
	bool ssd1357;
};

static inline int ssd135x_set_hardware_config(const struct device *dev)
{
	const struct ssd135x_config *config = dev->config;
	int err;
	uint8_t tmp;

	tmp = SSD135X_UNLOCK_1;
	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config, SSD135X_SET_LOCK, &tmp,
				     1);
	if (err < 0) {
		return err;
	}
	if (!config->ssd1357) {
		tmp = SSD135X_UNLOCK_2;
		err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config,
					     SSD135X_SET_LOCK, &tmp, 1);
		if (err < 0) {
			return err;
		}
	}
	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config, SSD135X_SET_OSC_FREQ,
				     &config->oscillator_freq, 1);
	if (err < 0) {
		return err;
	}
	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config,
				     SSD135X_SET_MULTIPLEX_RATIO, &config->multiplex_ratio, 1);
	if (err < 0) {
		return err;
	}
	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config,
				     SSD135X_SET_DISPLAY_OFFSET, &config->display_offset, 1);
	if (err < 0) {
		return err;
	}
	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config, SSD135X_SET_REMAP,
				     &config->remap_value, 1);
	if (err < 0) {
		return err;
	}
	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config,
				     SSD135X_SET_DISPLAY_START_LINE, &config->start_line, 1);
	if (err < 0) {
		return err;
	}
	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config,
				     SSD135X_SET_PHASE_LENGTH, &config->phase_length, 1);
	if (err < 0) {
		return err;
	}
	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config, SSD135X_SET_VCOMH,
				     &config->vcomh_voltage, 1);
	if (err < 0) {
		return err;
	}
	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config, SSD135X_SET_CURRENT_ATT,
				     &config->current_att, 1);
	if (err < 0) {
		return err;
	}
	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config, SSD135X_SET_PRECHARGE_V,
				     &config->precharge_voltage, 1);
	if (err < 0) {
		return err;
	}
	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config, SSD135X_SET_PRECHARGE_P,
				     &config->precharge_time, 1);
	if (err < 0) {
		return err;
	}

	return mipi_dbi_command_write(config->mipi_dev, &config->dbi_config, SSD135X_STOP_SCROLL,
				      NULL, 0);
}

static int ssd135x_resume(const struct device *dev)
{
	const struct ssd135x_config *config = dev->config;
	int err;

	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config, SSD135X_DISPLAY_ON,
				     NULL, 0);
	if (err < 0) {
		return err;
	}
	return mipi_dbi_release(config->mipi_dev, &config->dbi_config);
}

static int ssd135x_suspend(const struct device *dev)
{
	const struct ssd135x_config *config = dev->config;
	int err;

	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config, SSD135X_DISPLAY_OFF,
				     NULL, 0);
	if (err < 0) {
		return err;
	}
	return mipi_dbi_release(config->mipi_dev, &config->dbi_config);
}

static int ssd135x_write(const struct device *dev, const uint16_t x, const uint16_t y,
			 const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct ssd135x_config *config = dev->config;
	int err;
	size_t buf_len;
	struct display_buffer_descriptor mipi_desc = *desc;
	/* The window commands only care about the first 7-bits (0-127: 128 positions) */
	uint8_t x_position[] = {(x + config->column_offset) & 0x7F,
		(x + desc->width - 1 + config->column_offset) & 0x7F};
	uint8_t y_position[] = {y & 0x7F, (y + desc->height - 1) & 0x7F};

	if (desc->pitch != desc->width) {
		LOG_ERR("Pitch is not width");
		return -EINVAL;
	}

	/* Following the datasheet, two segment are split in one register */
	buf_len = MIN(desc->buf_size, desc->height * desc->width * 2);
	if (buf == NULL || buf_len == 0U) {
		LOG_ERR("Display buffer is not available");
		return -EINVAL;
	}

	LOG_DBG("x %u, y %u, pitch %u, width %u, height %u, buf_len %u", x, y, desc->pitch,
		desc->width, desc->height, buf_len);

	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config, SSD135X_SET_COLUMN_ADDR,
				     x_position, 2);
	if (err < 0) {
		return err;
	}

	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config, SSD135X_SET_ROW_ADDR,
				     y_position, 2);
	if (err < 0) {
		return err;
	}

	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config, SSD135X_WRITE, NULL, 0);
	if (err < 0) {
		return err;
	}

	err = mipi_dbi_write_display(config->mipi_dev, &config->dbi_config, buf, &mipi_desc,
				     PIXEL_FORMAT_RGB_565);
	if (err < 0) {
		return err;
	}
	return mipi_dbi_release(config->mipi_dev, &config->dbi_config);
}

static int ssd135x_set_contrast(const struct device *dev, const uint8_t contrast)
{
	const struct ssd135x_config *config = dev->config;
	uint8_t tmp[3];

	tmp[0] = (contrast * CONFIG_SSD135X_CONTRASTA) / 0xFF;
	tmp[1] = (contrast * CONFIG_SSD135X_CONTRASTB) / 0xFF;
	tmp[2] = (contrast * CONFIG_SSD135X_CONTRASTC) / 0xFF;
	return mipi_dbi_command_write(config->mipi_dev, &config->dbi_config, SSD135X_CONTRAST, tmp,
				      3);
}

static void ssd135x_get_capabilities(const struct device *dev, struct display_capabilities *caps)
{
	const struct ssd135x_config *config = dev->config;

	memset(caps, 0, sizeof(struct display_capabilities));
	caps->x_resolution = config->width;
	caps->y_resolution = config->height;
	caps->supported_pixel_formats = PIXEL_FORMAT_RGB_565;
	caps->current_pixel_format = PIXEL_FORMAT_RGB_565;
	caps->screen_info = 0;
}

static int ssd135x_set_pixel_format(const struct device *dev, const enum display_pixel_format pf)
{
	if (pf == PIXEL_FORMAT_RGB_565) {
		return 0;
	}
	LOG_ERR("Unsupported pixel format");
	return -ENOTSUP;
}

static int ssd135x_init_device(const struct device *dev)
{
	const struct ssd135x_config *config = dev->config;
	int err;

	/* Turn display off */
	err = ssd135x_suspend(dev);
	if (err < 0) {
		return err;
	}

	err = ssd135x_set_hardware_config(dev);
	if (err < 0) {
		return err;
	}

	err = ssd135x_set_contrast(dev, CONFIG_SSD135X_DEFAULT_CONTRAST);
	if (err < 0) {
		return err;
	}

	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config,
				     config->color_inversion ? SSD135X_SET_REVERSE_DISPLAY
							     : SSD135X_SET_NORMAL_DISPLAY,
				     NULL, 0);
	if (err < 0) {
		return err;
	}

	err = ssd135x_resume(dev);
	if (err < 0) {
		return err;
	}

	return mipi_dbi_release(config->mipi_dev, &config->dbi_config);
}

static int ssd135x_init(const struct device *dev)
{
	const struct ssd135x_config *config = dev->config;
	int err;

	LOG_DBG("Initializing device");

	if (!device_is_ready(config->mipi_dev)) {
		LOG_ERR("MIPI Device not ready!");
		return -EINVAL;
	}

	err = mipi_dbi_reset(config->mipi_dev, SSD135X_RESET_DELAY);
	if (err < 0) {
		LOG_ERR("Failed to reset device!");
		return err;
	}

	err = ssd135x_init_device(dev);
	if (err < 0) {
		LOG_ERR("Failed to initialize device! %d", err);
		return err;
	}

	return 0;
}

static DEVICE_API(display, ssd135x_driver_api) = {
	.blanking_on = ssd135x_suspend,
	.blanking_off = ssd135x_resume,
	.write = ssd135x_write,
	.set_contrast = ssd135x_set_contrast,
	.get_capabilities = ssd135x_get_capabilities,
	.set_pixel_format = ssd135x_set_pixel_format,
};

#define SSD135X_WORD_SIZE(inst)                                                                    \
	((DT_STRING_UPPER_TOKEN(inst, mipi_mode) == MIPI_DBI_MODE_SPI_4WIRE) ? SPI_WORD_SET(8)     \
									     : SPI_WORD_SET(9))

#define SSD135X_DEFINE_MIPI(node_id)                                                               \
	static const struct ssd135x_config config##node_id = {                                     \
		.mipi_dev = DEVICE_DT_GET(DT_PARENT(node_id)),                                     \
		.dbi_config = MIPI_DBI_CONFIG_DT(                                                  \
			node_id, SSD135X_WORD_SIZE(node_id) | SPI_OP_MODE_MASTER, 0),              \
		.height = DT_PROP(node_id, height),                                                \
		.width = DT_PROP(node_id, width),                                                  \
		.display_offset = DT_PROP(node_id, display_offset),                                \
		.start_line = DT_PROP(node_id, start_line),                                        \
		.multiplex_ratio = DT_PROP(node_id, multiplex_ratio),                              \
		.phase_length = DT_PROP(node_id, phase_length),                                    \
		.oscillator_freq = DT_PROP(node_id, oscillator_freq),                              \
		.precharge_time = DT_PROP(node_id, precharge_time),                                \
		.precharge_voltage = DT_PROP(node_id, precharge_voltage),                          \
		.vcomh_voltage = DT_PROP(node_id, vcomh_voltage),                                  \
		.current_att = DT_PROP(node_id, current_att),                                      \
		.color_inversion = DT_PROP(node_id, inversion_on),                                 \
		.remap_value = DT_PROP(node_id, remap_value),                                      \
		.column_offset = DT_PROP(node_id, column_offset),                                  \
		.ssd1357 = DT_NODE_HAS_COMPAT(node_id, solomon_ssd1357),                           \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(node_id, ssd135x_init, NULL, NULL, &config##node_id,                      \
			 POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY, &ssd135x_driver_api);

DT_FOREACH_STATUS_OKAY(solomon_ssd1351, SSD135X_DEFINE_MIPI)
DT_FOREACH_STATUS_OKAY(solomon_ssd1357, SSD135X_DEFINE_MIPI)
