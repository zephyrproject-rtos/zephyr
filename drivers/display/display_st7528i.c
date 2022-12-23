/*
 * Copyright (c) 2022-2023 SILA Embedded Solutions GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT sitronix_st7528i

#include "display_st7528i.h"

#include <errno.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(display_st7528i, CONFIG_DISPLAY_LOG_LEVEL);

/* All following bytes will contain commands */
#define ST7528I_CONTROL_ALL_BYTES_CMD  0x00
/* All following bytes will contain data */
#define ST7528I_CONTROL_ALL_BYTES_DATA 0x40
/* The next byte will contain a command */
#define ST7528I_CONTROL_ONE_BYTE_CMD   0x80
/* The next byte will contain data */
#define ST7528I_CONTROL_ONE_BYTE_DATA  0xC0

struct st7528i_config {
	struct i2c_dt_spec bus;
	struct gpio_dt_spec reset;
	struct gpio_dt_spec chip_select;

	uint16_t height;
	uint16_t width;

	uint8_t framerate;
	uint8_t com_partial_display;
	uint8_t com_offset;
	bool invert_com;
	bool invert_segments;

	uint8_t lcd_bias;
	uint8_t regulator_resistor;
	uint8_t electronic_volume;
	uint8_t boost;
};

struct st7528i_data {
	uint8_t framebuffer[ST7528_NUM_PAGES * ST7528_BPP * ST7528_WIDTH];
};

static int st7528i_write_cmds(const struct device *dev, const uint8_t *data, uint32_t len)
{
	const struct st7528i_config *config = dev->config;

	return i2c_burst_write_dt(&config->bus, ST7528I_CONTROL_ALL_BYTES_CMD, data, len);
}

static int st7528i_write_data(const struct device *dev, const uint8_t *data, uint32_t len)
{
	const struct st7528i_config *config = dev->config;

	return i2c_burst_write_dt(&config->bus, ST7528I_CONTROL_ALL_BYTES_DATA, data, len);
}

static int st7528i_set_mode(const struct device *dev, uint8_t mode)
{
	const struct st7528i_config *config = dev->config;
	uint8_t cmd_buf[] = {
		ST7528_CMD_SET_MODE,
		ST7528_MODE_BOOSTER_EFFICIENCY_2 | (config->framerate << 4) | mode,
	};

	return st7528i_write_cmds(dev, cmd_buf, ARRAY_SIZE(cmd_buf));
}

static int st7528i_write_palette(const struct device *dev)
{
	int ret;

	ret = st7528i_set_mode(dev, ST7528_MODE_PALETTE);
	if (ret) {
		return ret;
	}

	for (int index = 0; index < ST7528_NUM_GRAY_LEVELS; index++) {
		uint8_t value = ST7528_PWM_LEVELS - (index * ST7528_PWM_LEVELS / 15);
		uint8_t cmd = ST7528_CMD_SET_PALETTE_ENTRY | (index << 2);

		uint8_t cmd_buf[] = {
			/*
			 * Same value for all four "frames" - not entirely clear from the datasheet
			 * what that even means
			 */
			/* clang-format off */
			cmd | 0x00, value,
			cmd | 0x01, value,
			cmd | 0x02, value,
			cmd | 0x03, value,
			/* clang-format on */
		};

		ret = st7528i_write_cmds(dev, cmd_buf, ARRAY_SIZE(cmd_buf));
		if (ret) {
			return ret;
		}
	}

	return st7528i_set_mode(dev, ST7528_MODE_NORMAL);
}

