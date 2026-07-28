/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/clock.h>
#include <zephyr/sys/timeutil.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>

#define PERIOD_TICKS   50
#define DURATION_TICKS 100
#define GRID_SAMPLES   8

/* in-handler expiry timestamps: written in timeout-handler context, read by the tests */
static ZTEST_BMEM uint64_t expiry_stamp[GRID_SAMPLES + 2];
static ZTEST_BMEM uint32_t expiry_count;

static void stamp_expiry(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	if (expiry_count < ARRAY_SIZE(expiry_stamp)) {
		expiry_stamp[expiry_count] = (uint64_t)k_uptime_ticks();
	}
	expiry_count++;
}

K_TIMER_DEFINE(stamped_timer, stamp_expiry, NULL);
K_TIMER_DEFINE_EXT(mono_timer, NULL, NULL, SYS_CLOCK_MONOTONIC);
K_TIMER_DEFINE_EXT(rt_timer, NULL, NULL, SYS_CLOCK_REALTIME);

static void ts_from_ticks(uint64_t ticks, struct timespec *ts)
{
	timespec_from_timeout(K_TICKS((k_ticks_t)ticks), ts);
}

static uint64_t ts_to_ticks(const struct timespec *ts)
{
	return k_sec_to_ticks_ceil64((uint64_t)ts->tv_sec) + k_ns_to_ticks_ceil64(ts->tv_nsec);
}

static int settime_ticks(struct k_timer *timer, uint32_t flags, uint64_t value_ticks,
			 uint64_t interval_ticks)
{
	struct timespec value;
	struct timespec interval;

	ts_from_ticks(value_ticks, &value);
	ts_from_ticks(interval_ticks, &interval);

	return k_timer_settime(timer, flags, &value, &interval, NULL, NULL);
}

static void relative_at_least(void)
{
	uint64_t before = (uint64_t)k_uptime_ticks();

	zassert_ok(settime_ticks(&stamped_timer, 0, DURATION_TICKS, 0));
	zassert_true(k_timer_status_sync(&stamped_timer) > 0);

	uint64_t elapsed = expiry_stamp[0] - before;

	zassert_true(elapsed >= DURATION_TICKS,
		     "relative arming expired early: %llu < %d ticks",
		     (unsigned long long)elapsed, DURATION_TICKS);
	zassert_true(elapsed <= DURATION_TICKS + 2, "relative arming expired unexpectedly late");
}

static void abs_monotonic_exact(void)
{
	uint64_t deadline = (uint64_t)k_uptime_ticks() + DURATION_TICKS;

	zassert_ok(settime_ticks(&stamped_timer, SYS_TIMER_ABSTIME, deadline, 0));
	zassert_true(k_timer_status_sync(&stamped_timer) > 0);

	zassert_equal(expiry_stamp[0], deadline,
		      "absolute deadline missed: expired at %llu, wanted %llu",
		      (unsigned long long)expiry_stamp[0], (unsigned long long)deadline);
}

static void abs_past_fires_immediately(void)
{
	/* the monotonic epoch (uptime 0) is always in the past */
	zassert_ok(settime_ticks(&mono_timer, SYS_TIMER_ABSTIME, 1, 0));
	zassert_true(k_timer_status_sync(&mono_timer) > 0);
	zassert_equal(k_timer_gettime(&mono_timer, NULL, NULL), 0,
		      "expired one-shot still reports armed");
}

static void periodic_no_drift(void)
{
	zassert_ok(settime_ticks(&stamped_timer, 0, PERIOD_TICKS, PERIOD_TICKS));

	while (expiry_count < GRID_SAMPLES) {
		(void)k_timer_status_sync(&stamped_timer);
	}
	k_timer_stop(&stamped_timer);

	for (uint32_t i = 1; i < GRID_SAMPLES; i++) {
		zassert_equal(expiry_stamp[i], expiry_stamp[0] + (uint64_t)i * PERIOD_TICKS,
			      "expiry %u off-grid: %llu != %llu", i,
			      (unsigned long long)expiry_stamp[i],
			      (unsigned long long)(expiry_stamp[0] + (uint64_t)i * PERIOD_TICKS));
	}
}

