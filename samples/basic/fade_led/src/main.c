/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Sample app to demonstrate PWM-based LED fade
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>

#define PWM_LED_ALIAS(i) DT_ALIAS(_CONCAT(pwm_led, i))
#define PWM_LED_IS_OKAY(i) DT_NODE_HAS_STATUS_OKAY(DT_PARENT(PWM_LED_ALIAS(i)))
#define PWM_LED(i, _) IF_ENABLED(PWM_LED_IS_OKAY(i), (PWM_DT_SPEC_GET(PWM_LED_ALIAS(i)),))

#define MAX_LEDS 10
static const struct pwm_dt_spec pwm_leds[] = {LISTIFY(MAX_LEDS, PWM_LED, ())};

#define NUM_STEPS	50U
#define SLEEP_MSEC	25U

int main(void)
{
	uint32_t pulse_widths[ARRAY_SIZE(pwm_leds)];
	uint32_t steps[ARRAY_SIZE(pwm_leds)];
	uint8_t dir = 1U;
	int ret;

	printk("PWM-based LED fade. Found %d LEDs\n", ARRAY_SIZE(pwm_leds));

	for (size_t i = 0; i < ARRAY_SIZE(pwm_leds); i++) {
		pulse_widths[i] = 0;
		steps[i] = pwm_leds[i].period / NUM_STEPS;
		if (!pwm_is_ready_dt(&pwm_leds[i])) {
			printk("Error: PWM device %s is not ready\n", pwm_leds[i].dev->name);
			return 0;
		}
	}

	while (1) {
		for (size_t i = 0; i < ARRAY_SIZE(pwm_leds); i++) {
			ret = pwm_set_pulse_dt(&pwm_leds[i], pulse_widths[i]);
			if (ret) {
				printk("Error %d: failed to set pulse width for LED %d\n", ret, i);
			}
			printk("LED %d: Using pulse width %d%%\n", i,
			       100 * pulse_widths[i] / pwm_leds[i].period);

			if (dir) {
				if (pulse_widths[i] + steps[i] >= pwm_leds[i].period) {
					pulse_widths[i] = pwm_leds[i].period;
					dir = 0U;
				} else {
					pulse_widths[i] += steps[i];
				}
			} else {
				if (pulse_widths[i] <= steps[i]) {
					pulse_widths[i] = 0;
					dir = 1U;
				} else {
					pulse_widths[i] -= steps[i];
				}
			}
		}

		k_sleep(K_MSEC(SLEEP_MSEC));
	}
	return 0;
}
