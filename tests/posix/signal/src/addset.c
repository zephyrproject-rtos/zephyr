/*
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <signal.h>
#include <errno.h>

#include <zephyr/ztest.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

ZTEST(posix_signal_apis, test_posix_signal_addset)
{
	sigset_t set;
	int signo;
	int rc;

	for (int i = 0; i < ARRAY_SIZE(set._elem); i++) {
		set._elem[i] = 0u;
	}

	signo = 21;
	zassert_ok(sigaddset(&set, signo));
	zassert_equal(set._elem[signo >> 5], BIT(signo), "Signal %d is not set", signo);

	signo = 42;
	zassert_ok(sigaddset(&set, signo));
	zassert_equal(set._elem[21 >> 5], BIT(21), "Signal %d is not set", 21);
	zassert_equal(set._elem[signo >> 5], BIT(signo % (8 * sizeof(set._elem[0]))),
		      "Signal %d is not set", signo);

	rc = sigaddset(&set, 0);
	zassert_equal(rc, -1, "rc should be -1, not %d", rc);
	zassert_equal(errno, EINVAL, "errno should be EINVAL, not %d", errno);

	rc = sigaddset(&set, NSIG);
	zassert_equal(rc, -1, "rc should be -1, not %d", rc);
	zassert_equal(errno, EINVAL, "errno should be EINVAL, not %d", errno);
}
