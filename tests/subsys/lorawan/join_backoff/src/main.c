/*
 * Copyright (c) 2026 Embeint Inc
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/lorawan/lorawan.h>
#include <zephyr/ztest.h>

#define FIRST_PERIOD_AIRTIME_MS  36000U
#define SECOND_PERIOD_AIRTIME_MS 36000U
#define LATER_PERIOD_AIRTIME_MS  8700U

#define FIRST_PERIOD_END_MS   (1U * SEC_PER_HOUR * MSEC_PER_SEC)
#define SECOND_PERIOD_END_MS  (11U * SEC_PER_HOUR * MSEC_PER_SEC)
#define LATER_PERIOD_1_END_MS (35U * SEC_PER_HOUR * MSEC_PER_SEC)
#define LATER_PERIOD_2_END_MS (59U * SEC_PER_HOUR * MSEC_PER_SEC)

#define JOIN_ACCEPT_DELAY_2_MS (6U * MSEC_PER_SEC)

static void run_join_request_timeline(uint16_t join_airtime)
{
	uint32_t first_period_airtime = 0U;
	uint32_t second_period_airtime = 0U;
	uint32_t later_period_1_airtime = 0U;
	uint32_t later_period_2_airtime = 0U;
	uint32_t tx_time_ms;

	/* Reset local uptime */
	sys_clock_tick_set(0);

	while (true) {
		/* Backoff before each join request, as required by the public API. */
		k_sleep(K_MSEC(lorawan_join_backoff(join_airtime)));

		/* Charge the request to the period in which it is actually transmitted. */
		tx_time_ms = k_uptime_get_32();
		if (tx_time_ms < FIRST_PERIOD_END_MS) {
			first_period_airtime += join_airtime;
		} else if (tx_time_ms < SECOND_PERIOD_END_MS) {
			second_period_airtime += join_airtime;
		} else if (tx_time_ms < LATER_PERIOD_1_END_MS) {
			later_period_1_airtime += join_airtime;
		} else if (tx_time_ms < LATER_PERIOD_2_END_MS) {
			later_period_2_airtime += join_airtime;
		} else {
			break;
		}

		/* Simulate the request airtime and waiting through the RX2 window. */
		k_sleep(K_MSEC(join_airtime + JOIN_ACCEPT_DELAY_2_MS));
	}

	zassert_true(first_period_airtime < FIRST_PERIOD_AIRTIME_MS,
		     "First hour had %u milliseconds of airtime", first_period_airtime);
	zassert_true(second_period_airtime < SECOND_PERIOD_AIRTIME_MS,
		     "Next 10 hours had %u milliseconds of airtime", second_period_airtime);
	zassert_true(later_period_1_airtime < LATER_PERIOD_AIRTIME_MS,
		     "Next 24 hours had %u milliseconds of airtime", later_period_1_airtime);
	zassert_true(later_period_2_airtime < LATER_PERIOD_AIRTIME_MS,
		     "Second 24 hours had %u milliseconds of airtime", later_period_1_airtime);
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
