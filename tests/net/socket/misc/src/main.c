/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <stdio.h>
#include <ztest_assert.h>

#include <net/socket.h>

#include "../../socket_helpers.h"

void test_gethostname(void)
{
	static ZTEST_BMEM char buf[80];
	int res;

	res = gethostname(buf, sizeof(buf));
	zassert_equal(res, 0, "");
	printk("%s\n", buf);
	zassert_equal(strcmp(buf, "ztest_hostname"), 0, "");
}

void test_inet_pton(void)
{
	int res;
	u8_t buf[32];

	res = inet_pton(AF_INET, "127.0.0.1", buf);
	zassert_equal(res, 1, "");

	res = inet_pton(AF_INET, "127.0.0.1a", buf);
	zassert_equal(res, 0, "");

	res = inet_pton(AF_INET6, "a:b:c:d:0:1:2:3", buf);
	zassert_equal(res, 1, "");

	res = inet_pton(AF_INET6, "::1", buf);
	zassert_equal(res, 1, "");

	res = inet_pton(AF_INET6, "1::", buf);
	zassert_equal(res, 1, "");

	res = inet_pton(AF_INET6, "a:b:c:d:0:1:2:3z", buf);
	zassert_equal(res, 0, "");
}

void test_main(void)
{
	k_thread_system_pool_assign(k_current_get());

	ztest_test_suite(socket_misc,
			 ztest_user_unit_test(test_gethostname),
			 ztest_user_unit_test(test_inet_pton));

	ztest_run_test_suite(socket_misc);
}
