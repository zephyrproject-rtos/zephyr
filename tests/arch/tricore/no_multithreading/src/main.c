/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

/*
 * With CONFIG_MULTITHREADING=n the kernel reaches the application through
 * z_tricore_switch_to_main_no_multithreading rather than the scheduler.
 * Reaching any test at all proves that path works; the cases below confirm
 * the core no-thread primitives still operate.
 */

ZTEST(tricore_no_mt, test_reached_main)
{
	zassert_true(true, "did not reach the test without multithreading");
}

ZTEST(tricore_no_mt, test_irq_lock_unlock)
{
	unsigned int key = irq_lock();

	irq_unlock(key);
	zassert_true(true);
}

ZTEST(tricore_no_mt, test_busy_wait)
{
	uint64_t hz = sys_clock_hw_cycles_per_sec();
	uint64_t t0 = k_cycle_get_64();

	k_busy_wait(1000);

	uint64_t dt = k_cycle_get_64() - t0;

	zassert_true(dt >= (hz * 8) / 10000, "1ms busy_wait elapsed only %llu cycles", dt);
}

ZTEST_SUITE(tricore_no_mt, NULL, NULL, NULL, NULL, NULL);
