/*
 * Copyright (c) 2022 Jimmy Ou <yanagiis@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT maxim_max7219

/**
 * @file
 * @brief MAX7219 LED driver
 *
 * Datasheet: https://datasheets.maximintegrated.com/en/ds/MAX7219-MAX7221.pdf
 *
 * Limitations:
 *  1. This driver only implements no-decode mode.
 */

#include <zephyr/drivers/led.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/zephyr.h>

#define LOG_LEVEL CONFIG_LED_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(max7219);

#define MAX7219_SEGMENTS_PER_DIGIT  8
#define MAX7219_DIGITS_PER_DEVICE   8
#define MAX7219_SEGMENTS_PER_DEVICE (MAX7219_SEGMENTS_PER_DIGIT * MAX7219_DIGITS_PER_DEVICE)

/* MAX7219 registers and fields */
#define MAX7219_REG_NOOP 0x00
#define MAX7219_NOOP	 0x00

#define MAX7219_REG_DECODE_MODE 0x09
#define MAX7219_NO_DECODE	0x00

#define MAX7219_REG_INTENSITY 0x0A

#define MAX7219_REG_SCAN_LIMIT 0x0B

#define MAX7219_REG_SHUTDOWN	    0x0C
#define MAX7219_SHUTDOWN_MODE	    0x00
#define MAX7219_LEAVE_SHUTDOWN_MODE 0x01

#define MAX7219_REG_DISPLAY_TEST	0x0F
#define MAX7219_LEAVE_DISPLAY_TEST_MODE 0x00
#define MAX7219_DISPLAY_TEST_MODE	0x01

/*
 * led channel format:
 * aaaaaaaa aaaaaaaa aaaaaaaa aabbbccc
 *
 * a: index of max7219
 * b: index of digit
 * c: index of segment
 */

#define MAX7219_INDEX(led) ((led) & (~BIT_MASK(6))) >> 6
#define DIGIT_INDEX(led)   (((led)&BIT_MASK(6)) >> 3)
#define SEGMENT_INDEX(led) ((led)&BIT_MASK(3))

struct max7219_config {
	struct spi_dt_spec spi;
	uint32_t num_cascading;
	uint8_t intensity;
	uint8_t scan_limit;
};

struct max7219_data {
	/* Keep all digits for all cascading MAX7219 */
	uint8_t (*digits)[8];
	uint8_t *tx_buf;
};

static int max7219_write(const struct device *dev, uint32_t max7219_idx, uint8_t addr,
			 uint8_t value)
{
	const struct max7219_config *config = dev->config;
	struct max7219_data *data = dev->data;
	const struct spi_buf tx_buf = {
		.buf = data->tx_buf,
		.len = config->num_cascading * 2,
	};
	const struct spi_buf_set tx_bufs = {
		.buffers = &tx_buf,
		.count = 1U,
	};

	for (int i = 0; i < config->num_cascading; i++) {
		if (i != (config->num_cascading - 1 - max7219_idx)) {
			data->tx_buf[i * 2] = MAX7219_REG_NOOP;
			data->tx_buf[i * 2 + 1] = MAX7219_NOOP;
			continue;
		}

		data->tx_buf[i * 2] = addr;
		data->tx_buf[i * 2 + 1] = value;
	}

	return spi_write_dt(&config->spi, &tx_bufs);
}

static int max7219_update_led(const struct device *dev, uint32_t max7219_idx, uint8_t digit_idx,
			      uint8_t digit_data)
{
	/* led register begins from 0x1 */
	return max7219_write(dev, max7219_idx, digit_idx + 1, digit_data);
}

static int max7219_led_on(const struct device *dev, uint32_t led)
{
	const struct max7219_config *config = dev->config;
	struct max7219_data *data = dev->data;
	uint32_t max7219_idx;
	uint8_t digit_idx;
	uint8_t segment_idx;
	uint8_t digit_data;
	int ret;

	if (led >= config->num_cascading * MAX7219_SEGMENTS_PER_DEVICE) {
		return -EINVAL;
	}

	max7219_idx = MAX7219_INDEX(led);
	digit_idx = DIGIT_INDEX(led);
	segment_idx = SEGMENT_INDEX(led);

	digit_data = data->digits[max7219_idx][digit_idx] | (1 << segment_idx);
	ret = max7219_update_led(dev, max7219_idx, digit_idx, digit_data);
	if (ret != 0) {
		return ret;
	}

	data->digits[max7219_idx][digit_idx] = digit_data;
	return 0;
}

