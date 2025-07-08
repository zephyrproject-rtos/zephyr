/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/sys_clock.h>
#include <zephyr/console/console.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

int main(void)
{
	int ret;
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(lptmr0));

	struct counter_top_cfg top_cfg = {
		.ticks = counter_us_to_ticks(dev, 5000000),
		.callback = NULL,
		.user_data = NULL,
		.flags = 0,
	};

	console_init();

	if (!device_is_ready(dev)) {
		printf("Counter device not ready\n");
		return 0;
	}

	ret = counter_set_top_value(dev, &top_cfg);

	printf("Will wakeup after 5 seconds\n");
	printf("Press key to power off the system\n");
	console_getchar();

	ret = counter_start(dev);
	if (ret < 0) {
		printf("Could not start wakeup counter (%d)\n", ret);
		return 0;
	}

	printf("Powering off\n");

	sys_poweroff();

	return 0;
}
