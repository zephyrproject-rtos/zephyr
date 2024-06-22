/*
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>

#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

ZTEST(signal, test_sigemptyset)
{
	sigset_t set;

	memset(&set, 0xff, sizeof(set));
	zassert_ok(sigemptyset(&set));

	for (int i = 1; i <= SIGRTMAX; ++i) {
		zassert_false(sigismember(&set, i), "signal %d is part of empty set", i);
	}
}

ZTEST(signal, test_sigfillset)
{
	sigset_t set;

	memset(&set, 0x00, sizeof(set));
	zassert_ok(sigfillset(&set));

	for (int i = 1; i <= SIGRTMAX; ++i) {
		zassert_true(sigismember(&set, i), "signal %d is not part of filled set", i);
	}
}

static void sigaddset_common(sigset_t *set, int signo)
{
	zassert_ok(sigemptyset(set));
	zassert_ok(sigaddset(set, signo), "failed to add signal %d", signo);
	for (int i = 1; i <= SIGRTMAX; ++i) {
		if (i == signo) {
			zassert_true(sigismember(set, i), "signal %d should be part of set", i);
		} else {
			zassert_false(sigismember(set, i), "signal %d should not be part of set",
				      i);
		}
	}
}

ZTEST(signal, test_sigaddset)
{
	sigset_t set;

	zassert_ok(sigemptyset(&set));

	{
		/* Degenerate cases */
		zassert_equal(sigaddset(&set, -1), -1, "rc should be -1");
		zassert_equal(errno, EINVAL, "errno should be %s", "EINVAL");

		zassert_equal(sigaddset(&set, 0), -1, "rc should be -1");
		zassert_equal(errno, EINVAL, "errno should be %s", "EINVAL");

		zassert_equal(sigaddset(&set, CONFIG_SIGNAL_SET_SIZE + 1), -1, "rc should be -1");
		zassert_equal(errno, EINVAL, "errno should be %s", "EINVAL");
	}

	sigaddset_common(&set, SIGHUP);
	sigaddset_common(&set, SIGSYS);
	/* >=32, will be in the second sig set for 32bit */
	sigaddset_common(&set, SIGRTMIN);
	sigaddset_common(&set, SIGRTMAX);
}

static void sigdelset_common(sigset_t *set, int signo)
{
	zassert_ok(sigfillset(set));
	zassert_ok(sigdelset(set, signo));
	for (int i = 1; i <= SIGRTMAX; ++i) {
		if (i == signo) {
			zassert_false(sigismember(set, i), "signal %d should not be part of set",
				      i);
		} else {
			zassert_true(sigismember(set, i), "signal %d should be part of set", i);
		}
	}
}

ZTEST(signal, test_sigdelset)
{
	sigset_t set;

	zassert_ok(sigemptyset(&set));

	{
		/* Degenerate cases */
		zassert_equal(sigdelset(&set, -1), -1, "rc should be -1");
		zassert_equal(errno, EINVAL, "errno should be %s", "EINVAL");

		zassert_equal(sigdelset(&set, 0), -1, "rc should be -1");
		zassert_equal(errno, EINVAL, "errno should be %s", "EINVAL");

		zassert_equal(sigdelset(&set, CONFIG_SIGNAL_SET_SIZE + 1), -1, "rc should be -1");
		zassert_equal(errno, EINVAL, "errno should be %s", "EINVAL");
	}

	sigdelset_common(&set, SIGHUP);
	sigdelset_common(&set, SIGSYS);
	/* >=32, will be in the second sig set for 32bit */
	sigdelset_common(&set, SIGRTMIN);
	sigdelset_common(&set, SIGRTMAX);
}

ZTEST(signal, test_sigismember)
{
	sigset_t set;

	zassert_ok(sigemptyset(&set));

	{
		/* Degenerate cases */
		zassert_equal(sigismember(&set, -1), -1, "rc should be -1");
		zassert_equal(errno, EINVAL, "errno should be %s", "EINVAL");

		zassert_equal(sigismember(&set, 0), -1, "rc should be -1");
		zassert_equal(errno, EINVAL, "errno should be %s", "EINVAL");

		zassert_equal(sigismember(&set, SIGRTMAX + 1), -1, "rc should be -1");
		zassert_equal(errno, EINVAL, "errno should be %s", "EINVAL");
	}

	for (int i = 0; i < RTSIG_MAX; ++i) {
		zassert_equal(0, sigismember(&set, (SIGRTMIN + i)),
			      "signal %d should not be part of set", (SIGRTMIN + i));
	}

	zassert_ok(sigfillset(&set));

	for (int i = 0; i < RTSIG_MAX; ++i) {
		zassert_equal(1, sigismember(&set, (SIGRTMIN + i)),
			      "signal %d should be part of set", (SIGRTMIN + i));
	}

	zassert_ok(sigdelset(&set, SIGKILL));
	zassert_equal(sigismember(&set, SIGKILL), 0, "%s not expected to be member", "SIGKILL");
}

