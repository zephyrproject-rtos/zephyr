/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "_common.h"

#ifdef CONFIG_POSIX_API
#include <mqueue.h>
#else
#include <zephyr/posix/mqueue.h>
#endif

/**
 * @brief existence test for `<mqueue.h>`
 *
 * @see <a href="https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/mqueue.h.html">mqueue.h</a>
 */
ZTEST(posix_headers, test_mqueue_h)
{
	zassert_not_equal((mqd_t)-1, (mqd_t)NULL);

	zassert_not_equal(-1, offsetof(struct mq_attr, mq_flags));
	zassert_not_equal(-1, offsetof(struct mq_attr, mq_maxmsg));
	zassert_not_equal(-1, offsetof(struct mq_attr, mq_msgsize));
	zassert_not_equal(-1, offsetof(struct mq_attr, mq_curmsgs));

	if (IS_ENABLED(CONFIG_POSIX_API)) {
		zassert_not_null(mq_close);
		zassert_not_null(mq_getattr);
		zassert_not_null(mq_notify);
		zassert_not_null(mq_open);
		zassert_not_null(mq_receive);
		zassert_not_null(mq_send);
		zassert_not_null(mq_setattr);
		zassert_not_null(mq_timedreceive);
		zassert_not_null(mq_timedsend);
		zassert_not_null(mq_unlink);
	}
}
