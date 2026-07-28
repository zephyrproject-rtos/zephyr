/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <kernel_internal.h>
#include <zephyr/kernel/signal.h>
#include <zephyr/sys/clock.h>
#include <zephyr/sys/timer.h>
#include <zephyr/sys/timeutil.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>

#define PERIOD_TICKS 10
#define VAL_COOKIE   0x5eaf00d

static void ts_from_ticks(uint64_t ticks, struct timespec *ts)
{
	timespec_from_timeout(K_TICKS((k_ticks_t)ticks), ts);
}

static int arm_ticks(struct k_timer *t, uint64_t value_ticks, uint64_t interval_ticks)
{
	struct timespec value;
	struct timespec interval;

	ts_from_ticks(value_ticks, &value);
	ts_from_ticks(interval_ticks, &interval);

	return k_timer_settime(t, 0, &value, &interval, NULL, NULL);
}

/* create a self-targeted signal timer */
static struct k_timer *create_sig_timer(int signo)
{
	struct k_timer *t = NULL;
	struct k_timer_notify notify = {
		.target = (k_pid_t)k_current_get(),
		.signo = signo,
		.value.sival_int = VAL_COOKIE,
	};

	zassert_ok(sys_timer_create(SYS_CLOCK_MONOTONIC, &notify, 0, NULL, NULL, &t));

	return t;
}

static void mask_signo(int signo, bool block)
{
	struct k_sig_set set;

	zassert_ok(k_sig_emptyset(&set));
	zassert_ok(k_sig_addset(&set, signo));
	zassert_ok(k_sig_mask(block ? K_SIG_BLOCK : K_SIG_UNBLOCK, &set, NULL));
}

static int wait_signo(int signo, struct k_sig_info *info, k_timeout_t timeout)
{
	struct k_sig_set set;

	zassert_ok(k_sig_emptyset(&set));
	zassert_ok(k_sig_addset(&set, signo));

	return k_sig_timedwait(&set, info, timeout);
}

static void drain_signo(int signo)
{
	while (wait_signo(signo, NULL, K_NO_WAIT) >= 0) {
	}
}

static void delivery_timedwait(void)
{
	struct k_sig_info info = {0};
	struct k_timer *t = create_sig_timer(K_SIG_USR1);

	zassert_ok(arm_ticks(t, PERIOD_TICKS, 0));
	zassert_equal(wait_signo(K_SIG_USR1, &info, K_TICKS(10 * PERIOD_TICKS)), K_SIG_USR1,
		      "expiry signal was not delivered");
	zassert_equal(info.code, K_SI_TIMER, "si_code is %d, not K_SI_TIMER", info.code);
	zassert_equal(info.value.sival_int, VAL_COOKIE, "signal value did not round-trip");

	zassert_ok(sys_timer_delete(t));
}

ZTEST_BMEM static volatile int handler_code;
ZTEST_BMEM static volatile int handler_hits;

static void timer_sig_handler(int sig, struct k_sig_info *info, void *ucontext)
{
	ARG_UNUSED(sig);
	ARG_UNUSED(ucontext);

	handler_code = info->code;
	handler_hits++;
}

static void delivery_handler(void)
{
	struct k_sig_action action = {
		.handler = timer_sig_handler,
	};
	struct k_timer *t = create_sig_timer(K_SIG_USR2);

	handler_code = -1;
	handler_hits = 0;

	/* kernel threads must explicitly unmask signals they wish to receive */
	mask_signo(K_SIG_USR2, false);
	zassert_ok(k_sig_action(K_SIG_USR2, &action, NULL));

	zassert_ok(arm_ticks(t, PERIOD_TICKS, 0));

	for (int i = 0; (handler_hits == 0) && (i < 20 * PERIOD_TICKS); ++i) {
		k_sleep(K_TICKS(1));
	}

	zassert_equal(handler_hits, 1, "handler ran %d times", handler_hits);
	zassert_equal(handler_code, K_SI_TIMER);

	action.handler = K_SIG_DFL;
	zassert_ok(k_sig_action(K_SIG_USR2, &action, NULL));
	mask_signo(K_SIG_USR2, true);
	zassert_ok(sys_timer_delete(t));
}

