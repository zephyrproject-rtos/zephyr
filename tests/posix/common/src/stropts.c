/*
 * Copyright (c) 2024 Abhinav Srivastava
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <stropts.h>
#include <errno.h>

ZTEST(stropts, test_putmsg)
{
	const struct strbuf *ctrl = NULL;
	const struct strbuf *data = NULL;
	int fd = -1;
	int ret = putmsg(fd, ctrl, data, 0);

	zassert_equal(ret, -1, "Expected return value -1, got %d", ret);
	zassert_equal(errno, ENOSYS, "Expected errno ENOSYS, got %d", errno);
}

ZTEST_SUITE(stropts, NULL, NULL, NULL, NULL, NULL);
