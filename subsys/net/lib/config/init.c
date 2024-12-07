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
#include <zephyr/net/ipv4_autoconf.h>
#include <zephyr/net/net_config.h>

#include "ieee802154_settings.h"
#include "net_private.h"

#include "net_init_config.inc"

extern int net_init_clock_via_sntp(struct net_if *iface,
				   const char *server,
				   int timeout);

static K_SEM_DEFINE(waiter, 0, 1);
static K_SEM_DEFINE(counter, 0, UINT_MAX);

#if defined(CONFIG_NET_NATIVE)
static struct net_mgmt_event_callback mgmt_iface_cb;

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

	/* Network interface did not come up. */
	if (timeout > 0 && count < 0) {
		NET_ERR("Timeout while waiting network %s", "interface");
		return -ENETDOWN;
	}

	return 0;
}

static struct net_if *get_interface(const struct net_init_config *config,
				    int config_ifindex,
				    const struct device *dev,
				    const char **iface_name)
{
	const struct net_init_config_network_interfaces *cfg;
	struct net_if *iface = NULL;
	const char *name;

	NET_ASSERT(IN_RANGE(config_ifindex, 0, ARRAY_SIZE(config->network_interfaces) - 1));

	cfg = &config->network_interfaces[config_ifindex];

	name = cfg->set_name;
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

		name = cfg->device_name;

		iface_dev = device_get_binding(name);
		if (iface_dev) {
			iface = net_if_lookup_by_dev(iface_dev);
		}
	}

	/* Use the default interface if nothing is found */
	if (iface == NULL && name == NULL) {
		static char ifname[CONFIG_NET_INTERFACE_NAME_LEN + 1];

		iface = net_if_get_default();
		(void)net_if_get_name(iface, ifname, sizeof(ifname));
		name = ifname;
	}

	if (iface_name != NULL) {
		*iface_name = name;
	}

	return iface;
}

