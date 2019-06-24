/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <device.h>
#include <counter.h>
#include <misc/printk.h>
#include <drivers/rtc/ds3231.h>

void main(void)
{
	struct device *ds3231;
	char buf[128];

	printk("Test DS3231 driver\n");
	ds3231 = device_get_binding(DT_INST_0_MAXIM_DS3231_LABEL);
	if (!ds3231) {
		printk("No device available\n");
		return;
	}

	printk("Counter at %p\n", ds3231);
	printk("\tMax top value: %u (%08x)\n",
	       counter_get_max_top_value(ds3231),
	       counter_get_max_top_value(ds3231));
	printk("\t%u channels\n", counter_get_num_of_channels(ds3231));
	printk("\t%u Hz\n", counter_get_frequency(ds3231));

	printk("Top counter value: %u (%08x)\n",
	       counter_get_top_value(ds3231),
	       counter_get_top_value(ds3231));

	time_t now = counter_read(ds3231);
	struct tm *tp = gmtime(&now);

	strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S %a %j", tp);
	printk("Now %u: %s\n", (u32_t)now, buf);

	int rc = rtc_ds3231_get_ctrlstat(ds3231);

	if (rc < 0) {
		printk("Get config failed: %d\n", rc);
		return;
	}
	printk("CtrlStat: %04x\n", rc);

	struct rtc_ds3231_alarms alarms;

	rc = rtc_ds3231_get_alarms(ds3231, &alarms);
	printk("Get alarms: %d\n", rc);
	if (rc >= 0) {
		printk("Alarm 1 flags %x at %u\n", alarms.flags[0], (u32_t)alarms.time[0]);
		printk("Alarm 2 flags %x at %u\n", alarms.flags[1], (u32_t)alarms.time[1]);
	}

	alarms.time[0] = now + 5;
	alarms.flags[0] = RTC_DS3231_ALARM_FLAGS_ENABLED;
	alarms.time[1] = now + 60;
	alarms.flags[1] = RTC_DS3231_ALARM_FLAGS_ENABLED | RTC_DS3231_ALARM_FLAGS_DOW;

	rc = rtc_ds3231_set_alarms(ds3231, &alarms);
	printk("Set alarms: %d\n", rc);

	rc = rtc_ds3231_get_alarms(ds3231, &alarms);
	printk("Get alarms: %d\n", rc);
	if (rc >= 0) {
		tp = gmtime(alarms.time + 0);
		strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S %a %j", tp);
		printk("Alarm 1 flags %x at %u ~ %s\n", alarms.flags[0],
		       (u32_t)alarms.time[0], buf);
		tp = gmtime(alarms.time + 1);
		strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S %a %j", tp);
		printk("Alarm 2 flags %x at %u ~ %s\n", alarms.flags[1],
		       (u32_t)alarms.time[1], buf);
	}

	unsigned int count = 5;

	do {
		static u16_t lcs;

		k_sleep(1000);

		time_t now = counter_read(ds3231);
		u16_t cs = rtc_ds3231_get_ctrlstat(ds3231);

		if (lcs != cs) {
			tp = gmtime(&now);
			strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S %a %j", tp);
			printk("Counter %u, %s, ctrl %04x\n", (u32_t)now, buf, cs);
			lcs = cs;
			--count;
		}
	} while (count);
}
