/*
 * Copyright (c) 2025 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ssd1331, CONFIG_DISPLAY_LOG_LEVEL);

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/kernel.h>

#define SSD1331_DISPLAY_OFF         0xAE
#define SSD1331_DISPLAY_ON          0xAF
#define SSD1331_SET_NORMAL_DISPLAY  0xA4
#define SSD1331_SET_REVERSE_DISPLAY 0xA7

#define SSD1331_SET_COLUMN_ADDR 0x15
#define SSD1331_SET_ROW_ADDR    0x75

#define SSD1331_SET_DISPLAY_START_LINE 0xA1
#define SSD1331_SET_DISPLAY_OFFSET     0xA2
#define SSD1331_SET_MULTIPLEX_RATIO    0xA8
#define SSD1331_SET_PHASE_LENGTH       0xB1
#define SSD1331_SET_OSC_FREQ           0xB3
#define SSD1331_SET_PRECHARGE_A        0x8A
#define SSD1331_SET_PRECHARGE_B        0x8B
#define SSD1331_SET_PRECHARGE_C        0x8C
#define SSD1331_SET_PRECHARGE_V        0xBB
#define SSD1331_SET_VCOMH              0xBE
#define SSD1331_SET_CURRENT_ATT        0x87
#define SSD1331_SET_REMAP              0xA0
#define SSD1331_DISABLE_SCROLL         0x2E

#define SSD1331_SET_EXTERNAL_SUPPLY 0xAD
#define SSD1331_EXTERNAL_SUPPLY     0x8E

#define SSD1331_SET_POWER_SAVE 0xB0
#define SSD1331_POWER_SAVE     0x1A
#define SSD1331_NOT_POWER_SAVE 0x0B

#define SSD1331_CONTRASTA 0x81
#define SSD1331_CONTRASTB 0x82
#define SSD1331_CONTRASTC 0x83

#define SSD1331_RESET_DELAY 10

struct ssd1331_config {
	const struct device *mipi_dev;
	const struct mipi_dbi_config dbi_config;
	uint16_t height;
	uint16_t width;
	uint8_t start_line;
	uint8_t display_offset;
	uint8_t multiplex_ratio;
	uint8_t phase_length;
	uint8_t oscillator_freq;
	uint8_t precharge_time_a;
	uint8_t precharge_time_b;
	uint8_t precharge_time_c;
	uint8_t precharge_voltage;
	uint8_t vcomh_voltage;
	uint8_t current_att;
	uint8_t remap_value;
	bool power_save;
	bool color_inversion;
};

/* SSD1331 doesn't follow typical DBI behavior with D/C pin so we do this to make it fit in DBI */
static inline int ssd1331_write_command(const struct device *dev, uint8_t cmd, const uint8_t *buf,
					size_t len)
{
	const struct ssd1331_config *config = dev->config;
	int err;

	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config, cmd, NULL, 0);
	if (err < 0) {
		return err;
	}
	for (int x = 0; x < len; x++) {
		err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config, buf[x], NULL,
					     0);
		if (err < 0) {
			return err;
		}
	}

	return err;
}

