/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Smile
 */

#define DT_DRV_COMPAT titanmec_tm1650

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/auxdisplay.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(tm1650_auxdisplay, CONFIG_AUXDISPLAY_LOG_LEVEL);

/*
 * TM1650 protocol commands
 * Convert 8-bit TM1650 addresses to 7-bit I2C addresses.
 */
#define TM1650_CMD_DISP_ADDR_BASE (0x68 >> 1)
#define TM1650_CMD_SYS_ADDR       (0x48 >> 1)

/* TM1650 segment bit definitions */
#define TM1650_BLANK     (0)    /* No segments lit */
#define TM1650_DP_BIT    BIT(7) /* Decimal point */
#define TM1650_MINUS_BIT BIT(6) /* Segment G only */

/* TM1650 capabilities */
#define TM1650_BRIGHTNESS_MAX	7
#define TM1650_BRIGHTNESS_MIN	0

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

/* Look-up table matching brightness (0-7) to TM1650 brightness levels */
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
	struct i2c_dt_spec i2c;
};

struct tm1650_data {
	uint8_t display_buffer[4];  /* raw segment data for maximum 4 digits */
	int16_t cursor_x;
	int16_t cursor_y;
	uint8_t current_brightness; /* level (0-7) */
	bool display_on;
};

static inline int tm1650_write_cmd(const struct device *dev, uint8_t addr, uint8_t data_byte)
{
	const struct tm1650_config *cfg = dev->config;

	return i2c_write(cfg->i2c.bus, &data_byte, 1, addr);
}

static int tm1650_config_display(const struct device *dev)
{
	struct tm1650_data *data = dev->data;
	uint8_t sys_config = table_brightness[data->current_brightness] | data->display_on;

	return tm1650_write_cmd(dev, TM1650_CMD_SYS_ADDR, sys_config);
}

static int tm1650_write_display(const struct device *dev)
{
	struct tm1650_data *data = dev->data;
	const struct tm1650_config *cfg = dev->config;
	int err;

	for (int i = 0; i < cfg->capabilities.columns; i++) {
		err = tm1650_write_cmd(dev, TM1650_CMD_DISP_ADDR_BASE + i*0x1U,
				       data->display_buffer[i]);
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
			segment_code = TM1650_MINUS_BIT;
			valid_char = true;
		} else if (c == ' ') {
			segment_code = TM1650_BLANK;
			valid_char = true;
		} else {
			valid_char = false;
		}

		if (valid_char) {
			/* Check if next character is a decimal point */
			if (i + 1 < len && buf[i + 1] == '.') {
				segment_code |= TM1650_DP_BIT; /* Add decimal point */
				i += 2; /* Skip both the character and the '.' */
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

	memset(data->display_buffer, TM1650_BLANK, sizeof(data->display_buffer));
	data->cursor_x = 0;
	data->cursor_y = 0;

	return tm1650_write_display(dev);
}

static int tm1650_auxdisplay_set_brightness(const struct device *dev, uint8_t brightness)
{
	struct tm1650_data *data = dev->data;

	if (brightness > TM1650_BRIGHTNESS_MAX) {
		return -EINVAL;
	}
	data->current_brightness = brightness;

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

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->i2c.bus);
		return -ENODEV;
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

#define TM1650_INIT(n)                                                                             \
	static const struct tm1650_config tm1650_config_##n = {                                    \
		.capabilities =                                                                    \
			{                                                                          \
				.columns = DT_INST_PROP(n, columns),                               \
				.rows = DT_INST_PROP(n, rows),                                     \
				.brightness = {                                                    \
					.minimum = TM1650_BRIGHTNESS_MIN,                          \
					.maximum = TM1650_BRIGHTNESS_MAX,                          \
				},                                                                 \
			},                                                                         \
		.i2c = I2C_DT_SPEC_INST_GET(n),                                                    \
	};                                                                                         \
	static struct tm1650_data tm1650_data_##n;                                                 \
	DEVICE_DT_INST_DEFINE(n, tm1650_initialize, NULL, &tm1650_data_##n, &tm1650_config_##n,    \
			      POST_KERNEL, CONFIG_AUXDISPLAY_INIT_PRIORITY,                        \
			      &tm1650_auxdisplay_api);

DT_INST_FOREACH_STATUS_OKAY(TM1650_INIT)
