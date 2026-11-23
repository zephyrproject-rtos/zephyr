/*
 * Copyright (c) 2026 Embeint Inc
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/lorawan/lorawan.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/__assert.h>

#define JOIN_BACKOFF_1_PERCENT_FACTOR   100U
#define JOIN_BACKOFF_01_PERCENT_FACTOR  1000U
#define JOIN_BACKOFF_001_PERCENT_FACTOR 10000U

#define JOIN_BACKOFF_FIRST_PERIOD_MS  (1U * SEC_PER_HOUR * MSEC_PER_SEC)
#define JOIN_BACKOFF_SECOND_PERIOD_MS (10U * SEC_PER_HOUR * MSEC_PER_SEC)
#define JOIN_BACKOFF_LATER_PERIOD_MS  (24U * SEC_PER_HOUR * MSEC_PER_SEC)

#define JOIN_BACKOFF_FIRST_PERIOD_END (JOIN_BACKOFF_FIRST_PERIOD_MS)
#define JOIN_BACKOFF_SECOND_PERIOD_END                                                             \
	(JOIN_BACKOFF_FIRST_PERIOD_MS + JOIN_BACKOFF_SECOND_PERIOD_MS)

static uint32_t calculate_backoff(uint16_t join_airtime, uint32_t duty_cycle_factor)
{
	uint32_t backoff = join_airtime * (duty_cycle_factor - 1U);

#ifdef CONFIG_LORAWAN_JOIN_BACKOFF_JITTER_PERCENT
	uint8_t jitter_percent;
	uint32_t jitter;

	/* Desynchronise join requests across the network by applying jitter to each backoff */
	jitter_percent = sys_rand8_get() % (CONFIG_LORAWAN_JOIN_BACKOFF_JITTER_PERCENT + 1);
	jitter = (jitter_percent * backoff) / 100;
	backoff += jitter;
#endif /* CONFIG_LORAWAN_JOIN_BACKOFF_JITTER_PERCENT */

	return backoff;
}

/* LoRaWAN L2 1.0.4 Specification Section 7
 *
 * Hour  0-1: <36 seconds per 1 hour (1% duty cycle)
 * Hour 1-11: <36 seconds per 10 hours (0.1% duty cycle)
 * Hour 11-N: <8.7 seconds per 24 hours (0.01% duty cycle)
 */
uint32_t lorawan_join_backoff(uint16_t join_airtime)
{
	int64_t uptime_ms = k_uptime_get();
	uint32_t duty_cycle_factor;
	uint32_t next_duty_cycle_factor;
	uint32_t window_remaining_ms;
	uint32_t backoff;

	/* Longest known join packet is ~1.6 seconds */
	__ASSERT_NO_MSG(join_airtime < 2000);

	if (uptime_ms < JOIN_BACKOFF_FIRST_PERIOD_END) {
		duty_cycle_factor = JOIN_BACKOFF_1_PERCENT_FACTOR;
		next_duty_cycle_factor = JOIN_BACKOFF_01_PERCENT_FACTOR;
		window_remaining_ms = JOIN_BACKOFF_FIRST_PERIOD_MS - uptime_ms;
	} else if (uptime_ms < JOIN_BACKOFF_SECOND_PERIOD_END) {
		duty_cycle_factor = JOIN_BACKOFF_01_PERCENT_FACTOR;
		next_duty_cycle_factor = JOIN_BACKOFF_001_PERCENT_FACTOR;
		window_remaining_ms = JOIN_BACKOFF_SECOND_PERIOD_END - uptime_ms;
	} else {
		int64_t elapsed = uptime_ms - JOIN_BACKOFF_SECOND_PERIOD_END;

		duty_cycle_factor = JOIN_BACKOFF_001_PERCENT_FACTOR;
		next_duty_cycle_factor = JOIN_BACKOFF_001_PERCENT_FACTOR;
		window_remaining_ms =
			JOIN_BACKOFF_LATER_PERIOD_MS - (elapsed % JOIN_BACKOFF_LATER_PERIOD_MS);
	}

	backoff = calculate_backoff(join_airtime, duty_cycle_factor);

	/* Do not carry a transmission scheduled under one duty-cycle limit into the next
	 * aggregation period. Start a fresh backoff at the boundary instead.
	 */
	if (backoff >= window_remaining_ms) {
		backoff = window_remaining_ms +
			  calculate_backoff(join_airtime, next_duty_cycle_factor);
	}

	return backoff;
}
