/*
 * Copyright (c) 2015 Intel Corporation
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * Copyright (c) 2022-2023 Jamie McCrae
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT jhd_jhd1313

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/auxdisplay.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(auxdisplay_jhd1313, CONFIG_AUXDISPLAY_LOG_LEVEL);

#define JHD1313_BACKLIGHT_ADDR		(0x62)

/* Defines for the JHD1313_CMD_CURSOR_SHIFT */
#define JHD1313_CS_DISPLAY_SHIFT	(1 << 3)
#define JHD1313_CS_RIGHT_SHIFT		(1 << 2)

/* Defines for the JHD1313_CMD_INPUT_SET to change text direction */
#define JHD1313_IS_INCREMENT		(1 << 1)
#define JHD1313_IS_DECREMENT		(0 << 1)
#define JHD1313_IS_SHIFT		(1 << 0)

/* Defines for the JHD1313_CMD_FUNCTION_SET */
#define JHD1313_FS_8BIT_MODE		(1 << 4)
#define JHD1313_FS_ROWS_2		(1 << 3)
#define JHD1313_FS_ROWS_1		(0 << 3)
#define JHD1313_FS_DOT_SIZE_BIG		(1 << 2)
#define JHD1313_FS_DOT_SIZE_LITTLE	(0 << 2)

/* LCD Display Commands */
#define JHD1313_CMD_SCREEN_CLEAR	(1 << 0)
#define JHD1313_CMD_CURSOR_RETURN	(1 << 1)
#define JHD1313_CMD_INPUT_SET		(1 << 2)
#define JHD1313_CMD_DISPLAY_SWITCH	(1 << 3)
#define JHD1313_CMD_CURSOR_SHIFT	(1 << 4)
#define JHD1313_CMD_FUNCTION_SET	(1 << 5)
#define JHD1313_CMD_SET_CGRAM_ADDR	(1 << 6)
#define JHD1313_CMD_SET_DDRAM_ADDR	(1 << 7)

#define JHD1313_DS_DISPLAY_ON		(1 << 2)
#define JHD1313_DS_CURSOR_ON		(1 << 1)
#define JHD1313_DS_BLINK_ON		(1 << 0)

#define JHD1313_LED_REG_R		0x04
#define JHD1313_LED_REG_G		0x03
#define JHD1313_LED_REG_B		0x02

#define JHD1313_LINE_FIRST		0x80
#define JHD1313_LINE_SECOND		0xC0

#define CLEAR_DELAY_MS			20
#define UPDATE_DELAY_MS			5

struct auxdisplay_jhd1313_data {
	uint8_t input_set;
	bool power;
	bool cursor;
	bool blinking;
	uint8_t function;
	uint8_t backlight;
};

struct auxdisplay_jhd1313_config {
	struct auxdisplay_capabilities capabilities;
	struct i2c_dt_spec bus;
};

static const uint8_t colour_define[][4] = {
	{ 0,   0,   0   },      /* Off */
	{ 255, 255, 255 },	/* White */
	{ 255, 0,   0   },      /* Red */
	{ 0,   255, 0   },      /* Green */
	{ 0,   0,   255 },      /* Blue */
};

static void auxdisplay_jhd1313_reg_set(const struct device *i2c, uint8_t addr, uint8_t data)
{
	uint8_t command[2] = { addr, data };

	i2c_write(i2c, command, sizeof(command), JHD1313_BACKLIGHT_ADDR);
}

static int auxdisplay_jhd1313_print(const struct device *dev, const uint8_t *data, uint16_t size)
{
	const struct auxdisplay_jhd1313_config *config = dev->config;
	uint8_t buf[] = { JHD1313_CMD_SET_CGRAM_ADDR, 0 };
	int rc = 0;
	int16_t i;

	for (i = 0; i < size; i++) {
		buf[1] = data[i];
		rc = i2c_write_dt(&config->bus, buf, sizeof(buf));
	}

	return rc;
}

static int auxdisplay_jhd1313_cursor_position_set(const struct device *dev,
						  enum auxdisplay_position type, int16_t x,
						  int16_t y)
{
	const struct auxdisplay_jhd1313_config *config = dev->config;
	unsigned char data[2];

	if (type != AUXDISPLAY_POSITION_ABSOLUTE) {
		return -EINVAL;
	}

	if (y == 0U) {
		x |= JHD1313_LINE_FIRST;
	} else {
		x |= JHD1313_LINE_SECOND;
	}

	data[0] = JHD1313_CMD_SET_DDRAM_ADDR;
	data[1] = x;

	return i2c_write_dt(&config->bus, data, 2);
}

