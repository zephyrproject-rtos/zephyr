/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#define ALIGN_MS_BOUNDARY		       \
	do {				       \
		uint32_t t = k_uptime_get_32();   \
		while (t == k_uptime_get_32()) \
			Z_SPIN_DELAY(50);      \
	} while (0)

/** @cond INTERNAL_HIDDEN */
struct timer_data {
	int duration_count;
	int stop_count;
};
static void duration_expire(struct k_timer *timer);
static void stop_expire(struct k_timer *timer);

/* TESTPOINT: init timer via K_TIMER_DEFINE */
K_TIMER_DEFINE(ktimer, duration_expire, stop_expire);

static ZTEST_BMEM struct timer_data tdata;
/** @endcond */

#define DURATION 100
#define LESS_DURATION 70

/**
 * @defgroup kernel_clock_tests Clock Operations
 * @ingroup all_tests
 * @{
 * @}
 *
 * @addtogroup kernel_clock_tests
 * @{
 */

/**
 * @brief Verify the uptime clock advances monotonically and reports deltas.
 *
 * @ingroup kernel_clock_tests
 *
 * @details
 * Passing proves the system uptime clock is a forward-progressing time source
 * usable from user mode: both the 64-bit and lower-32-bit uptime readings
 * increase as wall time elapses, the 32-bit reading observes a millisecond
 * boundary crossing, and k_uptime_delta() reports a non-zero elapsed interval
 * between successive calls.
 *
 * Test steps:
 * - Spin until k_uptime_get() advances by at least 5 ms.
 * - Spin until k_uptime_get_32() advances by at least 5 ms.
 * - Sample the 32-bit uptime, align to a millisecond boundary, and confirm it grew.
 * - Seed a delta reference, then spin until k_uptime_delta() returns non-zero.
 *
 * Expected result:
 * - Uptime readings increase over time and k_uptime_delta() reports elapsed time.
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
		Z_SPIN_DELAY(50);
	}

	/**TESTPOINT: uptime elapse lower 32-bit*/
	t32 = k_uptime_get_32();
	while (k_uptime_get_32() < (t32 + 5)) {
		Z_SPIN_DELAY(50);
	}

	/**TESTPOINT: uptime straddled ms boundary*/
	t32 = k_uptime_get_32();
	ALIGN_MS_BOUNDARY;
	zassert_true(k_uptime_get_32() > t32);

	/**TESTPOINT: uptime delta*/
	d64 = k_uptime_delta(&d64);
	while (k_uptime_delta(&d64) == 0) {
		Z_SPIN_DELAY(50);
	}
}

/**
 * @brief Verify the 32-bit cycle counter advances consistently with uptime.
 *
 * @ingroup kernel_clock_tests
 *
 * @details
 * Passing proves that the architecture's 32-bit hardware cycle counter
 * (k_cycle_get_32()) is a monotonically increasing time source whose rate is
 * consistent with the tick-derived uptime clock (k_uptime_get_32()): the number
 * of cycles elapsed over one observed millisecond of uptime corresponds to at
 * least one millisecond worth of cycles and at least one millisecond when
 * converted to nanoseconds, cross-validating the cycle counter against uptime.
 *
 * Test steps:
 * - Align to a millisecond boundary and spin until the cycle counter advances,
 *   guarding against counter wrap-around.
 * - Sample the cycle counter, align to a millisecond boundary, and spin until
 *   the 32-bit uptime advances by one millisecond.
 * - Compute the cycle delta over that interval (skipping if the counter wrapped).
 *
 * Expected result:
 * - The cycle delta exceeds one millisecond of cycles and, converted via
 *   k_cyc_to_ns_floor64(), exceeds one millisecond in nanoseconds.
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
		Z_SPIN_DELAY(50);
	}

	/**TESTPOINT: cycle/uptime cross check*/
	c0 = k_cycle_get_32();
	ALIGN_MS_BOUNDARY;
	t32 = k_uptime_get_32();
	while (t32 == k_uptime_get_32()) {
		Z_SPIN_DELAY(50);
	}

	c1 = k_uptime_get_32();
	/*avoid cycle counter wrap around*/
	if (c1 > c0) {
		/* delta cycle should be greater than 1 milli-second*/
		zassert_true((c1 - c0) > (sys_clock_hw_cycles_per_sec() / MSEC_PER_SEC));
		/* delta NS should be greater than 1 milli-second */
		zassert_true((uint32_t)k_cyc_to_ns_floor64(c1 - c0) >
			     (NSEC_PER_SEC / MSEC_PER_SEC));
	}
}

/**
 * @brief Verify the 64-bit cycle counter tracks the 32-bit counter over an interval.
 *
 * @ingroup kernel_clock_tests
 *
 * @details
 * Passing proves that, on platforms providing a 64-bit cycle counter, the
 * 64-bit and 32-bit cycle counters measure the same elapsed time over a short
 * sleep: the 64-bit delta is at least as large as the 32-bit delta (it does not
 * lose resolution) yet remains below twice the 32-bit delta (it does not run at
 * a different rate). The test skips on platforms without a 64-bit cycle counter.
 *
 * Test steps:
 * - Skip unless CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER is enabled.
 * - Sample both the 64-bit and 32-bit cycle counters, sleep 1 ms, then sample again.
 * - Compute the 32-bit and 64-bit deltas across the sleep.
 *
 * Expected result:
 * - The 64-bit delta is >= the 32-bit delta and < twice the 32-bit delta.
 *
 * @see k_cycle_get_64()
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
 * @brief Verify a kernel timer expires only after its millisecond duration elapses.
 *
 * @ingroup kernel_clock_tests
 *
 * @details
 * Passing proves that k_timer_start() honors a millisecond expiry duration with
 * correct timing semantics: the timer's expiry callback does not fire while less
 * than the configured duration has elapsed, and fires exactly once once the full
 * duration is exceeded, while the stop callback is not invoked on normal expiry.
 *
 * Test steps:
 * - Start the timer for DURATION ms one-shot and busy-wait LESS_DURATION ms;
 *   confirm neither the expiry nor stop counter has incremented.
 * - Restart the timer, align to a tick, busy-wait just over DURATION ms, and
 *   confirm the expiry counter is exactly 1 and the stop counter is 0.
 * - Stop the timer to clean up.
 *
 * Expected result:
 * - No expiry before the duration elapses; exactly one expiry afterward and no
 *   stop-callback invocation.
 *
 * @see k_timer_start(), k_timer_stop(), k_busy_wait()
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

extern void *common_setup(void);
ZTEST_SUITE(clock, NULL, common_setup, NULL, NULL, NULL);
