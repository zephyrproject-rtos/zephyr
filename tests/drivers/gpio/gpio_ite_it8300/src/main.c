/*
 * Copyright (c) 2026 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_ite_it8300_emul.h>
#include <zephyr/ztest.h>

#define IT8300_REG_PDRGE  0x05U
#define IT8300_REG_GPISRE 0x2fU
#define IT8300_REG_GPCRE  0x32U
#define IT8300_REG_PDMRE  0x2dU
#define IT8300_REG_OODERE 0x3dU

#define TEST_GPIO_GROUP 4U
#define TEST_INT_PIN    4U
#define TEST_PIN_MASK   (BIT(0) | GENMASK(6, 2))

static struct gpio_callback test_callback;
static struct k_sem callback_sem;
static gpio_port_pins_t callback_pins;

static const struct device *const gpio_dev = DEVICE_DT_GET(DT_ALIAS(it8300_gpe));
static const struct emul *const it8300_emul = EMUL_DT_GET(DT_ALIAS(it8300));

static void it8300_before(void *data)
{
	ARG_UNUSED(data);

	zassert_true(device_is_ready(gpio_dev));
	zassert_not_null(it8300_emul);
	for (gpio_pin_t pin = 0; pin < 7; pin++) {
		if ((TEST_PIN_MASK & BIT(pin)) != 0U) {
			zassert_ok(gpio_pin_interrupt_configure(gpio_dev, pin, GPIO_INT_DISABLE));
		}
	}
	it8300_emul_reset(it8300_emul);
	callback_pins = 0U;
}

ZTEST_SUITE(gpio_ite_it8300, NULL, NULL, it8300_before, NULL, NULL);

ZTEST(gpio_ite_it8300, test_pin_configure)
{
	gpio_flags_t flags;
	uint8_t value;

	zassert_ok(gpio_pin_configure(gpio_dev, 2, GPIO_OUTPUT_HIGH | GPIO_OPEN_DRAIN));
	zassert_ok(it8300_emul_get_reg(it8300_emul, IT8300_REG_PDRGE, &value));
	zassert_equal(value, BIT(2));
	zassert_ok(it8300_emul_get_reg(it8300_emul, IT8300_REG_OODERE, &value));
	zassert_equal(value, BIT(2));
	zassert_ok(it8300_emul_get_reg(it8300_emul, IT8300_REG_GPCRE + 2, &value));
	zassert_equal(value, BIT(6));

	zassert_ok(gpio_pin_get_config(gpio_dev, 2, &flags));
	zassert_true((flags & GPIO_OUTPUT) != 0U);
	zassert_true((flags & GPIO_OUTPUT_HIGH) != 0U);
	zassert_true((flags & GPIO_OPEN_DRAIN) != 0U);

	zassert_ok(gpio_pin_configure(gpio_dev, 3, GPIO_INPUT | GPIO_PULL_UP));
	zassert_ok(it8300_emul_get_reg(it8300_emul, IT8300_REG_GPCRE + 3, &value));
	zassert_equal(value, BIT(7) | BIT(2));

	zassert_ok(gpio_pin_configure(gpio_dev, 3, GPIO_INPUT));
	zassert_ok(it8300_emul_get_reg(it8300_emul, IT8300_REG_GPCRE + 3, &value));
	zassert_equal(value, BIT(7));
}

ZTEST(gpio_ite_it8300, test_port_access)
{
	gpio_port_value_t value;
	uint8_t reg_value;

	zassert_ok(it8300_emul_set_reg(it8300_emul, IT8300_REG_PDMRE, 0x55));
	zassert_ok(gpio_port_get_raw(gpio_dev, &value));
	zassert_equal(value, 0x55);

	zassert_ok(gpio_port_set_masked_raw(gpio_dev, 0x0d, 0x05));
	zassert_ok(it8300_emul_get_reg(it8300_emul, IT8300_REG_PDRGE, &reg_value));
	zassert_equal(reg_value, 0x05);
	zassert_ok(gpio_port_get_raw(gpio_dev, &value));
	zassert_equal(value, 0x05);

	zassert_ok(gpio_port_toggle_bits(gpio_dev, 0x14));
	zassert_ok(it8300_emul_get_reg(it8300_emul, IT8300_REG_PDRGE, &reg_value));
	zassert_equal(reg_value, 0x11);
}

ZTEST(gpio_ite_it8300, test_get_direction)
{
	gpio_port_pins_t inputs;
	gpio_port_pins_t outputs;

	zassert_ok(gpio_pin_configure(gpio_dev, 0, GPIO_INPUT));
	zassert_ok(gpio_pin_configure(gpio_dev, 2, GPIO_OUTPUT_LOW));
	zassert_ok(gpio_port_get_direction(gpio_dev, BIT(0) | BIT(2), &inputs, &outputs));
	zassert_equal(inputs, BIT(0));
	zassert_equal(outputs, BIT(2));
}

ZTEST(gpio_ite_it8300, test_unsupported_configuration)
{
	zassert_equal(gpio_pin_configure(gpio_dev, 0, GPIO_DISCONNECTED), -ENOTSUP);
	zassert_equal(gpio_pin_configure(gpio_dev, 0, GPIO_OUTPUT | GPIO_INPUT), -ENOTSUP);
	zassert_equal(gpio_pin_configure(gpio_dev, 0, GPIO_OUTPUT | GPIO_OPEN_SOURCE), -ENOTSUP);
	zassert_equal(gpio_pin_interrupt_configure(gpio_dev, 0, GPIO_INT_LEVEL_HIGH), -ENOTSUP);
}

static void test_gpio_callback(const struct device *dev, struct gpio_callback *cb,
			       gpio_port_pins_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);

	callback_pins |= pins;
	k_sem_give(&callback_sem);
}

ZTEST(gpio_ite_it8300, test_rising_edge_interrupt)
{
	uint8_t value;

	k_sem_init(&callback_sem, 0, 1);
	gpio_init_callback(&test_callback, test_gpio_callback, BIT(TEST_INT_PIN));
	zassert_ok(gpio_add_callback(gpio_dev, &test_callback));

	zassert_ok(it8300_emul_set_input(it8300_emul, TEST_GPIO_GROUP, 0U));
	zassert_ok(gpio_pin_interrupt_configure(gpio_dev, TEST_INT_PIN, GPIO_INT_EDGE_RISING));
	zassert_ok(it8300_emul_set_input(it8300_emul, TEST_GPIO_GROUP, BIT(TEST_INT_PIN)));
	zassert_ok(k_sem_take(&callback_sem, K_MSEC(100)));
	zassert_equal(callback_pins, BIT(TEST_INT_PIN));
	/* Let the alert work finish releasing ALERT# before injecting another edge. */
	k_msleep(1);

	/* The work handler must clear both the per-group and gathered status. */
	zassert_ok(it8300_emul_get_reg(it8300_emul, IT8300_REG_GPISRE, &value));
	zassert_equal(value, 0U);
	zassert_ok(it8300_emul_get_reg(it8300_emul, 0x00U, &value));
	zassert_equal(value, 0U);

	/* A falling edge is latched and drained, but filtered from this callback. */
	callback_pins = 0U;
	zassert_ok(it8300_emul_set_input(it8300_emul, TEST_GPIO_GROUP, 0U));
	zassert_equal(k_sem_take(&callback_sem, K_MSEC(20)), -EAGAIN);
	zassert_equal(callback_pins, 0U);

	zassert_ok(gpio_remove_callback(gpio_dev, &test_callback));
}

