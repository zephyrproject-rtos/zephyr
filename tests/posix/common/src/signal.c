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

#ifndef SIGRTMIN
#define SIGRTMIN (32)
#endif

#ifndef SIGRTMAX
#define SIGRTMAX (32 + CONFIG_POSIX_RTSIG_MAX)
#endif

/* some libcs define these as very non-functional macros */
#undef sigemptyset
#undef sigfillset
#undef sigaddset
#undef sigdelset
#undef sigismember

ZTEST(signal, test_sigemptyset)
{
	sigset_t set;

	zassert_ok(sigemptyset(&set));

	for (int i = 1; i <= SIGRTMAX; i++) {
		zassert_false(sigismember(&set, i), "sigset is not empty (%d)", i);
	}
}

ZTEST(signal, test_sigfillset)
{
	sigset_t set;

	zassert_ok(sigfillset(&set));

	for (int i = 1; i <= SIGRTMAX; i++) {
		zassert_true(sigismember(&set, i), "sigset is not full (%d)", i);
	}
}

ZTEST(signal, test_sigaddset)
{
	sigset_t set;

	sigemptyset(&set);

	{
		if (false) {
			/* this breaks for newlib */
			/* zassert_equal(sigaddset(NULL, -1), -1, "rc should be -1"); */
		}

		/* degenerate cases */
		zexpect_equal(sigaddset(&set, -1), -1, "rc should be -1");
		zexpect_equal(errno, EINVAL, "errno should be %s", "EINVAL");

		zexpect_equal(sigaddset(&set, 0), -1, "rc should be -1");
		zexpect_equal(errno, EINVAL, "errno should be %s", "EINVAL");

		zexpect_equal(sigaddset(&set, SIGRTMAX + 1), -1, "rc should be -1");
		zexpect_equal(errno, EINVAL, "errno should be %s", "EINVAL");
	}

	zexpect_ok(sigaddset(&set, SIGHUP));
	for (int i = 1; i <= SIGRTMAX; i++) {
		zexpect_equal(sigismember(&set, i), (i == SIGHUP), "sig %d %s to be member", i,
			      (i == SIGHUP) ? "expected" : "not expected");
	}

	zexpect_ok(sigaddset(&set, SIGSYS));
	for (int i = 1; i <= SIGRTMAX; i++) {
		zexpect_equal(sigismember(&set, i), (i == SIGHUP) || (i == SIGSYS),
			      "sig %d %s to be member", i,
			      ((i == SIGHUP) || (i == SIGSYS)) ? "expected" : "not expected");
	}

	if (IS_ENABLED(CONFIG_NEWLIB_LIBC) || IS_ENABLED(CONFIG_PICOLIBC)) {
		return;
	}

	/* SIGRTMIN is useful to test beyond the 32-bit boundary */
	zexpect_ok(sigaddset(&set, SIGRTMIN));
	for (int i = 1; i <= SIGRTMAX; i++) {
		zexpect_equal(sigismember(&set, i),
			      (i == SIGHUP) || (i == SIGSYS) || (i == SIGRTMIN),
			      "sig %d %s to be member", i,
			      ((i == SIGHUP) || (i == SIGSYS) || (i == SIGRTMIN)) ? "expected"
										  : "not expected");
	}

	/* SIGRTMAX is at the final boundary */
	zexpect_ok(sigaddset(&set, SIGRTMAX));
	for (int i = 1; i <= SIGRTMAX; i++) {
		zexpect_equal(sigismember(&set, i),
			      (i == SIGHUP) || (i == SIGSYS) || (i == SIGRTMIN) || (i == SIGRTMAX),
			      "sig %d %s to be member", i,
			      ((i == SIGHUP) || (i == SIGSYS) || (i == SIGRTMIN) || (i == SIGRTMAX))
				      ? "expected"
				      : "not expected");
	}

	zexpect_not_ok(sigaddset(&set, SIGRTMAX + 1));
}

