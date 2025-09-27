/*
 * TM1637 7-segment display driver for Zephyr (auxdisplay)
 * Copyright (c) 2025 Siratul Islam
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT titanmicro_tm1637

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/auxdisplay.h>
#include <string.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(tm1637_auxdisplay, CONFIG_AUXDISPLAY_LOG_LEVEL);

/* TM1637 protocol commands */
#define TM1637_CMD_DATA_AUTO_INC 0x40
#define TM1637_CMD_ADDR_BASE     0xC0
#define TM1637_CMD_DISPLAY_CTRL  0x80

/* Segment mapping: A=bit0, B=bit1, C=bit2, D=bit3, E=bit4, F=bit5, G=bit6; DP=bit7 (0x80) */
static const uint8_t segment_codes[] = {
	0x3F, /* 0 */
	0x06, /* 1 */
	0x5B, /* 2 */
	0x4F, /* 3 */
	0x66, /* 4 */
	0x6D, /* 5 */
	0x7D, /* 6 */
	0x07, /* 7 */
	0x7F, /* 8 */
	0x6F, /* 9 */
	0x77, /* A */
	0x7C, /* b */
	0x39, /* C */
	0x5E, /* d */
	0x79, /* E */
	0x71, /* F */
	0x3D, /* G - same as C but with middle segment */
	0x76, /* H */
	0x30, /* I */
	0x1E, /* J */
	0x00, /* K - not easily representable */
	0x38, /* L */
	0x00, /* M - not easily representable */
	0x37, /* n */
	0x3F, /* O - same as 0 */
	0x73, /* P */
	0x67, /* q */
	0x50, /* r */
	0x6D, /* S - same as 5 */
	0x78, /* t */
	0x3E, /* U */
	0x00, /* V - not easily representable */
	0x00, /* W - not easily representable */
	0x76, /* X - same as H */
	0x6E, /* y */
	0x5B, /* Z - same as 2 */
};

static const uint8_t minus_code = 0x40; /* Segment G only */
#define DP_BIT (0x80)

struct tm1637_config {
	struct gpio_dt_spec clock_pin;
	struct gpio_dt_spec data_pin;
	uint32_t bit_delay_us;
	struct auxdisplay_capabilities capabilities;
};

struct tm1637_data {
	uint8_t current_brightness; /* bits 0-2: level (0-7), bit 3: enabled (1=on) */
	uint8_t display_buffer[4];  /* raw segment data for 4 digits */
	int16_t cursor_x;
	int16_t cursor_y;
};

/* === Low-level TM1637 protocol === */

static inline void tm1637_wait(const struct device *dev)
{
	const struct tm1637_config *cfg = dev->config;

	k_usleep(cfg->bit_delay_us);
}

static void tm1637_start_condition(const struct device *dev)
{
	const struct tm1637_config *cfg = dev->config;

	gpio_pin_configure_dt(&cfg->clock_pin, GPIO_OUTPUT);
	gpio_pin_configure_dt(&cfg->data_pin, GPIO_OUTPUT);

	gpio_pin_set_dt(&cfg->data_pin, 1);
	gpio_pin_set_dt(&cfg->clock_pin, 1);
	tm1637_wait(dev);

	gpio_pin_set_dt(&cfg->data_pin, 0);
	tm1637_wait(dev);

	gpio_pin_set_dt(&cfg->clock_pin, 0);
	tm1637_wait(dev);
}

static void tm1637_stop_condition(const struct device *dev)
{
	const struct tm1637_config *cfg = dev->config;

	gpio_pin_configure_dt(&cfg->clock_pin, GPIO_OUTPUT);
	gpio_pin_configure_dt(&cfg->data_pin, GPIO_OUTPUT);

	gpio_pin_set_dt(&cfg->data_pin, 0);
	gpio_pin_set_dt(&cfg->clock_pin, 1);
	tm1637_wait(dev);

	gpio_pin_set_dt(&cfg->data_pin, 1);
	tm1637_wait(dev);
}

