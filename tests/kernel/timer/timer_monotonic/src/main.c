/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <tc_util.h>
#include <ztest.h>

static struct k_timer test_timer;
static struct k_timer test_periodtimer;

u32_t start_time, stop_time, cycles_spent, nanoseconds_spent,
	   time_spent[10], cycles_cost[10], pctlist[10];
int count = 0;



static void callback_period(struct k_timer *timer)
{
	u32_t pct;

	stop_time = k_cycle_get_32();
	cycles_spent = stop_time - start_time;
	nanoseconds_spent = (u32_t)k_cyc_to_ns_floor64(cycles_spent);
	pct = (u64_t)cycles_spent * 100000U / sys_clock_hw_cycles_per_sec();
	start_time = stop_time;
	pctlist[count] = pct;
	time_spent[count] = nanoseconds_spent;
	cycles_cost[count] = cycles_spent;

	pct =0;

	count++;
	if (count >= 10) {
		k_timer_stop(timer);
	}
}

static void callback_duration(struct k_timer *timer)
{
	stop_time = k_cycle_get_32();
}

int test_periodtimer_frequency(void)
{
	int i;

	TC_PRINT("Testing system tick frequency\n");

	k_sleep(1);
	k_timer_init(&test_periodtimer, callback_period, NULL);
	start_time = k_cycle_get_32();
	k_timer_start(&test_periodtimer, 1, 1);
	k_sleep(1000);
	k_timer_stop(&test_periodtimer);
	for(i = 0; i < 10; i++)
	{
		printk("delta: %u expected cycles: %u "
				"expected time(nanosec): %u "
				"cost time(nanosec): %u  %u%%\n",
				cycles_cost[i], sys_clock_hw_cycles_per_sec()/1000,
				1000000 ,time_spent[i], pctlist[i]);
		/* Heuristic: if we're more than 10% off, throw an error */
		if (pctlist[i] < 90 || pctlist[i] > 110) {
			TC_PRINT("Clock calibration is way off!\n");
			return -1;
			break;
		}
		time_spent[i] = 0;
		cycles_cost[i] = 0;
		pctlist[i] = 0;
	}

	start_time = 0;
	stop_time = 0;
	cycles_spent = 0;
	nanoseconds_spent = 0;

	return 0;
}

int test_oneshottimer_frequency(void)
{
	u32_t delta, pct;

	TC_PRINT("Testing system tick frequency\n");

	k_sleep(1);
	k_timer_init(&test_timer, callback_duration, NULL);
	start_time = k_cycle_get_32();

	k_timer_start(&test_timer, 1, 0);
	k_timer_status_sync(&test_timer);
	delta = stop_time - start_time;
	nanoseconds_spent = (u32_t)k_cyc_to_ns_floor64(delta);
	k_timer_stop(&test_timer);
	pct = (u64_t)delta * 100000U / sys_clock_hw_cycles_per_sec();

	printk("delta: %u expected cycles: %u "
			"expected time(nanosec): %u "
			"cost time(nanosec): %u  %u%%\n",
			delta, sys_clock_hw_cycles_per_sec()/1000,
			1000000, nanoseconds_spent, pct);

	start_time = 0;
	stop_time = 0;
	nanoseconds_spent = 0;
	/* Heuristic: if we're more than 10% off, throw an error */
	if (pct < 90 || pct > 110) {
		TC_PRINT("Clock calibration is way off!\n");
		return -1;
	}

	return 0;
}

int test_frequency_second(void)
{
	u32_t start, end, delta, nanoseconds_spent, pct;

	TC_PRINT("Testing system tick frequency\n");

	start = k_cycle_get_32();
	k_sleep(K_MSEC(1000));
	end = k_cycle_get_32();
	delta = end - start;
	nanoseconds_spent = (u32_t)k_cyc_to_ns_floor64(delta);
	pct = (u64_t)delta * 100U / sys_clock_hw_cycles_per_sec();

	printk("delta: %u expected cycles: %u "
			"expected time(nanosec): %u "
			"cost time(nanosec): %u  %u%%\n",
			delta, sys_clock_hw_cycles_per_sec(),
			1000000000, nanoseconds_spent,pct);

	/* Heuristic: if we're more than 10% off, throw an error */
	if (pct < 90 || pct > 110) {
		TC_PRINT("Clock calibration is way off!\n");
		return -1;
	}

	return 0;
}

