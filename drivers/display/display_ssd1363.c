/*
 * Copyright (c) 2025 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ssd1363, CONFIG_DISPLAY_LOG_LEVEL);

#define SSD1363_SET_COMMAND_LOCK 0xFD
#define SSD1363_UNLOCK_COMMAND   0x12

#define SSD1363_CONTROL_ALL_BYTES_CMD  0x0
#define SSD1363_CONTROL_ALL_BYTES_DATA 0x40

#define SSD1363_SET_PHASE_LENGTH       0xB1
#define SSD1363_SET_OSC_FREQ           0xB3
#define SSD1363_LINEAR_LUT             0xB9
#define SSD1363_SET_LUT                0xB8
#define SSD1363_SET_PRECHARGE_V_CFG    0xBA
#define SSD1363_SET_PRECHARGE_VOLTAGE  0xBB
#define SSD1363_SET_VCOMH              0xBE
#define SSD1363_SET_INTERNAL_IREF      0xAD
#define SSD1363_SET_DISPLAY_START_LINE 0xA1
#define SSD1363_SET_DISPLAY_OFFSET     0xA2
#define SSD1363_SET_NORMAL_DISPLAY     0xA6
#define SSD1363_SET_REVERSE_DISPLAY    0xA7
#define SSD1363_SET_ENTIRE_DISPLAY_ON  0xA5
#define SSD1363_SET_ENTIRE_DISPLAY_OFF 0xA4
#define SSD1363_DISPLAY_ON             0xAF
#define SSD1363_DISPLAY_OFF            0xAE
#define SSD1363_SET_CONTRAST_CTRL      0xC1
#define SSD1363_SET_MULTIPLEX_RATIO    0xCA
#define SSD1363_SET_PRECHARGE_PERIOD   0xB6
#define SSD1363_SET_COLUMN_ADDR        0x15
#define SSD1363_SET_ROW_ADDR           0x75
#define SSD1363_WRITE_RAM              0x5C
#define SSD1363_READ_RAM               0x5D
#define SSD1363_SET_REMAP_VALUE        0xA0
#define SSD1363_SET_GRAY_ENHANCE       0xB4

#define SSD1363_RESET_DELAY   100
#define SSD1363_SET_LUT_COUNT 15

typedef int (*ssd1363_write_bus_cmd_fn)(const struct device *dev, const uint8_t cmd,
					const uint8_t *data, size_t len);
typedef int (*ssd1363_write_pixels_fn)(const struct device *dev, const uint8_t *buf,
				       uint32_t pixel_count,
				       const struct display_buffer_descriptor *desc);

struct ssd1363_config {
	struct i2c_dt_spec i2c;
	ssd1363_write_bus_cmd_fn write_cmd;
	ssd1363_write_pixels_fn write_pixels;
	const struct device *mipi_dev;
	const struct mipi_dbi_config dbi_config;
	uint16_t height;
	uint16_t width;
	uint8_t oscillator_freq;
	uint8_t start_line;
	uint8_t display_offset;
	uint8_t multiplex_ratio;
	uint8_t internal_iref;
	uint16_t remap_value;
	uint8_t phase_length;
	uint8_t precharge_voltage;
	uint8_t vcomh_voltage;
	uint8_t precharge_period;
	uint8_t precharge_config;
	uint16_t column_offset;
	const uint8_t *grayscale_table;
	bool color_inversion;
	bool grayscale_enhancement;
	uint8_t *conversion_buf;
	size_t conversion_buf_size;
};

static inline int ssd1363_write_bus_cmd_mipi(const struct device *dev, const uint8_t cmd,
					     const uint8_t *data, size_t len)
{
	const struct ssd1363_config *config = dev->config;
	int err;

	err = mipi_dbi_command_write(config->mipi_dev, &config->dbi_config, cmd, data, len);
	if (err < 0) {
		return err;
	}
	mipi_dbi_release(config->mipi_dev, &config->dbi_config);

	return 0;
}

static inline int ssd1363_write_bus_cmd_i2c(const struct device *dev, const uint8_t cmd,
					    const uint8_t *data, size_t len)
{
	const struct ssd1363_config *config = dev->config;
	int err;

	err = i2c_burst_write_dt(&config->i2c, SSD1363_CONTROL_ALL_BYTES_CMD, &cmd, 1);
	if (err < 0) {
		return err;
	}
	if (len > 0) {
		return i2c_burst_write_dt(&config->i2c, SSD1363_CONTROL_ALL_BYTES_DATA, data, len);
	} else {
		return err;
	}
}

static inline int ssd1363_set_hardware_config(const struct device *dev)
{
	const struct ssd1363_config *config = dev->config;
	uint8_t buf[2];
	int err;

	buf[0] = SSD1363_UNLOCK_COMMAND;
	err = config->write_cmd(dev, SSD1363_SET_COMMAND_LOCK, buf, 1);
	if (err < 0) {
		return err;
	}
	err = config->write_cmd(dev, SSD1363_SET_OSC_FREQ, &config->oscillator_freq, 1);
	if (err < 0) {
		return err;
	}
	err = config->write_cmd(dev, SSD1363_SET_MULTIPLEX_RATIO, &config->multiplex_ratio, 1);
	if (err < 0) {
		return err;
	}
	err = config->write_cmd(dev, SSD1363_SET_DISPLAY_START_LINE, &config->start_line, 1);
	if (err < 0) {
		return err;
	}
	err = config->write_cmd(dev, SSD1363_SET_DISPLAY_OFFSET, &config->display_offset, 1);
	if (err < 0) {
		return err;
	}
	buf[1] = config->remap_value & 0xFF;
	buf[0] = config->remap_value >> 8;
	err = config->write_cmd(dev, SSD1363_SET_REMAP_VALUE, buf, 2);
	if (err < 0) {
		return err;
	}
	err = config->write_cmd(dev, SSD1363_SET_PRECHARGE_V_CFG, &config->precharge_config, 1);
	if (err < 0) {
		return err;
	}
	err = config->write_cmd(dev, SSD1363_LINEAR_LUT, NULL, 0);
	if (err < 0) {
		return err;
	}
	if (config->grayscale_table != NULL) {
		err = config->write_cmd(dev, SSD1363_SET_LUT, config->grayscale_table,
					SSD1363_SET_LUT_COUNT);
		if (err < 0) {
			return err;
		}
	}
	err = config->write_cmd(dev, SSD1363_SET_INTERNAL_IREF, &config->internal_iref, 1);
	if (err < 0) {
		return err;
	}
	err = config->write_cmd(dev, SSD1363_SET_PHASE_LENGTH, &config->phase_length, 1);
	if (err < 0) {
		return err;
	}
	err = config->write_cmd(dev, SSD1363_SET_PRECHARGE_VOLTAGE, &config->precharge_voltage, 1);
	if (err < 0) {
		return err;
	}
	err = config->write_cmd(dev, SSD1363_SET_VCOMH, &config->vcomh_voltage, 1);
	if (err < 0) {
		return err;
	}
	if (config->grayscale_enhancement) {
		/* Undocumented values from datasheet */
		buf[0] = 0x32;
		buf[1] = 0x0c;
		err = config->write_cmd(dev, SSD1363_SET_GRAY_ENHANCE, buf, 2);
		if (err < 0) {
			return err;
		}
	}
	buf[0] = (config->precharge_period & 0xF) | 0xC0;
	return config->write_cmd(dev, SSD1363_SET_PRECHARGE_PERIOD, buf, 1);
}

