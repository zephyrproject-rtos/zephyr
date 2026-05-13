/*
 * Copyright (c) 2026 Siemens
 * Copyright (c) 2026 Stefan Gloor
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/drivers/led.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define LED_NODE DT_ALIAS(led0)

/* GPIO backing the LED under test */
#define LED_GPIO_NODE DT_GPIO_CTLR(LED_NODE, gpios)
#define LED_GPIO_PIN  DT_GPIO_PIN(LED_NODE, gpios)
#define LED_GPIO_FLAGS DT_GPIO_FLAGS(LED_NODE, gpios)

static const struct device *led_dev = DEVICE_DT_GET(DT_PARENT(LED_NODE));
static const struct device *gpio_dev = DEVICE_DT_GET(LED_GPIO_NODE);

static int led_pin_get(void)
{
	int val;

	val = gpio_emul_output_get(gpio_dev, LED_GPIO_PIN);
	zassert_true(val >= 0, "gpio_emul_output_get failed: %d", val);

	if (LED_GPIO_FLAGS & GPIO_ACTIVE_LOW) {
		val = !val;
	}

	return val;
}

static void *led_trigger_setup(void)
{
	zassert_true(device_is_ready(led_dev),
		     "LED device not ready");
	zassert_true(device_is_ready(gpio_dev),
		     "GPIO device not ready");
	return NULL;
}

static void led_trigger_before(void *fixture)
{
	ARG_UNUSED(fixture);

	/* led_off cancels any active trigger and forces the LED off,
	 * giving each test a known starting state.
	 */
	led_off(led_dev, 0);
	k_sleep(K_MSEC(50));
}

ZTEST(led_trigger, test_blink_starts)
{
	int ret;

	ret = led_blink(led_dev, 0, 100, 100);
	zassert_ok(ret, "led_blink failed: %d", ret);

	k_sleep(K_MSEC(10));
	zassert_equal(led_pin_get(), 1,
		      "LED should be ON at start of blink");

	k_sleep(K_MSEC(110));
	zassert_equal(led_pin_get(), 0,
		      "LED should be OFF after delay_on elapsed");

	k_sleep(K_MSEC(110));
	zassert_equal(led_pin_get(), 1,
		      "LED should be ON again after delay_off elapsed");
}

ZTEST(led_trigger, test_blink_stop_off)
{
	int ret;

	ret = led_blink(led_dev, 0, 100, 100);
	zassert_ok(ret, "led_blink failed: %d", ret);
	k_sleep(K_MSEC(50));

	/* delay_on == 0 (with delay_off != 0) cancels and turns the LED off. */
	ret = led_blink(led_dev, 0, 0, 100);
	zassert_ok(ret, "led_blink stop failed: %d", ret);

	k_sleep(K_MSEC(10));
	zassert_equal(led_pin_get(), 0,
		      "LED should be OFF after blink stopped");

	k_sleep(K_MSEC(200));
	zassert_equal(led_pin_get(), 0,
		      "LED should remain OFF");
}

ZTEST(led_trigger, test_blink_stop_stay_on)
{
	int ret;

	ret = led_blink(led_dev, 0, 100, 100);
	zassert_ok(ret, "led_blink failed: %d", ret);
	k_sleep(K_MSEC(50));

	/* delay_off == 0 cancels and leaves the LED lit. */
	ret = led_blink(led_dev, 0, 100, 0);
	zassert_ok(ret, "led_blink stop failed: %d", ret);

	k_sleep(K_MSEC(10));
	zassert_equal(led_pin_get(), 1,
		      "LED should remain ON after delay_off=0 stop");

	k_sleep(K_MSEC(200));
	zassert_equal(led_pin_get(), 1,
		      "LED should remain ON");
}

ZTEST(led_trigger, test_blink_stop_both_zero_turns_off)
{
	int ret;

	ret = led_blink(led_dev, 0, 100, 100);
	zassert_ok(ret, "led_blink failed: %d", ret);
	k_sleep(K_MSEC(50));

	/* When both delays are zero, delay_on == 0 wins -> LED off. */
	ret = led_blink(led_dev, 0, 0, 0);
	zassert_ok(ret, "led_blink stop failed: %d", ret);

	k_sleep(K_MSEC(10));
	zassert_equal(led_pin_get(), 0,
		      "LED should be OFF when both delays are zero");
}

