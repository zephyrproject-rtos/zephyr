/* Networking */

/*
 * SPDX-FileCopyrightText: © 2025-2026 Tenstorrent USA, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bmc_net, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/linker/sections.h>
#include <errno.h>
#include <stdio.h>

#include <zephyr/net/net_config.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/dns_resolve.h>

#include "net.h"
#include "dhcp.h"
#include "config.h"
#include "ntp.h"

int net_do_set_hostname(const char *hostname)
{
	int rc;

	rc = net_hostname_set(hostname, strlen(hostname));
	if (rc) {
		return rc;
	}

	if (config_bmc_use_dhcp4()) {
		rc = restart_dhcp4();
		if (rc) {
			return rc;
		}
	}

	return rc;
}

static int net_do_set_default_ip4(uint32_t ip4_addr, uint32_t ip4_netmask, uint32_t ip4_gateway)
{
	struct net_if *iface = net_if_get_default();
	struct in_addr addr;
	struct in_addr netmask;
	struct in_addr gateway;
	struct net_if_addr *if_addr;
	struct net_if_ipv4 *ipv4;

	if (!iface) {
		LOG_ERR("No default interface to set IPv4 address");
		return -ENOENT;
	}

	/* Set gateway */
	gateway.s_addr = ip4_gateway;
	net_if_ipv4_set_gw(iface, &gateway);

	/* Remove existing MANUAL/OVERRIDABLE */
	ipv4 = iface->config.ip.ipv4;
	if (ipv4) {
		for (int i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
			if (!ipv4->unicast[i].ipv4.is_used)
				continue;

			if (ipv4->unicast[i].ipv4.addr_type == NET_ADDR_MANUAL ||
			    ipv4->unicast[i].ipv4.addr_type == NET_ADDR_OVERRIDABLE) {
				/* XXX: Check return value? */
				net_if_ipv4_addr_rm(iface, &ipv4->unicast[i].ipv4.address.in_addr);
			}
		}
	}

	/* Just remove any manual address if this was 0 */
	if (!ip4_addr)
		return 0;

	addr.s_addr = ip4_addr;
	if_addr = net_if_ipv4_addr_add(iface, &addr, NET_ADDR_OVERRIDABLE, 0);
	if (!if_addr) {
		LOG_ERR("Failed to add IPv4 address");
		return -EINVAL;
	}

	netmask.s_addr = ip4_netmask;
	if (!net_if_ipv4_set_netmask_by_addr(iface, &addr, &netmask)) {
		LOG_ERR("Failed to set IPv4 netmask address");
		return -EINVAL;
	}

	return 0;
}

int net_do_set_default_ip4_from_config(void)
{
	uint32_t ip4_addr, ip4_nm, ip4_gw;
	int rc;

	ip4_addr = config_bmc_default_ip4();
	ip4_nm = config_bmc_default_ip4_nm();
	ip4_gw = config_bmc_default_ip4_gw();
	rc = net_do_set_default_ip4(ip4_addr, ip4_nm, ip4_gw);
	if (rc)
		LOG_ERR("Cannot set default IPv4 address (err=%d)", rc);

	return rc;
}

int net_start_dhcp4(void)
{
	return start_dhcp4();
}

int net_stop_dhcp4(void)
{
	int rc;

	rc = stop_dhcp4();
	if (rc)
		return rc;

	rc = net_do_set_default_ip4_from_config();
	if (rc)
		LOG_ERR("Cannot reset IPv4 address (err=%d)", rc);

#ifdef CONFIG_BMC_APP_DNS_RESOLVE
	static const char *dns_servers[] = { CONFIG_DNS_SERVER1, NULL };
	rc = dns_resolve_reconfigure(dns_resolve_get_default(), dns_servers,
					NULL, DNS_SOURCE_MANUAL);
	if (rc)
		LOG_ERR("Cannot reset DNS resolver (err=%d)", rc);
#endif

	return 0;
}

int net_init(void)
{
	int rc;
	uint32_t net_config_flags = 0;
	const char *hostname = config_bmc_hostname();

	rc = net_hostname_set(hostname, strlen(hostname));
	if (rc)
		LOG_ERR("Net hostname set failed");

	if (config_bmc_default_ip4()) {
		LOG_INF("Network static IP set");
		rc = net_do_set_default_ip4_from_config();
		if (rc)
			LOG_ERR("Static IPv4 init failed");
	}

	rc = dhcp4_init();
	if (rc) {
		LOG_ERR("DHCPv4 init failed");
		return rc;
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		net_config_flags |= NET_CONFIG_NEED_IPV4;
	}

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		net_config_flags |= NET_CONFIG_NEED_IPV6;
	}

	rc = net_config_init("Initializing network", net_config_flags,
			     CONFIG_NET_CONFIG_INIT_TIMEOUT * MSEC_PER_SEC);
	if (rc) {
		LOG_ERR("Network init failed");
		return rc;
	}

	/*
	 * Net init always starts dhcp if it is in the Kconfig.
	 * Stop it if our config does not want it. This is
	 * somewhat hacky.
	 */
	if (!config_bmc_use_dhcp4()) {
		if (net_stop_dhcp4())
			LOG_ERR("DHCPv4 stop failed, continuing");
	}

	LOG_INF("Network hostname: %s", net_hostname_get());

	LOG_DBG("NTP init");
	rc = ntp_init();
	if (rc)
		LOG_ERR("NTP init failed");

	return 0;
}