static int auxdisplay_jhd1313_clear(const struct device *dev)
{
	int rc;
	const struct auxdisplay_jhd1313_config *config = dev->config;
	uint8_t clear[] = { 0, JHD1313_CMD_SCREEN_CLEAR };

	rc = i2c_write_dt(&config->bus, clear, sizeof(clear));
	LOG_DBG("Clear, delay 20 ms");

	k_sleep(K_MSEC(CLEAR_DELAY_MS));

	return rc;
}

static int auxdisplay_jhd1313_update_display_state(
				const struct auxdisplay_jhd1313_config *config,
				struct auxdisplay_jhd1313_data *data)
{
	int rc;
	uint8_t buf[] = { 0, JHD1313_CMD_DISPLAY_SWITCH };

	if (data->power) {
		buf[1] |= JHD1313_DS_DISPLAY_ON;
	}

	if (data->cursor) {
		buf[1] |= JHD1313_DS_CURSOR_ON;
	}

	if (data->blinking) {
		buf[1] |= JHD1313_DS_BLINK_ON;
	}

	rc = i2c_write_dt(&config->bus, buf, sizeof(buf));

	LOG_DBG("Set display_state options, delay 5 ms");
	k_sleep(K_MSEC(UPDATE_DELAY_MS));

	return rc;
}

static int auxdisplay_jhd1313_cursor_set_enabled(const struct device *dev, bool enabled)
{
	const struct auxdisplay_jhd1313_config *config = dev->config;
	struct auxdisplay_jhd1313_data *data = dev->data;

	data->cursor = enabled;
	return auxdisplay_jhd1313_update_display_state(config, data);
}

static int auxdisplay_jhd1313_position_blinking_set_enabled(const struct device *dev, bool enabled)
{
	const struct auxdisplay_jhd1313_config *config = dev->config;
	struct auxdisplay_jhd1313_data *data = dev->data;

	data->blinking = enabled;
	return auxdisplay_jhd1313_update_display_state(config, data);
}

static void auxdisplay_jhd1313_input_state_set(const struct device *dev, uint8_t opt)
{
	const struct auxdisplay_jhd1313_config *config = dev->config;
	struct auxdisplay_jhd1313_data *data = dev->data;
	uint8_t buf[] = { 0, 0 };

	data->input_set = opt;
	buf[1] = (opt | JHD1313_CMD_INPUT_SET);

	i2c_write_dt(&config->bus, buf, sizeof(buf));
	LOG_DBG("Set the input_set, no delay");
}

static int auxdisplay_jhd1313_backlight_set(const struct device *dev, uint8_t colour)
{
	const struct auxdisplay_jhd1313_config *config = dev->config;
	struct auxdisplay_jhd1313_data *data = dev->data;

	if (colour > ARRAY_SIZE(colour_define)) {
		LOG_WRN("Selected colour is too high a value");
		return -EINVAL;
	}

	data->backlight = colour;

	auxdisplay_jhd1313_reg_set(config->bus.bus, JHD1313_LED_REG_R, colour_define[colour][0]);
	auxdisplay_jhd1313_reg_set(config->bus.bus, JHD1313_LED_REG_G, colour_define[colour][1]);
	auxdisplay_jhd1313_reg_set(config->bus.bus, JHD1313_LED_REG_B, colour_define[colour][2]);

	return 0;
}

static int auxdisplay_jhd1313_backlight_get(const struct device *dev, uint8_t *backlight)
{
	struct auxdisplay_jhd1313_data *data = dev->data;

	*backlight = data->backlight;

	return 0;
}

static void auxdisplay_jhd1313_function_set(const struct device *dev, uint8_t opt)
{
	const struct auxdisplay_jhd1313_config *config = dev->config;
	struct auxdisplay_jhd1313_data *data = dev->data;
	uint8_t buf[] = { 0, 0 };

	data->function = opt;
	buf[1] = (opt | JHD1313_CMD_FUNCTION_SET);

	i2c_write_dt(&config->bus, buf, sizeof(buf));

	LOG_DBG("Set function options, delay 5 ms");
	k_sleep(K_MSEC(5));
}

