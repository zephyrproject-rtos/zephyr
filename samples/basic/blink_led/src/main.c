/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Sample app to demonstrate PWM.
 *
 * This app uses PWM[0].
 */

#include <zephyr.h>
#include <misc/printk.h>
#include <device.h>
#include <pwm.h>
#include <board.h>

#if defined(CONFIG_SOC_STM32F401XE) || defined(CONFIG_SOC_STM32F412ZG) || \
	defined(CONFIG_SOC_STM32F413XH) || defined(CONFIG_SOC_STM32L476XG)
#define PWM_DRIVER CONFIG_PWM_STM32_2_DEV_NAME
#define PWM_CHANNEL 1
#elif CONFIG_SOC_STM32F103XB
#define PWM_DRIVER CONFIG_PWM_STM32_1_DEV_NAME
#define PWM_CHANNEL 1
#elif defined(CONFIG_SOC_QUARK_SE_C1000) || defined(CONFIG_SOC_QUARK_D2000)
#define PWM_DRIVER CONFIG_PWM_QMSI_DEV_NAME
#define PWM_CHANNEL 0
#elif defined(CONFIG_BOARD_HEXIWEAR_K64)
#define PWM_DRIVER	GREEN_PWM_NAME
#define PWM_CHANNEL	GREEN_PWM_CHANNEL
#elif !defined(CONFIG_PWM)
#error "Choose supported PWM driver"
#endif

/* in micro second */
#define MIN_PERIOD	(USEC_PER_SEC / 64)

/* in micro second */
#define MAX_PERIOD	USEC_PER_SEC

void main(void)
{
	struct device *pwm_dev;
	u32_t period = MAX_PERIOD;
	u8_t dir = 0;

	printk("PWM demo app-blink LED\n");

	pwm_dev = device_get_binding(PWM_DRIVER);
	if (!pwm_dev) {
		printk("Cannot find %s!\n", PWM_DRIVER);
		return;
	}

	while (1) {
		if (pwm_pin_set_usec(pwm_dev, PWM_CHANNEL,
				     period, period / 2)) {
			printk("pwm pin set fails\n");
			return;
		}

		if (dir) {
			period *= 2;

			if (period > MAX_PERIOD) {
				dir = 0;
				period = MAX_PERIOD;
			}
		} else {
			period /= 2;

			if (period < MIN_PERIOD) {
				dir = 1;
				period = MIN_PERIOD;
			}
		}

		k_sleep(MSEC_PER_SEC * 4);
	}
}
