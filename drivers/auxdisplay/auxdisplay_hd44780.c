/*
 * Copyright (c) 2023 Jamie McCrae
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT hit_hd44780

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/auxdisplay.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(auxdisplay_hd44780, CONFIG_AUXDISPLAY_LOG_LEVEL);

#define AUXDISPLAY_HD44780_BACKLIGHT_MIN 0
#define AUXDISPLAY_HD44780_BACKLIGHT_MAX 1

#define AUXDISPLAY_HD44780_CUSTOM_CHARACTERS 8
#define AUXDISPLAY_HD44780_CUSTOM_CHARACTER_WIDTH 5
#define AUXDISPLAY_HD44780_CUSTOM_CHARACTER_HEIGHT 8

enum {
	AUXDISPLAY_HD44780_MODE_4_BIT = 0,
	AUXDISPLAY_HD44780_MODE_8_BIT = 1,

	/* Reserved for internal driver use only */
	AUXDISPLAY_HD44780_MODE_4_BIT_ONCE,
};

/* Display commands */
#define AUXDISPLAY_HD44780_CMD_CLEAR 0x01
#define AUXDISPLAY_HD44780_CMD_ENTRY_MODE 0x04
#define AUXDISPLAY_HD44780_CMD_DISPLAY_MODE 0x08
#define AUXDISPLAY_HD44780_CMD_CGRAM_SET 0x40
#define AUXDISPLAY_HD44780_CMD_POSITION_SET 0x80
#define AUXDISPLAY_HD44780_CMD_SETUP 0x20

#define AUXDISPLAY_HD44780_8_BIT_CONFIG 0x10
#define AUXDISPLAY_HD44780_2_LINE_CONFIG 0x08

#define AUXDISPLAY_HD44780_POSITION_BLINK_ENABLED 0x01
#define AUXDISPLAY_HD44780_CURSOR_ENABLED 0x02
#define AUXDISPLAY_HD44780_DISPLAY_ENABLED 0x04

#define AUXDISPLAY_HD44780_DISPLAY_SHIFT 0x01
#define AUXDISPLAY_HD44780_CURSOR_MOVE_RIGHT 0x02

struct auxdisplay_hd44780_data {
	uint16_t character_x;
	uint16_t character_y;
	bool cursor_enabled;
	bool position_blink_enabled;
	uint8_t direction;
	bool display_shift;
	bool backlight_state;
};

struct auxdisplay_hd44780_config {
	struct auxdisplay_capabilities capabilities;
	struct gpio_dt_spec rs_gpio;
	struct gpio_dt_spec rw_gpio;
	struct gpio_dt_spec e_gpio;
	struct gpio_dt_spec db_gpios[8];
	struct gpio_dt_spec backlight_gpio;
	uint8_t line_addresses[4];
	uint16_t enable_line_rise_delay;
	uint16_t enable_line_fall_delay;
	uint16_t clear_delay;
	uint16_t boot_delay;
};

static void auxdisplay_hd44780_set_entry_mode(const struct device *dev);
static void auxdisplay_hd44780_set_display_mode(const struct device *dev, bool enabled);

static void auxdisplay_hd44780_command(const struct device *dev, bool rs, uint8_t cmd,
				       uint8_t mode)
{
	const struct auxdisplay_hd44780_config *config = dev->config;
	int8_t i = 7;

	if (mode == AUXDISPLAY_HD44780_MODE_8_BIT) {
		while (i >= 0) {
			gpio_pin_set_dt(&config->db_gpios[i], ((cmd & BIT(i)) ? 1 : 0));
			--i;
		}
	} else {
		while (i >= 4) {
			gpio_pin_set_dt(&config->db_gpios[i], ((cmd & BIT(i)) ? 1 : 0));
			--i;
		}
	}

	gpio_pin_set_dt(&config->rs_gpio, rs);

	if (config->rw_gpio.port) {
		gpio_pin_set_dt(&config->rw_gpio, 0);
	}

	gpio_pin_set_dt(&config->e_gpio, 1);
	k_sleep(K_USEC(config->enable_line_rise_delay));
	gpio_pin_set_dt(&config->e_gpio, 0);
	k_sleep(K_USEC(config->enable_line_fall_delay));

	if (mode == AUXDISPLAY_HD44780_MODE_4_BIT) {
		while (i >= 0) {
			gpio_pin_set_dt(&config->db_gpios[(i + 4)], ((cmd & BIT(i)) ? 1 : 0));
			--i;
		}

		gpio_pin_set_dt(&config->e_gpio, 1);
		k_sleep(K_USEC(config->enable_line_rise_delay));
		gpio_pin_set_dt(&config->e_gpio, 0);
		k_sleep(K_USEC(config->enable_line_fall_delay));
	}
}