static int ssd1363_resume(const struct device *dev)
{
	const struct ssd1363_config *config = dev->config;

	return config->write_cmd(dev, SSD1363_DISPLAY_ON, NULL, 0);
}

static int ssd1363_suspend(const struct device *dev)
{
	const struct ssd1363_config *config = dev->config;

	return config->write_cmd(dev, SSD1363_DISPLAY_OFF, NULL, 0);
}

static int ssd1363_set_display(const struct device *dev, const uint16_t x, const uint16_t y,
			       const struct display_buffer_descriptor *desc)
{
	const struct ssd1363_config *config = dev->config;
	int err;
	uint8_t x_position[] = {x / 4, x / 4 + desc->width / 4 - 1};
	uint8_t y_position[] = {y, y + desc->height - 1};

	err = config->write_cmd(dev, SSD1363_SET_COLUMN_ADDR, x_position, sizeof(x_position));
	if (err < 0) {
		return err;
	}
	err = config->write_cmd(dev, SSD1363_SET_ROW_ADDR, y_position, sizeof(y_position));
	if (err < 0) {
		return err;
	}
	return config->write_cmd(dev, SSD1363_WRITE_RAM, NULL, 0);
}

/* SSD1363 has peculiar addressing:
 * It stores 2 bytes per address, and 2 pixels per byte
 */
