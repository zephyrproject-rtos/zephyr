/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "_common.h"

#ifdef CONFIG_POSIX_API
#include <sys/select.h>
#else
#include <zephyr/posix/sys/select.h>
#endif

/**
 * @brief existence test for `<sys/select.h>`
 *
 * @see <a href="https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/sys_select.h.html">sys/select.h</a>
 */
ZTEST(posix_headers, test_sys_select_h)
{
	fd_set fds = {0};

	zassert_not_equal(-1, FD_SETSIZE);
	FD_CLR(0, &fds);
	FD_ISSET(0, &fds);
	FD_SET(0, &fds);
	FD_ZERO(&fds);

	if (IS_ENABLED(CONFIG_POSIX_API)) {
		/* zassert_not_null(pselect); */ /* not implemented */
		zassert_not_null(select);
	}
}
