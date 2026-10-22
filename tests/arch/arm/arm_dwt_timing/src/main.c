/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Yiren Guo <guoyr_2013@hotmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/timing/timing.h>
#include <cmsis_core.h>

ZTEST(arm_dwt_timing, test_freq_get_with_counter_stopped)
{
	uint64_t freq;

	arch_timing_init();
	arch_timing_start();
	arch_timing_stop();

	zassert_equal(DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk, 0,
		      "arch_timing_stop() did not stop the cycle counter");

	freq = arch_timing_freq_get();

	zassert_true(freq > 0, "no DWT frequency reported");
	zassert_equal(DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk, 0,
		      "arch_timing_freq_get() left the cycle counter running");
}

ZTEST(arm_dwt_timing, test_measurement_after_freq_get)
{
	timing_t start, end;

	arch_timing_init();
	arch_timing_start();

	start = arch_timing_counter_get();
	k_busy_wait(1000);
	end = arch_timing_counter_get();

	zassert_true(arch_timing_cycles_get(&start, &end) > 0, "cycle counter did not advance");

	arch_timing_stop();
}

ZTEST_SUITE(arm_dwt_timing, NULL, NULL, NULL, NULL, NULL);
