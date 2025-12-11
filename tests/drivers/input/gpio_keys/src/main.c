/*
 * Copyright (c) 2022 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/input/input.h>

static const struct device *const test_gpio_keys_dev = DEVICE_DT_GET(DT_NODELABEL(buttons));
#define BUTTON_0_IDX DT_NODE_CHILD_IDX(DT_NODELABEL(voldown_button))

struct gpio_keys_pin_config {
	/** GPIO specification from devicetree */
	struct gpio_dt_spec spec;
	/** Zephyr code from devicetree */
	uint32_t zephyr_code;
};
struct gpio_keys_config {
	/** Debounce interval in milliseconds from devicetree */
	uint32_t debounce_interval_ms;
	const int num_keys;
	const struct gpio_keys_pin_config *pin_cfg;
};

/**
 * @brief Test Suite: Verifies gpio_keys_config functionality.
 */
ZTEST_SUITE(gpio_keys, NULL, NULL, NULL, NULL, NULL);

static int event_count;
static uint16_t last_code;
static bool last_val;
static void test_gpio_keys_cb_handler(struct input_event *evt, void *user_data)
{
	TC_PRINT("GPIO_KEY %s pressed, zephyr_code=%u, value=%d\n",
		 evt->dev->name, evt->code, evt->value);
	event_count++;
	last_code = evt->code;
	last_val = evt->value;
}
INPUT_CALLBACK_DEFINE(test_gpio_keys_dev, test_gpio_keys_cb_handler, NULL);

/**
 * @brief TestPurpose: Verify gpio_keys_config pressed raw.
 *
 */
ZTEST(gpio_keys, test_gpio_keys_pressed)
{
	const struct gpio_keys_config *config = test_gpio_keys_dev->config;
	const struct gpio_keys_pin_config *pin_cfg = &config->pin_cfg[BUTTON_0_IDX];
	const struct gpio_dt_spec *spec = &pin_cfg->spec;

	last_code = 0;
	last_val = false;

	zassert_ok(gpio_emul_input_set(spec->port, spec->pin, 0));

	/* Check interrupt doesn't prematurely fires */
	k_sleep(K_MSEC(config->debounce_interval_ms / 2));
	zassert_equal(event_count, 0);

	/* Check interrupt fires after debounce interval */
	k_sleep(K_MSEC(config->debounce_interval_ms));
	zassert_equal(event_count, 1);
	zassert_equal(last_code, pin_cfg->zephyr_code);
	zassert_equal(last_val, true);

	zassert_ok(gpio_emul_input_set(spec->port, spec->pin, 1));

	/* Check interrupt doesn't prematurely fires */
	k_sleep(K_MSEC(config->debounce_interval_ms / 2));
	zassert_equal(event_count, 1);

	/* Check interrupt fires after debounce interval */
	k_sleep(K_MSEC(config->debounce_interval_ms));
	zassert_equal(event_count, 2);
	zassert_equal(last_code, pin_cfg->zephyr_code);
	zassert_equal(last_val, false);
}
