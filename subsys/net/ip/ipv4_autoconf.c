/** @file
 * @brief IPv4 autoconf related functions
 */

/*
 * Copyright (c) 2017 Matthias Boesl
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_ipv4_autoconf, CONFIG_NET_IPV4_AUTO_LOG_LEVEL);

#include "net_private.h"
#include <errno.h>
#include "../l2/ethernet/arp.h"
#include <zephyr/net/ipv4_autoconf.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>
#include <zephyr/random/random.h>

static struct net_mgmt_event_callback mgmt4_acd_cb;

static inline void ipv4_autoconf_addr_set(struct net_if_ipv4_autoconf *ipv4auto)
{
	struct in_addr netmask = { { { 255, 255, 0, 0 } } };

	if (ipv4auto->state == NET_IPV4_AUTOCONF_INIT) {
		ipv4auto->requested_ip.s4_addr[0] = 169U;
		ipv4auto->requested_ip.s4_addr[1] = 254U;
		ipv4auto->requested_ip.s4_addr[2] = sys_rand8_get() % 254;
		ipv4auto->requested_ip.s4_addr[3] = sys_rand8_get() % 254;
	}

	NET_DBG("%s: Starting probe for 169.254.%d.%d",
		ipv4auto->state == NET_IPV4_AUTOCONF_INIT ? "Init" : "Renew",
		ipv4auto->requested_ip.s4_addr[2],
		ipv4auto->requested_ip.s4_addr[3]);

	/* Add IPv4 address to the interface, this will trigger conflict detection. */
	if (!net_if_ipv4_addr_add(ipv4auto->iface, &ipv4auto->requested_ip,
				  NET_ADDR_AUTOCONF, 0)) {
		NET_DBG("Failed to add IPv4 addr to iface %p",
			ipv4auto->iface);
		return;
	}

	net_if_ipv4_set_netmask_by_addr(ipv4auto->iface,
					&ipv4auto->requested_ip,
					&netmask);

	ipv4auto->state = NET_IPV4_AUTOCONF_ASSIGNED;
}

static void acd_event_handler(struct net_mgmt_event_callback *cb,
			      uint32_t mgmt_event, struct net_if *iface)
{
	struct net_if_config *cfg;
	struct in_addr *addr;

	cfg = net_if_get_config(iface);
	if (!cfg) {
		return;
	}

	if (cfg->ipv4auto.iface == NULL) {
		return;
	}

	if (mgmt_event != NET_EVENT_IPV4_ACD_SUCCEED &&
	    mgmt_event != NET_EVENT_IPV4_ACD_FAILED &&
	    mgmt_event != NET_EVENT_IPV4_ACD_CONFLICT) {
		return;
	}

	if (cb->info_length != sizeof(struct in_addr)) {
		return;
	}

	addr = (struct in_addr *)cb->info;

	if (!net_ipv4_addr_cmp(&cfg->ipv4auto.requested_ip, addr)) {
		return;
	}

	switch (mgmt_event) {
	case NET_EVENT_IPV4_ACD_SUCCEED:
		cfg->ipv4auto.state = NET_IPV4_AUTOCONF_ASSIGNED;
		break;
	case NET_EVENT_IPV4_ACD_CONFLICT:
		net_ipv4_autoconf_reset(iface);
		__fallthrough;
	case NET_EVENT_IPV4_ACD_FAILED:
		/* Try new address. */
		cfg->ipv4auto.state = NET_IPV4_AUTOCONF_INIT;
		ipv4_autoconf_addr_set(&cfg->ipv4auto);
		break;
	default:
		break;
	}
}

void net_ipv4_autoconf_start(struct net_if *iface)
{
	/* Initialize interface and start probing */
	struct net_if_config *cfg;

	if (!net_if_flag_is_set(iface, NET_IF_IPV4)) {
		return;
	}

	cfg = net_if_get_config(iface);
	if (!cfg) {
		return;
	}

	/* Remove the existing registration if found */
	if (cfg->ipv4auto.iface == iface) {
		net_ipv4_autoconf_reset(iface);
	}

	cfg->ipv4auto.iface = iface;

	NET_DBG("Starting IPv4 autoconf for iface %p", iface);

	if (cfg->ipv4auto.state == NET_IPV4_AUTOCONF_ASSIGNED) {
		/* Try to reuse previously used address. */
		cfg->ipv4auto.state = NET_IPV4_AUTOCONF_RENEW;
	} else {
		cfg->ipv4auto.state = NET_IPV4_AUTOCONF_INIT;
	}

	ipv4_autoconf_addr_set(&cfg->ipv4auto);
}

void net_ipv4_autoconf_reset(struct net_if *iface)
{
	struct net_if_config *cfg;

	cfg = net_if_get_config(iface);
	if (!cfg) {
		return;
	}

	net_if_ipv4_addr_rm(iface, &cfg->ipv4auto.requested_ip);

	NET_DBG("Autoconf reset for %p", iface);
}

void net_ipv4_autoconf_init(void)
{
	net_mgmt_init_event_callback(&mgmt4_acd_cb, acd_event_handler,
				     NET_EVENT_IPV4_ACD_SUCCEED |
				     NET_EVENT_IPV4_ACD_FAILED |
				     NET_EVENT_IPV4_ACD_CONFLICT);
	net_mgmt_add_event_callback(&mgmt4_acd_cb);
}
