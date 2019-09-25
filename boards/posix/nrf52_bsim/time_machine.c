/*
 * Copyright (c) 2017-2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "NRF_HW_model_top.h"
#include "NRF_HWLowL.h"
#include "bs_tracing.h"
#include "bs_types.h"
#include "bs_utils.h"

/* Note: All timers are relative to hw_time and NOT to 'now' */
extern bs_time_t timer_nrf_main_timer;

/* The events priorities are as in this list from top to bottom
 * Priority being which timer executes first if several trigger at the same
 * instant
 */
static enum {
	NRF_HW_MAIN_TIMER = 0,
	NUMBER_OF_TIMERS,
	NONE
} next_timer_index = NONE;

static bs_time_t *Timer_list[NUMBER_OF_TIMERS] = {
		&timer_nrf_main_timer,
};
static bs_time_t next_timer_time = TIME_NEVER;

/*
 * Current absolute time of this device, as the device knows it.
 * It is never reset:
 */
static bs_time_t now;
/* Current time the HW of this device things it is */
static bs_time_t hw_time;
/*
 * Offset between the current absolute time of the device and the HW time
 * That is, the absolute time when the HW_time got reset
 */
static bs_time_t hw_time_delta;

/* Last time we synchronized with the bsim PHY, in device abs time */
static bs_time_t last_bsim_phy_sync_time;

#define BSIM_DEFAULT_PHY_MAX_RESYNC_OFFSET 1000000
/* At least every second we will inform the simulator about our timing */
static bs_time_t max_resync_offset = BSIM_DEFAULT_PHY_MAX_RESYNC_OFFSET;

/**
 * Set the maximum amount of time the device will spend without talking
 * (synching) with the phy.
 * This does not change the functional behavior of the Zephyr code or of the
 * radio emulation, and it is only relevant if special test code running in the
 * device interacts behind the scenes with other devices test code.
 * Setting for example a value of 5ms will ensure that this device time
 * will never be more than 5ms away from the phy. Setting it in all devices
 * to 5ms would then ensure no device time is farther apart than 5ms from any
 * other.
 *
 * Note that setting low values has a performance penalty.
 */
void tm_set_phy_max_resync_offset(bs_time_t offset_in_us)
{
	max_resync_offset = offset_in_us;
}

/**
 * Return the absolute current time (no HW model except the RADIO
 * should look into this)
 */
bs_time_t tm_get_abs_time(void)
{
	return now;
}

/**
 * Return the current HW time
 */
bs_time_t tm_get_hw_time(void)
{
	return hw_time;
}

bs_time_t posix_get_hw_cycle(void)
{
	return tm_get_hw_time();
}

/**
 * Reset the HW time
 */
static void tm_reset_hw_time(void)
{
	hw_time = 0;
	hw_time_delta = now;
	if (now != 0) {
		bs_trace_error_line("Reset not supposed to happen after "
				    "initialization\n");
	}
}

/**
 * Update the current hw_time value given the absolute time
 */
INLINE void tm_update_HW_time(void)
{
	hw_time = now - hw_time_delta;
}

bs_time_t tm_get_hw_time_from_abs_time(bs_time_t abstime)
{
	if (abstime == TIME_NEVER) {
		return TIME_NEVER;
	}
	return abstime - hw_time_delta;
}

/*
 * Reset the HW time
 */
void tm_reset_hw_times(void)
{
	tm_reset_hw_time();
}

/**
 * Advance the internal time values of this device until time
 */
static void tm_sleep_until_abs_time(bs_time_t time)
{
	if (time >= now) {
		/*
		 * Ensure that at least we sync with the phy
		 * every max_resync_offset
		 */
		if (time > last_bsim_phy_sync_time + max_resync_offset) {
			hwll_sync_time_with_phy(time);
			last_bsim_phy_sync_time = time;
		}
		now = time;
	} else {
		/* LCOV_EXCL_START */
		bs_trace_warning_manual_time_line(now, "next_time_time "
			"corrupted (%"PRItime"<= %"PRItime", timer idx=%i)\n",
			time, now, next_timer_index);
		/* LCOV_EXCL_STOP */
	}
	tm_update_HW_time();
}

/**
 * Keep track of the last time we synchronized the time with the scheduler
 */
void tm_update_last_phy_sync_time(bs_time_t abs_time)
{
	last_bsim_phy_sync_time = abs_time;
}

/**
 * Advance the internal time values of this device
 * until the HW time reaches hw_time
 */
static void tm_sleep_until_hw_time(bs_time_t hw_time)
{
	bs_time_t next_time = TIME_NEVER;

	if (hw_time != TIME_NEVER) {
		next_time = hw_time + hw_time_delta;
	}
	tm_sleep_until_abs_time(next_time);
}

/**
 * Look into all timers and update next_timer accordingly
 * To be called each time a "timed process" updates its timer
 */
void tm_find_next_timer_to_trigger(void)
{
	next_timer_time = *Timer_list[0];
	next_timer_index = 0;

	for (uint i = 1; i < NUMBER_OF_TIMERS ; i++) {
		if (next_timer_time > *Timer_list[i]) {
			next_timer_time = *Timer_list[i];
			next_timer_index = i;
		}
	}
}

bs_time_t tm_get_next_timer_abstime(void)
{
	return next_timer_time + hw_time_delta;
}

bs_time_t tm_hw_time_to_abs_time(bs_time_t hwtime)
{
	if (hwtime == TIME_NEVER) {
		return TIME_NEVER;
	}
	return hwtime + hw_time_delta;
}

bs_time_t tm_abs_time_to_hw_time(bs_time_t abstime)
{
	if (abstime == TIME_NEVER) {
		return TIME_NEVER;
	}
	return abstime - hw_time_delta;
}

/**
 * Run ahead: Run the HW models and advance time as needed
 * Note that this function does not return
 */
void tm_run_forever(void)
{
	while (1) {
		tm_sleep_until_hw_time(next_timer_time);
		switch (next_timer_index) {
		case NRF_HW_MAIN_TIMER:
			nrf_hw_some_timer_reached();
			break;
		default:
			bs_trace_error_time_line("next_timer_index "
						 "corrupted\n");
			break;
		}
		tm_find_next_timer_to_trigger();
	}
}
