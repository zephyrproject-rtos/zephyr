/*
 * Copyright (c) 2021 LACROIX Group
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/rtc.h>
#include <stdio.h>
#include <zephyr.h>
#include <sys/printk.h>

#include <time.h>
#include <sys/timeutil.h>

const struct device *dev;

// Select Wake Up Timer test (Wut) or alarm test
#define TEST_WUT        1
#define TEST_ALARM      2
#define TEST_TYPE       TEST_ALARM

#if (TEST_TYPE == TEST_WUT)
// Perform some stuff when wut interrupt occurs
void wakeup_timer_callback(const struct device *dev)
{
	struct tm date_time;    // Date and time structure
	static int cnt;

	cnt++;
	printk("wakeup_timer_callback: %i \t", cnt);
	rtc_get_time(dev, &date_time);

	if (cnt == 3) {
		rtc_stop_wakeup_timer(dev, RTC_WUT0);
		cnt = 0;
	}
}
#endif  // #if(TEST_TYPE == TEST_WUT)

#if (TEST_TYPE == TEST_ALARM)
// Perform some stuff when alaram A interrupt occurs
void alarmA_callback(const struct device *dev)
{
	struct tm date_time;    // Date and time structure
	static int cnt;

	cnt++;
	printk("alarmA_callback: %i \t", cnt);
	rtc_get_time(dev, &date_time);
}

// Perform some stuff when alaram B interrupt occurs
void alarmB_callback(const struct device *dev)
{
	struct tm date_time;    // Date and time structure
	static int cnt;

	cnt++;
	printk("alarmB_callback: %i \t", cnt);
	rtc_get_time(dev, &date_time);
}
#endif  // #if(TEST_TYPE == TEST_ALARM)

void main(void)
{
	int i_cnt = 0;
	int i_cnt_alarm = 0;
	int64_t time_epoch = 0; /* POSIX epoch time scale */

	#if (TEST_TYPE == TEST_WUT)
	struct rtc_wakeup wut;          /* Wake up timer structure */
	bool toggle_set = false;
	#endif  /* #if(TEST_TYPE == TEST_WUT) */
	#if (TEST_TYPE == TEST_ALARM)
	struct rtc_alarm alarmA = { 0 };
	struct rtc_alarm alarmB = { 0 };
	#endif  /* #if(TEST_TYPE == TEST_ALARM) */
	struct tm date_time;    /* Date and time structure */

	printk("RTC driver! %s\n", CONFIG_BOARD);

	/* Init time */
	date_time.tm_year = 2021 - 1900;        /* Year since 1900 */
	date_time.tm_mon = 9;                   /* month of year [0,11], where 0 = jan */
	date_time.tm_mday = 21;                 /* Day of month [1,31] */
	date_time.tm_hour = 14;                 /* Hour [0,23] */
	date_time.tm_min = 30;                  /* Minutes [0,59] */
	date_time.tm_sec = 0;                   /* Seconds [0,61] */
	date_time.tm_isdst = -1;                /* Daylight savings flag: Is DST on? 1 = yes, 0 = no, -1 = unknown */
	date_time.tm_wday = 2;                  /* Day of week [0,6] (Sunday = 0) */
	// date_time.tm_yday = ; 			/* Day of year [0,365] */
	// t_of_day = mktime(&t);

	#if (TEST_TYPE == TEST_WUT)
	/* Init rtc wake up timer */
	wut.period = 10000U;            /* Wut period in milliseconds */
	wut.callback = wakeup_timer_callback;
	#endif  /* #if(TEST_TYPE == TEST_WUT) */

	#if (TEST_TYPE == TEST_ALARM)
	/* AlarmA is periodic and is generated every  minute from 14h30min30s thanks to mask = RTC_ALARM_MASK_MIN */
	alarmA.alarm_time.tm_year = 2021 - 1900;        /* Year since 1900 */
	alarmA.alarm_time.tm_mon = 9;                   /* month of year [0,11], where 0 = jan */
	alarmA.alarm_time.tm_mday = 21;                 /* Day of month [1,31] */
	alarmA.alarm_time.tm_hour = 14;                 /* Hour [0,23] */
	alarmA.alarm_time.tm_min = 30;                  /* Minutes [0,59] */
	alarmA.alarm_time.tm_sec = 30;                  /* Seconds [0,61] */
	alarmA.alarm_time.tm_isdst = -1;                /* Daylight savings flag: Is DST on? 1 = yes, 0 = no, -1 = unknown */
	alarmA.alarm_time.tm_wday = 2;                  /* Day of week [0,6] (Sunday = 0) */
	// alarmA.alarm_time.tm_yday = ; 			/* Day of year [0,365] */
	alarmA.alarm_date_weekday_sel = RTC_ALARM_WEEKDAY_SEL;
	alarmA.alarm_mask = RTC_ALARM_MASK_MIN;         // RTC_ALARM_MASK_SEC;	// RTC_ALARM_MASK_NONE;
	alarmA.callback = alarmA_callback;

	/* AlarmB is oneshoot on 14h30min10s thanks to mask = RTC_ALARM_MASK_NONE */
	alarmB.alarm_time.tm_year = 2021 - 1900;        /* Year since 1900 */
	alarmB.alarm_time.tm_mon = 9;                   /* month of year [0,11], where 0 = jan */
	alarmB.alarm_time.tm_mday = 21;                 /* Day of month [1,31] */
	alarmB.alarm_time.tm_hour = 14;                 /* Hour [0,23] */
	alarmB.alarm_time.tm_min = 30;                  /* Minutes [0,59] */
	alarmB.alarm_time.tm_sec = 10;                  /* Seconds [0,61] */
	alarmA.alarm_time.tm_isdst = -1;                /* Daylight savings flag: Is DST on? 1 = yes, 0 = no, -1 = unknown */
	alarmB.alarm_time.tm_wday = 2;                  /* Day of week [0,6] (Sunday = 0) */
	// alarmB.alarm_time.tm_yday = ; 			/* Day of year [0,365] */
	alarmB.alarm_date_weekday_sel = RTC_ALARM_WEEKDAY_SEL;
	alarmB.alarm_mask = RTC_ALARM_MASK_NONE;                // RTC_ALARM_MASK_SEC;	// RTC_ALARM_MASK_NONE;
	alarmB.callback = alarmB_callback;
	#endif  /* #if(TEST_TYPE == TEST_ALARM) */

	// dev = device_get_binding("CUSTOM_DRIVER");
	dev = device_get_binding("RTC_0");

	__ASSERT(dev, "Failed to get device binding");

	printk("device is %p, name is %s\n", dev, dev->name);

	rtc_set_time(dev, date_time);
	rtc_get_time(dev, &date_time);

	#if (TEST_TYPE == TEST_WUT)
	rtc_set_wakeup_timer(dev, wut, RTC_WUT0);
	rtc_start_wakeup_timer(dev, RTC_WUT0);
	#endif  // #if(TEST_TYPE == TEST_WUT)

	#if (TEST_TYPE == TEST_ALARM)
	rtc_set_alarm(dev, alarmA, RTC_ALARMA);
	rtc_set_alarm(dev, alarmB, RTC_ALARMB);
	rtc_start_alarm(dev, RTC_ALARMA);
	rtc_start_alarm(dev, RTC_ALARMB);
	#endif  // #if(TEST_TYPE == TEST_ALARM)

	// rtc_set_alarm(dev);
	// rtc_get_alarm(dev);
	// rtc_cancel_alarm(dev);
	// rtc_set_wakeup_timer(dev, 2048);

	while (1) {
		rtc_get_time(dev, &date_time);

		time_epoch = timeutil_timegm64(&date_time);
		printk("epoch:%ld wd:%i %i/%i/%i %i:%i:%i\n", (long) time_epoch, date_time.tm_wday,
		       (1900 + date_time.tm_year), date_time.tm_mon, date_time.tm_mday,
		       date_time.tm_hour, date_time.tm_min, date_time.tm_sec);


		k_msleep(2000);
		// k_msleep(25000);
		// printk("WakeUp counter: %d\n", i_cnt);
		// rtc_set_wakeup_timer(dev, 2048);
		i_cnt++;
		if (i_cnt == 25) {
			#if (TEST_TYPE == TEST_WUT)
			if (toggle_set == false) {
				wut.period = 10000U;            // Wut period in milliseconds
				printk("rtc_set_wakeup_timer 10s\n");
				rtc_set_wakeup_timer(dev, wut, RTC_WUT0);
				toggle_set = true;
			} else {
				wut.period = 5000U;             // Wut period in milliseconds
				printk("rtc_set_wakeup_timer 5s\n");
				rtc_set_wakeup_timer(dev, wut, RTC_WUT0);
				toggle_set = false;
			}

			rtc_start_wakeup_timer(dev, RTC_WUT0);
			#endif  // #if(TEST_TYPE == TEST_WUT)
			i_cnt = 0;
		}

		i_cnt_alarm++;
		if (i_cnt_alarm == 80) {
			#if (TEST_TYPE == TEST_ALARM)
			rtc_stop_alarm(dev, RTC_ALARMA);
			printk("rtc_stop_alarm(dev, RTC_ALARMA)\n");
			rtc_stop_alarm(dev, RTC_ALARMB);
			printk("rtc_stop_alarm(dev, RTC_ALARMB)\n");
			#endif  // #if(TEST_TYPE == TEST_ALARM)
			i_cnt_alarm = 0;
		}
	}
}