static int ssd1363_convert_L_8(const struct device *dev, const uint8_t *buf, int cur_offset,
			       uint32_t pixel_count)
{
	const struct ssd1363_config *config = dev->config;
	int i = 0;

	for (; i / 2 < config->conversion_buf_size && pixel_count > cur_offset + i; i += 4) {
		config->conversion_buf[i / 2 + 1] =
			(buf[cur_offset + i] >> 4) | ((buf[cur_offset + i + 1] >> 4) << 4);
		config->conversion_buf[i / 2] =
			(buf[cur_offset + i + 2] >> 4) | ((buf[cur_offset + i + 3] >> 4) << 4);
	}
	return i;
}

#if DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(solomon_ssd1363, mipi_dbi)
static int ssd1363_write_pixels_mipi(const struct device *dev, const uint8_t *buf,
				     uint32_t pixel_count,
				     const struct display_buffer_descriptor *desc)
{
	const struct ssd1363_config *config = dev->config;
	struct display_buffer_descriptor mipi_desc;
	int ret, i;
	int total = 0;

	mipi_desc.pitch = desc->pitch;

	while (pixel_count > total) {
		i = ssd1363_convert_L_8(dev, buf, total, pixel_count);

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

#if DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(solomon_ssd1363, i2c)
static int ssd1363_write_pixels_i2c(const struct device *dev, const uint8_t *buf,
				    uint32_t pixel_count,
				    const struct display_buffer_descriptor *desc)
{
	const struct ssd1363_config *config = dev->config;
	int ret, i;
	int total = 0;

	while (pixel_count > total) {
		i = ssd1363_convert_L_8(dev, buf, total, pixel_count);

		ret = i2c_burst_write_dt(&config->i2c, SSD1363_CONTROL_ALL_BYTES_DATA,
					 config->conversion_buf, i / 2);
		if (ret < 0) {
			return ret;
		}
		total += i;
	}
	return 0;
}
#endif

static int ssd1363_write(const struct device *dev, const uint16_t x, const uint16_t y,
			 const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct ssd1363_config *config = dev->config;
	int err;
	size_t buf_len;
	int32_t pixel_count = desc->width * desc->height;

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

	if ((x & 3) != 0U) {
		LOG_ERR("Unsupported origin");
		return -EINVAL;
	}

	LOG_DBG("x %u, y %u, pitch %u, width %u, height %u, buf_len %u", x, y, desc->pitch,
		desc->width, desc->height, buf_len);

	err = ssd1363_set_display(dev, x + config->column_offset, y, desc);
	if (err < 0) {
		return err;
	}

	return config->write_pixels(dev, buf, pixel_count, desc);
}

static int ssd1363_set_contrast(const struct device *dev, const uint8_t contrast)
{
	const struct ssd1363_config *config = dev->config;

	return config->write_cmd(dev, SSD1363_SET_CONTRAST_CTRL, &contrast, 1);
}

static void ssd1363_get_capabilities(const struct device *dev, struct display_capabilities *caps)
{
	const struct ssd1363_config *config = dev->config;

	memset(caps, 0, sizeof(struct display_capabilities));
	caps->x_resolution = config->width;
	caps->y_resolution = config->height;
	caps->supported_pixel_formats = PIXEL_FORMAT_L_8;
	caps->current_pixel_format = PIXEL_FORMAT_L_8;
	caps->screen_info = 0;
}

static int ssd1363_set_pixel_format(const struct device *dev, const enum display_pixel_format pf)
{
	if (pf == PIXEL_FORMAT_L_8) {
		return 0;
	}
	LOG_ERR("Unsupported pixel format");
	(void)dev;
	return -ENOTSUP;
}

static int ssd1363_init_device(const struct device *dev)
{
	const struct ssd1363_config *config = dev->config;
	uint8_t buf;
	int err;

	/* Turn display off */
	err = ssd1363_suspend(dev);
	if (err < 0) {
		return err;
	}

	err = ssd1363_set_hardware_config(dev);
	if (err < 0) {
		return err;
	}

	err = ssd1363_set_contrast(dev, CONFIG_SSD1363_DEFAULT_CONTRAST);
	if (err < 0) {
		return err;
	}

	buf = (config->color_inversion ? SSD1363_SET_REVERSE_DISPLAY : SSD1363_SET_NORMAL_DISPLAY);
	err = config->write_cmd(dev, buf, NULL, 0);
	if (err < 0) {
		return err;
	}

	return ssd1363_resume(dev);
}

#if DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(solomon_ssd1363, mipi_dbi)
static int ssd1363_init(const struct device *dev)
{
	const struct ssd1363_config *config = dev->config;
	int err;

	LOG_DBG("Initializing device");

	if (!device_is_ready(config->mipi_dev)) {
		LOG_ERR("MIPI Device not ready!");
		return -EINVAL;
	}

	err = mipi_dbi_reset(config->mipi_dev, SSD1363_RESET_DELAY);
	if (err < 0) {
		LOG_ERR("Failed to reset device!");
		return err;
	}
	k_msleep(SSD1363_RESET_DELAY);

	err = ssd1363_init_device(dev);
	if (err < 0) {
		LOG_ERR("Failed to initialize device! %d", err);
		return err;
	}

	return 0;
}
#endif

#if DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(solomon_ssd1363, i2c)
static int ssd1363_init_i2c(const struct device *dev)
{
	const struct ssd1363_config *config = dev->config;
	int err;

	LOG_DBG("Initializing device");

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C Device not ready!");
		return -EINVAL;
	}

	err = ssd1363_init_device(dev);
	if (err < 0) {
		LOG_ERR("Failed to initialize device! %d", err);
		return err;
	}

	return 0;
}
#endif

static DEVICE_API(display, ssd1363_driver_api) = {
	.blanking_on = ssd1363_suspend,
	.blanking_off = ssd1363_resume,
	.write = ssd1363_write,
	.set_contrast = ssd1363_set_contrast,
	.get_capabilities = ssd1363_get_capabilities,
	.set_pixel_format = ssd1363_set_pixel_format,
};

#define SSD1363_WORD_SIZE(inst)                                                                    \
	((DT_STRING_UPPER_TOKEN(inst, mipi_mode) == MIPI_DBI_MODE_SPI_4WIRE) ? SPI_WORD_SET(8)     \
									     : SPI_WORD_SET(9))

#define SSD1363_CONV_BUFFER_SIZE(node_id)                                                          \
	DIV_ROUND_UP(DT_PROP(node_id, width) * CONFIG_SSD1363_CONV_BUFFER_LINES, 1)

#define SSD1363_GRAYSCALE_TABLE(node_id)                                                           \
	.grayscale_table = COND_CODE_1(DT_NODE_HAS_PROP(node_id, grayscale_table),                 \
	(ssd1363_grayscale_table_##node_id), (NULL))

#define SSD1363_DEFINE_I2C(node_id)                                                                \
	static uint8_t conversion_buf##node_id[SSD1363_CONV_BUFFER_SIZE(node_id)];                 \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, grayscale_table), (                                  \
	static const uint8_t ssd1363_grayscale_table_##node_id[SSD1363_SET_LUT_COUNT] =            \
	DT_PROP(node_id, grayscale_table);), ())                                                   \
	static const struct ssd1363_config config##node_id = {                                     \
		.i2c = I2C_DT_SPEC_GET(node_id),                                                   \
		.height = DT_PROP(node_id, height),                                                \
		.width = DT_PROP(node_id, width),                                                  \
		.oscillator_freq = DT_PROP(node_id, oscillator_freq),                              \
		.display_offset = DT_PROP(node_id, display_offset),                                \
		.start_line = DT_PROP(node_id, start_line),                                        \
		.multiplex_ratio = DT_PROP(node_id, multiplex_ratio),                              \
		.remap_value = DT_PROP(node_id, remap_value),                                      \
		.color_inversion = DT_PROP(node_id, inversion_on),                                 \
		.phase_length = DT_PROP(node_id, phase_length),                                    \
		.internal_iref = DT_PROP(node_id, internal_iref),                                  \
		.precharge_voltage = DT_PROP(node_id, precharge_voltage),                          \
		.vcomh_voltage = DT_PROP(node_id, vcomh_voltage),                                  \
		.precharge_period = DT_PROP(node_id, precharge_period),                            \
		.precharge_config = DT_PROP(node_id, precharge_config),                            \
		.column_offset = DT_PROP(node_id, column_offset),                                  \
		.grayscale_enhancement = DT_PROP(node_id, grayscale_enhancement),                  \
		SSD1363_GRAYSCALE_TABLE(node_id),                                                  \
		.write_cmd = ssd1363_write_bus_cmd_i2c,                                            \
		.write_pixels = ssd1363_write_pixels_i2c,                                          \
		.conversion_buf = conversion_buf##node_id,                                         \
		.conversion_buf_size = sizeof(conversion_buf##node_id),                            \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(node_id, ssd1363_init_i2c, NULL, NULL, &config##node_id,                  \
			 POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY, &ssd1363_driver_api);

#define SSD1363_DEFINE_MIPI(node_id)                                                               \
	static uint8_t conversion_buf##node_id[SSD1363_CONV_BUFFER_SIZE(node_id)];                 \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, grayscale_table), (                                  \
	static const uint8_t ssd1363_grayscale_table_##node_id[SSD1363_SET_LUT_COUNT] =            \
	DT_PROP(node_id, grayscale_table);), ())                                                   \
	static const struct ssd1363_config config##node_id = {                                     \
		.mipi_dev = DEVICE_DT_GET(DT_PARENT(node_id)),                                     \
		.dbi_config = MIPI_DBI_CONFIG_DT(                                                  \
			node_id, SSD1363_WORD_SIZE(node_id) | SPI_OP_MODE_MASTER, 0),              \
		.height = DT_PROP(node_id, height),                                                \
		.width = DT_PROP(node_id, width),                                                  \
		.oscillator_freq = DT_PROP(node_id, oscillator_freq),                              \
		.display_offset = DT_PROP(node_id, display_offset),                                \
		.start_line = DT_PROP(node_id, start_line),                                        \
		.multiplex_ratio = DT_PROP(node_id, multiplex_ratio),                              \
		.remap_value = DT_PROP(node_id, remap_value),                                      \
		.color_inversion = DT_PROP(node_id, inversion_on),                                 \
		.phase_length = DT_PROP(node_id, phase_length),                                    \
		.internal_iref = DT_PROP(node_id, internal_iref),                                  \
		.precharge_voltage = DT_PROP(node_id, precharge_voltage),                          \
		.vcomh_voltage = DT_PROP(node_id, vcomh_voltage),                                  \
		.precharge_period = DT_PROP(node_id, precharge_period),                            \
		.precharge_config = DT_PROP(node_id, precharge_config),                            \
		.column_offset = DT_PROP(node_id, column_offset),                                  \
		.grayscale_enhancement = DT_PROP(node_id, grayscale_enhancement),                  \
		SSD1363_GRAYSCALE_TABLE(node_id),                                                  \
		.write_cmd = ssd1363_write_bus_cmd_mipi,                                           \
		.write_pixels = ssd1363_write_pixels_mipi,                                         \
		.conversion_buf = conversion_buf##node_id,                                         \
		.conversion_buf_size = sizeof(conversion_buf##node_id),                            \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(node_id, ssd1363_init, NULL, NULL, &config##node_id,                      \
			 POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY, &ssd1363_driver_api);

#define SSD1363_DEFINE(node_id)                                                                    \
	COND_CODE_1(DT_ON_BUS(node_id, i2c), \
	(SSD1363_DEFINE_I2C(node_id)), (SSD1363_DEFINE_MIPI(node_id)))

DT_FOREACH_STATUS_OKAY(solomon_ssd1363, SSD1363_DEFINE)