ZTEST(signal, test_signal_strsignal)
{
	/* Using -INT_MAX here because compiler resolves INT_MIN to (-2147483647 - 1) */
	char buf[sizeof("RT signal -" STRINGIFY(INT_MAX))] = {0};

	zassert_mem_equal(strsignal(-1), "Invalid signal", sizeof("Invalid signal"));
	zassert_mem_equal(strsignal(0), "Invalid signal", sizeof("Invalid signal"));
	zassert_mem_equal(strsignal(SIGRTMAX + 1), "Invalid signal", sizeof("Invalid signal"));

	zassert_mem_equal(strsignal(30), "Signal 30", sizeof("Signal 30"));
	snprintf(buf, sizeof(buf), "RT signal %d", SIGRTMIN - SIGRTMIN);
	zassert_mem_equal(strsignal(SIGRTMIN), buf, strlen(buf));
	snprintf(buf, sizeof(buf), "RT signal %d", SIGRTMAX - SIGRTMIN);
	zassert_mem_equal(strsignal(SIGRTMAX), buf, strlen(buf));

#ifdef CONFIG_POSIX_SIGNAL_STRING_DESC
	zassert_mem_equal(strsignal(SIGHUP), "Hangup", sizeof("Hangup"));
	zassert_mem_equal(strsignal(SIGSYS), "Bad system call", sizeof("Bad system call"));
#else
	zassert_mem_equal(strsignal(SIGHUP), "Signal 1", sizeof("Signal 1"));
	zassert_mem_equal(strsignal(SIGSYS), "Signal 31", sizeof("Signal 31"));
#endif
}

typedef int (*sigmask_fn)(int how, const sigset_t *set, sigset_t *oset);
static void *test_sigmask_entry(void *arg)
{
/* for clarity */
#define SIG_GETMASK SIG_SETMASK

	enum {
		NEW,
		OLD,
	};
	static sigset_t set[2];
	const int invalid_how = 0x9a2ba9e;
	sigmask_fn sigmask = arg;

	sigemptyset(&set[0]);
	sigemptyset(&set[1]);

	/* invalid how results in EINVAL */
	zassert_equal(sigmask(invalid_how, NULL, NULL), EINVAL);
	zassert_equal(sigmask(invalid_how, &set[NEW], &set[OLD]), EINVAL);

	/* verify setting / getting masks */
	zassert_ok(sigemptyset(&set[NEW]));
	zassert_ok(sigmask(SIG_SETMASK, &set[NEW], NULL));
	zassert_ok(sigfillset(&set[OLD]));
	zassert_ok(sigmask(SIG_GETMASK, NULL, &set[OLD]));
	zassert_mem_equal(&set[OLD], &set[NEW], sizeof(set[OLD]));

	zassert_ok(sigfillset(&set[NEW]));
	zassert_ok(sigmask(SIG_SETMASK, &set[NEW], NULL));
	zassert_ok(sigemptyset(&set[OLD]));
	zassert_ok(sigmask(SIG_GETMASK, NULL, &set[OLD]));
	zassert_mem_equal(&set[OLD], &set[NEW], sizeof(set[OLD]));

	/* start with an empty mask */
	zassert_ok(sigemptyset(&set[NEW]));
	zassert_ok(sigmask(SIG_SETMASK, &set[NEW], NULL));

	/* verify SIG_BLOCK: expect (SIGUSR1 | SIGUSR2 | SIGHUP) */
	zassert_ok(sigemptyset(&set[NEW]));
	zassert_ok(sigaddset(&set[NEW], SIGUSR1));
	zassert_ok(sigmask(SIG_BLOCK, &set[NEW], NULL));

	zassert_ok(sigemptyset(&set[NEW]));
	zassert_ok(sigaddset(&set[NEW], SIGUSR2));
	zassert_ok(sigaddset(&set[NEW], SIGHUP));
	zassert_ok(sigmask(SIG_BLOCK, &set[NEW], NULL));

	zassert_ok(sigemptyset(&set[OLD]));
	zassert_ok(sigaddset(&set[OLD], SIGUSR1));
	zassert_ok(sigaddset(&set[OLD], SIGUSR2));
	zassert_ok(sigaddset(&set[OLD], SIGHUP));

	zassert_ok(sigmask(SIG_GETMASK, NULL, &set[NEW]));
	zassert_mem_equal(&set[NEW], &set[OLD], sizeof(set[NEW]));

	/* start with full mask */
	zassert_ok(sigfillset(&set[NEW]));
	zassert_ok(sigmask(SIG_SETMASK, &set[NEW], NULL));

	/* verify SIG_UNBLOCK: expect ~(SIGUSR1 | SIGUSR2 | SIGHUP) */
	zassert_ok(sigemptyset(&set[NEW]));
	zassert_ok(sigaddset(&set[NEW], SIGUSR1));
	zassert_ok(sigmask(SIG_UNBLOCK, &set[NEW], NULL));

	zassert_ok(sigemptyset(&set[NEW]));
	zassert_ok(sigaddset(&set[NEW], SIGUSR2));
	zassert_ok(sigaddset(&set[NEW], SIGHUP));
	zassert_ok(sigmask(SIG_UNBLOCK, &set[NEW], NULL));

	zassert_ok(sigfillset(&set[OLD]));
	zassert_ok(sigdelset(&set[OLD], SIGUSR1));
	zassert_ok(sigdelset(&set[OLD], SIGUSR2));
	zassert_ok(sigdelset(&set[OLD], SIGHUP));

	zassert_ok(sigmask(SIG_GETMASK, NULL, &set[NEW]));
	zassert_mem_equal(&set[NEW], &set[OLD], sizeof(set[NEW]));

	return NULL;
}

