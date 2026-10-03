/*
 * Copyright (c) 2026 Embeint Inc
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/lorawan/lorawan.h>
#include <zephyr/ztest.h>

#define FIRST_PERIOD_AIRTIME_MS  36000U
#define SECOND_PERIOD_AIRTIME_MS 36000U
#define LATER_PERIOD_AIRTIME_MS  8700U

#define HOUR_MS                (60ULL * 60ULL * MSEC_PER_SEC)
#define JOIN_ACCEPT_DELAY_2_MS (6U * MSEC_PER_SEC)

static uint64_t run_join_requests(uint16_t join_airtime, uint32_t *cumulative_airtime,
				  uint64_t *elapsed_time, uint32_t period_airtime)
{
	uint64_t start_time = *elapsed_time;
	uint32_t end_airtime = *cumulative_airtime + period_airtime;

	/* Run 'joins' until the airtime hits the provided threshold */
	while (*cumulative_airtime < end_airtime) {
		*cumulative_airtime += join_airtime;
		*elapsed_time += join_airtime;
		*elapsed_time += JOIN_ACCEPT_DELAY_2_MS;
		*elapsed_time += lorawan_join_backoff(join_airtime, *cumulative_airtime);
	}

	/* Return the total duration required to hit the transmit airtime */
	return *elapsed_time - start_time;
}

static void run_join_request_timeline(uint16_t join_airtime)
{
	uint32_t cumulative_airtime = 0U;
	uint64_t elapsed_time = 0U;
	uint64_t period_duration;

	period_duration = run_join_requests(join_airtime, &cumulative_airtime, &elapsed_time,
					    FIRST_PERIOD_AIRTIME_MS);
	zassert_true(period_duration > HOUR_MS,
		     "first 36 seconds of airtime took only %llu milliseconds", period_duration);

	period_duration = run_join_requests(join_airtime, &cumulative_airtime, &elapsed_time,
					    SECOND_PERIOD_AIRTIME_MS);
	zassert_true(period_duration > 10U * HOUR_MS,
		     "next 36 seconds of airtime took only %llu milliseconds", period_duration);

	period_duration = run_join_requests(join_airtime, &cumulative_airtime, &elapsed_time,
					    LATER_PERIOD_AIRTIME_MS);
	zassert_true(period_duration > 24U * HOUR_MS,
		     "next 8.7 seconds of airtime took only %llu milliseconds", period_duration);
}

ZTEST(lorawan_join_backoff, test_join_duty_cycle_100ms)
{
	run_join_request_timeline(100U);
}

ZTEST(lorawan_join_backoff, test_join_duty_cycle_1000ms)
{
	run_join_request_timeline(1000U);
}

ZTEST(lorawan_join_backoff, test_join_duty_cycle_1600ms)
{
	run_join_request_timeline(1600U);
}

ZTEST(lorawan_join_backoff, test_receive_duration_exceeds_backoff)
{
	const uint16_t join_airtime = 1U;
	uint32_t interval;

	interval = join_airtime + JOIN_ACCEPT_DELAY_2_MS +
		   lorawan_join_backoff(join_airtime, join_airtime);

	zassert_equal(interval, join_airtime + JOIN_ACCEPT_DELAY_2_MS);
}

static void *join_backoff_test_setup(void)
{
#ifdef CONFIG_LORAWAN_JOIN_BACKOFF_JITTER_PERCENT
	printk("Backoff jitter: %d%%\r\n", CONFIG_LORAWAN_JOIN_BACKOFF_JITTER_PERCENT);
#else
	printk("Backoff jitter: None\r\n");
#endif /* CONFIG_LORAWAN_JOIN_BACKOFF_JITTER_PERCENT */
	return NULL;
}

ZTEST_SUITE(lorawan_join_backoff, NULL, join_backoff_test_setup, NULL, NULL, NULL);
