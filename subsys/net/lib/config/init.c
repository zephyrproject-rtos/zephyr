/* init.c */

/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_config, CONFIG_NET_CONFIG_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include <zephyr/logging/log_backend.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/dhcpv4.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/dns_resolve.h>

#include <zephyr/net/net_config.h>

#include "ieee802154_settings.h"
#include "bt_settings.h"

extern const struct log_backend *log_backend_net_get(void);
extern int net_init_clock_via_sntp(void);

static K_SEM_DEFINE(waiter, 0, 1);
static K_SEM_DEFINE(counter, 0, UINT_MAX);
static atomic_t services_flags;

#if defined(CONFIG_NET_NATIVE)
static struct net_mgmt_event_callback mgmt_iface_cb;
#endif

static inline void services_notify_ready(int flags)
{
	atomic_or(&services_flags, flags);
	k_sem_give(&waiter);
}

static inline bool services_are_ready(int flags)
{
	return (atomic_get(&services_flags) & flags) == flags;
}

#if defined(CONFIG_NET_DHCPV4) && defined(CONFIG_NET_NATIVE_IPV4)
static struct net_mgmt_event_callback mgmt4_cb;

static void ipv4_addr_add_handler(struct net_mgmt_event_callback *cb,
				  uint32_t mgmt_event,
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
			 net_addr_ntop(AF_INET,
					&if_addr->address.in_addr,
					hr_addr, sizeof(hr_addr)));
		NET_INFO("Lease time: %u seconds",
			 iface->config.dhcpv4.lease_time);
		NET_INFO("Subnet: %s",
			 net_addr_ntop(AF_INET,
				       &iface->config.ip.ipv4->netmask,
				       hr_addr, sizeof(hr_addr)));
		NET_INFO("Router: %s",
			 net_addr_ntop(AF_INET,
					&iface->config.ip.ipv4->gw,
					hr_addr, sizeof(hr_addr)));
#endif
		break;
	}

	services_notify_ready(NET_CONFIG_NEED_IPV4);
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
		 net_addr_ntop(AF_INET, &addr, hr_addr, sizeof(hr_addr)));
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

	services_notify_ready(NET_CONFIG_NEED_IPV4);
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
			       uint32_t mgmt_event, struct net_if *iface)
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
				break;
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
			 net_addr_ntop(AF_INET6, &laddr, hr_addr, NET_IPV6_ADDR_LEN));
#endif

		services_notify_ready(NET_CONFIG_NEED_IPV6);
	}

	if (mgmt_event == NET_EVENT_IPV6_ROUTER_ADD) {
		services_notify_ready(NET_CONFIG_NEED_ROUTER);
	}
}

static void setup_ipv6(struct net_if *iface, uint32_t flags)
{
	struct net_if_addr *ifaddr;
	uint32_t mask = NET_EVENT_IPV6_DAD_SUCCEED;

	if (sizeof(CONFIG_NET_CONFIG_MY_IPV6_ADDR) == 1) {
		/* Empty address, skip setting ANY address in this case */
		goto exit;
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

exit:

	if (!IS_ENABLED(CONFIG_NET_IPV6_DAD) ||
	    net_if_flag_is_set(iface, NET_IF_IPV6_NO_ND)) {
		services_notify_ready(NET_CONFIG_NEED_IPV6);
	}

	return;
}

#else
#define setup_ipv6(...)
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_NATIVE)
static void iface_up_handler(struct net_mgmt_event_callback *cb,
			     uint32_t mgmt_event, struct net_if *iface)
{
	if (mgmt_event == NET_EVENT_IF_UP) {
		NET_INFO("Interface %d (%p) coming up",
			 net_if_get_by_iface(iface), iface);

		k_sem_reset(&counter);
		k_sem_give(&waiter);
	}
}

static bool check_interface(struct net_if *iface)
{
	if (net_if_is_up(iface)) {
		k_sem_reset(&counter);
		k_sem_give(&waiter);
		return true;
	}

	NET_INFO("Waiting interface %d (%p) to be up...",
		 net_if_get_by_iface(iface), iface);

	net_mgmt_init_event_callback(&mgmt_iface_cb, iface_up_handler,
				     NET_EVENT_IF_UP);
	net_mgmt_add_event_callback(&mgmt_iface_cb);

	return false;
}
#else
static bool check_interface(struct net_if *iface)
{
	k_sem_reset(&counter);
	k_sem_give(&waiter);

	return true;
}
#endif

