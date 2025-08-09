/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/sys_clock.h>
#include <zephyr/console/console.h>
#include <zephyr/drivers/gpio.h>

int main(void)
{
	int ret;

	const struct gpio_dt_spec wakeup_button =
		GPIO_DT_SPEC_GET(DT_ALIAS(poweroff_wakeup_source), gpios);

	ret = gpio_pin_configure_dt(&wakeup_button, GPIO_INPUT);
	if (ret < 0) {
		printf("Could not configure wakeup GPIO (%d)\n", ret);
		return 0;
	}

	ret = gpio_pin_interrupt_configure_dt(&wakeup_button, GPIO_INT_LEVEL_ACTIVE);
	if (ret < 0) {
		printf("Could not configure wakeup GPIO interrupt (%d)\n", ret);
		return 0;
	}

	console_init();
	printf("Press 'enter' key to power off the system\n");
	console_getchar();
	printf("Will wakeup after press wakeup button\n");

	printf("Powering off\n");

	sys_poweroff();

	return 0;
}
