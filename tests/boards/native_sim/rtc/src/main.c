/*
 * Copyright (c) 2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "timer_model.h"
#include "native_rtc.h"

#include <stdio.h>

static char *us_time_to_str(char *dest, uint64_t time)
{
	if (time != NSI_NEVER) {
		unsigned int hour;
		unsigned int minute;
		unsigned int second;
		unsigned int us;

		hour   = (time / 3600U / 1000000U) % 24;
		minute = (time / 60U / 1000000U) % 60;
		second = (time / 1000000U) % 60;
		us     = time % 1000000;

		sprintf(dest, "%02u:%02u:%02u.%06u", hour, minute, second, us);
	} else {
		sprintf(dest, " NEVER/UNKNOWN ");

	}
	return dest;
}

#define WAIT_TIME 250 /* ms */
#define TOLERANCE 20 /* ms Tolerance in native_posix time after WAIT_TIME */
#define TICK_MS (1000ul / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

/**
 * @brief Test native_posix real time control
 */
ZTEST(native_realtime, test_realtime)
{
	extern uint64_t get_host_us_time(void);
	uint64_t time;
	char time_s[60];
	uint64_t end_time, start_time;
	int64_t diff, error;
	uint64_t start_rtc_time[3];
	double acc_ratio = 1;
	double time_ratios[5] = {0.25, 2, 2, 2, 2};
	/* This ratio adjustments lead to test speeds 0.25x, 0.5x, 1x, 2x & 4x*/

	time = native_rtc_gettime_us(RTC_CLOCK_REALTIME);

	us_time_to_str(time_s, time);
	printk("Booted @%s\n", time_s);

	/*
	 * We override the real time speed in case it was set from command
	 * line
	 */
	hwtimer_set_rt_ratio(1.0);

	/*Let's wait >=1 tick to ensure everything is settled*/
	k_msleep(TICK_MS);

	start_time = get_host_us_time();
	start_rtc_time[2] = native_rtc_gettime_us(RTC_CLOCK_PSEUDOHOSTREALTIME);
	start_rtc_time[0] = native_rtc_gettime_us(RTC_CLOCK_BOOT);
	start_rtc_time[1] = native_rtc_gettime_us(RTC_CLOCK_REALTIME);

	for (int i = 0; i < 5; i++) {
		native_rtc_adjust_clock(time_ratios[i]);
		acc_ratio *= time_ratios[i];

		/* k_sleep waits 1 tick more than asked */
		k_msleep(WAIT_TIME - TICK_MS);

		/*
		 * Check that during the sleep, the correct amount of real time
		 * passed
		 */
		end_time = get_host_us_time();
		diff = end_time - start_time;
		error = diff / 1000 - WAIT_TIME / acc_ratio;

		posix_print_trace("%i/5: Speed ratio %.2f. Took %.3fms. "
				"Should take %.3fms +- %ims\n",
				i+1,
				acc_ratio,
				diff / 1000.0,
				WAIT_TIME / acc_ratio,
				TOLERANCE);

		zassert_true(llabs(error) < TOLERANCE,
			     "Real time error over TOLERANCE");

		/*
		 * Check that the RTC clocks advanced WAIT_TIME
		 * independently of the real timeness ratio
		 */
		diff = native_rtc_gettime_us(RTC_CLOCK_PSEUDOHOSTREALTIME) -
			start_rtc_time[2];
		error = diff - WAIT_TIME * 1000;

		posix_print_trace("%i/5: PSEUDOHOSTREALTIME reports %.3fms "
				"(error %.3fms)\n",
				i+1,
				diff / 1000.0,
				error / 1000.0);

		error /= 1000;
		zassert_true(llabs(error) < TOLERANCE,
			     "PSEUDOHOSTREALTIME time error over TOLERANCE");

		diff = native_rtc_gettime_us(RTC_CLOCK_BOOT) -
			start_rtc_time[0];

		zassert_true(diff == WAIT_TIME * 1000,
				"Error in RTC_CLOCK_BOOT");

		diff = native_rtc_gettime_us(RTC_CLOCK_REALTIME) -
			start_rtc_time[1];

		zassert_true(diff == WAIT_TIME * 1000,
				"Error in RTC_CLOCK_REALTIME");

		start_time += WAIT_TIME * 1000 / acc_ratio;
		start_rtc_time[0] += WAIT_TIME * 1000;
		start_rtc_time[1] += WAIT_TIME * 1000;
		start_rtc_time[2] += WAIT_TIME * 1000;
	}

}

/**
 * @brief Test native_posix RTC offset functionality
 */
ZTEST(native_realtime,  test_rtc_offset)
{
	int offset = 567;
	uint64_t start_rtc_time[2];
	int64_t diff, error;

	start_rtc_time[0] = native_rtc_gettime_us(RTC_CLOCK_REALTIME);
	start_rtc_time[1] = native_rtc_gettime_us(RTC_CLOCK_PSEUDOHOSTREALTIME);
	native_rtc_offset(offset);
	diff = native_rtc_gettime_us(RTC_CLOCK_PSEUDOHOSTREALTIME)
		- start_rtc_time[1];

	error = diff - offset;
	zassert_true(llabs(error) < TOLERANCE,
		     "PSEUDOHOSTREALTIME offset error over TOLERANCE");

	diff = native_rtc_gettime_us(RTC_CLOCK_REALTIME) - start_rtc_time[0];

	zassert_true(diff == offset, "Offsetting RTC failed\n");
}

ZTEST_SUITE(native_realtime, NULL, NULL, NULL, NULL, NULL);