static void one_pending_and_overrun(void)
{
	struct k_timer *t = create_sig_timer(K_SIG_USR1);

	zassert_ok(arm_ticks(t, PERIOD_TICKS, PERIOD_TICKS));

	/* let several periods elapse while the signal stays queued (masked) */
	k_sleep(K_TICKS(PERIOD_TICKS * 5 + PERIOD_TICKS / 2));

	/* one-pending: exactly one instance is queued, no matter how many expiries */
	zassert_equal(wait_signo(K_SIG_USR1, NULL, K_NO_WAIT), K_SIG_USR1);

	int overrun = k_timer_overrun(t);

	zassert_true((overrun >= 3) && (overrun <= 5),
		     "overrun %d outside the expected 3..5 window", overrun);
	/* non-destructive: repeated reads return the latched value */
	zassert_equal(k_timer_overrun(t), overrun);
	zassert_equal(k_timer_overrun(t), overrun);

	zassert_equal(wait_signo(K_SIG_USR1, NULL, K_NO_WAIT), -EAGAIN,
		      "more than one signal instance was queued for one timer");

	zassert_ok(sys_timer_delete(t));
}

static void overrun_oneshot_zero(void)
{
	struct k_timer *t = create_sig_timer(K_SIG_USR1);

	zassert_ok(arm_ticks(t, PERIOD_TICKS, 0));
	zassert_equal(wait_signo(K_SIG_USR1, NULL, K_TICKS(10 * PERIOD_TICKS)), K_SIG_USR1);
	zassert_equal(k_timer_overrun(t), 0, "one-shot delivered promptly has overrun");

	zassert_ok(sys_timer_delete(t));
}

static void overrun_counts_coalesced_gap(void)
{
	struct k_timer *t = create_sig_timer(K_SIG_USR1);

	zassert_ok(arm_ticks(t, PERIOD_TICKS, PERIOD_TICKS));

	/* first expiry generates the pending signal */
	k_sleep(K_TICKS(PERIOD_TICKS + 1));

	/*
	 * Busy-block for several periods: expiry handling may coalesce into fewer
	 * handler invocations, but the grid-based overrun must count all of them.
	 */
	k_busy_wait(k_ticks_to_us_ceil32(3 * PERIOD_TICKS));

	zassert_equal(wait_signo(K_SIG_USR1, NULL, K_NO_WAIT), K_SIG_USR1);
	zassert_true(k_timer_overrun(t) >= 2, "coalesced expiries were not counted: overrun %d",
		     k_timer_overrun(t));

	zassert_ok(sys_timer_delete(t));
}

static void disarm_freezes_overrun(void)
{
	struct timespec zero = {0, 0};
	struct k_timer *t = create_sig_timer(K_SIG_USR1);

	zassert_ok(arm_ticks(t, PERIOD_TICKS, PERIOD_TICKS));

	/* accumulate roughly two extra expiries, then disarm */
	k_sleep(K_TICKS(3 * PERIOD_TICKS + PERIOD_TICKS / 2));
	zassert_ok(k_timer_settime(t, 0, &zero, NULL, NULL, NULL));

	/* time passing after disarm must not inflate the overrun */
	k_sleep(K_TICKS(5 * PERIOD_TICKS));

	zassert_equal(wait_signo(K_SIG_USR1, NULL, K_NO_WAIT), K_SIG_USR1);
	zassert_true(k_timer_overrun(t) <= 3,
		     "overrun %d kept accumulating after disarm", k_timer_overrun(t));

	zassert_ok(sys_timer_delete(t));
}

struct helper_result {
	int signo;
	int code;
};

static K_THREAD_STACK_DEFINE(helper_stack, 2048);
static struct k_thread helper_thread;
ZTEST_BMEM static struct helper_result helper_res;

static void helper_entry(void *a, void *b, void *c)
{
	struct k_sig_set set;
	struct k_sig_info info = {0};

	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	(void)k_sig_emptyset(&set);
	(void)k_sig_addset(&set, K_SIG_USR2);
	(void)k_sig_mask(K_SIG_BLOCK, &set, NULL);

	/* K_FOREVER: an unbounded sigwait must block until the timer signal arrives */
	helper_res.signo = k_sig_timedwait(&set, &info, K_FOREVER);
	helper_res.code = info.code;
}

