/*
 * Copyright (c) 2023 Jan Henke <Jan.Henke@taujhe.de>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sparkfun_serlcd

#include <stdlib.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/auxdisplay.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(auxdisplay_serlcd, CONFIG_AUXDISPLAY_LOG_LEVEL);

/*
 * | in ASCII, used to begin a display command
 */
#define SERLCD_BEGIN_COMMAND 0x7C

/*
 * special command for the underlying display controller
 */
#define SERLCD_BEGIN_SPECIAL_COMMAND 0xFE

/*
 * maximum amount of custom chars the display supports
 */
#define SERLCD_CUSTOM_CHAR_MAX_COUNT 8

/*
 * height of a custom char in bits
 */
#define SERLCD_CUSTOM_CHAR_HEIGHT 8

/*
 * width of a custom char in bits
 */
#define SERLCD_CUSTOM_CHAR_WIDTH 5

/*
 * char code for the first custom char
 */
#define SERLCD_CUSTOM_CHAR_INDEX_BASE 0x08

/*
 * bitmask for custom character detection
 */
#define SERLCD_CUSTOM_CHAR_BITMASK 0xf8

/*
 * bit to set in the display control special command to indicate the display should be powered on
 */
#define SERLCD_DISPLAY_CONTROL_POWER_BIT BIT(2)

/*
 * bit to set in the display control special command to indicate the cursor should be displayed
 */
#define SERLCD_DISPLAY_CONTROL_CURSOR_BIT BIT(1)

/*
 * bit to set in the display control special command to indicate the cursor should be blinking
 */
#define SERLCD_DISPLAY_CONTROL_BLINKING_BIT BIT(0)

struct auxdisplay_serlcd_data {
	bool power;
	bool cursor;
	bool blinking;
	uint16_t cursor_x;
	uint16_t cursor_y;
};

struct auxdisplay_serlcd_config {
	struct auxdisplay_capabilities capabilities;
	struct i2c_dt_spec bus;
	uint16_t command_delay_ms;
	uint16_t special_command_delay_ms;
};

enum auxdisplay_serlcd_command {
	SERLCD_COMMAND_SET_CUSTOM_CHAR = 0x1B,
	SERLCD_COMMAND_WRITE_CUSTOM_CHAR = 0x23,
	SERLCD_COMMAND_CLEAR = 0x2D,
};

enum auxdisplay_serlcd_special_command {
	SERLCD_SPECIAL_RETURN_HOME = 0x02,
	SERLCD_SPECIAL_DISPLAY_CONTROL = 0x08,
	SERLCD_SPECIAL_SET_DD_RAM_ADDRESS = 0x80,
};

static int auxdisplay_serlcd_send_command(const struct device *dev,
					  const enum auxdisplay_serlcd_command command)
{
	const struct auxdisplay_serlcd_config *config = dev->config;
	const uint8_t buffer[2] = {SERLCD_BEGIN_COMMAND, command};

	int rc = i2c_write_dt(&config->bus, buffer, sizeof(buffer));

	k_sleep(K_MSEC(config->command_delay_ms));
	return rc;
}

static int
auxdisplay_serlcd_send_special_command(const struct device *dev,
				       const enum auxdisplay_serlcd_special_command command)
{
	const struct auxdisplay_serlcd_config *config = dev->config;
	const uint8_t buffer[2] = {SERLCD_BEGIN_SPECIAL_COMMAND, command};

	int rc = i2c_write_dt(&config->bus, buffer, sizeof(buffer));

	k_sleep(K_MSEC(config->special_command_delay_ms));
	return rc;
}

static int auxdisplay_serlcd_send_display_state(const struct device *dev,
						const struct auxdisplay_serlcd_data *data)
{
	uint8_t command = SERLCD_SPECIAL_DISPLAY_CONTROL;

	if (data->power) {
		command |= SERLCD_DISPLAY_CONTROL_POWER_BIT;
	}
	if (data->cursor) {
		command |= SERLCD_DISPLAY_CONTROL_CURSOR_BIT;
	}
	if (data->blinking) {
		command |= SERLCD_DISPLAY_CONTROL_BLINKING_BIT;
	}

	return auxdisplay_serlcd_send_special_command(dev, command);
}

static int auxdisplay_serlcd_display_on(const struct device *dev)
{
	struct auxdisplay_serlcd_data *data = dev->data;

	data->power = true;

	return auxdisplay_serlcd_send_display_state(dev, data);
}

static int auxdisplay_serlcd_display_off(const struct device *dev)
{
	struct auxdisplay_serlcd_data *data = dev->data;

	data->power = false;

	return auxdisplay_serlcd_send_display_state(dev, data);
}

static int auxdisplay_serlcd_cursor_set_enabled(const struct device *dev, bool enable)
{
	struct auxdisplay_serlcd_data *data = dev->data;

	data->cursor = enable;

	return auxdisplay_serlcd_send_display_state(dev, data);
}