static int auxdisplay_jhd1313_initialize(const struct device *dev)
{
	const struct auxdisplay_jhd1313_config *config = dev->config;
	struct auxdisplay_jhd1313_data *data = dev->data;
	uint8_t cmd;

	LOG_DBG("Initialize called");

	if (!device_is_ready(config->bus.bus)) {
		return -ENODEV;
	}

	/*
	 * Initialization sequence from the data sheet:
	 * 1 - Power on
	 *   - Wait for more than 30 ms AFTER VDD rises to 4.5v
	 * 2 - Send FUNCTION set
	 *   - Wait for 39 us
	 * 3 - Send DISPLAY Control
	 *   - wait for 39 us
	 * 4 - send DISPLAY Clear
	 *   - wait for 1.5 ms
	 * 5 - send ENTRY Mode
	 * 6 - Initialization is done
	 */

	/*
	 * We're here! Let's just make sure we've had enough time for the
	 * VDD to power on, so pause a little here, 30 ms min, so we go 50
	 */
	LOG_DBG("Delay 50 ms while the VDD powers on");
	k_sleep(K_MSEC(50));

	/* Configure everything for the display function first */
	cmd = JHD1313_CMD_FUNCTION_SET | JHD1313_FS_ROWS_2;
	auxdisplay_jhd1313_function_set(dev, cmd);

	/* Turn the display on - by default no cursor and no blinking */
	auxdisplay_jhd1313_update_display_state(config, data);

	/* Clear the screen */
	auxdisplay_jhd1313_clear(dev);

	/*
	 * Initialize to the default text direction for romance languages
	 * (increment, no shift)
	 */
	cmd = JHD1313_IS_INCREMENT;

	auxdisplay_jhd1313_input_state_set(dev, cmd);

	/* Now power on the background RGB control */
	LOG_INF("Configuring the RGB background");
	auxdisplay_jhd1313_reg_set(config->bus.bus, 0x00, 0x00);
	auxdisplay_jhd1313_reg_set(config->bus.bus, 0x01, 0x05);
	auxdisplay_jhd1313_reg_set(config->bus.bus, 0x08, 0xAA);

	/* Now set the background colour to black */
	LOG_DBG("Background set to off");
	auxdisplay_jhd1313_backlight_set(dev, 0);

	return 0;
}

static int auxdisplay_jhd1313_display_on(const struct device *dev)
{
	const struct auxdisplay_jhd1313_config *config = dev->config;
	struct auxdisplay_jhd1313_data *data = dev->data;

	data->power = true;
	return auxdisplay_jhd1313_update_display_state(config, data);
}

static int auxdisplay_jhd1313_display_off(const struct device *dev)
{
	const struct auxdisplay_jhd1313_config *config = dev->config;
	struct auxdisplay_jhd1313_data *data = dev->data;

	data->power = false;
	return auxdisplay_jhd1313_update_display_state(config, data);
}

static int auxdisplay_jhd1313_capabilities_get(const struct device *dev,
					       struct auxdisplay_capabilities *capabilities)
{
	const struct auxdisplay_jhd1313_config *config = dev->config;

	memcpy(capabilities, &config->capabilities, sizeof(struct auxdisplay_capabilities));

	return 0;
}

static DEVICE_API(auxdisplay, auxdisplay_jhd1313_auxdisplay_api) = {
	.display_on = auxdisplay_jhd1313_display_on,
	.display_off = auxdisplay_jhd1313_display_off,
	.cursor_set_enabled = auxdisplay_jhd1313_cursor_set_enabled,
	.position_blinking_set_enabled = auxdisplay_jhd1313_position_blinking_set_enabled,
	.cursor_position_set = auxdisplay_jhd1313_cursor_position_set,
	.capabilities_get = auxdisplay_jhd1313_capabilities_get,
	.clear = auxdisplay_jhd1313_clear,
	.backlight_get = auxdisplay_jhd1313_backlight_get,
	.backlight_set = auxdisplay_jhd1313_backlight_set,
	.write = auxdisplay_jhd1313_print,
};

#define AUXDISPLAY_JHD1313_DEVICE(inst)								\
	static const struct auxdisplay_jhd1313_config auxdisplay_jhd1313_config_##inst = {	\
		.capabilities = {								\
			.columns = 16,								\
			.rows = 2,								\
			.mode = 0,								\
			.brightness.minimum = AUXDISPLAY_LIGHT_NOT_SUPPORTED,			\
			.brightness.maximum = AUXDISPLAY_LIGHT_NOT_SUPPORTED,			\
			.backlight.minimum = 0,							\
			.backlight.maximum = ARRAY_SIZE(colour_define),				\
			.custom_characters = 0,							\
		},										\
		.bus = I2C_DT_SPEC_INST_GET(inst),						\
	};											\
	static struct auxdisplay_jhd1313_data auxdisplay_jhd1313_data_##inst = {		\
		.power = true,									\
		.cursor = false,								\
		.blinking = false,								\
	};											\
	DEVICE_DT_INST_DEFINE(inst,								\
			&auxdisplay_jhd1313_initialize,						\
			NULL,									\
			&auxdisplay_jhd1313_data_##inst,					\
			&auxdisplay_jhd1313_config_##inst,					\
			POST_KERNEL,								\
			CONFIG_AUXDISPLAY_INIT_PRIORITY,					\
			&auxdisplay_jhd1313_auxdisplay_api);

DT_INST_FOREACH_STATUS_OKAY(AUXDISPLAY_JHD1313_DEVICE)
