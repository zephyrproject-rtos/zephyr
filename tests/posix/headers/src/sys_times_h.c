/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "_common.h"

#ifdef CONFIG_POSIX_API
#include <sys/times.h>
#else
#include <zephyr/posix/sys/times.h>
#endif

/**
 * @brief existence test for `<sys/times.h>`
 *
 * @see <a href="https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/sys_times.h.html">sys/time.h</a>
 */
ZTEST(posix_headers, test_sys_times_h)
{
	zassert_not_equal(-1, offsetof(struct tms, tms_utime));
	zassert_equal(sizeof(((struct tms *)0)->tms_utime), sizeof(clock_t));
	zassert_not_equal(-1, offsetof(struct tms, tms_stime));
	zassert_equal(sizeof(((struct tms *)0)->tms_stime), sizeof(clock_t));
	zassert_not_equal(-1, offsetof(struct tms, tms_cutime));
	zassert_equal(sizeof(((struct tms *)0)->tms_cutime), sizeof(clock_t));
	zassert_not_equal(-1, offsetof(struct tms, tms_cstime));
	zassert_equal(sizeof(((struct tms *)0)->tms_cstime), sizeof(clock_t));

	if (IS_ENABLED(CONFIG_POSIX_API)) {
		zassert_not_null(times);
	}
}
