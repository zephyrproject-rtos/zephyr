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

void test_getnameinfo_ipv4(void)
{
	struct sockaddr_in saddr;
	char host[80];
	char serv[10];
	int ret;

	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;

	ret = getnameinfo((struct sockaddr *)&saddr, sizeof(saddr),
			  host, sizeof(host), serv, sizeof(serv), 0);
	zassert_equal(ret, 0, "");

	printk("%s %s\n", host, serv);
	zassert_equal(strcmp(host, "0.0.0.0"), 0, "");
	zassert_equal(strcmp(serv, "0"), 0, "");

	saddr.sin_port = htons(1234);
	saddr.sin_addr.s_addr = htonl(0x7f000001);

	ret = getnameinfo((struct sockaddr *)&saddr, sizeof(saddr),
			  host, sizeof(host), serv, sizeof(serv), 0);
	zassert_equal(ret, 0, "");

	printk("%s %s\n", host, serv);
	zassert_equal(strcmp(host, "127.0.0.1"), 0, "");
	zassert_equal(strcmp(serv, "1234"), 0, "");
}

void test_getnameinfo_ipv6(void)
{
	struct sockaddr_in6 saddr;
	char host[80];
	char serv[10];
	int ret;

	memset(&saddr, 0, sizeof(saddr));
	saddr.sin6_family = AF_INET6;

	ret = getnameinfo((struct sockaddr *)&saddr, sizeof(saddr),
			  host, sizeof(host), serv, sizeof(serv), 0);
	zassert_equal(ret, 0, "");

	printk("%s %s\n", host, serv);
	zassert_equal(strcmp(host, "::"), 0, "");
	zassert_equal(strcmp(serv, "0"), 0, "");

	saddr.sin6_port = htons(4321);
	saddr.sin6_addr.s6_addr[0] = 0xff;
	saddr.sin6_addr.s6_addr[1] = 0x55;
	saddr.sin6_addr.s6_addr[15] = 0x11;

	ret = getnameinfo((struct sockaddr *)&saddr, sizeof(saddr),
			  host, sizeof(host), serv, sizeof(serv), 0);
	zassert_equal(ret, 0, "");

	printk("%s %s\n", host, serv);
	zassert_equal(strcmp(host, "ff55::11"), 0, "");
	zassert_equal(strcmp(serv, "4321"), 0, "");
}

void test_main(void)
{
	k_thread_system_pool_assign(k_current_get());

	ztest_test_suite(socket_getnameinfo,
			 ztest_user_unit_test(test_getnameinfo_ipv4),
			 ztest_user_unit_test(test_getnameinfo_ipv6));

	ztest_run_test_suite(socket_getnameinfo);
}
