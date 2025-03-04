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

static K_MUTEX_DEFINE(lock);

static struct net_mgmt_event_callback mgmt4_acd_cb;

/* Must be called with lock held */
static inline void ipv4_autoconf_addr_set(struct net_if_ipv4_autoconf *ipv4auto)
{
	struct in_addr netmask = { { { 255, 255, 0, 0 } } };

	if (ipv4auto->state == NET_IPV4_AUTOCONF_INIT) {
		/* RFC3927 2.1 allowed address range: 169.254.1.0 - 169.254.254.255 */
		ipv4auto->requested_ip.s4_addr[0] = 169U;
		ipv4auto->requested_ip.s4_addr[1] = 254U;
		ipv4auto->requested_ip.s4_addr[2] = 1 + (sys_rand8_get() % 254);
		ipv4auto->requested_ip.s4_addr[3] = sys_rand8_get();
	}

	NET_DBG("%s: Starting probe for 169.254.%d.%d",
		ipv4auto->state == NET_IPV4_AUTOCONF_INIT ? "Init" : "Renew",
		ipv4auto->requested_ip.s4_addr[2],
		ipv4auto->requested_ip.s4_addr[3]);

	ipv4auto->state = NET_IPV4_AUTOCONF_ALLOCATING;

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
}

static void acd_event_handler(struct net_mgmt_event_callback *cb,
			      uint32_t mgmt_event, struct net_if *iface)
{
	struct net_if_config *cfg;
	struct in_addr *addr;

	if (mgmt_event != NET_EVENT_IPV4_ACD_SUCCEED &&
	    mgmt_event != NET_EVENT_IPV4_ACD_FAILED &&
	    mgmt_event != NET_EVENT_IPV4_ACD_CONFLICT) {
		return;
	}

	if (cb->info_length != sizeof(struct in_addr)) {
		return;
	}

	addr = (struct in_addr *)cb->info;

	cfg = net_if_get_config(iface);
	if (!cfg) {
		return;
	}

	(void) k_mutex_lock(&lock, K_FOREVER);

	if (cfg->ipv4auto.iface == NULL) {
		goto out;
	}

	if (!net_ipv4_addr_cmp(&cfg->ipv4auto.requested_ip, addr)) {
		goto out;
	}

	switch (mgmt_event) {
	case NET_EVENT_IPV4_ACD_SUCCEED:
		cfg->ipv4auto.state = NET_IPV4_AUTOCONF_ASSIGNED;
		break;
	case NET_EVENT_IPV4_ACD_CONFLICT:
	case NET_EVENT_IPV4_ACD_FAILED:
		/* Try new address. */
		cfg->ipv4auto.state = NET_IPV4_AUTOCONF_INIT;
		ipv4_autoconf_addr_set(&cfg->ipv4auto);
		break;
	default:
		break;
	}

out:
	k_mutex_unlock(&lock);
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

	(void) k_mutex_lock(&lock, K_FOREVER);

	/* Remove the existing registration if found */
	if (cfg->ipv4auto.iface == iface) {
		net_if_ipv4_addr_rm(iface, &cfg->ipv4auto.requested_ip);
	}

	NET_DBG("Starting IPv4 autoconf for iface %p", iface);

	cfg->ipv4auto.iface = iface;

	switch (cfg->ipv4auto.state) {
	case NET_IPV4_AUTOCONF_INIT:
		break;
	case NET_IPV4_AUTOCONF_RENEW:
	case NET_IPV4_AUTOCONF_ALLOCATING:
	case NET_IPV4_AUTOCONF_ASSIGNED:
		/* Reuse address */
		cfg->ipv4auto.state = NET_IPV4_AUTOCONF_RENEW;
		break;
	default:
		/* Unknown state */
		cfg->ipv4auto.state = NET_IPV4_AUTOCONF_INIT;
		break;
	}

	ipv4_autoconf_addr_set(&cfg->ipv4auto);

	k_mutex_unlock(&lock);
}

void net_ipv4_autoconf_reset(struct net_if *iface)
{
	struct net_if_config *cfg;

	cfg = net_if_get_config(iface);
	if (!cfg) {
		return;
	}

	(void) k_mutex_lock(&lock, K_FOREVER);

	switch (cfg->ipv4auto.state) {
	case NET_IPV4_AUTOCONF_INIT:
		break;
	case NET_IPV4_AUTOCONF_RENEW:
	case NET_IPV4_AUTOCONF_ALLOCATING:
	case NET_IPV4_AUTOCONF_ASSIGNED:
		/* Reuse address on next start */
		cfg->ipv4auto.state = NET_IPV4_AUTOCONF_RENEW;
		break;
	default:
		/* Unknown state */
		cfg->ipv4auto.state = NET_IPV4_AUTOCONF_INIT;
		break;
	}

	net_if_ipv4_addr_rm(iface, &cfg->ipv4auto.requested_ip);
	cfg->ipv4auto.iface = NULL;

	NET_DBG("Autoconf reset for %p", iface);

	k_mutex_unlock(&lock);
}

void net_ipv4_autoconf_init(void)
{
	net_mgmt_init_event_callback(&mgmt4_acd_cb, acd_event_handler,
				     NET_EVENT_IPV4_ACD_SUCCEED |
				     NET_EVENT_IPV4_ACD_FAILED |
				     NET_EVENT_IPV4_ACD_CONFLICT);
	net_mgmt_add_event_callback(&mgmt4_acd_cb);
}
