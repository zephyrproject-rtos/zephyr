/*
 * Copyright (c) 2020 Seagate Technology LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <errno.h>
#include <drivers/led.h>
#include <sys/util.h>
#include <zephyr.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

#if DT_NODE_HAS_STATUS(DT_INST(0, pwm_leds), okay)
#define LED_PWM_NODE_ID		DT_INST(0, pwm_leds)
#define LED_PWM_DEV_NAME	DT_INST_PROP_OR(0, label, "LED_PWM_0")
#else
#error "No LED PWM device found"
#endif

#define LED_PWM_LABEL(led_node_id) DT_PROP_OR(led_node_id, label, NULL),

const char *led_label[] = {
	DT_FOREACH_CHILD(LED_PWM_NODE_ID, LED_PWM_LABEL)
};

const int num_leds = ARRAY_SIZE(led_label);

#define MAX_BRIGHTNESS	100

#define FADE_DELAY_MS	10
#define FADE_DELAY	K_MSEC(FADE_DELAY_MS)

/**
 * @brief Run tests on a single LED using the LED API syscalls.
 *
 * @param led_pwm LED PWM device.
 * @param led Number of the LED to test.
 */
static void run_led_test(const struct device *led_pwm, uint8_t led)
{
	int err;
	uint16_t level;

	LOG_INF("Testing LED %d - %s", led, led_label[led] ? : "no label");

	/* Turn LED on. */
	err = led_on(led_pwm, led);
	LOG_INF("  Turning on");
	if (err < 0) {
		LOG_ERR("err=%d", err);
		return;
	}
	k_sleep(K_MSEC(1000));

	/* Turn LED off. */
	err = led_off(led_pwm, led);
	LOG_INF("  Turning off");
	if (err < 0) {
		LOG_ERR("err=%d", err);
		return;
	}
	k_sleep(K_MSEC(1000));

	/* Increase LED brightness gradually up to the maximum level. */
	LOG_INF("  Increasing brightness gradually");
	for (level = 0; level <= MAX_BRIGHTNESS; level++) {
		err = led_set_brightness(led_pwm, led, level);
		if (err < 0) {
			LOG_ERR("err=%d brightness=%d\n", err, level);
			return;
		}
		k_sleep(FADE_DELAY);
	}
	k_sleep(K_MSEC(1000));

	/* Set LED blinking (on: 0.1 sec, off: 0.1 sec) */
	LOG_INF("  Blinking on: 0.1 sec, off: 0.1 sec");
	err = led_blink(led_pwm, led, 100, 100);
	if (err < 0) {
		LOG_ERR("err=%d", err);
		return;
	}
	k_sleep(K_MSEC(5000));

	/* Enable LED blinking (on: 1 sec, off: 1 sec) */
	LOG_INF("  Blinking on: 1 sec, off: 1 sec");
	err = led_blink(led_pwm, led, 1000, 1000);
	if (err < 0) {
		LOG_ERR("err=%d", err);
		return;
	}
	k_sleep(K_MSEC(5000));

	/* Turn LED off. */
	LOG_INF("  Turning off");
	err = led_off(led_pwm, led);
	if (err < 0) {
		LOG_ERR("err=%d", err);
		return;
	}
}

void main(void)
{
	const struct device *led_pwm;
	uint8_t led;

	led_pwm = device_get_binding(LED_PWM_DEV_NAME);
	if (led_pwm) {
		LOG_INF("Found device %s", LED_PWM_DEV_NAME);
	} else {
		LOG_ERR("Device %s not found", LED_PWM_DEV_NAME);
		return;
	}

	if (!num_leds) {
		LOG_ERR("No LEDs found for %s", LED_PWM_DEV_NAME);
		return;
	}

	do {
		for (led = 0; led < num_leds; led++) {
			run_led_test(led_pwm, led);
		}
	} while (true);
}
