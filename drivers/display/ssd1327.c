/*
 * Copyright (c) 2024 Savoir-faire Linux
 * Copyright (c) 2025 MASSDRIVER EI (massdriver.space)
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
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/kernel.h>

#include "ssd1327_regs.h"

#define SSD1327_ENABLE_VDD         0x01
#define SSD1327_UNLOCK_COMMAND     0x12
#define SSD1327_MAXIMUM_CMD_LENGTH 16

typedef int (*ssd1327_write_bus_cmd_fn)(const struct device *dev, const uint8_t cmd,
					 const uint8_t *data, size_t len);
typedef int (*ssd1327_write_pixels_fn)(const struct device *dev, const uint8_t *buf,
					uint32_t pixel_count,
					const struct display_buffer_descriptor *desc);

struct ssd1327_config {
	struct i2c_dt_spec i2c;
	ssd1327_write_bus_cmd_fn write_cmd;
	ssd1327_write_pixels_fn write_pixels;
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
	uint8_t phase_length;
	uint8_t function_selection_b;
	uint8_t precharge_voltage;
	uint8_t vcomh_voltage;
	const uint8_t *grayscale_table;
	bool color_inversion;
	uint8_t *conversion_buf;
	size_t conversion_buf_size;
};

struct ssd1327_data {
	uint8_t contrast;
	uint8_t scan_mode;
};

static inline int ssd1327_write_bus_cmd_mipi(const struct device *dev, const uint8_t cmd,
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

static inline int ssd1327_write_bus_cmd_i2c(const struct device *dev, const uint8_t cmd,
					    const uint8_t *data, size_t len)
{
	const struct ssd1327_config *config = dev->config;
	uint8_t buf[SSD1327_MAXIMUM_CMD_LENGTH];

	if (len > SSD1327_MAXIMUM_CMD_LENGTH - 1) {
		return -EINVAL;
	}

	buf[0] = cmd;
	memcpy(&(buf[1]), data, len);

	return i2c_burst_write_dt(&config->i2c, SSD1327_CONTROL_ALL_BYTES_CMD, buf, len + 1);
}

static inline int ssd1327_set_timing_setting(const struct device *dev)
{
	const struct ssd1327_config *config = dev->config;
	uint8_t buf = SSD1327_UNLOCK_COMMAND;
	int err;

	err = config->write_cmd(dev, SSD1327_SET_PHASE_LENGTH, &config->phase_length, 1);
	if (err < 0) {
		return err;
	}
	err = config->write_cmd(dev, SSD1327_SET_OSC_FREQ, &config->oscillator_freq, 1);
	if (err < 0) {
		return err;
	}
	err = config->write_cmd(dev, SSD1327_SET_PRECHARGE_PERIOD, &config->prechargep, 1);
	if (err < 0) {
		return err;
	}
	err = config->write_cmd(dev, SSD1327_LINEAR_LUT, NULL, 0);
	if (err < 0) {
		return err;
	}
	if (config->grayscale_table != NULL) {
		err = config->write_cmd(dev, SSD1327_SET_LUT, config->grayscale_table,
					SSD1327_SET_LUT_COUNT);
		if (err < 0) {
			return err;
		}
	}
	err = config->write_cmd(dev, SSD1327_SET_PRECHARGE_VOLTAGE, &config->precharge_voltage, 1);
	if (err < 0) {
		return err;
	}
	err = config->write_cmd(dev, SSD1327_SET_VCOMH, &config->vcomh_voltage, 1);
	if (err < 0) {
		return err;
	}
	err = config->write_cmd(dev, SSD1327_FUNCTION_SELECTION_B, &config->function_selection_b,
				1);
	if (err < 0) {
		return err;
	}
	return config->write_cmd(dev, SSD1327_SET_COMMAND_LOCK, &buf, 1);
}

static inline int ssd1327_set_hardware_config(const struct device *dev)
{
	const struct ssd1327_config *config = dev->config;
	uint8_t buf;
	int err;

	err = config->write_cmd(dev, SSD1327_SET_DISPLAY_START_LINE, &config->start_line, 1);
	if (err < 0) {
		return err;
	}
	err = config->write_cmd(dev, SSD1327_SET_DISPLAY_OFFSET, &config->display_offset, 1);
	if (err < 0) {
		return err;
	}
	err = config->write_cmd(dev, SSD1327_SET_NORMAL_DISPLAY, NULL, 0);
	if (err < 0) {
		return err;
	}
	err = config->write_cmd(dev, SSD1327_SET_SEGMENT_MAP_REMAPED, &config->remap_value, 1);
	if (err < 0) {
		return err;
	}
	err = config->write_cmd(dev, SSD1327_SET_MULTIPLEX_RATIO, &config->multiplex_ratio, 1);
	if (err < 0) {
		return err;
	}
	buf = SSD1327_ENABLE_VDD;
	return config->write_cmd(dev, SSD1327_SET_FUNCTION_A, &buf, 1);
}

static int ssd1327_resume(const struct device *dev)
{
	const struct ssd1327_config *config = dev->config;

	return config->write_cmd(dev, SSD1327_DISPLAY_ON, NULL, 0);
}

static int ssd1327_suspend(const struct device *dev)
{
	const struct ssd1327_config *config = dev->config;

	return config->write_cmd(dev, SSD1327_DISPLAY_OFF, NULL, 0);
}

static int ssd1327_set_display(const struct device *dev)
{
	const struct ssd1327_config *config = dev->config;
	int err;
	uint8_t x_position[] = {0, config->width - 1};
	uint8_t y_position[] = {0, config->height - 1};

	err = config->write_cmd(dev, SSD1327_SET_COLUMN_ADDR, x_position, sizeof(x_position));
	if (err < 0) {
		return err;
	}
	err = config->write_cmd(dev, SSD1327_SET_ROW_ADDR, y_position, sizeof(y_position));
	if (err < 0) {
		return err;
	}
	return config->write_cmd(dev, SSD1327_SET_SEGMENT_MAP_REMAPED, &config->remap_value, 1);
}

/* Convert what the conversion buffer can hold to pixelx (3:0) and pixelx+1 (7:4) */
static int ssd1327_convert_L_8(const struct device *dev, const uint8_t *buf, int cur_offset,
			       uint32_t pixel_count)
{
	const struct ssd1327_config *config = dev->config;
	int i = 0;

	for (; i / 2 < config->conversion_buf_size && pixel_count > cur_offset + i; i += 2) {
		config->conversion_buf[i / 2] = buf[cur_offset + i] >> 4;
		config->conversion_buf[i / 2] |= (buf[cur_offset + i + 1] >> 4) << 4;
	}
	return i;
}

