/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#if defined(CONFIG_ARCH_POSIX)
#define ALIGN_MS_BOUNDARY		       \
	do {				       \
		uint32_t t = k_uptime_get_32();   \
		while (t == k_uptime_get_32()) \
			k_busy_wait(50);       \
	} while (0)
#else
#define ALIGN_MS_BOUNDARY		       \
	do {				       \
		uint32_t t = k_uptime_get_32();   \
		while (t == k_uptime_get_32()) \
			;		       \
	} while (0)
#endif

struct timer_data {
	int duration_count;
	int stop_count;
};
static void duration_expire(struct k_timer *timer);
static void stop_expire(struct k_timer *timer);

/** TESTPOINT: init timer via K_TIMER_DEFINE */
K_TIMER_DEFINE(ktimer, duration_expire, stop_expire);

static ZTEST_BMEM struct timer_data tdata;

#define DURATION 100
#define LESS_DURATION 70

/**
 * @addtogroup kernel_common_tests
 * @{
 */

/**
 * @brief Test clock uptime APIs functionality
 *
 * @see k_uptime_get(), k_uptime_get_32(), k_uptime_delta()
 */
ZTEST_USER(clock, test_clock_uptime)
{
	uint64_t t64, t32;
	int64_t d64 = 0;

	/**TESTPOINT: uptime elapse*/
	t64 = k_uptime_get();
	while (k_uptime_get() < (t64 + 5)) {
#if defined(CONFIG_ARCH_POSIX)
		k_busy_wait(50);
#endif
	}

	/**TESTPOINT: uptime elapse lower 32-bit*/
	t32 = k_uptime_get_32();
	while (k_uptime_get_32() < (t32 + 5)) {
#if defined(CONFIG_ARCH_POSIX)
		k_busy_wait(50);
#endif
	}

	/**TESTPOINT: uptime straddled ms boundary*/
	t32 = k_uptime_get_32();
	ALIGN_MS_BOUNDARY;
	zassert_true(k_uptime_get_32() > t32);

	/**TESTPOINT: uptime delta*/
	d64 = k_uptime_delta(&d64);
	while (k_uptime_delta(&d64) == 0) {
#if defined(CONFIG_ARCH_POSIX)
		k_busy_wait(50);
#endif
	}
}

/**
 * @brief Test 32-bit clock cycle functionality
 *
 * @details
 * Test Objective:
 * - The kernel architecture provide a 32bit monotonically increasing
 *   cycle counter
 * - This routine tests the k_cycle_get_32() and k_uptime_get_32()
 *   k_cycle_get_32() get cycles by accessing hardware clock.
 *   k_uptime_get_32() return cycles by transforming ticks into cycles.
 *
 * Testing techniques
 * - Functional and black box testing
 *
 * Prerequisite Condition:
 * - N/A
 *
 * Input Specifications:
 * - N/A
 *
 * Expected Test Result:
 * - The timer increases monotonically
 *
 * Pass/Fail criteria:
 * - Success if cycles increase monotonically, failure otherwise.
 *
 * Test Procedure:
 * -# At milli-second boundary, get cycles repeatedly by k_cycle_get_32()
 *  till cycles increased
 * -# At milli-second boundary, get cycles repeatedly by k_uptime_get_32()
 *  till cycles increased
 * -# Cross check cycles gotten by k_cycle_get_32() and k_uptime_get_32(),
 *  the delta cycle should be greater than 1 milli-second.
 *
 * Assumptions and Constraints
 * - N/A
 *
 * @see k_cycle_get_32(), k_uptime_get_32()
 */

ZTEST(clock, test_clock_cycle_32)
{
	uint32_t c32, c0, c1, t32;

	/**TESTPOINT: cycle elapse*/
	ALIGN_MS_BOUNDARY;
	c32 = k_cycle_get_32();
	/*break if cycle counter wrap around*/
	while (k_cycle_get_32() > c32 &&
	       k_cycle_get_32() < (c32 + k_ticks_to_cyc_floor32(1))) {
#if defined(CONFIG_ARCH_POSIX)
		k_busy_wait(50);
#endif
	}

	/**TESTPOINT: cycle/uptime cross check*/
	c0 = k_cycle_get_32();
	ALIGN_MS_BOUNDARY;
	t32 = k_uptime_get_32();
	while (t32 == k_uptime_get_32()) {
#if defined(CONFIG_ARCH_POSIX)
		k_busy_wait(50);
#endif
	}

	c1 = k_uptime_get_32();
	/*avoid cycle counter wrap around*/
	if (c1 > c0) {
		/* delta cycle should be greater than 1 milli-second*/
		zassert_true((c1 - c0) >
			     (sys_clock_hw_cycles_per_sec() / MSEC_PER_SEC),
			     NULL);
		/* delta NS should be greater than 1 milli-second */
		zassert_true((uint32_t)k_cyc_to_ns_floor64(c1 - c0) >
			     (NSEC_PER_SEC / MSEC_PER_SEC), NULL);
	}
}

/**
 * @brief Test 64-bit clock cycle functionality
 */
ZTEST(clock, test_clock_cycle_64)
{
	uint32_t d32;
	uint64_t d64;
	uint32_t t32[2];
	uint64_t t64[2];

	if (!IS_ENABLED(CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER)) {
		ztest_test_skip();
	}

	t64[0] = k_cycle_get_64();
	t32[0] = k_cycle_get_32();

	k_msleep(1);

	t32[1] = k_cycle_get_32();
	t64[1] = k_cycle_get_64();

	d32 = MIN(t32[1] - t32[0], t32[0] - t32[1]);
	d64 = MIN(t64[1] - t64[0], t64[1] - t64[0]);

	zassert_true(d64 >= d32,
		"k_cycle_get() (64-bit): d64: %" PRIu64 " < d32: %u", d64, d32);

	zassert_true(d64 < (d32 << 1),
		"k_cycle_get() (64-bit): d64: %" PRIu64 " >= 2 * d32: %u",
		d64, (d32 << 1));
}

/*
 *help function
 */
static void duration_expire(struct k_timer *timer)
{
	tdata.duration_count++;
}

static void stop_expire(struct k_timer *timer)
{
	tdata.stop_count++;
}

static void init_data_count(void)
{
	tdata.duration_count = 0;
	tdata.stop_count = 0;
}

/**
 * @brief Test millisecond time duration
 *
 * @details initialize a timer, then providing time duration in
 * millisecond, and check the duration time whether correct.
 *
 * @see k_timer_init(), k_timer_start(), k_timer_stop(),
 * k_busy_wait()
 *
 *
 */

ZTEST(clock, test_ms_time_duration)
{
	init_data_count();
	k_timer_start(&ktimer, K_MSEC(DURATION), K_NO_WAIT);

	/** TESTPOINT: waiting time less than duration and check the count*/
	k_busy_wait(LESS_DURATION * 1000);
	zassert_true(tdata.duration_count == 0);
	zassert_true(tdata.stop_count == 0);

	/** TESTPOINT: proving duration in millisecond */
	init_data_count();
	k_timer_start(&ktimer, K_MSEC(100), K_MSEC(50));

	/** TESTPOINT: waiting time more than duration and check the count */
	k_usleep(1);		/* align to tick */
	k_busy_wait((DURATION + 1) * 1000);
	zassert_true(tdata.duration_count == 1, "duration %u not 1",
		     tdata.duration_count);
	zassert_true(tdata.stop_count == 0,
		     "stop %u not 0", tdata.stop_count);

	/** cleanup environment */
	k_timer_stop(&ktimer);
}
/**
 * @}
 */