ZTEST(signal, test_pthread_sigmask)
{
	pthread_t th;

	zassert_ok(pthread_create(&th, NULL, test_sigmask_entry, pthread_sigmask));
	zassert_ok(pthread_join(th, NULL));
}

ZTEST(signal, test_sigprocmask)
{
	if (IS_ENABLED(CONFIG_MULTITHREADING)) {
		if (!IS_ENABLED(CONFIG_ASSERT)) {
			zassert_not_ok(sigprocmask(SIG_SETMASK, NULL, NULL));
			zassert_equal(errno, ENOSYS);
		}
	} else {
		pthread_t th;

		zassert_ok(pthread_create(&th, NULL, test_sigmask_entry, sigprocmask));
		zassert_ok(pthread_join(th, NULL));
	}
}

static void realtime_sigset(sigset_t *set)
{
	sigemptyset(set);
	for (int i = 0; i < RTSIG_MAX; ++i) {
		sigaddset(set, (SIGRTMIN + i));
	}
}

static void block_non_realtime_signals(void)
{
	/*
	 * Normal signals are not yet a thing in Zephyr. Specifically, they behave just like
	 * realtime signals (which is not POSIX conformant). More specifically, normal signals are
	 * not delivered asynchronously. To keep this test mostly compliant though, we block normal
	 * signals and only consider realtime signals.
	 */

	sigset_t set;

	sigfillset(&set);
	for (int i = 0; i < RTSIG_MAX; ++i) {
		sigdelset(&set, (SIGRTMIN + i));
	}

	zassert_ok(pthread_sigmask(SIG_BLOCK, &set, NULL));
}

/*
 * This is a workaround for the fact that Zephyr threads and POSIX threads are not yet fully
 * interchangeable. I.e. the "is a" relationship is not bidirectional. A POSIX thread is a
 * Zephyr thread, but a Zephyr thread is not a POSIX thread. The most obvious tell is
 * that pthread_self() only returns a valid pthread_t for POSIX threads; invalid pthread_t
 * cannot be used successfully in subsequent pthread operations.
 *
 * Note: Zephyr threads can still use k_sig_queue() directly.
 */
