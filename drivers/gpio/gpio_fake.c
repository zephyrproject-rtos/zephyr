/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio/gpio_fake.h>
#include <zephyr/drivers/gpio/gpio_utils.h>

#ifdef CONFIG_ZTEST
#include <zephyr/ztest.h>
#endif

#define DT_DRV_COMPAT zephyr_gpio_fake

DEFINE_FAKE_VALUE_FUNC(int,
		       gpio_fake_pin_configure,
		       const struct device *,
		       gpio_pin_t,
		       gpio_flags_t);

#ifdef CONFIG_GPIO_GET_CONFIG
DEFINE_FAKE_VALUE_FUNC(int,
		       gpio_fake_pin_get_config,
		       const struct device *,
		       gpio_pin_t,
		       gpio_flags_t *);
#endif

DEFINE_FAKE_VALUE_FUNC(int,
		       gpio_fake_port_get_raw,
		       const struct device *,
		       gpio_port_value_t *);

DEFINE_FAKE_VALUE_FUNC(int,
		       gpio_fake_port_set_masked_raw,
		       const struct device *,
		       gpio_port_pins_t,
		       gpio_port_value_t);

DEFINE_FAKE_VALUE_FUNC(int,
		       gpio_fake_port_set_bits_raw,
		       const struct device *,
		       gpio_port_pins_t);

DEFINE_FAKE_VALUE_FUNC(int,
		       gpio_fake_port_clear_bits_raw,
		       const struct device *,
		       gpio_port_pins_t);

DEFINE_FAKE_VALUE_FUNC(int,
		       gpio_fake_port_toggle_bits,
		       const struct device *,
		       gpio_port_pins_t);

DEFINE_FAKE_VALUE_FUNC(int,
		       gpio_fake_pin_interrupt_configure,
		       const struct device *,
		       gpio_pin_t,
		       enum gpio_int_mode,
		       enum gpio_int_trig);

DEFINE_FAKE_VALUE_FUNC(int,
		       gpio_fake_manage_callback,
		       const struct device *,
		       struct gpio_callback *,
		       bool);

DEFINE_FAKE_VALUE_FUNC(uint32_t,
		       gpio_fake_get_pending_int,
		       const struct device *);

#ifdef CONFIG_GPIO_GET_DIRECTION
DEFINE_FAKE_VALUE_FUNC(int,
		       gpio_fake_port_get_direction,
		       const struct device *,
		       gpio_port_pins_t,
		       gpio_port_pins_t *,
		       gpio_port_pins_t *);
#endif

static DEVICE_API(gpio, gpio_fake_api) = {
	.pin_configure = gpio_fake_pin_configure,
#ifdef CONFIG_GPIO_GET_CONFIG
	.pin_get_config = gpio_fake_pin_get_config,
#endif
	.port_get_raw = gpio_fake_port_get_raw,
	.port_set_masked_raw = gpio_fake_port_set_masked_raw,
	.port_set_bits_raw = gpio_fake_port_set_bits_raw,
	.port_clear_bits_raw = gpio_fake_port_clear_bits_raw,
	.port_toggle_bits = gpio_fake_port_toggle_bits,
	.pin_interrupt_configure = gpio_fake_pin_interrupt_configure,
	.manage_callback = gpio_fake_manage_callback,
	.get_pending_int = gpio_fake_get_pending_int,
#ifdef CONFIG_GPIO_GET_DIRECTION
	.port_get_direction = gpio_fake_port_get_direction,
#endif
};

#ifdef CONFIG_ZTEST
static void gpio_fake_reset_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	ARG_UNUSED(test);
	ARG_UNUSED(fixture);

	RESET_FAKE(gpio_fake_pin_configure);
#ifdef CONFIG_GPIO_GET_CONFIG
	RESET_FAKE(gpio_fake_pin_get_config);
#endif
	RESET_FAKE(gpio_fake_port_get_raw);
	RESET_FAKE(gpio_fake_port_set_masked_raw);
	RESET_FAKE(gpio_fake_port_set_bits_raw);
	RESET_FAKE(gpio_fake_port_clear_bits_raw);
	RESET_FAKE(gpio_fake_port_toggle_bits);
	RESET_FAKE(gpio_fake_pin_interrupt_configure);
	RESET_FAKE(gpio_fake_manage_callback);
	RESET_FAKE(gpio_fake_get_pending_int);
#ifdef CONFIG_GPIO_GET_DIRECTION
	RESET_FAKE(gpio_fake_port_get_direction);
#endif
}

ZTEST_RULE(gpio_fake_reset_rule, gpio_fake_reset_rule_before, NULL);
#endif

static struct gpio_driver_data data;

static const struct gpio_driver_config config = GPIO_COMMON_CONFIG_FROM_DT_INST(0);

DEVICE_DT_INST_DEFINE(
	0,
	NULL,
	NULL,
	&data,
	&config,
	POST_KERNEL,
	CONFIG_GPIO_INIT_PRIORITY,
	&gpio_fake_api
);
