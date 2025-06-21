/*
 * Copyright (c) 2025 Tinu K Cheriya tinuk11c@gmail.com
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/drivers/misc/tm1637/tm1637.h>

#define DT_DRV_COMPAT hw_tm1637

#define TM1637_DELAY_US 5
#define TM1637_CMD_DATA_AUTO 0x40
#define TM1637_CMD_ADDR 0xC0
#define TM1637_CMD_DISPLAY 0x88

LOG_MODULE_REGISTER(tm1637, CONFIG_LOG_DEFAULT_LEVEL);

/**
   Configuration structure for TM1637.
 */
struct tm1637_config {
	struct gpio_dt_spec clk;
	struct gpio_dt_spec dio;
};

/**

/**
   Delay for TM1637 communication.
 */
static void tm1637_delay(void)
{
	k_busy_wait(TM1637_DELAY_US);
}

/**
   Start TM1637 communication.
 
  return 0 on success, negative errno on failure.
 */
static int tm1637_start(const struct tm1637_config *cfg)
{
	int ret;

	ret = gpio_pin_set_dt(&cfg->dio, 1);
	if (ret < 0) {
		LOG_ERR("Failed to set DIO high: %d", ret);
		return ret;
	}
	ret = gpio_pin_set_dt(&cfg->clk, 1);
	if (ret < 0) {
		LOG_ERR("Failed to set CLK high: %d", ret);
		return ret;
	}
	tm1637_delay();
	ret = gpio_pin_set_dt(&cfg->dio, 0);
	if (ret < 0) {
		LOG_ERR("Failed to set DIO low: %d", ret);
		return ret;
	}
	tm1637_delay();
	ret = gpio_pin_set_dt(&cfg->clk, 0);
	if (ret < 0) {
		LOG_ERR("Failed to set CLK low: %d", ret);
		return ret;
	}

	return 0;
}

/**
 Stop TM1637 communication.
 
 
  on success, negative errno on failure.
 */
static int tm1637_stop(const struct tm1637_config *cfg)
{
	int ret;

	ret = gpio_pin_set_dt(&cfg->clk, 0);
	if (ret < 0) {
		LOG_ERR("Failed to set CLK low: %d", ret);
		return ret;
	}
	tm1637_delay();
	ret = gpio_pin_set_dt(&cfg->dio, 0);
	if (ret < 0) {
		LOG_ERR("Failed to set DIO low: %d", ret);
		return ret;
	}
	tm1637_delay();
	ret = gpio_pin_set_dt(&cfg->clk, 1);
	if (ret < 0) {
		LOG_ERR("Failed to set CLK high: %d", ret);
		return ret;
	}
	tm1637_delay();
	ret = gpio_pin_set_dt(&cfg->dio, 1);
	if (ret < 0) {
		LOG_ERR("Failed to set DIO high: %d", ret);
		return ret;
	}

	return 0;
}

/**
Write a single byte to TM1637.
return 0 on success, negative errno on failure.
 */
static int tm1637_write_byte(const struct tm1637_config *cfg, uint8_t data)
{
	int ret;

	for (int i = 0; i < 8; i++) {
		ret = gpio_pin_set_dt(&cfg->clk, 0);
		if (ret < 0) {
			LOG_ERR("Failed to set CLK low: %d", ret);
			return ret;
		}
		tm1637_delay();
		ret = gpio_pin_set_dt(&cfg->dio, data & 0x01);
		if (ret < 0) {
			LOG_ERR("Failed to set DIO: %d", ret);
			return ret;
		}
		tm1637_delay();
		ret = gpio_pin_set_dt(&cfg->clk, 1);
		if (ret < 0) {
			LOG_ERR("Failed to set CLK high: %d", ret);
			return ret;
		}
		tm1637_delay();
		data >>= 1;
	}

	/* ACK sequence */
	ret = gpio_pin_set_dt(&cfg->clk, 0);
	if (ret < 0) {
		LOG_ERR("Failed to set CLK low: %d", ret);
		return ret;
	}
	tm1637_delay();
	ret = gpio_pin_set_dt(&cfg->dio, 1);
	if (ret < 0) {
		LOG_ERR("Failed to set DIO high: %d", ret);
		return ret;
	}
	tm1637_delay();
	ret = gpio_pin_set_dt(&cfg->clk, 1);
	if (ret < 0) {
		LOG_ERR("Failed to set CLK high: %d", ret);
		return ret;
	}
	tm1637_delay();
	ret = gpio_pin_set_dt(&cfg->clk, 0);
	if (ret < 0) {
		LOG_ERR("Failed to set CLK low: %d", ret);
		return ret;
	}

	return 0;
}

/**
 Digit to segment mapping for TM1637.
 */
