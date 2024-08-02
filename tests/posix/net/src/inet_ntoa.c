/*
 * Copyright (c) 2024, Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/posix/arpa/inet.h>
#include <zephyr/posix/netinet/in.h>

#include <zephyr/ztest.h>

ZTEST(net, test_inet_ntoa)
{
	struct in_addr in;

	in.s_addr = htonl(0x7f000001);
	zassert_mem_equal(inet_ntoa(in), "127.0.0.1", strlen("127.0.0.1") + 1);

	in.s_addr = htonl(0);
	zassert_mem_equal(inet_ntoa(in), "0.0.0.0", strlen("0.0.0.0") + 1);

	in.s_addr = htonl(0xffffffff);
	zassert_mem_equal(inet_ntoa(in), "255.255.255.255", strlen("255.255.255.255") + 1);
}
