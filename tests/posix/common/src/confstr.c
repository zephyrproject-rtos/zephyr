/*
 * Copyright (c) 2024, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <unistd.h>

#include <zephyr/ztest.h>
#include <zephyr/sys/util.h>

ZTEST(confstr, test_confstr)
{
	char buf[1];

	/* degenerate cases */
	{
		struct arg {
			int name;
			char *buf;
			size_t len;
		};

		const struct arg arg1s[] = {
			{-1, NULL, 0},
			{-1, NULL, sizeof(buf)},
			{-1, buf, 0},
			{-1, buf, sizeof(buf)},
		};

		const struct arg arg2s[] = {
			{_CS_PATH, NULL, 0},
			{_CS_PATH, buf, 0},
		};

		const struct arg arg3s[] = {
			{_CS_PATH, NULL, sizeof(buf)},
		};

		ARRAY_FOR_EACH_PTR(arg1s, arg) {
			errno = 0;
			zassert_equal(0, confstr(arg->name, arg->buf, arg->len));
			zassert_equal(errno, EINVAL);
		}

		ARRAY_FOR_EACH_PTR(arg2s, arg) {
			errno = 0;
			buf[0] = 0xff;
			zassert_true(confstr(arg->name, arg->buf, arg->len) > 0);
			zassert_equal(errno, 0);
			zassert_equal((uint8_t)buf[0], 0xff);
		}

		ARRAY_FOR_EACH_PTR(arg3s, arg) {
			errno = 0;
			zassert_true(confstr(arg->name, arg->buf, arg->len) > 0);
			zassert_equal(errno, 0);
		}
	}

	buf[0] = 0xff;
	zassert_true(confstr(_CS_PATH, buf, sizeof(buf) > 0));
	zassert_equal(buf[0], '\0');
}

ZTEST_SUITE(confstr, NULL, NULL, NULL, NULL, NULL);
