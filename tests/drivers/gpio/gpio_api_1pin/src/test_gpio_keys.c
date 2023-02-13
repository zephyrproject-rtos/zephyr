/*
 * Copyright (c) 2022 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/sys/util.h"
#include <zephyr/device.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>

#include <zephyr/drivers/gpio_keys.h>

LOG_MODULE_REGISTER(gpio_keys_test, LOG_LEVEL_DBG);

#if DT_NODE_EXISTS(DT_NODELABEL(voldown_button))

const struct device *test_gpio_keys_dev = DEVICE_DT_GET(DT_PARENT(DT_NODELABEL(voldown_button)));
#define BUTTON_0_IDX DT_NODE_CHILD_IDX(DT_NODELABEL(voldown_button))
#define BUTTON_1_IDX DT_NODE_CHILD_IDX(DT_NODELABEL(volup_button))

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

/**
 * @brief TestPurpose: Verify gpio_keys_config pressed raw.
 *
 */
ZTEST(gpio_keys, test_gpio_keys_pressed)
{
	const struct gpio_keys_config *config = test_gpio_keys_dev->config;
	const struct gpio_keys_pin_config *pin_cfg = &config->pin_cfg[BUTTON_0_IDX];
	const struct gpio_dt_spec *spec = &pin_cfg->spec;

	zassert_ok(gpio_pin_configure(spec->port, spec->pin, GPIO_INPUT));

	zassert_ok(gpio_emul_input_set(spec->port, spec->pin, 1));
	zassert_equal(1, gpio_keys_get_pin(test_gpio_keys_dev, BUTTON_0_IDX));

	zassert_ok(gpio_emul_input_set(spec->port, spec->pin, 0));
	zassert_equal(0, gpio_keys_get_pin(test_gpio_keys_dev, BUTTON_0_IDX));
}

/**
 * @brief TestPurpose: Verify button interrupt.
 *
 */
uint32_t gpio_keys_interrupt_called;
void test_gpio_keys_cb_handler(const struct device *dev, struct gpio_keys_callback *cbdata,
			       uint32_t pins)
{
	LOG_DBG("GPIO_KEY %s pressed, pins=%d, zephyr_code=%u, pin_state=%d", dev->name, pins,
		cbdata->zephyr_code, cbdata->pin_state);
	gpio_keys_interrupt_called = cbdata->zephyr_code;
}

ZTEST(gpio_keys, test_gpio_keys_interrupt)
{
	int button_idx[] = {BUTTON_0_IDX, BUTTON_1_IDX};
	int num_gpio_keys = ARRAY_SIZE(button_idx);
	const struct gpio_keys_config *config = test_gpio_keys_dev->config;
	const struct gpio_keys_pin_config *pin_cfg;
	const struct gpio_dt_spec *spec;

	for (int i = 0; i < num_gpio_keys; i++) {
		pin_cfg = &config->pin_cfg[button_idx[i]];
		spec = &pin_cfg->spec;

		LOG_DBG("GPIO_KEY config=[0x%p, %d]", config->debounce_interval_ms,
			pin_cfg->zephyr_code);
		LOG_DBG("GPIO_KEY spec=[0x%p, %d]", spec->port, spec->pin);

		zassert_ok(gpio_pin_configure(spec->port, spec->pin, GPIO_INPUT));
		zassert_ok(gpio_keys_disable_interrupt(test_gpio_keys_dev), NULL);
		k_sleep(K_MSEC(500));

		/* Check interrupts are disabled */
		gpio_keys_interrupt_called = 0;
		zassert_ok(gpio_emul_input_set(spec->port, spec->pin, 0));
		k_sleep(K_MSEC(1000));
		zassert_ok(gpio_emul_input_set(spec->port, spec->pin, 1));
		k_sleep(K_MSEC(1000));
		zassert_equal(gpio_keys_interrupt_called, 0);

		zassert_ok(
			gpio_keys_enable_interrupt(test_gpio_keys_dev, test_gpio_keys_cb_handler),
			NULL);
		zassert_ok(gpio_emul_input_set(spec->port, spec->pin, 0));
		k_sleep(K_MSEC(1000));

		gpio_keys_interrupt_called = 0;
		zassert_ok(gpio_emul_input_set(spec->port, spec->pin, 1));

		/* Check interrupt doesn't prematurely fires */
		k_sleep(K_MSEC(config->debounce_interval_ms / 2));
		zassert_equal(gpio_keys_interrupt_called, 0);

		/* Check interrupt fires after debounce interval */
		k_sleep(K_MSEC(config->debounce_interval_ms));
		zassert_equal(gpio_keys_interrupt_called, pin_cfg->zephyr_code);
	}
}

#endif /* DT_NODE_EXISTS(DT_NODELABEL(voldown_button)) */
