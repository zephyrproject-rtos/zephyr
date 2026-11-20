/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Rate control for the zperf senders */

#include <limits.h>
#include <zephyr/kernel.h>

#include "zperf_pacing.h"

void zperf_pacer_init(struct zperf_pacer *pacer, uint32_t packet_duration_us, int64_t start_time)
{
	uint64_t packet_ticks;

	/* Keep the fractional tick instead of rounding every packet up. */
	pacer->period = (uint64_t)packet_duration_us * CONFIG_SYS_CLOCK_TICKS_PER_SEC;

	/* Allow one packet interval of debt, or one tick at coarse tick rates.
	 * Compute ceil(period / USEC_PER_SEC) without adding to period.
	 */
	packet_ticks = pacer->period / USEC_PER_SEC +
		       (pacer->period % USEC_PER_SEC != 0U);
	pacer->catch_up_ticks = (int64_t)MAX(1ULL, packet_ticks);
	pacer->fraction = 0U;
	pacer->next_tick = start_time;
	pacer->deadline = start_time;
}

void zperf_pacer_next(struct zperf_pacer *pacer, int64_t loop_time)
{
	uint64_t interval;

	if (pacer->period == 0U) {
		pacer->deadline = loop_time;
		return;
	}

	/* Bound catch-up by the packet interval rather than a fixed tick.
	 * Discard excess debt after a stall while preserving fractional phase.
	 */
	if (loop_time - pacer->next_tick > pacer->catch_up_ticks) {
		pacer->next_tick = loop_time - pacer->catch_up_ticks;
	}

	/* Advance once per packet, including packets sent in the same tick.
	 * The quotient and remainder represent the cumulative ideal deadline.
	 */
	interval = pacer->period + pacer->fraction;
	pacer->next_tick += interval / USEC_PER_SEC;
	pacer->fraction = interval % USEC_PER_SEC;
	pacer->deadline = pacer->next_tick + (pacer->fraction != 0U);

}

void zperf_pacer_wait(const struct zperf_pacer *pacer, int64_t end_time)
{
#if defined(CONFIG_ARCH_POSIX)
	ARG_UNUSED(pacer);
	ARG_UNUSED(end_time);

	/* Preserve the native simulator's existing time-advancement behavior. */
	k_busy_wait(USEC_PER_MSEC);
#else
	int64_t deadline;
	int64_t remaining;

	if (pacer->period == 0U) {
		return;
	}

	deadline = MIN(pacer->deadline, end_time);

	/* Use an absolute deadline: relative sleeps round up from the time of
	 * the call and would add a tick even when sending took only part of one.
	 * Recheck after sending, which may itself have blocked past the deadline.
	 */
	remaining = deadline - k_uptime_ticks();

	if (remaining > 0) {
#ifdef CONFIG_TIMEOUT_64BIT
		k_sleep(K_TIMEOUT_ABS_TICKS(deadline));
#else
		/* Absolute timeouts require TIMEOUT_64BIT. With 32-bit timeouts,
		 * allow up to one tick of early sending instead of introducing
		 * a systematic extra tick with every relative sleep. Fractions
		 * remain in the cumulative schedule, so this does not add drift.
		 */
		if (remaining > 1) {
			k_sleep(K_TICKS(MIN(remaining - 1, INT_MAX)));
		}
#endif
	}
#endif
}
