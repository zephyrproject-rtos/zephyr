/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr_pwm_pool.h"

static PWM_POOL_DEFINE(pwm_pool);

int main(void)
{
	if (!pwm_pool_init(&pwm_pool)) {
		printk("pwm_pool_init failed\n");
		return 0;
	}

	printk("Application started with %u PWM\n", pwm_pool.out_len);

#if CONFIG_PWM_POOL_APP_STATIC
	for (size_t pwm = 0;;) {
		/* Increase LED brightness */
		for (size_t permille = 0; permille <= PERMILLE_MAX; permille += 10) {
			if (!pwm_pool_set(&pwm_pool, pwm, PWM_FIXED, permille)) {
				printk("LED%u PWM failed\n", pwm);
			}
			k_msleep(20);
		}
		/* Switch off LED */
		if (!pwm_pool_set(&pwm_pool, pwm, PWM_OFF)) {
			printk("LED%u PWM failed\n", pwm);
		}
		pwm++;
		if (pwm >= pwm_pool.out_len) {
			pwm = 0;
		}
	}
#else
	if (!pwm_pool_set(&pwm_pool, 0, PWM_BREATH, K_MSEC(2000))) {
		printk("LED0 PWM failed\n");
	}
	if (!pwm_pool_set(&pwm_pool, 1, PWM_BREATH, K_MSEC(1000))) {
		printk("LED1 PWM failed\n");
	}
	if (!pwm_pool_set(&pwm_pool, 2, PWM_BLINK, K_MSEC(100), K_MSEC(900))) {
		printk("LED2 PWM failed\n");
	}
#endif /* CONFIG_PWM_POOL_APP_STATIC */
	return 0;
}
