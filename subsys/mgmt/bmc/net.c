/*
 * SPDX-FileCopyrightText: Copyright (c) 2025-2026 Tenstorrent USA, Inc.
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/bmc.h>
#include <zephyr/mgmt/bmc/config.h>
#include <zephyr/net/dns_resolve.h>
#include <zephyr/net/hostname.h>
#include <zephyr/net/net_config.h>
#include <zephyr/net/net_if.h>

#include "bmc_internal.h"

LOG_MODULE_DECLARE(bmc, CONFIG_BMC_LOG_LEVEL);

int bmc_net_set_hostname(const char *hostname)
{
	int ret;

	ret = net_hostname_set(hostname, strlen(hostname));
	if (ret < 0) {
		return ret;
	}

	if (bmc_config_use_dhcp4()) {
		return bmc_dhcp4_restart();
	}

	return 0;
}

static int net_set_static_ip4(uint32_t ip4_addr, uint32_t ip4_netmask, uint32_t ip4_gateway)
{
	struct net_if *iface = net_if_get_default();
	struct net_if_ipv4 *ipv4;
	struct net_if_addr *if_addr;
	struct in_addr addr;
	struct in_addr netmask;
	struct in_addr gateway;

	if (iface == NULL) {
		LOG_ERR("No default interface to set the IPv4 address on");
		return -ENOENT;
	}

	gateway.s_addr = ip4_gateway;
	net_if_ipv4_set_gw(iface, &gateway);

	/* Drop any address we configured previously. */
	ipv4 = iface->config.ip.ipv4;
	if (ipv4 != NULL) {
		for (int i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
			if (!ipv4->unicast[i].ipv4.is_used) {
				continue;
			}

			if (ipv4->unicast[i].ipv4.addr_type == NET_ADDR_MANUAL ||
			    ipv4->unicast[i].ipv4.addr_type == NET_ADDR_OVERRIDABLE) {
				(void)net_if_ipv4_addr_rm(
					iface, &ipv4->unicast[i].ipv4.address.in_addr);
			}
		}
	}

	if (ip4_addr == 0) {
		/* Address cleared, nothing left to add. */
		return 0;
	}

	addr.s_addr = ip4_addr;
	if_addr = net_if_ipv4_addr_add(iface, &addr, NET_ADDR_OVERRIDABLE, 0);
	if (if_addr == NULL) {
		LOG_ERR("Failed to add the IPv4 address");
		return -EINVAL;
	}

	netmask.s_addr = ip4_netmask;
	if (!net_if_ipv4_set_netmask_by_addr(iface, &addr, &netmask)) {
		LOG_ERR("Failed to set the IPv4 netmask");
		return -EINVAL;
	}

	return 0;
}

int bmc_net_apply_static_ip4(void)
{
	return net_set_static_ip4(bmc_config_static_ip4(), bmc_config_static_ip4_netmask(),
				  bmc_config_static_ip4_gateway());
}

int bmc_net_start_dhcp4(void)
{
	return bmc_dhcp4_start();
}

int bmc_net_stop_dhcp4(void)
{
	int ret;

	ret = bmc_dhcp4_stop();
	if (ret < 0) {
		return ret;
	}

	ret = bmc_net_apply_static_ip4();
	if (ret < 0) {
		LOG_ERR("Could not restore the static IPv4 address (err=%d)", ret);
	}

#if defined(CONFIG_BMC_DNS_RESOLVE)
	static const char *const dns_servers[] = {CONFIG_DNS_SERVER1, NULL};

	ret = dns_resolve_reconfigure(dns_resolve_get_default(), dns_servers, NULL,
				      DNS_SOURCE_MANUAL);
	if (ret < 0) {
		LOG_ERR("Could not reconfigure the DNS resolver (err=%d)", ret);
	}
#endif

	return 0;
}

static int bmc_net_init(void)
{
	uint32_t net_config_flags = 0;
	int ret;

	ret = bmc_net_set_hostname(bmc_config_hostname());
	if (ret < 0) {
		LOG_ERR("Could not set the network hostname (err=%d)", ret);
	}

	if (bmc_config_static_ip4() != 0) {
		ret = bmc_net_apply_static_ip4();
		if (ret < 0) {
			LOG_ERR("Static IPv4 configuration failed (err=%d)", ret);
		}
	}

	ret = bmc_dhcp4_init();
	if (ret < 0) {
		LOG_ERR("DHCPv4 init failed (err=%d)", ret);
		return ret;
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		net_config_flags |= NET_CONFIG_NEED_IPV4;
	}

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		net_config_flags |= NET_CONFIG_NEED_IPV6;
	}

	ret = net_config_init("Initializing network", net_config_flags,
			      CONFIG_NET_CONFIG_INIT_TIMEOUT * MSEC_PER_SEC);
	if (ret < 0) {
		LOG_ERR("Network init failed (err=%d)", ret);
		return ret;
	}

	/*
	 * net_config_init() unconditionally starts DHCPv4 when it is built in,
	 * so stop it again if the stored configuration asks for a static
	 * address.
	 */
	if (!bmc_config_use_dhcp4()) {
		ret = bmc_net_stop_dhcp4();
		if (ret < 0) {
			LOG_ERR("Could not stop DHCPv4 (err=%d), continuing", ret);
		}
	}

	LOG_INF("Network hostname: %s", net_hostname_get());

	bmc_event_notify(BMC_EVENT_NET_READY, NULL, 0);

	return 0;
}

BMC_COMPONENT_DEFINE(bmc_net, BMC_INIT_PHASE_NETWORK, bmc_net_init, false);
