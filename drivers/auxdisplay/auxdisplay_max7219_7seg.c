/*
 * Copyright (c) 2025 Maksim Salau <maksim.salau@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT maxim_max7219_7seg

/**
 * @file
 * @brief MAX7219 8-Digit LED Display Driver
 *
 * Datasheet: https://datasheets.maximintegrated.com/en/ds/MAX7219-MAX7221.pdf
 *
 * Limitations:
 *  1. This driver only implements Code-B decode mode.
 */

#include <stddef.h>

#include <zephyr/drivers/auxdisplay.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
LOG_MODULE_REGISTER(auxdisplay_max7219_7seg, CONFIG_AUXDISPLAY_LOG_LEVEL);

#define MAX7219_DIGITS_PER_DEVICE 8

/* clang-format off */

/* MAX7219 registers and fields */
#define MAX7219_REG_NOOP		0x00
#define MAX7219_NOOP			0x00

#define MAX7219_REG_DIGIT0		0x01

#define MAX7219_CODE_B_DASH		0x0A
#define MAX7219_CODE_B_E		0x0B
#define MAX7219_CODE_B_H		0x0C
#define MAX7219_CODE_B_L		0x0D
#define MAX7219_CODE_B_P		0x0E
#define MAX7219_CODE_B_BLANK		0x0F
#define MAX7219_CODE_B_DP		BIT(7)

#define MAX7219_REG_DECODE_MODE		0x09
#define MAX7219_NO_DECODE		0x00
#define MAX7219_DECODE_ALL		0xFF

#define MAX7219_REG_INTENSITY		0x0A

#define MAX7219_REG_SCAN_LIMIT		0x0B

#define MAX7219_REG_SHUTDOWN		0x0C
#define MAX7219_SHUTDOWN_MODE		0x00
#define MAX7219_LEAVE_SHUTDOWN_MODE	0x01

#define MAX7219_REG_DISPLAY_TEST	0x0F
#define MAX7219_LEAVE_DISPLAY_TEST_MODE	0x00
#define MAX7219_DISPLAY_TEST_MODE	0x01

/* clang-format on */

struct max7219_7seg_config {
	struct spi_dt_spec spi;
	struct auxdisplay_capabilities capabilities;
	uint32_t num_cascading;
	uint8_t scan_limit;
	bool digit_order_reversed;
	uint8_t *digit_buf;
};

struct max7219_7seg_data {
	int16_t cursor;
	uint8_t brightness;
};

static int max7219_7seg_transmit_all(const struct device *dev, const uint8_t addr,
				     const uint8_t value)
{
	const struct max7219_7seg_config *dev_config = dev->config;
	uint16_t buf[dev_config->num_cascading];

	const struct spi_buf tx_buf = {
		.buf = buf,
		.len = sizeof(buf),
	};
	const struct spi_buf_set tx_bufs = {
		.buffers = &tx_buf,
		.count = 1U,
	};

	for (int i = 0; i < dev_config->num_cascading; i++) {
		buf[i] = sys_cpu_to_be16((addr << 8) | value);
	}

	return spi_write_dt(&dev_config->spi, &tx_bufs);
}

static int max7219_7seg_update_one_digit(const struct device *dev, const uint8_t position)
{
	const struct max7219_7seg_config *dev_config = dev->config;
	const uint8_t max7219_num = position / MAX7219_DIGITS_PER_DEVICE;
	const uint8_t max7219_idx = dev_config->num_cascading - max7219_num - 1;
	const uint8_t value = dev_config->digit_buf[position];
	const uint8_t reg_offset = position % MAX7219_DIGITS_PER_DEVICE;
	const uint8_t reg_addr =
		MAX7219_REG_DIGIT0 + (dev_config->digit_order_reversed
					      ? (MAX7219_DIGITS_PER_DEVICE - reg_offset - 1)
					      : reg_offset);
	uint16_t buf[dev_config->num_cascading];

	const struct spi_buf tx_buf = {
		.buf = buf,
		.len = sizeof(buf),
	};
	const struct spi_buf_set tx_bufs = {
		.buffers = &tx_buf,
		.count = 1U,
	};

	if (max7219_num >= dev_config->num_cascading) {
		LOG_ERR("Invalid digit number: %u", position);
		return -EINVAL;
	}

	for (int i = 0; i < dev_config->num_cascading; i++) {
		if (i == max7219_idx) {
			buf[i] = sys_cpu_to_be16((reg_addr << 8) | value);
		} else {
			buf[i] = sys_cpu_to_be16(MAX7219_REG_NOOP << 8);
		}
	}

	return spi_write_dt(&dev_config->spi, &tx_bufs);
}

static int max7219_7seg_display_on(const struct device *dev)
{
	return max7219_7seg_transmit_all(dev, MAX7219_REG_SHUTDOWN, MAX7219_LEAVE_SHUTDOWN_MODE);
}

