/*
 * Copyright (c) 2025 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(display_st7567, CONFIG_DISPLAY_LOG_LEVEL);

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/kernel.h>

#include "display_st7567_regs.h"

struct st7567_dbi {
	const struct device *mipi_dev;
	const struct mipi_dbi_config dbi_config;
};

union st7567_bus {
	struct i2c_dt_spec i2c;
	struct st7567_dbi dbi;
};

typedef bool (*st7567_bus_ready_fn)(const struct device *dev);
typedef int (*st7567_write_cmd_bus_fn)(const struct device *dev, const uint8_t *buf, size_t len);
typedef int (*st7567_write_pixels_bus_fn)(const struct device *dev, const uint8_t *buf, size_t len);
typedef void (*st7567_release_bus_fn)(const struct device *dev);
typedef int (*st7567_reset_fn)(const struct device *dev);
typedef const char *(*st7567_bus_name_fn)(const struct device *dev);

struct st7567_config {
	union st7567_bus bus;
	st7567_bus_ready_fn bus_ready;
	st7567_write_cmd_bus_fn write_cmd_bus;
	st7567_write_pixels_bus_fn write_pixels_bus;
	st7567_release_bus_fn release_bus;
	st7567_reset_fn reset;
	st7567_bus_name_fn bus_name;
	uint16_t height;
	uint16_t width;
	uint8_t column_offset;
	uint8_t line_offset;
	uint8_t regulation_ratio;
	bool com_invdir;
	bool segment_invdir;
	bool inversion_on;
	bool bias;
};

struct st7567_data {
	enum display_pixel_format pf;
};

#if DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(sitronix_st7567, i2c)

static bool st7567_bus_ready_i2c(const struct device *dev)
{
	const struct st7567_config *config = dev->config;

	return i2c_is_ready_dt(&config->bus.i2c);
}

static int st7567_write_cmd_bus_i2c(const struct device *dev, const uint8_t *buf, size_t len)
{
	const struct st7567_config *config = dev->config;

	return i2c_burst_write_dt(&config->bus.i2c, ST7567_CONTROL_ALL_BYTES_CMD, buf, len);
}

static int st7567_write_pixels_bus_i2c(const struct device *dev, const uint8_t *buf, size_t len)
{
	const struct st7567_config *config = dev->config;

	return i2c_burst_write_dt(&config->bus.i2c, ST7567_CONTROL_ALL_BYTES_DATA, buf, len);
}

static const char *st7567_bus_name_i2c(const struct device *dev)
{
	const struct st7567_config *config = dev->config;

	return config->bus.i2c.bus->name;
}

static int st7567_reset_i2c(const struct device *dev)
{
	/* do nothing */
	return 0;
}

static void st7567_release_bus_i2c(const struct device *dev)
{
	/* do nothing */
}

#endif

#if DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(sitronix_st7567, mipi_dbi)

static bool st7567_bus_ready_dbi(const struct device *dev)
{
	const struct st7567_config *config = dev->config;

	return device_is_ready(config->bus.dbi.mipi_dev);
}

static int st7567_write_cmd_bus_dbi(const struct device *dev, const uint8_t *buf, size_t len)
{
	const struct st7567_config *config = dev->config;
	int ret = 0;

	for (size_t i = 0; i < len; i++) {
		ret = mipi_dbi_command_write(config->bus.dbi.mipi_dev, &config->bus.dbi.dbi_config,
					buf[i], NULL, 0);
		if (ret) {
			return ret;
		}
	}
	mipi_dbi_release(config->bus.dbi.mipi_dev, &config->bus.dbi.dbi_config);

	return ret;
}

static int st7567_write_pixels_bus_dbi(const struct device *dev, const uint8_t *buf, size_t len)
{
	const struct st7567_config *config = dev->config;
	struct display_buffer_descriptor mipi_desc;
	int ret;

	mipi_desc.height = 8;
	mipi_desc.width = len;
	mipi_desc.pitch = len;
	mipi_desc.buf_size = len;

	ret = mipi_dbi_write_display(config->bus.dbi.mipi_dev, &config->bus.dbi.dbi_config,
				     buf, &mipi_desc, PIXEL_FORMAT_MONO01);

	return ret;
}

static const char *st7567_bus_name_dbi(const struct device *dev)
{
	const struct st7567_config *config = dev->config;

	return config->bus.dbi.mipi_dev->name;
}

static int st7567_reset_dbi(const struct device *dev)
{
	const struct st7567_config *config = dev->config;
	int err;

	err = mipi_dbi_reset(config->bus.dbi.mipi_dev, ST7567_RESET_DELAY);
	if (err < 0) {
		LOG_ERR("Failed to reset device!");
	}

	return err;
}

