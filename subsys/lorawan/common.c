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
#define JOIN_ACCEPT_DELAY_2_MS           (6U * MSEC_PER_SEC)

/* LoRaWAN L2 1.0.4 Specification Section 7
 *
 * Hour  0-1: <36 seconds per 1 hour (1% duty cycle)
 * Hour 1-11: <36 seconds per 10 hours (0.1% duty cycle)
 * Hour 11-N: <8.7 seconds per 24 hours (0.01% duty cycle)
 */
uint32_t lorawan_join_backoff(uint16_t join_airtime, uint32_t cumulative_join_airtime)
{
	uint32_t backoff;

	/* Longest known join packet is ~1.6 seconds */
	__ASSERT_NO_MSG(join_airtime < 2000);

	/* Base duty cycle backoff */
	if (cumulative_join_airtime < (36 * MSEC_PER_SEC)) {
		backoff = join_airtime * (JOIN_BACKOFF_1_PERCENT_FACTOR - 1);
	} else if (cumulative_join_airtime < (72 * MSEC_PER_SEC)) {
		backoff = join_airtime * (JOIN_BACKOFF_01_PERCENT_FACTOR - 1);
	} else {
		backoff = join_airtime * (JOIN_BACKOFF_001_PERCENT_FACTOR - 1);
	}

#ifdef CONFIG_LORAWAN_JOIN_BACKOFF_JITTER_PERCENT
	uint8_t jitter_percent;
	uint32_t jitter;

	/* Desynchronise join requests across the network by applying jitter to each backoff */
	jitter_percent = sys_rand8_get() % (CONFIG_LORAWAN_JOIN_BACKOFF_JITTER_PERCENT + 1);
	jitter = (jitter_percent * backoff) / 100;
	backoff += jitter;
#endif /* CONFIG_LORAWAN_JOIN_BACKOFF_JITTER_PERCENT */

	return backoff > JOIN_ACCEPT_DELAY_2_MS ? backoff - JOIN_ACCEPT_DELAY_2_MS : 0U;
}