static int auxdisplay_serlcd_position_blinking_set_enabled(const struct device *dev, bool enable)
{
	struct auxdisplay_serlcd_data *data = dev->data;

	data->blinking = enable;

	return auxdisplay_serlcd_send_display_state(dev, data);
}

static int auxdisplay_serlcd_cursor_position_set(const struct device *dev,
						 enum auxdisplay_position type, int16_t x,
						 int16_t y)
{
	static const uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54};

	const struct auxdisplay_serlcd_config *config = dev->config;
	const struct auxdisplay_capabilities capabilities = config->capabilities;
	const uint16_t columns = capabilities.columns;
	const uint16_t rows = capabilities.rows;
	struct auxdisplay_serlcd_data *data = dev->data;

	if (type == AUXDISPLAY_POSITION_ABSOLUTE) {
		/*
		 * shortcut for (0,0) position
		 */
		if (x == 0 && y == 0) {
			data->cursor_x = x;
			data->cursor_y = y;
			return auxdisplay_serlcd_send_special_command(dev,
								      SERLCD_SPECIAL_RETURN_HOME);
		}

		/*
		 * bounds checking
		 */
		if (x < 0 || x >= columns) {
			return -EINVAL;
		}
		if (y < 0 || y >= rows) {
			return -EINVAL;
		}

		data->cursor_x = x;
		data->cursor_y = y;

		const uint8_t cursor_address = x + row_offsets[y];

		return auxdisplay_serlcd_send_special_command(
			dev, SERLCD_SPECIAL_SET_DD_RAM_ADDRESS | cursor_address);

	} else if (type == AUXDISPLAY_POSITION_RELATIVE) {
		/*
		 * clip relative move to display dimensions
		 */
		const int new_x = (data->cursor_x + x) % columns;
		const int new_y = (data->cursor_y + y + x / columns) % rows;
		const uint16_t column = new_x < 0 ? new_x + columns : new_x;
		const uint16_t row = new_y < 0 ? new_y + rows : new_y;

		data->cursor_x = column;
		data->cursor_y = row;

		const uint8_t cursor_address = column + row_offsets[row];

		return auxdisplay_serlcd_send_special_command(
			dev, SERLCD_SPECIAL_SET_DD_RAM_ADDRESS | cursor_address);
	}

	/*
	 * other types of movement are not implemented/supported
	 */
	return -ENOSYS;
}

static int auxdisplay_serlcd_cursor_position_get(const struct device *dev, int16_t *x, int16_t *y)
{
	const struct auxdisplay_serlcd_data *data = dev->data;

	*x = (int16_t)data->cursor_x;
	*y = (int16_t)data->cursor_y;

	return 0;
}

static int auxdisplay_serlcd_capabilities_get(const struct device *dev,
					      struct auxdisplay_capabilities *capabilities)
{
	const struct auxdisplay_serlcd_config *config = dev->config;

	memcpy(capabilities, &config->capabilities, sizeof(struct auxdisplay_capabilities));

	return 0;
}

static int auxdisplay_serlcd_clear(const struct device *dev)
{
	const struct auxdisplay_serlcd_config *config = dev->config;

	int rc = auxdisplay_serlcd_send_command(dev, SERLCD_COMMAND_CLEAR);

	k_sleep(K_MSEC(config->command_delay_ms));
	return rc;
}

static int auxdisplay_serlcd_custom_character_set(const struct device *dev,
						  struct auxdisplay_character *character)
{
	const struct auxdisplay_serlcd_config *config = dev->config;
	int rc;

	/*
	 * only indexes 0..7 are supported
	 */
	const uint8_t char_index = character->index;

	if (char_index > (SERLCD_CUSTOM_CHAR_MAX_COUNT - 1)) {
		return -EINVAL;
	}

	/*
	 * custom characters are accessible via char codes 0x08..0x0f
	 */
	character->character_code = SERLCD_CUSTOM_CHAR_INDEX_BASE | char_index;

	rc = auxdisplay_serlcd_send_command(dev, SERLCD_COMMAND_SET_CUSTOM_CHAR + char_index);
	if (!rc) {
		return rc;
	}

	/*
	 * the display expects the custom character as 8 lines of 5 bit each, shades are not
	 * supported
	 */
	for (int l = 0; l < SERLCD_CUSTOM_CHAR_HEIGHT; ++l) {
		uint8_t buffer = 0;

		for (int i = 0; i < SERLCD_CUSTOM_CHAR_WIDTH; ++i) {
			if (character->data[(l * 5) + i]) {
				buffer |= BIT(4 - i);
			}
		}
		rc = i2c_write_dt(&config->bus, &buffer, sizeof(buffer));
		if (!rc) {
			return rc;
		}
	}

	return rc;
}