static void periodic_missed_periods_keep_grid(void)
{
	zassert_ok(settime_ticks(&stamped_timer, 0, PERIOD_TICKS, PERIOD_TICKS));

	/* let the first expiry land to anchor the grid */
	(void)k_timer_status_sync(&stamped_timer);

	/*
	 * Block for more than two periods with a busy-wait; expiries during the window
	 * may coalesce, but the grid anchored at the first expiry must be preserved.
	 */
	k_busy_wait(k_ticks_to_us_ceil32(5 * PERIOD_TICKS / 2));

	uint32_t caught_up = expiry_count;

	while (expiry_count < caught_up + 2) {
		(void)k_timer_status_sync(&stamped_timer);
	}
	k_timer_stop(&stamped_timer);

	for (uint32_t i = 1; i < MIN(expiry_count, (uint32_t)ARRAY_SIZE(expiry_stamp)); i++) {
		zassert_equal((expiry_stamp[i] - expiry_stamp[0]) % PERIOD_TICKS, 0,
			      "post-gap expiry %u drifted off the original grid", i);
	}
}

static void gettime_consistency(void)
{
	struct timespec rem;
	struct timespec per;

	zassert_ok(settime_ticks(&mono_timer, 0, DURATION_TICKS, PERIOD_TICKS));

	zassert_equal(k_timer_gettime(&mono_timer, &rem, &per), 1, "armed timer reports disarmed");
	zassert_true((rem.tv_sec > 0) || (rem.tv_nsec > 0), "armed timer reports zero remaining");

	struct timespec expect_per;

	ts_from_ticks(PERIOD_TICKS, &expect_per);
	zassert_true(timespec_equal(&per, &expect_per), "reported period differs from the one set");

	k_timer_stop(&mono_timer);
	zassert_equal(k_timer_gettime(&mono_timer, &rem, &per), 0);
	zassert_true((rem.tv_sec == 0) && (rem.tv_nsec == 0),
		     "disarmed timer reports non-zero remaining");
}

static void settime_reports_old_state(void)
{
	struct timespec ov;
	struct timespec oi;
	struct timespec value;
	struct timespec zero = {0, 0};

	zassert_ok(settime_ticks(&mono_timer, 0, DURATION_TICKS, PERIOD_TICKS));

	/* disarm, capturing the previous state */
	ts_from_ticks(0, &value);
	zassert_ok(k_timer_settime(&mono_timer, 0, &value, NULL, &ov, &oi));

	zassert_true((ov.tv_sec > 0) || (ov.tv_nsec > 0), "old remaining not reported");

	struct timespec expect_per;

	ts_from_ticks(PERIOD_TICKS, &expect_per);
	zassert_true(timespec_equal(&oi, &expect_per), "old period not reported");
	zassert_equal(k_timer_gettime(&mono_timer, NULL, NULL), 0, "zero it_value did not disarm");

	/* a disarmed timer reports zero old state */
	zassert_ok(k_timer_settime(&mono_timer, 0, &zero, NULL, &ov, NULL));
	zassert_true((ov.tv_sec == 0) && (ov.tv_nsec == 0));
}

static void settime_invalid(void)
{
	struct timespec bad_nsec = {.tv_sec = 0, .tv_nsec = NSEC_PER_SEC};
	struct timespec neg = {.tv_sec = -1, .tv_nsec = 0};
	struct timespec ok = {.tv_sec = 0, .tv_nsec = 1000};

	zassert_equal(k_timer_settime(&mono_timer, 0, NULL, NULL, NULL, NULL), -EINVAL);
	zassert_equal(k_timer_settime(&mono_timer, 0, &bad_nsec, NULL, NULL, NULL), -EINVAL);
	zassert_equal(k_timer_settime(&mono_timer, 0, &neg, NULL, NULL, NULL), -EINVAL);
	zassert_equal(k_timer_settime(&mono_timer, 0, &ok, &neg, NULL, NULL), -EINVAL);
	zassert_equal(k_timer_settime(&mono_timer, 0xffffffffU, &ok, NULL, NULL, NULL), -EINVAL);
	if (k_is_user_context()) {
		/* a handle that is not a live timer of the caller's is an invalid id, not a fault */
		zassert_equal(k_timer_settime((struct k_timer *)&ok, 0, &ok, NULL, NULL, NULL),
			      -EINVAL);
	}
}

