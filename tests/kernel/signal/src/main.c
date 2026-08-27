/*
 * Copyright (c) 2024, Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

BUILD_ASSERT(K_SIG_NUM_RT >= 0);

static void block_rt_signals(void);

static struct k_sig_set rt_sigset;

ZTEST(signal, test_k_sig_queue)
{
	{
		/* Degenerate cases */
		zassert_not_ok(k_sig_queue(NULL, -1, (union k_sig_val){0}));
		zassert_not_ok(k_sig_queue(NULL, K_SIG_RTMIN, (union k_sig_val){0}));
		zassert_not_ok(k_sig_queue((k_pid_t)k_current_get(), -1, (union k_sig_val){0}));
	}

	struct k_sig_info info;
	const k_timeout_t timeout = K_NO_WAIT;
	const struct k_sig_set set = rt_sigset;

	block_rt_signals();

	/*
	 * Test for multiple real-time signals of the same type being returned in the order
	 * they were queued.
	 */
	for (int i = 0; i < SIGNAL_QUEUE_SIZE; ++i) {
		zassert_ok(k_sig_queue((k_pid_t)k_current_get(), K_SIG_RTMIN,
				       (union k_sig_val){.sival_int = i}),
			   "failed to queue the %d-th signal", i);
	}

	/* Test that queuing another signal will fail once the queue is full */
	zassert_equal(-EAGAIN, k_sig_queue((k_pid_t)k_current_get(), K_SIG_RTMIN,
				    (union k_sig_val){0}));

	for (int i = 0; i < SIGNAL_QUEUE_SIZE; ++i) {
		int actual;

		info = (struct k_sig_info){0};
		actual = k_sig_timedwait(&set, &info, timeout);

		zassert_equal(
			K_SIG_RTMIN, actual,
			"iteration %d expected K_SIG_RTMIN (%d) but k_sig_timedwait() returned %d "
			"(errno: %d)",
			i, K_SIG_RTMIN, actual, errno);
		zassert_equal(info.value.sival_int, i);
	}

	/*
	 * test for different real-time signals being delivered lowest-numbered first
	 */
	for (int i = K_SIG_NUM_RT - 1; i >= 0; --i) {
		zassert_ok(k_sig_queue((k_pid_t)k_current_get(), (K_SIG_RTMIN + i),
				       (union k_sig_val){0}),
			   "unable to queue signal %d", (K_SIG_RTMIN + i));
	}

	for (int i = 0; i < K_SIG_NUM_RT; ++i) {
		int actual = k_sig_timedwait(&set, NULL, timeout);

		zassert_equal((K_SIG_RTMIN + i), actual,
			      "expected signal %d, but k_sig_timedwait() returned %d",
			      (K_SIG_RTMIN + i), actual);
	}
}

static volatile int kill_handler_calls;

static void kill_handler(int sig, struct k_sig_info *info, void *ucontext)
{
	ARG_UNUSED(sig);
	ARG_UNUSED(info);
	ARG_UNUSED(ucontext);

	kill_handler_calls++;
}

ZTEST(signal, test_k_kill)
{
	struct k_sig_set set;
	struct k_sig_action action = {.handler = kill_handler};

	/* degenerate cases */
	zassert_equal(k_kill(NULL, -1), -EINVAL);

	/* existence checks: NULL and any live thread handle name the single process */
	zassert_ok(k_kill(NULL, 0));
	zassert_ok(k_kill(k_current_get(), 0));

	/* every thread masks it (the kernel default): it pends at system scope */
	zassert_ok(k_kill(NULL, K_SIG_RTMIN));

	/* and the first thread to wait for it claims it */
	zassert_ok(k_sig_emptyset(&set));
	zassert_ok(k_sig_addset(&set, K_SIG_RTMIN));
	zassert_equal(k_sig_timedwait(&set, NULL, K_NO_WAIT), K_SIG_RTMIN);

	/* a caller with the signal unblocked takes the delivery itself */
	zassert_ok(k_sig_emptyset(&action.mask));
	zassert_ok(k_sig_action(K_SIG_USR1, &action, NULL));
	zassert_ok(k_sig_emptyset(&set));
	zassert_ok(k_sig_addset(&set, K_SIG_USR1));
	zassert_ok(k_sig_mask(K_SIG_UNBLOCK, &set, NULL));

	kill_handler_calls = 0;
	zassert_ok(k_kill(NULL, K_SIG_USR1));
	zassert_equal(kill_handler_calls, 1, "the eligible caller did not take the delivery");

	zassert_ok(k_sig_mask(K_SIG_BLOCK, &set, NULL));
	action.handler = K_SIG_DFL;
	zassert_ok(k_sig_action(K_SIG_USR1, &action, NULL));
}

