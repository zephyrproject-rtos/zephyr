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
#include <zephyr/logging/log_backend_net.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/dhcpv4.h>
#include <zephyr/net/dhcpv4_server.h>
#include <zephyr/net/dhcpv6.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/dns_resolve.h>
#include <zephyr/net/virtual.h>

#include <zephyr/net/net_config.h>

#include "ieee802154_settings.h"
#include "net_private.h"

extern int net_init_clock_via_sntp(struct net_if *iface,
				   const char *server,
				   int timeout);

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

#if defined(CONFIG_NET_NATIVE_IPV4)

#if defined(CONFIG_NET_DHCPV4)

static void setup_dhcpv4(struct net_if *iface)
{
	NET_INFO("Running dhcpv4 client...");

	net_dhcpv4_start(iface);
}

static void print_dhcpv4_info(struct net_if *iface)
{
#if CONFIG_NET_CONFIG_LOG_LEVEL >= LOG_LEVEL_INF
	char hr_addr[NET_IPV4_ADDR_LEN];
#endif
	ARRAY_FOR_EACH(iface->config.ip.ipv4->unicast, i) {
		struct net_if_addr *if_addr =
					&iface->config.ip.ipv4->unicast[i].ipv4;

		if (if_addr->addr_type != NET_ADDR_DHCP ||
		    !if_addr->is_used) {
			continue;
		}

#if CONFIG_NET_CONFIG_LOG_LEVEL >= LOG_LEVEL_INF
		NET_INFO("IPv4 address: %s",
			 net_addr_ntop(AF_INET, &if_addr->address.in_addr,
				       hr_addr, sizeof(hr_addr)));
		NET_INFO("Lease time: %u seconds",
			 iface->config.dhcpv4.lease_time);
		NET_INFO("Subnet: %s",
			 net_addr_ntop(AF_INET,
				       &iface->config.ip.ipv4->unicast[i].netmask,
				       hr_addr, sizeof(hr_addr)));
		NET_INFO("Router: %s",
			 net_addr_ntop(AF_INET, &iface->config.ip.ipv4->gw,
				       hr_addr, sizeof(hr_addr)));
#endif
		break;
	}
}

#else
#define setup_dhcpv4(...)
#define print_dhcpv4_info(...)
#endif /* CONFIG_NET_DHCPV4 */

static struct net_mgmt_event_callback mgmt4_cb;

static void ipv4_addr_add_handler(struct net_mgmt_event_callback *cb,
				  uint32_t mgmt_event,
				  struct net_if *iface)
{
	if (mgmt_event == NET_EVENT_IPV4_ADDR_ADD) {
		print_dhcpv4_info(iface);

		if (!IS_ENABLED(CONFIG_NET_IPV4_ACD)) {
			services_notify_ready(NET_CONFIG_NEED_IPV4);
		}
	}

	if (mgmt_event == NET_EVENT_IPV4_ACD_SUCCEED) {
		services_notify_ready(NET_CONFIG_NEED_IPV4);
	}
}

#if defined(CONFIG_NET_VLAN) && (CONFIG_NET_CONFIG_MY_VLAN_ID > 0)

static void setup_vlan(struct net_if *iface)
{
	int ret = net_eth_vlan_enable(iface, CONFIG_NET_CONFIG_MY_VLAN_ID);

	if (ret < 0) {
		NET_ERR("Network interface %d (%p): cannot set VLAN tag (%d)",
			net_if_get_by_iface(iface), iface, ret);
	}
}

#else
#define setup_vlan(...)
#endif /* CONFIG_NET_VLAN && (CONFIG_NET_CONFIG_MY_VLAN_ID > 0) */

#if defined(CONFIG_NET_NATIVE_IPV4) && !defined(CONFIG_NET_DHCPV4) && \
	!defined(CONFIG_NET_CONFIG_MY_IPV4_ADDR)
#error "You need to define an IPv4 address or enable DHCPv4!"
#endif

