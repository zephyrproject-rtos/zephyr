/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Sample app to demonstrate PWM-based LED fade
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <device.h>
#include <drivers/pwm.h>

#define PWM_LED0_NODE	DT_ALIAS(pwm_led0)

#if DT_NODE_HAS_STATUS(PWM_LED0_NODE, okay)
#define PWM_LABEL	DT_PWMS_LABEL(PWM_LED0_NODE)
#define PWM_CHANNEL	DT_PWMS_CHANNEL(PWM_LED0_NODE)
#define PWM_FLAGS	DT_PWMS_FLAGS(PWM_LED0_NODE)
#else
#error "Unsupported board: pwm-led0 devicetree alias is not defined"
#define PWM_LABEL	""
#define PWM_CHANNEL	0
#define PWM_FLAGS	0
#endif

/*
 * This period should be fast enough to be above the flicker fusion
 * threshold. The steps should also be small enough, and happen
 * quickly enough, to make the output fade change appear continuous.
 */
#define PERIOD_USEC	20000U
#define NUM_STEPS	50U
#define STEP_USEC	(PERIOD_USEC / NUM_STEPS)
#define SLEEP_MSEC	25U

void main(void)
{
	struct device *pwm;
	uint32_t pulse_width = 0U;
	uint8_t dir = 1U;
	int ret;

	printk("PWM-based LED fade\n");

	pwm = device_get_binding(PWM_LABEL);
	if (!pwm) {
		printk("Error: didn't find %s device\n", PWM_LABEL);
		return;
	}

	while (1) {
		ret = pwm_pin_set_usec(pwm, PWM_CHANNEL, PERIOD_USEC,
				       pulse_width, PWM_FLAGS);
		if (ret) {
			printk("Error %d: failed to set pulse width\n", ret);
			return;
		}

		if (dir) {
			pulse_width += STEP_USEC;
			if (pulse_width >= PERIOD_USEC) {
				pulse_width = PERIOD_USEC - STEP_USEC;
				dir = 0U;
			}
		} else {
			if (pulse_width >= STEP_USEC) {
				pulse_width -= STEP_USEC;
			} else {
				pulse_width = STEP_USEC;
				dir = 1U;
			}
		}

		k_sleep(K_MSEC(SLEEP_MSEC));
	}
}