static void thread_targeted_delivery(void)
{
	struct k_timer *t = NULL;
	struct k_timer_notify notify = {
		.signo = K_SIG_USR2,
		.value.sival_int = VAL_COOKIE,
	};

	helper_res.signo = -1;
	helper_res.code = -1;

	notify.target = (k_pid_t)k_thread_create(
		&helper_thread, helper_stack, K_THREAD_STACK_SIZEOF(helper_stack), helper_entry,
		NULL, NULL, NULL, K_LOWEST_APPLICATION_THREAD_PRIO,
		IS_ENABLED(CONFIG_USERSPACE) ? (K_USER | K_INHERIT_PERMS) : 0, K_NO_WAIT);

	/* let the helper block in k_sig_timedwait() first */
	k_sleep(K_TICKS(2));

	zassert_ok(sys_timer_create(SYS_CLOCK_MONOTONIC, &notify, 0, NULL, NULL, &t));
	zassert_ok(arm_ticks(t, PERIOD_TICKS, 0));

	zassert_ok(k_thread_join(&helper_thread, K_TICKS(200 * PERIOD_TICKS)));
	zassert_equal(helper_res.signo, K_SIG_USR2, "helper did not receive the timer signal");
	zassert_equal(helper_res.code, K_SI_TIMER);

	zassert_ok(sys_timer_delete(t));
}

static struct k_sem never_sem;

static void block_after_handler_delivery(void)
{
	int ret;
	struct k_sig_action action = {
		.handler = timer_sig_handler,
	};
	struct k_timer *t = create_sig_timer(K_SIG_USR2);

	handler_hits = 0;

	/* deliver at least one signal through the handler path while sleeping */
	mask_signo(K_SIG_USR2, false);
	zassert_ok(k_sig_action(K_SIG_USR2, &action, NULL));
	zassert_ok(arm_ticks(t, PERIOD_TICKS, PERIOD_TICKS));
	for (int i = 0; (handler_hits == 0) && (i < 20 * PERIOD_TICKS); ++i) {
		k_sleep(K_TICKS(1));
	}
	zassert_true(handler_hits >= 1);

	zassert_ok(sys_timer_delete(t));
	action.handler = K_SIG_DFL;
	zassert_ok(k_sig_action(K_SIG_USR2, &action, NULL));
	mask_signo(K_SIG_USR2, true);

	/*
	 * Regression: handler delivery must not damage this thread's context - a
	 * subsequent bounded block must still time out normally.
	 */
	k_sem_init(&never_sem, 0, 1);
	ret = k_sem_take(&never_sem, K_TICKS(5));
	zassert_equal(ret, -EAGAIN, "post-delivery block wedged (ret %d)", ret);
}

static void pending_signal_while_blocked(void)
{
	int ret;
	struct k_sig_action action = {
		.handler = timer_sig_handler,
	};
	struct k_timer *t = create_sig_timer(K_SIG_USR2);

	handler_hits = 0;

	/* handler-based delivery, unmasked, periodic timer left running */
	mask_signo(K_SIG_USR2, false);
	zassert_ok(k_sig_action(K_SIG_USR2, &action, NULL));
	zassert_ok(arm_ticks(t, PERIOD_TICKS, PERIOD_TICKS));

	/*
	 * Block on an object that is never given: expiry signals target this thread
	 * while it waits. The wait must still time out normally (and the handler run),
	 * never wedge.
	 */
	k_sem_init(&never_sem, 0, 1);
	ret = k_sem_take(&never_sem, K_TICKS(5 * PERIOD_TICKS));
	zassert_equal(ret, -EAGAIN, "blocked wait did not time out normally (ret %d)", ret);
	zassert_true(handler_hits > 0, "no delivery while blocked");

	action.handler = K_SIG_DFL;
	zassert_ok(k_sig_action(K_SIG_USR2, &action, NULL));
	mask_signo(K_SIG_USR2, true);
	zassert_ok(sys_timer_delete(t));
}

