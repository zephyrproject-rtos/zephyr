/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if 1
#define SYS_LOG_DOMAIN "dns-res"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1
#endif

#include <zephyr.h>
#include <linker/sections.h>
#include <errno.h>
#include <stdio.h>

#include <net/net_core.h>
#include <net/net_if.h>
#include <net/net_mgmt.h>
#include <net/dns_resolve.h>

#define STACKSIZE 2000
K_THREAD_STACK_DEFINE(thread_stack, STACKSIZE);
static struct k_thread thread_data;

#define DNS_TIMEOUT 2000 /* ms */

void dns_result_cb(enum dns_resolve_status status,
		   struct dns_addrinfo *info,
		   void *user_data)
{
	char hr_addr[NET_IPV6_ADDR_LEN];
	char *hr_family;
	void *addr;

	switch (status) {
	case DNS_EAI_CANCELED:
		NET_INFO("DNS query was canceled");
		return;
	case DNS_EAI_FAIL:
		NET_INFO("DNS resolve failed");
		return;
	case DNS_EAI_NODATA:
		NET_INFO("Cannot resolve address");
		return;
	case DNS_EAI_ALLDONE:
		NET_INFO("DNS resolving finished");
		return;
	case DNS_EAI_INPROGRESS:
		break;
	default:
		NET_INFO("DNS resolving error (%d)", status);
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
		NET_ERR("Invalid IP address family %d", info->ai_family);
		return;
	}

	NET_INFO("%s address: %s", hr_family,
		 net_addr_ntop(info->ai_family, addr,
			       hr_addr, sizeof(hr_addr)));
}

#if defined(CONFIG_NET_DHCPV4)
static struct net_mgmt_event_callback mgmt4_cb;
static struct k_delayed_work ipv4_timer;

static void do_ipv4_lookup(struct k_work *work)
{
	u16_t dns_id;
	int ret;

	ret = dns_get_addr_info("www.zephyrproject.org",
				DNS_QUERY_TYPE_A,
				&dns_id,
				dns_result_cb,
				NULL,
				DNS_TIMEOUT);
	if (ret < 0) {
		NET_ERR("Cannot resolve IPv4 address (%d)", ret);
		return;
	}

	NET_DBG("DNS id %u", dns_id);
}

static void ipv4_addr_add_handler(struct net_mgmt_event_callback *cb,
				  u32_t mgmt_event,
				  struct net_if *iface)
{
	char hr_addr[NET_IPV4_ADDR_LEN];
	int i;

	if (mgmt_event != NET_EVENT_IPV4_ADDR_ADD) {
		return;
	}

	for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		struct net_if_addr *if_addr = &iface->ipv4.unicast[i];

		if (if_addr->addr_type != NET_ADDR_DHCP || !if_addr->is_used) {
			continue;
		}

		NET_INFO("IPv4 address: %s",
			 net_addr_ntop(AF_INET, &if_addr->address.in_addr,
				       hr_addr, NET_IPV4_ADDR_LEN));
		NET_INFO("Lease time: %u seconds", iface->dhcpv4.lease_time);
		NET_INFO("Subnet: %s",
			 net_addr_ntop(AF_INET, &iface->ipv4.netmask,
				       hr_addr, NET_IPV4_ADDR_LEN));
		NET_INFO("Router: %s",
			 net_addr_ntop(AF_INET, &iface->ipv4.gw,
				       hr_addr, NET_IPV4_ADDR_LEN));
		break;
	}

	/* We cannot run DNS lookup directly from this thread as the
	 * management event thread stack is very small by default.
	 * So run it from work queue instead.
	 */
	k_delayed_work_init(&ipv4_timer, do_ipv4_lookup);
	k_delayed_work_submit(&ipv4_timer, 0);
}

static void setup_dhcpv4(struct net_if *iface)
{
	NET_INFO("Getting IPv4 address via DHCP before issuing DNS query");

	net_mgmt_init_event_callback(&mgmt4_cb, ipv4_addr_add_handler,
				     NET_EVENT_IPV4_ADDR_ADD);
	net_mgmt_add_event_callback(&mgmt4_cb);

	net_dhcpv4_start(iface);
}

#else
#define setup_dhcpv4(...)
#endif /* CONFIG_NET_DHCPV4 */

#if defined(CONFIG_NET_IPV4) && !defined(CONFIG_NET_DHCPV4)