ZTEST(gpio_ite_it8300, test_falling_and_both_edge_interrupt)
{
	k_sem_init(&callback_sem, 0, 1);
	gpio_init_callback(&test_callback, test_gpio_callback, BIT(TEST_INT_PIN));
	zassert_ok(gpio_add_callback(gpio_dev, &test_callback));

	/* Start high so the first injected transition is falling. */
	zassert_ok(it8300_emul_set_input(it8300_emul, TEST_GPIO_GROUP, BIT(TEST_INT_PIN)));
	zassert_ok(gpio_pin_interrupt_configure(gpio_dev, TEST_INT_PIN, GPIO_INT_EDGE_FALLING));
	zassert_ok(it8300_emul_set_input(it8300_emul, TEST_GPIO_GROUP, 0U));
	zassert_ok(k_sem_take(&callback_sem, K_MSEC(100)));
	zassert_equal(callback_pins, BIT(TEST_INT_PIN));
	k_msleep(1);

	callback_pins = 0U;
	zassert_ok(gpio_pin_interrupt_configure(gpio_dev, TEST_INT_PIN, GPIO_INT_EDGE_BOTH));
	zassert_ok(it8300_emul_set_input(it8300_emul, TEST_GPIO_GROUP, BIT(TEST_INT_PIN)));
	zassert_ok(k_sem_take(&callback_sem, K_MSEC(100)));
	zassert_equal(callback_pins, BIT(TEST_INT_PIN));
	k_msleep(1);

	callback_pins = 0U;
	zassert_ok(it8300_emul_set_input(it8300_emul, TEST_GPIO_GROUP, 0U));
	zassert_ok(k_sem_take(&callback_sem, K_MSEC(100)));
	zassert_equal(callback_pins, BIT(TEST_INT_PIN));

	zassert_ok(gpio_remove_callback(gpio_dev, &test_callback));
}