static void sigval_self(void)
{
	struct k_timer *t = NULL;
	struct k_sig_info info = {0};
	struct k_timer_notify notify = {
		.target = (k_pid_t)k_current_get(),
		.signo = K_SIG_USR1,
		/* .value deliberately unset: SYS_TIMER_SIGVAL_SELF overrides it */
	};

	zassert_ok(sys_timer_create(SYS_CLOCK_MONOTONIC, &notify, SYS_TIMER_SIGVAL_SELF, NULL,
				    NULL, &t));
	zassert_ok(arm_ticks(t, PERIOD_TICKS, 0));

	zassert_equal(wait_signo(K_SIG_USR1, &info, K_TICKS(10 * PERIOD_TICKS)), K_SIG_USR1);
	zassert_equal_ptr(info.value.sival_ptr, t,
			  "SYS_TIMER_SIGVAL_SELF did not deliver the timer's own address");

	zassert_ok(sys_timer_delete(t));
}

static void delete_purges_pending(void)
{
	struct k_timer *t = create_sig_timer(K_SIG_USR1);

	zassert_ok(arm_ticks(t, PERIOD_TICKS, 0));

	/* wait for the (masked) signal to be generated, then delete without accepting it */
	k_sleep(K_TICKS(2 * PERIOD_TICKS));
	zassert_ok(sys_timer_delete(t));

	zassert_equal(wait_signo(K_SIG_USR1, NULL, K_NO_WAIT), -EAGAIN,
		      "deleted timer left a queued signal behind");
}

static void queue_full_best_effort(void)
{
	struct k_timer *t = create_sig_timer(K_SIG_USR1);

	/* fill the (small) signal queue with unrelated masked signals */
	mask_signo(K_SIG_USR2, true);
	while (k_sig_queue((k_pid_t)k_current_get(), K_SIG_USR2, (union k_sig_val){0}) == 0) {
	}

	/* expiries survive generation failure; that period's signal is simply lost */
	zassert_ok(arm_ticks(t, PERIOD_TICKS, PERIOD_TICKS));
	k_sleep(K_TICKS(2 * PERIOD_TICKS + 2));

	/* drain the queue: generation succeeds again on a later period */
	drain_signo(K_SIG_USR2);
	zassert_equal(wait_signo(K_SIG_USR1, NULL, K_TICKS(10 * PERIOD_TICKS)), K_SIG_USR1,
		      "signal generation did not recover after queue-full");

	zassert_ok(sys_timer_delete(t));
	drain_signo(K_SIG_USR2);
}

static void coexist_expiry_fn(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	handler_hits++;
}

K_TIMER_DEFINE_EXT(coexist_timer, coexist_expiry_fn, NULL, SYS_CLOCK_MONOTONIC);

static struct k_sem notify_fn_sem;
ZTEST_BMEM static volatile int notify_fn_count;
ZTEST_BMEM static volatile intptr_t notify_fn_value;

static void notify_fn(union k_sig_val value)
{
	notify_fn_value = (intptr_t)value.sival_int;
	++notify_fn_count;
	k_sem_give(&notify_fn_sem);
}

static void notify_fn_dispatch(void)
{
	struct k_timer *t = NULL;
	struct k_timer_notify notify = {
		.fn = notify_fn,
		.value.sival_int = VAL_COOKIE,
		.fn_priority = K_PRIO_PREEMPT(0),
	};

	notify_fn_count = 0;
	notify_fn_value = 0;
	k_sem_init(&notify_fn_sem, 0, K_SEM_MAX_LIMIT);

	zassert_ok(sys_timer_create(SYS_CLOCK_MONOTONIC, &notify, 0, NULL, NULL, &t));
	zassert_ok(arm_ticks(t, PERIOD_TICKS, PERIOD_TICKS));

	/* each expiry runs the function, with the notification value, in a fresh thread */
	for (int i = 1; i <= 3; ++i) {
		zassert_ok(k_sem_take(&notify_fn_sem, K_TICKS(100 * PERIOD_TICKS)),
			   "notification %d did not run", i);
	}
	zassert_true(notify_fn_count >= 3, "function ran %d times", notify_fn_count);
	zassert_equal(notify_fn_value, VAL_COOKIE, "wrong notification value");

	/* deletion detaches and purges; no notification runs afterwards */
	zassert_ok(sys_timer_delete(t));
	notify_fn_count = 0;
	zassert_equal(k_sem_take(&notify_fn_sem, K_TICKS(4 * PERIOD_TICKS)), -EAGAIN);
	zassert_equal(notify_fn_count, 0, "notification ran after deletion");
}