static int auxdisplay_hd44780_init(const struct device *dev)
{
	const struct auxdisplay_hd44780_config *config = dev->config;
	struct auxdisplay_hd44780_data *data = dev->data;
	int rc;
	uint8_t i = 0;
	uint8_t cmd = AUXDISPLAY_HD44780_CMD_SETUP | AUXDISPLAY_HD44780_8_BIT_CONFIG;

	if (config->capabilities.mode > AUXDISPLAY_HD44780_MODE_8_BIT) {
		/* This index is reserved for internal driver usage */
		LOG_ERR("HD44780 mode must be 4 or 8-bit");
		return -EINVAL;
	}

	/* Configure and set GPIOs */
	rc = gpio_pin_configure_dt(&config->rs_gpio, GPIO_OUTPUT);

	if (rc < 0) {
		LOG_ERR("Configuration of RS GPIO failed: %d", rc);
		return rc;
	}

	if (config->rw_gpio.port) {
		rc = gpio_pin_configure_dt(&config->rw_gpio, GPIO_OUTPUT);

		if (rc < 0) {
			LOG_ERR("Configuration of RW GPIO failed: %d", rc);
			return rc;
		}
	}

	rc = gpio_pin_configure_dt(&config->e_gpio, GPIO_OUTPUT);

	if (rc < 0) {
		LOG_ERR("Configuration of E GPIO failed: %d", rc);
		return rc;
	}

	if (config->capabilities.mode == AUXDISPLAY_HD44780_MODE_4_BIT) {
		i = 4;
	}

	while (i < 8) {
		if (config->db_gpios[i].port) {
			rc = gpio_pin_configure_dt(&config->db_gpios[i], GPIO_OUTPUT);

			if (rc < 0) {
				LOG_ERR("Configuration of DB%d GPIO failed: %d", i, rc);
				return rc;
			}
		} else if (config->capabilities.mode == AUXDISPLAY_HD44780_MODE_4_BIT && i > 3) {
			/* Required pin missing */
			LOG_ERR("Required DB%d pin missing (DB4-DB7 needed for 4-bit mode)", i);
			return -EINVAL;
		} else if (config->capabilities.mode == AUXDISPLAY_HD44780_MODE_8_BIT) {
			/* Required pin missing */
			LOG_ERR("Required DB%d pin missing", i);
			return -EINVAL;
		}

		++i;
	}

	if (config->backlight_gpio.port) {
		rc = gpio_pin_configure_dt(&config->backlight_gpio, GPIO_OUTPUT);

		if (rc < 0) {
			LOG_ERR("Configuration of backlight GPIO failed: %d", rc);
			return rc;
		}

		gpio_pin_set_dt(&config->backlight_gpio, 0);
	}

	data->character_x = 0;
	data->character_y = 0;
	data->backlight_state = false;
	data->cursor_enabled = false;
	data->position_blink_enabled = false;
	data->direction = AUXDISPLAY_DIRECTION_RIGHT;

	if (config->boot_delay != 0) {
		/* Boot delay is set, wait for a period of time for the LCD to become ready to
		 * accept commands
		 */
		k_sleep(K_MSEC(config->boot_delay));
	}

	if (config->capabilities.mode == AUXDISPLAY_HD44780_MODE_4_BIT) {
		/* Reset display to known state in 8-bit mode */
		auxdisplay_hd44780_command(dev, false, cmd, AUXDISPLAY_HD44780_MODE_4_BIT_ONCE);
		auxdisplay_hd44780_command(dev, false, cmd, AUXDISPLAY_HD44780_MODE_4_BIT_ONCE);

		/* Put display into 4-bit mode */
		cmd = AUXDISPLAY_HD44780_CMD_SETUP;
		auxdisplay_hd44780_command(dev, false, cmd, AUXDISPLAY_HD44780_MODE_4_BIT_ONCE);
	}

	if (config->capabilities.rows > 1) {
		cmd |= AUXDISPLAY_HD44780_2_LINE_CONFIG;
	}

	/* Configure display */
	auxdisplay_hd44780_command(dev, false, cmd, config->capabilities.mode);
	auxdisplay_hd44780_set_display_mode(dev, true);
	auxdisplay_hd44780_set_entry_mode(dev);
	auxdisplay_hd44780_command(dev, false, AUXDISPLAY_HD44780_CMD_CLEAR,
				   config->capabilities.mode);

	k_sleep(K_USEC(config->clear_delay));

	return 0;
}