static int max7219_7seg_display_off(const struct device *dev)
{
	return max7219_7seg_transmit_all(dev, MAX7219_REG_SHUTDOWN, MAX7219_SHUTDOWN_MODE);
}

static int max7219_7seg_brightness_get(const struct device *dev, uint8_t *brightness)
{
	struct max7219_7seg_data *dev_data = dev->data;

	if (brightness == NULL) {
		return -EINVAL;
	}

	*brightness = dev_data->brightness;
	return 0;
}

static int max7219_7seg_brightness_set(const struct device *dev, uint8_t brightness)
{
	const struct max7219_7seg_config *dev_config = dev->config;
	struct max7219_7seg_data *dev_data = dev->data;

	if (brightness < dev_config->capabilities.brightness.minimum ||
	    brightness > dev_config->capabilities.brightness.maximum) {
		return -EINVAL;
	}

	dev_data->brightness = brightness;

	return max7219_7seg_transmit_all(dev, MAX7219_REG_INTENSITY, brightness);
}

static int max7219_7seg_cursor_position_set(const struct device *dev, enum auxdisplay_position type,
					    int16_t x, int16_t y)
{
	const struct max7219_7seg_config *dev_config = dev->config;
	struct max7219_7seg_data *dev_data = dev->data;

	ARG_UNUSED(y);

	if (type == AUXDISPLAY_POSITION_ABSOLUTE) {
		/* The x value will be used as is */
	} else if (type == AUXDISPLAY_POSITION_RELATIVE) {
		x += dev_data->cursor;
	} else {
		return -EINVAL;
	}

	if (x < 0 || x > dev_config->capabilities.columns) {
		return -EINVAL;
	}

	dev_data->cursor = x;

	return 0;
}

static int max7219_7seg_cursor_position_get(const struct device *dev, int16_t *x, int16_t *y)
{
	struct max7219_7seg_data *dev_data = dev->data;

	*x = dev_data->cursor;
	*y = 0;

	return 0;
}

static int max7219_7seg_capabilities_get(const struct device *dev,
					 struct auxdisplay_capabilities *capabilities)
{
	const struct max7219_7seg_config *dev_config = dev->config;

	memcpy(capabilities, &dev_config->capabilities, sizeof(*capabilities));

	return 0;
}

static int max7219_7seg_clear(const struct device *dev)
{
	const struct max7219_7seg_config *dev_config = dev->config;
	struct max7219_7seg_data *dev_data = dev->data;
	int err;

	memset(dev_config->digit_buf, MAX7219_CODE_B_BLANK,
	       dev_config->num_cascading * MAX7219_DIGITS_PER_DEVICE);

	dev_data->cursor = 0;

	/* Transmit all values */
	for (unsigned int i = 0; i < MAX7219_DIGITS_PER_DEVICE; i++) {
		err = max7219_7seg_transmit_all(dev, MAX7219_REG_DIGIT0 + i, MAX7219_CODE_B_BLANK);
		if (err < 0) {
			return err;
		}
	}

	return 0;
}

static int max7219_7seg_write(const struct device *dev, const uint8_t *data, uint16_t len)
{
	const struct max7219_7seg_config *dev_config = dev->config;
	struct max7219_7seg_data *dev_data = dev->data;
	int err;

	while (len--) {
		const char c = *data++;
		uint8_t code;

		/* Decimal point should be added to the previous digit. */
		if (c == '.' && dev_data->cursor > 0) {
			dev_config->digit_buf[dev_data->cursor - 1] |= MAX7219_CODE_B_DP;

			err = max7219_7seg_update_one_digit(dev, dev_data->cursor - 1);
			if (err) {
				LOG_ERR("Failed to update digit at position %d: %d",
					dev_data->cursor - 1, err);
				return err;
			}

			continue;
		}

		/* Skip everything that doesn't fit onto the display */
		if (dev_data->cursor >= dev_config->capabilities.columns) {
			LOG_WRN("Reached the end of the display");
			break;
		}

		if (c >= '0' && c <= '9') {
			code = c - '0';
		} else if (c == ' ') {
			code = MAX7219_CODE_B_BLANK;
		} else if (c == '-') {
			code = MAX7219_CODE_B_DASH;
		} else if (c == 'H') {
			code = MAX7219_CODE_B_H;
		} else if (c == 'E') {
			code = MAX7219_CODE_B_E;
		} else if (c == 'L') {
			code = MAX7219_CODE_B_L;
		} else if (c == 'P') {
			code = MAX7219_CODE_B_P;
		} else if (c == '.') {
			/* Leading dot, leave the digit blank */
			code = MAX7219_CODE_B_BLANK | MAX7219_CODE_B_DP;
		} else {
			/* Unsupported symbol - leave it blank */
			LOG_WRN("Unsupported symbold: '%c' (%u)", c, c);
			code = MAX7219_CODE_B_BLANK;
		}

		dev_config->digit_buf[dev_data->cursor] = code;
		err = max7219_7seg_update_one_digit(dev, dev_data->cursor);
		if (err) {
			LOG_ERR("Failed to write digit at position %d: %d", dev_data->cursor, err);
			return err;
		}

		dev_data->cursor++;
	}

	return 0;
}

