/*
 * Copyright (c) 2024 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "_common.h"

#ifdef CONFIG_POSIX_API
#include <aio.h>
#else
#include <zephyr/posix/aio.h>
#endif

ZTEST(posix_headers, test_aio_h)
{
	zassert_not_equal(offsetof(struct aiocb, aio_fildes), -1);
	zassert_not_equal(offsetof(struct aiocb, aio_offset), -1);
	zassert_not_equal(offsetof(struct aiocb, aio_buf), -1);
	zassert_not_equal(offsetof(struct aiocb, aio_nbytes), -1);
	zassert_not_equal(offsetof(struct aiocb, aio_reqprio), -1);
	zassert_not_equal(offsetof(struct aiocb, aio_sigevent), -1);
	zassert_not_equal(offsetof(struct aiocb, aio_lio_opcode), -1);

	if (IS_ENABLED(CONFIG_POSIX_API)) {
		zassert_not_null(aio_cancel);
		zassert_not_null(aio_error);
		zassert_not_null(aio_fsync);
		zassert_not_null(aio_read);
		zassert_not_null(aio_return);
		zassert_not_null(aio_suspend);
		zassert_not_null(aio_write);
		zassert_not_null(lio_listio);
	}
}
