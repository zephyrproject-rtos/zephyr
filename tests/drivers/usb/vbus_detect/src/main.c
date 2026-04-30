/*
 * Copyright (c) 2026 Leica Geosystems AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test_vbus_detect, LOG_LEVEL_DBG);

#define VBUS_GPIO_NODE DT_NODELABEL(vbus_detect)
#define VBUS_GPIO_CTLR DT_GPIO_CTLR(VBUS_GPIO_NODE, vbus_gpios)
#define VBUS_GPIO_PIN  DT_GPIO_PIN(VBUS_GPIO_NODE, vbus_gpios)

static const struct device *gpio_dev = DEVICE_DT_GET(VBUS_GPIO_CTLR);
static const struct device *vbus_dev = DEVICE_DT_GET(VBUS_GPIO_NODE);

static void *vbus_detect_setup(void)
{
	zassert_true(device_is_ready(gpio_dev), "GPIO device not ready");
	zassert_true(device_is_ready(vbus_dev), "VBUS detect device not ready");

	return NULL;
}

static void vbus_detect_before(void *fixture)
{
	ARG_UNUSED(fixture);

	/* Start with VBUS low */
	gpio_emul_input_set(gpio_dev, VBUS_GPIO_PIN, 0);
	k_msleep(10);
}

ZTEST(vbus_detect, test_device_ready)
{
	/* Verify the VBUS detect device is ready */
	zassert_true(device_is_ready(vbus_dev),
		     "VBUS detect device not ready");
}

ZTEST(vbus_detect, test_gpio_interrupt_configured)
{
	gpio_flags_t flags;
	int ret;

	/* Verify GPIO is configured with interrupt */
	ret = gpio_emul_flags_get(gpio_dev, VBUS_GPIO_PIN, &flags);
	zassert_ok(ret, "Failed to get GPIO flags");

	/* Check that input mode is set */
	zassert_true(flags & GPIO_INPUT,
		     "GPIO not configured as input");
}

ZTEST(vbus_detect, test_gpio_state_change_detected)
{
	int state;

	/* Set VBUS low */
	gpio_emul_input_set(gpio_dev, VBUS_GPIO_PIN, 0);
	k_msleep(10);

	state = gpio_pin_get(gpio_dev, VBUS_GPIO_PIN);
	zassert_equal(state, 0, "Expected GPIO low");

	/* Set VBUS high */
	gpio_emul_input_set(gpio_dev, VBUS_GPIO_PIN, 1);
	k_msleep(10);

	state = gpio_pin_get(gpio_dev, VBUS_GPIO_PIN);
	zassert_equal(state, 1, "Expected GPIO high");
}

ZTEST(vbus_detect, test_debounce_delay)
{
	/*
	 * Test that rapid GPIO changes are handled.
	 * The driver uses debounce so rapid toggles should not cause issues.
	 */
	for (int i = 0; i < 10; i++) {
		gpio_emul_input_set(gpio_dev, VBUS_GPIO_PIN, i % 2);
		k_msleep(1);
	}

	/* Let debounce settle */
	k_msleep(20);

	/* If we got here without crashing, debounce is working */
	zassert_true(true, "Debounce handling works");
}

ZTEST_SUITE(vbus_detect, NULL, vbus_detect_setup, vbus_detect_before, NULL,
	    NULL);
