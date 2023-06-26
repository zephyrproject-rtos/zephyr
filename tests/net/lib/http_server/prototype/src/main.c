/*
 * Copyright (c) 2023, Emna Rekik
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/net/socket.h>
#include <string.h>

#include "headers/server_functions.h"

ZTEST_SUITE(server_function_tests, NULL, NULL, NULL, NULL, NULL);

ZTEST(server_function_tests, test_on_url)
{
	struct http_parser parser;
	const char *at = "/test HTTP/1.1";
	size_t length = 5;

	const char *url = on_url(&parser, at, length);

	zassert_mem_equal(url, "/test", 5, "URL was not stored correctly");
}