static int rt_deadline_in(int64_t sec_from_now, struct timespec *deadline)
{
	int ret;

	ret = sys_clock_gettime(SYS_CLOCK_REALTIME, deadline);
	if (ret == 0) {
		deadline->tv_sec += sec_from_now;
	}

	return ret;
}

static void realtime_abs_basic(void)
{
	struct timespec deadline;

	zassert_ok(rt_deadline_in(0, &deadline));
	/* ~2 ticks out, expressed in realtime */
	uint64_t two_ticks_ns = (uint64_t)k_ticks_to_ns_ceil64(2);

	deadline.tv_nsec += (long)two_ticks_ns;
	while (deadline.tv_nsec >= (long)NSEC_PER_SEC) {
		deadline.tv_nsec -= (long)NSEC_PER_SEC;
		deadline.tv_sec++;
	}

	zassert_ok(k_timer_settime(&rt_timer, SYS_TIMER_ABSTIME, &deadline, NULL, NULL, NULL));
	zassert_true(k_timer_status_sync(&rt_timer) > 0, "realtime absolute timer did not fire");
}

static void realtime_settime_forward_fires(void)
{
	struct timespec deadline;
	struct timespec jump;

	/* deadline one hour out on the realtime clock */
	zassert_ok(rt_deadline_in(3600, &deadline));
	zassert_ok(k_timer_settime(&rt_timer, SYS_TIMER_ABSTIME, &deadline, NULL, NULL, NULL));

	/* jump the wall clock past the deadline: the timer must fire promptly */
	jump = deadline;
	jump.tv_sec += 1;
	zassert_ok(sys_clock_settime(SYS_CLOCK_REALTIME, &jump));

	zassert_true(k_timer_status_sync(&rt_timer) > 0,
		     "clock_settime() past the deadline did not fire the timer");
}

static void realtime_settime_backward_defers(void)
{
	struct timespec now;
	struct timespec deadline;
	struct timespec back;
	struct timespec rem;

	zassert_ok(sys_clock_gettime(SYS_CLOCK_REALTIME, &now));

	deadline = now;
	deadline.tv_sec += 2;
	zassert_ok(k_timer_settime(&rt_timer, SYS_TIMER_ABSTIME, &deadline, NULL, NULL, NULL));

	/* move the wall clock back one hour: the deadline is now ~1h2s away */
	back = now;
	back.tv_sec -= 3600;
	zassert_ok(sys_clock_settime(SYS_CLOCK_REALTIME, &back));

	zassert_equal(k_timer_gettime(&rt_timer, &rem, NULL), 1);
	zassert_true(rem.tv_sec > 3000, "backward clock jump did not defer the deadline (rem %lld)",
		     (long long)rem.tv_sec);

	k_timer_stop(&rt_timer);
}

static void realtime_periodic_first_expiry_only(void)
{
	struct timespec deadline;
	struct timespec interval;
	struct timespec rem;

	zassert_ok(rt_deadline_in(0, &deadline));
	deadline.tv_nsec += (long)k_ticks_to_ns_ceil64(2);
	while (deadline.tv_nsec >= (long)NSEC_PER_SEC) {
		deadline.tv_nsec -= (long)NSEC_PER_SEC;
		deadline.tv_sec++;
	}
	ts_from_ticks(PERIOD_TICKS, &interval);

	zassert_ok(k_timer_settime(&rt_timer, SYS_TIMER_ABSTIME, &deadline, &interval, NULL,
				   NULL));
	zassert_true(k_timer_status_sync(&rt_timer) > 0);

	/* after the first expiry, a big forward wall-clock jump must not disturb the period */
	struct timespec jump;

	zassert_ok(rt_deadline_in(7200, &jump));
	zassert_ok(sys_clock_settime(SYS_CLOCK_REALTIME, &jump));

	zassert_equal(k_timer_gettime(&rt_timer, &rem, NULL), 1);
	zassert_true(ts_to_ticks(&rem) <= PERIOD_TICKS + 1,
		     "post-first-expiry period was disturbed by clock_settime()");

	k_timer_stop(&rt_timer);
}

