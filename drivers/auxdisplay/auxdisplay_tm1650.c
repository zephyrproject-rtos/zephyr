/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Smile
 */

#define DT_DRV_COMPAT titanmec_tm1650

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/auxdisplay.h>
#include <string.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(tm16350_auxdisplay, CONFIG_AUXDISPLAY_LOG_LEVEL);

#define TM1650_HAS_I2C  DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#define TM1650_HAS_GPIO DT_ANY_INST_HAS_PROP_STATUS_OKAY(clk_gpios)

/* TM1650 protocol commands */
#define TM1650_CMD_SYS_CMD	0x48
#define TM1650_CMD_ADDR_BASE	0x68

/* Display control register bits */
#define TM1650_DISPLAY_ON_MASK	BIT_MASK(1)
#define TM1650_BRIGHTNESS_MASK	BIT_MASK(3)

/* Segment bit definitions */
#define MINUS_BIT BIT(6) /* Segment G only */
#define DP_BIT    BIT(7) /* Decimal point */
#define BLANK     (0)    /* No segments lit */

/* Segment mapping: A=bit0, B=bit1, C=bit2, D=bit3, E=bit4, F=bit5, G=bit6; DP=bit7 */
static const uint8_t digit_segment_codes[] = {
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
};

/* Look-up table matching brightness (0-7) to TM1650 brightness levels*/
static const uint8_t table_brightness[] = {
	0x10, /* Level 1 */
	0x20, /* Level 2 */
	0x30, /* Level 3 */
	0x40, /* Level 4 */
	0x50, /* Level 5 */
	0x60, /* Level 6 */
	0x70, /* Level 7 */
	0x00, /* Level 8 */
};

struct tm1650_config {
	struct auxdisplay_capabilities capabilities;
	union {
#if TM1650_HAS_I2C
		struct i2c_dt_spec i2c;
#endif /* TM1650_HAS_I2C */
#if TM1650_HAS_GPIO
		struct {
			struct gpio_dt_spec clock_pin;
			struct gpio_dt_spec data_pin;
			uint16_t bit_delay_us;
		};
#endif /* TM1650_HAS_GPIO */
	};
	bool is_i2c;
};

struct tm1650_data {
	uint8_t display_buffer[4];  /* raw segment data for maximum 4 digits */
	int16_t cursor_x;
	int16_t cursor_y;
	uint8_t current_brightness; /* level (0-7) */
	bool display_on;
};

/* Low-level TM1650 protocol */
#if TM1650_HAS_GPIO

static inline void tm1650_wait(const struct device *dev)
{
	const struct tm1650_config *cfg = dev->config;

	k_usleep(cfg->bit_delay_us);
}

static void tm1650_start_condition(const struct device *dev)
{
	const struct tm1650_config *cfg = dev->config;

	gpio_pin_configure_dt(&cfg->data_pin, GPIO_OUTPUT);

	gpio_pin_set_dt(&cfg->data_pin, 1);
	gpio_pin_set_dt(&cfg->clock_pin, 1);
	tm1650_wait(dev);

	gpio_pin_set_dt(&cfg->data_pin, 0);
	tm1650_wait(dev);
}

static void tm1650_stop_condition(const struct device *dev)
{
	const struct tm1650_config *cfg = dev->config;

	gpio_pin_configure_dt(&cfg->data_pin, GPIO_OUTPUT);

	gpio_pin_set_dt(&cfg->data_pin, 0);
	gpio_pin_set_dt(&cfg->clock_pin, 1);
	tm1650_wait(dev);

	gpio_pin_set_dt(&cfg->data_pin, 1);
	tm1650_wait(dev);
}

static int tm1650_send_byte(const struct device *dev, uint8_t data_byte)
{
	const struct tm1650_config *cfg = dev->config;
	bool ack;

	gpio_pin_configure_dt(&cfg->data_pin, GPIO_OUTPUT);

	for (int i = 7; i >= 0; i--) {
		gpio_pin_set_dt(&cfg->clock_pin, 0);
		tm1650_wait(dev);

		gpio_pin_set_dt(&cfg->data_pin, IS_BIT_SET(data_byte, i));
		gpio_pin_set_dt(&cfg->clock_pin, 1);
		tm1650_wait(dev);
	}

	/* Read ACK */
	gpio_pin_set_dt(&cfg->clock_pin, 0);
	gpio_pin_configure_dt(&cfg->data_pin, GPIO_INPUT);
	tm1650_wait(dev);

	gpio_pin_set_dt(&cfg->clock_pin, 1);
	tm1650_wait(dev);

	ack = !gpio_pin_get_dt(&cfg->data_pin);

	gpio_pin_set_dt(&cfg->clock_pin, 0);

	if (!ack) {
		LOG_ERR("No ack received from device %s", dev->name);
		return -EIO;
	}
	return 0;
}

