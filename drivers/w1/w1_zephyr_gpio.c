/*
 * Copyright (c) 2023 Hudson C. Dalpra
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_w1_gpio

/**
 * @brief 1-Wire Bus Master driver using Zephyr GPIO interface.
 *
 * This file contains the implementation of the 1-Wire Bus Master driver using
 * the Zephyr GPIO interface. The driver is based on GPIO bit-banging and
 * follows the timing specifications for 1-Wire communication.
 *
 * The driver supports both standard speed and overdrive speed modes.
 *
 * This driver is heavily based on the w1_zephyr_serial.c driver and the
 * technical article from Analog Devices.
 *
 * - w1_zephyr_serial.c: drivers/w1/w1_zephyr_serial.c
 * - Analog Devices 1-Wire Communication Through Software:
 * https://www.analog.com/en/resources/technical-articles/1wire-communication-through-software.html
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/w1.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(w1_gpio, CONFIG_W1_LOG_LEVEL);

/*
 * The time critical sections are used to ensure that the timing
 * between communication operations is correct.
 */
#if defined(CONFIG_W1_ZEPHYR_GPIO_TIME_CRITICAL)
#define W1_GPIO_ENTER_CRITICAL()   irq_lock()
#define W1_GPIO_EXIT_CRITICAL(key) irq_unlock(key)
#define W1_GPIO_WAIT_US(us)        k_busy_wait(us)
#else
#define W1_GPIO_ENTER_CRITICAL()   0u
#define W1_GPIO_EXIT_CRITICAL(key) (void)key
#define W1_GPIO_WAIT_US(us)        k_usleep(us)
#endif

/*
 * Standard timing between communication operations:
 */
#define W1_GPIO_TIMING_STD_A 6u
#define W1_GPIO_TIMING_STD_B 64u
#define W1_GPIO_TIMING_STD_C 60u
#define W1_GPIO_TIMING_STD_D 10u
#define W1_GPIO_TIMING_STD_E 9u
#define W1_GPIO_TIMING_STD_F 55u
#define W1_GPIO_TIMING_STD_G 0u
#define W1_GPIO_TIMING_STD_H 480u
#define W1_GPIO_TIMING_STD_I 70u
#define W1_GPIO_TIMING_STD_J 410u

/*
 * Overdrive timing between communication operations:
 *
 * Not completely correct since the overdrive communication requires
 * delays of 2.5us, 7.5us and 8.5us.
 * The delays are approximated by flooring the values.
 */
#define W1_GPIO_TIMING_OD_A 1u
#define W1_GPIO_TIMING_OD_B 7u
#define W1_GPIO_TIMING_OD_C 7u
#define W1_GPIO_TIMING_OD_D 2u
#define W1_GPIO_TIMING_OD_E 1u
#define W1_GPIO_TIMING_OD_F 7u
#define W1_GPIO_TIMING_OD_G 2u
#define W1_GPIO_TIMING_OD_H 70u
#define W1_GPIO_TIMING_OD_I 8u
#define W1_GPIO_TIMING_OD_J 40u

struct w1_gpio_timing {
	uint16_t a;
	uint16_t b;
	uint16_t c;
	uint16_t d;
	uint16_t e;
	uint16_t f;
	uint16_t g;
	uint16_t h;
	uint16_t i;
	uint16_t j;
};

struct w1_gpio_config {
	/** w1 master config, common to all drivers */
	struct w1_master_config master_config;
	/** GPIO device used for 1-Wire communication */
	const struct gpio_dt_spec spec;
};

struct w1_gpio_data {
	/** w1 master data, common to all drivers */
	struct w1_master_data master_data;
	/** timing parameters for 1-Wire communication */
	const struct w1_gpio_timing *timing;
	/** overdrive speed mode active */
	bool overdrive_active;
};

static const struct w1_gpio_timing std = {
	.a = W1_GPIO_TIMING_STD_A,
	.b = W1_GPIO_TIMING_STD_B,
	.c = W1_GPIO_TIMING_STD_C,
	.d = W1_GPIO_TIMING_STD_D,
	.e = W1_GPIO_TIMING_STD_E,
	.f = W1_GPIO_TIMING_STD_F,
	.g = W1_GPIO_TIMING_STD_G,
	.h = W1_GPIO_TIMING_STD_H,
	.i = W1_GPIO_TIMING_STD_I,
	.j = W1_GPIO_TIMING_STD_J,
};

static const struct w1_gpio_timing od = {
	.a = W1_GPIO_TIMING_OD_A,
	.b = W1_GPIO_TIMING_OD_B,
	.c = W1_GPIO_TIMING_OD_C,
	.d = W1_GPIO_TIMING_OD_D,
	.e = W1_GPIO_TIMING_OD_E,
	.f = W1_GPIO_TIMING_OD_F,
	.g = W1_GPIO_TIMING_OD_G,
	.h = W1_GPIO_TIMING_OD_H,
	.i = W1_GPIO_TIMING_OD_I,
	.j = W1_GPIO_TIMING_OD_J,
};

