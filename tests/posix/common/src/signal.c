/*
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <signal.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

ZTEST(posix_apis, test_signal_emptyset)
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

ZTEST(posix_apis, test_signal_fillset)
{
	sigset_t set = (sigset_t){0};

	zassert_ok(sigfillset(&set));

	for (int i = 0; i < ARRAY_SIZE(set.sig); i++) {
		zassert_equal(set.sig[i], -1, "set.sig[%d] is not filled: 0x%lx", i, set.sig[i]);
	}
}

ZTEST(posix_apis, test_signal_addset_oor)
{
	sigset_t set = (sigset_t){0};

	zassert_equal(sigaddset(&set, -1), -1, "rc should be -1");
	zassert_equal(errno, EINVAL, "errno should be %s", "EINVAL");

	zassert_equal(sigaddset(&set, 0), -1, "rc should be -1");
	zassert_equal(errno, EINVAL, "errno should be %s", "EINVAL");

	zassert_equal(sigaddset(&set, _NSIG), -1, "rc should be -1");
	zassert_equal(errno, EINVAL, "errno should be %s", "EINVAL");
}

ZTEST(posix_apis, test_signal_addset)
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
