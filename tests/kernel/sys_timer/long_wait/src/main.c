/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Tests for CONFIG_SYSTEM_CLOCK_LONG_WAIT: the 64-bit system-clock tick
 * interface that lets a capable timer driver honour a single wait longer than
 * the 32-bit tick range, instead of being forced into periodic intermediate
 * sys_clock_announce() wake-ups. The suite is only built for platforms whose
 * driver advertises the capability (see tests.yaml), so the feature is always
 * enabled here.
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/timer/system_timer.h>

/**
 * @brief Verify the widened system-clock tick interface.
 *
 * With CONFIG_SYSTEM_CLOCK_LONG_WAIT the driver-facing tick type and the
 * kernel's announce-range cap must both be 64-bit, otherwise a wait longer
 * than the 32-bit tick range would be truncated on its way to the driver.
 */
ZTEST(long_wait, test_type_widths)
{
	zassert_equal(sizeof(sys_clock_ticks_t), sizeof(uint64_t),
		      "sys_clock_ticks_t must be 64-bit under LONG_WAIT");

	/* The announce-range cap must exceed the 32-bit range it would have
	 * been clamped to without the feature; that headroom is the whole
	 * point of the option.
	 */
	zassert_true(SYS_CLOCK_MAX_WAIT > (sys_clock_ticks_t)UINT32_MAX,
		     "SYS_CLOCK_MAX_WAIT must exceed UINT32_MAX under LONG_WAIT");
}

static void far_timer_expiry(struct k_timer *t)
{
	ARG_UNUSED(t);
}

static K_TIMER_DEFINE(far_timer, far_timer_expiry, NULL);
static K_SEM_DEFINE(near_sem, 0, 1);

static void near_timer_expiry(struct k_timer *t)
{
	ARG_UNUSED(t);
	k_sem_give(&near_sem);
}

static K_TIMER_DEFINE(near_timer, near_timer_expiry, NULL);

/**
 * @brief A timeout beyond the 32-bit tick range must survive intact.
 *
 * Arm a timer whose deadline is farther out than UINT32_MAX ticks. Its
 * remaining time, read back through the (64-bit) tick accounting, must stay
 * above the 32-bit range -- proving the front end did not truncate the wait
 * into a spurious early expiry. A short timer armed alongside it must still
 * fire normally, confirming the near deadline is unaffected.
 */
ZTEST(long_wait, test_far_timeout)
{
	const k_ticks_t far_ticks = (k_ticks_t)UINT32_MAX + 1000000;

	k_timer_start(&far_timer, K_TICKS(far_ticks), K_NO_WAIT);
	k_timer_start(&near_timer, K_MSEC(20), K_NO_WAIT);

	/* The near timer must fire on time even though a far-future timeout
	 * is pending: it is the next expiry and must not be starved.
	 */
	zassert_ok(k_sem_take(&near_sem, K_MSEC(500)),
		   "near timer did not fire while a far timeout was pending");

	/* The far timer must still be counting down with more than a 32-bit
	 * range of ticks left, i.e. it was not truncated to a near deadline.
	 */
	k_ticks_t remaining = k_timer_remaining_ticks(&far_timer);

	zassert_true(remaining > (k_ticks_t)UINT32_MAX,
		     "far timeout was truncated: %lld ticks remaining", (long long)remaining);

	k_timer_stop(&far_timer);
}

/*
 * The two tests below drive sys_clock_announce() directly with synthetic
 * >32-bit tick counts. They are restricted two ways:
 *
 *  - UP only (!CONFIG_SMP). sys_clock_announce() is contractually an ISR-only
 *    operation: its firing loop ends in z_time_slice(), which on SMP asserts
 *    the caller is not CPU-mobile (i.e. in an ISR or with IRQs masked).
 *    Calling it from a plain thread on SMP trips that assert, and the loop may
 *    also be split across CPUs rather than running synchronously. On UP the
 *    wrapper runs the whole loop to completion synchronously here.
 *  - Not the timer-wheel backend. Most backends advance curr_tick by the
 *    announced delta in O(1), but the wheel owns its announce loop and walks
 *    it in bounded windows, i.e. O(announced ticks) -- billions of iterations
 *    for a >32-bit value, far too slow under QEMU. The wheel's own
 *    large-announce handling is exercised elsewhere.
 *
 * The other long-wait tests (type widths, far-timeout) still run under every
 * backend and on SMP.
 */
#if !defined(CONFIG_SMP) && !defined(CONFIG_TIMEOUT_BACKEND_WHEEL)

static atomic_t announce_fire_count;

static void announce_timer_expiry(struct k_timer *t)
{
	ARG_UNUSED(t);
	atomic_inc(&announce_fire_count);
}

static K_TIMER_DEFINE(announce_timer, announce_timer_expiry, NULL);

