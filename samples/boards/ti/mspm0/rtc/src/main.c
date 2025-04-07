/*
 * Copyright (c) 2024, Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/sys/util.h>

const struct device *const rtc = DEVICE_DT_GET(DT_ALIAS(rtc));

static int set_date_time(const struct device *rtc)
{
	int ret = 0;
	struct rtc_time tm = {
		.tm_year = 2024 - 1900,
		.tm_mon = 11 - 1,
		.tm_mday = 17,
		.tm_hour = 11,
		.tm_min = 20,
		.tm_sec = 0,
	};

	ret = rtc_set_time(rtc, &tm);
	if (ret < 0) {
		printk("Cannot write date time: %d\n", ret);
		return ret;
	}
	return ret;
}

static int get_date_time(const struct device *rtc)
{
	int ret = 0;
	struct rtc_time tm;

	ret = rtc_get_time(rtc, &tm);
	if (ret < 0) {
		printk("Cannot read date time: %d\n", ret);
		return ret;
	}

	printk("RTC date and time: %04d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900,
	       tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);


	return ret;
}

static int set_alarm(const struct device *rtc)
{
	int ret = 0;
	struct rtc_time tm = {0};

	ret = rtc_get_time(rtc, &tm);
	if (ret < 0) {
		printk("Cannot read date time: %d\n", ret);
		return ret;
	}

	tm.tm_min += 1;
	ret = rtc_alarm_set_time(rtc, 1, RTC_ALARM_TIME_MASK_MINUTE, &tm);
	if (ret < 0) {
		printk("Failed to set alarm %d\n", ret);
		return ret;
	}

	printk("alarm set success\n");

	return ret;
}

static void alarm_callback(const struct device *dev, uint16_t id, void *userdata)
{
	printk("alarm %u is triggered\n", id);
}

int main(void)
{
	int ret;
	/* Check if the RTC is ready */
	if (!device_is_ready(rtc)) {
		printk("Device is not ready\n");
		return 0;
	}

	set_date_time(rtc);

	ret = rtc_alarm_set_callback(rtc, 1, alarm_callback, NULL);
	if (ret < 0) {
		printk("Failed to set callback %d\n", ret);
		return ret;
	}

	set_alarm(rtc);
	/* Continuously read the current date and time from the RTC */
	while (get_date_time(rtc) == 0) {
		k_sleep(K_MSEC(1000));
	};

	return 0;
}
