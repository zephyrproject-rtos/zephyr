/*
 * Copyright (c) 2024 Nuvoton Technology Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <time.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/sys/poweroff.h>

/* Fri Jan 01 2021 13:29:50 GMT+0000 */
#define RTC_TEST_ALARM_SET_TIME		      1609507790
#define RTC_TEST_ALARM_TIME_MINUTE	      30
#define RTC_TEST_ALARM_TIME_HOUR	      13

static const struct device *rtc = DEVICE_DT_GET(DT_ALIAS(rtc));

static void set_alarm_10s(void)
{
	int ret;
	time_t timer_set;
	struct rtc_time time_set;
	struct rtc_time alarm_time_set;
	uint16_t alarm_time_mask_set;

	/* Initialize RTC time to set */
	timer_set = RTC_TEST_ALARM_SET_TIME;

	gmtime_r(&timer_set, (struct tm *)(&time_set));

	time_set.tm_isdst = -1;
	time_set.tm_nsec = 0;

	/* Set RTC time */
	ret = rtc_set_time(rtc, &time_set);

	/* Set alarm time */
	alarm_time_set.tm_min = RTC_TEST_ALARM_TIME_MINUTE;
	alarm_time_set.tm_hour = RTC_TEST_ALARM_TIME_HOUR;
	alarm_time_mask_set = (RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR);
	ret = rtc_alarm_set_time(rtc, 0, alarm_time_mask_set, &alarm_time_set);
	if (ret) {
		printk("Failed to set alarm\n");
	}
}

int main(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(rtc));

	if (!device_is_ready(dev)) {
		printf("Counter device not ready\n");
		return 0;
	}

	printk("Wake-up alarm set for 10 seconds.\n");
	set_alarm_10s();

	/* Set PF.4~PF.11 as digital path disable to save power at power down */
	GPIO_DISABLE_DIGITAL_PATH(PF, BIT11 | BIT10 | BIT9 | BIT8 | BIT7 | BIT6 | BIT5 | BIT4);
	printk("Disable digital path pins\n");

	printk("Powering off ....................\n");
	sys_poweroff();

	return 0;
}
