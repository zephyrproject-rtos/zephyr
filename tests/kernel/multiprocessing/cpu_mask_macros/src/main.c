/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>
#include <zephyr/sys/util.h>
#include <ksmp.h>

#define LAST_CPU   (CONFIG_MP_MAX_NUM_CPUS - 1)

/**
 * @brief Tests for the CPU_MASK_CPU_ALL, CPU_MASK_CPU() and CPU_MASK_CPU_RANGE() macros
 *
 * @defgroup kernel_cpu_mask_macros_tests CPU Mask Macros
 *
 * @ingroup all_tests
 * @{
 * @}
 */

/* Test that CPU_MASK_CPU_ALL has exactly one bit for each configured CPU */
ZTEST(cpu_mask_macros, test_cpu_all)
{
	uint32_t expected = 0U;

	for (int i = 0; i < CONFIG_MP_MAX_NUM_CPUS; i++) {
		expected |= (uint32_t)BIT(i);
	}

	zassert_equal(CPU_MASK_CPU_ALL, expected, "CPU_MASK_CPU_ALL is 0x%x, expected 0x%x",
		      CPU_MASK_CPU_ALL, expected);
	zassert_equal(POPCOUNT(CPU_MASK_CPU_ALL), CONFIG_MP_MAX_NUM_CPUS, "wrong number of bits");

	/* No bit is set for a CPU that does not exist */
	for (int i = CONFIG_MP_MAX_NUM_CPUS; i < 32; i++) {
		zassert_equal(CPU_MASK_CPU_ALL & (uint32_t)BIT(i), 0U,
			      "bit %d of a nonexistent CPU is set", i);
	}
}

/* Test that CPU_MASK_CPU() yields the bit of each existing CPU */
ZTEST(cpu_mask_macros, test_single_cpu)
{
	uint32_t seen = 0U;

	for (int i = 0; i < CONFIG_MP_MAX_NUM_CPUS; i++) {
		uint32_t mask = CPU_MASK_CPU(i);

		zassert_equal(mask, (uint32_t)BIT(i), "CPU %d has mask 0x%x", i, mask);

		seen |= mask;
	}

	zassert_equal(seen, CPU_MASK_CPU_ALL, "the CPU masks do not add up to all CPUs");
}

/* Test that CPU_MASK_CPU_RANGE() covers first to last, both included */
ZTEST(cpu_mask_macros, test_range)
{
	zassert_equal(CPU_MASK_CPU_RANGE(0, 0), 0x01U, "range 0-0");
	zassert_equal(CPU_MASK_CPU_RANGE(7, 7), 0x80U, "range 7-7");
	zassert_equal(CPU_MASK_CPU_RANGE(0, 2), 0x07U, "range 0-2");
	zassert_equal(CPU_MASK_CPU_RANGE(1, 3), 0x0eU, "range 1-3");
	zassert_equal(CPU_MASK_CPU_RANGE(4, 7), 0xf0U, "range 4-7");
	zassert_equal(CPU_MASK_CPU_RANGE(0, 7), CPU_MASK_CPU_ALL, "range 0-7 is not all CPUs");
}

/* Test comparisons of masks against CPU_MASK_CPU_ALL */
ZTEST(cpu_mask_macros, test_compare_with_all)
{
	uint32_t one = CPU_MASK_CPU(3);
	uint32_t others = CPU_MASK_CPU_ALL & ~one;

	zassert_not_equal(one, CPU_MASK_CPU_ALL, "one CPU equals all CPUs");
	zassert_not_equal(others, CPU_MASK_CPU_ALL, "all but one CPU equals all CPUs");

	/* A mask that includes bits of nonexistent CPUs is not all CPUs.
	 * Not possible as this test suite is configured for 8 CPUs.
	 */
	zassert_not_equal(UINT32_MAX, CPU_MASK_CPU_ALL, "0xffffffff equals all CPUs");
}

/*
 * An out of range CPU makes the macros assert, which is a fatal error that ends the test thread.
 * Each case therefore needs a test of its own, and a macro that returns instead of asserting fails
 * the test. The arguments are volatile to keep the compiler from rejecting the shift of a constant
 * out of range.
 */
#define EXPECT_ASSERT(expr)                                                                        \
	do {                                                                                       \
		if (!IS_ENABLED(CONFIG_ASSERT)) {                                                  \
			ztest_test_skip();                                                         \
		}                                                                                  \
		ztest_set_fault_valid(true);                                                       \
		volatile uint32_t mask = (expr);                                                   \
                                                                                                   \
		ARG_UNUSED(mask);                                                                  \
		ztest_test_fail();                                                                 \
	} while (0)

/* Test that CPU_MASK_CPU() asserts for the CPU after the last one */
ZTEST(cpu_mask_macros, test_cpu_after_last_asserts)
{
	volatile int cpu = LAST_CPU + 1;

	EXPECT_ASSERT(CPU_MASK_CPU(cpu));
}

/* Test that CPU_MASK_CPU() asserts for a negative CPU */
ZTEST(cpu_mask_macros, test_cpu_negative_asserts)
{
	volatile int cpu = -1;

	EXPECT_ASSERT(CPU_MASK_CPU(cpu));
}

/* Test that CPU_MASK_CPU_RANGE() asserts when the last CPU does not exist */
ZTEST(cpu_mask_macros, test_range_last_after_last_asserts)
{
	volatile int first = 0;
	volatile int last = LAST_CPU + 1;

	EXPECT_ASSERT(CPU_MASK_CPU_RANGE(first, last));
}

/* Test that CPU_MASK_CPU_RANGE() asserts when the first CPU is negative */
ZTEST(cpu_mask_macros, test_range_first_negative_asserts)
{
	volatile int first = -1;
	volatile int last = 0;

	EXPECT_ASSERT(CPU_MASK_CPU_RANGE(first, last));
}

/* Test that CPU_MASK_CPU_RANGE() asserts when first is after last */
ZTEST(cpu_mask_macros, test_range_reversed_asserts)
{
	volatile int first = 1;
	volatile int last = 0;

	EXPECT_ASSERT(CPU_MASK_CPU_RANGE(first, last));
}

ZTEST_SUITE(cpu_mask_macros, NULL, NULL, NULL, NULL, NULL);
