/*
 * Copyright (c) 2026 Zhaoming Li
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/ztest.h>

#define TEST_NODE DT_NODELABEL(test_latch)
#define TEST_DEV DEVICE_DT_GET(TEST_NODE)
#define TEST_REG_ADDR DT_REG_ADDR(TEST_NODE)
#define TEST_PIN_MASK GPIO_PORT_PIN_MASK_FROM_DT_NODE(TEST_NODE)
#define TEST_INITIAL_OUTPUT DT_PROP(TEST_NODE, initial_output)

#define TEST_BACKING_RAW_VALUE 0xa5
#define TEST_MASKED_WRITE_MASK 0x0f
#define TEST_MASKED_WRITE_VALUE 0x05
#define TEST_AFTER_MASKED_WRITE 0x55
#define TEST_SET_BITS (BIT(0) | BIT(7))
#define TEST_AFTER_SET_BITS 0xdb
#define TEST_CLEAR_BITS (BIT(1) | BIT(3))
#define TEST_AFTER_CLEAR_BITS 0xd1
#define TEST_TOGGLE_BITS (BIT(0) | BIT(4) | BIT(6))
#define TEST_AFTER_TOGGLE_BITS 0x80
#define TEST_INIT_HIGH_PIN 0
#define TEST_AFTER_INIT_HIGH 0x5b
#define TEST_INIT_LOW_PIN 1
#define TEST_AFTER_INIT_LOW 0x59

static const struct device *test_dev;

static void reset_latch(gpio_port_value_t value)
{
	zassert_ok(gpio_port_set_masked_raw(test_dev, TEST_PIN_MASK, value));
}

static void assert_shadow_and_register(gpio_port_value_t expected)
{
	gpio_port_value_t value;

	zassert_ok(gpio_port_get_raw(test_dev, &value));
	zassert_equal(value, expected);
	zassert_equal(sys_read32(TEST_REG_ADDR), expected);
}

static void *gpio_mmio_latch_setup(void)
{
	gpio_port_value_t value;

	test_dev = DEVICE_DT_GET(TEST_NODE);
	zassert_true(device_is_ready(test_dev), "GPIO MMIO latch device not ready");

	zassert_ok(gpio_port_get_raw(test_dev, &value));
	zassert_equal(value, TEST_INITIAL_OUTPUT);
	zassert_equal(sys_read32(TEST_REG_ADDR), TEST_INITIAL_OUTPUT);

	return NULL;
}

ZTEST(gpio_mmio_latch, test_port_get_raw_returns_shadow)
{
	gpio_port_value_t value;

	reset_latch(TEST_INITIAL_OUTPUT);
	sys_write32(TEST_BACKING_RAW_VALUE, TEST_REG_ADDR);

	/* Readback must come from the software shadow, not the backing register. */
	zassert_ok(gpio_port_get_raw(test_dev, &value));
	zassert_equal(value, TEST_INITIAL_OUTPUT);
	zassert_equal(sys_read32(TEST_REG_ADDR), TEST_BACKING_RAW_VALUE);
}

ZTEST(gpio_mmio_latch, test_port_set_masked_raw_updates_only_masked_bits)
{
	reset_latch(TEST_INITIAL_OUTPUT);

	zassert_ok(gpio_port_set_masked_raw(test_dev, TEST_MASKED_WRITE_MASK,
					    TEST_MASKED_WRITE_VALUE));
	assert_shadow_and_register(TEST_AFTER_MASKED_WRITE);
}

ZTEST(gpio_mmio_latch, test_port_set_clear_toggle_bits)
{
	reset_latch(TEST_INITIAL_OUTPUT);

	zassert_ok(gpio_port_set_bits_raw(test_dev, TEST_SET_BITS));
	assert_shadow_and_register(TEST_AFTER_SET_BITS);

	zassert_ok(gpio_port_clear_bits_raw(test_dev, TEST_CLEAR_BITS));
	assert_shadow_and_register(TEST_AFTER_CLEAR_BITS);

	zassert_ok(gpio_port_toggle_bits(test_dev, TEST_TOGGLE_BITS));
	assert_shadow_and_register(TEST_AFTER_TOGGLE_BITS);
}

ZTEST(gpio_mmio_latch, test_pin_configure_rejects_unsupported_flags)
{
	zassert_equal(gpio_pin_configure(test_dev, 0, GPIO_OUTPUT | GPIO_PULL_UP), -ENOTSUP);
	zassert_equal(gpio_pin_configure(test_dev, 0, GPIO_INPUT), -ENOTSUP);
}

ZTEST(gpio_mmio_latch, test_pin_configure_init_level_updates_shadow)
{
	reset_latch(TEST_INITIAL_OUTPUT);

	zassert_ok(gpio_pin_configure(test_dev, TEST_INIT_HIGH_PIN,
				      GPIO_OUTPUT | GPIO_OUTPUT_INIT_HIGH));
	assert_shadow_and_register(TEST_AFTER_INIT_HIGH);

	zassert_ok(gpio_pin_configure(test_dev, TEST_INIT_LOW_PIN,
				      GPIO_OUTPUT | GPIO_OUTPUT_INIT_LOW));
	assert_shadow_and_register(TEST_AFTER_INIT_LOW);
}

ZTEST_SUITE(gpio_mmio_latch, NULL, gpio_mmio_latch_setup, NULL, NULL, NULL);
