/*
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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
