/*
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/util.h>
#include <stdint.h>

#include <signal.h>

ZTEST(posix_signal_apis, test_posix_signal_fillset)
{
	sigset_t set;

	for (int i = 0; i < ARRAY_SIZE(set._elem); i++) {
		set._elem[i] = 0u;
	}

	zassert_ok(sigfillset(&set));

	for (int i = 0; i < ARRAY_SIZE(set._elem); i++) {
		zassert_equal(set._elem[i], UINT32_MAX, "signal is not filled");
	}
}