int test_frequency_msecond(void)
{
	u32_t start, end, delta, nanoseconds_spent, pct;

	TC_PRINT("Testing system tick frequency\n");

	start = k_cycle_get_32();
	k_sleep(K_MSEC(1));
	end = k_cycle_get_32();
	delta = end - start;
	nanoseconds_spent = (u32_t)k_cyc_to_ns_floor64(delta);
	pct = (u64_t)delta * 100000U / sys_clock_hw_cycles_per_sec();

	printk("delta: %u expected cycles: %u "
			"expected time(nanosec): %u "
			"cost time(nanosec): %u  %u%%\n",
			delta, sys_clock_hw_cycles_per_sec()/1000,
			1000000, nanoseconds_spent, pct);

	/* Heuristic: if we're more than 10% off, throw an error */
	if (pct < 90 || pct > 110) {
		TC_PRINT("Clock calibration is way off!\n");
		return -1;
	}

	return 0;
}
/**
 *  @brief Test monotonic timer
 *
 * Validates monotonic timer's clock calibration for 1 second.
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
void test_timer_sec(void)
{
	u32_t t_last, t_now, i, errors;
	s32_t diff;

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

	zassert_false(test_frequency_second(),
			"test frequency failed");
}

/**
 * @brief Test monotonic timer
 *
 * Validates monotonic timer's clock calibration for 1 ms.
 *
 * It reads the System clock’s h/w timer frequency value continuously
 * using k_cycle_get_32() to verify its working and correctiveness.
 * It also checks system tick frequency by checking the delta error
 * between generated and system clock provided HW cycles per msec values.
 *
 * @ingroup kernel_timer_tests
 *
 * @see k_cycle_get_32(), sys_clock_hw_cycles_per_sec()
 */
void test_timer_msec(void)
{
	u32_t t_last, t_now, i, errors;
	s32_t diff;

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

	}
	zassert_false(errors, "errors = %d\n", errors);
	zassert_false(test_frequency_msecond(),
			"test frequency failed");
}

/**
 * @brief Test one shot timer
 *
 * Validates one shot timer's clock calibration for 1 ms.
 *
 * It reads the System clock’s h/w timer frequency value continuously
 * using k_cycle_get_32() to verify its working and correctiveness.
 * It also checks system tick frequency by checking the delta error
 * between generated and system clock provided HW cycles per msec values.
 *
 * @ingroup kernel_timer_tests
 *
 * @see k_cycle_get_32(), sys_clock_hw_cycles_per_sec()
 */
void test_oneshot_timer(void)
{

	u32_t t_last, t_now, i, errors;
	s32_t diff;

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
	}
	zassert_false(errors, "errors = %d\n", errors);
	zassert_false(test_oneshottimer_frequency(),
			"test one shot timer frequency failed");
}


/**
 * @brief Test period timer
 *
 * Validates period timer's clock calibration for 1 ms.
 *
 * It reads the System clock’s h/w timer frequency value continuously
 * using k_cycle_get_32() to verify its working and correctiveness.
 * It also checks system tick frequency by checking the delta error
 * between generated and system clock provided HW cycles per msec values.
 *
 * @ingroup kernel_timer_tests
 *
 * @see k_cycle_get_32(), sys_clock_hw_cycles_per_sec()
 */
void test_period_timer(void)
{
	u32_t t_last, t_now, i, errors;
	s32_t diff;

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
					"i = %u\n",diff, t_last, t_now, i);
			errors++;
		}
	}
	zassert_false(errors, "errors = %d\n", errors);
	zassert_false(test_periodtimer_frequency(),
			"test period timer frequency failed");
}

void test_main(void)
{
	ztest_test_suite(timer_fn,
			ztest_unit_test(test_timer_sec),
			ztest_unit_test(test_timer_msec),
			ztest_unit_test(test_oneshot_timer),
			ztest_unit_test(test_period_timer));

	ztest_run_test_suite(timer_fn);
}