int net_config_init_by_iface(struct net_if *iface, const char *app_info,
			     uint32_t flags, int32_t timeout)
{
#define LOOP_DIVIDER 10
	int loop = timeout / LOOP_DIVIDER;
	int count;

	if (app_info) {
		NET_INFO("%s", app_info);
	}

	if (!iface) {
		iface = net_if_get_default();
	}

	if (timeout < 0) {
		count = -1;
	} else if (timeout == 0) {
		count = 0;
	} else {
		count = LOOP_DIVIDER;
	}

	/* First make sure that network interface is up */
	if (check_interface(iface) == false) {
		k_sem_init(&counter, 1, K_SEM_MAX_LIMIT);

		while (count-- > 0) {
			if (!k_sem_count_get(&counter)) {
				break;
			}

			if (k_sem_take(&waiter, K_MSEC(loop))) {
				if (!k_sem_count_get(&counter)) {
					break;
				}
			}
		}

#if defined(CONFIG_NET_NATIVE)
		net_mgmt_del_event_callback(&mgmt_iface_cb);
#endif
	}

	setup_ipv4(iface);
	setup_dhcpv4(iface);
	setup_ipv6(iface, flags);

	/* Network interface did not come up. */
	if (timeout > 0 && count < 0) {
		NET_ERR("Timeout while waiting network %s", "interface");
		return -ENETDOWN;
	}

	/* Loop here until we are ready to continue. As we might need
	 * to wait multiple events, sleep smaller amounts of data.
	 */
	while (!services_are_ready(flags) && count-- > 0) {
		k_sem_take(&waiter, K_MSEC(loop));
	}

	if (count == -1 && timeout > 0) {
		NET_ERR("Timeout while waiting network %s", "setup");
		return -ETIMEDOUT;
	}

	return 0;
}

int net_config_init(const char *app_info, uint32_t flags,
		    int32_t timeout)
{
	return net_config_init_by_iface(NULL, app_info, flags, timeout);
}

static void iface_find_cb(struct net_if *iface, void *user_data)
{
	struct net_if **iface_to_use = user_data;

	if (*iface_to_use == NULL &&
	    !net_if_flag_is_set(iface, NET_IF_NO_AUTO_START)) {
		*iface_to_use = iface;
		return;
	}
}

int net_config_init_app(const struct device *dev, const char *app_info)
{
	struct net_if *iface = NULL;
	uint32_t flags = 0U;
	int ret;

	if (dev) {
		iface = net_if_lookup_by_dev(dev);
		if (iface == NULL) {
			NET_WARN("No interface for device %p, using default",
				 dev);
		}
	}

	ret = z_net_config_ieee802154_setup();
	if (ret < 0) {
		NET_ERR("Cannot setup IEEE 802.15.4 interface (%d)", ret);
	}

#if defined(CONFIG_NET_IPV6)
	/* Bluetooth is only usable if IPv6 is enabled */
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

	/* Only try to use a network interface that is auto started */
	if (iface == NULL) {
		net_if_foreach(iface_find_cb, &iface);
	}

	/* Initialize the application automatically if needed */
	ret = net_config_init_by_iface(iface, app_info, flags,
				CONFIG_NET_CONFIG_INIT_TIMEOUT * MSEC_PER_SEC);
	if (ret < 0) {
		NET_ERR("Network initialization failed (%d)", ret);
	}

	if (IS_ENABLED(CONFIG_NET_CONFIG_CLOCK_SNTP_INIT)) {
		net_init_clock_via_sntp();
	}

	/* This is activated late as it requires the network stack to be up
	 * and running before syslog messages can be sent to network.
	 */
	if (IS_ENABLED(CONFIG_LOG_BACKEND_NET) &&
	    IS_ENABLED(CONFIG_LOG_BACKEND_NET_AUTOSTART)) {
		const struct log_backend *backend = log_backend_net_get();

		if (!log_backend_is_active(backend)) {
			if (backend->api->init != NULL) {
				backend->api->init(backend);
			}

			log_backend_activate(backend, NULL);
		}
	}

	return ret;
}

#if defined(CONFIG_NET_CONFIG_AUTO_INIT)
static int init_app(void)
{

	(void)net_config_init_app(NULL, "Initializing network");

	return 0;
}

SYS_INIT(init_app, APPLICATION, CONFIG_NET_CONFIG_INIT_PRIO);
#endif /* CONFIG_NET_CONFIG_AUTO_INIT */