static void stop_deregisters_realtime(void)
{
	struct timespec deadline;

	zassert_ok(rt_deadline_in(3600, &deadline));
	zassert_ok(k_timer_settime(&rt_timer, SYS_TIMER_ABSTIME, &deadline, NULL, NULL, NULL));
	k_timer_stop(&rt_timer);

	/* jumping the clock past the (stopped) deadline must not re-arm the timer */
	deadline.tv_sec += 2;
	zassert_ok(sys_clock_settime(SYS_CLOCK_REALTIME, &deadline));

	k_sleep(K_TICKS(4));
	zassert_equal(k_timer_gettime(&rt_timer, NULL, NULL), 0,
		      "stopped realtime timer was re-armed by clock_settime()");
}

static void legacy_start_unaffected(void)
{
	/* exact mode first ... */
	zassert_ok(settime_ticks(&stamped_timer, 0, DURATION_TICKS, 0));
	k_timer_stop(&stamped_timer);

	/* ... then legacy start/status_get still behave as always */
	k_timer_start(&stamped_timer, K_TICKS(PERIOD_TICKS), K_TICKS(PERIOD_TICKS));
	zassert_true(k_timer_status_sync(&stamped_timer) > 0);
	k_timer_stop(&stamped_timer);
}

#ifdef CONFIG_USERSPACE
/* fault-harness exception to one-ZTEST-per-unit: this test point must oops */
ZTEST_USER(timer_clock, test_k_timer_settime_fault)
{
	/* an argument buffer the caller cannot read must fault, not corrupt */
	ztest_set_fault_valid(true);
	(void)k_timer_settime(&mono_timer, 0, (const struct timespec *)&mono_timer, NULL, NULL,
			      NULL);
	ztest_test_fail(); /* unreachable: the syscall must oops */
}
#endif /* CONFIG_USERSPACE */

/* the per-test cleanup, applied between the sections of a merged per-unit test */
static void section_reset(void)
{
	k_timer_stop(&stamped_timer);
	k_timer_stop(&mono_timer);
	k_timer_stop(&rt_timer);
	expiry_count = 0;
	(void)memset(expiry_stamp, 0, sizeof(expiry_stamp));
}

ZTEST_USER(timer_clock, test_k_timer_settime)
{
	relative_at_least();
	section_reset();
	abs_monotonic_exact();
	section_reset();
	abs_past_fires_immediately();
	section_reset();
	periodic_no_drift();
	section_reset();
	periodic_missed_periods_keep_grid();
	section_reset();
	settime_reports_old_state();
	section_reset();
	settime_invalid();
	section_reset();
	realtime_abs_basic();
	section_reset();
}

ZTEST_USER(timer_clock, test_k_timer_gettime)
{
	gettime_consistency();
	section_reset();
}

ZTEST_USER(timer_clock, test_sys_clock_settime)
{
	realtime_settime_forward_fires();
	section_reset();
	realtime_settime_backward_defers();
	section_reset();
	realtime_periodic_first_expiry_only();
	section_reset();
	stop_deregisters_realtime();
	section_reset();
}

ZTEST_USER(timer_clock, test_k_timer_start)
{
	legacy_start_unaffected();
	section_reset();
}

static void reset_state(void *arg)
{
	ARG_UNUSED(arg);

	k_timer_stop(&stamped_timer);
	k_timer_stop(&mono_timer);
	k_timer_stop(&rt_timer);
	expiry_count = 0;
	(void)memset(expiry_stamp, 0, sizeof(expiry_stamp));
}

static void *suite_setup(void)
{
#ifdef CONFIG_USERSPACE
	k_thread_access_grant(k_current_get(), &stamped_timer, &mono_timer, &rt_timer);
#endif
	return NULL;
}

ZTEST_SUITE(timer_clock, NULL, suite_setup, reset_state, reset_state, NULL);