static void ipv6_setup(struct net_if *iface,
		       int ifindex,
		       const struct net_init_config_network_interfaces *cfg)
{
#if defined(CONFIG_NET_IPV6)
	const struct net_init_config_ipv6 *ipv6 = &cfg->ipv6;
	struct net_if_mcast_addr *ifmaddr;
	struct net_if_addr *ifaddr;
	bool ret;

	if (!ipv6->status) {
		NET_DBG("Skipping IPv%c setup for iface %d", '6', ifindex);
		net_if_flag_clear(iface, NET_IF_IPV6);
		return;
	}

	/* First set all the static addresses and then enable DHCP */
	ARRAY_FOR_EACH(ipv6->ipv6_addresses, j) {
		struct sockaddr_in6 addr = { 0 };
		uint8_t prefix_len;

		if (ipv6->ipv6_addresses[j].value == NULL ||
		    ipv6->ipv6_addresses[j].value[0] == '\0') {
			continue;
		}

		ret = net_ipaddr_mask_parse(ipv6->ipv6_addresses[j].value,
					    strlen(ipv6->ipv6_addresses[j].value),
					    (struct sockaddr *)&addr, &prefix_len);
		if (!ret) {
			NET_DBG("Invalid IPv%c %s address \"%s\"", '6', "unicast",
				ipv6->ipv6_addresses[j].value);
			continue;
		}

		if (net_ipv6_is_addr_unspecified(&addr.sin6_addr)) {
			continue;
		}

		ifaddr = net_if_ipv6_addr_add(
				iface,
				&addr.sin6_addr,
				/* If DHCPv6 is enabled, then allow address
				 * to be overridden.
				 */
				COND_CODE_1(CONFIG_NET_DHCPV6,
					    (ipv6->dhcpv6.status),
					    (false)) ?
				NET_ADDR_OVERRIDABLE : NET_ADDR_MANUAL,
				0);
		if (ifaddr == NULL) {
			NET_DBG("Cannot %s %s %s to iface %d", "add", "address",
				net_sprint_ipv6_addr(&addr.sin6_addr),
				ifindex);
			continue;
		}

		NET_DBG("Added %s address %s to iface %d", "unicast",
			net_sprint_ipv6_addr(&addr.sin6_addr),
			ifindex);
	}

	ARRAY_FOR_EACH(ipv6->ipv6_multicast_addresses, j) {
		struct sockaddr_in6 addr = { 0 };

		if (ipv6->ipv6_multicast_addresses[j].value == NULL ||
		    ipv6->ipv6_multicast_addresses[j].value[0] == '\0') {
			continue;
		}

		ret = net_ipaddr_mask_parse(ipv6->ipv6_multicast_addresses[j].value,
					    strlen(ipv6->ipv6_multicast_addresses[j].value),
					    (struct sockaddr *)&addr, NULL);
		if (!ret) {
			NET_DBG("Invalid IPv%c %s address \"%s\"", '6', "multicast",
				ipv6->ipv6_multicast_addresses[j].value);
			continue;
		}

		if (net_ipv6_is_addr_unspecified(&addr.sin6_addr)) {
			continue;
		}

		ifmaddr = net_if_ipv6_maddr_add(iface, &addr.sin6_addr);
		if (ifmaddr == NULL) {
			NET_DBG("Cannot %s %s %s to iface %d", "add", "address",
				net_sprint_ipv6_addr(&addr.sin6_addr),
				ifindex);
			continue;
		}

		NET_DBG("Added %s address %s to iface %d", "multicast",
			net_sprint_ipv6_addr(&addr.sin6_addr),
			ifindex);
	}

	ARRAY_FOR_EACH(ipv6->prefixes, j) {
		struct net_if_ipv6_prefix *prefix;
		struct sockaddr_in6 addr = { 0 };

		if (ipv6->prefixes[j].address == NULL ||
		    ipv6->prefixes[j].address[0] == '\0') {
			continue;
		}

		ret = net_ipaddr_mask_parse(ipv6->prefixes[j].address,
					    strlen(ipv6->prefixes[j].address),
					    (struct sockaddr *)&addr, NULL);
		if (!ret) {
			NET_DBG("Invalid IPv%c %s address \"%s\"", '6', "prefix",
				ipv6->prefixes[j].address);
			continue;
		}

		if (net_ipv6_is_addr_unspecified(&addr.sin6_addr)) {
			continue;
		}

		prefix = net_if_ipv6_prefix_add(iface,
						&addr.sin6_addr,
						ipv6->prefixes[j].len,
						ipv6->prefixes[j].lifetime);
		if (prefix == NULL) {
			NET_DBG("Cannot %s %s %s to iface %d", "add", "prefix",
				net_sprint_ipv6_addr(&addr.sin6_addr),
				ifindex);
			continue;
		}

		NET_DBG("Added %s address %s to iface %d", "prefix",
			net_sprint_ipv6_addr(&addr.sin6_addr),
			ifindex);
	}

	if (ipv6->hop_limit > 0) {
		net_if_ipv6_set_hop_limit(iface, ipv6->hop_limit);
	}

	if (ipv6->multicast_hop_limit > 0) {
		net_if_ipv6_set_mcast_hop_limit(iface, ipv6->multicast_hop_limit);
	}

	if (COND_CODE_1(CONFIG_NET_DHCPV6, (ipv6->dhcpv6.status), (false))) {
		struct net_dhcpv6_params params = {
			.request_addr = COND_CODE_1(CONFIG_NET_DHCPV6,
						    (ipv6->dhcpv6.do_request_address),
						    (false)),
			.request_prefix = COND_CODE_1(CONFIG_NET_DHCPV6,
						      (ipv6->dhcpv6.do_request_prefix),
						      (false)),
		};

		net_dhcpv6_start(iface, &params);
	}
#endif
}