static void setup_ipv4(struct net_if *iface)
{
#if CONFIG_NET_CONFIG_LOG_LEVEL >= LOG_LEVEL_INF
	char hr_addr[NET_IPV4_ADDR_LEN];
#endif
	struct in_addr addr, netmask;

	if (IS_ENABLED(CONFIG_NET_IPV4_ACD) || IS_ENABLED(CONFIG_NET_DHCPV4)) {
		net_mgmt_init_event_callback(&mgmt4_cb, ipv4_addr_add_handler,
					     NET_EVENT_IPV4_ADDR_ADD |
					     NET_EVENT_IPV4_ACD_SUCCEED);
		net_mgmt_add_event_callback(&mgmt4_cb);
	}

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
				  &netmask)) {
			NET_ERR("Invalid netmask: %s",
				CONFIG_NET_CONFIG_MY_IPV4_NETMASK);
		} else {
			net_if_ipv4_set_netmask_by_addr(iface, &addr, &netmask);
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

	if (!IS_ENABLED(CONFIG_NET_IPV4_ACD)) {
		services_notify_ready(NET_CONFIG_NEED_IPV4);
	}
}

#else
#define setup_ipv4(...)
#define setup_dhcpv4(...)
#define setup_vlan(...)
#endif /* CONFIG_NET_NATIVE_IPV4*/

#if defined(CONFIG_NET_NATIVE_IPV6)

#if defined(CONFIG_NET_DHCPV6)
static void setup_dhcpv6(struct net_if *iface)
{
	struct net_dhcpv6_params params = {
		.request_addr = IS_ENABLED(CONFIG_NET_CONFIG_DHCPV6_REQUEST_ADDR),
		.request_prefix = IS_ENABLED(CONFIG_NET_CONFIG_DHCPV6_REQUEST_PREFIX),
	};

	NET_INFO("Running dhcpv6 client...");

	net_dhcpv6_start(iface, &params);
}
#else /* CONFIG_NET_DHCPV6 */
#define setup_dhcpv6(...)
#endif /* CONFIG_NET_DHCPV6 */

#if !defined(CONFIG_NET_CONFIG_DHCPV6_REQUEST_ADDR) && \
	!defined(CONFIG_NET_CONFIG_MY_IPV6_ADDR)
