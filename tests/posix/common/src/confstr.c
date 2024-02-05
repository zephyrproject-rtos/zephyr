/*
 * Copyright (c) 2024 Perrot Gaetan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/posix/unistd.h>
#include <errno.h>

ZTEST(posix_apis, test_posix_confstr)
{
	size_t ret;
	char *buf = NULL;

	ret = confstr(_CS_PATH, buf, 0);
	zassert_equal(ret, 0,"ztress_execute failed (ret: %d)", ret);
	zassert_not_equal(errno, EINVAL);

	ret = confstr(200, buf, 0);
	zassert_equal(ret, 0);
	zassert_equal(errno, EINVAL);
}