static int max7219_led_off(const struct device *dev, uint32_t led)
{
	const struct max7219_config *config = dev->config;
	struct max7219_data *data = dev->data;
	uint32_t max7219_idx;
	uint8_t digit_idx;
	uint8_t segment_idx;
	uint8_t digit_data;
	int ret;

	if (led >= config->num_cascading * MAX7219_SEGMENTS_PER_DEVICE) {
		return -EINVAL;
	}

	max7219_idx = MAX7219_INDEX(led);
	digit_idx = DIGIT_INDEX(led);
	segment_idx = SEGMENT_INDEX(led);

	digit_data = data->digits[max7219_idx][digit_idx] & ~(1 << segment_idx);
	ret = max7219_update_led(dev, max7219_idx, digit_idx, digit_data);
	if (ret != 0) {
		return ret;
	}

	data->digits[max7219_idx][digit_idx] = digit_data;
	return 0;
}

static const struct led_driver_api max7219_api = {
	.on = max7219_led_on,
	.off = max7219_led_off,
};

static int max7219_led_init(const struct device *dev)
{
	const struct max7219_config *config = dev->config;
	struct max7219_data *data = dev->data;
	int ret;

#if defined(CONFIG_MAX7219_INITIALIZATION_DELAY) && CONFIG_MAX7219_INITIALIZATION_DELAY > 0
	k_msleep(CONFIG_MAX7219_INITIALIZATION_DELAY);
#endif

	if (!device_is_ready(config->spi.bus)) {
		LOG_ERR("SPI device not ready");
		return -ENODEV;
	}

	for (int max7219_idx = 0; max7219_idx < config->num_cascading; max7219_idx++) {
		ret = max7219_write(dev, max7219_idx, MAX7219_REG_DISPLAY_TEST,
				    MAX7219_LEAVE_DISPLAY_TEST_MODE);
		if (ret < 0) {
			LOG_ERR("Failed to disable display test");
			return ret;
		}

		ret = max7219_write(dev, max7219_idx, MAX7219_REG_DECODE_MODE, MAX7219_NO_DECODE);
		if (ret < 0) {
			LOG_ERR("Failed to set decode mode");
			return ret;
		}

		ret = max7219_write(dev, max7219_idx, MAX7219_REG_INTENSITY, config->intensity);
		if (ret < 0) {
			LOG_ERR("Failed to set global brightness");
			return ret;
		}

		ret = max7219_write(dev, max7219_idx, MAX7219_REG_SCAN_LIMIT, config->scan_limit);
		if (ret < 0) {
			LOG_ERR("Failed to set scan limit");
			return ret;
		}

		ret = max7219_write(dev, max7219_idx, MAX7219_REG_SHUTDOWN,
				    MAX7219_LEAVE_SHUTDOWN_MODE);
		if (ret < 0) {
			LOG_ERR("Failed to leave shutdown state");
			return ret;
		}

		/* turn off all leds */
		memset(data->digits[max7219_idx], 0, MAX7219_DIGITS_PER_DEVICE * sizeof(uint8_t));
		for (int digit_idx = 0; digit_idx < MAX7219_DIGITS_PER_DEVICE; digit_idx++) {
			ret = max7219_update_led(dev, max7219_idx, digit_idx,
						 data->digits[max7219_idx][digit_idx]);
			if (ret < 0) {
				return ret;
			}
		}
	}

	return 0;
}

#define LED_MAX7219_INIT(n)                                                                        \
	static uint8_t max7219_digit_data_##n[DT_INST_PROP(n, num_cascading)][8];                  \
	static uint8_t max7219_tx_buf##n[DT_INST_PROP(n, num_cascading) * 2];                      \
	static struct max7219_data max7219_data_##n = {                                            \
		.digits = max7219_digit_data_##n,                                                  \
		.tx_buf = max7219_tx_buf##n,                                                       \
	};                                                                                         \
	static const struct max7219_config max7219_config_##n = {                                  \
		.spi = SPI_DT_SPEC_INST_GET(n, SPI_OP_MODE_MASTER | SPI_WORD_SET(8U), 0U),         \
		.num_cascading = DT_INST_PROP(n, num_cascading),                                   \
		.intensity = DT_INST_PROP(n, intensity),                                           \
		.scan_limit = DT_INST_PROP(n, scan_limit),                                         \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, max7219_led_init, NULL, &max7219_data_##n, &max7219_config_##n,   \
			      POST_KERNEL, CONFIG_LED_INIT_PRIORITY, &max7219_api);

DT_INST_FOREACH_STATUS_OKAY(LED_MAX7219_INIT)