static int st7528i_reset(const struct device *dev)
{
	const struct st7528i_config *config = dev->config;
	int ret;

	if (config->reset.port) {
		gpio_pin_set_dt(&config->reset, true);
		k_sleep(ST7528_RESET_DELAY);
		gpio_pin_set_dt(&config->reset, false);
		k_sleep(ST7528_RESET_DELAY);
	}

	{
		uint8_t cmd_buf[] = {
			/* clang-format off */
			ST7528_CMD_SET_PARTIAL_DISPLAY_LINES, config->com_partial_display,
			ST7528_CMD_ADC_SELECT | config->invert_segments,
			ST7528_CMD_SHL_SELECT | (config->invert_com << 3),
			ST7528_CMD_SET_INITIAL_COM0, config->com_offset,

			ST7528_CMD_START_OSCILLATOR,
			ST7528_CMD_SET_REGULATOR_RESISTOR | config->regulator_resistor,
			ST7528_CMD_SET_ELECTRONIC_VOLUME, config->electronic_volume,
			ST7528_CMD_SET_LCD_BIAS | (config->lcd_bias - 5),

			ST7528_CMD_SET_BOOST | 0x00,
			/* clang-format on */
		};

		ret = st7528i_write_cmds(dev, cmd_buf, ARRAY_SIZE(cmd_buf));
		if (ret) {
			return ret;
		}
	}

	k_sleep(K_MSEC(200));

	{
		uint8_t cmd_buf[] = {
			ST7528_CMD_POWER_CONTROL | ST7528_POWER_CONTROL_VC,
			ST7528_CMD_SET_BOOST | (config->boost - 3),
		};
		ret = st7528i_write_cmds(dev, cmd_buf, ARRAY_SIZE(cmd_buf));
		if (ret) {
			return ret;
		}
	}

	k_sleep(K_MSEC(200));

	{
		uint8_t cmd_buf[] = {
			ST7528_CMD_POWER_CONTROL | ST7528_POWER_CONTROL_VC |
				ST7528_POWER_CONTROL_VR,
		};
		ret = st7528i_write_cmds(dev, cmd_buf, ARRAY_SIZE(cmd_buf));
		if (ret) {
			return ret;
		}
	}

	k_sleep(K_MSEC(10));

	{
		uint8_t cmd_buf[] = {
			ST7528_CMD_POWER_CONTROL | ST7528_POWER_CONTROL_VC |
				ST7528_POWER_CONTROL_VR | ST7528_POWER_CONTROL_VF,
			ST7528_CMD_SET_FRC_PWM | ST7528_FRC_4 |
				CONCAT(ST7528_PWM_, ST7528_PWM_LEVELS),

		};
		ret = st7528i_write_cmds(dev, cmd_buf, ARRAY_SIZE(cmd_buf));
		if (ret) {
			return ret;
		}
	}

	return st7528i_write_palette(dev);
}

static int st7528i_init(const struct device *dev)
{
	const struct st7528i_config *config = dev->config;
	int ret;

	if (config->height != ST7528_HEIGHT) {
		LOG_ERR("Invalid display height: expected %d, got %d", ST7528_HEIGHT,
			config->height);
		return -ENODEV;
	}

	if (config->width != ST7528_WIDTH) {
		LOG_ERR("Invalid display width: expected %d, got %d", ST7528_WIDTH, config->width);
		return -ENODEV;
	}

	if (!i2c_is_ready_dt(&config->bus)) {
		LOG_ERR("I2C bus %s not ready", config->bus.bus->name);
		return -ENODEV;
	}

	if (config->reset.port != NULL) {
		if (!device_is_ready(config->reset.port)) {
			LOG_ERR("Reset pin GPIO port not ready");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&config->reset, GPIO_OUTPUT_INACTIVE);
		if (ret) {
			LOG_ERR("Failed to configure reset pin");
			return ret;
		}
	}

	if (config->chip_select.port != NULL) {
		if (!device_is_ready(config->chip_select.port)) {
			LOG_ERR("Chip select pin GPIO port not ready");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&config->chip_select, GPIO_OUTPUT_ACTIVE);
		if (ret) {
			LOG_ERR("Failed to configure chip select pin");
			return ret;
		}
	}

	ret = st7528i_reset(dev);
	if (ret) {
		LOG_ERR("reset failed");
		return ret;
	}

	return 0;
}

static inline void st7528i_set_pixel(struct st7528i_data *data, const uint16_t col,
				     const uint16_t row, const uint8_t level)
{
	__ASSERT(col < ST7528_WIDTH, "width out of bounds");
	__ASSERT(row < ST7528_HEIGHT, "height out of bounds");

	const int page = row / 8;
	const int page_mask = 1 << (row % 8);
	uint8_t *ptr = &data->framebuffer[(page * ST7528_WIDTH + col) * ST7528_BPP];

	/* scatter the four bits of the level across the four "internal columns" that make up this
	 * column of pixels
	 */
	for (int level_column = 0; level_column < ST7528_BPP; level_column++) {
		/* first column is most significant bit */
		uint8_t level_bit = 1 << (ST7528_BPP - level_column - 1);

		if (level & level_bit) {
			ptr[level_column] |= page_mask;
		} else {
			ptr[level_column] &= ~page_mask;
		}
	}
}