static int w1_gpio_reset_bus(const struct device *dev)
{
	const struct w1_gpio_config *cfg = dev->config;
	const struct w1_gpio_data *data = dev->data;

	const struct gpio_dt_spec *spec = &cfg->spec;
	const struct w1_gpio_timing *timing = data->timing;

	int ret = 0;
	unsigned int key = W1_GPIO_ENTER_CRITICAL();

	W1_GPIO_WAIT_US(timing->g);
	ret = gpio_pin_set_dt(spec, 0);
	if (ret < 0) {
		goto out;
	}

	W1_GPIO_WAIT_US(timing->h);
	ret = gpio_pin_set_dt(spec, 1);
	if (ret < 0) {
		goto out;
	}

	W1_GPIO_WAIT_US(timing->i);
	ret = gpio_pin_get_dt(spec);
	if (ret < 0) {
		goto out;
	}
	ret ^= 0x01;

	W1_GPIO_WAIT_US(timing->j);
out:
	W1_GPIO_EXIT_CRITICAL(key);
	return ret;
}

static int w1_gpio_read_bit(const struct device *dev)
{
	const struct w1_gpio_config *cfg = dev->config;
	const struct w1_gpio_data *data = dev->data;

	const struct gpio_dt_spec *spec = &cfg->spec;
	const struct w1_gpio_timing *timing = data->timing;

	int ret = 0;
	unsigned int key = W1_GPIO_ENTER_CRITICAL();

	ret = gpio_pin_set_dt(spec, 0);
	if (ret < 0) {
		goto out;
	}

	W1_GPIO_WAIT_US(timing->a);
	ret = gpio_pin_set_dt(spec, 1);
	if (ret < 0) {
		goto out;
	}

	W1_GPIO_WAIT_US(timing->e);
	ret = gpio_pin_get_dt(spec);
	if (ret < 0) {
		goto out;
	}
	ret &= 0x01;

	W1_GPIO_WAIT_US(timing->f);
out:
	W1_GPIO_EXIT_CRITICAL(key);
	return ret;
}

static int w1_gpio_write_bit(const struct device *dev, const bool bit)
{
	const struct w1_gpio_config *cfg = dev->config;
	const struct w1_gpio_data *data = dev->data;

	const struct gpio_dt_spec *spec = &cfg->spec;
	const struct w1_gpio_timing *timing = data->timing;

	int ret = 0;
	unsigned int key = W1_GPIO_ENTER_CRITICAL();

	ret = gpio_pin_set_dt(spec, 0);
	if (ret < 0) {
		goto out;
	}

	W1_GPIO_WAIT_US(bit ? timing->a : timing->c);
	ret = gpio_pin_set_dt(spec, 1);
	if (ret < 0) {
		goto out;
	}

	W1_GPIO_WAIT_US(bit ? timing->b : timing->d);
out:
	W1_GPIO_EXIT_CRITICAL(key);
	return ret;
}

static int w1_gpio_read_byte(const struct device *dev)
{
	int ret = 0;
	int byte = 0x00;

	for (int i = 0; i < 8; i++) {
		ret = w1_gpio_read_bit(dev);
		if (ret < 0) {
			return ret;
		}

		byte >>= 1;
		if (ret) {
			byte |= 0x80;
		}
	}

	return byte;
}

static int w1_gpio_write_byte(const struct device *dev, const uint8_t byte)
{
	int ret = 0;
	uint8_t write = byte;

	for (int i = 0; i < 8; i++) {
		ret = w1_gpio_write_bit(dev, write & 0x01);
		if (ret < 0) {
			return ret;
		}
		write >>= 1;
	}

	return ret;
}

static int w1_gpio_configure(const struct device *dev, enum w1_settings_type type, uint32_t value)
{
	struct w1_gpio_data *data = dev->data;

	switch (type) {
	case W1_SETTING_SPEED:
		data->overdrive_active = (value != 0);
		data->timing = data->overdrive_active ? &od : &std;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int w1_gpio_init(const struct device *dev)
{
	const struct w1_gpio_config *cfg = dev->config;
	const struct gpio_dt_spec *spec = &cfg->spec;
	struct w1_gpio_data *data = dev->data;

	if (gpio_is_ready_dt(spec)) {
		int ret = gpio_pin_configure_dt(spec, GPIO_OUTPUT_INACTIVE | GPIO_OPEN_DRAIN |
							      GPIO_INPUT);
		if (ret < 0) {
			LOG_ERR("Failed to configure GPIO port %s pin %d", spec->port->name,
				spec->pin);
			return ret;
		}
	} else {
		LOG_ERR("GPIO port %s is not ready", spec->port->name);
		return -ENODEV;
	}

	data->timing = &std;
	data->overdrive_active = false;

	LOG_DBG("w1-gpio initialized, with %d slave devices", cfg->master_config.slave_count);
	return 0;
}

static const struct w1_driver_api w1_gpio_driver_api = {
	.reset_bus = w1_gpio_reset_bus,
	.read_bit = w1_gpio_read_bit,
	.write_bit = w1_gpio_write_bit,
	.read_byte = w1_gpio_read_byte,
	.write_byte = w1_gpio_write_byte,
	.configure = w1_gpio_configure,
};

#define W1_ZEPHYR_GPIO_INIT(inst)                                                                  \
	static const struct w1_gpio_config w1_gpio_cfg_##inst = {                                  \
		.master_config.slave_count = W1_INST_SLAVE_COUNT(inst),                            \
		.spec = GPIO_DT_SPEC_INST_GET(inst, gpios)};                                       \
	static struct w1_gpio_data w1_gpio_data_##inst = {};                                       \
	DEVICE_DT_INST_DEFINE(inst, &w1_gpio_init, NULL, &w1_gpio_data_##inst,                     \
			      &w1_gpio_cfg_##inst, POST_KERNEL, CONFIG_W1_INIT_PRIORITY,           \
			      &w1_gpio_driver_api);

DT_INST_FOREACH_STATUS_OKAY(W1_ZEPHYR_GPIO_INIT)