static int auxdisplay_hd44780_capabilities_get(const struct device *dev,
					       struct auxdisplay_capabilities *capabilities)
{
	const struct auxdisplay_hd44780_config *config = dev->config;

	memcpy(capabilities, &config->capabilities, sizeof(struct auxdisplay_capabilities));

	return 0;
}

static int auxdisplay_hd44780_clear(const struct device *dev)
{
	const struct auxdisplay_hd44780_config *config = dev->config;
	struct auxdisplay_hd44780_data *data = dev->data;

	auxdisplay_hd44780_command(dev, false, AUXDISPLAY_HD44780_CMD_CLEAR,
				   config->capabilities.mode);

	data->character_x = 0;
	data->character_y = 0;

	k_sleep(K_USEC(config->clear_delay));

	return 0;
}

static void auxdisplay_hd44780_set_entry_mode(const struct device *dev)
{
	const struct auxdisplay_hd44780_config *config = dev->config;
	struct auxdisplay_hd44780_data *data = dev->data;
	uint8_t cmd = AUXDISPLAY_HD44780_CMD_ENTRY_MODE;

	if (data->direction == AUXDISPLAY_DIRECTION_RIGHT) {
		cmd |= AUXDISPLAY_HD44780_CURSOR_MOVE_RIGHT;
	}

	if (data->display_shift) {
		cmd |= AUXDISPLAY_HD44780_DISPLAY_SHIFT;
	}

	auxdisplay_hd44780_command(dev, false, cmd, config->capabilities.mode);
}

static void auxdisplay_hd44780_set_display_mode(const struct device *dev, bool enabled)
{
	const struct auxdisplay_hd44780_config *config = dev->config;
	struct auxdisplay_hd44780_data *data = dev->data;
	uint8_t cmd = AUXDISPLAY_HD44780_CMD_DISPLAY_MODE;

	if (data->cursor_enabled) {
		cmd |= AUXDISPLAY_HD44780_CURSOR_ENABLED;
	}

	if (data->position_blink_enabled) {
		cmd |= AUXDISPLAY_HD44780_POSITION_BLINK_ENABLED;
	}

	if (enabled) {
		cmd |= AUXDISPLAY_HD44780_DISPLAY_ENABLED;
	}

	auxdisplay_hd44780_command(dev, false, cmd, config->capabilities.mode);
}

static int auxdisplay_hd44780_display_on(const struct device *dev)
{
	auxdisplay_hd44780_set_display_mode(dev, true);
	return 0;
}

static int auxdisplay_hd44780_display_off(const struct device *dev)
{
	auxdisplay_hd44780_set_display_mode(dev, false);
	return 0;
}