static inline int ssd1331_set_hardware_config(const struct device *dev)
{
	const struct ssd1331_config *config = dev->config;
	int err;
	uint8_t tmp;

	err = ssd1331_write_command(dev, SSD1331_SET_REMAP, &config->remap_value, 1);
	if (err < 0) {
		return err;
	}
	err = ssd1331_write_command(dev, SSD1331_SET_DISPLAY_START_LINE, &config->start_line, 1);
	if (err < 0) {
		return err;
	}
	err = ssd1331_write_command(dev, SSD1331_SET_DISPLAY_OFFSET, &config->display_offset, 1);
	if (err < 0) {
		return err;
	}
	err = ssd1331_write_command(dev, SSD1331_SET_MULTIPLEX_RATIO, &config->multiplex_ratio, 1);
	if (err < 0) {
		return err;
	}
	tmp = SSD1331_EXTERNAL_SUPPLY;
	err = ssd1331_write_command(dev, SSD1331_SET_EXTERNAL_SUPPLY, &tmp, 1);
	if (err < 0) {
		return err;
	}
	tmp = config->power_save ? SSD1331_POWER_SAVE : SSD1331_NOT_POWER_SAVE;
	err = ssd1331_write_command(dev, SSD1331_SET_POWER_SAVE, &tmp, 1);
	if (err < 0) {
		return err;
	}
	err = ssd1331_write_command(dev, SSD1331_SET_PHASE_LENGTH, &config->phase_length, 1);
	if (err < 0) {
		return err;
	}
	err = ssd1331_write_command(dev, SSD1331_SET_OSC_FREQ, &config->oscillator_freq, 1);
	if (err < 0) {
		return err;
	}
	err = ssd1331_write_command(dev, SSD1331_SET_PRECHARGE_A, &config->precharge_time_a, 1);
	if (err < 0) {
		return err;
	}
	err = ssd1331_write_command(dev, SSD1331_SET_PRECHARGE_B, &config->precharge_time_b, 1);
	if (err < 0) {
		return err;
	}
	err = ssd1331_write_command(dev, SSD1331_SET_PRECHARGE_C, &config->precharge_time_c, 1);
	if (err < 0) {
		return err;
	}
	err = ssd1331_write_command(dev, SSD1331_SET_PRECHARGE_V, &config->precharge_voltage, 1);
	if (err < 0) {
		return err;
	}
	err = ssd1331_write_command(dev, SSD1331_SET_VCOMH, &config->vcomh_voltage, 1);
	if (err < 0) {
		return err;
	}
	err = ssd1331_write_command(dev, SSD1331_SET_CURRENT_ATT, &config->current_att, 1);
	if (err < 0) {
		return err;
	}
	return ssd1331_write_command(dev, SSD1331_DISABLE_SCROLL, NULL, 0);
}

static int ssd1331_resume(const struct device *dev)
{
	const struct ssd1331_config *config = dev->config;
	int err;

	err = ssd1331_write_command(dev, SSD1331_DISPLAY_ON, NULL, 0);
	if (err < 0) {
		return err;
	}
	return mipi_dbi_release(config->mipi_dev, &config->dbi_config);
}

static int ssd1331_suspend(const struct device *dev)
{
	const struct ssd1331_config *config = dev->config;
	int err;

	err = ssd1331_write_command(dev, SSD1331_DISPLAY_OFF, NULL, 0);
	if (err < 0) {
		return err;
	}
	return mipi_dbi_release(config->mipi_dev, &config->dbi_config);
}

