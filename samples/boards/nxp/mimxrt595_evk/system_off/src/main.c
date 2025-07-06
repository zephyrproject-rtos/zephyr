/*
 * Copyright 2022, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/drivers/counter.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/sys_clock.h>

int main(void)
{
	int ret;
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(rtc));
	struct counter_alarm_cfg alarm_cfg = { 0 };

	if (!device_is_ready(dev)) {
		printf("Counter device not ready\n");
		return 0;
	}

	ret = counter_start(dev);
	if (ret < 0) {
		printf("Could not start counter (%d)\n", ret);
		return 0;
	}

	alarm_cfg.ticks = counter_us_to_ticks(dev, 10 * USEC_PER_SEC);

	ret = counter_set_channel_alarm(dev, 0, &alarm_cfg);
	if (ret < 0) {
		printf("Could not set counter channel alarm (%d)\n", ret);
		return 0;
	}

	printf("Wake-up alarm set for 10 seconds\n");
	printf("Powering off\n");

	sys_poweroff();

	return 0;
}