#if DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(solomon_ssd1327fb, mipi_dbi)
static int ssd1327_write_pixels_mipi(const struct device *dev, const uint8_t *buf,
				     uint32_t pixel_count,
				     const struct display_buffer_descriptor *desc)
{
	const struct ssd1327_config *config = dev->config;
	struct display_buffer_descriptor mipi_desc;
	int ret, i;
	int total = 0;

	mipi_desc.pitch = desc->pitch;

	while (pixel_count > total) {
		i = ssd1327_convert_L_8(dev, buf, total, pixel_count);

		mipi_desc.buf_size = i / 2;
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
#endif

#if DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(solomon_ssd1327fb, i2c)
static int ssd1327_write_pixels_i2c(const struct device *dev, const uint8_t *buf,
				    uint32_t pixel_count,
				    const struct display_buffer_descriptor *desc)
{
	const struct ssd1327_config *config = dev->config;
	int ret, i;
	int total = 0;

	while (pixel_count > total) {
		i = ssd1327_convert_L_8(dev, buf, total, pixel_count);

		ret = i2c_burst_write_dt(&config->i2c, SSD1327_CONTROL_ALL_BYTES_DATA,
					 config->conversion_buf, i / 2);
		if (ret < 0) {
			return ret;
		}
		total += i;
	}
	return 0;
}
#endif

static int ssd1327_write(const struct device *dev, const uint16_t x, const uint16_t y,
			 const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct ssd1327_config *config = dev->config;
	int err;
	size_t buf_len;
	int32_t pixel_count = desc->width * desc->height;
	uint8_t x_position[] = {x / 2, (x + desc->width - 1) / 2};
	uint8_t y_position[] = {y, y + desc->height - 1};

	if (desc->pitch != desc->width) {
		LOG_ERR("Pitch is not width");
		return -EINVAL;
	}

	/* Following the datasheet, in the GDDRAM, two segment are split in one register */
	buf_len = MIN(desc->buf_size, desc->height * desc->width / 2);
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

	err = config->write_cmd(dev, SSD1327_SET_COLUMN_ADDR, x_position, sizeof(x_position));
	if (err) {
		return err;
	}

	err = config->write_cmd(dev, SSD1327_SET_ROW_ADDR, y_position, sizeof(y_position));
	if (err) {
		return err;
	}

	return config->write_pixels(dev, buf, pixel_count, desc);
}

static int ssd1327_set_contrast(const struct device *dev, const uint8_t contrast)
{
	const struct ssd1327_config *config = dev->config;

	return config->write_cmd(dev, SSD1327_SET_CONTRAST_CTRL, &contrast, 1);
}

static void ssd1327_get_capabilities(const struct device *dev, struct display_capabilities *caps)
{
	const struct ssd1327_config *config = dev->config;

	memset(caps, 0, sizeof(struct display_capabilities));
	caps->x_resolution = config->width;
	caps->y_resolution = config->height;
	caps->supported_pixel_formats = PIXEL_FORMAT_L_8;
	caps->current_pixel_format = PIXEL_FORMAT_L_8;
	caps->screen_info = 0;
}

static int ssd1327_set_pixel_format(const struct device *dev, const enum display_pixel_format pf)
{
	if (pf == PIXEL_FORMAT_L_8) {
		return 0;
	}
	LOG_ERR("Unsupported pixel format");
	return -ENOTSUP;
}

static int ssd1327_init_device(const struct device *dev)
{
	const struct ssd1327_config *config = dev->config;
	uint8_t buf;
	int err;

	/* Turn display off */
	err = ssd1327_suspend(dev);
	if (err < 0) {
		return err;
	}

	err = ssd1327_set_display(dev);
	if (err < 0) {
		return err;
	}

	err = ssd1327_set_contrast(dev, CONFIG_SSD1327_DEFAULT_CONTRAST);
	if (err < 0) {
		return err;
	}

	err = ssd1327_set_hardware_config(dev);
	if (err < 0) {
		return err;
	}

	buf = (config->color_inversion ? SSD1327_SET_REVERSE_DISPLAY : SSD1327_SET_NORMAL_DISPLAY);
	err = config->write_cmd(dev, SSD1327_SET_ENTIRE_DISPLAY_OFF, &buf, 1);
	if (err < 0) {
		return err;
	}

	err = ssd1327_set_timing_setting(dev);
	if (err < 0) {
		return err;
	}

	err = ssd1327_resume(dev);
	if (err < 0) {
		return err;
	}

	return 0;
}

#if DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(solomon_ssd1327fb, mipi_dbi)
static int ssd1327_init(const struct device *dev)
{
	const struct ssd1327_config *config = dev->config;
	int err;

	LOG_DBG("Initializing device");

	if (!device_is_ready(config->mipi_dev)) {
		LOG_ERR("MIPI Device not ready!");
		return -EINVAL;
	}

	err = mipi_dbi_reset(config->mipi_dev, SSD1327_RESET_DELAY);
	if (err < 0) {
		LOG_ERR("Failed to reset device!");
		return err;
	}
	k_msleep(SSD1327_RESET_DELAY);

	err = ssd1327_init_device(dev);
	if (err < 0) {
		LOG_ERR("Failed to initialize device! %d", err);
		return err;
	}

	return 0;
}
#endif

#if DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(solomon_ssd1327fb, i2c)
static int ssd1327_init_i2c(const struct device *dev)
{
	const struct ssd1327_config *config = dev->config;
	int err;

	LOG_DBG("Initializing device");

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C Device not ready!");
		return -EINVAL;
	}

	err = ssd1327_init_device(dev);
	if (err < 0) {
		LOG_ERR("Failed to initialize device! %d", err);
		return err;
	}

	return 0;
}
#endif

static DEVICE_API(display, ssd1327_driver_api) = {
	.blanking_on = ssd1327_suspend,
	.blanking_off = ssd1327_resume,
	.write = ssd1327_write,
	.set_contrast = ssd1327_set_contrast,
	.get_capabilities = ssd1327_get_capabilities,
	.set_pixel_format = ssd1327_set_pixel_format,
};

#define SSD1327_WORD_SIZE(inst)                                                                    \
	((DT_STRING_UPPER_TOKEN(inst, mipi_mode) == MIPI_DBI_MODE_SPI_4WIRE) ? SPI_WORD_SET(8)     \
									     : SPI_WORD_SET(9))

#define SSD1327_CONV_BUFFER_SIZE(node_id)                                                          \
	DIV_ROUND_UP(DT_PROP(node_id, width) * CONFIG_SSD1327_CONV_BUFFER_LINES, 2)

#define SSD1327_GRAYSCALE_TABLE(node_id)                                                           \
	.grayscale_table = COND_CODE_1(DT_NODE_HAS_PROP(node_id, grayscale_table),                 \
	(ssd1327_grayscale_table_##node_id), (NULL))

#define SSD1327_DEFINE_I2C(node_id)                                                                \
	static uint8_t conversion_buf##node_id[SSD1327_CONV_BUFFER_SIZE(node_id)];                 \
	static struct ssd1327_data data##node_id;                                                  \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, grayscale_table), (                                  \
	static const uint8_t ssd1327_grayscale_table_##node_id[SSD1327_SET_LUT_COUNT] =            \
	DT_PROP(node_id, grayscale_table);), ())                                                   \
	static const struct ssd1327_config config##node_id = {                                     \
		.i2c = I2C_DT_SPEC_GET(node_id),                                                   \
		.height = DT_PROP(node_id, height),                                                \
		.width = DT_PROP(node_id, width),                                                  \
		.oscillator_freq = DT_PROP(node_id, oscillator_freq),                              \
		.display_offset = DT_PROP(node_id, display_offset),                                \
		.start_line = DT_PROP(node_id, start_line),                                        \
		.multiplex_ratio = DT_PROP(node_id, multiplex_ratio),                              \
		.prechargep = DT_PROP(node_id, prechargep),                                        \
		.remap_value = DT_PROP(node_id, remap_value),                                      \
		.color_inversion = DT_PROP(node_id, inversion_on),                                 \
		.phase_length = DT_PROP(node_id, phase_length),                                    \
		.function_selection_b = DT_PROP(node_id, function_selection_b),                    \
		.precharge_voltage = DT_PROP(node_id, precharge_voltage),                          \
		.vcomh_voltage = DT_PROP(node_id, vcomh_voltage),                                  \
		SSD1327_GRAYSCALE_TABLE(node_id),                                                  \
		.write_cmd = ssd1327_write_bus_cmd_i2c,                                            \
		.write_pixels = ssd1327_write_pixels_i2c,                                          \
		.conversion_buf = conversion_buf##node_id,                                         \
		.conversion_buf_size = sizeof(conversion_buf##node_id),                            \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(node_id, ssd1327_init_i2c, NULL, &data##node_id, &config##node_id,        \
			 POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY, &ssd1327_driver_api);

#define SSD1327_DEFINE_MIPI(node_id)                                                               \
	static uint8_t conversion_buf##node_id[SSD1327_CONV_BUFFER_SIZE(node_id)];                 \
	static struct ssd1327_data data##node_id;                                                  \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, grayscale_table), (                                  \
	static const uint8_t ssd1327_grayscale_table_##node_id[SSD1327_SET_LUT_COUNT] =            \
	DT_PROP(node_id, grayscale_table);), ())                                                   \
	static const struct ssd1327_config config##node_id = {                                     \
		.mipi_dev = DEVICE_DT_GET(DT_PARENT(node_id)),                                     \
		.dbi_config = MIPI_DBI_CONFIG_DT(                                                  \
			node_id, SSD1327_WORD_SIZE(node_id) | SPI_OP_MODE_MASTER, 0),              \
		.height = DT_PROP(node_id, height),                                                \
		.width = DT_PROP(node_id, width),                                                  \
		.oscillator_freq = DT_PROP(node_id, oscillator_freq),                              \
		.display_offset = DT_PROP(node_id, display_offset),                                \
		.start_line = DT_PROP(node_id, start_line),                                        \
		.multiplex_ratio = DT_PROP(node_id, multiplex_ratio),                              \
		.prechargep = DT_PROP(node_id, prechargep),                                        \
		.remap_value = DT_PROP(node_id, remap_value),                                      \
		.color_inversion = DT_PROP(node_id, inversion_on),                                 \
		.phase_length = DT_PROP(node_id, phase_length),                                    \
		.function_selection_b = DT_PROP(node_id, function_selection_b),                    \
		.precharge_voltage = DT_PROP(node_id, precharge_voltage),                          \
		.vcomh_voltage = DT_PROP(node_id, vcomh_voltage),                                  \
		SSD1327_GRAYSCALE_TABLE(node_id),                                                  \
		.write_cmd = ssd1327_write_bus_cmd_mipi,                                           \
		.write_pixels = ssd1327_write_pixels_mipi,                                         \
		.conversion_buf = conversion_buf##node_id,                                         \
		.conversion_buf_size = sizeof(conversion_buf##node_id),                            \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(node_id, ssd1327_init, NULL, &data##node_id, &config##node_id,            \
			 POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY, &ssd1327_driver_api);

#define SSD1327_DEFINE(node_id)                                                                    \
	COND_CODE_1(DT_ON_BUS(node_id, i2c), \
	(SSD1327_DEFINE_I2C(node_id)), (SSD1327_DEFINE_MIPI(node_id)))

DT_FOREACH_STATUS_OKAY(solomon_ssd1327fb, SSD1327_DEFINE)