static inline uint8_t st7528i_get_pixel(struct st7528i_data *data, const uint16_t col,
					const uint16_t row)
{
	__ASSERT(col < ST7528_WIDTH, "width out of bounds");
	__ASSERT(row < ST7528_HEIGHT, "height out of bounds");

	const int page = row / 8;
	const int page_mask = 1 << (row % 8);
	uint8_t *ptr = &data->framebuffer[(page * ST7528_WIDTH + col) * ST7528_BPP];
	uint8_t level = 0;

	/* gather the four bits of the level from the four "internal columns" that make up this
	 * column of pixels
	 */
	for (int level_column = 0; level_column < ST7528_BPP; level_column++) {
		/* first column is most significant bit */
		uint8_t level_bit = 1 << (ST7528_BPP - level_column - 1);

		if (ptr[level_column] & page_mask) {
			level |= level_bit;
		}
	}

	return level;
}

static int st7528i_write_columns(const struct device *dev, const uint16_t page,
				 const uint16_t start_col, const uint16_t num_cols)
{
	__ASSERT(page < ST7528_NUM_PAGES, "page out of bounds");
	__ASSERT(start_col + num_cols <= ST7528_WIDTH, "columns out of bounds");

	struct st7528i_data *data = dev->data;
	int ret;

	uint8_t cmd_buf[] = {
		ST7528_CMD_SET_PAGE | page,
		ST7528_CMD_SET_COL_MSN | (start_col >> 4),
		ST7528_CMD_SET_COL_LSN | (start_col & 0x0f),
	};

	ret = st7528i_write_cmds(dev, cmd_buf, ARRAY_SIZE(cmd_buf));
	if (ret) {
		return ret;
	}

	return st7528i_write_data(
		dev, &data->framebuffer[(page * ST7528_WIDTH + start_col) * ST7528_BPP],
		num_cols * ST7528_BPP);
}

/* -- API functions -- */

static int st7528i_blanking_on(const struct device *dev)
{
	uint8_t cmd_buf[] = {
		ST7528_CMD_DISPLAY_OFF,
	};
	return st7528i_write_cmds(dev, cmd_buf, ARRAY_SIZE(cmd_buf));
}

static int st7528i_blanking_off(const struct device *dev)
{
	uint8_t cmd_buf[] = {
		ST7528_CMD_DISPLAY_ON,
	};
	return st7528i_write_cmds(dev, cmd_buf, ARRAY_SIZE(cmd_buf));
}

static int st7528i_write(const struct device *dev, const uint16_t x_origin, const uint16_t y_origin,
			 const struct display_buffer_descriptor *desc, const void *buf)
{
	struct st7528i_data *data = dev->data;
	int ret;

	const uint8_t pixels_per_byte = 8 / ST7528_BPP;

	__ASSERT(desc->width <= desc->pitch, "Pitch is smaller than width");
	__ASSERT((desc->pitch * desc->height) / pixels_per_byte <= desc->buf_size,
		 "Input buffer too small");
	__ASSERT(desc->pitch % pixels_per_byte == 0, "Input buffer rows are not byte-aligned");

	LOG_DBG("Writing %dx%d (w,h) @ %dx%d (x,y)", desc->width, desc->height, x_origin, y_origin);

	const uint8_t *row_ptr = buf;
	const int last_row = y_origin + desc->height - 1;

	for (int row = y_origin; row <= last_row; row++) {
		for (int x = 0; x < desc->width; x++) {
			uint16_t col = x_origin + x;
			uint8_t value;

			if (x % pixels_per_byte == 0) {
				value = (row_ptr[x / pixels_per_byte] & 0xf0) >> ST7528_BPP;
			} else {
				value = row_ptr[x / pixels_per_byte] & 0x0f;
			}

			st7528i_set_pixel(data, col, row, value);
		}

		if (row % 8 == 7 || row == last_row) {
			/* last row of the current bank, send framebuffer to display */
			ret = st7528i_write_columns(dev, row / 8, x_origin, desc->width);
			if (ret) {
				return ret;
			}
		}

		row_ptr += (desc->pitch / pixels_per_byte);
	}

	return 0;
}