static void ipv4_setup(struct net_if *iface,
		       int ifindex,
		       const struct net_init_config_network_interfaces *cfg)
{
#if defined(CONFIG_NET_IPV4)
	const struct net_init_config_ipv4 *ipv4 = &cfg->ipv4;
	struct net_if_addr *ifaddr;
	struct net_if_mcast_addr *ifmaddr;
	bool ret;

	if (!ipv4->status) {
		NET_DBG("Skipping IPv%c setup for iface %d", '4', ifindex);
		net_if_flag_clear(iface, NET_IF_IPV4);
		return;
	}

	/* First set all the static addresses and then enable DHCP */
	ARRAY_FOR_EACH(ipv4->ipv4_addresses, j) {
		struct sockaddr_in addr = { 0 };
		uint8_t mask_len = 0;

		if (ipv4->ipv4_addresses[j].value == NULL ||
		    ipv4->ipv4_addresses[j].value[0] == '\0') {
			continue;
		}

		ret = net_ipaddr_mask_parse(ipv4->ipv4_addresses[j].value,
					    strlen(ipv4->ipv4_addresses[j].value),
					    (struct sockaddr *)&addr, &mask_len);
		if (!ret) {
			NET_DBG("Invalid IPv%c %s address \"%s\"", '4', "unicast",
				ipv4->ipv4_addresses[j].value);
			continue;
		}

		if (net_ipv4_is_addr_unspecified(&addr.sin_addr)) {
			continue;
		}

		ifaddr = net_if_ipv4_addr_add(
				iface,
				&addr.sin_addr,
				/* If DHCPv4 is enabled, then allow address
				 * to be overridden.
				 */
				ipv4->dhcpv4_enabled ? NET_ADDR_OVERRIDABLE :
						       NET_ADDR_MANUAL,
				0);

		if (ifaddr == NULL) {
			NET_DBG("Cannot %s %s %s to iface %d", "add", "address",
				net_sprint_ipv4_addr(&addr.sin_addr),
				ifindex);
			continue;
		}

		/* Wait until Address Conflict Detection is ok.
		 * DHCPv4 server startup will fail if the address is not in
		 * preferred state.
		 */
		if (IS_ENABLED(CONFIG_NET_IPV4_ACD) &&
		    (COND_CODE_1(CONFIG_NET_DHCPV4_SERVER,
				 (ipv4->dhcpv4_server.status), (false)))) {
			if (WAIT_FOR(ifaddr->addr_state == NET_ADDR_PREFERRED,
				     USEC_PER_MSEC * MSEC_PER_SEC * 2 /* 2 sec */,
				     k_msleep(100)) == false) {
				NET_DBG("Address %s still is not preferred",
					net_sprint_ipv4_addr(&addr.sin_addr));
			}
		}

		NET_DBG("Added %s address %s to iface %d", "unicast",
			net_sprint_ipv4_addr(&addr.sin_addr),
			ifindex);

		if (mask_len > 0) {
			struct in_addr netmask = { 0 };

			netmask.s_addr = BIT_MASK(mask_len);

			net_if_ipv4_set_netmask_by_addr(iface,
							&addr.sin_addr,
							&netmask);

			NET_DBG("Added %s address %s to iface %d", "netmask",
				net_sprint_ipv4_addr(&netmask),
				ifindex);
		}
	}

	ARRAY_FOR_EACH(ipv4->ipv4_multicast_addresses, j) {
		struct sockaddr_in addr = { 0 };

		if (ipv4->ipv4_multicast_addresses[j].value == NULL ||
		    ipv4->ipv4_multicast_addresses[j].value[0] == '\0') {
			continue;
		}

		ret = net_ipaddr_mask_parse(ipv4->ipv4_multicast_addresses[j].value,
					    strlen(ipv4->ipv4_multicast_addresses[j].value),
					    (struct sockaddr *)&addr, NULL);
		if (!ret) {
			NET_DBG("Invalid IPv%c %s address \"%s\"", '4', "multicast",
				ipv4->ipv4_addresses[j].value);
			continue;
		}

		if (net_ipv4_is_addr_unspecified(&addr.sin_addr)) {
			continue;
		}

		ifmaddr = net_if_ipv4_maddr_add(iface, &addr.sin_addr);
		if (ifmaddr == NULL) {
			NET_DBG("Cannot %s %s %s to iface %d", "add", "address",
				net_sprint_ipv4_addr(&addr.sin_addr),
				ifindex);
			continue;
		}

		NET_DBG("Added %s address %s to iface %d", "multicast",
			net_sprint_ipv4_addr(&addr.sin_addr),
			ifindex);
	}

	if (ipv4->time_to_live > 0) {
		net_if_ipv4_set_ttl(iface, ipv4->time_to_live);
	}

	if (ipv4->multicast_time_to_live > 0) {
		net_if_ipv4_set_mcast_ttl(iface, ipv4->multicast_time_to_live);
	}

	if (ipv4->gateway != NULL && ipv4->gateway[0] != '\0') {
		struct sockaddr_in addr = { 0 };

		ret = net_ipaddr_mask_parse(ipv4->gateway,
					    strlen(ipv4->gateway),
					    (struct sockaddr *)&addr, NULL);
		if (!ret) {
			NET_DBG("Invalid IPv%c %s address \"%s\"", '4', "geteway",
				ipv4->gateway);
		} else {
			if (!net_ipv4_is_addr_unspecified(&addr.sin_addr)) {
				net_if_ipv4_set_gw(iface, &addr.sin_addr);

				NET_DBG("Added %s address %s to iface %d", "gateway",
					net_sprint_ipv4_addr(&addr.sin_addr),
					ifindex);
			}
		}
	}

	if (IS_ENABLED(CONFIG_NET_DHCPV4) && ipv4->dhcpv4_enabled) {
		NET_DBG("DHCPv4 client started");
		net_dhcpv4_start(iface);
	}

	if (COND_CODE_1(CONFIG_NET_DHCPV4_SERVER,
			(ipv4->dhcpv4_server.status), (false))) {
		struct sockaddr_in addr = { 0 };

		if (ipv4->dhcpv4_server.base_address != NULL) {
			ret = net_ipaddr_mask_parse(ipv4->dhcpv4_server.base_address,
						    strlen(ipv4->dhcpv4_server.base_address),
						    (struct sockaddr *)&addr, NULL);
			if (!ret) {
				NET_DBG("Invalid IPv%c %s address \"%s\"", '4', "DHCPv4 base",
					ipv4->dhcpv4_server.base_address);
			} else {
				int retval;

				retval = net_dhcpv4_server_start(iface,
					COND_CODE_1(CONFIG_NET_DHCPV4_SERVER,
						    (&addr.sin_addr),
						    (&((struct in_addr){ 0 }))));
				if (retval < 0) {
					NET_DBG("DHCPv4 server start failed (%d)", retval);
				} else {
					NET_DBG("DHCPv4 server started");
				}
			}
		}
	}

	if (IS_ENABLED(CONFIG_NET_IPV4_AUTO) && ipv4->ipv4_autoconf_enabled) {
		NET_DBG("IPv4 autoconf started");
		net_ipv4_autoconf_start(iface);
	}
#endif
}

