/*
 * main.c - DNS Fuzz testing client
 *
 * Boots up and starts making DNS requests for www.zephyrproject.org until it
 * receives a TCP connection on port 4242, which is its signal to shutdown.
 *
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>

#include <ztest.h>

#include <net/ethernet.h>
#include <net/buf.h>
#include <net/net_ip.h>
#include <net/net_if.h>
#include <net/dns_resolve.h>

#define NET_LOG_ENABLED 1
#include "net_private.h"

#if defined(CONFIG_NET_DEBUG_DNS_RESOLVE)
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

#define NAME4 "4.zephyr.test"
#define NAME6 "6.zephyr.test"
#define NAME_IPV4 "192.0.2.1"
#define NAME_IPV6 "2001:db8::1"

/* DNS fuzzing client code */
#include <net/net_core.h>
#include <net/net_if.h>
#include <net/net_mgmt.h>
#include <net/dns_resolve.h>

const static char dns_fdqn[] = "www.zephyrproject.org";

static void do_ipv4_lookup();
static void dns_result_cb(enum dns_resolve_status status,
		   struct dns_addrinfo *info,
		   void *user_data);

#define DNS_TIMEOUT K_SECONDS(2)

static void do_ipv4_lookup()
{
	u16_t dns_id;
	int ret;

	ret = dns_get_addr_info(dns_fdqn,
				DNS_QUERY_TYPE_A,
				&dns_id,
				dns_result_cb,
				(void *)dns_fdqn,
				DNS_TIMEOUT);
	if (ret < 0) {
		NET_ERR("Cannot resolve IPv4 address (%d)", ret);
		return;
	}

	NET_DBG("DNS id %u", dns_id);
}

void dns_result_cb(enum dns_resolve_status status,
		   struct dns_addrinfo *info,
		   void *user_data)
{
	char hr_addr[NET_IPV6_ADDR_LEN];
	char *queried = user_data ? (char *)user_data : "<address unknown>";
	char *hr_family;
	void *addr;

	switch (status) {
	case DNS_EAI_CANCELED:
		TC_PRINT("DNS query was canceled: %s", queried);
		return;
	case DNS_EAI_FAIL:
		TC_PRINT("DNS resolve failed: %s", queried);
		return;
	case DNS_EAI_NODATA:
		TC_PRINT("Cannot resolve address: %s", queried);
		return;
	case DNS_EAI_ALLDONE:
		TC_PRINT("DNS resolving finished: %s", queried);
		return;
	case DNS_EAI_INPROGRESS:
		break;
	default:
		TC_PRINT("DNS resolving error (%d): %s", status, queried);
		return;
	}

	if (!info) {
		return;
	}

	if (info->ai_family == AF_INET) {
		hr_family = "IPv4";
		addr = &net_sin(&info->ai_addr)->sin_addr;
	} else if (info->ai_family == AF_INET6) {
		hr_family = "IPv6";
		addr = &net_sin6(&info->ai_addr)->sin6_addr;
	} else {
		NET_ERR("Invalid IP address family %d: %s", (char *)user_data,
			info->ai_family);
		return;
	}

	TC_PRINT("DNS result for \"%s\": %s address: %s",
		 queried,
		 hr_family,
		 net_addr_ntop(info->ai_family, addr,
			       hr_addr, sizeof(hr_addr)));

	do_ipv4_lookup();
}

static void setup_ipv4(void)
{
	char hr_addr[NET_IPV4_ADDR_LEN];
	struct in_addr addr;

	if (net_addr_pton(AF_INET, CONFIG_NET_APP_MY_IPV4_ADDR, &addr)) {
		NET_ERR("Invalid address: %s", CONFIG_NET_APP_MY_IPV4_ADDR);
		return;
	}

	net_if_ipv4_addr_add(net_if_get_by_index(0),
			     &addr, NET_ADDR_MANUAL, 0);

	TC_PRINT("IPv4 address: %s",
		 net_addr_ntop(AF_INET, &addr, hr_addr, NET_IPV4_ADDR_LEN));

	do_ipv4_lookup();
}

void test_main(void)
{
	ztest_test_suite(dns_tests,
			 ztest_unit_test(setup_ipv4));

	ztest_run_test_suite(dns_tests);
}