static int auxdisplay_hd44780_cursor_set_enabled(const struct device *dev, bool enabled)
{
	struct auxdisplay_hd44780_data *data = dev->data;

	data->cursor_enabled = enabled;

	auxdisplay_hd44780_set_display_mode(dev, true);

	return 0;
}

static int auxdisplay_hd44780_position_blinking_set_enabled(const struct device *dev, bool enabled)
{
	struct auxdisplay_hd44780_data *data = dev->data;

	data->position_blink_enabled = enabled;

	auxdisplay_hd44780_set_display_mode(dev, true);

	return 0;
}

static int auxdisplay_hd44780_cursor_shift_set(const struct device *dev, uint8_t direction,
					       bool display_shift)
{
	struct auxdisplay_hd44780_data *data = dev->data;

	if (display_shift) {
		/* Not currently supported */
		return -EINVAL;
	}

	data->direction = direction;
	data->display_shift = (display_shift ? true : false);

	auxdisplay_hd44780_set_entry_mode(dev);

	return 0;
}

static int auxdisplay_hd44780_cursor_position_set(const struct device *dev,
						  enum auxdisplay_position type, int16_t x,
						  int16_t y)
{
	const struct auxdisplay_hd44780_config *config = dev->config;
	struct auxdisplay_hd44780_data *data = dev->data;
	uint8_t cmd = AUXDISPLAY_HD44780_CMD_POSITION_SET;

	if (type == AUXDISPLAY_POSITION_RELATIVE) {
		x += (int16_t)data->character_x;
		y += (int16_t)data->character_y;
	} else if (type == AUXDISPLAY_POSITION_RELATIVE_DIRECTION) {
		if (data->direction == AUXDISPLAY_DIRECTION_RIGHT) {
			x += (int16_t)data->character_x;
			y += (int16_t)data->character_y;
		} else {
			x -= (int16_t)data->character_x;
			y -= (int16_t)data->character_y;
		}
	}

	/* Check position is valid before applying */
	if (x < 0 || y < 0) {
		return -EINVAL;
	} else if (x >= config->capabilities.columns || y >= config->capabilities.rows) {
		return -EINVAL;
	}

	data->character_x = (uint16_t)x;
	data->character_y = (uint16_t)y;
	cmd |= config->line_addresses[data->character_y] + data->character_x;

	auxdisplay_hd44780_command(dev, false, cmd, config->capabilities.mode);

	return 0;
}

static int auxdisplay_hd44780_cursor_position_get(const struct device *dev, int16_t *x, int16_t *y)
{
	struct auxdisplay_hd44780_data *data = dev->data;

	*x = (int16_t)data->character_x;
	*y = (int16_t)data->character_y;

	return 0;
}

static int auxdisplay_hd44780_backlight_get(const struct device *dev, uint8_t *backlight)
{
	const struct auxdisplay_hd44780_config *config = dev->config;
	struct auxdisplay_hd44780_data *data = dev->data;

	if (!config->backlight_gpio.port) {
		return -ENOTSUP;
	}

	*backlight = (data->backlight_state == true ? 1 : 0);

	return 0;
}

static int auxdisplay_hd44780_backlight_set(const struct device *dev, uint8_t backlight)
{
	const struct auxdisplay_hd44780_config *config = dev->config;
	struct auxdisplay_hd44780_data *data = dev->data;

	if (!config->backlight_gpio.port) {
		return -ENOTSUP;
	}

	data->backlight_state = (bool)backlight;

	gpio_pin_set_dt(&config->backlight_gpio, (uint8_t)data->backlight_state);

	return 0;
}

static int auxdisplay_hd44780_custom_character_set(const struct device *dev,
						   struct auxdisplay_character *character)
{
	const struct auxdisplay_hd44780_config *config = dev->config;
	struct auxdisplay_hd44780_data *data = dev->data;
	uint8_t i = 0;
	uint8_t cmd = AUXDISPLAY_HD44780_CMD_CGRAM_SET | (character->index << 3);

	auxdisplay_hd44780_command(dev, false, cmd, config->capabilities.mode);