#endif /* TM1650_HAS_GPIO */

static int tm1650_write_cmd(const struct device *dev, uint8_t tm_addr, uint8_t data_byte)
{
	const struct tm1650_config *cfg = dev->config;
	int err = 0;

	if (cfg->is_i2c) {
#if TM1650_HAS_I2C
		/* Base addresse 0x48 shifted right by 1 bit for correct I2C addressing*/
		err = i2c_write(cfg->i2c.bus, &data_byte, 1, tm_addr >> 1);
#else
		err = -ENOTSUP;
#endif /* TM1650_HAS_I2C */
	} else {
#if TM1650_HAS_GPIO
		/* Bit banging */
		tm1650_start_condition(dev);
		err = tm1650_send_byte(dev, tm_addr);
		err |= tm1650_send_byte(dev, data_byte);
		tm1650_stop_condition(dev);
#else
		err = -ENOTSUP;
#endif /* TM1650_HAS_GPIO */
	}
	return err;
}

static int tm1650_config_display(const struct device *dev)
{
	struct tm1650_data *data = dev->data;
	uint8_t sys_config = (table_brightness[data->current_brightness] |
			    (data->display_on & TM1650_DISPLAY_ON_MASK));

	return tm1650_write_cmd(dev, TM1650_CMD_SYS_CMD, sys_config);
}

static int tm1650_write_display(const struct device *dev)
{
	struct tm1650_data *data = dev->data;
	const struct tm1650_config *cfg = dev->config;
	int err = 0;

	for (int i = 0; i < cfg->capabilities.columns; i++) {
		err = tm1650_write_cmd(dev, TM1650_CMD_ADDR_BASE + i*0x2U, data->display_buffer[i]);
		if (err != 0) {
			return err;
		}
	}
	return 0;
}

/* auxdisplay driver API */

static int tm1650_auxdisplay_write(const struct device *dev, const uint8_t *buf, uint16_t len)
{
	struct tm1650_data *data = dev->data;
	const struct tm1650_config *cfg = dev->config;
	uint32_t pos = 0;
	uint16_t i = 0;

	while (i < len && pos < cfg->capabilities.columns) {
		char c = buf[i];
		uint8_t segment_code = 0;
		bool valid_char = false;

		if (c >= '0' && c <= '9') {
			segment_code = digit_segment_codes[c - '0'];
			valid_char = true;
		} else if (c == '-') {
			segment_code = MINUS_BIT;
			valid_char = true;
		} else if (c == ' ') {
			segment_code = BLANK;
			valid_char = true;
		} else {
			valid_char = false;
		}

		if (valid_char) {
			/* Check if next character is a decimal point */
			if (i + 1 < len && buf[i + 1] == '.') {
				segment_code |= DP_BIT; /* Add decimal point to current digit */
				i += 2;         /* Skip both the character and the '.' */
			} else {
				i++; /* Move to next character */
			}

			data->display_buffer[data->cursor_x] = segment_code;
			data->cursor_x = (data->cursor_x + 1) % cfg->capabilities.columns;
		} else {
			/* Skip unknown characters */
			i++;
		}
	}


	return tm1650_write_display(dev);
}

static int tm1650_auxdisplay_clear(const struct device *dev)
{
	struct tm1650_data *data = dev->data;

	memset(data->display_buffer, 0, sizeof(data->display_buffer));
	data->cursor_x = 0;
	data->cursor_y = 0;

	return tm1650_write_display(dev);
}

static int tm1650_auxdisplay_set_brightness(const struct device *dev, uint8_t brightness)
{
	struct tm1650_data *data = dev->data;

	/* Clamp brightness to 0-7 */
	data->current_brightness = brightness & TM1650_BRIGHTNESS_MASK;

	return tm1650_config_display(dev);
}

static int tm1650_auxdisplay_get_brightness(const struct device *dev, uint8_t *brightness)
{
	struct tm1650_data *data = dev->data;

	*brightness = data->current_brightness;
	return 0;
}

static int tm1650_auxdisplay_display_on(const struct device *dev)
{
	struct tm1650_data *data = dev->data;

	data->display_on = true;

	return tm1650_config_display(dev);
}

static int tm1650_auxdisplay_display_off(const struct device *dev)
{
	struct tm1650_data *data = dev->data;

	data->display_on = false;

	return tm1650_config_display(dev);
}

