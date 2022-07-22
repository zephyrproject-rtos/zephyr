/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/busy_sim.h>

static void test_busy_sim(void)
{
	uint32_t ms = 1000;
	uint32_t delta = 80;
	uint32_t busy_ms;
	uint32_t t = k_uptime_get_32();

	k_busy_wait(1000 * ms);
	t = k_uptime_get_32() - t;

	zassert_true((t > (ms - delta)) && (t < (ms + delta)), NULL);

	/* Start busy simulator and check that k_busy_wait last longer */
	t = k_uptime_get_32();
	busy_sim_start(500, 200, 1000, 400, NULL);
	k_busy_wait(1000 * ms);
	t = k_uptime_get_32() - t;
	busy_ms = (3 * ms) / 2;

	busy_sim_stop();
	/* due to clock imprecision, randomness and additional cpu load overhead
	 * expected time range is increased.
	 */
	zassert_true((t > (busy_ms - 2 * delta)) && (t < (busy_ms + 4 * delta)),
			"expected in range: %d-%d, k_busy_wait lasted %d",
			busy_ms - 2 * delta, busy_ms + 4 * delta,  t);

	/* Check that k_busy_wait is not interrupted after busy_sim_stop. */
	t = k_uptime_get_32();
	k_busy_wait(1000 * ms);
	t = k_uptime_get_32() - t;
	zassert_true((t > (ms - delta)) && (t < (ms + delta)), NULL);
}

void test_main(void)
{
	ztest_test_suite(busy_sim_tests,
			 ztest_unit_test(test_busy_sim)
			 );

	ztest_run_test_suite(busy_sim_tests);
}
