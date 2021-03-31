/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <tc_util.h>
#include <ztest.h>

#define DURATION 50
#define PERIOD   50
#define EXPIRE_TIMES 5

static ZTEST_BMEM uint32_t t_start, t_delta, t_end, d_cycle;
static ZTEST_BMEM uint32_t flag1, flag2;
static struct k_timer timer;

int test_frequency(void)
{
	uint32_t start, end, delta, pct;

	TC_PRINT("Testing system tick frequency\n");

	start = k_cycle_get_32();
	k_sleep(K_MSEC(1000));
	end = k_cycle_get_32();

	delta = end - start;
	pct = (uint64_t)delta * 100U / sys_clock_hw_cycles_per_sec();

	printk("delta: %u  expected: %u  %u%%\n", delta,
	       sys_clock_hw_cycles_per_sec(), pct);

	/* Heuristic: if we're more than 10% off, throw an error */
	if (pct < 90 || pct > 110) {
		TC_PRINT("Clock calibration is way off!\n");
		return -1;
	}

	return 0;
}

/**
 * @brief Test monotonic timer
 *
 * Validates monotonic timer's clock calibration.
 *
 * It reads the System clock’s h/w timer frequency value continuously
 * using k_cycle_get_32() to verify its working and correctiveness.
 * It also checks system tick frequency by checking the delta error
 * between generated and system clock provided HW cycles per sec values.
 *
 * @ingroup kernel_timer_tests
 *
 * @see k_cycle_get_32(), sys_clock_hw_cycles_per_sec()
 */
void test_timer(void)
{
	uint32_t t_last, t_now, i, errors;
	int32_t diff;

	errors = 0U;

	TC_PRINT("k_ticks_to_cyc_floor32(1) = %d\n",
		 k_ticks_to_cyc_floor32(1));
	TC_PRINT("sys_clock_hw_cycles_per_sec() = %d\n",
		 sys_clock_hw_cycles_per_sec());

	TC_START("test monotonic timer");

	t_last = k_cycle_get_32();

	for (i = 0U; i < 1000000; i++) {
		t_now = k_cycle_get_32();

		if (t_now < t_last) {
			diff = t_now - t_last;
			TC_PRINT("diff = %d (t_last = %u : t_now = %u);"
				"i = %u\n", diff, t_last, t_now, i);
			errors++;
		}
		t_last = t_now;
	}

	zassert_false(errors, "errors = %d\n", errors);

	zassert_false(test_frequency(), "test frequency failed");
}

static void stop_duration(struct k_timer *timer)
{
	flag2++;
}

static void expiry_entry(struct k_timer *timer)
{
	t_end = k_cycle_get_32();
	t_delta = t_end - t_start;
	printk("start = %d, end = %d, delta = %d, d_cycle = %d\n",
	       t_start, t_end, t_delta, d_cycle);
	zassert_true(t_delta >= d_cycle, "expiry duration error");

	flag1++;
	/* Get the current cycle */
	t_start = k_cycle_get_32();
}

static void timer_init(struct k_timer *timer)
{
	/* init timer object and data */
	k_timer_init(timer, expiry_entry, stop_duration);
	flag1 = 0;
	flag2 = 0;
}

/**
 * @brief Test timer duration precise
 *
 * @details Define and init a timer. Get cycle before start timer,
 * Every timer duration need to get cycle and verify duration equal
 * with cycle or not.
 *
 * @ingroup kernel_timer_tests
 *
 * @see k_cycle_get_32(), k_timer_init(), k_timer_start(), k_timer_stop()
 */
void test_timer_cycle(void)
{
	/* convert ms to cyc is not exactly correct */
	d_cycle = k_ms_to_cyc_floor32(45);

	/* get current cycle */
	t_start = k_cycle_get_32();
	k_timer_start(&timer, K_MSEC(DURATION), K_MSEC(PERIOD));

	k_busy_wait(PERIOD * EXPIRE_TIMES * 1000 + PERIOD / 2 * 1000);
	k_timer_stop(&timer);

	/* check flag count */
	zassert_equal(flag1, EXPIRE_TIMES, "expiry count error");
	zassert_equal(flag2, 1, "stop count error");
}

void test_main(void)
{
	timer_init(&timer);
	k_thread_access_grant(k_current_get(), &timer);

	ztest_test_suite(timer_fn,
			ztest_unit_test(test_timer),
			ztest_unit_test(test_timer_cycle));
	ztest_run_test_suite(timer_fn);
}
