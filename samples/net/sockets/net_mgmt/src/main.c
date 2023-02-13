/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_mgmt_sock_sample, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>
#include <errno.h>
#include <stdio.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/socket_net_mgmt.h>
#include <zephyr/net/net_if.h>

#define MAX_BUF_LEN 64
#define STACK_SIZE 1024
#if defined(CONFIG_NET_TC_THREAD_COOPERATIVE)
#define THREAD_PRIORITY K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1)
#else
#define THREAD_PRIORITY K_PRIO_PREEMPT(8)
#endif

/* A test thread that spits out events that we can catch and show to user */
static void trigger_events(void)
{
	int operation = 0;
	struct net_if_addr *ifaddr_v6;
	struct net_if *iface;
	struct in6_addr addr_v6;
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
			ret = net_if_ipv6_addr_rm(iface, &addr_v6);
			if (!ret) {
				LOG_ERR("Cannot del IPv%c address", '6');
				break;
			}

			break;
		default:
			operation = -1;
			break;
		}

		operation++;

		k_sleep(K_SECONDS(1));
	}
}

K_THREAD_DEFINE(trigger_events_thread_id, STACK_SIZE,
		trigger_events, NULL, NULL, NULL,
		THREAD_PRIORITY, 0, -1);

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

static void listener(void)
{
	struct sockaddr_nm sockaddr;
	struct sockaddr_nm event_addr;
	socklen_t event_addr_len;
	char ipaddr[INET6_ADDRSTRLEN];
	uint8_t buf[MAX_BUF_LEN];
	int fd, ret;

	fd = socket(AF_NET_MGMT, SOCK_DGRAM, NET_MGMT_EVENT_PROTO);
	if (fd < 0) {
		printk("Cannot create net_mgmt socket (%d)\n", errno);
		exit(1);
	}

	memset(&sockaddr, 0, sizeof(sockaddr));

	sockaddr.nm_family = AF_NET_MGMT;
	sockaddr.nm_ifindex = 0; /* Any network interface */
	sockaddr.nm_pid = (uintptr_t)k_current_get();
	sockaddr.nm_mask = NET_EVENT_IPV6_DAD_SUCCEED |
			    NET_EVENT_IPV6_ADDR_ADD |
			    NET_EVENT_IPV6_ADDR_DEL;

	ret = bind(fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
	if (ret < 0) {
		printk("Cannot bind net_mgmt socket (%d)\n", errno);
		exit(1);
	}

	while (1) {
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
		case NET_EVENT_IPV6_DAD_SUCCEED:
			printk("DAD succeed for interface %d (%s)\n",
			       event_addr.nm_ifindex,
			       get_ip_addr(ipaddr, sizeof(ipaddr),
					   AF_INET6, hdr));
			break;
		case NET_EVENT_IPV6_ADDR_ADD:
			printk("IPv6 address added to interface %d (%s)\n",
			       event_addr.nm_ifindex,
			       get_ip_addr(ipaddr, sizeof(ipaddr),
					   AF_INET6, hdr));
			break;
		case NET_EVENT_IPV6_ADDR_DEL:
			printk("IPv6 address removed from interface %d (%s)\n",
			       event_addr.nm_ifindex,
			       get_ip_addr(ipaddr, sizeof(ipaddr),
					   AF_INET6, hdr));
			break;
		}
	}
}

void main(void)
{
	/* The thread start to trigger network management events that
	 * we then can catch.
	 */
	k_thread_start(trigger_events_thread_id);

	if (IS_ENABLED(CONFIG_USERSPACE)) {
		k_thread_user_mode_enter((k_thread_entry_t)listener,
					 NULL, NULL, NULL);
	} else {
		listener();
	}
}
