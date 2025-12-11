/*
 * Copyright (c) 2020 Seagate Technology LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <errno.h>
#include <zephyr/drivers/led.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

#define LED_PWM_NODE_ID	 DT_COMPAT_GET_ANY_STATUS_OKAY(pwm_leds)

const char *led_label[] = {
	DT_FOREACH_CHILD_SEP_VARGS(LED_PWM_NODE_ID, DT_PROP_OR, (,), label, NULL)
};

#if CONFIG_BLINK_DELAY_AUTO_FALLBACK
/* Generic macro to get PWM period for each child */
#define GET_LED_PERIOD(node_id) DT_PWMS_PERIOD(node_id),

/* Create array of PWM periods */
static const uint32_t led_periods_ns[] = {
	DT_FOREACH_CHILD(LED_PWM_NODE_ID, GET_LED_PERIOD)
};

/**
 * @brief Automatically calculate and apply a fallback blink delay for an LED
 *
 * This function calculates an appropriate blink delay based on the LED's PWM period
 * when the requested delay exceeds the PWM hardware capabilities. The delay is
 * calculated by dividing the PWM period by a divisor and ensuring a minimum
 * visibility threshold.
 *
 * @param led_pwm LED PWM device
 * @param led LED index to configure
 * @param led_delay Pointer to store the calculated delay (in milliseconds)
 * @param div Divisor to scale down the PWM period for delay calculation
 *
 * @return 0 on success, negative error code on failure from led_blink()
 */
static int blink_delay_auto_fallback(const struct device *led_pwm, uint8_t led,
					uint32_t *led_delay, uint32_t div)
{
	*led_delay = led_periods_ns[led] / 1000000 / div / 2;
	/* Ensure minimum 1ms for visibility */
	*led_delay = MAX(1, *led_delay);

	LOG_INF("Auto-calculated delay: %u ms.", *led_delay);

	return led_blink(led_pwm, led, *led_delay, *led_delay);
}
#endif

const int num_leds = ARRAY_SIZE(led_label);

#define MAX_BRIGHTNESS	100

/**
 * @brief Run tests on a single LED using the LED API syscalls.
 *
 * @param led_pwm LED PWM device.
 * @param led Number of the LED to test.
 */
static void run_led_test(const struct device *led_pwm, uint8_t led)
{
	int err;
	int16_t level;
	uint32_t led_delay;

	LOG_INF("Testing LED %d - %s", led, led_label[led] ? : "no label");

	/* Turn LED on. */
	err = led_on(led_pwm, led);
	if (err < 0) {
		LOG_ERR("err=%d", err);
		return;
	}
	LOG_INF("  Turned on");
	k_sleep(K_MSEC(1000));

	/* Turn LED off. */
	err = led_off(led_pwm, led);
	if (err < 0) {
		LOG_ERR("err=%d", err);
		return;
	}
	LOG_INF("  Turned off");
	k_sleep(K_MSEC(1000));

	/* Increase LED brightness gradually up to the maximum level. */
	LOG_INF("  Increasing brightness gradually");
	for (level = 0; level <= MAX_BRIGHTNESS; level++) {
		err = led_set_brightness(led_pwm, led, level);
		if (err < 0) {
			LOG_ERR("err=%d brightness=%d\n", err, level);
			return;
		}
		k_sleep(K_MSEC(CONFIG_FADE_DELAY));
	}
	k_sleep(K_MSEC(1000));

	/* Decrease LED brightness gradually down to the minimum level. */
	LOG_INF("  Decreasing brightness gradually");
	for (level = MAX_BRIGHTNESS; level >= 0; level--) {
		err = led_set_brightness(led_pwm, led, level);
		if (err < 0) {
			LOG_ERR("err=%d brightness=%d\n", err, level);
			return;
		}
		k_sleep(K_MSEC(CONFIG_FADE_DELAY));
	}
	k_sleep(K_MSEC(1000));

#if CONFIG_BLINK_DELAY_SHORT > 0
	led_delay = (uint32_t)CONFIG_BLINK_DELAY_SHORT;
	/* Start LED blinking (short cycle) */
	err = led_blink(led_pwm, led, led_delay, led_delay);
	if (err < 0) {
#if CONFIG_BLINK_DELAY_AUTO_FALLBACK && CONFIG_BLINK_DELAY_SHORT_LED_PERIOD_DIV > 0
		LOG_INF("CONFIG_BLINK_DELAY_SHORT (%u ms) exceeds PWM capabilities for LED %u.",
			led_delay, led);
		err = blink_delay_auto_fallback(led_pwm, led, &led_delay,
			(uint32_t)CONFIG_BLINK_DELAY_SHORT_LED_PERIOD_DIV);
#endif
		if (err < 0) {
			LOG_ERR("err=%d", err);
			return;
		}
	}
	LOG_INF("  Blinking on: %u msec, off: %u msec", led_delay, led_delay);
	k_sleep(K_MSEC(5000));
#endif

#if CONFIG_BLINK_DELAY_LONG > 0
	led_delay = (uint32_t)CONFIG_BLINK_DELAY_LONG;
	/* Start LED blinking (long cycle) */
	err = led_blink(led_pwm, led, led_delay, led_delay);
	if (err < 0) {
#if CONFIG_BLINK_DELAY_AUTO_FALLBACK && CONFIG_BLINK_DELAY_LONG_LED_PERIOD_DIV > 0
		LOG_INF("CONFIG_BLINK_DELAY_LONG (%u ms) exceeds PWM capabilities for LED %u.",
			led_delay, led);
		err = blink_delay_auto_fallback(led_pwm, led, &led_delay,
			(uint32_t)CONFIG_BLINK_DELAY_LONG_LED_PERIOD_DIV);
#endif
		if (err < 0) {
			LOG_ERR("err=%d", err);
			LOG_INF("  Cycle period not supported - on: %u msec, "
				"off: %u msec", led_delay, led_delay);
		}
	}
	LOG_INF("  Blinking on: %u msec, off: %u msec", led_delay, led_delay);
	k_sleep(K_MSEC(5000));
#endif

	/* Turn LED off. */
	err = led_off(led_pwm, led);
	if (err < 0) {
		LOG_ERR("err=%d", err);
		return;
	}
	LOG_INF("  Turned off, loop end");
}

int main(void)
{
	const struct device *led_pwm;
	uint8_t led;

	led_pwm = DEVICE_DT_GET(LED_PWM_NODE_ID);
	if (!device_is_ready(led_pwm)) {
		LOG_ERR("Device %s is not ready", led_pwm->name);
		return 0;
	}

	if (!num_leds) {
		LOG_ERR("No LEDs found for %s", led_pwm->name);
		return 0;
	}

	do {
		for (led = 0; led < num_leds; led++) {
			run_led_test(led_pwm, led);
		}
	} while (true);
	return 0;
}
