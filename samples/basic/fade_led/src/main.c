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

#if defined(CONFIG_SOC_STM32F401XE) || defined(CONFIG_SOC_STM32L476XG)
#define PWM_DRIVER CONFIG_PWM_STM32_2_DEV_NAME
#define PWM_CHANNEL 1
#elif CONFIG_SOC_STM32F103XB
#define PWM_DRIVER CONFIG_PWM_STM32_1_DEV_NAME
#define PWM_CHANNEL 1
#elif defined(CONFIG_SOC_QUARK_SE_C1000) || defined(CONFIG_SOC_QUARK_D2000)
#define PWM_DRIVER CONFIG_PWM_QMSI_DEV_NAME
#define PWM_CHANNEL 0
#elif defined(CONFIG_SOC_FAMILY_NRF)
#include <board.h>
#define PWM_DRIVER CONFIG_PWM_NRF5_SW_0_DEV_NAME
#define PWM_CHANNEL LED0_GPIO_PIN
#elif defined(CONFIG_BOARD_HEXIWEAR_K64)
#define PWM_DRIVER	GREEN_PWM_NAME
#define PWM_CHANNEL	GREEN_PWM_CHANNEL
#elif defined(CONFIG_SOC_ESP32)
#define PWM_DRIVER CONFIG_PWM_LED_ESP32_DEV_NAME_0
#define PWM_CHANNEL	21
#else
#error "Choose supported PWM driver"
#endif

/*
 * 50 is flicker fusion threshold. Modulated light will be perceived
 * as steady by our eyes when blinking rate is at least 50.
 */
#define PERIOD (USEC_PER_SEC / 50)

/* in micro second */
#define FADESTEP	2000

void main(void)
{
	struct device *pwm_dev;
	u32_t pulse_width = 0;
	u8_t dir = 0;

	printk("PWM demo app-fade LED\n");

	pwm_dev = device_get_binding(PWM_DRIVER);
	if (!pwm_dev) {
		printk("Cannot find %s!\n", PWM_DRIVER);
		return;
	}

	while (1) {
		if (pwm_pin_set_usec(pwm_dev, PWM_CHANNEL,
					PERIOD, pulse_width)) {
			printk("pwm pin set fails\n");
			return;
		}

		if (dir) {
			if (pulse_width < FADESTEP) {
				dir = 0;
				pulse_width = 0;
			} else {
				pulse_width -= FADESTEP;
			}
		} else {
			pulse_width += FADESTEP;

			if (pulse_width >= PERIOD) {
				dir = 1;
				pulse_width = PERIOD;
			}
		}

		k_sleep(MSEC_PER_SEC);
	}
}