static void st7567_release_bus_dbi(const struct device *dev)
{
	const struct st7567_config *config = dev->config;

	mipi_dbi_release(config->bus.dbi.mipi_dev, &config->bus.dbi.dbi_config);
}

#endif

static inline bool st7567_bus_ready(const struct device *dev)
{
	const struct st7567_config *config = dev->config;

	return config->bus_ready(dev);
}

static inline int st7567_write_cmd_bus(const struct device *dev, const uint8_t *buf, size_t len)
{
	const struct st7567_config *config = dev->config;

	return config->write_cmd_bus(dev, buf, len);
}

static inline int st7567_write_pixels_bus(const struct device *dev, const uint8_t *buf, size_t len)
{
	const struct st7567_config *config = dev->config;

	return config->write_pixels_bus(dev, buf, len);
}

static inline void release_bus(const struct device *dev)
{
	const struct st7567_config *config = dev->config;

	config->release_bus(dev);
}

static inline int st7567_hw_reset(const struct device *dev)
{
	const struct st7567_config *config = dev->config;

	return config->reset(dev);
}

static inline int st7567_set_panel_orientation(const struct device *dev)
{
	const struct st7567_config *config = dev->config;
	uint8_t cmd_buf[] = {(config->segment_invdir ? ST7567_SET_SEGMENT_MAP_FLIPPED
						     : ST7567_SET_SEGMENT_MAP_NORMAL),
			     (config->com_invdir ? ST7567_SET_COM_OUTPUT_SCAN_FLIPPED
						 : ST7567_SET_COM_OUTPUT_SCAN_NORMAL)};

	return st7567_write_cmd_bus(dev, cmd_buf, sizeof(cmd_buf));
}

static inline int st7567_set_hardware_config(const struct device *dev)
{
	const struct st7567_config *config = dev->config;
	int ret;
	uint8_t cmd_buf[1];

	cmd_buf[0] = ST7567_SET_BIAS | (config->bias ? 1 : 0);
	ret = st7567_write_cmd_bus(dev, cmd_buf, 1);
	if (ret < 0) {
		return ret;
	}

	cmd_buf[0] = ST7567_POWER_CONTROL | ST7567_POWER_CONTROL_VB;
	ret = st7567_write_cmd_bus(dev, cmd_buf, 1);
	if (ret < 0) {
		return ret;
	}

	cmd_buf[0] = ST7567_POWER_CONTROL | ST7567_POWER_CONTROL_VB | ST7567_POWER_CONTROL_VR;
	ret = st7567_write_cmd_bus(dev, cmd_buf, 1);
	if (ret < 0) {
		return ret;
	}

	cmd_buf[0] = ST7567_POWER_CONTROL | ST7567_POWER_CONTROL_VB | ST7567_POWER_CONTROL_VR |
		     ST7567_POWER_CONTROL_VF;
	ret = st7567_write_cmd_bus(dev, cmd_buf, 1);
	if (ret < 0) {
		return ret;
	}

	cmd_buf[0] = ST7567_SET_REGULATION_RATIO | (config->regulation_ratio & 0x7);
	ret = st7567_write_cmd_bus(dev, cmd_buf, 1);
	if (ret < 0) {
		return ret;
	}

	cmd_buf[0] = ST7567_LINE_SCROLL | (config->line_offset & 0x3F);
	ret = st7567_write_cmd_bus(dev, cmd_buf, 1);
	if (ret < 0) {
		return ret;
	}

	return ret;
}

static int st7567_resume(const struct device *dev)
{
	uint8_t cmd_buf[] = {
		ST7567_DISPLAY_ALL_PIXEL_NORMAL,
		ST7567_DISPLAY_ON,
	};

	return st7567_write_cmd_bus(dev, cmd_buf, sizeof(cmd_buf));
}

static int st7567_suspend(const struct device *dev)
{
	uint8_t cmd_buf[] = {
		ST7567_DISPLAY_OFF,
		ST7567_DISPLAY_ALL_PIXEL_ON,
	};

	return st7567_write_cmd_bus(dev, cmd_buf, sizeof(cmd_buf));
}

static int st7567_write_default(const struct device *dev, const uint16_t x, const uint16_t y,
				const uint8_t *buf, const size_t buf_len)
{
	const struct st7567_config *config = dev->config;
	int ret;
	uint8_t cmd_buf[3];
	uint16_t column = x + config->column_offset;

	cmd_buf[0] = ST7567_COLUMN_LSB | (column & 0xF);
	cmd_buf[1] = ST7567_COLUMN_MSB | ((column >> 4) & 0xF);
	cmd_buf[2] = ST7567_PAGE | (y >> 3);

	ret = st7567_write_cmd_bus(dev, cmd_buf, sizeof(cmd_buf));
	if (ret < 0) {
		return ret;
	}

	return st7567_write_pixels_bus(dev, buf, buf_len);
}

