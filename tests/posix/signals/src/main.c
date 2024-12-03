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

ZTEST(posix_signals, test_sigemptyset)
{
	sigset_t set;

	for (int i = 0; i < ARRAY_SIZE(set.sig); i++) {
		set.sig[i] = -1;
	}

	zassert_ok(sigemptyset(&set));

	for (int i = 0; i < ARRAY_SIZE(set.sig); i++) {
		zassert_equal(set.sig[i], 0u, "set.sig[%d] is not empty: 0x%lx", i, set.sig[i]);
	}
}

ZTEST(posix_signals, test_sigfillset)
{
	sigset_t set = (sigset_t){0};

	zassert_ok(sigfillset(&set));

	for (int i = 0; i < ARRAY_SIZE(set.sig); i++) {
		zassert_equal(set.sig[i], -1, "set.sig[%d] is not filled: 0x%lx", i, set.sig[i]);
	}
}

ZTEST(posix_signals, test_sigaddset_oor)
{
	sigset_t set = (sigset_t){0};

	zassert_equal(sigaddset(&set, -1), -1, "rc should be -1");
	zassert_equal(errno, EINVAL, "errno should be %s", "EINVAL");

	zassert_equal(sigaddset(&set, 0), -1, "rc should be -1");
	zassert_equal(errno, EINVAL, "errno should be %s", "EINVAL");

	zassert_equal(sigaddset(&set, _NSIG), -1, "rc should be -1");
	zassert_equal(errno, EINVAL, "errno should be %s", "EINVAL");
}

ZTEST(posix_signals, test_sigaddset)
{
	int signo;
	sigset_t set = (sigset_t){0};
	sigset_t target = (sigset_t){0};

	signo = SIGHUP;
	zassert_ok(sigaddset(&set, signo));
	WRITE_BIT(target.sig[0], signo, 1);
	for (int i = 0; i < ARRAY_SIZE(set.sig); i++) {
		zassert_equal(set.sig[i], target.sig[i],
			      "set.sig[%d of %d] has content: %lx, expected %lx", i,
			      ARRAY_SIZE(set.sig) - 1, set.sig[i], target.sig[i]);
	}

	signo = SIGSYS;
	zassert_ok(sigaddset(&set, signo));
	WRITE_BIT(target.sig[0], signo, 1);
	for (int i = 0; i < ARRAY_SIZE(set.sig); i++) {
		zassert_equal(set.sig[i], target.sig[i],
			      "set.sig[%d of %d] has content: %lx, expected %lx", i,
			      ARRAY_SIZE(set.sig) - 1, set.sig[i], target.sig[i]);
	}

	signo = SIGRTMIN; /* >=32, will be in the second sig set for 32bit */
	zassert_ok(sigaddset(&set, signo));
#ifdef CONFIG_64BIT
	WRITE_BIT(target.sig[0], signo, 1);
#else /* 32BIT */
	WRITE_BIT(target.sig[1], (signo)-BITS_PER_LONG, 1);
#endif
	for (int i = 0; i < ARRAY_SIZE(set.sig); i++) {
		zassert_equal(set.sig[i], target.sig[i],
			      "set.sig[%d of %d] has content: %lx, expected %lx", i,
			      ARRAY_SIZE(set.sig) - 1, set.sig[i], target.sig[i]);
	}

	signo = SIGRTMAX;
	zassert_ok(sigaddset(&set, signo));
	WRITE_BIT(target.sig[signo / BITS_PER_LONG], signo % BITS_PER_LONG, 1);
	for (int i = 0; i < ARRAY_SIZE(set.sig); i++) {
		zassert_equal(set.sig[i], target.sig[i],
			      "set.sig[%d of %d] has content: %lx, expected %lx", i,
			      ARRAY_SIZE(set.sig) - 1, set.sig[i], target.sig[i]);
	}
}

ZTEST(posix_signals, test_sigdelset_oor)
{
	sigset_t set = (sigset_t){0};

	zassert_equal(sigdelset(&set, -1), -1, "rc should be -1");
	zassert_equal(errno, EINVAL, "errno should be %s", "EINVAL");

	zassert_equal(sigdelset(&set, 0), -1, "rc should be -1");
	zassert_equal(errno, EINVAL, "errno should be %s", "EINVAL");

	zassert_equal(sigdelset(&set, _NSIG), -1, "rc should be -1");
	zassert_equal(errno, EINVAL, "errno should be %s", "EINVAL");
}