static int st7528i_read(const struct device *dev, const uint16_t x_origin, const uint16_t y_origin,
			const struct display_buffer_descriptor *desc, void *buf)
{
	struct st7528i_data *data = dev->data;
	const uint8_t pixels_per_byte = 8 / ST7528_BPP;

	__ASSERT(desc->width <= desc->pitch, "Pitch is smaller than width");
	__ASSERT((desc->pitch * desc->height) / pixels_per_byte <= desc->buf_size,
		 "Output buffer too small");
	__ASSERT(desc->pitch % pixels_per_byte == 0, "Output buffer rows are not byte-aligned");

	LOG_DBG("Reading %dx%d (w,h) @ %dx%d (x,y)", desc->width, desc->height, x_origin, y_origin);

	uint8_t *row_ptr = buf;

	for (uint16_t y = 0; y < desc->height; y++) {
		uint16_t row = y_origin + y;

		for (uint16_t x = 0; x < desc->width; x++) {
			uint16_t col = x_origin + x;
			uint8_t value = st7528i_get_pixel(data, col, row);

			if (x % pixels_per_byte == 0) {
				row_ptr[x / pixels_per_byte] &= ~0xf0;
				row_ptr[x / pixels_per_byte] |= value << ST7528_BPP;
			} else {
				row_ptr[x / pixels_per_byte] &= ~0x0f;
				row_ptr[x / pixels_per_byte] |= value;
			}
		}

		row_ptr += (desc->pitch / pixels_per_byte);
	}

	return 0;
}

static void *st7528i_get_framebuffer(const struct device *dev)
{
	return NULL;
}

static int st7528i_set_brightness(const struct device *dev, const uint8_t brightness)
{
	return -ENOTSUP;
}

static int st7528i_set_contrast(const struct device *dev, const uint8_t contrast)
{
	return -ENOTSUP;
}

static void st7528i_get_capabilities(const struct device *dev,
				     struct display_capabilities *capabilities)
{
	*capabilities = (struct display_capabilities){
		.x_resolution = ST7528_WIDTH,
		.y_resolution = ST7528_HEIGHT,
		.supported_pixel_formats = PIXEL_FORMAT_GRAY16,
		.screen_info = 0,
		.current_pixel_format = PIXEL_FORMAT_GRAY16,
		.current_orientation = DISPLAY_ORIENTATION_NORMAL,
	};
}

static int st7528i_set_pixel_format(const struct device *dev,
				    const enum display_pixel_format pixel_format)
{
	if (pixel_format == PIXEL_FORMAT_GRAY16) {
		return 0;
	}

	LOG_ERR("Pixel format change not implemented");

	return -ENOTSUP;
}

static int st7528i_set_orientation(const struct device *dev,
				   const enum display_orientation orientation)
{
	if (orientation == DISPLAY_ORIENTATION_NORMAL) {
		return 0;
	}

	LOG_ERR("Changing display orientation not implemented");

	return -ENOTSUP;
}

static const struct display_driver_api st7528i_api = {
	.blanking_on = st7528i_blanking_on,
	.blanking_off = st7528i_blanking_off,
	.write = st7528i_write,
	.read = st7528i_read,
	.get_framebuffer = st7528i_get_framebuffer,
	.set_brightness = st7528i_set_brightness,
	.set_contrast = st7528i_set_contrast,
	.get_capabilities = st7528i_get_capabilities,
	.set_pixel_format = st7528i_set_pixel_format,
	.set_orientation = st7528i_set_orientation,
};

#define ST7528I_INIT(inst)                                                                         \
	const static struct st7528i_config st7528i_config_##inst = {                               \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {}),                          \
		.chip_select = GPIO_DT_SPEC_INST_GET_OR(inst, chip_select_gpios, {}),              \
		.height = DT_INST_PROP(inst, height),                                              \
		.width = DT_INST_PROP(inst, width),                                                \
		.framerate = DT_INST_PROP(inst, framerate),                                        \
		.com_partial_display = DT_INST_PROP(inst, com_partial_display),                    \
		.com_offset = DT_INST_PROP(inst, com_offset),                                      \
		.invert_com = DT_INST_PROP(inst, invert_com),                                      \
		.invert_segments = DT_INST_PROP(inst, invert_segments),                            \
		.lcd_bias = DT_INST_PROP(inst, lcd_bias),                                          \
		.regulator_resistor = DT_INST_PROP(inst, regulator_resistor),                      \
		.electronic_volume = DT_INST_PROP(inst, electronic_volume),                        \
		.boost = DT_INST_PROP(inst, boost),                                                \
	};                                                                                         \
                                                                                                   \
	static struct st7528i_data st7528i_data_##inst = {                                         \
		.framebuffer = {0},                                                                \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, st7528i_init, NULL, &st7528i_data_##inst,                      \
			      &st7528i_config_##inst, POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY,   \
			      &st7528i_api);

DT_INST_FOREACH_STATUS_OKAY(ST7528I_INIT)
