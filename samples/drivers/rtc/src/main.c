/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <device.h>
#include <rtc.h>
#include <misc/printk.h>

#define ALARM (RTC_ALARM_SECOND)

void test_rtc_interrupt_fn(struct device *rtc_dev)
{
	uint32_t now = rtc_read(rtc_dev);

	printk("Alarm\n");
	rtc_set_alarm(rtc_dev, now + ALARM);
}

void main(void)
{
	struct rtc_config config;
	struct device *rtc_dev;

	printk("Test RTC driver\n");
	rtc_dev = device_get_binding(CONFIG_RTC_0_NAME);

	config.init_val = 0;
	config.alarm_enable = 1;
	config.alarm_val = ALARM;
	config.cb_fn = test_rtc_interrupt_fn;

	rtc_enable(rtc_dev);

	rtc_set_config(rtc_dev, &config);

	while (1) {
		/* do nothing */
	}
}
