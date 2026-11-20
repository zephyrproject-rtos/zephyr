/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/* Rate control for the zperf senders */

#ifndef ZPERF_PACING_H_
#define ZPERF_PACING_H_

#include <zephyr/kernel.h>

struct zperf_pacer {
	/* Packet duration in ticks multiplied by USEC_PER_SEC. */
	uint64_t period;
	/* Maximum catch-up debt: one packet interval, rounded up, at least one tick. */
	int64_t catch_up_ticks;
	/* Fractional tick numerator, with denominator USEC_PER_SEC. */
	uint32_t fraction;
	/* Integer part of the cumulative ideal deadline. */
	int64_t next_tick;
	/* Ideal deadline rounded up once, after accumulating fractions. */
	int64_t deadline;
};

/**
 * Prepare to send packets of the given duration, starting at @p start_time
 * (in ticks). A zero duration disables rate limiting.
 */
void zperf_pacer_init(struct zperf_pacer *pacer, uint32_t packet_duration_us,
			  int64_t start_time);

/**
 * Account for a packet about to be sent at @p loop_time (in ticks).
 * Update the cumulative deadline used by zperf_pacer_wait().
 */
void zperf_pacer_next(struct zperf_pacer *pacer, int64_t loop_time);

/** Wait until the next packet deadline, but not past @p end_time (absolute ticks). */
void zperf_pacer_wait(const struct zperf_pacer *pacer, int64_t end_time);

#endif /* ZPERF_PACING_H_ */