static void auxdisplay_serlcd_advance_current_position(const struct device *dev)
{
	const struct auxdisplay_serlcd_config *config = dev->config;
	struct auxdisplay_serlcd_data *data = dev->data;

	++(data->cursor_x);
	if (data->cursor_x >= config->capabilities.columns) {
		data->cursor_x = 0;
		++(data->cursor_y);
	}
	if (data->cursor_y >= config->capabilities.rows) {
		data->cursor_y = 0;
	}
}

static int auxdisplay_serlcd_write(const struct device *dev, const uint8_t *text, uint16_t len)
{
	const struct auxdisplay_serlcd_config *config = dev->config;

	int rc = 0;

	/*
	 * the display wraps around by itself, just write the text and update the position data
	 */
	for (int i = 0; i < len; ++i) {
		uint8_t character = text[i];

		/*
		 * customer characters require a special command, so check for custom char
		 */
		if ((character & SERLCD_CUSTOM_CHAR_BITMASK) == SERLCD_CUSTOM_CHAR_INDEX_BASE) {
			const uint8_t command = SERLCD_COMMAND_WRITE_CUSTOM_CHAR +
						(character & ~SERLCD_CUSTOM_CHAR_BITMASK);

			rc = auxdisplay_serlcd_send_command(dev, command);
			if (!rc) {
				return rc;
			}
			auxdisplay_serlcd_advance_current_position(dev);
		} else if (character == SERLCD_BEGIN_COMMAND ||
			   character == SERLCD_BEGIN_SPECIAL_COMMAND) {
			/*
			 * skip these characters in text, as they have a special meaning, if
			 * required a custom character can be used as replacement
			 */
			continue;
		} else {
			rc = i2c_write_dt(&config->bus, text, len);
			if (!rc) {
				return rc;
			}
			auxdisplay_serlcd_advance_current_position(dev);
		}
	}

	return rc;
}

static int auxdisplay_serlcd_init(const struct device *dev)
{
	const struct auxdisplay_serlcd_config *config = dev->config;
	struct auxdisplay_serlcd_data *data = dev->data;

	/*
	 * Initialize our data structure
	 */
	data->power = true;

	if (!device_is_ready(config->bus.bus)) {
		return -ENODEV;
	}

	auxdisplay_serlcd_clear(dev);

	return 0;
}

static DEVICE_API(auxdisplay, auxdisplay_serlcd_auxdisplay_api) = {
	.display_on = auxdisplay_serlcd_display_on,
	.display_off = auxdisplay_serlcd_display_off,
	.cursor_set_enabled = auxdisplay_serlcd_cursor_set_enabled,
	.position_blinking_set_enabled = auxdisplay_serlcd_position_blinking_set_enabled,
	.cursor_position_set = auxdisplay_serlcd_cursor_position_set,
	.cursor_position_get = auxdisplay_serlcd_cursor_position_get,
	.capabilities_get = auxdisplay_serlcd_capabilities_get,
	.clear = auxdisplay_serlcd_clear,
	.custom_character_set = auxdisplay_serlcd_custom_character_set,
	.write = auxdisplay_serlcd_write,
};

#define AUXDISPLAY_SERLCD_INST(inst)                                                               \
	static const struct auxdisplay_serlcd_config auxdisplay_serlcd_config_##inst = {           \
		.capabilities = {                                                                  \
				.columns = DT_INST_PROP(inst, columns),                            \
				.rows = DT_INST_PROP(inst, rows),                                  \
				.mode = 0,                                                         \
				.brightness.minimum = AUXDISPLAY_LIGHT_NOT_SUPPORTED,              \
				.brightness.maximum = AUXDISPLAY_LIGHT_NOT_SUPPORTED,              \
				.backlight.minimum = AUXDISPLAY_LIGHT_NOT_SUPPORTED,               \
				.backlight.maximum = AUXDISPLAY_LIGHT_NOT_SUPPORTED,               \
				.custom_characters = SERLCD_CUSTOM_CHAR_MAX_COUNT,                 \
				.custom_character_width = SERLCD_CUSTOM_CHAR_WIDTH,                \
				.custom_character_height = SERLCD_CUSTOM_CHAR_HEIGHT,              \
			},                                                                         \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.command_delay_ms = DT_INST_PROP(inst, command_delay_ms),                          \
		.special_command_delay_ms = DT_INST_PROP(inst, special_command_delay_ms),          \
	};                                                                                         \
                                                                                                   \
	static struct auxdisplay_serlcd_data auxdisplay_serlcd_data_##inst;                        \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &auxdisplay_serlcd_init, NULL, &auxdisplay_serlcd_data_##inst, \
			      &auxdisplay_serlcd_config_##inst, POST_KERNEL,                       \
			      CONFIG_AUXDISPLAY_INIT_PRIORITY, &auxdisplay_serlcd_auxdisplay_api);

DT_INST_FOREACH_STATUS_OKAY(AUXDISPLAY_SERLCD_INST)
