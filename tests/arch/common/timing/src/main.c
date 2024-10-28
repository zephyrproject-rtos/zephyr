/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/arch_interface.h>

#if defined(CONFIG_SMP) && CONFIG_MP_MAX_NUM_CPUS > 1
#define SMP_TEST
#endif

#ifdef SMP_TEST
#define MAX_NUM_THREADS CONFIG_MP_MAX_NUM_CPUS
#define STACK_SIZE  1024
#define PRIORITY    7

static struct k_thread threads[MAX_NUM_THREADS];
static K_THREAD_STACK_ARRAY_DEFINE(tstack, MAX_NUM_THREADS, STACK_SIZE);
#endif

#define WAIT_US 1000
#define WAIT_NS (WAIT_US * 1000)
#define TOLERANCE 0.1

static void perform_tests(void)
{
	timing_t start, middle, end;
	uint64_t diff1, diff2, diff_all, freq_hz;
	uint64_t diff1_ns, diff2_ns, diff_all_ns, diff_avg_ns;
	uint32_t freq_mhz;

	arch_timing_start();

	start = arch_timing_counter_get();
	k_busy_wait(WAIT_US);
	middle = arch_timing_counter_get();
	k_busy_wait(WAIT_US);
	end = arch_timing_counter_get();

	/* Time shouldn't stop or go backwards */
	diff1 = arch_timing_cycles_get(&start, &middle);
	diff2 = arch_timing_cycles_get(&middle, &end);
	diff_all = arch_timing_cycles_get(&start, &end);
	zassert_true(diff1 > 0, NULL);
	zassert_true(diff2 > 0, NULL);
	zassert_true(diff_all > 0, NULL);

	/* Differences shouldn't be so different, as both are spaced by
	 * k_busy_wait(WAIT_US).
	 */
	zassert_within(diff1, diff2, diff1 * TOLERANCE, NULL);
	zassert_within(diff_all, diff1 + diff2, (diff1 + diff2) * TOLERANCE,
		       NULL);

	freq_hz = arch_timing_freq_get();
	freq_mhz = arch_timing_freq_get_mhz();
	zassert_equal(freq_mhz, (uint32_t)(freq_hz / 1000000ULL), NULL);

	diff1_ns = arch_timing_cycles_to_ns(diff1);
	diff2_ns = arch_timing_cycles_to_ns(diff2);
	diff_all_ns = arch_timing_cycles_to_ns(diff_all);

	/* Ensure the differences are close to 100us */
	zassert_within(diff1_ns, WAIT_NS, WAIT_NS * TOLERANCE, NULL);
	zassert_within(diff2_ns, WAIT_NS, WAIT_NS * TOLERANCE, NULL);
	zassert_within(diff_all_ns, 2 * WAIT_NS, 2 * WAIT_NS * TOLERANCE, NULL);

	diff_avg_ns = arch_timing_cycles_to_ns_avg(diff1 + diff2, 2);
	zassert_within(diff_avg_ns, WAIT_NS, WAIT_NS * TOLERANCE, NULL);

	arch_timing_stop();
}

void *timing_setup(void)
{
	arch_timing_init();

	return NULL;
}

ZTEST(arch_timing, test_arch_timing)
{
	perform_tests();
	/* Run tests again to ensure nothing breaks after arch_timing_stop */
	perform_tests();
}

#ifdef SMP_TEST
static void thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	perform_tests();
	/* Run tests again to ensure nothing breaks after arch_timing_stop */
	perform_tests();
}

ZTEST(arch_timing, test_arch_timing_smp)
{
	int i;
	unsigned int num_threads = arch_num_cpus();

	for (i = 0; i < num_threads; i++) {
		k_thread_create(&threads[i], tstack[i], STACK_SIZE,
				thread_entry, NULL, NULL, NULL,
				PRIORITY, 0, K_FOREVER);
		k_thread_cpu_mask_enable(&threads[i], i);
		k_thread_start(&threads[i]);
	}

	for (i = 0; i < num_threads; i++) {
		k_thread_join(&threads[i], K_FOREVER);
	}
}
#else
ZTEST(arch_timing, test_arch_timing_smp)
{
	ztest_test_skip();
}
#endif

ZTEST_SUITE(arch_timing, NULL, timing_setup, NULL, NULL, NULL);
