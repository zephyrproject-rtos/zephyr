/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/sys_clock.h>
#include <zephyr/console/console.h>
#include <zephyr/drivers/counter.h>

int main(void)
{
	int ret;

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

	printf("Powering off\n");

	sys_poweroff();

	return 0;
}