static void vlan_setup(const struct net_init_config *config,
		       const struct net_init_config_network_interfaces *cfg)
{
#if defined(CONFIG_NET_VLAN)
	const struct net_init_config_vlan *vlan = &cfg->vlan;
	struct net_if *bound = NULL;
	int ret, ifindex;

	if (!vlan->status) {
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
		NET_DBG("Cannot enable %s for %s %d (%d)", "VLAN", "tag", vlan->tag, ret);
		return;
	}

	NET_DBG("Added %s %s %d to iface %d", "VLAN", "tag", vlan->tag,
		net_if_get_by_iface(net_eth_get_vlan_iface(
					net_if_get_by_index(ifindex),
					vlan->tag)));

	if (cfg->set_name != NULL) {
		struct net_if *iface;

		iface = net_eth_get_vlan_iface(bound, vlan->tag);

		ret = net_if_set_name(iface, cfg->set_name);
		if (ret < 0) {
			NET_DBG("Cannot rename interface %d to \"%s\" (%d)",
				ifindex, cfg->set_name, ret);
			return;
		}

		NET_DBG("Changed interface %d name to \"%s\"", ifindex,
			cfg->set_name);
	}
#endif
}

static void virtual_iface_setup(struct net_if *iface,
				int ifindex,
				const struct net_init_config *config,
				const struct net_init_config_network_interfaces *cfg)
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

		return;
	}

	NET_DBG("Added %s %s %d to iface %d", "virtual", "interface", ifindex,
		net_if_get_by_iface(bound));