ZTEST(posix_signals, test_sigdelset)
{
	int signo;
	sigset_t set = (sigset_t){0};
	sigset_t target = (sigset_t){0};

	signo = SIGHUP;
	zassert_ok(sigdelset(&set, signo));
	WRITE_BIT(target.sig[0], signo, 0);
	for (int i = 0; i < ARRAY_SIZE(set.sig); i++) {
		zassert_equal(set.sig[i], target.sig[i],
			      "set.sig[%d of %d] has content: %lx, expected %lx", i,
			      ARRAY_SIZE(set.sig) - 1, set.sig[i], target.sig[i]);
	}

	signo = SIGSYS;
	zassert_ok(sigdelset(&set, signo));
	WRITE_BIT(target.sig[0], signo, 0);
	for (int i = 0; i < ARRAY_SIZE(set.sig); i++) {
		zassert_equal(set.sig[i], target.sig[i],
			      "set.sig[%d of %d] has content: %lx, expected %lx", i,
			      ARRAY_SIZE(set.sig) - 1, set.sig[i], target.sig[i]);
	}

	signo = SIGRTMIN; /* >=32, will be in the second sig set for 32bit */
	zassert_ok(sigdelset(&set, signo));
#ifdef CONFIG_64BIT
	WRITE_BIT(target.sig[0], signo, 0);
#else /* 32BIT */
	WRITE_BIT(target.sig[1], (signo)-BITS_PER_LONG, 0);
#endif
	for (int i = 0; i < ARRAY_SIZE(set.sig); i++) {
		zassert_equal(set.sig[i], target.sig[i],
			      "set.sig[%d of %d] has content: %lx, expected %lx", i,
			      ARRAY_SIZE(set.sig) - 1, set.sig[i], target.sig[i]);
	}

	signo = SIGRTMAX;
	zassert_ok(sigdelset(&set, signo));
	WRITE_BIT(target.sig[signo / BITS_PER_LONG], signo % BITS_PER_LONG, 0);
	for (int i = 0; i < ARRAY_SIZE(set.sig); i++) {
		zassert_equal(set.sig[i], target.sig[i],
			      "set.sig[%d of %d] has content: %lx, expected %lx", i,
			      ARRAY_SIZE(set.sig) - 1, set.sig[i], target.sig[i]);
	}
}

ZTEST(posix_signals, test_sigismember_oor)
{
	sigset_t set = {0};

	zassert_equal(sigismember(&set, -1), -1, "rc should be -1");
	zassert_equal(errno, EINVAL, "errno should be %s", "EINVAL");

	zassert_equal(sigismember(&set, 0), -1, "rc should be -1");
	zassert_equal(errno, EINVAL, "errno should be %s", "EINVAL");

	zassert_equal(sigismember(&set, _NSIG), -1, "rc should be -1");
	zassert_equal(errno, EINVAL, "errno should be %s", "EINVAL");
}

ZTEST(posix_signals, test_sigismember)
{
	sigset_t set = (sigset_t){0};

#ifdef CONFIG_64BIT
	set.sig[0] = BIT(SIGHUP) | BIT(SIGSYS) | BIT(SIGRTMIN);
#else /* 32BIT */
	set.sig[0] = BIT(SIGHUP) | BIT(SIGSYS);
	set.sig[1] = BIT((SIGRTMIN)-BITS_PER_LONG);
#endif
	WRITE_BIT(set.sig[SIGRTMAX / BITS_PER_LONG], SIGRTMAX % BITS_PER_LONG, 1);

	zassert_equal(sigismember(&set, SIGHUP), 1, "%s expected to be member", "SIGHUP");
	zassert_equal(sigismember(&set, SIGSYS), 1, "%s expected to be member", "SIGSYS");
	zassert_equal(sigismember(&set, SIGRTMIN), 1, "%s expected to be member", "SIGRTMIN");
	zassert_equal(sigismember(&set, SIGRTMAX), 1, "%s expected to be member", "SIGRTMAX");

	zassert_equal(sigismember(&set, SIGKILL), 0, "%s not expected to be member", "SIGKILL");
	zassert_equal(sigismember(&set, SIGTERM), 0, "%s not expected to be member", "SIGTERM");
}

ZTEST(posix_signals, test_signal_strsignal)
{
	/* Using -INT_MAX here because compiler resolves INT_MIN to (-2147483647 - 1) */
	char buf[sizeof("RT signal -" STRINGIFY(INT_MAX))] = {0};

	zassert_mem_equal(strsignal(-1), "Invalid signal", sizeof("Invalid signal"));
	zassert_mem_equal(strsignal(0), "Invalid signal", sizeof("Invalid signal"));
	zassert_mem_equal(strsignal(_NSIG), "Invalid signal", sizeof("Invalid signal"));

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

ZTEST(posix_signals, test_pthread_sigmask)
{
	pthread_t th;

	zassert_ok(pthread_create(&th, NULL, test_sigmask_entry, pthread_sigmask));
	zassert_ok(pthread_join(th, NULL));
}

ZTEST(posix_signals, test_sigprocmask)
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

ZTEST_SUITE(posix_signals, NULL, NULL, before, NULL, NULL);