ZTEST(signal, test_k_sigqueue)
{
	struct k_sig_set set;
	struct k_sig_info info;

	/* degenerate cases */
	zassert_equal(k_sigqueue(NULL, -1, (union k_sig_val){0}), -EINVAL);

	/* signal 0 checks for existence and delivers nothing */
	zassert_ok(k_sigqueue(NULL, 0, (union k_sig_val){0}));

	/* pends at system scope with the value intact */
	zassert_ok(k_sigqueue(NULL, K_SIG_RTMIN, (union k_sig_val){.sival_int = 41}));
	zassert_ok(k_sig_emptyset(&set));
	zassert_ok(k_sig_addset(&set, K_SIG_RTMIN));
	info = (struct k_sig_info){0};
	zassert_equal(k_sig_timedwait(&set, &info, K_NO_WAIT), K_SIG_RTMIN);
	zassert_equal(info.value.sival_int, 41);
}

static struct sigqueue_work {
	struct k_work_delayable dwork;
	k_tid_t tid;
} sigq_work;

static void do_queue(struct k_work *work)
{
	struct sigqueue_work *sq_work = CONTAINER_OF(
		CONTAINER_OF(work, struct k_work_delayable, work), struct sigqueue_work, dwork);

	zassert_ok(k_sig_queue(sq_work->tid, K_SIG_RTMIN, (union k_sig_val){0}));
}

static void queue_signal_after_ms(k_tid_t tid, int delay_ms)
{
	sigq_work.tid = tid;
	k_work_init_delayable(&sigq_work.dwork, do_queue);
	k_work_schedule(&sigq_work.dwork, K_MSEC(delay_ms));
}

ZTEST(signal, test_k_sig_timedwait)
{
	struct k_sig_info info;
	k_pid_t self = (k_pid_t)k_current_get();
	uint32_t begin_ms, delta_ms, end_ms;
	const struct k_sig_set set = rt_sigset;

	const struct stw_args_exp {
		const struct k_sig_set *set;
		struct k_sig_info *info;
		k_timeout_t timeout;
		int expected_error;
	} harness[] = {
#ifdef CONFIG_TIMEOUT_64BIT
		{&set, &info, K_TIMEOUT_ABS_SEC(1), -EINVAL},
#endif
		{&set, NULL, K_NO_WAIT, 0},
		{&set, &info, K_NO_WAIT, 0},
	};

	block_rt_signals();

	ARRAY_FOR_EACH_PTR(harness, a) {
		errno = 0;
		if (a->expected_error == 0) {
			zassert_ok(k_sig_queue(self, K_SIG_RTMIN, (union k_sig_val){0}));
			zassert_equal(K_SIG_RTMIN, k_sig_timedwait(a->set, a->info, K_NO_WAIT));
			if (a->info != NULL) {
				zassert_equal(a->info->signo, K_SIG_RTMIN);
			}
		} else {
			zassert_equal(a->expected_error,
				      k_sig_timedwait(a->set, a->info, a->timeout),
				      "k_sig_timeout() succeeded, but error %d was expected",
				      a->expected_error);
		}
	}

	/* Without a queued signal, k_sig_timedwait should timeout immediately if timespec == 0.0 */
	begin_ms = k_uptime_get_32();
	zassert_equal(-EAGAIN, k_sig_timedwait(&set, NULL, K_NO_WAIT));
	end_ms = k_uptime_get_32();
	delta_ms = end_ms - begin_ms;
	/* hard to say how fast this will execute on every platform, but 50 ms should be safe */
	zassert_true(delta_ms < 50);

	/* Without a queued signal, k_sig_timedwait should timeout after 100 ms if timeout is 100 ms
	 */
	begin_ms = k_uptime_get_32();
	zassert_equal(-EAGAIN, k_sig_timedwait(&set, NULL, K_MSEC(100)));
	end_ms = k_uptime_get_32();
	delta_ms = end_ms - begin_ms;
	zassert_true(delta_ms >= 100);

	/*
	 * Queue a signal after 1s. k_sig_timedwait should return successfully after 100 ms
	 * and before 200 ms
	 */
	begin_ms = k_uptime_get_32();
	queue_signal_after_ms(k_current_get(), 100);
	zassert_equal(K_SIG_RTMIN, k_sig_timedwait(&set, NULL, K_MSEC(300)));
	end_ms = k_uptime_get_32();
	delta_ms = end_ms - begin_ms;
	zassert_true(delta_ms >= 100);
	zassert_true(delta_ms < 200);
}

