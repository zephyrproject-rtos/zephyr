/*
 * Copyright (c) 2018 Oticon A/S
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include "nsi_tracing.h"
#include "native_rtc.h"
#include "nsi_hw_scheduler.h"
#include "nsi_timer_model.h"

/**
 * Return the (simulation) time in microseconds
 * where clock_type is one of RTC_CLOCK_*
 */
uint64_t native_rtc_gettime_us(int clock_type)
{
	if (clock_type == RTC_CLOCK_BOOT) {
		return nsi_hws_get_time();
	} else if (clock_type == RTC_CLOCK_REALTIME) { /* RTC_CLOCK_REALTIME */
		return hwtimer_get_simu_rtc_time();
	} else if (clock_type == RTC_CLOCK_PSEUDOHOSTREALTIME) {
		uint32_t nsec;
		uint64_t sec;

		hwtimer_get_pseudohost_rtc_time(&nsec, &sec);
		return sec * 1000000UL + nsec / 1000U;
	}

	nsi_print_error_and_exit("Unknown clock source %i\n",
				   clock_type);
	return 0;
}

/**
 * Similar to POSIX clock_gettime()
 * get the simulation time split in nsec and seconds
 * where clock_type is one of RTC_CLOCK_*
 */
void native_rtc_gettime(int clock_type, uint32_t *nsec, uint64_t *sec)
{
	if (clock_type == RTC_CLOCK_BOOT || clock_type == RTC_CLOCK_REALTIME) {
		uint64_t us = native_rtc_gettime_us(clock_type);
		*nsec = (us % 1000000UL) * 1000U;
		*sec  = us / 1000000UL;
	} else { /* RTC_CLOCK_PSEUDOHOSTREALTIME */
		hwtimer_get_pseudohost_rtc_time(nsec, sec);
	}
}

/**
 * Offset the real time clock by a number of microseconds.
 * Note that this only affects the RTC_CLOCK_REALTIME and
 * RTC_CLOCK_PSEUDOHOSTREALTIME clocks.
 */
void native_rtc_offset(int64_t delta_us)
{
	hwtimer_adjust_rtc_offset(delta_us);
}

/**
 * Adjust the speed of the clock source by a multiplicative factor
 */
void native_rtc_adjust_clock(double clock_correction)
{
	hwtimer_adjust_rt_ratio(clock_correction);
}