#endif
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

static int wait_for_interface(const struct net_init_config_network_interfaces *ifaces,
			      size_t iface_count)
{
	struct net_if *iface = NULL;

	ARG_UNUSED(ifaces);
	ARG_UNUSED(iface_count);

	net_if_foreach(iface_find_cb, &iface);

	if (!iface) {
		NET_WARN("No auto-started network interface - "
			 "network-bound app initialization skipped.");
		return 0;
	}

	/* TODO: implement waiting of interface(s) */
	return 0;
}

struct iface_str_flags {
	enum net_if_flag flag;
	const char * const str;
};

#define FLAG(val) { .flag = val, .str = STRINGIFY(val) }

#if defined(CONFIG_NET_TEST)
enum net_if_flag get_iface_flag(const char *flag_str, bool *clear)
#else
static enum net_if_flag get_iface_flag(const char *flag_str, bool *clear)
#endif
{
	static const struct iface_str_flags flags[] = {
		FLAG(NET_IF_POINTOPOINT),
		FLAG(NET_IF_PROMISC),
		FLAG(NET_IF_NO_AUTO_START),
		FLAG(NET_IF_FORWARD_MULTICASTS),
		FLAG(NET_IF_IPV6_NO_ND),
		FLAG(NET_IF_IPV6_NO_MLD),
		{ 0, NULL }
	};

	if (flag_str == NULL || flag_str[0] == '\0') {
		return NET_IF_NUM_FLAGS;
	}

	if (flag_str[0] == '^') {
		*clear = true;
	} else {
		*clear = false;
	}

	ARRAY_FOR_EACH(flags, i) {
		if (strcmp(flags[i].str, &flag_str[(*clear) ? 1 : 0]) == 0) {
			return flags[i].flag;
		}
	}

	return NET_IF_NUM_FLAGS;
}

static int process_iface_flag(struct net_if *iface, const char *flag_str)
{
	enum net_if_flag flag;
	bool clear;

	flag = get_iface_flag(flag_str, &clear);
	if (flag == NET_IF_NUM_FLAGS) {
		return -ENOENT;
	}

	if (clear) {
		NET_DBG("%s flag %s for interface %d", "Clear",
			flag_str + 1, net_if_get_by_iface(iface));
		net_if_flag_clear(iface, flag);
	} else {
		NET_DBG("%s flag %s for interface %d", "Set",
			flag_str, net_if_get_by_iface(iface));
		net_if_flag_set(iface, flag);
	}

	return 0;
}