static int st7567_write_desc(const struct device *dev, const uint16_t x, const uint16_t y,
			     const struct display_buffer_descriptor *desc, const void *buf,
			     const size_t buf_len)
{
	int ret = 0;

	for (int i = 0; i < desc->height / 8; i++) {
		ret = st7567_write_default(dev, x, y + (i << 3),
					   ((const uint8_t *)buf + i * desc->pitch), desc->pitch);
		if (ret < 0) {
			return ret;
		}
	}

	release_bus(dev);

	return ret;
}

static int st7567_write(const struct device *dev, const uint16_t x, const uint16_t y,
			const struct display_buffer_descriptor *desc, const void *buf)
{
	size_t buf_len;

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

	if ((y & 0x7) != 0U) {
		LOG_ERR("Y coordinate must be aligned on page boundary");
		return -EINVAL;
	}

	LOG_DBG("x %u, y %u, pitch %u, width %u, height %u, buf_len %u", x, y, desc->pitch,
		desc->width, desc->height, buf_len);

	return st7567_write_desc(dev, x, y, desc, buf, buf_len);
}

static int st7567_set_contrast(const struct device *dev, const uint8_t contrast)
{
	uint8_t cmd_buf[] = {
		ST7567_SET_CONTRAST_CTRL,
		contrast,
	};

	return st7567_write_cmd_bus(dev, cmd_buf, sizeof(cmd_buf));
}

