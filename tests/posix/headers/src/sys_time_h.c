/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "_common.h"

#ifdef CONFIG_POSIX_API
#include <sys/time.h>
#else
#include <zephyr/posix/sys/time.h>
#endif

/**
 * @brief existence test for `<sys/time.h>`
 *
 * @see <a href="https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/sys_time.h.html">sys/time.h</a>
 */
ZTEST(posix_headers, test_sys_time_h)
{
	zassert_not_equal(-1, offsetof(struct timeval, tv_sec));
	zassert_not_equal(-1, offsetof(struct timeval, tv_usec));

	/* zassert_not_equal(-1, offsetof(struct itimerval, it_interval)); */ /* not implemented */
	/* zassert_not_equal(-1, offsetof(struct itimerval, it_value)); */ /* not implemented */

	/* zassert_not_equal(-1, ITIMER_REAL); */ /* not implemented */
	/* zassert_not_equal(-1, ITIMER_VIRTUAL); */ /* not implemented */
	/* zassert_not_equal(-1, ITIMER_PROF); */ /* not implemented */

	if (IS_ENABLED(CONFIG_POSIX_API)) {
		/* zassert_not_null(getitimer); */ /* not implemented */
		zassert_not_null(gettimeofday);
		/* zassert_not_null(setitimer); */ /* not implemented */
		/* zassert_not_null(utimes); */ /* not implemented */
	}
}