ZTEST(led_trigger, test_led_on_cancels_blink)
{
	int ret;

	ret = led_blink(led_dev, 0, 100, 100);
	zassert_ok(ret, "led_blink failed: %d", ret);
	k_sleep(K_MSEC(50));

	ret = led_on(led_dev, 0);
	zassert_ok(ret, "led_on failed: %d", ret);
	k_sleep(K_MSEC(10));
	zassert_equal(led_pin_get(), 1,
		      "LED should be ON after led_on");

	k_sleep(K_MSEC(250));
	zassert_equal(led_pin_get(), 1,
		      "LED should remain ON (blink cancelled)");
}

ZTEST(led_trigger, test_led_off_cancels_blink)
{
	int ret;

	ret = led_blink(led_dev, 0, 100, 100);
	zassert_ok(ret, "led_blink failed: %d", ret);
	k_sleep(K_MSEC(50));

	ret = led_off(led_dev, 0);
	zassert_ok(ret, "led_off failed: %d", ret);
	k_sleep(K_MSEC(10));
	zassert_equal(led_pin_get(), 0,
		      "LED should be OFF after led_off");

	k_sleep(K_MSEC(250));
	zassert_equal(led_pin_get(), 0,
		      "LED should remain OFF (blink cancelled)");
}

ZTEST(led_trigger, test_blink_update_timing)
{
	int ret;

	ret = led_blink(led_dev, 0, 100, 100);
	zassert_ok(ret, "led_blink failed: %d", ret);
	k_sleep(K_MSEC(50));

	ret = led_blink(led_dev, 0, 200, 200);
	zassert_ok(ret, "led_blink update failed: %d", ret);

	int initial = led_pin_get();

	k_sleep(K_MSEC(210));
	int after = led_pin_get();

	zassert_not_equal(initial, after,
			  "LED should have toggled with new timing");
}

ZTEST(led_trigger, test_set_brightness_during_blink)
{
	int ret;

	ret = led_blink(led_dev, 0, 100, 100);
	zassert_ok(ret, "led_blink failed: %d", ret);
	k_sleep(K_MSEC(50));

	ret = led_set_brightness(led_dev, 0, 50);
	zassert_ok(ret, "led_set_brightness failed: %d", ret);

	/* Verify blink is still active by checking for a toggle */
	int val1 = led_pin_get();

	k_sleep(K_MSEC(120));
	int val2 = led_pin_get();

	zassert_not_equal(val1, val2,
			  "Blink should still be active after set_brightness");
}

ZTEST(led_trigger, test_pool_exhaustion)
{
	int ret;

	/* Fill up the pool (4 slots) - channel 0 is valid, others provoke
	 * driver errors but still claim pool slots.
	 */
	for (uint32_t i = 0; i < CONFIG_LED_TRIGGER_MAX_CHANNELS; i++) {
		ret = led_blink(led_dev, i, 500, 500);
		/* Channel 0 should succeed; others may fail in the driver
		 * (GPIO out of range) but the trigger slot is still claimed.
		 */
		if (i == 0) {
			zassert_ok(ret, "led_blink ch0 failed: %d", ret);
		}
	}

	/* Next allocation should fail with -ENOMEM */
	ret = led_blink(led_dev, CONFIG_LED_TRIGGER_MAX_CHANNELS, 500, 500);
	zassert_equal(ret, -ENOMEM,
		      "Expected -ENOMEM when pool exhausted, got %d", ret);

	/* Cleanup: stop all blinks */
	for (uint32_t i = 0; i <= CONFIG_LED_TRIGGER_MAX_CHANNELS; i++) {
		led_blink(led_dev, i, 0, 0);
	}
}

ZTEST_SUITE(led_trigger, NULL, led_trigger_setup, led_trigger_before, NULL, NULL);
