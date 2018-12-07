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

#if defined(CONFIG_SOC_STM32F401XE) || defined(CONFIG_SOC_STM32L476XG)
#define PWM_DRIVER DT_PWM_STM32_2_DEV_NAME
#define PWM_CHANNEL 1
#elif CONFIG_SOC_STM32F103XB
#define PWM_DRIVER DT_PWM_STM32_1_DEV_NAME
#define PWM_CHANNEL 1
#elif defined(CONFIG_SOC_QUARK_SE_C1000) || defined(CONFIG_SOC_QUARK_D2000)
#define PWM_DRIVER CONFIG_PWM_QMSI_DEV_NAME
#define PWM_CHANNEL 0
#elif defined(CONFIG_SOC_FAMILY_NRF)
#if defined(CONFIG_PWM_NRF5_SW)
#define PWM_DRIVER CONFIG_PWM_NRF5_SW_0_DEV_NAME
#else
#define PWM_DRIVER CONFIG_PWM_0_NAME
#endif  /* CONFIG_PWM_NRF5_SW */
#define PWM_CHANNEL LED0_GPIO_PIN
#elif defined(CONFIG_SOC_ESP32)
#define PWM_DRIVER CONFIG_PWM_LED_ESP32_DEV_NAME_0
#define PWM_CHANNEL	21
#elif defined(PWM_LED0_PWM_CONTROLLER) && defined(PWM_LED0_PWM_CHANNEL)
/* get the defines from dt (based on alias 'pwm-led0') */
#define PWM_DRIVER	PWM_LED0_PWM_CONTROLLER
#define PWM_CHANNEL	PWM_LED0_PWM_CHANNEL
#elif defined(CONFIG_BOARD_HIFIVE1)
/* Blink the blue channel of the RGB LED */
#define PWM_DRIVER	LED1_PWM_CONTROLLER
#define PWM_CHANNEL	LED1_PWM_CHANNEL
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
	u32_t pulse_width = 0U;
	u8_t dir = 0U;

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
				dir = 0U;
				pulse_width = 0U;
			} else {
				pulse_width -= FADESTEP;
			}
		} else {
			pulse_width += FADESTEP;

			if (pulse_width >= PERIOD) {
				dir = 1U;
				pulse_width = PERIOD;
			}
		}

		k_sleep(MSEC_PER_SEC);
	}
}
