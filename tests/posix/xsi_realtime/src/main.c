/*
 * Copyright (c) 2024 Marvin Ouma <pancakesdeath@protonmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

static void before(void *arg)
{
	ARG_UNUSED(arg);

	if (!IS_ENABLED(CONFIG_DYNAMIC_THREAD)) {
		/* skip redundant testing if there is no thread pool / heap allocation */
		ztest_test_skip();
	}
}

ZTEST_SUITE(xsi_realtime, NULL, NULL, before, NULL, NULL);