static void *test_sigqueue_thread(void *arg)
{
	sigset_t set;
	siginfo_t info;
	struct timespec timeout = {0};

	ARG_UNUSED(arg);

	block_non_realtime_signals();
	realtime_sigset(&set);

	/*
	 * Test for multiple real-time signals of the same type being returned in the order
	 * they were queued.
	 *
	 * Note: Normally, sigtimedwait() would only populate the siginfo_t argument if a SA_SIGINFO
	 * flag provided by sigaction(), but that is part of POSIX_SIGNALS Option Group
	 * (unimplemented) and sigqueue(), sigtimedwait(), sigwaitinfo() are part of the
	 * POSIX_REALTIME_SIGNALS Option Group.
	 *
	 * Currently, we sigwaitinfo(), sigtimedwait(), assume that the siginfo_t argument should be
	 * populated if it is non-NULL.
	 */
	for (int i = 0; i < SIGQUEUE_MAX; ++i) {
		zassert_ok(
			sigqueue((pid_t)pthread_self(), SIGRTMIN, (union sigval){.sival_int = i}),
			"failed to que the %d-th signal", i);
	}

	for (int i = 0; i < SIGQUEUE_MAX; ++i) {
		int actual;

		info = (siginfo_t){0};
		actual = sigtimedwait(&set, &info, &timeout);

		zassert_equal(SIGRTMIN, actual,
			      "iteration %d expected SIGRTMIN (%d) but sigtimedwait() returned %d "
			      "(errno: %d)",
			      i, SIGRTMIN, actual, errno);
		zassert_equal(info.si_value.sival_int, i);
	}

	/*
	 * test for different real-time signals being delivered lowest-numbered first
	 */
	for (int i = RTSIG_MAX - 1; i >= 0; --i) {
		zassert_ok(sigqueue((pid_t)pthread_self(), (SIGRTMIN + i), (union sigval){0}),
			   "unable to queue signal %d", (SIGRTMIN + i));
	}

	for (int i = 0; i < RTSIG_MAX; ++i) {
		int actual = sigtimedwait(&set, NULL, &timeout);

		zassert_equal((SIGRTMIN + i), actual,
			      "expected signal %d, but sigtimedwait() returned %d", (SIGRTMIN + i),
			      actual);
	}

	return NULL;
}

ZTEST(signal, test_sigqueue)
{
	pthread_t th;

	{
		/* Degenerate cases */
		zassert_not_ok(sigqueue(-1, -1, (union sigval){0}));
		zassert_not_ok(sigqueue(-1, SIGUSR1, (union sigval){0}));
		zassert_not_ok(sigqueue((pid_t)pthread_self(), -1, (union sigval){0}));
	}

	zassert_ok(pthread_create(&th, NULL, test_sigqueue_thread, NULL));
	zassert_ok(pthread_join(th, NULL));
}

static void *sleep_100ms_and_queue(void *arg)
{
	k_msleep(100);
	zassert_ok(sigqueue(POINTER_TO_INT(arg), SIGRTMIN, (union sigval){0}));

	return NULL;
}

static void queue_signal_after_100ms(pthread_t th)
{
	pthread_t t1;

	zassert_ok(pthread_create(&t1, NULL, sleep_100ms_and_queue, INT_TO_POINTER(th)));
	zassert_ok(pthread_join(t1, NULL));
}

/*
 * This is a workaround for the fact that Zephyr threads and POSIX threads are not yet fully
 * interchangeable. I.e. the "is a" relationship is not bidirectional. A POSIX thread is a
 * Zephyr thread, but a Zephyr thread is not a POSIX thread. The most obvious tell is
 * that pthread_self() only returns a valid pthread_t for POSIX threads; invalid pthread_t
 * cannot be used successfully in subsequent pthread operations.
 *
 * Note: Zephyr threads can still use k_sig_timedwait() directly.
 */
