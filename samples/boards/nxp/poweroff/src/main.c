/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/sys_clock.h>
#include <zephyr/console/console.h>

#if defined(CONFIG_COUNTER_WAKEUP_ENABLE)
#include <zephyr/drivers/counter.h>
#endif

#if defined(CONFIG_GPIO_WAKEUP_ENABLE)
#include <zephyr/drivers/gpio.h>
#endif

int main(void)
{
	int ret;

#if defined(CONFIG_COUNTER_WAKEUP_ENABLE)
	const struct device *dev = DEVICE_DT_GET(DT_ALIAS(poweroff_wakeup_source));

	struct counter_top_cfg top_cfg = {
		.ticks = counter_us_to_ticks(dev, 5 * USEC_PER_SEC),
		.callback = NULL,
		.user_data = NULL,
		.flags = 0,
	};

	if (!device_is_ready(dev)) {
		printf("Wakeup counter device not ready\n");
		return 0;
	}

	ret = counter_set_top_value(dev, &top_cfg);

	if (ret < 0) {
		printf("Could not set wakeup counter value (%d)\n", ret);
		return 0;
	}

	ret = counter_start(dev);
	if (ret < 0) {
		printf("Could not start wakeup counter (%d)\n", ret);
		return 0;
	}

	console_init();
	printf("Press 'enter' key to power off the system\n");
	console_getchar();
	printf("Will wakeup after 5 seconds\n");
#endif

#if defined(CONFIG_GPIO_WAKEUP_ENABLE)
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
#endif

	printf("Powering off\n");

	sys_poweroff();

	return 0;
}