static void block_rt_signals(void)
{
	/* k_sig_timedwait() only works on masked signals */
	zassert_ok(k_sig_mask(K_SIG_BLOCK, &rt_sigset, NULL));
}

ZTEST_BMEM volatile bool noop_handler_called;
ZTEST_BMEM static volatile bool noop_handler_returned;
static K_THREAD_STACK_DEFINE(noop_test_stack, CONFIG_ZTEST_STACK_SIZE);
static struct k_thread noop_test_thread;

static void noop_sig_handler(int sig, struct k_sig_info *info, void *ucontext)
{
	ARG_UNUSED(sig);
	ARG_UNUSED(info);
	ARG_UNUSED(ucontext);

	noop_handler_called = true;
}

static void test_noop_signal_entry(void *arg1, void *arg2, void *arg3)
{
	struct k_sig_set mask;
	struct k_sig_action action = {
		.handler = noop_sig_handler,
	};

	if (!k_is_user_context()) {
		/* kernel threads must explicitly unmask any signals they wish to receive */

		zassert_ok(k_sig_emptyset(&mask));
		zassert_ok(k_sig_addset(&mask, K_SIG_USR1));
		zassert_ok(k_sig_mask(K_SIG_UNBLOCK, &mask, NULL));
	}

	zassert_ok(k_sig_mask(K_SIG_SETMASK, NULL, &mask));
	zassert_true(!k_sig_ismember(&mask, K_SIG_USR1));

	zassert_ok(k_sig_action(K_SIG_USR1, &action, NULL));

	/* test thread execution continues after "no-op" signal handler */
	zassert_ok(k_sig_queue((k_pid_t)k_current_get(), K_SIG_USR1, (union k_sig_val){0}));
	k_msleep(0);

	noop_handler_returned = true;

	action.handler = K_SIG_DFL;
	zassert_ok(k_sig_action(K_SIG_USR1, &action, NULL));
}

ZTEST(signal, test_noop_signal_handler_kernel)
{
	noop_handler_called = false;
	noop_handler_returned = false;

	k_thread_create(&noop_test_thread, noop_test_stack, K_THREAD_STACK_SIZEOF(noop_test_stack),
			(k_thread_entry_t)test_noop_signal_entry, NULL, NULL, NULL,
			K_LOWEST_APPLICATION_THREAD_PRIO, 0, K_NO_WAIT);
	zassert_ok(k_thread_join(&noop_test_thread, K_FOREVER));

	zassert_true(noop_handler_called, "no-op signal handler was not called");
	zassert_true(noop_handler_returned, "no-op signal handler did not return properly");
}

#ifdef CONFIG_USERSPACE
ZTEST_USER(signal, test_noop_signal_handler_user)
{
	noop_handler_called = false;
	noop_handler_returned = false;

	k_thread_create(&noop_test_thread, noop_test_stack, K_THREAD_STACK_SIZEOF(noop_test_stack),
			(k_thread_entry_t)test_noop_signal_entry, NULL, NULL, NULL,
			K_LOWEST_APPLICATION_THREAD_PRIO, K_USER | K_INHERIT_PERMS, K_NO_WAIT);
	zassert_ok(k_thread_join(&noop_test_thread, K_FOREVER));

	zassert_true(noop_handler_called, "no-op signal handler was not called");
	zassert_true(noop_handler_returned, "no-op signal handler did not return properly");
}

ZTEST_USER(signal, test_cannot_signal_kernel_from_user)
{
	/* By default, user threads do not have permissions on any kernel objects. So this thread
	 * should not be able to signal e.g. the main thread */

	int ret;
	extern struct k_thread z_main_thread;

	ret = k_sig_queue((k_pid_t)&z_main_thread, K_SIG_SEGV, (union k_sig_val){0});
	zassert_equal(ret, -EPERM, "Should not have signaled. Expected: -EPERM actual: %d", ret);
}
#endif

static void *setup(void)
{
	k_sig_emptyset(&rt_sigset);

	for (int i = 0; i < K_SIG_NUM_RT; ++i) {
		k_sig_addset(&rt_sigset, (K_SIG_RTMIN + i));
	}

	return NULL;
}

static void before(void *arg)
{
	ARG_UNUSED(arg);

#ifdef CONFIG_USERSPACE
	k_thread_access_grant(k_current_get(), &noop_test_thread, &noop_test_stack);
#endif
}

ZTEST_SUITE(signal, NULL, setup, before, NULL, NULL);