/**
 * @brief Drive sys_clock_announce() with a >32-bit tick count.
 *
 * Announce a number of elapsed ticks larger than the 32-bit range in a single
 * call and confirm the front end handles it without truncation:
 *   - a timer armed a few ticks out (i.e. within the announced window) fires
 *     exactly once, and
 *   - the system uptime tick count advances by at least the announced amount
 *     (a truncated announce would advance by less).
 *
 * This exercises the announce path directly, complementing the driver-level
 * far-timeout test above. This test is UP-only (see the #if above), so calling
 * the legacy sys_clock_announce() wrapper from thread context runs the whole
 * firing loop synchronously before returning.
 */
ZTEST(long_wait, test_announce)
{
	const sys_clock_ticks_t announce_ticks = (sys_clock_ticks_t)UINT32_MAX + 100000;
	const uint32_t near_ticks = 10;

	zassert_true(announce_ticks > (sys_clock_ticks_t)UINT32_MAX,
		     "test bug: announce value must exceed the 32-bit range");

	atomic_clear(&announce_fire_count);

	/* Arm a timer a few ticks out; it is the next expiry and must fire
	 * within the window we are about to announce.
	 */
	k_timer_start(&announce_timer, K_TICKS(near_ticks), K_NO_WAIT);

	int64_t tick_before = sys_clock_tick_get();

	/* Report the whole >32-bit elapsed-tick count in one announce. On this
	 * UP platform the firing loop runs to completion synchronously here.
	 */
	sys_clock_announce(announce_ticks);

	int64_t tick_after = sys_clock_tick_get();

	/* The near timer must have fired exactly once. */
	zassert_equal(atomic_get(&announce_fire_count), 1, "timer fired %ld times, expected 1",
		      (long)atomic_get(&announce_fire_count));

	/* The uptime tick count must have advanced by at least the full
	 * announced amount -- a 32-bit-truncated announce would instead advance
	 * by less than the value we passed. We only assert the lower bound: the
	 * real timer driver keeps running underneath and folds its own elapsed
	 * ticks into this synchronous announce, so curr_tick may legitimately
	 * advance somewhat further than the value we injected.
	 */
	int64_t delta = tick_after - tick_before;

	zassert_true(delta >= (int64_t)announce_ticks,
		     "tick count advanced by only %lld, expected >= %llu (truncated?)",
		     (long long)delta, (unsigned long long)announce_ticks);

	k_timer_stop(&announce_timer);
}

static atomic_t split_fire_count;

static void split_timer_expiry(struct k_timer *t)
{
	ARG_UNUSED(t);
	atomic_inc(&split_fire_count);
}

static K_TIMER_DEFINE(split_timer, split_timer_expiry, NULL);

/**
 * @brief A >32-bit timeout fires only once its deadline is actually crossed.
 *
 * Arm a timer past the 32-bit tick range, then announce elapsed time in two
 * steps:
 *   1. a >32-bit delta that is still short of the deadline -- the timer must
 *      NOT fire, and must still report ticks remaining, and
 *   2. a further delta that carries curr_tick past the deadline -- the timer
 *      must fire exactly once.
 *
 * A truncating announce path would either fire the timer early on step 1 (a
 * >32-bit delta wrapping to a small value that overshoots a truncated
 * deadline) or fail to fire it on step 2; either way the split announce pins
 * the behaviour down.
 */
ZTEST(long_wait, test_split_announce)
{
	const sys_clock_ticks_t far_ticks = (sys_clock_ticks_t)UINT32_MAX + 2000000;
	const sys_clock_ticks_t first_announce = (sys_clock_ticks_t)UINT32_MAX + 1000000;
	const sys_clock_ticks_t second_announce = 2000000;

	BUILD_ASSERT(first_announce > (sys_clock_ticks_t)UINT32_MAX,
		     "first announce must exceed the 32-bit range");
	BUILD_ASSERT(first_announce < far_ticks, "first announce must fall short of the deadline");

	atomic_clear(&split_fire_count);

	k_timer_start(&split_timer, K_TICKS(far_ticks), K_NO_WAIT);

	/* Step 1: announce a >32-bit delta that is still short of the timeout.
	 * The deadline has not been reached, so the timer must not fire.
	 */
	sys_clock_announce(first_announce);
	zassert_equal(atomic_get(&split_fire_count), 0,
		      "timer fired early after announcing %llu of %llu ticks",
		      (unsigned long long)first_announce, (unsigned long long)far_ticks);
	zassert_true(k_timer_remaining_ticks(&split_timer) > 0,
		     "timer that has not reached its deadline reports no time left");

	/* Step 2: announce enough further ticks to carry curr_tick past the
	 * deadline (comfortably more than the ~1000000 that remain). The timer
	 * must now fire exactly once.
	 */
	sys_clock_announce(second_announce);
	zassert_equal(atomic_get(&split_fire_count), 1,
		      "timer did not fire once after its deadline was crossed (fired %ld)",
		      (long)atomic_get(&split_fire_count));

	k_timer_stop(&split_timer);
}
#endif /* !CONFIG_SMP && !CONFIG_TIMEOUT_BACKEND_WHEEL */

ZTEST_SUITE(long_wait, NULL, NULL, NULL, NULL, NULL);