static bool tm1637_send_byte(const struct device *dev, uint8_t data_byte)
{
	const struct tm1637_config *cfg = dev->config;

	uint8_t bit;

	gpio_pin_configure_dt(&cfg->clock_pin, GPIO_OUTPUT);

	for (int i = 0; i < 8; i++) {
		gpio_pin_set_dt(&cfg->clock_pin, 0);
		tm1637_wait(dev);

		bit = (data_byte >> i) & 0x01;
		if (bit) {
			gpio_pin_configure_dt(&cfg->data_pin, GPIO_INPUT);
		} else {
			gpio_pin_configure_dt(&cfg->data_pin, GPIO_OUTPUT);
			gpio_pin_set_dt(&cfg->data_pin, 0);
		}
		tm1637_wait(dev);

		gpio_pin_set_dt(&cfg->clock_pin, 1);
		tm1637_wait(dev);
	}

	/* Read ACK */
	gpio_pin_set_dt(&cfg->clock_pin, 0);
	gpio_pin_configure_dt(&cfg->data_pin, GPIO_INPUT);
	tm1637_wait(dev);

	gpio_pin_set_dt(&cfg->clock_pin, 1);
	tm1637_wait(dev);

	bool ack = !gpio_pin_get_dt(&cfg->data_pin);

	if (!ack) {
		gpio_pin_configure_dt(&cfg->data_pin, GPIO_OUTPUT);
		gpio_pin_set_dt(&cfg->data_pin, 0);
	}

	tm1637_wait(dev);
	gpio_pin_set_dt(&cfg->clock_pin, 0);
	return ack;
}

static int tm1637_update_display(const struct device *dev)
{
	const struct tm1637_config *cfg = dev->config;

	struct tm1637_data *data = dev->data;

	if (!device_is_ready(cfg->clock_pin.port) || !device_is_ready(cfg->data_pin.port)) {
		return -ENODEV;
	}

	/* Send data command */
	tm1637_start_condition(dev);
	tm1637_send_byte(dev, TM1637_CMD_DATA_AUTO_INC);
	tm1637_stop_condition(dev);

	/* Send segment data */
	tm1637_start_condition(dev);
	tm1637_send_byte(dev, TM1637_CMD_ADDR_BASE);

	for (int i = 0; i < 4; i++) {
		tm1637_send_byte(dev, data->display_buffer[i]);
	}

	tm1637_stop_condition(dev);

	/* Send display control */
	tm1637_start_condition(dev);
	tm1637_send_byte(dev, TM1637_CMD_DISPLAY_CTRL | (data->current_brightness & 0x0F));
	tm1637_stop_condition(dev);

	return 0;
}

/* === auxdisplay driver API === */

static int tm1637_auxdisplay_write(const struct device *dev, const uint8_t *buf, uint16_t len)
{
	struct tm1637_data *data = dev->data;
	const struct tm1637_config *cfg = dev->config;

	/* Clear the display buffer first */
	memset(data->display_buffer, 0, sizeof(data->display_buffer));

	uint32_t pos = 0;
	uint16_t i = 0;

	while (i < len && pos < 4) {
		char c = buf[i];

		uint8_t segment_code = 0;
		bool valid_char = false;

		if (c >= '0' && c <= '9') {
			segment_code = segment_codes[c - '0'];
			valid_char = true;
		} else if (c >= 'A' && c <= 'Z') {
			segment_code = segment_codes[c - 'A' + 10]; /* A starts at index 10 */
			valid_char = true;
		} else if (c >= 'a' && c <= 'z') {
			segment_code = segment_codes[c - 'a' + 10]; /* a starts at index 10 */
			valid_char = true;
		} else if (c == '-') {
			segment_code = minus_code;
			valid_char = true;
		} else if (c == ' ') {
			segment_code = 0;
			valid_char = true;
		}

		if (valid_char) {
			data->display_buffer[pos] = segment_code;

			/* Check if next character is a decimal point */
			if (i + 1 < len && buf[i + 1] == '.') {
				data->display_buffer[pos] |=
					DP_BIT; /* Add decimal point to current digit */
				i += 2;         /* Skip both the character and the '.' */
			} else {
				i++; /* Just move to next character */
			}
			pos++;
		} else {
			/* Skip unknown characters */
			i++;
		}
	}

	/* Reset cursor to end of valid data */
	data->cursor_x = pos;
	data->cursor_y = 0;

	return tm1637_update_display(dev);
}

static int tm1637_auxdisplay_clear(const struct device *dev)
{
	struct tm1637_data *data = dev->data;

	memset(data->display_buffer, 0, sizeof(data->display_buffer));
	data->cursor_x = 0;
	data->cursor_y = 0;

	return tm1637_update_display(dev);
}