#error "You need to define an IPv6 address or enable DHCPv6!"
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

		if (ifaddr->addr_type == NET_ADDR_DHCP) {
			char remaining_str[] = "infinite";
			uint32_t remaining;

			remaining = net_timeout_remaining(&ifaddr->lifetime,
							  k_uptime_get_32());

			if (!ifaddr->is_infinite) {
				snprintk(remaining_str, sizeof(remaining_str),
					 "%u", remaining);
			}

			NET_INFO("Lifetime: %s seconds", remaining_str);
		}
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
#define setup_dhcpv6(...)
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

	if (!iface) {
		return -ENOENT;
	}

	if (net_if_flag_is_set(iface, NET_IF_NO_AUTO_START)) {
		return -ENETDOWN;
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

	setup_vlan(iface);
	setup_ipv4(iface);
	setup_dhcpv4(iface);
	setup_ipv6(iface, flags);
	setup_dhcpv6(iface);

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

static int net_config_init_app_by_kconfig(const struct device *dev, const char *app_info)
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

#ifdef CONFIG_NET_L2_IEEE802154_SECURITY
	struct ieee802154_security_params sec_params = {
		.key = CONFIG_NET_CONFIG_IEEE802154_SECURITY_KEY,
		.key_len = sizeof(CONFIG_NET_CONFIG_IEEE802154_SECURITY_KEY),
		.key_mode = CONFIG_NET_CONFIG_IEEE802154_SECURITY_KEY_MODE,
		.level = CONFIG_NET_CONFIG_IEEE802154_SECURITY_LEVEL,
	};
	struct ieee802154_security_params *kconfig_sec_params_ptr = &sec_params;
#else
#define kconfig_sec_params_ptr 0
#endif /* CONFIG_NET_L2_IEEE802154_SECURITY */

	ret = z_net_config_ieee802154_setup(iface,
					    CONFIG_NET_CONFIG_IEEE802154_CHANNEL,
					    CONFIG_NET_CONFIG_IEEE802154_PAN_ID,
					    CONFIG_NET_CONFIG_IEEE802154_RADIO_TX_POWER,
					    kconfig_sec_params_ptr);
	if (ret < 0) {
		NET_ERR("Cannot setup IEEE 802.15.4 interface (%d)", ret);
	}

	/* Only try to use a network interface that is auto started */
	if (iface == NULL) {
		net_if_foreach(iface_find_cb, &iface);
	}

	if (!iface) {
		NET_WARN("No auto-started network interface - "
			 "network-bound app initialization skipped.");
		return 0;
	}

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
	ret = net_config_init_by_iface(iface, app_info, flags,
				CONFIG_NET_CONFIG_INIT_TIMEOUT * MSEC_PER_SEC);
	if (ret < 0) {
		NET_ERR("Network initialization failed (%d)", ret);
	}

	if (IS_ENABLED(CONFIG_NET_CONFIG_CLOCK_SNTP_INIT)) {
		net_init_clock_via_sntp(NULL,
					COND_CODE_1(CONFIG_NET_CONFIG_CLOCK_SNTP_INIT,
						    (CONFIG_NET_CONFIG_SNTP_INIT_SERVER,),
						    (NULL,))
					COND_CODE_1(CONFIG_NET_CONFIG_CLOCK_SNTP_INIT,
						    (CONFIG_NET_CONFIG_SNTP_INIT_TIMEOUT),
						    (0)));
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

static struct net_if *get_interface(const struct net_init_config *config,
				    int config_ifindex,
				    const struct device *dev,
				    const char **iface_name)
{
	const struct net_init_config_iface *cfg;
	struct net_if *iface = NULL;
	const char *name;

	NET_ASSERT(IN_RANGE(config_ifindex, 0, ARRAY_SIZE(config->iface) - 1));

	cfg = &config->iface[config_ifindex];

	name = cfg->new_name;
	if (name != NULL) {
		iface = net_if_get_by_index(net_if_get_by_name(name));
	}

	if (iface == NULL) {
		name = cfg->name;

		if (name != NULL) {
			iface = net_if_get_by_index(net_if_get_by_name(name));
		}
	}

	if (iface == NULL) {
		/* Get the interface by device */
		const struct device *iface_dev;

		name = cfg->dev;

		iface_dev = device_get_binding(name);
		if (iface_dev) {
			iface = net_if_lookup_by_dev(iface_dev);
		}
	}

	if (iface_name != NULL) {
		*iface_name = name;
	}

	return iface;
}

static void ipv6_setup_from_generated(struct net_if *iface,
				      int ifindex,
				      const struct net_init_config_iface *cfg)
{
#if defined(CONFIG_NET_IPV6)
	const struct net_init_config_ipv6 *ipv6 = &cfg->ipv6;
	struct net_if_addr *ifaddr;
	struct net_if_mcast_addr *ifmaddr;

	if (!ipv6->is_enabled) {
		NET_DBG("Skipping %s setup for iface %d", "IPv6", ifindex);
		net_if_flag_clear(iface, NET_IF_IPV6);
		return;
	}

	/* First set all the static addresses and then enable DHCP */
	ARRAY_FOR_EACH(ipv6->address, j) {
		if (net_ipv6_is_addr_unspecified(&ipv6->address[j])) {
			continue;
		}

		ifaddr = net_if_ipv6_addr_add(
				iface,
				(struct in6_addr *)&ipv6->address[j],
				/* If DHCPv6 is enabled, then allow address
				 * to be overridden.
				 */
				COND_CODE_1(CONFIG_NET_DHCPV6,
					    (ipv6->dhcpv6.is_enabled),
					    (false)) ?
				NET_ADDR_OVERRIDABLE : NET_ADDR_MANUAL,
				0);
		if (ifaddr == NULL) {
			NET_DBG("Cannot %s %s %s to iface %d", "add", "address",
				net_sprint_ipv6_addr(&ipv6->address[j]),
				ifindex);
			continue;
		}

		NET_DBG("Added %saddress %s to iface %d", "",
			net_sprint_ipv6_addr(&ipv6->address[j]),
			ifindex);
	}

	ARRAY_FOR_EACH(ipv6->mcast_address, j) {
		if (net_ipv6_is_addr_unspecified(&ipv6->mcast_address[j])) {
			continue;
		}

		ifmaddr = net_if_ipv6_maddr_add(iface,
						(struct in6_addr *)&ipv6->mcast_address[j]);
		if (ifmaddr == NULL) {
			NET_DBG("Cannot %s %s %s to iface %d", "add", "address",
				net_sprint_ipv6_addr(&ipv6->mcast_address[j]),
				ifindex);
			continue;
		}

		NET_DBG("Added %saddress %s to iface %d", "multicast ",
			net_sprint_ipv6_addr(&ipv6->mcast_address[j]),
			ifindex);
	}

	ARRAY_FOR_EACH(ipv6->prefix, j) {
		struct net_if_ipv6_prefix *prefix;

		if (net_ipv6_is_addr_unspecified(&ipv6->prefix[j].address)) {
			continue;
		}

		prefix = net_if_ipv6_prefix_add(iface,
						(struct in6_addr *)&ipv6->prefix[j].address,
						ipv6->prefix[j].len,
						ipv6->prefix[j].lifetime);
		if (prefix == NULL) {
			NET_DBG("Cannot %s %s %s to iface %d", "add", "prefix",
				net_sprint_ipv6_addr(&ipv6->prefix[j].address),
				ifindex);
			continue;
		}

		NET_DBG("Added %saddress %s to iface %d", "prefix ",
			net_sprint_ipv6_addr(&ipv6->prefix[j].address),
			ifindex);
	}

	if (ipv6->hop_limit > 0) {
		net_if_ipv6_set_hop_limit(iface, ipv6->hop_limit);
	}

	if (ipv6->mcast_hop_limit > 0) {
		net_if_ipv6_set_mcast_hop_limit(iface, ipv6->mcast_hop_limit);
	}

	if (COND_CODE_1(CONFIG_NET_DHCPV6, (ipv6->dhcpv6.is_enabled), (false))) {
		struct net_dhcpv6_params params = {
			.request_addr = COND_CODE_1(CONFIG_NET_DHCPV6,
						    (ipv6->dhcpv6.do_request_addr),
						    (false)),
			.request_prefix = COND_CODE_1(CONFIG_NET_DHCPV6,
						      (ipv6->dhcpv6.do_request_prefix),
						      (false)),
		};

		net_dhcpv6_start(iface, &params);
	}
#endif
}

static void ipv4_setup_from_generated(struct net_if *iface,
				      int ifindex,
				      const struct net_init_config_iface *cfg)
{
#if defined(CONFIG_NET_IPV4)
	const struct net_init_config_ipv4 *ipv4 = &cfg->ipv4;
	struct net_if_addr *ifaddr;
	struct net_if_mcast_addr *ifmaddr;

	if (!ipv4->is_enabled) {
		NET_DBG("Skipping %s setup for iface %d", "IPv4", ifindex);
		net_if_flag_clear(iface, NET_IF_IPV4);
		return;
	}

	/* First set all the static addresses and then enable DHCP */
	ARRAY_FOR_EACH(cfg->ipv4.address, j) {
		if (net_ipv4_is_addr_unspecified(&cfg->ipv4.address[j].ipv4)) {
			continue;
		}

		ifaddr = net_if_ipv4_addr_add(iface,
				(struct in_addr *)&ipv4->address[j].ipv4,
				/* If DHCPv4 is enabled, then allow address
				 * to be overridden.
				 */
				ipv4->is_dhcpv4 ? NET_ADDR_OVERRIDABLE :
					      NET_ADDR_MANUAL,
				0);

		if (ifaddr == NULL) {
			NET_DBG("Cannot %s %s %s to iface %d", "add", "address",
				net_sprint_ipv4_addr(&ipv4->address[j].ipv4),
				ifindex);
			continue;
		}

		NET_DBG("Added %saddress %s to iface %d", "",
			net_sprint_ipv4_addr(&ipv4->address[j].ipv4),
			ifindex);

		if (ipv4->address[j].netmask_len > 0) {
			struct in_addr netmask = { 0 };

			netmask.s_addr = BIT_MASK(ipv4->address[j].netmask_len);

			net_if_ipv4_set_netmask_by_addr(iface,
							&ipv4->address[j].ipv4,
							&netmask);

			NET_DBG("Added %saddress %s to iface %d", "netmask ",
				net_sprint_ipv4_addr(&netmask),
				ifindex);
		}
	}

	ARRAY_FOR_EACH(ipv4->mcast_address, j) {
		if (net_ipv4_is_addr_unspecified(&ipv4->mcast_address[j])) {
			continue;
		}

		ifmaddr = net_if_ipv4_maddr_add(iface, &ipv4->mcast_address[j]);
		if (ifmaddr == NULL) {
			NET_DBG("Cannot %s %s %s to iface %d", "add", "address",
				net_sprint_ipv4_addr(&ipv4->mcast_address[j]),
				ifindex);
			continue;
		}

		NET_DBG("Added %saddress %s to iface %d", "multicast ",
			net_sprint_ipv4_addr(&ipv4->mcast_address[j]),
			ifindex);
	}

	if (ipv4->ttl > 0) {
		net_if_ipv4_set_ttl(iface, ipv4->ttl);
	}

	if (ipv4->mcast_ttl > 0) {
		net_if_ipv4_set_mcast_ttl(iface, ipv4->mcast_ttl);
	}

	if (!net_ipv4_is_addr_unspecified(&ipv4->gateway)) {
		net_if_ipv4_set_gw(iface, &ipv4->gateway);

		NET_DBG("Added %saddress %s to iface %d", "gateway ",
			net_sprint_ipv4_addr(&ipv4->gateway),
			ifindex);
	}

	if (IS_ENABLED(CONFIG_NET_DHCPV4) && ipv4->is_dhcpv4) {
		net_dhcpv4_start(iface);
	}

	if (COND_CODE_1(CONFIG_NET_DHCPV4_SERVER,
			(ipv4->dhcpv4_server.is_enabled), (false))) {
		net_dhcpv4_server_start(iface,
			COND_CODE_1(CONFIG_NET_DHCPV4_SERVER,
				    ((struct in_addr *)&ipv4->dhcpv4_server.base_addr),
				    (&((struct in_addr){ 0 }))));
	}
#endif
}

static void vlan_setup_from_generated(const struct net_init_config *config,
				      const struct net_init_config_iface *cfg)
{
#if defined(CONFIG_NET_VLAN)
	const struct net_init_config_vlan *vlan = &cfg->vlan;
	struct net_if *bound = NULL;
	int ret, ifindex;

	if (!vlan->is_enabled) {
		return;
	}

	bound = get_interface(config, cfg->bind_to - 1, NULL, NULL);
	if (bound == NULL) {
		NET_DBG("Cannot find VLAN bound interface %d", cfg->bind_to - 1);
		return;
	}

	ifindex = net_if_get_by_iface(bound);

	ret = net_eth_vlan_enable(bound, vlan->tag);
	if (ret < 0) {
		NET_DBG("Cannot %s %s %s to iface %d", "add", "VLAN", "tag", ifindex);
		NET_DBG("Cannot enable %s for %s %d (%d)", "VLAN", "tag",
			vlan->tag, ret);
	} else {
		NET_DBG("Added %s %s %d to iface %d", "VLAN", "tag", vlan->tag,
			net_if_get_by_iface(
				net_eth_get_vlan_iface(
					net_if_get_by_index(ifindex), vlan->tag)));

		if (cfg->new_name != NULL) {
			struct net_if *iface;

			iface = net_eth_get_vlan_iface(bound, vlan->tag);

			ret = net_if_set_name(iface, cfg->new_name);
			if (ret < 0) {
				NET_DBG("Cannot rename interface %d to \"%s\" (%d)",
					ifindex, cfg->new_name, ret);
			} else {
				NET_DBG("Changed interface %d name to \"%s\"", ifindex,
					cfg->new_name);
			}
		}
	}
#endif
}

static void virtual_setup_from_generated(struct net_if *iface,
					 int ifindex,
					 const struct net_init_config *config,
					 const struct net_init_config_iface *cfg)
{
#if defined(CONFIG_NET_L2_VIRTUAL)
	const struct virtual_interface_api *api = net_if_get_device(iface)->api;
	struct net_if *bound = NULL;
	int ret;

	if (net_if_l2(iface) != &NET_L2_GET_NAME(VIRTUAL)) {
		return;
	}

	/* VLAN interfaces are handled separately */
	if (api->get_capabilities(iface) & VIRTUAL_INTERFACE_VLAN) {
		return;
	}

	if (cfg->bind_to > 0) {
		bound = get_interface(config, cfg->bind_to - 1, NULL, NULL);
	}

	if (bound == NULL) {
		NET_DBG("Cannot %s %s %s to iface %d", "add", "virtual", "interface",
			cfg->bind_to - 1);
		return;
	}

	ret = net_virtual_interface_attach(iface, bound);
	if (ret < 0) {
		if (ret != -EALREADY) {
			NET_DBG("Cannot %s %s %s to iface %d (%d)", "attach",
				"virtual", "interface",
				net_if_get_by_iface(bound), ret);
		}
	} else {
		NET_DBG("Added %s %s %d to iface %d", "virtual", "interface", ifindex,
			net_if_get_by_iface(bound));
	}
#endif
}

static int net_config_init_app_from_generated(const struct device *dev,
					      const char *app_info)
{
	const struct net_init_config *config;
	int ret, ifindex;

	config = net_config_get_init_config();
	if (config == NULL) {
		NET_ERR("Network configuration not found.");
		return -ENOENT;
	}

	/* We first need to setup any VLAN interfaces so that other
	 * interfaces can use them (the interface name etc are correctly
	 * set so that referencing works ok).
	 */
	if (IS_ENABLED(CONFIG_NET_VLAN)) {
		ARRAY_FOR_EACH(config->iface, i) {
			const struct net_init_config_iface *cfg = &config->iface[i];

			vlan_setup_from_generated(config, cfg);
		}
	}

	ARRAY_FOR_EACH(config->iface, i) {
		const struct net_init_config_iface *cfg;
		struct net_if *iface;
		const char *name;

		cfg = &config->iface[i];

		iface = get_interface(config, i, dev, &name);
		if (iface == NULL || name == NULL) {
			NET_WARN("No such interface \"%s\" found.",
				 name == NULL ? "<?>" : name);
			continue;
		}

		ifindex = net_if_get_by_iface(iface);

		NET_DBG("Configuring interface %d (%p)", ifindex, iface);

		/* Do we need to change the interface name */
		if (cfg->new_name != NULL) {
			ret = net_if_set_name(iface, cfg->new_name);
			if (ret < 0) {
				NET_DBG("Cannot rename interface %d to \"%s\" (%d)",
					ifindex, cfg->new_name, ret);
				continue;
			}

			NET_DBG("Changed interface %d name to \"%s\"", ifindex,
				cfg->new_name);
		}

		if (cfg->is_default) {
			net_if_set_default(iface);

			NET_DBG("Setting interface %d as default", ifindex);
		}

		ipv6_setup_from_generated(iface, ifindex, cfg);
		ipv4_setup_from_generated(iface, ifindex, cfg);
		virtual_setup_from_generated(iface, ifindex, config, cfg);
	}

	if (IS_ENABLED(CONFIG_NET_CONFIG_CLOCK_SNTP_INIT) && config->is_sntp) {
#if defined(CONFIG_SNTP)
		struct net_if *iface = NULL;

		if (config->sntp.bind_to > 0) {
			iface = get_interface(config,
					      config->sntp.bind_to - 1,
					      dev,
					      NULL);
		}

		ret = net_init_clock_via_sntp(iface, config->sntp.server,
					      config->sntp.timeout);
		if (ret < 0) {
			NET_DBG("Cannot init SNTP interface %d (%d)",
				net_if_get_by_iface(iface), ret);
		} else {
			NET_DBG("Initialized SNTP to use interface %d",
				net_if_get_by_iface(iface));
		}
#endif
	}

	if (IS_ENABLED(CONFIG_NET_L2_IEEE802154) &&
	    IS_ENABLED(CONFIG_NET_CONFIG_SETTINGS) &&
	    config->is_ieee802154) {
#ifdef CONFIG_NET_L2_IEEE802154_SECURITY
		struct ieee802154_security_params sec_params =
					config->ieee802154.sec_params;
	struct ieee802154_security_params *generated_sec_params_ptr = &sec_params;
#else
#define generated_sec_params_ptr 0
#endif /* CONFIG_NET_L2_IEEE802154_SECURITY */

		struct net_if *iface = NULL;

		if (COND_CODE_1(CONFIG_NET_L2_IEEE802154,
				(config->ieee802154.bind_to), (0)) > 0) {
			iface = get_interface(
				config,
				COND_CODE_1(CONFIG_NET_L2_IEEE802154,
					    (config->ieee802154.bind_to - 1), (0)),
				dev,
				NULL);
		}

		ret = z_net_config_ieee802154_setup(
			    IF_ENABLED(CONFIG_NET_L2_IEEE802154,
				       (iface,
					config->ieee802154.channel,
					config->ieee802154.pan_id,
					config->ieee802154.tx_power,
					generated_sec_params_ptr)));
		if (ret < 0) {
			NET_ERR("Cannot setup IEEE 802.15.4 interface (%d)", ret);
		}
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

	return 0;
}

int net_config_init_app(const struct device *dev, const char *app_info)
{
	if (IS_ENABLED(CONFIG_NET_CONFIG_USE_YAML)) {
		return net_config_init_app_from_generated(dev, app_info);
	}

	return net_config_init_app_by_kconfig(dev, app_info);
}

#if defined(CONFIG_NET_CONFIG_AUTO_INIT)
static int init_app(void)
{
	(void)net_config_init_app(NULL, "Initializing network");

	return 0;
}

SYS_INIT(init_app, APPLICATION, CONFIG_NET_CONFIG_INIT_PRIO);

const struct net_init_config *net_config_get_init_config(void)
{
#if defined(CONFIG_NET_CONFIG_USE_YAML)
#define NET_CONFIG_ENABLE_DATA
#include "net_init_config.inc"

BUILD_ASSERT(ARRAY_SIZE(net_init_config_data.iface) == NET_CONFIG_IFACE_COUNT);

#if defined(CONFIG_NET_DHCPV4_SERVER)
BUILD_ASSERT(NET_CONFIG_DHCPV4_SERVER_INSTANCE_COUNT == CONFIG_NET_DHCPV4_SERVER_INSTANCES);

/* If we are starting DHCPv4 server, then the socket service needs to be started before
 * this config lib as the server will need to use the socket service.
 */
BUILD_ASSERT(CONFIG_NET_SOCKETS_SERVICE_INIT_PRIO < CONFIG_NET_CONFIG_INIT_PRIO);
#endif

	return &net_init_config_data;
#else
	return NULL;
#endif
}

#endif /* CONFIG_NET_CONFIG_AUTO_INIT */