static int ssd1331_write(const struct device *dev, const uint16_t x, const uint16_t y,
			 const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct ssd1331_config *config = dev->config;
	int err;
	size_t buf_len;
	struct display_buffer_descriptor mipi_desc = *desc;
	uint8_t x_position[] = {x, x + desc->width - 1};
	uint8_t y_position[] = {y, y + desc->height - 1};

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

	err = ssd1331_write_command(dev, SSD1331_SET_COLUMN_ADDR, x_position, 2);
	if (err < 0) {
		return err;
	}

	err = ssd1331_write_command(dev, SSD1331_SET_ROW_ADDR, y_position, 2);
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

static int ssd1331_set_contrast(const struct device *dev, const uint8_t contrast)
{
	int err;
	uint8_t tmp;

	tmp = (contrast * CONFIG_SSD1331_CONTRASTA) / 0xFF;
	err = ssd1331_write_command(dev, SSD1331_CONTRASTA, &tmp, 1);
	if (err < 0) {
		return err;
	}
	tmp = (contrast * CONFIG_SSD1331_CONTRASTB) / 0xFF;
	err = ssd1331_write_command(dev, SSD1331_CONTRASTB, &tmp, 1);
	if (err < 0) {
		return err;
	}
	tmp = (contrast * CONFIG_SSD1331_CONTRASTC) / 0xFF;
	return ssd1331_write_command(dev, SSD1331_CONTRASTC, &tmp, 1);
}

static void ssd1331_get_capabilities(const struct device *dev, struct display_capabilities *caps)
{
	const struct ssd1331_config *config = dev->config;

	memset(caps, 0, sizeof(struct display_capabilities));
	caps->x_resolution = config->width;
	caps->y_resolution = config->height;
	caps->supported_pixel_formats = PIXEL_FORMAT_RGB_565;
	caps->current_pixel_format = PIXEL_FORMAT_RGB_565;
	caps->screen_info = 0;
}

static int ssd1331_set_pixel_format(const struct device *dev, const enum display_pixel_format pf)
{
	if (pf == PIXEL_FORMAT_RGB_565) {
		return 0;
	}
	LOG_ERR("Unsupported pixel format");
	return -ENOTSUP;
}

static int ssd1331_init_device(const struct device *dev)
{
	const struct ssd1331_config *config = dev->config;
	int err;

	/* Turn display off */
	err = ssd1331_suspend(dev);
	if (err < 0) {
		return err;
	}

	err = ssd1331_set_hardware_config(dev);
	if (err < 0) {
		return err;
	}

	err = ssd1331_set_contrast(dev, CONFIG_SSD1331_DEFAULT_CONTRAST);
	if (err < 0) {
		return err;
	}

	err = ssd1331_write_command(dev,
				    config->color_inversion ? SSD1331_SET_REVERSE_DISPLAY
							    : SSD1331_SET_NORMAL_DISPLAY,
				    NULL, 0);
	if (err < 0) {
		return err;
	}

	err = ssd1331_resume(dev);
	if (err < 0) {
		return err;
	}

	return mipi_dbi_release(config->mipi_dev, &config->dbi_config);
}

static int ssd1331_init(const struct device *dev)
{
	const struct ssd1331_config *config = dev->config;
	int err;

	LOG_DBG("Initializing device");

	if (!device_is_ready(config->mipi_dev)) {
		LOG_ERR("MIPI Device not ready!");
		return -EINVAL;
	}

	err = mipi_dbi_reset(config->mipi_dev, SSD1331_RESET_DELAY);
	if (err < 0) {
		LOG_ERR("Failed to reset device!");
		return err;
	}

	err = ssd1331_init_device(dev);
	if (err < 0) {
		LOG_ERR("Failed to initialize device! %d", err);
		return err;
	}

	return 0;
}

static DEVICE_API(display, ssd1331_driver_api) = {
	.blanking_on = ssd1331_suspend,
	.blanking_off = ssd1331_resume,
	.write = ssd1331_write,
	.set_contrast = ssd1331_set_contrast,
	.get_capabilities = ssd1331_get_capabilities,
	.set_pixel_format = ssd1331_set_pixel_format,
};

#define SSD1331_WORD_SIZE(inst)                                                                    \
	((DT_STRING_UPPER_TOKEN(inst, mipi_mode) == MIPI_DBI_MODE_SPI_4WIRE) ? SPI_WORD_SET(8)     \
									     : SPI_WORD_SET(9))

#define SSD1331_DEFINE_MIPI(node_id)                                                               \
	static const struct ssd1331_config config##node_id = {                                     \
		.mipi_dev = DEVICE_DT_GET(DT_PARENT(node_id)),                                     \
		.dbi_config = MIPI_DBI_CONFIG_DT(                                                  \
			node_id, SSD1331_WORD_SIZE(node_id) | SPI_OP_MODE_MASTER, 0),              \
		.height = DT_PROP(node_id, height),                                                \
		.width = DT_PROP(node_id, width),                                                  \
		.display_offset = DT_PROP(node_id, display_offset),                                \
		.start_line = DT_PROP(node_id, start_line),                                        \
		.multiplex_ratio = DT_PROP(node_id, multiplex_ratio),                              \
		.phase_length = DT_PROP(node_id, phase_length),                                    \
		.oscillator_freq = DT_PROP(node_id, oscillator_freq),                              \
		.power_save = DT_PROP(node_id, power_save),                                        \
		.precharge_time_a = DT_PROP(node_id, precharge_time_a),                            \
		.precharge_time_b = DT_PROP(node_id, precharge_time_b),                            \
		.precharge_time_c = DT_PROP(node_id, precharge_time_c),                            \
		.precharge_voltage = DT_PROP(node_id, precharge_voltage),                          \
		.vcomh_voltage = DT_PROP(node_id, vcomh_voltage),                                  \
		.current_att = DT_PROP(node_id, current_att),                                      \
		.color_inversion = DT_PROP(node_id, inversion_on),                                 \
		.remap_value = DT_PROP(node_id, remap_value),                                      \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(node_id, ssd1331_init, NULL, NULL, &config##node_id,                      \
			 POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY, &ssd1331_driver_api);

DT_FOREACH_STATUS_OKAY(solomon_ssd1331, SSD1331_DEFINE_MIPI)