static int tm1650_auxdisplay_cursor_position_set(const struct device *dev,
						 enum auxdisplay_position type,
						 int16_t x, int16_t y)
{
	const struct tm1650_config *cfg = dev->config;
	struct tm1650_data *data = dev->data;

	switch (type) {
	case AUXDISPLAY_POSITION_RELATIVE:
		x += data->cursor_x;
		y += data->cursor_y;
		break;
	case AUXDISPLAY_POSITION_RELATIVE_DIRECTION:
		return -ENOTSUP;
	case AUXDISPLAY_POSITION_ABSOLUTE:
		/* x, y already in absolute coordinates */
		break;
	default:
		return -EINVAL;
	}

	if (x < 0 || y < 0 || x >= cfg->capabilities.columns || y >= cfg->capabilities.rows) {
		return -EINVAL;
	}

	data->cursor_x = x;
	data->cursor_y = y;
	return 0;
}

static int tm1650_auxdisplay_cursor_position_get(const struct device *dev, int16_t *x, int16_t *y)
{
	struct tm1650_data *data = dev->data;

	*x = data->cursor_x;
	*y = data->cursor_y;
	return 0;
}

static int tm1650_auxdisplay_capabilities_get(const struct device *dev,
					      struct auxdisplay_capabilities *cap)
{
	const struct tm1650_config *cfg = dev->config;

	memcpy(cap, &cfg->capabilities, sizeof(*cap));
	return 0;
}

/* Device initialization */

static int tm1650_initialize(const struct device *dev)
{
	const struct tm1650_config *cfg = dev->config;
	struct tm1650_data *data = dev->data;

	if (cfg->is_i2c) {
#if TM1650_HAS_I2C
		if (!device_is_ready(cfg->i2c.bus)) {
			LOG_ERR_DEVICE_NOT_READY(cfg->i2c.bus);
			return -ENODEV;
		}
#endif /* TM1650_HAS_I2C */
	} else {
#if TM1650_HAS_GPIO
		if (!gpio_is_ready_dt(&cfg->clock_pin) || !gpio_is_ready_dt(&cfg->data_pin)) {
			LOG_ERR("GPIOs not ready");
			return -ENODEV;
		}

		gpio_pin_configure_dt(&cfg->clock_pin, GPIO_OUTPUT_INACTIVE);
		gpio_pin_configure_dt(&cfg->data_pin, GPIO_OUTPUT_INACTIVE);
#endif /* TM1650_HAS_GPIO */
	}

	data->display_on = true;
	data->current_brightness = cfg->capabilities.brightness.minimum;

	if (tm1650_config_display(dev) != 0) {
		LOG_ERR("Failed to configure display");
		return -ENXIO;
	}

	if (tm1650_auxdisplay_clear(dev) != 0) {
		LOG_ERR("Failed to clear display");
		return -ENXIO;
	}
	return 0;
}

static DEVICE_API(auxdisplay, tm1650_auxdisplay_api) = {
	.write = tm1650_auxdisplay_write,
	.clear = tm1650_auxdisplay_clear,
	.brightness_set = tm1650_auxdisplay_set_brightness,
	.brightness_get = tm1650_auxdisplay_get_brightness,
	.display_on = tm1650_auxdisplay_display_on,
	.display_off = tm1650_auxdisplay_display_off,
	.cursor_position_set = tm1650_auxdisplay_cursor_position_set,
	.cursor_position_get = tm1650_auxdisplay_cursor_position_get,
	.capabilities_get = tm1650_auxdisplay_capabilities_get,
};

#define TM1650_CONFIG_I2C(n)                                                                       \
	.is_i2c = true,                                                                            \
	.i2c = I2C_DT_SPEC_INST_GET(n),

#define TM1650_CONFIG_GPIO(n)                                                                      \
	.is_i2c = false,                                                                           \
	.clock_pin = GPIO_DT_SPEC_INST_GET(n, clk_gpios),                                          \
	.data_pin = GPIO_DT_SPEC_INST_GET(n, dio_gpios),                                           \
	.bit_delay_us = DT_INST_PROP_OR(n, bit_delay_us, 100),

#define TM1650_INIT(n)                                                                             \
	static const struct tm1650_config tm1650_config_##n = {                                    \
		.capabilities =                                                                    \
			{                                                                          \
				.columns = DT_INST_PROP(n, columns),                               \
				.rows = 1,                                                         \
				.brightness = {                                                    \
					.minimum = 0,                                              \
					.maximum = 7,                                              \
				},                                                                 \
			},                                                                         \
		COND_CODE_1(DT_INST_ON_BUS(n, i2c),                                                \
			   (TM1650_CONFIG_I2C(n)),                                                 \
			   (TM1650_CONFIG_GPIO(n)))                                                \
	};                                                                                         \
	static struct tm1650_data tm1650_data_##n;                                                 \
	DEVICE_DT_INST_DEFINE(n, tm1650_initialize, NULL, &tm1650_data_##n, &tm1650_config_##n,    \
			      POST_KERNEL, CONFIG_AUXDISPLAY_INIT_PRIORITY,                        \
			      &tm1650_auxdisplay_api);

DT_INST_FOREACH_STATUS_OKAY(TM1650_INIT)
