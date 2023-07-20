/*
 * Copyright 2022, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/sys/poweroff.h>

#define SOFT_OFF_S 10U

#define RTC_NODE DT_NODELABEL(rtc)
#if !DT_NODE_HAS_STATUS(RTC_NODE, okay)
#error "Unsupported board: rtc node is not enabled"
#endif

#define RTC_CHANNEL_ID 0

static const struct device *const rtc_dev = DEVICE_DT_GET(RTC_NODE);

int main(void)
{
	int ret;

	printk("\n%s system off demo\n", CONFIG_BOARD);

	uint32_t sleep_ticks = counter_us_to_ticks(rtc_dev, SOFT_OFF_S * 1000ULL * 1000ULL);

	counter_start(rtc_dev);

	if (sleep_ticks == 0) {
		/* counter_us_to_ticks will round down the number of ticks to the nearest int. */
		/* Ensure at least one tick is used in the RTC */
		sleep_ticks++;
	}
	const struct counter_alarm_cfg alarm_cfg = {
		.ticks = sleep_ticks,
		.flags = 0,
	};

	ret = counter_set_channel_alarm(rtc_dev, RTC_CHANNEL_ID, &alarm_cfg);
	if (ret != 0) {
		printk("Could not rtc alarm.\n");
		return 0;
	}
	printk("RTC Alarm set for %llu seconds to wake from soft-off.\n",
	       counter_ticks_to_us(rtc_dev, alarm_cfg.ticks) / (1000ULL * 1000ULL));
	printk("Entering system off");

	sys_poweroff();

	return 0;
}