#if !defined(CONFIG_NET_APP_MY_IPV4_ADDR)
#error "You need to define an IPv4 address or enable DHCPv4!"
#endif

static void setup_ipv4(struct net_if *iface)
{
	char hr_addr[NET_IPV4_ADDR_LEN];
	struct in_addr addr;
	u16_t dns_id;
	int ret;

	if (net_addr_pton(AF_INET, CONFIG_NET_APP_MY_IPV4_ADDR, &addr)) {
		NET_ERR("Invalid address: %s", CONFIG_NET_APP_MY_IPV4_ADDR);
		return;
	}

	net_if_ipv4_addr_add(iface, &addr, NET_ADDR_MANUAL, 0);

	NET_INFO("IPv4 address: %s",
		 net_addr_ntop(AF_INET, &addr, hr_addr, NET_IPV4_ADDR_LEN));

	ret = dns_get_addr_info("www.zephyrproject.org",
				DNS_QUERY_TYPE_A,
				&dns_id,
				dns_result_cb,
				NULL,
				DNS_TIMEOUT);
	if (ret < 0) {
		NET_ERR("Cannot resolve IPv4 address (%d)", ret);
		return;
	}

	NET_DBG("DNS id %u", dns_id);
}

#else
#define setup_ipv4(...)
#endif /* CONFIG_NET_IPV4 && !CONFIG_NET_DHCPV4 */

#if defined(CONFIG_NET_IPV6)

#if !defined(CONFIG_NET_APP_MY_IPV6_ADDR)
#error "You need to define an IPv6 address!"
#endif

static struct net_mgmt_event_callback mgmt6_cb;
static struct k_delayed_work ipv6_timer;

static void do_ipv6_lookup(struct k_work *work)
{
	u16_t dns_id;
	int ret;

	ret = dns_get_addr_info("www.zephyrproject.org",
				DNS_QUERY_TYPE_AAAA,
				&dns_id,
				dns_result_cb,
				NULL,
				DNS_TIMEOUT);
	if (ret < 0) {
		NET_ERR("Cannot resolve IPv6 address (%d)", ret);
		return;
	}

	NET_DBG("DNS id %u", dns_id);
}

/* DNS query will most probably fail if we do not have a default
 * router configured because typically the DNS server is outside of local
 * network. So wait for that before continuing.
 */
static void ipv6_router_add_handler(struct net_mgmt_event_callback *cb,
				    u32_t mgmt_event,
				    struct net_if *iface)
{
	if (mgmt_event != NET_EVENT_IPV6_ROUTER_ADD) {
		return;
	}

	/* We cannot run DNS lookup directly from this thread as the
	 * management event thread stack is very small by default.
	 * So run it from work queue instead.
	 */
	k_delayed_work_init(&ipv6_timer, do_ipv6_lookup);
	k_delayed_work_submit(&ipv6_timer, 0);
}

static void setup_ipv6(struct net_if *iface)
{
	char hr_addr[NET_IPV6_ADDR_LEN];
	struct in6_addr addr;

	if (net_addr_pton(AF_INET6, CONFIG_NET_APP_MY_IPV6_ADDR, &addr)) {
		NET_ERR("Invalid address: %s", CONFIG_NET_APP_MY_IPV6_ADDR);
		return;
	}

	net_mgmt_init_event_callback(&mgmt6_cb, ipv6_router_add_handler,
				     NET_EVENT_IPV6_ROUTER_ADD);
	net_mgmt_add_event_callback(&mgmt6_cb);

	net_if_ipv6_addr_add(iface, &addr, NET_ADDR_MANUAL, 0);

	NET_INFO("IPv6 address: %s",
		 net_addr_ntop(AF_INET6, &addr, hr_addr, NET_IPV6_ADDR_LEN));

	NET_INFO("Waiting for IPv6 default router notification "
		 "before issuing DNS query");
}

#else
#define setup_ipv6(...)
#endif /* CONFIG_NET_IPV6 */

static void network_setup(void)
{
	struct net_if *iface = net_if_get_default();

	setup_ipv4(iface);

	setup_dhcpv4(iface);

	setup_ipv6(iface);
}

void main(void)
{
	NET_INFO("Starting DNS resolve sample");

	k_thread_create(&thread_data, thread_stack, STACKSIZE,
			(k_thread_entry_t)network_setup,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, 0);
}