static int generated_net_config_init_app(const struct device *dev,
					 const char *app_info)
{
	const struct net_init_config *config;
	int ret, ifindex;

	config = net_config_get_init_config();
	if (config == NULL) {
		NET_ERR("Network configuration not found.");
		return -ENOENT;
	}

	ret = wait_for_interface(config->network_interfaces,
				 sizeof(config->network_interfaces));
	if (ret < 0) {
		NET_WARN("Timeout while waiting network interfaces (%d)", ret);
		return ret;
	}

	ARRAY_FOR_EACH(config->network_interfaces, i) {
		const struct net_init_config_network_interfaces *cfg;
		struct net_if *iface;
		const char *name;

		cfg = &config->network_interfaces[i];

		/* We first need to setup any VLAN interfaces so that other
		 * interfaces can use them (the interface name etc are correctly
		 * set so that referencing works ok).
		 */
		if (IS_ENABLED(CONFIG_NET_VLAN)) {
			vlan_setup(config, cfg);
		}

		iface = get_interface(config, i, dev, &name);
		if (iface == NULL || name == NULL) {
			NET_WARN("No such interface \"%s\" found.",
				 name == NULL ? "<?>" : name);
			continue;
		}

		ifindex = net_if_get_by_iface(iface);

		NET_DBG("Configuring interface %d (%p)", ifindex, iface);

		/* Do we need to change the interface name */
		if (cfg->set_name != NULL) {
			ret = net_if_set_name(iface, cfg->set_name);
			if (ret < 0) {
				NET_DBG("Cannot rename interface %d to \"%s\" (%d)",
					ifindex, cfg->set_name, ret);
				continue;
			}

			NET_DBG("Changed interface %d name to \"%s\"", ifindex,
				cfg->set_name);
		}

		if (cfg->set_default) {
			net_if_set_default(iface);

			NET_DBG("Setting interface %d as default", ifindex);
		}

		ARRAY_FOR_EACH(cfg->flags, j) {
			if (cfg->flags[j].value == NULL ||
			    cfg->flags[j].value[0] == '\0') {
				continue;
			}

			ret = process_iface_flag(iface, cfg->flags[j].value);
			if (ret < 0) {
				NET_DBG("Cannot set/clear flag %s", cfg->flags[j].value);
			}
		}

		ipv6_setup(iface, ifindex, cfg);
		ipv4_setup(iface, ifindex, cfg);
		virtual_iface_setup(iface, ifindex, config, cfg);
	}

	if (IS_ENABLED(CONFIG_NET_CONFIG_CLOCK_SNTP_INIT) && config->sntp.status) {
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
	    config->ieee_802_15_4.status) {
#ifdef CONFIG_NET_L2_IEEE802154_SECURITY
		struct ieee802154_security_params sec_params = { 0 };
		struct ieee802154_security_params *generated_sec_params_ptr = &sec_params;
#else
#define generated_sec_params_ptr NULL
#endif /* CONFIG_NET_L2_IEEE802154_SECURITY */

		struct net_if *iface = NULL;

		if (COND_CODE_1(CONFIG_NET_L2_IEEE802154,
				(config->ieee_802_15_4.bind_to), (0)) > 0) {
			iface = get_interface(
				config,
				COND_CODE_1(CONFIG_NET_L2_IEEE802154,
					    (config->ieee_802_15_4.bind_to - 1), (0)),
				dev,
				NULL);
		}

#ifdef CONFIG_NET_L2_IEEE802154_SECURITY
		memcpy(sec_params.key, config->ieee_802_15_4.security_key,
		       MIN(sizeof(sec_params.key),
			   sizeof(config->ieee_802_15_4.security_key)));
		sec_params.key_len = sizeof(config->ieee_802_15_4.security_key);
		sec_params.key_mode = config->ieee_802_15_4.security_key_mode;
		sec_params.level = config->ieee_802_15_4.security_level;
#endif

		ret = z_net_config_ieee802154_setup(
			    IF_ENABLED(CONFIG_NET_L2_IEEE802154,
				       (iface,
					config->ieee_802_15_4.channel,
					config->ieee_802_15_4.pan_id,
					config->ieee_802_15_4.tx_power,
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
	return generated_net_config_init_app(dev, app_info);
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
#define NET_INIT_CONFIG_ENABLE_DATA
#include "net_init_config.inc"

#if defined(CONFIG_NET_DHCPV4_SERVER)
/* If we are starting DHCPv4 server, then the socket service needs to be started before
 * this config lib as the server will need to use the socket service.
 */
BUILD_ASSERT(CONFIG_NET_SOCKETS_SERVICE_THREAD_PRIO < CONFIG_NET_CONFIG_INIT_PRIO);
#endif

	return &NET_INIT_CONFIG_DATA; /* defined in net_init_config.inc file */
}

#endif /* CONFIG_NET_CONFIG_AUTO_INIT */
