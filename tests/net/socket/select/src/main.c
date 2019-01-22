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

void test_fd_set(void)
{
	fd_set set;

	/* Relies on specific value of CONFIG_POSIX_MAX_FDS in prj.conf */
	zassert_equal(sizeof(set.bitset), sizeof(u32_t) * 2, "");

	FD_ZERO(&set);
	zassert_equal(set.bitset[0], 0, "");
	zassert_equal(set.bitset[1], 0, "");
	zassert_false(FD_ISSET(0, &set), "");

	FD_SET(0, &set);
	zassert_true(FD_ISSET(0, &set), "");

	FD_CLR(0, &set);
	zassert_false(FD_ISSET(0, &set), "");

	FD_SET(0, &set);
	zassert_equal(set.bitset[0], 0x00000001, "");
	zassert_equal(set.bitset[1], 0, "");

	FD_SET(31, &set);
	zassert_equal(set.bitset[0], 0x80000001, "");
	zassert_equal(set.bitset[1], 0, "");

	FD_SET(33, &set);
	zassert_equal(set.bitset[0], 0x80000001, "");
	zassert_equal(set.bitset[1], 0x00000002, "");

	FD_ZERO(&set);
	zassert_equal(set.bitset[0], 0, "");
	zassert_equal(set.bitset[1], 0, "");
}

void test_main(void)
{
	k_thread_system_pool_assign(k_current_get());

	ztest_test_suite(socket_select,
			 ztest_user_unit_test(test_fd_set));

	ztest_run_test_suite(socket_select);
}