ZTEST(signal, test_sigdelset)
{
	sigset_t set;

	sigfillset(&set);

	{
		if (false) {
			/* this breaks for newlib */
			/* zassert_equal(sigaddset(NULL, -1), -1, "rc should be -1"); */
		}

		/* degenerate cases */
		zexpect_equal(sigdelset(&set, -1), -1, "rc should be -1");
		zexpect_equal(errno, EINVAL, "errno should be %s", "EINVAL");

		zexpect_equal(sigdelset(&set, 0), -1, "rc should be -1");
		zexpect_equal(errno, EINVAL, "errno should be %s", "EINVAL");

		zexpect_equal(sigdelset(&set, SIGRTMAX + 1), -1, "rc should be -1");
		zexpect_equal(errno, EINVAL, "errno should be %s", "EINVAL");
	}

	zexpect_ok(sigdelset(&set, SIGHUP));
	for (int i = 1; i <= SIGRTMAX; i++) {
		zexpect_equal(sigismember(&set, i), !(i == SIGHUP), "sig %d %s to be member", i,
			      !(i == SIGHUP) ? "expected" : "not expected");
	}

	zexpect_ok(sigdelset(&set, SIGSYS));
	for (int i = 1; i <= SIGRTMAX; i++) {
		zexpect_equal(sigismember(&set, i), !((i == SIGHUP) || (i == SIGSYS)),
			      "sig %d %s to be member", i,
			      !((i == SIGHUP) || (i == SIGSYS)) ? "expected" : "not expected");
	}

	if (IS_ENABLED(CONFIG_NEWLIB_LIBC) || IS_ENABLED(CONFIG_PICOLIBC)) {
		return;
	}

	/* SIGRTMIN is useful to test beyond the 32-bit boundary */
	zexpect_ok(sigdelset(&set, SIGRTMIN));
	for (int i = 1; i <= SIGRTMAX; i++) {
		zexpect_equal(
			sigismember(&set, i), !((i == SIGHUP) || (i == SIGSYS) || (i == SIGRTMIN)),
			"sig %d %s to be member", i,
			!((i == SIGHUP) || (i == SIGSYS) || (i == SIGRTMIN)) ? "expected"
									     : "not expected");
	}

	/* SIGRTMAX is at the final boundary */
	zexpect_ok(sigdelset(&set, SIGRTMAX));
	for (int i = 1; i <= SIGRTMAX; i++) {
		zexpect_equal(
			sigismember(&set, i),
			!((i == SIGHUP) || (i == SIGSYS) || (i == SIGRTMIN) || (i == SIGRTMAX)),
			"sig %d %s to be member", i,
			!((i == SIGHUP) || (i == SIGSYS) || (i == SIGRTMIN) || (i == SIGRTMAX))
				? "expected"
				: "not expected");
	}

	zexpect_not_ok(sigdelset(&set, SIGRTMAX + 1));
}

ZTEST(signal, test_sigismember)
{
	sigset_t set;

	sigemptyset(&set);

	{
		/* degenerate cases */
		zassert_equal(sigismember(&set, -1), -1, "rc should be -1");
		zassert_equal(errno, EINVAL, "errno should be %s", "EINVAL");

		zassert_equal(sigismember(&set, 0), -1, "rc should be -1");
		zassert_equal(errno, EINVAL, "errno should be %s", "EINVAL");

		zassert_equal(sigismember(&set, SIGRTMAX + 1), -1, "rc should be -1");
		zassert_equal(errno, EINVAL, "errno should be %s", "EINVAL");
	}
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

static void before(void *arg)
{
	ARG_UNUSED(arg);

	if (!IS_ENABLED(CONFIG_DYNAMIC_THREAD)) {
		/* skip redundant testing if there is no thread pool / heap allocation */
		ztest_test_skip();
	}
}

ZTEST_SUITE(signal, NULL, NULL, before, NULL, NULL);
