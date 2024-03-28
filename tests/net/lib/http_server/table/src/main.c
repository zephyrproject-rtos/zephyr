/*
 * Copyright (c) 2023, Emna Rekik
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/http/hpack.h>
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
	zassert_equal(-EINVAL, _get_fun(NULL, HTTP_SERVER_HPACK_INVALID, value));
	/* 01 */
	zassert_equal(-EINVAL, _get_fun(NULL, HTTP_SERVER_HPACK_AUTHORITY, value));
	/* 10 */
	zassert_equal(-EINVAL, _get_fun(&server, HTTP_SERVER_HPACK_INVALID, value));

	for (int i = HTTP_SERVER_HPACK_AUTHORITY; i <= HTTP_SERVER_HPACK_WWW_AUTHENTICATE; i++) {
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

ZTEST(http_hpack, test_contains)
{
	test_get_common(NULL);
}

ZTEST(http_hpack, test_get)
{
	void *value;

	test_get_common(&value);
}

ZTEST(http_hpack, test_remove)
{
}

ZTEST(http_hpack, test_add)
{
}

ZTEST(http_hpack, test_path)
{
	struct http_client_ctx ctx;
	uint8_t payload[] = {HTTP_SERVER_HPACK_PATH_ROOT_SYMBOL};

	ctx.current_frame.payload = payload;
	ctx.current_frame.length = sizeof(payload);

	char *result = http_hpack_parse_header(&ctx, HTTP_SERVER_HPACK_PATH);

	zassert_true(strcmp(result, "/") == 0, "Expected /");
}

ZTEST(http_hpack, test_method)
{
	struct http_client_ctx ctx;
	uint8_t payload[] = {HTTP_SERVER_HPACK_METHOD_GET_SYMBOL};

	ctx.current_frame.payload = payload;
	ctx.current_frame.length = sizeof(payload);

	char *result = http_hpack_parse_header(&ctx, HTTP_SERVER_HPACK_METHOD);

	zassert_true(strcmp(result, "GET") == 0, "Expected GET");
}

ZTEST_SUITE(http_hpack, NULL, NULL, NULL, NULL, NULL);
