/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <stdio.h>
#include <ztest_assert.h>

#include <net/socket.h>

APPMEM_PARTITION_OBJECT_DEFINE(libc)
APPMEM_PARTITION_HEADER_DEFINE(libc)

void test_getaddrinfo_ok(void)
{
	struct addrinfo *res = NULL;
	int ret;

	ret = getaddrinfo("www.zephyrproject.org", NULL, NULL, &res);

	/* Without a local dnsmasq server this request will be canceled. */
	zassert_equal(ret, DNS_EAI_CANCELED, "Invalid result");

	/* With a local dnsmasq server this request shall return 0. */
	/* zassert_equal(ret, 0, "Invalid result"); */

	freeaddrinfo(res);
}

void test_getaddrinfo_no_host(void)
{
	struct addrinfo *res = NULL;
	int ret;

	ret = getaddrinfo(NULL, NULL, NULL, &res);

	zassert_equal(ret, DNS_EAI_SYSTEM, "Invalid result");
	zassert_equal(errno, EINVAL, "Invalid errno");
	zassert_is_null(res, "ai_addr is not NULL");

	freeaddrinfo(res);
}

void test_main(void)
{
#ifdef CONFIG_APP_SHARED_MEM
	APPMEM_INIT_PARTITIONS(libc);
	appmem_init_app_memory();
	appmem_add_part_ztest_dom0(libc);
#endif	/* CONFIG_APP_SHARED_MEM */

	k_thread_system_pool_assign(k_current_get());

	ztest_test_suite(socket_getaddrinfo,
			 ztest_user_unit_test(test_getaddrinfo_ok),
			 ztest_user_unit_test(test_getaddrinfo_no_host));

	ztest_run_test_suite(socket_getaddrinfo);
}