static DEVICE_API(auxdisplay, max7219_7seg_api) = {
	.display_on = max7219_7seg_display_on,
	.display_off = max7219_7seg_display_off,
	.brightness_get = max7219_7seg_brightness_get,
	.brightness_set = max7219_7seg_brightness_set,
	.cursor_position_set = max7219_7seg_cursor_position_set,
	.cursor_position_get = max7219_7seg_cursor_position_get,
	.capabilities_get = max7219_7seg_capabilities_get,
	.clear = max7219_7seg_clear,
	.write = max7219_7seg_write,
};

static int max7219_7seg_init(const struct device *dev)
{
	const struct max7219_7seg_config *dev_config = dev->config;
	struct max7219_7seg_data *dev_data = dev->data;
	int ret;

	if (!spi_is_ready_dt(&dev_config->spi)) {
		LOG_ERR("SPI device not ready");
		return -ENODEV;
	}

	ret = max7219_7seg_transmit_all(dev, MAX7219_REG_DISPLAY_TEST,
					MAX7219_LEAVE_DISPLAY_TEST_MODE);
	if (ret < 0) {
		LOG_ERR("Failed to disable display test");
		return ret;
	}

	ret = max7219_7seg_transmit_all(dev, MAX7219_REG_DECODE_MODE, MAX7219_DECODE_ALL);
	if (ret < 0) {
		LOG_ERR("Failed to set decode mode");
		return ret;
	}

	ret = max7219_7seg_transmit_all(dev, MAX7219_REG_INTENSITY, dev_data->brightness);
	if (ret < 0) {
		LOG_ERR("Failed to set global brightness");
		return ret;
	}

	ret = max7219_7seg_transmit_all(dev, MAX7219_REG_SCAN_LIMIT, dev_config->scan_limit);
	if (ret < 0) {
		LOG_ERR("Failed to set scan limit");
		return ret;
	}

	ret = max7219_7seg_clear(dev);
	if (ret < 0) {
		LOG_ERR("Failed to clear the display");
		return ret;
	}

	ret = max7219_7seg_transmit_all(dev, MAX7219_REG_SHUTDOWN, MAX7219_LEAVE_SHUTDOWN_MODE);
	if (ret < 0) {
		LOG_ERR("Failed to leave shutdown state");
		return ret;
	}

	return 0;
}

#define MAX7219_7SEG_NUM_CASCADING(n)                                                              \
	DIV_ROUND_UP((DT_INST_PROP(n, columns) * DT_INST_PROP(n, rows)), MAX7219_DIGITS_PER_DEVICE)

#define MAX7219_7SEG_INIT(n)                                                                       \
	static uint8_t max7219_7seg_digit_data_##n[MAX7219_7SEG_NUM_CASCADING(n) *                 \
						   MAX7219_DIGITS_PER_DEVICE];                     \
	static struct max7219_7seg_data max7219_7seg_data_##n = {                                  \
		.cursor = 0,                                                                       \
		.brightness = DT_INST_PROP(n, intensity),                                          \
	};                                                                                         \
	static const struct max7219_7seg_config max7219_7seg_config_##n = {                        \
		.spi = SPI_DT_SPEC_INST_GET(n, SPI_OP_MODE_MASTER | SPI_WORD_SET(8U)),             \
		.num_cascading = MAX7219_7SEG_NUM_CASCADING(n),                                    \
		.digit_order_reversed = DT_INST_PROP(n, digit_order_reversed),                     \
		.scan_limit = DT_INST_PROP(n, scan_limit),                                         \
		.capabilities =                                                                    \
			{                                                                          \
				.columns = DT_INST_PROP(n, columns),                               \
				.rows = DT_INST_PROP(n, rows),                                     \
				.brightness =                                                      \
					{                                                          \
						.minimum = 0,                                      \
						.maximum = 15,                                     \
					},                                                         \
			},                                                                         \
		.digit_buf = max7219_7seg_digit_data_##n,                                          \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, max7219_7seg_init, NULL, &max7219_7seg_data_##n,                  \
			      &max7219_7seg_config_##n, POST_KERNEL,                               \
			      CONFIG_AUXDISPLAY_INIT_PRIORITY, &max7219_7seg_api);

DT_INST_FOREACH_STATUS_OKAY(MAX7219_7SEG_INIT)
