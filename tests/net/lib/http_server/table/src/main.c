/*
 * Copyright (c) 2023, Emna Rekik
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/http/table.h>
#include <zephyr/net/http/server.h>
#include <zephyr/ztest.h>

#define _get_fun(s, k, v)                                                                          \
	((v) == NULL ? http_hpack_table_contains(s, k) : http_hpack_table_get(s, k, v))

struct table_fixture {
};

static void test_get_common(void **value)
{
	struct http_client_ctx server = {};
	int ret;

	/* 00 */
	zassert_equal(-EINVAL, _get_fun(NULL, HTTP_HPACK_INVALID, value));
	/* 01 */
	zassert_equal(-EINVAL, _get_fun(NULL, HTTP_HPACK_AUTHORITY, value));
	/* 10 */
	zassert_equal(-EINVAL, _get_fun(&server, HTTP_HPACK_INVALID, value));

	for (int i = HTTP_HPACK_AUTHORITY; i <= HTTP_HPACK_WWW_AUTHENTICATE; i++) {
		if (value != NULL) {
			*value = NULL;
		}
		ret = _get_fun(&server, i, value);
		zassert_ok(ret, "%s() failed %d",
			   value == NULL ? "http_table_contains" : "http_table_get", ret);
		if (value != NULL) {
			zassert_not_null(*value);
		}
	}
}

ZTEST(table, test_contains)
{
	test_get_common(NULL);
}

ZTEST(table, test_get)
{
	void *value;

	test_get_common(&value);
}

ZTEST(table, test_remove)
{
}

ZTEST(table, test_add)
{
}

static void after(void *arg)
{
}

static void before(void *arg)
{
}

ZTEST_SUITE(table, NULL, NULL, before, after, NULL);
