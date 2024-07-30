/*
 * Copyright (c) 2024, Muhammad Waleed Badar 
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/sys/util.h>
#include <stdio.h>

static int32_t set_date_time(const struct device *dev)
{
	int32_t ret = 0;
	struct rtc_time tm = {
		.tm_year = 2024 - 1900,
		.tm_mon	 = 7 - 1,
		.tm_mday = 28,
		.tm_hour = 3,
		.tm_min	 = 51,
		.tm_sec	 = 0
	};

	ret = rtc_set_time(dev, &tm);
	if (ret < 0) {
		printk("Cannot write date time: %d\n", ret);
		return ret;
	}
	return ret;
}


static int32_t get_date_time(const struct device *dev)
{
	int32_t ret = 0;
	struct rtc_time tm;
	
	ret = rtc_get_time(dev, &tm);
	if (ret < 0) {
		printk("Cannot read date time: %d\n", ret);
		return ret;
	}

	printk("Current RTC time: %04d-%02d-%02d %02d:%02d:%02d\n",
		tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec);

	return ret;
}

int main(void)
{
	int32_t ret = 0;

	/* Get the device structure for the Tiny RTC */
	const struct device *const dev =
		DEVICE_DT_GET_ONE(maxim_ds1307);

	/* Check if the Tiny RTC device is available */
	if (dev == NULL) {
		printk("Error: no device found\n");
		return 0;
	}

	/* Check if the Tiny RTC device is ready */
	if (!device_is_ready(dev)) {
		printk("Device is not ready\n");
		return 0;
	}

	/* Set the time Once. */
	// set_date_time(dev);

	/* Continuously read the current time from the Tiny RTC */
	while (1) {
		ret = get_date_time(dev);
		if (ret < 0) {
			printk("Failed to read tiny rtc\n");
			return 0;
		}
		k_sleep(K_MSEC(1000));  // Sleep for 1 second
	}
	return 0;
}
