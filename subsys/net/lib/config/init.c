/* init.c */

/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_config, CONFIG_NET_CONFIG_LOG_LEVEL);

#include <zephyr.h>
#include <init.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include <logging/log_backend.h>
#include <net/net_core.h>
#include <net/net_ip.h>
#include <net/net_if.h>
#include <net/dhcpv4.h>
#include <net/net_mgmt.h>
#include <net/dns_resolve.h>

#include <net/net_config.h>

#include "ieee802154_settings.h"
#include "bt_settings.h"

extern const struct log_backend *log_backend_net_get(void);
extern int net_init_clock_via_sntp(void);

static K_SEM_DEFINE(waiter, 0, 1);
static struct k_sem counter;

#if defined(CONFIG_NET_DHCPV4) && defined(CONFIG_NET_NATIVE_IPV4)
static struct net_mgmt_event_callback mgmt4_cb;

static void ipv4_addr_add_handler(struct net_mgmt_event_callback *cb,
				  u32_t mgmt_event,
				  struct net_if *iface)
{
#if CONFIG_NET_CONFIG_LOG_LEVEL >= LOG_LEVEL_INF
	char hr_addr[NET_IPV4_ADDR_LEN];
#endif
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

#if CONFIG_NET_CONFIG_LOG_LEVEL >= LOG_LEVEL_INF
		NET_INFO("IPv4 address: %s",
			 log_strdup(net_addr_ntop(AF_INET,
						  &if_addr->address.in_addr,
						  hr_addr, sizeof(hr_addr))));
		NET_INFO("Lease time: %u seconds",
			 iface->config.dhcpv4.lease_time);
		NET_INFO("Subnet: %s",
			 log_strdup(net_addr_ntop(AF_INET,
				       &iface->config.ip.ipv4->netmask,
				       hr_addr, sizeof(hr_addr))));
		NET_INFO("Router: %s",
			 log_strdup(net_addr_ntop(AF_INET,
						  &iface->config.ip.ipv4->gw,
						  hr_addr, sizeof(hr_addr))));
#endif
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

#if defined(CONFIG_NET_NATIVE_IPV4) && !defined(CONFIG_NET_DHCPV4) && \
	!defined(CONFIG_NET_CONFIG_MY_IPV4_ADDR)
#error "You need to define an IPv4 address or enable DHCPv4!"
#endif

#if defined(CONFIG_NET_NATIVE_IPV4) && defined(CONFIG_NET_CONFIG_MY_IPV4_ADDR)

static void setup_ipv4(struct net_if *iface)
{
#if CONFIG_NET_CONFIG_LOG_LEVEL >= LOG_LEVEL_INF
	char hr_addr[NET_IPV4_ADDR_LEN];
#endif
	struct in_addr addr;

	if (sizeof(CONFIG_NET_CONFIG_MY_IPV4_ADDR) == 1) {
		/* Empty address, skip setting ANY address in this case */
		return;
	}

	if (net_addr_pton(AF_INET, CONFIG_NET_CONFIG_MY_IPV4_ADDR, &addr)) {
		NET_ERR("Invalid address: %s", CONFIG_NET_CONFIG_MY_IPV4_ADDR);
		return;
	}

#if defined(CONFIG_NET_DHCPV4)
	/* In case DHCP is enabled, make the static address tentative,
	 * to allow DHCP address to override it. This covers a usecase
	 * of "there should be a static IP address for DHCP-less setups",
	 * but DHCP should override it (to use it, NET_IF_MAX_IPV4_ADDR
	 * should be set to 1). There is another usecase: "there should
	 * always be static IP address, and optionally, DHCP address".
	 * For that to work, NET_IF_MAX_IPV4_ADDR should be 2 (or more).
	 * (In this case, an app will need to bind to the needed addr
	 * explicitly.)
	 */
	net_if_ipv4_addr_add(iface, &addr, NET_ADDR_OVERRIDABLE, 0);
#else
	net_if_ipv4_addr_add(iface, &addr, NET_ADDR_MANUAL, 0);
#endif

#if CONFIG_NET_CONFIG_LOG_LEVEL >= LOG_LEVEL_INF
	NET_INFO("IPv4 address: %s",
		 log_strdup(net_addr_ntop(AF_INET, &addr, hr_addr,
					  sizeof(hr_addr))));
#endif

	if (sizeof(CONFIG_NET_CONFIG_MY_IPV4_NETMASK) > 1) {
		/* If not empty */
		if (net_addr_pton(AF_INET, CONFIG_NET_CONFIG_MY_IPV4_NETMASK,
				  &addr)) {
			NET_ERR("Invalid netmask: %s",
				CONFIG_NET_CONFIG_MY_IPV4_NETMASK);
		} else {
			net_if_ipv4_set_netmask(iface, &addr);
		}
	}

	if (sizeof(CONFIG_NET_CONFIG_MY_IPV4_GW) > 1) {
		/* If not empty */
		if (net_addr_pton(AF_INET, CONFIG_NET_CONFIG_MY_IPV4_GW,
				  &addr)) {
			NET_ERR("Invalid gateway: %s",
				CONFIG_NET_CONFIG_MY_IPV4_GW);
		} else {
			net_if_ipv4_set_gw(iface, &addr);
		}
	}

	k_sem_take(&counter, K_NO_WAIT);
	k_sem_give(&waiter);
}

#else
#define setup_ipv4(...)
#endif /* CONFIG_NET_IPV4 && !CONFIG_NET_DHCPV4 */

#if defined(CONFIG_NET_NATIVE_IPV6)
#if !defined(CONFIG_NET_CONFIG_MY_IPV6_ADDR)
#error "You need to define an IPv6 address!"
#endif

static struct net_mgmt_event_callback mgmt6_cb;
static struct in6_addr laddr;

static void ipv6_event_handler(struct net_mgmt_event_callback *cb,
			       u32_t mgmt_event, struct net_if *iface)
{
	struct net_if_ipv6 *ipv6 = iface->config.ip.ipv6;
	int i;

	if (!ipv6) {
		return;
	}

	if (mgmt_event == NET_EVENT_IPV6_ADDR_ADD) {
		/* save the last added IP address for this interface */
		for (i = NET_IF_MAX_IPV6_ADDR - 1; i >= 0; i--) {
			if (ipv6->unicast[i].is_used) {
				memcpy(&laddr,
				       &ipv6->unicast[i].address.in6_addr,
				       sizeof(laddr));
			}
		}
	}

	if (mgmt_event == NET_EVENT_IPV6_DAD_SUCCEED) {
#if CONFIG_NET_CONFIG_LOG_LEVEL >= LOG_LEVEL_INF
		char hr_addr[NET_IPV6_ADDR_LEN];
#endif
		struct net_if_addr *ifaddr;

		ifaddr = net_if_ipv6_addr_lookup(&laddr, &iface);
		if (!ifaddr ||
		    !(net_ipv6_addr_cmp(&ifaddr->address.in6_addr, &laddr) &&
		      ifaddr->addr_state == NET_ADDR_PREFERRED)) {
			/* Address is not yet properly setup */
			return;
		}

#if CONFIG_NET_CONFIG_LOG_LEVEL >= LOG_LEVEL_INF
		NET_INFO("IPv6 address: %s",
			 log_strdup(net_addr_ntop(AF_INET6, &laddr, hr_addr,
						  NET_IPV6_ADDR_LEN)));
#endif

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

	if (sizeof(CONFIG_NET_CONFIG_MY_IPV6_ADDR) == 1) {
		/* Empty address, skip setting ANY address in this case */
		return;
	}

	if (net_addr_pton(AF_INET6, CONFIG_NET_CONFIG_MY_IPV6_ADDR, &laddr)) {
		NET_ERR("Invalid address: %s", CONFIG_NET_CONFIG_MY_IPV6_ADDR);
		/* some interfaces may add IP address later */
		mask |= NET_EVENT_IPV6_ADDR_ADD;
	}

	if (flags & NET_CONFIG_NEED_ROUTER) {
		mask |= NET_EVENT_IPV6_ROUTER_ADD;
	}

	net_mgmt_init_event_callback(&mgmt6_cb, ipv6_event_handler, mask);
	net_mgmt_add_event_callback(&mgmt6_cb);

	/*
	 * check for CMD_ADDR_ADD bit here, NET_EVENT_IPV6_ADDR_ADD is
	 * a combination of _NET_EVENT_IPV6_BASE | NET_EVENT_IPV6_CMD_ADDR_ADD
	 * so it will always return != NET_EVENT_IPV6_CMD_ADDR_ADD if any other
	 * event is set (for instance NET_EVENT_IPV6_ROUTER_ADD)
	 */
	if ((mask & NET_EVENT_IPV6_CMD_ADDR_ADD) ==
	    NET_EVENT_IPV6_CMD_ADDR_ADD) {
		ifaddr = net_if_ipv6_addr_add(iface, &laddr,
					      NET_ADDR_MANUAL, 0);
		if (!ifaddr) {
			NET_ERR("Cannot add %s to interface",
				CONFIG_NET_CONFIG_MY_IPV6_ADDR);
		}
	}

#if !defined(CONFIG_NET_IPV6_DAD)
	k_sem_take(&counter, K_NO_WAIT);
	k_sem_give(&waiter);
#endif

	return;
}

#else
#define setup_ipv6(...)
#endif /* CONFIG_NET_IPV6 */

int net_config_init(const char *app_info, u32_t flags, s32_t timeout)
{
#define LOOP_DIVIDER 10
	struct net_if *iface = net_if_get_default();
	int loop = timeout / LOOP_DIVIDER;
	int count = 0;

	if (app_info) {
		NET_INFO("%s", log_strdup(app_info));
	}

	if (!iface) {
		NET_ERR("No network interfaces");
		return -ENODEV;
	}

	if (flags & NET_CONFIG_NEED_IPV6) {
		count++;
	}

	if (flags & NET_CONFIG_NEED_IPV4) {
		count++;
	}

	k_sem_init(&counter, count, UINT_MAX);

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

	/* Loop here until we are ready to continue. As we might need
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

#if defined(CONFIG_NET_CONFIG_AUTO_INIT)
static int init_app(struct device *device)
{
	u32_t flags = 0U;
	int ret;

	ARG_UNUSED(device);

#if defined(CONFIG_NET_IPV6)
	/* IEEE 802.15.4 is only usable if IPv6 is enabled */
	ret = z_net_config_ieee802154_setup();
	if (ret < 0) {
		NET_ERR("Cannot setup IEEE 802.15.4 interface (%d)", ret);
	}

	ret = z_net_config_bt_setup();
	if (ret < 0) {
		NET_ERR("Cannot setup Bluetooth interface (%d)", ret);
	}
#endif

	if (IS_ENABLED(CONFIG_NET_CONFIG_NEED_IPV6)) {
		flags |= NET_CONFIG_NEED_IPV6;
	}

	if (IS_ENABLED(CONFIG_NET_CONFIG_NEED_IPV6_ROUTER)) {
		flags |= NET_CONFIG_NEED_ROUTER;
	}

	if (IS_ENABLED(CONFIG_NET_CONFIG_NEED_IPV4)) {
		flags |= NET_CONFIG_NEED_IPV4;
	}

	/* Initialize the application automatically if needed */
	ret = net_config_init("Initializing network", flags,
			      K_SECONDS(CONFIG_NET_CONFIG_INIT_TIMEOUT));
	if (ret < 0) {
		NET_ERR("Network initialization failed (%d)", ret);
	}

	if (IS_ENABLED(CONFIG_NET_CONFIG_CLOCK_SNTP_INIT)) {
		net_init_clock_via_sntp();
	}

	/* This is activated late as it requires the network stack to be up
	 * and running before syslog messages can be sent to network.
	 */
	if (IS_ENABLED(CONFIG_LOG_BACKEND_NET)) {
		const struct log_backend *backend = log_backend_net_get();

		if (!log_backend_is_active(backend)) {
			log_backend_activate(backend, NULL);
		}
	}

	return ret;
}

SYS_INIT(init_app, APPLICATION, CONFIG_NET_CONFIG_INIT_PRIO);
#endif /* CONFIG_NET_CONFIG_AUTO_INIT */