static void st7567_get_capabilities(const struct device *dev, struct display_capabilities *caps)
{
	const struct st7567_config *config = dev->config;
	struct st7567_data *data = dev->data;

	caps->x_resolution = config->width;
	caps->y_resolution = config->height;
	caps->supported_pixel_formats = PIXEL_FORMAT_MONO10 | PIXEL_FORMAT_MONO01;
	caps->current_pixel_format = data->pf;
	caps->screen_info = SCREEN_INFO_MONO_VTILED;
	caps->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static int st7567_set_pixel_format(const struct device *dev, const enum display_pixel_format pf)
{
	struct st7567_data *data = dev->data;
	const struct st7567_config *config = dev->config;
	uint8_t cmd;
	int ret;

	if (pf == data->pf) {
		return 0;
	}

	if (pf == PIXEL_FORMAT_MONO10) {
		cmd = config->inversion_on ? ST7567_SET_REVERSE_DISPLAY : ST7567_SET_NORMAL_DISPLAY;
	} else if (pf == PIXEL_FORMAT_MONO01) {
		cmd = config->inversion_on ? ST7567_SET_NORMAL_DISPLAY : ST7567_SET_REVERSE_DISPLAY;
	} else {
		LOG_WRN("Unsupported pixel format");
		return -ENOTSUP;
	}

	ret = st7567_write_cmd_bus(dev, &cmd, 1);
	if (ret) {
		LOG_WRN("Couldn't set inversion");
		return ret;
	}

	data->pf = pf;

	return 0;
}

static int st7567_reset(const struct device *dev)
{
	const struct st7567_config *config = dev->config;
	int ret;
	uint8_t cmd_buf[] = {
		ST7567_DISPLAY_OFF,
		(config->inversion_on ? ST7567_SET_REVERSE_DISPLAY : ST7567_SET_NORMAL_DISPLAY),
	};

	ret = st7567_hw_reset(dev);
	if (ret < 0) {
		return ret;
	}

	return st7567_write_cmd_bus(dev, cmd_buf, sizeof(cmd_buf));
}

static int st7567_clear(const struct device *dev)
{
	const struct st7567_config *config = dev->config;
	int ret = 0;
	uint8_t buf = 0;

	for (int y = 0; y < config->height; y += 8) {
		for (int x = 0; x < config->width; x++) {
			ret = st7567_write_default(dev, x, y, &buf, 1);
			if (ret < 0) {
				LOG_ERR("Error clearing display");
				return ret;
			}
		}
	}
	return ret;
}

static int st7567_init_device(const struct device *dev)
{
	const struct st7567_config *config = dev->config;
	struct st7567_data *data = dev->data;
	int ret;
	uint8_t cmd_buf[] = {
		ST7567_DISPLAY_OFF,
		(config->inversion_on ? ST7567_SET_REVERSE_DISPLAY : ST7567_SET_NORMAL_DISPLAY),
	};

	ret = st7567_reset(dev);
	if (ret < 0) {
		return ret;
	}

	ret = st7567_suspend(dev);
	if (ret < 0) {
		return ret;
	}

	ret = st7567_set_hardware_config(dev);
	if (ret < 0) {
		return ret;
	}

	ret = st7567_set_panel_orientation(dev);
	if (ret < 0) {
		return ret;
	}

	/* Set inversion */
	ret = st7567_write_cmd_bus(dev, cmd_buf, sizeof(cmd_buf));
	if (ret < 0) {
		return ret;
	}

	data->pf = config->inversion_on ? PIXEL_FORMAT_MONO10 : PIXEL_FORMAT_MONO01;

	ret = st7567_set_contrast(dev, CONFIG_ST7567_DEFAULT_CONTRAST);
	if (ret < 0) {
		return ret;
	}

	/* Clear display, RAM is undefined at power up */
	ret = st7567_clear(dev);
	if (ret < 0) {
		return ret;
	}
	ret = st7567_resume(dev);

	return ret;
}

static int st7567_init(const struct device *dev)
{
	const struct st7567_config *config = dev->config;

	if (!st7567_bus_ready(dev)) {
		LOG_ERR("Bus device %s not ready!", config->bus_name(dev));
		return -EINVAL;
	}

	if (st7567_init_device(dev)) {
		LOG_ERR("Failed to initialize device!");
		return -EIO;
	}

	return 0;
}

static DEVICE_API(display, st7567_driver_api) = {
	.blanking_on = st7567_suspend,
	.blanking_off = st7567_resume,
	.write = st7567_write,
	.clear = st7567_clear,
	.set_contrast = st7567_set_contrast,
	.get_capabilities = st7567_get_capabilities,
	.set_pixel_format = st7567_set_pixel_format,
};

#define ST7567_WORD_SIZE(inst)                                                                     \
	((DT_STRING_UPPER_TOKEN(inst, mipi_mode) == MIPI_DBI_MODE_SPI_4WIRE) ? SPI_WORD_SET(8)     \
									     : SPI_WORD_SET(9))

#define ST7567_CONFIG_DBI(node_id)                                                                 \
	.bus = {.dbi.dbi_config = MIPI_DBI_CONFIG_DT(                                              \
			node_id, ST7567_WORD_SIZE(node_id) | SPI_OP_MODE_MASTER, 0),               \
		.dbi.mipi_dev = DEVICE_DT_GET(DT_PARENT(node_id)),},                               \
	.bus_ready = st7567_bus_ready_dbi,                                                         \
	.write_cmd_bus = st7567_write_cmd_bus_dbi, .write_pixels_bus = st7567_write_pixels_bus_dbi,\
	.bus_name = st7567_bus_name_dbi, .release_bus = st7567_release_bus_dbi,                    \
	.reset = st7567_reset_dbi,

#define ST7567_CONFIG_I2C(node_id)                                                                 \
	.bus = {.i2c = I2C_DT_SPEC_GET(node_id)}, .bus_ready = st7567_bus_ready_i2c,               \
	.write_cmd_bus = st7567_write_cmd_bus_i2c, .write_pixels_bus = st7567_write_pixels_bus_i2c,\
	.bus_name = st7567_bus_name_i2c, .release_bus = st7567_release_bus_i2c,                    \
	.reset = st7567_reset_i2c,

#define ST7567_DEFINE(node_id)                                                                     \
	static struct st7567_data data##node_id;                                                   \
	static const struct st7567_config config##node_id = {                                      \
		.height = DT_PROP(node_id, height),                                                \
		.width = DT_PROP(node_id, width),                                                  \
		.column_offset = DT_PROP(node_id, column_offset),                                  \
		.line_offset = DT_PROP(node_id, line_offset),                                      \
		.segment_invdir = DT_PROP(node_id, segment_invdir),                                \
		.com_invdir = DT_PROP(node_id, com_invdir),                                        \
		.inversion_on = DT_PROP(node_id, inversion_on),                                    \
		.bias = DT_PROP(node_id, bias),                                                    \
		.regulation_ratio = DT_PROP(node_id, regulation_ratio),                            \
		COND_CODE_1(DT_ON_BUS(node_id, mipi_dbi), (ST7567_CONFIG_DBI(node_id)),            \
			    (ST7567_CONFIG_I2C(node_id))) }; \
                                                                                                   \
	DEVICE_DT_DEFINE(node_id, st7567_init, NULL, &data##node_id, &config##node_id,             \
			 POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY, &st7567_driver_api);

DT_FOREACH_STATUS_OKAY(sitronix_st7567, ST7567_DEFINE)
