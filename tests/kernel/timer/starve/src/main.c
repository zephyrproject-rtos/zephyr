/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <tc_util.h>
#include <ztest.h>

#include <sys/printk.h>

#define STAMP_INTERVAL_s 60
#define TIMER_DELAY_ms 500
#define BUSY_WAIT_ms 100

static volatile uint32_t na;

static void handler(struct k_timer *timer)
{
	++na;
}

static uint32_t iters;
static uint32_t now;

static const char *tag(void)
{
	static char buf[32];

	snprintf(buf, sizeof(buf), "[%6u.%03u] %u: ",
		 now / MSEC_PER_SEC, now % MSEC_PER_SEC, iters);
	return buf;
}

static void test_starve(void)
{
	static struct k_timer tmr;
	static struct k_spinlock lock;
	uint32_t stamp = 0;
	uint32_t last_now = 0;
	uint64_t last_ticks = 0;
	k_spinlock_key_t key;

	TC_PRINT("Cycle clock runs at %u Hz\n",
		 k_ms_to_cyc_ceil32(MSEC_PER_SEC));
	TC_PRINT("There are %u cycles per tick (%u Hz ticks)\n",
		 k_ticks_to_cyc_ceil32(1U),
		 k_ms_to_ticks_ceil32(MSEC_PER_SEC));

	k_timer_init(&tmr, handler, NULL);
	while (true) {
		now = k_uptime_get_32();
		if ((now / MSEC_PER_SEC) > CONFIG_APP_STOP_S) {
			break;
		}

		++iters;

		if (now > stamp) {
			TC_PRINT("%sstill running, would pass at %u s\n",
				 tag(), CONFIG_APP_STOP_S);
			stamp += STAMP_INTERVAL_s * MSEC_PER_SEC;
		}

		int32_t now_diff = now - last_now;

		zassert_true(now_diff > 0,
			     "%sTime went backwards by %d: was %u.%03u\n",
			     tag(), -now_diff, last_now / MSEC_PER_SEC,
			     last_now % MSEC_PER_SEC);
		last_now = now;

		/* Assume tick delta fits in printable 32 bits */
		uint64_t ticks = z_tick_get();
		int64_t ticks_diff = ticks - last_ticks;

		zassert_true(ticks_diff > 0,
			     "%sTicks went backwards by %d\n",
			     tag(), -(int32_t)ticks_diff);
		last_ticks = ticks;

		uint32_t na_capture = na;

		zassert_equal(na_capture, 0,
			      "%sTimer alarm fired: %u\n",
			      na_capture);

		k_timer_start(&tmr, K_MSEC(TIMER_DELAY_ms), K_NO_WAIT);

		/* Wait with interrupts disabled to increase chance
		 * that overflow is detected.
		 */
		key = k_spin_lock(&lock);
		k_busy_wait(BUSY_WAIT_ms * USEC_PER_MSEC);
		k_spin_unlock(&lock, key);
	}
	TC_PRINT("%sCompleted %u iters without failure\n",
		 tag(), iters);
}

void test_main(void)
{
	ztest_test_suite(starve_fn, ztest_unit_test(test_starve));
	ztest_run_test_suite(starve_fn);
}
