/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "_common.h"

#ifdef CONFIG_POSIX_API
#include <poll.h>
#else
#include <zephyr/posix/poll.h>
#endif

/**
 * @brief existence test for `<poll.h>`
 *
 * @see <a href="https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/poll.h.html">poll.h</a>
 */
ZTEST(posix_headers, test_poll_h)
{
	zassert_not_equal(-1, offsetof(struct pollfd, fd));
	zassert_not_equal(-1, offsetof(struct pollfd, events));
	zassert_not_equal(-1, offsetof(struct pollfd, revents));

	/* zassert_true(sizeof(nfds_t) <= sizeof(long)); */ /* not implemented */

	zassert_not_equal(-1, POLLIN);
	/* zassert_not_equal(-1, POLLRDNORM); */ /* not implemented */
	/* zassert_not_equal(-1, POLLRDBAND); */ /* not implemented */
	/* zassert_not_equal(-1, POLLPRI); */ /* not implemented */
	zassert_not_equal(-1, POLLOUT);
	/* zassert_not_equal(-1, POLLWRNORM); */ /* not implemented */
	/* zassert_not_equal(-1, POLLWRBAND); */ /* not implemented */
	zassert_not_equal(-1, POLLERR);
	zassert_not_equal(-1, POLLHUP);
	zassert_not_equal(-1, POLLNVAL);

	if (IS_ENABLED(CONFIG_POSIX_API)) {
		zassert_not_null(poll);
	}
}