static void *test_sigtimedwait_thread(void *arg)
{
	sigset_t set;
	siginfo_t info;
	pid_t self = (pid_t)pthread_self();
	uint32_t begin_ms, delta_ms, end_ms;
	struct timespec timeout = {0};

	ARG_UNUSED(arg);

	block_non_realtime_signals();
	realtime_sigset(&set);

	const struct stw_args_exp {
		const sigset_t *set;
		siginfo_t *info;
		struct timespec *timeout;
		int expected_errno;
	} harness[] = {
		{NULL, NULL, NULL, EINVAL},
		{NULL, NULL, &timeout, EINVAL},
		{NULL, &info, NULL, EINVAL},
		{NULL, &info, &timeout, EINVAL},
		{&set, NULL, NULL, 0},
		{&set, NULL, &timeout, 0},
		{&set, &info, NULL, 0},
		{&set, &info, &timeout, 0},
	};

	ARRAY_FOR_EACH_PTR(harness, a) {
		errno = 0;
		if (a->expected_errno == 0) {
			zassert_ok(sigqueue(self, SIGRTMIN, (union sigval){0}));
			zassert_equal(SIGRTMIN, sigtimedwait(a->set, a->info, a->timeout));
			zassert_equal(errno, 0);
		} else {
			zassert_equal(-1, sigtimedwait(a->set, a->info, a->timeout));
			zassert_equal(errno, a->expected_errno);
		}
	}

	/* Without a queued signal, sigtimedwait should timeout immediately if timespec == 0.0 */
	begin_ms = k_uptime_get_32();
	zassert_equal(-1, sigtimedwait(&set, NULL, &timeout));
	zassert_equal(errno, EAGAIN);
	end_ms = k_uptime_get_32();
	delta_ms = end_ms - begin_ms;
	/* hard to say how fast this will execute on every platform, but this should be safe */
	zassert_true(delta_ms < 50);

	/* Without a queued signal, sigtimedwait should timeout after 1s if timespec == 1.0 */
	timeout.tv_sec = 1;
	begin_ms = k_uptime_get_32();
	zassert_equal(-1, sigtimedwait(&set, NULL, &timeout));
	zassert_equal(errno, EAGAIN);
	end_ms = k_uptime_get_32();
	delta_ms = end_ms - begin_ms;
	zassert_true(delta_ms >= MSEC_PER_SEC);

	/*
	 * Queue a signal after 100 ms. sigtimedwait should return successfully after 100 ms
	 * and before 200 ms if timespec == INFINITY
	 */
	timeout.tv_sec = 42; /* basically infinity ¯\_(ツ)_/¯ */
	begin_ms = k_uptime_get_32();
	queue_signal_after_100ms(pthread_self());
	zassert_equal(SIGRTMIN, sigtimedwait(&set, NULL, &timeout));
	end_ms = k_uptime_get_32();
	delta_ms = end_ms - begin_ms;
	zassert_true(delta_ms >= 100);
	zassert_true(delta_ms < 200);

	return NULL;
}

ZTEST(signal, test_sigtimedwait)
{
	pthread_t t1;

	zassert_ok(pthread_create(&t1, NULL, test_sigtimedwait_thread, NULL));
	zassert_ok(pthread_join(t1, NULL));
}

/*
 * This is a workaround for the fact that Zephyr threads and POSIX threads are not yet fully
 * interchangeable. I.e. the "is a" relationship is not bidirectional. A POSIX thread is a
 * Zephyr thread, but a Zephyr thread is not a POSIX thread. The most obvious tell is
 * that pthread_self() only returns a valid pthread_t for POSIX threads; invalid pthread_t
 * cannot be used successfully in subsequent pthread operations.
 */
static void *test_sigwaitinfo_thread(void *arg)
{
	sigset_t set;
	siginfo_t info;
	uint32_t begin_ms, delta_ms, end_ms;

	ARG_UNUSED(arg);

	block_non_realtime_signals();
	realtime_sigset(&set);

	const struct swi_args_exp {
		const sigset_t *set;
		siginfo_t *info;
		int expected_errno;
	} harness[] = {
		{NULL, NULL, EINVAL},
		{NULL, &info, EINVAL},
		{&set, NULL, 0},
		{&set, &info, 0},
	};

	ARRAY_FOR_EACH_PTR(harness, a) {
		errno = 0;
		if (a->expected_errno == 0) {
			zassert_ok(sigqueue((pid_t)pthread_self(), SIGRTMIN, (union sigval){0}));
			zassert_equal(SIGRTMIN, sigwaitinfo(a->set, a->info));
			zassert_equal(errno, 0);
		} else {
			zassert_equal(-1, sigwaitinfo(a->set, a->info));
			zassert_equal(errno, a->expected_errno);
		}
	}

	/*
	 * Queue a signal after 100 ms. sigwaitinfo should return successfully after 100 ms and
	 * before 200 ms
	 */
	begin_ms = k_uptime_get_32();
	queue_signal_after_100ms(pthread_self());
	zassert_equal(SIGRTMIN, sigwaitinfo(&set, NULL));
	end_ms = k_uptime_get_32();
	delta_ms = end_ms - begin_ms;
	zassert_true(delta_ms >= 100);
	zassert_true(delta_ms < 200);

	return NULL;
}

ZTEST(signal, test_sigwaitinfo)
{
	pthread_t t1;

	zassert_ok(pthread_create(&t1, NULL, test_sigwaitinfo_thread, NULL));
	zassert_ok(pthread_join(t1, NULL));
}

static void before(void *arg)
{
	ARG_UNUSED(arg);

	if (!IS_ENABLED(CONFIG_DYNAMIC_THREAD)) {
		/* skip redundant testing if there is no thread pool / heap allocation */
		ztest_test_skip();
	}
}

ZTEST_SUITE(signal, NULL, NULL, before, NULL, NULL);
