/*
 * Copyright (c) 2026 HubbleNetwork
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define TEST_NODE   DT_ALIAS(test_gpio)
#define TEST_NGPIOS DT_PROP(TEST_NODE, ngpios)

static const struct device *const test_dev = DEVICE_DT_GET(TEST_NODE);

#if defined(CONFIG_USERSPACE) || !defined(CONFIG_ASSERT)

/*
 * Out of range pins that alias a supported pin once the shift count is masked
 * to the width of an unsigned long: BIT(32) == 1 on a 32-bit target and
 * BIT(64) == 1 on a 64-bit one.
 */
static const gpio_pin_t oob_pins[] = {
	32, 33, 39, 64, 65, 71, 128, 129, 192, 255,
};

static void expect_rejected(gpio_pin_t pin)
{
	gpio_flags_t out_flags;
	int ret;

	ret = gpio_pin_configure(test_dev, pin, GPIO_OUTPUT_HIGH);
	zassert_true(ret < 0, "gpio_pin_configure(pin=%u) returned %d, expected an error", pin,
		     ret);

	ret = gpio_pin_interrupt_configure(test_dev, pin, GPIO_INT_EDGE_RISING);
	zassert_true(ret < 0, "gpio_pin_interrupt_configure(pin=%u) returned %d, expected an error",
		     pin, ret);

	ret = gpio_pin_get_config(test_dev, pin, &out_flags);
	zassert_true(ret < 0, "gpio_pin_get_config(pin=%u) returned %d, expected an error", pin,
		     ret);
}

ZTEST_USER(gpio_pin_range, test_out_of_range_pin_rejected)
{
	ARRAY_FOR_EACH(oob_pins, i) {
		expect_rejected(oob_pins[i]);
	}
}

#endif /* CONFIG_USERSPACE || !CONFIG_ASSERT */

/*
 * Pins that are in range for gpio_port_pins_t but not supported by this port
 * must keep being rejected. From user mode the syscall verifier turns these
 * into -EINVAL; from supervisor mode the GPIO API asserts on them, so without
 * userspace the test only runs with assertions compiled out.
 */
#if defined(CONFIG_USERSPACE) || !defined(CONFIG_ASSERT)
ZTEST_USER(gpio_pin_range, test_unsupported_pin_rejected)
{
	for (gpio_pin_t pin = TEST_NGPIOS; pin < GPIO_MAX_PINS_PER_PORT; pin++) {
		int ret = gpio_pin_configure(test_dev, pin, GPIO_OUTPUT_HIGH);

		zassert_equal(ret, -EINVAL, "gpio_pin_configure(pin=%u) returned %d, expected %d",
			      pin, ret, -EINVAL);
	}
}
#endif

/* The range check must not reject any pin the controller actually supports. */
ZTEST_USER(gpio_pin_range, test_supported_pins_still_work)
{
	for (gpio_pin_t pin = 0; pin < TEST_NGPIOS; pin++) {
		gpio_flags_t out_flags;
		int ret;

		ret = gpio_pin_configure(test_dev, pin, GPIO_OUTPUT_HIGH);
		zassert_ok(ret, "gpio_pin_configure(pin=%u) failed: %d", pin, ret);

		ret = gpio_pin_get_config(test_dev, pin, &out_flags);
		zassert_ok(ret, "gpio_pin_get_config(pin=%u) failed: %d", pin, ret);
		zassert_true((out_flags & GPIO_OUTPUT) != 0,
			     "pin %u was not configured as an output", pin);

		ret = gpio_pin_set(test_dev, pin, 0);
		zassert_ok(ret, "gpio_pin_set(pin=%u) failed: %d", pin, ret);

		ret = gpio_pin_toggle(test_dev, pin);
		zassert_ok(ret, "gpio_pin_toggle(pin=%u) failed: %d", pin, ret);
	}
}

/* Exercise the helper directly, including the boundary it guards. */
ZTEST(gpio_pin_range, test_gpio_port_pin_is_supported)
{
	zassert_true(gpio_port_pin_is_supported(0xffffffffU, 0));
	zassert_true(gpio_port_pin_is_supported(0xffffffffU, GPIO_MAX_PINS_PER_PORT - 1));
	zassert_false(gpio_port_pin_is_supported(0xffffffffU, GPIO_MAX_PINS_PER_PORT));
	zassert_false(gpio_port_pin_is_supported(0xffffffffU, UINT8_MAX));

	/* The aliasing cases that made the old check unsound. */
	zassert_true(gpio_port_pin_is_supported(0x1U, 0));
	zassert_false(gpio_port_pin_is_supported(0x1U, 32));
	zassert_false(gpio_port_pin_is_supported(0x1U, 64));
	zassert_false(gpio_port_pin_is_supported(0x1U, 128));
	zassert_false(gpio_port_pin_is_supported(0x1U, 192));

	/* A zero mask supports nothing. */
	zassert_false(gpio_port_pin_is_supported(0x0U, 0));
}

static void *gpio_pin_range_setup(void)
{
	zassert_true(device_is_ready(test_dev), "GPIO device is not ready");

	k_object_access_grant(test_dev, k_current_get());

	return NULL;
}

BUILD_ASSERT(TEST_NGPIOS < GPIO_MAX_PINS_PER_PORT,
	     "the test needs unsupported pins below GPIO_MAX_PINS_PER_PORT");

ZTEST_SUITE(gpio_pin_range, NULL, gpio_pin_range_setup, NULL, NULL, NULL);
