/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/clock.h>
#include <zephyr/sys/timer.h>
#include <zephyr/sys/timeutil.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>

#define ARM_TICKS 50

static void ts_from_ticks(uint64_t ticks, struct timespec *ts)
{
	timespec_from_timeout(K_TICKS((k_ticks_t)ticks), ts);
}

static void create_invalid(void)
{
	struct k_timer *t;
	struct k_timer_notify notify = {0};

	if (!k_is_user_context()) {
		/* in user mode a NULL output pointer faults in the syscall layer instead */
		zassert_equal(sys_timer_create(SYS_CLOCK_MONOTONIC, NULL, 0, NULL, NULL, NULL),
			      -EINVAL);
	}
	zassert_equal(sys_timer_create(0x7f, NULL, 0, NULL, NULL, &t), -EINVAL);
	/*
	 * Signal-based notification requires CONFIG_TIMER_SIGNAL. Kernel mode only: in
	 * user mode the syscall layer faults on the NULL target before the -ENOTSUP.
	 */
	if (!IS_ENABLED(CONFIG_TIMER_SIGNAL) && !k_is_user_context()) {
		zassert_equal(sys_timer_create(SYS_CLOCK_MONOTONIC, &notify, 0, NULL, NULL, &t),
			      -ENOTSUP);
	}
}

static void create_disarmed_delete(void)
{
	struct k_timer *t;
	struct timespec rem;

	zassert_ok(sys_timer_create(SYS_CLOCK_MONOTONIC, NULL, 0, NULL, NULL, &t));
	zassert_equal(k_timer_gettime(t, &rem, NULL), 0, "created timer is not disarmed");
	zassert_true((rem.tv_sec == 0) && (rem.tv_nsec == 0));
	zassert_ok(sys_timer_delete(t));
}

static void create_armed_fires(void)
{
	struct k_timer *t;
	struct timespec value;

	ts_from_ticks(ARM_TICKS, &value);

	zassert_ok(sys_timer_create(SYS_CLOCK_MONOTONIC, NULL, 0, &value, NULL, &t));
	zassert_equal(k_timer_gettime(t, NULL, NULL), 1, "arm-at-create timer is not armed");
	zassert_true(k_timer_status_sync(t) > 0, "arm-at-create timer did not fire");
	zassert_equal(k_timer_gettime(t, NULL, NULL), 0, "expired one-shot still armed");
	zassert_ok(sys_timer_delete(t));
}

static void delete_while_armed(void)
{
	struct k_timer *t;
	struct timespec value;
	struct timespec interval;

	ts_from_ticks(ARM_TICKS, &value);
	ts_from_ticks(ARM_TICKS, &interval);

	zassert_ok(sys_timer_create(SYS_CLOCK_REALTIME, NULL, 0, &value, &interval, &t));
	zassert_true(k_timer_status_sync(t) > 0);
	zassert_ok(sys_timer_delete(t), "deleting an armed periodic timer failed");
}

static void min_guaranteed(void)
{
	struct k_timer *timers[2];

	/* this test contributes SYS_TIMER_MIN_ADD_TEST=2 guaranteed (static) timers */
	for (size_t i = 0; i < ARRAY_SIZE(timers); ++i) {
		zassert_ok(sys_timer_create(SYS_CLOCK_MONOTONIC, NULL, 0, NULL, NULL, &timers[i]),
			   "guaranteed allocation %zu failed", i);
	}
	for (size_t i = 0; i < ARRAY_SIZE(timers); ++i) {
		zassert_ok(sys_timer_delete(timers[i]));
	}
}

static void exhaustion_eagain(void)
{
	struct k_timer *timers[16];
	struct k_timer *extra;
	const size_t limit = MIN((size_t)CONFIG_SYS_TIMER_MAX, ARRAY_SIZE(timers));
	size_t n;

	for (n = 0; n < limit; ++n) {
		zassert_ok(sys_timer_create(SYS_CLOCK_MONOTONIC, NULL, 0, NULL, NULL, &timers[n]),
			   "allocation %zu of %d failed", n, CONFIG_SYS_TIMER_MAX);
	}

	zassert_equal(sys_timer_create(SYS_CLOCK_MONOTONIC, NULL, 0, NULL, NULL, &extra), -EAGAIN,
		      "exhausted pool did not report -EAGAIN");

	/* releasing one slot makes creation possible again */
	zassert_ok(sys_timer_delete(timers[0]));
	zassert_ok(sys_timer_create(SYS_CLOCK_MONOTONIC, NULL, 0, NULL, NULL, &timers[0]));

	for (n = 0; n < limit; ++n) {
		zassert_ok(sys_timer_delete(timers[n]));
	}
}

static void delete_stale(void)
{
	struct k_timer not_from_pool;
	struct k_timer *t;

	k_timer_init(&not_from_pool, NULL, NULL);
	zassert_equal(sys_timer_delete(&not_from_pool), -EINVAL,
		      "deleting a non-pool timer did not fail");

	zassert_ok(sys_timer_create(SYS_CLOCK_MONOTONIC, NULL, 0, NULL, NULL, &t));
	zassert_ok(sys_timer_delete(t));
	zassert_equal(sys_timer_delete(t), -EINVAL, "double delete did not fail");
}

ZTEST_USER(sys_timer, test_sys_timer_create)
{
	create_invalid();
	create_armed_fires();
	min_guaranteed();
	if (CONFIG_SYS_TIMER_MAX <= 16) {
		/* only meaningful with a small, bounded pool (static_only variant) */
		exhaustion_eagain();
	}
}

static void delete_revokes_access(void)
{
	struct k_timer *t;

	zassert_ok(sys_timer_create(SYS_CLOCK_MONOTONIC, NULL, 0, NULL, NULL, &t));
	zassert_ok(sys_timer_delete(t));
	/* no longer a live timer of the caller's: an invalid id, not a fault */
	zassert_equal(k_timer_gettime(t, NULL, NULL), -EINVAL);
}

ZTEST_USER(sys_timer, test_sys_timer_delete)
{
	create_disarmed_delete();
	delete_while_armed();

	if (k_is_user_context()) {
		delete_revokes_access();
	} else {
		/* a stale handle given to sys_timer_delete() faults in the syscall layer instead */
		delete_stale();
	}
}

#ifdef CONFIG_USERSPACE
/* fault-harness exception to one-ZTEST-per-unit: this test point must oops */
ZTEST_USER(sys_timer, test_sys_timer_delete_fault)
{
	struct k_timer *t;

	zassert_ok(sys_timer_create(SYS_CLOCK_MONOTONIC, NULL, 0, NULL, NULL, &t));
	zassert_ok(sys_timer_delete(t));

	/* access was revoked at delete: a second delete is not the caller's object and must fault */
	ztest_set_fault_valid(true);
	(void)sys_timer_delete(t);
	ztest_test_fail(); /* unreachable: the syscall must oops */
}
#endif /* CONFIG_USERSPACE */

ZTEST_SUITE(sys_timer, NULL, NULL, NULL, NULL, NULL);
