/*
 * Copyright (c) 2023, Meta
 * Copyright (c) 2024, Adam Wojasinski <awojasinski@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include <zephyr/posix/unistd.h>

ZTEST(posix_single_process, test_posix_sysconf)
{
	long ret;

	/* SC that's implemented */
	ret = sysconf(_SC_VERSION);
	zassert_equal(ret, _POSIX_VERSION, "sysconf returned unexpected value %ld", ret);

	/* SC that's not implemented */
	ret = sysconf(_SC_MEMLOCK_RANGE);
	zassert_equal(ret, -1, "sysconf returned unexpected value %ld", ret);

	/* SC that value depends on target's configuration */
	ret = sysconf(_SC_SEMAPHORES);
	if (IS_ENABLED(CONFIG_POSIX_THREADS)) {
		zassert_equal(ret, _POSIX_VERSION, "sysconf returned unexpected value %ld", ret);
	} else {
		zassert_equal(ret, -1L, "sysconf returned unexpected value %ld", ret);
	}
}
