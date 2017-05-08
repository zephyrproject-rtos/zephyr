/* sample_app_setup.c */

/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if 1
#define SYS_LOG_DOMAIN "sample/net"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1
#endif

#include <zephyr.h>

#include <net/net_core.h>
#include <net/net_ip.h>
#include <net/net_if.h>
#include <net/dhcpv4.h>
#include <net/net_mgmt.h>

#include "net_sample_app.h"

#if defined(CONFIG_NET_L2_BLUETOOTH)
#include <bluetooth/bluetooth.h>
#include <gatt/ipss.h>
#endif

#if defined(CONFIG_NET_L2_IEEE802154)
#include <ieee802154_settings.h>
#endif

static struct k_sem waiter = K_SEM_INITIALIZER(waiter, 0, 1);
static struct k_sem counter;

#if defined(CONFIG_NET_DHCPV4)
static struct net_mgmt_event_callback mgmt4_cb;

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

	k_sem_take(&counter, K_NO_WAIT);
	k_sem_give(&waiter);
}

static void setup_dhcpv4(struct net_if *iface)
{
	NET_INFO("Running dhcpv4 client...");

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

	if (net_addr_pton(AF_INET, CONFIG_NET_APP_MY_IPV4_ADDR, &addr)) {
		NET_ERR("Invalid address: %s", CONFIG_NET_APP_MY_IPV4_ADDR);
		return;
	}

	net_if_ipv4_addr_add(iface, &addr, NET_ADDR_MANUAL, 0);

	NET_INFO("IPv4 address: %s",
		 net_addr_ntop(AF_INET, &addr, hr_addr, NET_IPV4_ADDR_LEN));

	k_sem_take(&counter, K_NO_WAIT);
	k_sem_give(&waiter);
}

#else
#define setup_ipv4(...)
#endif /* CONFIG_NET_IPV4 && !CONFIG_NET_DHCPV4 */

#if defined(CONFIG_NET_IPV6)
#if !defined(CONFIG_NET_APP_MY_IPV6_ADDR)
#error "You need to define an IPv6 address!"
#endif

static struct net_mgmt_event_callback mgmt6_cb;
static struct in6_addr laddr;

static void ipv6_event_handler(struct net_mgmt_event_callback *cb,
			       u32_t mgmt_event, struct net_if *iface)
{
	if (mgmt_event == NET_EVENT_IPV6_DAD_SUCCEED) {
		char hr_addr[NET_IPV6_ADDR_LEN];
		struct net_if_addr *ifaddr;

		ifaddr = net_if_ipv6_addr_lookup(&laddr, &iface);
		if (!ifaddr ||
		    !(net_ipv6_addr_cmp(&ifaddr->address.in6_addr, &laddr) &&
		      ifaddr->addr_state == NET_ADDR_PREFERRED)) {
			/* Address is not yet properly setup */
			return;
		}

		NET_INFO("IPv6 address: %s",
			 net_addr_ntop(AF_INET6, &laddr, hr_addr,
				       NET_IPV6_ADDR_LEN));

		k_sem_take(&counter, K_NO_WAIT);
		k_sem_give(&waiter);
	}

	if (mgmt_event == NET_EVENT_IPV6_ROUTER_ADD) {
		k_sem_take(&counter, K_NO_WAIT);
		k_sem_give(&waiter);
	}
}

static void setup_ipv6(struct net_if *iface, u32_t flags)
{
	struct net_if_addr *ifaddr;
	u32_t mask = NET_EVENT_IPV6_DAD_SUCCEED;

	if (net_addr_pton(AF_INET6, CONFIG_NET_APP_MY_IPV6_ADDR, &laddr)) {
		NET_ERR("Invalid address: %s", CONFIG_NET_APP_MY_IPV6_ADDR);
		return;
	}

	if (flags & NET_SAMPLE_NEED_ROUTER) {
		mask |= NET_EVENT_IPV6_ROUTER_ADD;
	}

	net_mgmt_init_event_callback(&mgmt6_cb, ipv6_event_handler, mask);
	net_mgmt_add_event_callback(&mgmt6_cb);

	ifaddr = net_if_ipv6_addr_add(iface, &laddr, NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		NET_ERR("Cannot add %s to interface",
			CONFIG_NET_APP_MY_IPV6_ADDR);
		return;
	}
}

#else
#define setup_ipv6(...)
#endif /* CONFIG_NET_IPV6 */

int net_sample_app_init(const char *app_info, u32_t flags, s32_t timeout)
{
#define LOOP_DEVIDER 10
	struct net_if *iface = net_if_get_default();
	int loop = timeout / LOOP_DEVIDER;
	int count = 0;

	if (app_info) {
		NET_INFO("%s", app_info);
	}

#if defined(CONFIG_NET_L2_BLUETOOTH)
	if (bt_enable(NULL)) {
		NET_ERR("Bluetooth init failed");
		return -EFAULT;
	} else {
		ipss_init();
		ipss_advertise();
	}
#endif

#if defined(CONFIG_NET_L2_IEEE802154)
	if (ieee802154_sample_setup()) {
		NET_ERR("IEEE 802.15.4 setup failed");
		return -EFAULT;
	}
#endif

	if (flags & NET_SAMPLE_NEED_IPV6) {
		count++;
	}

	if (flags & NET_SAMPLE_NEED_IPV4) {
		count++;
	}

	k_sem_init(&counter, count, count);

	setup_ipv4(iface);

	setup_dhcpv4(iface);

	setup_ipv6(iface, flags);

	if (timeout < 0) {
		count = -1;
	} else if (timeout == 0) {
		count = 0;
	} else {
		count = timeout / 1000 + 1;
	}

	/* Loop here until until we are ready to continue. As we might need
	 * to wait multiple events, sleep smaller amounts of data.
	 */
	while (count--) {
		if (k_sem_take(&waiter, loop)) {
			if (!k_sem_count_get(&counter)) {
				break;
			}
		}
	}

	if (!count && timeout) {
		NET_ERR("Timeout while waiting setup");
		return -ETIMEDOUT;
	}

	return 0;
}
