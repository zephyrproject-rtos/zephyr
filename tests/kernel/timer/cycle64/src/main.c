/*
 * Copyright (c) 2021 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/zephyr.h>

static void swap64(uint64_t *a, uint64_t *b)
{
	uint64_t t = *a;
	*a = *b;
	*b = t;
}

static void msg(uint64_t c64)
{
	int64_t ms = k_uptime_get();
	int s = ms / 1000;
	int m = s / 60;
	int h = m / 60;
	int d = h / 24;

	h %= 24;
	m %= 60;
	s %= 60;
	ms %= 1000;

	printk("[%03d:%02d:%02d:%02d.%03d]: cycle: %016" PRIx64 "\n", d, h, m, s, (int)ms, c64);
}

uint32_t timeout(uint64_t prev, uint64_t now)
{
	uint64_t next = prev + BIT64(32) - now;

	next &= UINT32_MAX;
	if (next == 0) {
		next = UINT32_MAX;
	}

	return (uint32_t)next;
}

static void test_32bit_wrap_around(void)
{
	enum {
		CURR,
		PREV,
	};

	int i;
	uint64_t now;
	uint64_t c64[2];

	printk("32-bit wrap-around should occur every %us\n",
	       (uint32_t)(BIT64(32) / (uint32_t)sys_clock_hw_cycles_per_sec()));

	printk("[ddd:hh:mm:ss.0ms]\n");

	c64[CURR] = k_cycle_get_64();
	msg(c64[CURR]);

	for (i = 0; i < 2; ++i) {
		k_sleep(Z_TIMEOUT_CYC(timeout(c64[CURR], k_cycle_get_64())));

		now = k_cycle_get_64();
		swap64(&c64[PREV], &c64[CURR]);
		c64[CURR] = now;

		msg(c64[CURR]);

		zassert_equal(((c64[CURR] - c64[PREV]) >> 32), 1,
			 "The 64-bit cycle counter did not increment by 2^32");
	}
}

void test_main(void)
{
	ztest_test_suite(cycle64_tests,
			 ztest_unit_test(test_32bit_wrap_around));

	ztest_run_test_suite(cycle64_tests);
}