static const uint8_t digits_to_segment[] = {
	0x3F, /* 0 */
	0x06, /* 1 */
	0x5B, /* 2 */
	0x4F, /* 3 */
	0x66, /* 4 */
	0x6D, /* 5 */
	0x7D, /* 6 */
	0x07, /* 7 */
	0x7F, /* 8 */
	0x6F  /* 9 */
};

/**
 Display raw segments at a specific position.
 */
int tm1637_display_raw_segments(const struct device *dev, uint8_t pos, uint8_t segments)
{
	const struct tm1637_config *cfg = dev->config;
	int ret;

	if (pos > 3) {
		LOG_ERR("Invalid position: %d", pos);
		return -EINVAL;
	}

	ret = tm1637_start(cfg);
	if (ret < 0) {
		return ret;
	}
	ret = tm1637_write_byte(cfg, TM1637_CMD_DATA_AUTO);
	if (ret < 0) {
		tm1637_stop(cfg);
		return ret;
	}
	ret = tm1637_stop(cfg);
	if (ret < 0) {
		return ret;
	}

	ret = tm1637_start(cfg);
	if (ret < 0) {
		return ret;
	}
	ret = tm1637_write_byte(cfg, TM1637_CMD_ADDR);
	if (ret < 0) {
		tm1637_stop(cfg);
		return ret;
	}
	for (uint8_t i = 0; i < 4; i++) {
		uint8_t data = (i == pos) ? segments : 0x00;
		ret = tm1637_write_byte(cfg, data);
		if (ret < 0) {
			tm1637_stop(cfg);
			return ret;
		}
	}
	ret = tm1637_stop(cfg);
	if (ret < 0) {
		return ret;
	}

	ret = tm1637_start(cfg);
	if (ret < 0) {
		return ret;
	}
	ret = tm1637_write_byte(cfg, TM1637_CMD_DISPLAY | 0x07);
	if (ret < 0) {
		tm1637_stop(cfg);
		return ret;
	}
	ret = tm1637_stop(cfg);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

/**
 Display a number on the TM1637.
 */
int tm1637_display_number(const struct device *dev, int num, bool colon)
{
	const struct tm1637_config *cfg = dev->config;
	uint8_t digits[4];
	int ret;

	if (num < 0 || num > 9999) {
		LOG_ERR("Number out of range: %d", num);
		return -EINVAL;
	}

	digits[0] = digits_to_segment[(num / 1000) % 10];
	digits[1] = digits_to_segment[(num / 100) % 10];
	digits[2] = digits_to_segment[(num / 10) % 10];
	digits[3] = digits_to_segment[num % 10];

	if (colon) {
		digits[1] |= 0x80;
	}

	ret = tm1637_start(cfg);
	if (ret < 0) {
		return ret;
	}
	ret = tm1637_write_byte(cfg, TM1637_CMD_DATA_AUTO);
	if (ret < 0) {
		tm1637_stop(cfg);
		return ret;
	}
	ret = tm1637_stop(cfg);
	if (ret < 0) {
		return ret;
	}

	ret = tm1637_start(cfg);
	if (ret < 0) {
		return ret;
	}
	ret = tm1637_write_byte(cfg, TM1637_CMD_ADDR);
	if (ret < 0) {
		tm1637_stop(cfg);
		return ret;
	}
	for (int i = 0; i < 4; i++) {
		ret = tm1637_write_byte(cfg, digits[i]);
		if (ret < 0) {
			tm1637_stop(cfg);
			return ret;
		}
	}
	ret = tm1637_stop(cfg);
	if (ret < 0) {
		return ret;
	}

	ret = tm1637_start(cfg);
	if (ret < 0) {
		return ret;
	}
	ret = tm1637_write_byte(cfg, TM1637_CMD_DISPLAY | 0x07);
	if (ret < 0) {
		tm1637_stop(cfg);
		return ret;
	}
	ret = tm1637_stop(cfg);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

/**
  Initialize the TM1637 driver.
 */
static int tm1637_init(const struct device *dev)
{
	const struct tm1637_config *cfg = dev->config;
	int ret;

	if (!device_is_ready(cfg->clk.port) || !device_is_ready(cfg->dio.port)) {
		LOG_ERR("GPIO ports not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&cfg->clk, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure CLK pin: %d", ret);
		return ret;
	}

	ret = gpio_pin_configure_dt(&cfg->dio, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure DIO pin: %d", ret);
		return ret;
	}

	LOG_INF("TM1637 initialized");
	return 0;
}

static const struct tm1637_config tm1637_config = {
	.clk = GPIO_DT_SPEC_INST_GET(0, clk_gpios),
	.dio = GPIO_DT_SPEC_INST_GET(0, dio_gpios),
};

DEVICE_DT_INST_DEFINE(0, tm1637_init, NULL, NULL, &tm1637_config,
		     POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);