static int tm1637_auxdisplay_set_brightness(const struct device *dev, uint8_t brightness)
{
	struct tm1637_data *data = dev->data;

	/* Clamp brightness to 0-7 and ensure display is ON */
	data->current_brightness = (brightness & 0x07) | 0x08;
	return tm1637_update_display(dev);
}

static int tm1637_auxdisplay_display_on(const struct device *dev)
{
	struct tm1637_data *data = dev->data;

	data->current_brightness |= 0x08;

	return tm1637_update_display(dev);
}

static int tm1637_auxdisplay_display_off(const struct device *dev)
{
	struct tm1637_data *data = dev->data;

	data->current_brightness &= ~0x08;

	return tm1637_update_display(dev);
}

static int tm1637_auxdisplay_cursor_position_set(const struct device *dev,
						 enum auxdisplay_position type, int16_t x,
						 int16_t y)
{
	const struct tm1637_config *cfg = dev->config;
	struct tm1637_data *data = dev->data;

	if (type == AUXDISPLAY_POSITION_RELATIVE) {
		x += data->cursor_x;
		y += data->cursor_y;
	} else if (type == AUXDISPLAY_POSITION_RELATIVE_DIRECTION) {
		return -ENOTSUP;
	}

	if (x < 0 || y < 0 || x >= cfg->capabilities.columns || y >= cfg->capabilities.rows) {
		return -EINVAL;
	}

	data->cursor_x = x;
	data->cursor_y = y;
	return 0;
}

static int tm1637_auxdisplay_cursor_position_get(const struct device *dev, int16_t *x, int16_t *y)
{
	struct tm1637_data *data = dev->data;

	*x = data->cursor_x;
	*y = data->cursor_y;
	return 0;
}

static int tm1637_auxdisplay_capabilities_get(const struct device *dev,
					      struct auxdisplay_capabilities *cap)
{
	const struct tm1637_config *cfg = dev->config;

	memcpy(cap, &cfg->capabilities, sizeof(*cap));
	return 0;
}

/* === Device initialization === */

static int tm1637_initialize(const struct device *dev)
{
	const struct tm1637_config *cfg = dev->config;
	struct tm1637_data *data = dev->data;

	if (!device_is_ready(cfg->clock_pin.port) || !device_is_ready(cfg->data_pin.port)) {
		return -ENODEV;
	}

	gpio_pin_configure_dt(&cfg->clock_pin, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&cfg->data_pin, GPIO_OUTPUT_INACTIVE);

	data->current_brightness = 0x08; /* enabled, min brightness */
	memset(data->display_buffer, 0, sizeof(data->display_buffer));
	data->cursor_x = 0;
	data->cursor_y = 0;

	return tm1637_auxdisplay_clear(dev);
}

static const struct auxdisplay_driver_api tm1637_auxdisplay_api = {
	.write = tm1637_auxdisplay_write,
	.clear = tm1637_auxdisplay_clear,
	.brightness_set = tm1637_auxdisplay_set_brightness,
	.display_on = tm1637_auxdisplay_display_on,
	.display_off = tm1637_auxdisplay_display_off,
	.cursor_position_set = tm1637_auxdisplay_cursor_position_set,
	.cursor_position_get = tm1637_auxdisplay_cursor_position_get,
	.capabilities_get = tm1637_auxdisplay_capabilities_get,
};

#define TM1637_INIT(n)                                                                             \
	static const struct tm1637_config tm1637_config_##n = {                                    \
		.clock_pin = GPIO_DT_SPEC_INST_GET(n, clk_gpios),                                  \
		.data_pin = GPIO_DT_SPEC_INST_GET(n, dio_gpios),                                   \
		.bit_delay_us = DT_INST_PROP_OR(n, bit_delay_us, 100),                             \
		.capabilities =                                                                    \
			{                                                                          \
				.columns = DT_INST_PROP_OR(n, columns, 4),                         \
				.rows = DT_INST_PROP_OR(n, rows, 1),                               \
			},                                                                         \
	};                                                                                         \
	static struct tm1637_data tm1637_data_##n;                                                 \
	DEVICE_DT_INST_DEFINE(n, tm1637_initialize, NULL, &tm1637_data_##n, &tm1637_config_##n,    \
			      POST_KERNEL, CONFIG_AUXDISPLAY_INIT_PRIORITY,                        \
			      &tm1637_auxdisplay_api);

DT_INST_FOREACH_STATUS_OKAY(TM1637_INIT)
