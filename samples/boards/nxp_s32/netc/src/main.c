/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nxp_s32_netc_sample, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>

#define PSI_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(nxp_s32_netc_psi)
#define VSI_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(nxp_s32_netc_vsi)

static K_SEM_DEFINE(iface_up, 0, 1);

static int setup_iface(struct net_if *iface, const char *ipv6_addr,
		       const char *ipv4_addr, const char *netmask)
{
	struct net_if_addr *ifaddr;
	struct in_addr addr4;
	struct in6_addr addr6;

	LOG_INF("Configuring iface %d (%p)", net_if_get_by_iface(iface), iface);

	if (IS_ENABLED(CONFIG_NET_IPV6) && net_if_flag_is_set(iface, NET_IF_IPV6)) {
		if (net_addr_pton(AF_INET6, ipv6_addr, &addr6)) {
			LOG_ERR("Invalid address: %s", ipv6_addr);
			return -EINVAL;
		}

		ifaddr = net_if_ipv6_addr_add(iface, &addr6, NET_ADDR_MANUAL, 0);
		if (!ifaddr) {
			LOG_ERR("Cannot add %s to interface %p", ipv6_addr, iface);
			return -EINVAL;
		}
		LOG_INF("IPv6 address: %s", ipv6_addr);
	}

	if (IS_ENABLED(CONFIG_NET_IPV4) && net_if_flag_is_set(iface, NET_IF_IPV4)) {
		if (net_addr_pton(AF_INET, ipv4_addr, &addr4)) {
			LOG_ERR("Invalid address: %s", ipv4_addr);
			return -EINVAL;
		}

		ifaddr = net_if_ipv4_addr_add(iface, &addr4, NET_ADDR_MANUAL, 0);
		if (!ifaddr) {
			LOG_ERR("Cannot add %s to interface %p", ipv4_addr, iface);
			return -EINVAL;
		}
		LOG_INF("IPv4 address: %s", ipv4_addr);

		if (netmask && netmask[0]) {
			if (net_addr_pton(AF_INET, netmask, &addr4)) {
				LOG_ERR("Invalid netmask: %s", netmask);
				return -EINVAL;
			}

			net_if_ipv4_set_netmask(iface, &addr4);
		}
	}

	return 0;
}

static void iface_up_handler(struct net_mgmt_event_callback *cb,
			     uint32_t mgmt_event, struct net_if *iface)
{
	if (mgmt_event == NET_EVENT_IF_UP) {
		k_sem_give(&iface_up);
	}
}

static void wait_for_iface(struct net_if *iface)
{
	struct net_mgmt_event_callback iface_up_cb;

	if (net_if_is_up(iface)) {
		return;
	}

	net_mgmt_init_event_callback(&iface_up_cb, iface_up_handler, NET_EVENT_IF_UP);
	net_mgmt_add_event_callback(&iface_up_cb);

	LOG_INF("Waiting for iface %d to come up", net_if_get_by_iface(iface));
	k_sem_take(&iface_up, K_FOREVER);

	net_mgmt_del_event_callback(&iface_up_cb);
}

int main(void)
{
	struct net_if *iface;

	LOG_INF("Starting sample");

	iface = net_if_lookup_by_dev(DEVICE_DT_GET(PSI_NODE));
	if (iface) {
		/* Wait for PSI iface to come up before attempting configuration */
		wait_for_iface(iface);
		setup_iface(iface,
			    CONFIG_NET_CONFIG_MY_IPV6_ADDR,
			    CONFIG_NET_CONFIG_MY_IPV4_ADDR,
			    CONFIG_NET_CONFIG_MY_IPV4_NETMASK);
	}

#if DT_HAS_COMPAT_STATUS_OKAY(nxp_s32_netc_vsi)
	iface = net_if_lookup_by_dev(DEVICE_DT_GET(VSI_NODE));
	if (iface) {
		setup_iface(iface,
			    CONFIG_NET_SAMPLE_IFACE2_MY_IPV6_ADDR,
			    CONFIG_NET_SAMPLE_IFACE2_MY_IPV4_ADDR,
			    CONFIG_NET_SAMPLE_IFACE2_MY_IPV4_NETMASK);
	}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(nxp_s32_netc_vsi)*/
	return 0;
}
