/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * This provides a model of:
 *  - A system tick
 *  - A real time clock
 *  - A one shot HW timer which can be used to awake the CPU at a given time
 *  - The clock source for all of this, and therefore for native_posix
 *
 * Please see doc/board.rst for more information, specially sections:
 *  About time in native_posix
 *  Peripherals:
 *	Clock source, system tick and timer
 *	Real time clock
 */

#include <stdint.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>
#include "hw_models_top.h"
#include "irq_ctrl.h"
#include "board_soc.h"
#include <zephyr/types.h>
#include <zephyr/arch/posix/posix_trace.h>
#include <zephyr/sys/util.h>
#include "cmdline.h"
#include "posix_native_task.h"

#define DEBUG_NP_TIMER 0

#if DEBUG_NP_TIMER

/**
 * Helper function to convert a 64 bit time in microseconds into a string.
 * The format will always be: hh:mm:ss.ssssss\0
 *
 * Note: the caller has to allocate the destination buffer (at least 17 chars)
 */
#include <stdio.h>
static char *us_time_to_str(char *dest, uint64_t time)
{
	if (time != NEVER) {
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
#endif

uint64_t hw_timer_timer;

uint64_t hw_timer_tick_timer;
uint64_t hw_timer_awake_timer;

static uint64_t tick_p; /* Period of the ticker */
static int64_t silent_ticks;

static bool real_time_mode =
#if defined(CONFIG_NATIVE_POSIX_SLOWDOWN_TO_REAL_TIME)
	true;
#else
	false;
#endif

static bool reset_rtc; /*"Reset" the RTC on boot*/

/*
 * When this executable started running, this value shall not be changed after
 * boot
 */
static uint64_t boot_time;

/*
 * Ratio of the simulated clock to the real host time
 * For ex. a clock_ratio = 1+100e-6 means the simulated time is 100ppm faster
 * than real time
 */
static double clock_ratio = 1.0;

#if DEBUG_NP_TIMER
/*
 * Offset of the simulated time vs the real host time due to drift/clock ratio
 * until "last_radj_*time"
 *
 * A positive value means simulated time is ahead of the host time
 *
 * This variable is only kept for debugging purposes
 */
static int64_t last_drift_offset;
#endif

/*
 * Offsets of the RTC relative to the hardware models simu_time
 * "simu_time" == simulated time which starts at 0 on boot
 */
static int64_t rtc_offset;

/* Last host/real time when the ratio was adjusted */
static uint64_t last_radj_rtime;
/* Last simulated time when the ratio was adjusted */
static uint64_t last_radj_stime;

extern uint64_t posix_get_hw_cycle(void);

void hwtimer_set_real_time_mode(bool new_rt)
{
	real_time_mode = new_rt;
}

static void hwtimer_update_timer(void)
{
	hw_timer_timer = MIN(hw_timer_tick_timer, hw_timer_awake_timer);
}

static inline void host_clock_gettime(struct timespec *tv)
{
#if defined(CLOCK_MONOTONIC_RAW)
	clock_gettime(CLOCK_MONOTONIC_RAW, tv);
#else
	clock_gettime(CLOCK_MONOTONIC, tv);
#endif
}

uint64_t get_host_us_time(void)
{
	struct timespec tv;

	host_clock_gettime(&tv);
	return (uint64_t)tv.tv_sec * 1e6 + tv.tv_nsec / 1000;
}

void hwtimer_init(void)
{
	silent_ticks = 0;
	hw_timer_tick_timer = NEVER;
	hw_timer_awake_timer = NEVER;
	hwtimer_update_timer();
	if (real_time_mode) {
		boot_time = get_host_us_time();
		last_radj_rtime = boot_time;
		last_radj_stime = 0U;
	}
	if (!reset_rtc) {
		struct timespec tv;
		uint64_t realhosttime;

		clock_gettime(CLOCK_REALTIME, &tv);
		realhosttime = (uint64_t)tv.tv_sec * 1e6 + tv.tv_nsec / 1000;

		rtc_offset += realhosttime;
	}
}

void hwtimer_cleanup(void)
{

}

/**
 * Enable the HW timer tick interrupts with a period <period> in microseconds
 */
void hwtimer_enable(uint64_t period)
{
	tick_p = period;
	hw_timer_tick_timer = hwm_get_time() + tick_p;
	hwtimer_update_timer();
	hwm_find_next_timer();
}

static void hwtimer_tick_timer_reached(void)
{
	if (real_time_mode) {
		uint64_t expected_rt = (hw_timer_tick_timer - last_radj_stime)
				    / clock_ratio
				    + last_radj_rtime;
		uint64_t real_time = get_host_us_time();

		int64_t diff = expected_rt - real_time;

#if DEBUG_NP_TIMER
		char es[30];
		char rs[30];

		us_time_to_str(es, expected_rt - boot_time);
		us_time_to_str(rs, real_time - boot_time);
		printf("tick @%5llims: diff = expected_rt - real_time = "
			"%5lli = %s - %s\n",
			hw_timer_tick_timer/1000U, diff, es, rs);
#endif

		if (diff > 0) { /* we need to slow down */
			struct timespec requested_time;
			struct timespec remaining;

			requested_time.tv_sec  = diff / 1e6;
			requested_time.tv_nsec = (diff -
						 requested_time.tv_sec*1e6)*1e3;

			(void) nanosleep(&requested_time, &remaining);
		}
	}

	hw_timer_tick_timer += tick_p;
	hwtimer_update_timer();

	if (silent_ticks > 0) {
		silent_ticks -= 1;
	} else {
		hw_irq_ctrl_set_irq(TIMER_TICK_IRQ);
	}
}

static void hwtimer_awake_timer_reached(void)
{
	hw_timer_awake_timer = NEVER;
	hwtimer_update_timer();
	hw_irq_ctrl_set_irq(PHONY_HARD_IRQ);
}

void hwtimer_timer_reached(void)
{
	uint64_t Now = hw_timer_timer;

	if (hw_timer_awake_timer == Now) {
		hwtimer_awake_timer_reached();
	}

	if (hw_timer_tick_timer == Now) {
		hwtimer_tick_timer_reached();
	}
}

/**
 * The timer HW will awake the CPU (without an interrupt) at least when <time>
 * comes (it may awake it earlier)
 *
 * If there was a previous request for an earlier time, the old one will prevail
 *
 * This is meant for k_busy_wait() like functionality
 */
void hwtimer_wake_in_time(uint64_t time)
{
	if (hw_timer_awake_timer > time) {
		hw_timer_awake_timer = time;
		hwtimer_update_timer();
		hwm_find_next_timer();
	}
}

/**
 * The kernel wants to skip the next sys_ticks tick interrupts
 * If sys_ticks == 0, the next interrupt will be raised.
 */
void hwtimer_set_silent_ticks(int64_t sys_ticks)
{
	silent_ticks = sys_ticks;
}

int64_t hwtimer_get_pending_silent_ticks(void)
{
	return silent_ticks;
}


/**
 * During boot set the real time clock simulated time not start
 * from the real host time
 */
void hwtimer_reset_rtc(void)
{
	reset_rtc = true;
}

/**
 * Set a time offset (microseconds) of the RTC simulated time
 * Note: This should not be used after starting
 */
void hwtimer_set_rtc_offset(int64_t offset)
{
	rtc_offset = offset;
}

/**
 * Set the ratio of the simulated time to host (real) time.
 * Note: This should not be used after starting
 */
void hwtimer_set_rt_ratio(double ratio)
{
	clock_ratio = ratio;
}

/**
 * Increase or decrease the RTC simulated time by offset_delta
 */
void hwtimer_adjust_rtc_offset(int64_t offset_delta)
{
	rtc_offset += offset_delta;
}

/**
 * Adjust the ratio of the simulated time by a factor
 */
void hwtimer_adjust_rt_ratio(double ratio_correction)
{
	uint64_t current_stime = hwm_get_time();
	int64_t s_diff = current_stime - last_radj_stime;
	/* Accumulated real time drift time since last adjustment: */

	last_radj_rtime += s_diff / clock_ratio;
	last_radj_stime = current_stime;

#if DEBUG_NP_TIMER
	char ct[30];
	int64_t r_drift = (long double)(clock_ratio-1.0)/(clock_ratio)*s_diff;

	last_drift_offset += r_drift;
	us_time_to_str(ct, current_stime);

	printf("%s(): @%s, s_diff= %llius after last adjust\n"
		" during which we drifted %.3fms\n"
		" total acc drift (last_drift_offset) = %.3fms\n"
		" last_radj_rtime = %.3fms (+%.3fms )\n"
		" Ratio adjusted to %f\n",
		__func__, ct, s_diff,
		r_drift/1000.0,
		last_drift_offset/1000.0,
		last_radj_rtime/1000.0,
		s_diff/clock_ratio/1000.0,
		clock_ratio*ratio_correction);
#endif

	clock_ratio *= ratio_correction;
}

/**
 * Return the current simulated RTC time in microseconds
 */
int64_t hwtimer_get_simu_rtc_time(void)
{
	return hwm_get_time() + rtc_offset;
}


/**
 * Return a version of the host time which would have drifted as if the host
 * real time clock had been running from the native_posix clock, and adjusted
 * both in rate and in offsets as the native_posix has been.
 *
 * Note that this time may be significantly ahead of the simulated time
 * (the time the Zephyr kernel thinks it is).
 * This will be the case in general if native_posix is not able to run at or
 * faster than real time.
 */
void hwtimer_get_pseudohost_rtc_time(uint32_t *nsec, uint64_t *sec)
{
	/*
	 * Note: long double has a 64bits mantissa in x86.
	 * Therefore to avoid loss of precision after 500 odd years into
	 * the epoch, we first calculate the offset from the last adjustment
	 * time split in us and ns. So we keep the full precision for 500 odd
	 * years after the last clock ratio adjustment (or native_posix boot,
	 * whichever is latest).
	 * Meaning, we will still start to loose precision after 500 off
	 * years of runtime without a clock ratio adjustment, but that really
	 * should not be much of a problem, given that the ns lower digits are
	 * pretty much noise anyhow.
	 * (So, all this is a huge overkill)
	 *
	 * The operation below in plain is just:
	 *   st = (rt - last_rt_adj_time)*ratio + last_dt_adj_time
	 * where st = simulated time
	 *       rt = real time
	 *       last_rt_adj_time = time (real) when the last ratio
	 *			    adjustment took place
	 *       last_st_adj_time = time (simulated) when the last ratio
	 *			    adjustment took place
	 *       ratio = ratio between simulated time and real time
	 */
	struct timespec tv;

	host_clock_gettime(&tv);

	uint64_t rt_us = (uint64_t)tv.tv_sec * 1000000ULL + tv.tv_nsec / 1000;
	uint32_t rt_ns = tv.tv_nsec % 1000;

	long double drt_us = (long double)rt_us - last_radj_rtime;
	long double drt_ns = drt_us * 1000.0L + (long double)rt_ns;
	long double st = drt_ns * (long double)clock_ratio +
			 (long double)(last_radj_stime + rtc_offset) * 1000.0L;

	*nsec = fmodl(st, 1e9L);
	*sec = st / 1e9L;
}

static struct {
	double stop_at;
	double rtc_offset;
	double rt_drift;
	double rt_ratio;
} args;

static void cmd_stop_at_found(char *argv, int offset)
{
	ARG_UNUSED(offset);
	if (args.stop_at < 0) {
		posix_print_error_and_exit("Error: stop-at must be positive "
					   "(%s)\n", argv);
	}
	hwm_set_end_of_time(args.stop_at*1e6);
}

static void cmd_realtime_found(char *argv, int offset)
{
	ARG_UNUSED(argv);
	ARG_UNUSED(offset);
	hwtimer_set_real_time_mode(true);
}

static void cmd_no_realtime_found(char *argv, int offset)
{
	ARG_UNUSED(argv);
	ARG_UNUSED(offset);
	hwtimer_set_real_time_mode(false);
}

static void cmd_rtcoffset_found(char *argv, int offset)
{
	ARG_UNUSED(argv);
	ARG_UNUSED(offset);
	hwtimer_set_rtc_offset(args.rtc_offset*1e6);
}

static void cmd_rt_drift_found(char *argv, int offset)
{
	ARG_UNUSED(argv);
	ARG_UNUSED(offset);
	if (!(args.rt_drift > -1)) {
		posix_print_error_and_exit("The drift needs to be > -1. "
					  "Please use --help for more info\n");
	}
	args.rt_ratio = args.rt_drift + 1;
	hwtimer_set_rt_ratio(args.rt_ratio);
}

static void cmd_rt_ratio_found(char *argv, int offset)
{
	ARG_UNUSED(argv);
	ARG_UNUSED(offset);
	if ((args.rt_ratio <= 0)) {
		posix_print_error_and_exit("The ratio needs to be > 0. "
					  "Please use --help for more info\n");
	}
	hwtimer_set_rt_ratio(args.rt_ratio);
}

static void cmd_rtcreset_found(char *argv, int offset)
{
	ARG_UNUSED(argv);
	ARG_UNUSED(offset);
	hwtimer_reset_rtc();
}

static void native_add_time_options(void)
{
	static struct args_struct_t timer_options[] = {
		/*
		 * Fields:
		 * manual, mandatory, switch,
		 * option_name, var_name ,type,
		 * destination, callback,
		 * description
		 */
		{false, false, true,
		"rt", "", 'b',
		NULL, cmd_realtime_found,
		"Slow down the execution to the host real time, "
		"or a ratio of it (see --rt-ratio below)"},

		{false, false, true,
		"no-rt", "", 'b',
		NULL, cmd_no_realtime_found,
		"Do NOT slow down the execution to real time, but advance "
		"Zephyr's time as fast as possible and decoupled from the host "
		"time"},

		{false, false, false,
		"rt-drift", "dratio", 'd',
		(void *)&args.rt_drift, cmd_rt_drift_found,
		"Drift of the simulated clock relative to the host real time. "
		"Normally this would be set to a value of a few ppm (e.g. 50e-6"
		") "
		"This option has no effect in non real time mode"
		},

		{false, false, false,
		"rt-ratio", "ratio", 'd',
		(void *)&args.rt_ratio, cmd_rt_ratio_found,
		"Relative speed of the simulated time vs real time. "
		"For ex. set to 2 to have simulated time pass at double the "
		"speed of real time. "
		"Note that both rt-drift & rt-ratio adjust the same clock "
		"speed, and therefore it does not make sense to use them "
		"simultaneously. "
		"This option has no effect in non real time mode"
		},

		{false, false, false,
		"rtc-offset", "time_offset", 'd',
		(void *)&args.rtc_offset, cmd_rtcoffset_found,
		"At boot offset the RTC clock by this amount of seconds"
		},

		{false, false, true,
		"rtc-reset", "", 'b',
		NULL, cmd_rtcreset_found,
		"Start the simulated real time clock at 0. Otherwise it starts "
		"matching the value provided by the host real time clock"},

		{false, false, false,
		 "stop_at", "time", 'd',
		(void *)&args.stop_at, cmd_stop_at_found,
		"In simulated seconds, when to stop automatically"},

		ARG_TABLE_ENDMARKER};

	native_add_command_line_opts(timer_options);
}

NATIVE_TASK(native_add_time_options, PRE_BOOT_1, 1);
