/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "_common.h"

#ifdef CONFIG_POSIX_API
#include <sys/eventfd.h>
#else
#include <zephyr/posix/sys/eventfd.h>
#endif

/**
 * @brief existence test for `<sys/eventfd.h>`
 *
 * @note the eventfd API is not (yet) a part of POSIX.
 *
 * @see <a href="https://man7.org/linux/man-pages/man2/eventfd.2.html">sys/eventfd.h</a>
 */
ZTEST(posix_headers, test_sys_eventfd_h)
{
	/* zassert_not_equal(-1, EFD_CLOEXEC); */ /* not implemented */
	zassert_not_equal(-1, EFD_NONBLOCK);
	zassert_not_equal(-1, EFD_SEMAPHORE);

	zassert_not_equal((eventfd_t)-1, (eventfd_t)0);

	if (IS_ENABLED(CONFIG_POSIX_API)) {
		zassert_not_null(eventfd);
		zassert_not_null(eventfd_read);
		zassert_not_null(eventfd_write);
	}
}
