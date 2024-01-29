/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "_common.h"

#ifdef CONFIG_POSIX_API
#include <sched.h>
#else
#include <zephyr/posix/sched.h>
#endif

/**
 * @brief existence test for `<sched.h>`
 *
 * @see <a href="https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/sched.h.html">sched.h</a>
 */
ZTEST(posix_headers, test_sched_h)
{
	zassert_not_equal(-1, offsetof(struct sched_param, sched_priority));

	zassert_not_equal(-1, SCHED_FIFO);
	zassert_not_equal(-1, SCHED_RR);
	/* zassert_not_equal(-1, SCHED_SPORADIC); */ /* not implemented */
	/* zassert_not_equal(-1, SCHED_OTHER); */ /* not implemented */

	if (IS_ENABLED(CONFIG_POSIX_API)) {
		zassert_not_null(sched_get_priority_max);
		zassert_not_null(sched_get_priority_min);

		zassert_not_null(sched_getparam);
		zassert_not_null(sched_getscheduler);

		/* zassert_not_null(sched_rr_get_interval); */ /* not implemented */

		zassert_not_null(sched_setparam);
		zassert_not_null(sched_setscheduler);

		zassert_not_null(sched_yield);
	}
}
