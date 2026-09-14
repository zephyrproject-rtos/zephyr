/*
 * Copyright (c) 2026 Siemens AG
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * GPIO relay backend test.
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/drivers/relay/relay.h>
#include <zephyr/ztest.h>

#define RELAY_ACTIVE_HIGH DT_NODELABEL(relay0)
#define RELAY_ACTIVE_LOW  DT_NODELABEL(relay1)

BUILD_ASSERT(DT_NODE_EXISTS(RELAY_ACTIVE_HIGH) && DT_NODE_EXISTS(RELAY_ACTIVE_LOW),
	     "board overlay must define relay0 (active-high) and relay1 (active-low)");

static const struct device *const relay_active_high = DEVICE_DT_GET(RELAY_ACTIVE_HIGH);
static const struct device *const relay_active_low = DEVICE_DT_GET(RELAY_ACTIVE_LOW);
static const struct gpio_dt_spec coil_active_high = GPIO_DT_SPEC_GET(RELAY_ACTIVE_HIGH, gpios);
static const struct gpio_dt_spec coil_active_low = GPIO_DT_SPEC_GET(RELAY_ACTIVE_LOW, gpios);

ZTEST(relay_gpio, test_devices_ready)
{
	zassert_true(device_is_ready(relay_active_high), "active-high relay not ready");
	zassert_true(device_is_ready(relay_active_low), "active-low relay not ready");
}

ZTEST(relay_gpio, test_initial_state_off)
{
	enum relay_state state;

	zassert_ok(relay_get_state(relay_active_high, &state), "get_state failed");
	zassert_equal(state, RELAY_STATE_OFF, "relay should power up off");
	zassert_ok(relay_get_state(relay_active_low, &state), "get_state failed");
	zassert_equal(state, RELAY_STATE_OFF, "relay should power up off");
	zassert_equal(gpio_emul_output_get_dt(&coil_active_high), 0,
		      "active-high coil should be de-energised at init");
	zassert_equal(gpio_emul_output_get_dt(&coil_active_low), 1,
		      "active-low coil line should idle high at init");
}

ZTEST(relay_gpio, test_set_state_active_high)
{
	enum relay_state state;

	zassert_ok(relay_set_state(relay_active_high, RELAY_STATE_ON), "set on failed");
	zassert_ok(relay_get_state(relay_active_high, &state), "get_state failed");
	zassert_equal(state, RELAY_STATE_ON, "state should read back on");
	zassert_equal(gpio_emul_output_get_dt(&coil_active_high), 1,
		      "active-high coil line should be high when on");

	zassert_ok(relay_set_state(relay_active_high, RELAY_STATE_OFF), "set off failed");
	zassert_ok(relay_get_state(relay_active_high, &state), "get_state failed");
	zassert_equal(state, RELAY_STATE_OFF, "state should read back off");
	zassert_equal(gpio_emul_output_get_dt(&coil_active_high), 0,
		      "active-high coil line should be low when off");
}

ZTEST(relay_gpio, test_set_state_active_low)
{
	enum relay_state state;

	zassert_ok(relay_set_state(relay_active_low, RELAY_STATE_ON), "set on failed");
	zassert_ok(relay_get_state(relay_active_low, &state), "get_state failed");
	zassert_equal(state, RELAY_STATE_ON, "state should read back on");
	zassert_equal(gpio_emul_output_get_dt(&coil_active_low), 0,
		      "active-low coil line should be low when on");

	zassert_ok(relay_set_state(relay_active_low, RELAY_STATE_OFF), "set off failed");
	zassert_ok(relay_get_state(relay_active_low, &state), "get_state failed");
	zassert_equal(state, RELAY_STATE_OFF, "state should read back off");
	zassert_equal(gpio_emul_output_get_dt(&coil_active_low), 1,
		      "active-low coil line should be high when off");
}

ZTEST_SUITE(relay_gpio, NULL, NULL, NULL, NULL, NULL);
