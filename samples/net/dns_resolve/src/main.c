/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_dns_resolve_client_sample, LOG_LEVEL_DBG);

#include <zephyr.h>
#include <linker/sections.h>
#include <errno.h>
#include <stdio.h>

#include <net/net_core.h>
#include <net/net_if.h>
#include <net/net_mgmt.h>
#include <net/dns_resolve.h>

#if defined(CONFIG_MDNS_RESOLVER)
#if defined(CONFIG_NET_IPV4)
static struct k_delayed_work mdns_ipv4_timer;
static void do_mdns_ipv4_lookup(struct k_work *work);
#endif
#if defined(CONFIG_NET_IPV6)
static struct k_delayed_work mdns_ipv6_timer;
static void do_mdns_ipv6_lookup(struct k_work *work);
#endif
#endif

#define DNS_TIMEOUT (2 * MSEC_PER_SEC)

void dns_result_cb(enum dns_resolve_status status,
		   struct dns_addrinfo *info,
		   void *user_data)
{
	char hr_addr[NET_IPV6_ADDR_LEN];
	char *hr_family;
	void *addr;

	switch (status) {
	case DNS_EAI_CANCELED:
		LOG_INF("DNS query was canceled");
		return;
	case DNS_EAI_FAIL:
		LOG_INF("DNS resolve failed");
		return;
	case DNS_EAI_NODATA:
		LOG_INF("Cannot resolve address");
		return;
	case DNS_EAI_ALLDONE:
		LOG_INF("DNS resolving finished");
		return;
	case DNS_EAI_INPROGRESS:
		break;
	default:
		LOG_INF("DNS resolving error (%d)", status);
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
		LOG_ERR("Invalid IP address family %d", info->ai_family);
		return;
	}

	LOG_INF("%s %s address: %s", user_data ? (char *)user_data : "<null>",
		hr_family,
		log_strdup(net_addr_ntop(info->ai_family, addr,
					 hr_addr, sizeof(hr_addr))));
}

void mdns_result_cb(enum dns_resolve_status status,
		    struct dns_addrinfo *info,
		    void *user_data)
{
	char hr_addr[NET_IPV6_ADDR_LEN];
	char *hr_family;
	void *addr;

	switch (status) {
	case DNS_EAI_CANCELED:
		LOG_INF("mDNS query was canceled");
		return;
	case DNS_EAI_FAIL:
		LOG_INF("mDNS resolve failed");
		return;
	case DNS_EAI_NODATA:
		LOG_INF("Cannot resolve address using mDNS");
		return;
	case DNS_EAI_ALLDONE:
		LOG_INF("mDNS resolving finished");
		return;
	case DNS_EAI_INPROGRESS:
		break;
	default:
		LOG_INF("mDNS resolving error (%d)", status);
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
		LOG_ERR("Invalid IP address family %d", info->ai_family);
		return;
	}

	LOG_INF("%s %s address: %s", user_data ? (char *)user_data : "<null>",
		hr_family,
		log_strdup(net_addr_ntop(info->ai_family, addr,
					 hr_addr, sizeof(hr_addr))));
}

#if defined(CONFIG_NET_DHCPV4)
static struct net_mgmt_event_callback mgmt4_cb;
static struct k_delayed_work ipv4_timer;

static void do_ipv4_lookup(struct k_work *work)
{
	static const char *query = "www.zephyrproject.org";
	static uint16_t dns_id;
	int ret;

	ret = dns_get_addr_info(query,
				DNS_QUERY_TYPE_A,
				&dns_id,
				dns_result_cb,
				(void *)query,
				DNS_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("Cannot resolve IPv4 address (%d)", ret);
		return;
	}

	LOG_DBG("DNS id %u", dns_id);
}

static void ipv4_addr_add_handler(struct net_mgmt_event_callback *cb,
				  uint32_t mgmt_event,
				  struct net_if *iface)
{
	char hr_addr[NET_IPV4_ADDR_LEN];
	int i;

	if (mgmt_event != NET_EVENT_IPV4_ADDR_ADD) {
		return;
	}

	for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		struct net_if_addr *if_addr =
			&iface->config.ip.ipv4->unicast[i];

		if (if_addr->addr_type != NET_ADDR_DHCP || !if_addr->is_used) {
			continue;
		}

		LOG_INF("IPv4 address: %s",
			log_strdup(net_addr_ntop(AF_INET,
						 &if_addr->address.in_addr,
						 hr_addr, NET_IPV4_ADDR_LEN)));
		LOG_INF("Lease time: %u seconds",
			 iface->config.dhcpv4.lease_time);
		LOG_INF("Subnet: %s",
			log_strdup(net_addr_ntop(AF_INET,
					       &iface->config.ip.ipv4->netmask,
					       hr_addr, NET_IPV4_ADDR_LEN)));
		LOG_INF("Router: %s",
			log_strdup(net_addr_ntop(AF_INET,
					       &iface->config.ip.ipv4->gw,
					       hr_addr, NET_IPV4_ADDR_LEN)));
		break;
	}

	/* We cannot run DNS lookup directly from this thread as the
	 * management event thread stack is very small by default.
	 * So run it from work queue instead.
	 */
	k_delayed_work_init(&ipv4_timer, do_ipv4_lookup);
	k_delayed_work_submit(&ipv4_timer, K_NO_WAIT);

