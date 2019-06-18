/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <stdio.h>
#include <ztest_assert.h>
#include <sys_clock.h>
#include <net/net_ip.h>
#include <net/socket.h>
#include <net/socket_net_mgmt.h>
#include <net/net_event.h>
#include <net/ethernet_mgmt.h>

#define MAX_BUF_LEN 64
#define STACK_SIZE 1024
#define THREAD_PRIORITY K_PRIO_COOP(8)

static ZTEST_BMEM int fd;
static ZTEST_DMEM struct in_addr addr_v4 = { { { 192, 0, 2, 3 } } };
static ZTEST_BMEM struct in6_addr addr_v6;

#if IS_ENABLED(CONFIG_NET_SOCKETS_LOG_LEVEL_DBG)
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

/* A test thread that spits out events that we can catch and show to user */
static void trigger_events(void)
{
	int operation = 0;
	struct net_if_addr *ifaddr_v6, *ifaddr_v4;
	struct net_if *iface;
	int ret;

	iface = net_if_get_default();

	net_ipv6_addr_create(&addr_v6, 0x2001, 0x0db8, 0, 0, 0, 0, 0, 0x0003);

	while (1) {
		switch (operation) {
		case 0:
			ifaddr_v6 = net_if_ipv6_addr_add(iface, &addr_v6,
							 NET_ADDR_MANUAL, 0);
			if (!ifaddr_v6) {
				LOG_ERR("Cannot add IPv%c address", '6');
				break;
			}

			break;
		case 1:
			ifaddr_v4 = net_if_ipv4_addr_add(iface, &addr_v4,
							 NET_ADDR_MANUAL, 0);
			if (!ifaddr_v4) {
				LOG_ERR("Cannot add IPv%c address", '4');
				break;
			}

			break;
		case 2:
			ret = net_if_ipv6_addr_rm(iface, &addr_v6);
			if (!ret) {
				LOG_ERR("Cannot del IPv%c address", '6');
				break;
			}

			break;
		case 3:
			ret = net_if_ipv4_addr_rm(iface, &addr_v4);
			if (!ret) {
				LOG_ERR("Cannot del IPv%c address", '4');
				break;
			}

			break;
		default:
			operation = -1;
			break;
		}

		operation++;

		k_sleep(K_MSEC(100));
	}
}

K_THREAD_DEFINE(trigger_events_thread_id, STACK_SIZE,
		trigger_events, NULL, NULL, NULL,
		THREAD_PRIORITY, 0, K_FOREVER);

static char *get_ip_addr(char *ipaddr, size_t len, sa_family_t family,
			 struct net_mgmt_msghdr *hdr)
{
	char *buf;

	buf = net_addr_ntop(family, hdr->nm_msg, ipaddr, len);
	if (!buf) {
		return "?";
	}

	return buf;
}

static void test_net_mgmt_setup(void)
{
	struct sockaddr_nm sockaddr;
	int ret;

	fd = socket(AF_NET_MGMT, SOCK_DGRAM, NET_MGMT_EVENT_PROTO);
	zassert_false(fd < 0, "Cannot create net_mgmt socket (%d)", errno);

	memset(&sockaddr, 0, sizeof(sockaddr));

	sockaddr.nm_family = AF_NET_MGMT;
	sockaddr.nm_ifindex = 0; /* Any network interface */
	sockaddr.nm_pid = (int)k_current_get();
	sockaddr.nm_mask = NET_EVENT_IPV6_DAD_SUCCEED |
			   NET_EVENT_IPV6_ADDR_ADD |
			   NET_EVENT_IPV6_ADDR_DEL;

	ret = bind(fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
	zassert_false(ret < 0, "Cannot bind net_mgmt socket (%d)", errno);

	k_thread_start(trigger_events_thread_id);
}

static void test_net_mgmt_catch_events(void)
{
	struct sockaddr_nm event_addr;
	socklen_t event_addr_len;
	char ipaddr[INET6_ADDRSTRLEN];
	u8_t buf[MAX_BUF_LEN];
	int event_count = 2;
	int ret;

	while (event_count > 0) {
		struct net_mgmt_msghdr *hdr;

		memset(buf, 0, sizeof(buf));
		event_addr_len = sizeof(event_addr);

		ret = recvfrom(fd, buf, sizeof(buf), 0,
			       (struct sockaddr *)&event_addr,
			       &event_addr_len);
		if (ret < 0) {
			continue;
		}

		hdr = (struct net_mgmt_msghdr *)buf;

		if (hdr->nm_msg_version != NET_MGMT_SOCKET_VERSION_1) {
			/* Do not know how to parse the message */
			continue;
		}

		switch (event_addr.nm_mask) {
		case NET_EVENT_IPV6_ADDR_ADD:
			DBG("IPv6 address added to interface %d (%s)\n",
			    event_addr.nm_ifindex,
			    get_ip_addr(ipaddr, sizeof(ipaddr),
					AF_INET6, hdr));
			zassert_equal(strncmp(ipaddr, "2001:db8::3",
					      sizeof(ipaddr) - 1), 0,
				      "Invalid IPv6 address %s added",
				      ipaddr);
			event_count--;
			break;
		case NET_EVENT_IPV6_ADDR_DEL:
			DBG("IPv6 address removed from interface %d (%s)\n",
			    event_addr.nm_ifindex,
			    get_ip_addr(ipaddr, sizeof(ipaddr),
					AF_INET6, hdr));
			zassert_equal(strncmp(ipaddr, "2001:db8::3",
					      sizeof(ipaddr) - 1), 0,
				      "Invalid IPv6 address %s removed",
				      ipaddr);
			event_count--;
			break;
		}
	}
}

static void test_net_mgmt_catch_kernel(void)
{
	test_net_mgmt_catch_events();
}

static void test_net_mgmt_catch_user(void)
{
	test_net_mgmt_catch_events();
}

void test_main(void)
{
	k_thread_system_pool_assign(k_current_get());

	ztest_test_suite(socket_net_mgmt,
			 ztest_unit_test(test_net_mgmt_setup),
			 ztest_unit_test(test_net_mgmt_catch_kernel),
			 ztest_user_unit_test(test_net_mgmt_catch_user));

	ztest_run_test_suite(socket_net_mgmt);
}
