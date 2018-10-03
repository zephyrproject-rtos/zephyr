/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

#if defined(CONFIG_ARCH_POSIX)
#define ALIGN_MS_BOUNDARY		       \
	do {				       \
		u32_t t = k_uptime_get_32();   \
		while (t == k_uptime_get_32()) \
			posix_halt_cpu();      \
	} while (0)
#else
#define ALIGN_MS_BOUNDARY		       \
	do {				       \
		u32_t t = k_uptime_get_32();   \
		while (t == k_uptime_get_32()) \
			;		       \
	} while (0)
#endif

/**
 * @addtogroup kernel_common_tests
 * @{
 */

/**
 * @brief Test clock uptime APIs functionality
 *
 * @see k_uptime_get(), k_uptime_get_32(), k_uptime_delta()
 * k_uptime_delta_32()
 */
void test_clock_uptime(void)
{
	u64_t t64, t32;
	s64_t d64 = 0;

	/**TESTPOINT: uptime elapse*/
	t64 = k_uptime_get();
	while (k_uptime_get() < (t64 + 5))
#if defined(CONFIG_ARCH_POSIX)
		posix_halt_cpu();
#else
		;
#endif

	/**TESTPOINT: uptime elapse lower 32-bit*/
	t32 = k_uptime_get_32();
	while (k_uptime_get_32() < (t32 + 5))
#if defined(CONFIG_ARCH_POSIX)
		posix_halt_cpu();
#else
		;
#endif

	/**TESTPOINT: uptime straddled ms boundary*/
	t32 = k_uptime_get_32();
	ALIGN_MS_BOUNDARY;
	zassert_true(k_uptime_get_32() > t32, NULL);

	/**TESTPOINT: uptime delta*/
	d64 = k_uptime_delta(&d64);
	/* Note: this will stall if the systick period < 5ms */
	while (k_uptime_delta(&d64) < 5)
#if defined(CONFIG_ARCH_POSIX)
		posix_halt_cpu();
#else
		;
#endif

	/**TESTPOINT: uptime delta lower 32-bit*/
	k_uptime_delta_32(&d64);
	/* Note: this will stall if the systick period < 5ms */
	while (k_uptime_delta_32(&d64) < 5)
#if defined(CONFIG_ARCH_POSIX)
		posix_halt_cpu();
#else
		;
#endif

	/**TESTPOINT: uptime delta straddled ms boundary*/
	k_uptime_delta_32(&d64);
	ALIGN_MS_BOUNDARY;
	zassert_true(k_uptime_delta_32(&d64) > 0, NULL);
}

/**
 * @brief Test clock cycle functionality
 *
 * @see k_cycle_get_32(), k_uptime_get_32()
 */
void test_clock_cycle(void)
{
	u32_t c32, c0, c1, t32;

	/**TESTPOINT: cycle elapse*/
	ALIGN_MS_BOUNDARY;
	c32 = k_cycle_get_32();
	/*break if cycle counter wrap around*/
	while (k_cycle_get_32() > c32 &&
	       k_cycle_get_32() < (c32 + sys_clock_hw_cycles_per_tick))
#if defined(CONFIG_ARCH_POSIX)
		posix_halt_cpu();
#else
		;
#endif

	/**TESTPOINT: cycle/uptime cross check*/
	c0 = k_cycle_get_32();
	ALIGN_MS_BOUNDARY;
	t32 = k_uptime_get_32();
	while (t32 == k_uptime_get_32())
#if defined(CONFIG_ARCH_POSIX)
		posix_halt_cpu();
#else
		;
#endif

	c1 = k_uptime_get_32();
	/*avoid cycle counter wrap around*/
	if (c1 > c0) {
		/* delta cycle should be greater than 1 milli-second*/
		zassert_true((c1 - c0) >
			     (sys_clock_hw_cycles_per_sec / MSEC_PER_SEC), NULL);
		/* delta NS should be greater than 1 milli-second */
		zassert_true(SYS_CLOCK_HW_CYCLES_TO_NS(c1 - c0) >
			     (NSEC_PER_SEC / MSEC_PER_SEC), NULL);
	}
}

/**
 * @}
 */
