/*
 * Copyright (c) 2024, Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

BUILD_ASSERT(K_SIG_NUM_RT >= 0);

static struct k_sig_set rt_sigset;

static void block_non_realtime_signals(void)
{
	/*
	 * Normal signals are not yet a thing in Zephyr. Specifically, they behave just like
	 * realtime signals (which is not POSIX conformant). More specifically, normal signals are
	 * not delivered asynchronously. To keep this test mostly compliant though, we block normal
	 * signals and only consider realtime signals.
	 */

	struct k_sig_set set;

	k_sig_fillset(&set);
	for (int i = 0; i < K_SIG_NUM_RT; ++i) {
		k_sig_delset(&set, (K_SIG_RTMIN + i));
	}

	zassert_ok(k_sig_mask(K_SIG_BLOCK, &set, NULL));
}

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

	block_non_realtime_signals();

	/*
	 * Test for multiple real-time signals of the same type being returned in the order
	 * they were queued.
	 */
	for (int i = 0; i < CONFIG_SIGNAL_QUEUE_SIZE; ++i) {
		zassert_ok(k_sig_queue((k_pid_t)k_current_get(), K_SIG_RTMIN,
				       (union k_sig_val){.sival_int = i}),
			   "failed to queue the %d-th signal", i);
	}

	for (int i = 0; i < CONFIG_SIGNAL_QUEUE_SIZE; ++i) {
		int actual;

		info = (struct k_sig_info){0};
		actual = k_sig_timedwait(&set, &info, timeout);

		zassert_equal(
			K_SIG_RTMIN, actual,
			"iteration %d expected K_SIG_RTMIN (%d) but k_sig_timedwait() returned %d "
			"(errno: %d)",
			i, K_SIG_RTMIN, actual, errno);
		zassert_equal(info.si_value.sival_int, i);
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
		{NULL, NULL, K_NO_WAIT, -EINVAL},
		{NULL, &info, K_NO_WAIT, -EINVAL},
		{&set, NULL, K_NO_WAIT, 0},
		{&set, &info, K_NO_WAIT, 0},
	};

	block_non_realtime_signals();

	ARRAY_FOR_EACH_PTR(harness, a) {
		errno = 0;
		if (a->expected_error == 0) {
			zassert_ok(k_sig_queue(self, K_SIG_RTMIN, (union k_sig_val){0}));
			zassert_equal(K_SIG_RTMIN, k_sig_timedwait(a->set, a->info, K_NO_WAIT));
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
	 * and before 200 ms if timespec == INFINITY
	 */
	begin_ms = k_uptime_get_32();
	queue_signal_after_ms(k_current_get(), 100);
	zassert_equal(K_SIG_RTMIN, k_sig_timedwait(&set, NULL, K_SECONDS(42)));
	end_ms = k_uptime_get_32();
	delta_ms = end_ms - begin_ms;
	zassert_true(delta_ms >= 100);
	zassert_true(delta_ms < 200);
}

static void *setup(void)
{
	k_sig_emptyset(&rt_sigset);

	for (int i = 0; i < K_SIG_NUM_RT; ++i) {
		k_sig_addset(&rt_sigset, (K_SIG_RTMIN + i));
	}

	return NULL;
}

ZTEST_SUITE(signal, NULL, setup, NULL, NULL, NULL);