	/* HD44780 accepts 5x8 font but needs 8x8 data to be sent, mask off top 3 bits
	 * for each line sent
	 */
	while (i < 8) {
		uint8_t l = 0;

		cmd = 0;

		while (l < 5) {
			if (character->data[(i * 5) + (4 - l)]) {
				cmd |= BIT(l);
			}

			++l;
		}

		auxdisplay_hd44780_command(dev, true, cmd, config->capabilities.mode);

		++i;
	}

	character->character_code = character->index;

	/* Send last known address to switch back to DDRAM entry mode */
	cmd = AUXDISPLAY_HD44780_CMD_POSITION_SET |
	      (config->line_addresses[data->character_y] +
	       data->character_x);

	auxdisplay_hd44780_command(dev, false, cmd, config->capabilities.mode);

	return 0;
}

static int auxdisplay_hd44780_write(const struct device *dev, const uint8_t *text, uint16_t len)
{
	const struct auxdisplay_hd44780_config *config = dev->config;
	struct auxdisplay_hd44780_data *data = dev->data;
	uint16_t i = 0;

	while (i < len) {
		auxdisplay_hd44780_command(dev, true, text[i], config->capabilities.mode);
		++i;

		if (data->direction == AUXDISPLAY_DIRECTION_RIGHT) {
			/* Increment */
			++data->character_x;

			if (data->character_x == config->capabilities.columns) {
				data->character_x = 0;
				++data->character_y;

				if (data->character_y == config->capabilities.rows) {
					data->character_y = 0;
				}

				/* Send command to set position */
				uint8_t cmd = AUXDISPLAY_HD44780_CMD_POSITION_SET |
					      config->line_addresses[data->character_y];
				auxdisplay_hd44780_command(dev, false, cmd,
							config->capabilities.mode);
			}
		} else {
			/* Decrement */
			if (data->character_x == 0) {
				data->character_x = config->capabilities.columns - 1;

				if (data->character_y == 0) {
					data->character_y = config->capabilities.rows - 1;
				} else {
					--data->character_y;
				}

				/* Send command to set position */
				uint8_t cmd = AUXDISPLAY_HD44780_CMD_POSITION_SET |
					      (config->line_addresses[data->character_y] +
					       data->character_x);
				auxdisplay_hd44780_command(dev, false, cmd,
							config->capabilities.mode);
			} else {
				--data->character_x;
			}
		}
	}

	return 0;
}

static const struct auxdisplay_driver_api auxdisplay_hd44780_auxdisplay_api = {
	.display_on = auxdisplay_hd44780_display_on,
	.display_off = auxdisplay_hd44780_display_off,
	.cursor_set_enabled = auxdisplay_hd44780_cursor_set_enabled,
	.position_blinking_set_enabled = auxdisplay_hd44780_position_blinking_set_enabled,
	.cursor_shift_set = auxdisplay_hd44780_cursor_shift_set,
	.cursor_position_set = auxdisplay_hd44780_cursor_position_set,
	.cursor_position_get = auxdisplay_hd44780_cursor_position_get,
	.capabilities_get = auxdisplay_hd44780_capabilities_get,
	.clear = auxdisplay_hd44780_clear,
	.backlight_get = auxdisplay_hd44780_backlight_get,
	.backlight_set = auxdisplay_hd44780_backlight_set,
	.custom_character_set = auxdisplay_hd44780_custom_character_set,
	.write = auxdisplay_hd44780_write,
};

/* Returns desired value if backlight is enabled, otherwise returns not supported value */
#define BACKLIGHT_CHECK(inst, value)							\
	COND_CODE_1(DT_PROP_HAS_IDX(DT_DRV_INST(inst), backlight_gpios, 0), (value),	\
		    (AUXDISPLAY_LIGHT_NOT_SUPPORTED))