static void notify_fn_invalid_priority(void)
{
	struct k_timer *t = NULL;
	struct k_timer_notify notify = {
		.fn = notify_fn,
		.fn_priority = K_LOWEST_APPLICATION_THREAD_PRIO + 1,
	};

	zassert_equal(sys_timer_create(SYS_CLOCK_MONOTONIC, &notify, 0, NULL, NULL, &t),
		      -EINVAL);
}

static void expiry_fn_coexists(void)
{
	handler_hits = 0;

	z_timer_sig_attach(&coexist_timer, (k_pid_t)k_current_get(), K_SIG_USR1,
			   (union k_sig_val){.sival_int = VAL_COOKIE});
	zassert_ok(arm_ticks(&coexist_timer, PERIOD_TICKS, 0));

	/* both the legacy expiry function and the signal must fire */
	zassert_equal(wait_signo(K_SIG_USR1, NULL, K_TICKS(10 * PERIOD_TICKS)), K_SIG_USR1);
	zassert_equal(handler_hits, 1, "expiry_fn did not run alongside signal generation");

	z_timer_sig_detach(&coexist_timer);
	k_timer_stop(&coexist_timer);
}

#ifdef CONFIG_USERSPACE
/* fault-harness exception to one-ZTEST-per-unit: this test point must oops */
ZTEST_USER(timer_signal, test_sys_timer_create_fault)
{
	struct k_timer *t = NULL;
	uint32_t not_a_thread[4];
	struct k_timer_notify notify = {
		.target = (k_pid_t)&not_a_thread,
		.signo = K_SIG_USR1,
	};

	/* a target that is not a granted kernel thread object must fault, not corrupt */
	ztest_set_fault_valid(true);
	(void)sys_timer_create(SYS_CLOCK_MONOTONIC, &notify, 0, NULL, NULL, &t);
	ztest_test_fail(); /* unreachable: the syscall must oops */
}
#endif /* CONFIG_USERSPACE */

/* the per-test cleanup, applied between the sections of a merged per-unit test */
static void section_reset(void)
{
	/* k_sig_timedwait() only works on masked signals */
	mask_signo(K_SIG_USR1, true);
	drain_signo(K_SIG_USR1);
	drain_signo(K_SIG_USR2);
}

ZTEST_USER(timer_signal, test_sys_timer_create)
{
	delivery_timedwait();
	section_reset();
	delivery_handler();
	section_reset();
	block_after_handler_delivery();
	section_reset();
	pending_signal_while_blocked();
	section_reset();
	sigval_self();
	section_reset();
	thread_targeted_delivery();
	section_reset();
	queue_full_best_effort();
	section_reset();
	notify_fn_dispatch();
	section_reset();
	notify_fn_invalid_priority();
	section_reset();
}

/*
 * Kernel mode: consuming signals from one ZTEST_USER incarnation of the ztest thread
 * hangs a blocked k_sig_timedwait() in the next (same k_thread address, new incarnation).
 * TODO(k_sig): root-cause the cross-incarnation fifo-ready state and make this ZTEST_USER.
 */
ZTEST(timer_signal, test_k_timer_overrun)
{
	one_pending_and_overrun();
	section_reset();
	overrun_oneshot_zero();
	section_reset();
	overrun_counts_coalesced_gap();
	section_reset();
	disarm_freezes_overrun();
	section_reset();
}

ZTEST_USER(timer_signal, test_sys_timer_delete)
{
	delete_purges_pending();
	section_reset();
}

ZTEST(timer_signal, test_k_timer_define_ext)
{
	/* attaches through the internal API against a statically-defined timer */
	expiry_fn_coexists();
	section_reset();
}

static void before(void *arg)
{
	ARG_UNUSED(arg);

	/* k_sig_timedwait() only works on masked signals */
	mask_signo(K_SIG_USR1, true);
}

static void after(void *arg)
{
	ARG_UNUSED(arg);

	drain_signo(K_SIG_USR1);
	drain_signo(K_SIG_USR2);
}

static void *suite_setup(void)
{
#ifdef CONFIG_USERSPACE
	k_thread_access_grant(k_current_get(), &helper_thread, &helper_stack, &notify_fn_sem,
			      &never_sem);
#endif
	return NULL;
}

ZTEST_SUITE(timer_signal, NULL, suite_setup, before, after, NULL);