#if defined(CONFIG_MDNS_RESOLVER)
	k_delayed_work_init(&mdns_ipv4_timer, do_mdns_ipv4_lookup);
	k_delayed_work_submit(&mdns_ipv4_timer, K_NO_WAIT);
#endif
}

static void setup_dhcpv4(struct net_if *iface)
{
	LOG_INF("Getting IPv4 address via DHCP before issuing DNS query");

	net_mgmt_init_event_callback(&mgmt4_cb, ipv4_addr_add_handler,
				     NET_EVENT_IPV4_ADDR_ADD);
	net_mgmt_add_event_callback(&mgmt4_cb);

	net_dhcpv4_start(iface);
}

#else
#define setup_dhcpv4(...)
#endif /* CONFIG_NET_DHCPV4 */

#if defined(CONFIG_NET_IPV4) || defined(CONFIG_NET_DHCPV4)
#if defined(CONFIG_MDNS_RESOLVER)
static void do_mdns_ipv4_lookup(struct k_work *work)
{
	static const char *query = "zephyr.local";
	int ret;

	LOG_DBG("Doing mDNS IPv4 query");

	ret = dns_get_addr_info(query,
				DNS_QUERY_TYPE_A,
				NULL,
				mdns_result_cb,
				(void *)query,
				DNS_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("Cannot resolve mDNS IPv4 address (%d)", ret);
		return;
	}

	LOG_DBG("mDNS v4 query sent");
}
#endif
#endif

#if defined(CONFIG_NET_IPV4) && !defined(CONFIG_NET_DHCPV4)

#if !defined(CONFIG_NET_CONFIG_MY_IPV4_ADDR)
#error "You need to define an IPv4 address or enable DHCPv4!"
#endif

static void do_ipv4_lookup(void)
{
	static const char *query = "www.zephyrproject.org";
	static uint16_t dns_id;
	int ret;

	ret = dns_get_addr_info(query,
				DNS_QUERY_TYPE_A,
				&dns_id,
				dns_result_cb,
				(void *)query,
				DNS_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("Cannot resolve IPv4 address (%d)", ret);
		return;
	}

	LOG_DBG("DNS id %u", dns_id);
}

static void setup_ipv4(struct net_if *iface)
{
	ARG_UNUSED(iface);

	do_ipv4_lookup();

#if defined(CONFIG_MDNS_RESOLVER) && defined(CONFIG_NET_IPV4)
	k_delayed_work_init(&mdns_ipv4_timer, do_mdns_ipv4_lookup);
	k_delayed_work_submit(&mdns_ipv4_timer, K_NO_WAIT);
#endif
}

#else
#define setup_ipv4(...)
#endif /* CONFIG_NET_IPV4 && !CONFIG_NET_DHCPV4 */

#if defined(CONFIG_NET_IPV6)

#if !defined(CONFIG_NET_CONFIG_MY_IPV6_ADDR)
#error "You need to define an IPv6 address!"
#endif

static void do_ipv6_lookup(void)
{
	static const char *query = "www.zephyrproject.org";
	static uint16_t dns_id;
	int ret;

	ret = dns_get_addr_info(query,
				DNS_QUERY_TYPE_AAAA,
				&dns_id,
				dns_result_cb,
				(void *)query,
				DNS_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("Cannot resolve IPv6 address (%d)", ret);
		return;
	}

	LOG_DBG("DNS id %u", dns_id);
}

static void setup_ipv6(struct net_if *iface)
{
	ARG_UNUSED(iface);

	do_ipv6_lookup();

#if defined(CONFIG_MDNS_RESOLVER) && defined(CONFIG_NET_IPV6)
	k_delayed_work_init(&mdns_ipv6_timer, do_mdns_ipv6_lookup);
	k_delayed_work_submit(&mdns_ipv6_timer, K_NO_WAIT);
#endif
}

#if defined(CONFIG_MDNS_RESOLVER)
static void do_mdns_ipv6_lookup(struct k_work *work)
{
	static const char *query = "zephyr.local";
	int ret;

	LOG_DBG("Doing mDNS IPv6 query");

	ret = dns_get_addr_info(query,
				DNS_QUERY_TYPE_AAAA,
				NULL,
				mdns_result_cb,
				(void *)query,
				DNS_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("Cannot resolve mDNS IPv6 address (%d)", ret);
		return;
	}

	LOG_DBG("mDNS v6 query sent");
}
#endif

#else
#define setup_ipv6(...)
#endif /* CONFIG_NET_IPV6 */

void main(void)
{
	struct net_if *iface = net_if_get_default();

	LOG_INF("Starting DNS resolve sample");

	setup_ipv4(iface);

	setup_dhcpv4(iface);

	setup_ipv6(iface);
}
