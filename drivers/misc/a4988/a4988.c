/*
 * Copyright (c) 2023 Kindhome Sp. z o.o <kindhome.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT allegromicro_a4988

#include <zephyr/device.h>
#include <zephyr/drivers/misc/a4988/a4988.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(a4988);

struct a4988_ms_pins {
	int ms1;
	int ms2;
	int ms3;
};

struct a4988_dev_config {
	struct {
		const struct gpio_dt_spec ms1;
		const struct gpio_dt_spec ms2;
		const struct gpio_dt_spec ms3;
		const struct gpio_dt_spec direction;
		const struct gpio_dt_spec step;
		const struct gpio_dt_spec sleep;
		const struct gpio_dt_spec enable;
		const struct gpio_dt_spec reset;
	} gpio;
};

static int a4988_microstep_to_pins(enum a4988_microstep microstep, struct a4988_ms_pins *ms_pins)
{
	switch (microstep) {
	case a4988_full_step:
		*ms_pins = (struct a4988_ms_pins){0, 0, 0};
		return 0;
	case a4988_half_step:
		*ms_pins = (struct a4988_ms_pins){1, 0, 0};
		return 0;
	case a4988_quarter_step:
		*ms_pins = (struct a4988_ms_pins){0, 1, 0};
		return 0;
	case a4988_eigth_step:
		*ms_pins = (struct a4988_ms_pins){1, 1, 0};
		return 0;
	case a4988_sixteenth_step:
		*ms_pins = (struct a4988_ms_pins){1, 1, 1};
		return 0;
	default:
		return -EINVAL;
	}
}

int a4988_step(const struct device *dev, enum a4988_microstep microstep, bool clockwise)
{
	int err;
	struct a4988_ms_pins ms_pins;
	const struct a4988_dev_config *config = dev->config;

	err = a4988_microstep_to_pins(microstep, &ms_pins);
	if (err < 0) {
		return 0;
	}

	err = gpio_pin_set_dt(&config->gpio.ms1, ms_pins.ms1);
	if (err < 0) {
		return 0;
	}
	err = gpio_pin_set_dt(&config->gpio.ms2, ms_pins.ms2);
	if (err < 0) {
		return 0;
	}
	err = gpio_pin_set_dt(&config->gpio.ms3, ms_pins.ms3);
	if (err < 0) {
		return 0;
	}

	err = gpio_pin_set_dt(&config->gpio.direction, clockwise);
	if (err < 0) {
		return 0;
	}

	k_sleep(K_NSEC(200));

	err = gpio_pin_set_dt(&config->gpio.step, 1);
	if (err < 0) {
		return 0;
	}

	k_sleep(K_USEC(1));

	err = gpio_pin_set_dt(&config->gpio.step, 0);
	if (err < 0) {
		return 0;
	}

	k_sleep(K_USEC(1));

	return 0;
}

int a4988_sleep(const struct device *dev, bool sleep)
{
	const struct a4988_dev_config *config = dev->config;
	int err;

	if (sleep) {
		return gpio_pin_set_dt(&config->gpio.sleep, 0);
	}

	err = gpio_pin_set_dt(&config->gpio.sleep, 1);
	if (err < 0) {
		return err;
	}

	k_sleep(K_MSEC(1));

	return 0;
}

int a4988_reset(const struct device *dev, bool reset)
{
	const struct a4988_dev_config *config = dev->config;
	int err;

	err = gpio_pin_set_dt(&config->gpio.reset, !reset);
	if (err < 0) {
		return err;
	}

	k_sleep(K_MSEC(1));

	return 0;
}

int a4988_enable(const struct device *dev, bool enable)
{
	const struct a4988_dev_config *config = dev->config;

	return gpio_pin_set_dt(&config->gpio.enable, !enable);
}

static int a4988_init(const struct device *dev)
{
	const struct a4988_dev_config *config = dev->config;
	int err;

	if (!gpio_is_ready_dt(&config->gpio.enable)) {
		LOG_ERR("Enable GPIO device not ready");
		return -ENODEV;
	}
	if (!gpio_is_ready_dt(&config->gpio.reset)) {
		LOG_ERR("Reset GPIO device not ready");
		return -ENODEV;
	}
	if (!gpio_is_ready_dt(&config->gpio.sleep)) {
		LOG_ERR("Sleep GPIO device not ready");
		return -ENODEV;
	}
	if (!gpio_is_ready_dt(&config->gpio.step)) {
		LOG_ERR("Step GPIO device not ready");
		return -ENODEV;
	}
	if (!gpio_is_ready_dt(&config->gpio.ms1)) {
		LOG_ERR("MS1 GPIO device not ready");
		return -ENODEV;
	}
	if (!gpio_is_ready_dt(&config->gpio.ms2)) {
		LOG_ERR("MS2 GPIO device not ready");
		return -ENODEV;
	}
	if (!gpio_is_ready_dt(&config->gpio.ms3)) {
		LOG_ERR("MS3 GPIO device not ready");
		return -ENODEV;
	}
	if (!gpio_is_ready_dt(&config->gpio.direction)) {
		LOG_ERR("Direction GPIO device not ready");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&config->gpio.enable, GPIO_OUTPUT_LOW);
	if (err < 0) {
		LOG_ERR("Enable GPIO configuration failed");
		return err;
	}
	err = gpio_pin_configure_dt(&config->gpio.sleep, GPIO_OUTPUT_HIGH);
	if (err < 0) {
		LOG_ERR("Sleep GPIO configuration failed");
		return err;
	}
	err = gpio_pin_configure_dt(&config->gpio.reset, GPIO_OUTPUT_HIGH);
	if (err < 0) {
		LOG_ERR("Reset GPIO configuration failed");
		return err;
	}
	err = gpio_pin_configure_dt(&config->gpio.ms1, GPIO_OUTPUT_LOW);
	if (err < 0) {
		LOG_ERR("MS1 GPIO configuration failed");
		return err;
	}
	err = gpio_pin_configure_dt(&config->gpio.ms2, GPIO_OUTPUT_LOW);
	if (err < 0) {
		LOG_ERR("MS2 GPIO configuration failed");
		return err;
	}
	err = gpio_pin_configure_dt(&config->gpio.ms3, GPIO_OUTPUT_LOW);
	if (err < 0) {
		LOG_ERR("MS3 GPIO configuration failed");
		return err;
	}
	err = gpio_pin_configure_dt(&config->gpio.direction, GPIO_OUTPUT_LOW);
	if (err < 0) {
		LOG_ERR("Direction GPIO configuration failed");
		return err;
	}
	err = gpio_pin_configure_dt(&config->gpio.step, GPIO_OUTPUT_LOW);
	if (err < 0) {
		LOG_ERR("Step GPIO configuration failed");
		return err;
	}

	return 0;
}

#define A4988_DEFINE(inst)                                                                         \
	static const struct a4988_dev_config a4988_config_##inst = {                               \
		.gpio = {                                                                          \
			.ms1 = GPIO_DT_SPEC_INST_GET_OR(inst, ms1_gpios, {0}),                     \
			.ms2 = GPIO_DT_SPEC_INST_GET_OR(inst, ms2_gpios, {0}),                     \
			.ms3 = GPIO_DT_SPEC_INST_GET_OR(inst, ms3_gpios, {0}),                     \
			.direction = GPIO_DT_SPEC_INST_GET_OR(inst, direction_gpios, {0}),         \
			.enable = GPIO_DT_SPEC_INST_GET_OR(inst, enable_gpios, {0}),               \
			.step = GPIO_DT_SPEC_INST_GET_OR(inst, step_gpios, {0}),                   \
			.sleep = GPIO_DT_SPEC_INST_GET_OR(inst, sleep_gpios, {0}),                 \
			.reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),                 \
		}};                                                                                \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, a4988_init, NULL, NULL, &a4988_config_##inst, POST_KERNEL,     \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

DT_INST_FOREACH_STATUS_OKAY(A4988_DEFINE)
