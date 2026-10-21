/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file test_gpio_api.c
 * @brief GPIO1-12 through the GPIO API of the cirrus,cs47l63-gpio child node
 *
 * Register addresses are written out from DS1249F2 table 4-56, as in test_gpio.c.
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/ztest.h>

#include "cs47l63.h"

#include "cs47l63_emul.h"

#define CODEC DEVICE_DT_GET(DT_NODELABEL(cs47l63_gpio))
#define EMUL  EMUL_DT_GET(DT_NODELABEL(cs47l63_gpio))
#define GPIO  DEVICE_DT_GET(DT_NODELABEL(cs47l63_gpio_ctl))

#define EMUL_ASP2 EMUL_DT_GET(DT_NODELABEL(cs47l63_gpio_asp2))
#define GPIO_ASP2 DEVICE_DT_GET(DT_NODELABEL(cs47l63_gpio_asp2_ctl))

#define EMUL_DUAL EMUL_DT_GET(DT_NODELABEL(cs47l63_gpio_dual))
#define GPIO_DUAL DEVICE_DT_GET(DT_NODELABEL(cs47l63_gpio_dual_ctl))

#define PINS 12U

#define GPIO_STATUS 0x00000C00U

#define WORD_OUT_HIGH 0x01008001U
#define WORD_OUT_LOW  0x01000001U

#define PIN_A 9U

static const struct gpio_dt_spec k_active_low =
	GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), cs47l63_gpios);

static void api_before_each(const struct device *gpio, const struct emul *target)
{
	cs47l63_emul_reset(target);
	for (gpio_pin_t pin = 0U; pin < PINS; pin++) {
		const int ret = gpio_pin_configure(gpio, pin, GPIO_DISCONNECTED);

		zassert_true((ret == 0) || (ret == -EBUSY), "pin %u: %d", pin, ret);
	}
	cs47l63_emul_reset(target);
}

static void main_before_each(void *fixture)
{
	ARG_UNUSED(fixture);

	api_before_each(GPIO, EMUL);
}

static void asp2_before_each(void *fixture)
{
	ARG_UNUSED(fixture);

	api_before_each(GPIO_ASP2, EMUL_ASP2);
}

static void dual_before_each(void *fixture)
{
	ARG_UNUSED(fixture);

	api_before_each(GPIO_DUAL, EMUL_DUAL);
}

static uint32_t last_write(const struct emul *target, uint32_t addr)
{
	const uint32_t count = cs47l63_emul_write_count(target, addr);
	uint32_t val = 0U;

	zassert_true(count > 0U, "no write to 0x%05x", addr);
	zassert_true(cs47l63_emul_nth_write(target, addr, count - 1U, &val));

	return val;
}

ZTEST_SUITE(cs47l63_gpio_api, NULL, NULL, main_before_each, NULL, NULL);
ZTEST_SUITE(cs47l63_gpio_api_asp2, NULL, NULL, asp2_before_each, NULL, NULL);
ZTEST_SUITE(cs47l63_gpio_api_dual, NULL, NULL, dual_before_each, NULL, NULL);

static ZTEST(cs47l63_gpio_api, test_devices_ready_after_parent)
{
	/* The child refuses to initialize unless the codec did first. */
	zassert_true(device_is_ready(CODEC));
	zassert_true(device_is_ready(GPIO));
	zassert_true(device_is_ready(GPIO_ASP2));
	zassert_true(device_is_ready(GPIO_DUAL));
}

static ZTEST(cs47l63_gpio_api, test_dt_active_low_is_logical)
{
	gpio_port_value_t val = 0U;

	zassert_equal(k_active_low.port, GPIO);
	zassert_equal(k_active_low.pin, PIN_A);

	/* Inactive is a high pin. */
	zassert_ok(gpio_pin_configure_dt(&k_active_low, GPIO_OUTPUT_INACTIVE));
	zassert_equal(last_write(EMUL, 0x00000C2CU), WORD_OUT_HIGH);

	zassert_ok(gpio_pin_set_dt(&k_active_low, 1));
	zassert_equal(last_write(EMUL, 0x00000C2CU), WORD_OUT_LOW);

	zassert_ok(gpio_pin_configure_dt(&k_active_low, GPIO_INPUT));
	cs47l63_emul_set_reg(EMUL, GPIO_STATUS, 0U);
	zassert_equal(gpio_pin_get_dt(&k_active_low), 1, "a low pin is active");
	cs47l63_emul_set_reg(EMUL, GPIO_STATUS, BIT(PIN_A));
	zassert_equal(gpio_pin_get_dt(&k_active_low), 0);

	zassert_ok(gpio_port_get_raw(GPIO, &val));
	zassert_equal(val, BIT(PIN_A));
}

static ZTEST(cs47l63_gpio_api, test_pin_numbers_are_zero_based)
{
	zassert_ok(gpio_pin_configure(GPIO, 11U, GPIO_INPUT));
	zassert_equal(cs47l63_emul_write_count(EMUL, 0x00000C34U), 1U, "pin 11 is GPIO12");
}

static ZTEST(cs47l63_gpio_api_asp2, test_pin_numbers_are_zero_based)
{
	zassert_ok(gpio_pin_configure(GPIO_ASP2, 0U, GPIO_INPUT));
	zassert_equal(cs47l63_emul_write_count(EMUL_ASP2, 0x00000C08U), 1U, "pin 0 is GPIO1");

	zassert_ok(gpio_pin_configure(GPIO_ASP2, 11U, GPIO_INPUT));
	zassert_equal(cs47l63_emul_write_count(EMUL_ASP2, 0x00000C34U), 1U);
}

static ZTEST(cs47l63_gpio_api_dual, test_taken_pad_is_refused_through_the_api)
{
	zassert_equal(gpio_pin_configure(GPIO_DUAL, 0U, GPIO_INPUT), -EBUSY);
	zassert_ok(gpio_pin_configure(GPIO_DUAL, 8U, GPIO_INPUT));
}

static void callback_handler(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);
}

static ZTEST(cs47l63_gpio_api, test_no_interrupt_api)
{
	static struct gpio_callback cb;

	gpio_init_callback(&cb, callback_handler, BIT(PIN_A));

	zassert_equal(gpio_pin_interrupt_configure(GPIO, PIN_A, GPIO_INT_EDGE_RISING), -ENOSYS);
	zassert_equal(gpio_add_callback(GPIO, &cb), -ENOSYS);
}