#define AUXDISPLAY_HD44780_DEVICE(inst)								\
	static struct auxdisplay_hd44780_data auxdisplay_hd44780_data_##inst;			\
	static const struct auxdisplay_hd44780_config auxdisplay_hd44780_config_##inst = {	\
		.capabilities = {								\
			.columns = DT_INST_PROP(inst, columns),					\
			.rows = DT_INST_PROP(inst, rows),					\
			.mode = DT_INST_ENUM_IDX(inst, mode),					\
			.brightness.minimum = AUXDISPLAY_LIGHT_NOT_SUPPORTED,			\
			.brightness.maximum = AUXDISPLAY_LIGHT_NOT_SUPPORTED,			\
			.backlight.minimum = BACKLIGHT_CHECK(inst,				\
							     AUXDISPLAY_HD44780_BACKLIGHT_MIN),	\
			.backlight.maximum = BACKLIGHT_CHECK(inst,				\
							     AUXDISPLAY_HD44780_BACKLIGHT_MAX),	\
			.custom_characters = AUXDISPLAY_HD44780_CUSTOM_CHARACTERS,		\
			.custom_character_width = AUXDISPLAY_HD44780_CUSTOM_CHARACTER_WIDTH,	\
			.custom_character_height = AUXDISPLAY_HD44780_CUSTOM_CHARACTER_HEIGHT,	\
		},										\
		.rs_gpio = GPIO_DT_SPEC_INST_GET(inst, register_select_gpios),			\
		.rw_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, read_write_gpios, {0}),		\
		.e_gpio = GPIO_DT_SPEC_INST_GET(inst, enable_gpios),				\
		.db_gpios[0] = GPIO_DT_SPEC_INST_GET_BY_IDX_OR(inst, data_bus_gpios, 0, {0}),	\
		.db_gpios[1] = GPIO_DT_SPEC_INST_GET_BY_IDX_OR(inst, data_bus_gpios, 1, {0}),	\
		.db_gpios[2] = GPIO_DT_SPEC_INST_GET_BY_IDX_OR(inst, data_bus_gpios, 2, {0}),	\
		.db_gpios[3] = GPIO_DT_SPEC_INST_GET_BY_IDX_OR(inst, data_bus_gpios, 3, {0}),	\
		.db_gpios[4] = GPIO_DT_SPEC_INST_GET_BY_IDX(inst, data_bus_gpios, 4),		\
		.db_gpios[5] = GPIO_DT_SPEC_INST_GET_BY_IDX(inst, data_bus_gpios, 5),		\
		.db_gpios[6] = GPIO_DT_SPEC_INST_GET_BY_IDX(inst, data_bus_gpios, 6),		\
		.db_gpios[7] = GPIO_DT_SPEC_INST_GET_BY_IDX(inst, data_bus_gpios, 7),		\
		.line_addresses[0] = DT_INST_PROP_BY_IDX(inst, line_addresses, 0),		\
		.line_addresses[1] = DT_INST_PROP_BY_IDX(inst, line_addresses, 1),		\
		.line_addresses[2] = DT_INST_PROP_BY_IDX(inst, line_addresses, 2),		\
		.line_addresses[3] = DT_INST_PROP_BY_IDX(inst, line_addresses, 3),		\
		.backlight_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, backlight_gpios, {0}),		\
		.enable_line_rise_delay = DT_INST_PROP(inst, enable_line_rise_delay_us),	\
		.enable_line_fall_delay = DT_INST_PROP(inst, enable_line_fall_delay_us),	\
		.clear_delay = DT_INST_PROP(inst, clear_command_delay_us),			\
		.boot_delay = DT_INST_PROP(inst, boot_delay_ms),				\
	};											\
	DEVICE_DT_INST_DEFINE(inst,								\
			&auxdisplay_hd44780_init,						\
			NULL,									\
			&auxdisplay_hd44780_data_##inst,					\
			&auxdisplay_hd44780_config_##inst,					\
			POST_KERNEL,								\
			CONFIG_AUXDISPLAY_INIT_PRIORITY,					\
			&auxdisplay_hd44780_auxdisplay_api);

DT_INST_FOREACH_STATUS_OKAY(AUXDISPLAY_HD44780_DEVICE)
