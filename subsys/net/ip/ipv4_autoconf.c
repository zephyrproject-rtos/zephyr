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
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>
#include <zephyr/random/rand32.h>

#include "ipv4_autoconf_internal.h"

/* Have only one timer in order to save memory */
static struct k_work_delayable ipv4auto_timer;

/* Track currently active timers */
static sys_slist_t ipv4auto_ifaces;

#define BUF_ALLOC_TIMEOUT K_MSEC(100)

static struct net_pkt *ipv4_autoconf_prepare_arp(struct net_if *iface)
{
	struct net_if_config *cfg = net_if_get_config(iface);
	struct net_pkt *pkt;

	/* We provide AF_UNSPEC to the allocator: this packet does not
	 * need space for any IPv4 header.
	 */
	pkt = net_pkt_alloc_with_buffer(iface, sizeof(struct net_arp_hdr),
					AF_UNSPEC, 0, BUF_ALLOC_TIMEOUT);
	if (!pkt) {
		return NULL;
	}

	net_pkt_set_family(pkt, AF_INET);
	net_pkt_set_ipv4_auto(pkt, true);

	return net_arp_prepare(pkt, &cfg->ipv4auto.requested_ip,
			       &cfg->ipv4auto.current_ip);
}

static void ipv4_autoconf_send_probe(struct net_if_ipv4_autoconf *ipv4auto)
{
	struct net_pkt *pkt;

	pkt = ipv4_autoconf_prepare_arp(ipv4auto->iface);
	if (!pkt) {
		NET_DBG("Failed to prepare probe %p", ipv4auto->iface);
		return;
	}

	NET_DBG("Probing pkt %p", pkt);

	if (net_if_send_data(ipv4auto->iface, pkt) == NET_DROP) {
		net_pkt_unref(pkt);
	} else {
		ipv4auto->probe_cnt++;
		ipv4auto->state = NET_IPV4_AUTOCONF_PROBE;
	}
}

static void ipv4_autoconf_send_announcement(
	struct net_if_ipv4_autoconf *ipv4auto)
{
	struct net_pkt *pkt;

	pkt = ipv4_autoconf_prepare_arp(ipv4auto->iface);
	if (!pkt) {
		NET_DBG("Failed to prepare announcement %p", ipv4auto->iface);
		return;
	}

	NET_DBG("Announcing pkt %p", pkt);

	if (net_if_send_data(ipv4auto->iface, pkt) == NET_DROP) {
		net_pkt_unref(pkt);
	} else {
		ipv4auto->announce_cnt++;
		ipv4auto->state = NET_IPV4_AUTOCONF_ANNOUNCE;
	}
}

enum net_verdict net_ipv4_autoconf_input(struct net_if *iface,
					 struct net_pkt *pkt)
{
	struct net_if_config *cfg;
	struct net_arp_hdr *arp_hdr;

	cfg = net_if_get_config(iface);
	if (!cfg) {
		NET_DBG("Interface %p configuration missing!", iface);
		return NET_DROP;
	}

	if (net_pkt_get_len(pkt) < sizeof(struct net_arp_hdr)) {
		NET_DBG("Invalid ARP header (len %zu, min %zu bytes)",
			net_pkt_get_len(pkt), sizeof(struct net_arp_hdr));
		return NET_DROP;
	}

	arp_hdr = NET_ARP_HDR(pkt);

	if (!net_ipv4_addr_cmp_raw(arp_hdr->dst_ipaddr,
				   (uint8_t *)&cfg->ipv4auto.requested_ip)) {
		/* No conflict */
		return NET_CONTINUE;
	}

	if (!net_ipv4_addr_cmp_raw(arp_hdr->src_ipaddr,
				   (uint8_t *)&cfg->ipv4auto.requested_ip)) {
		/* No need to defend */
		return NET_CONTINUE;
	}

	NET_DBG("Conflict detected from %s for %s, state %d",
		net_sprint_ll_addr((uint8_t *)&arp_hdr->src_hwaddr,
					      arp_hdr->hwlen),
		net_sprint_ipv4_addr(&arp_hdr->dst_ipaddr),
		cfg->ipv4auto.state);

	cfg->ipv4auto.conflict_cnt++;

	switch (cfg->ipv4auto.state) {
	case NET_IPV4_AUTOCONF_PROBE:
		/* restart probing with renewed IP */
		net_ipv4_autoconf_start(iface);
		break;
	case NET_IPV4_AUTOCONF_ANNOUNCE:
	case NET_IPV4_AUTOCONF_ASSIGNED:
		if (cfg->ipv4auto.conflict_cnt == 1U) {
			/* defend IP */
			ipv4_autoconf_send_announcement(&cfg->ipv4auto);
		} else {
			/* unset host ip */
			if (!net_if_ipv4_addr_rm(iface,
						 &cfg->ipv4auto.requested_ip)) {
				NET_DBG("Failed to remove addr from iface");
			}

			/* restart probing after second conflict */
			net_ipv4_autoconf_start(iface);
		}

		break;
	default:
		break;
	}

	return NET_DROP;
}

static void ipv4_autoconf_send(struct net_if_ipv4_autoconf *ipv4auto)
{
	switch (ipv4auto->state) {
	case NET_IPV4_AUTOCONF_INIT:
		ipv4auto->probe_cnt = 0U;
		ipv4auto->announce_cnt = 0U;
		ipv4auto->conflict_cnt = 0U;
		(void)memset(&ipv4auto->current_ip, 0, sizeof(struct in_addr));
		ipv4auto->requested_ip.s4_addr[0] = 169U;
		ipv4auto->requested_ip.s4_addr[1] = 254U;
		ipv4auto->requested_ip.s4_addr[2] = sys_rand32_get() % 254;
		ipv4auto->requested_ip.s4_addr[3] = sys_rand32_get() % 254;

		NET_DBG("%s: Starting probe for 169.254.%d.%d", "Init",
			ipv4auto->requested_ip.s4_addr[2],
			ipv4auto->requested_ip.s4_addr[3]);
		ipv4_autoconf_send_probe(ipv4auto);
		break;
	case NET_IPV4_AUTOCONF_RENEW:
		ipv4auto->probe_cnt = 0U;
		ipv4auto->announce_cnt = 0U;
		ipv4auto->conflict_cnt = 0U;
		(void)memset(&ipv4auto->current_ip, 0, sizeof(struct in_addr));
		NET_DBG("%s: Starting probe for 169.254.%d.%d", "Renew",
			ipv4auto->requested_ip.s4_addr[2],
			ipv4auto->requested_ip.s4_addr[3]);
		ipv4_autoconf_send_probe(ipv4auto);
		break;
	case NET_IPV4_AUTOCONF_PROBE:
		/* schedule next probe */
		if (ipv4auto->probe_cnt <= (IPV4_AUTOCONF_PROBE_NUM - 1)) {
			ipv4_autoconf_send_probe(ipv4auto);
			break;
		}
		__fallthrough;
	case NET_IPV4_AUTOCONF_ANNOUNCE:
		if (ipv4auto->announce_cnt <=
		    (IPV4_AUTOCONF_ANNOUNCE_NUM - 1)) {
			net_ipaddr_copy(&ipv4auto->current_ip,
					&ipv4auto->requested_ip);
			ipv4_autoconf_send_announcement(ipv4auto);
			break;
		}

		/* success, add new IPv4 address */
		if (!net_if_ipv4_addr_add(ipv4auto->iface,
					  &ipv4auto->requested_ip,
					  NET_ADDR_AUTOCONF, 0)) {
			NET_DBG("Failed to add IPv4 addr to iface %p",
				ipv4auto->iface);
			return;
		}

		ipv4auto->state = NET_IPV4_AUTOCONF_ASSIGNED;
		break;
	default:
		break;
	}
}

static uint32_t ipv4_autoconf_get_timeout(struct net_if_ipv4_autoconf *ipv4auto)
{
	switch (ipv4auto->state) {
	case NET_IPV4_AUTOCONF_PROBE:
		if (ipv4auto->conflict_cnt >= IPV4_AUTOCONF_MAX_CONFLICTS) {
			NET_DBG("Rate limiting");
			return MSEC_PER_SEC * IPV4_AUTOCONF_RATE_LIMIT_INTERVAL;

		} else if (ipv4auto->probe_cnt == IPV4_AUTOCONF_PROBE_NUM) {
			return MSEC_PER_SEC * IPV4_AUTOCONF_ANNOUNCE_INTERVAL;
		}

		return IPV4_AUTOCONF_PROBE_WAIT * MSEC_PER_SEC +
			(sys_rand32_get() % MSEC_PER_SEC);

	case NET_IPV4_AUTOCONF_ANNOUNCE:
		return MSEC_PER_SEC * IPV4_AUTOCONF_ANNOUNCE_INTERVAL;

	default:
		break;
	}

	return 0;
}

static void ipv4_autoconf_submit_work(uint32_t timeout)
{
	k_work_cancel_delayable(&ipv4auto_timer);
	k_work_reschedule(&ipv4auto_timer, K_MSEC(timeout));

	NET_DBG("Next wakeup in %d ms",
		k_ticks_to_ms_ceil32(
			k_work_delayable_remaining_get(&ipv4auto_timer)));
}

static bool ipv4_autoconf_check_timeout(int64_t start, uint32_t time, int64_t timeout)
{
	start += time;
	if (start < 0) {
		start = -start;
	}

	if (start > timeout) {
		return false;
	}

	return true;
}

static bool ipv4_autoconf_timedout(struct net_if_ipv4_autoconf *ipv4auto,
				   int64_t timeout)
{
	return ipv4_autoconf_check_timeout(ipv4auto->timer_start,
					   ipv4auto->timer_timeout,
					   timeout);
}

static uint32_t ipv4_autoconf_manage_timeouts(
	struct net_if_ipv4_autoconf *ipv4auto,
	int64_t timeout)
{
	if (ipv4_autoconf_timedout(ipv4auto, timeout)) {
		ipv4_autoconf_send(ipv4auto);
	}

	ipv4auto->timer_timeout = ipv4_autoconf_get_timeout(ipv4auto);

	return ipv4auto->timer_timeout;
}

static void ipv4_autoconf_timeout(struct k_work *work)
{
	uint32_t timeout_update = UINT32_MAX - 1;
	int64_t timeout = k_uptime_get();
	struct net_if_ipv4_autoconf *current, *next;

	ARG_UNUSED(work);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&ipv4auto_ifaces, current, next,
					  node) {
		uint32_t next_timeout;

		next_timeout = ipv4_autoconf_manage_timeouts(current, timeout);
		if (next_timeout < timeout_update) {
			timeout_update = next_timeout;
		}
	}

	if (timeout_update != UINT32_MAX && timeout_update > 0) {
		NET_DBG("Waiting for %u ms", timeout_update);

		k_work_reschedule(&ipv4auto_timer, K_MSEC(timeout_update));
	}
}

static void ipv4_autoconf_start_timer(struct net_if *iface,
				      struct net_if_ipv4_autoconf *ipv4auto)
{
	sys_slist_append(&ipv4auto_ifaces, &ipv4auto->node);

	ipv4auto->timer_start = k_uptime_get();
	ipv4auto->timer_timeout = MSEC_PER_SEC * IPV4_AUTOCONF_START_DELAY;
	ipv4auto->iface = iface;

	ipv4_autoconf_submit_work(ipv4auto->timer_timeout);
}

void net_ipv4_autoconf_start(struct net_if *iface)
{
	/* Initialize interface and start probing */
	struct net_if_config *cfg;

	cfg = net_if_get_config(iface);
	if (!cfg) {
		return;
	}

	/* Remove the existing registration if found */
	if (cfg->ipv4auto.iface == iface) {
		net_ipv4_autoconf_reset(iface);
	}

	NET_DBG("Starting IPv4 autoconf for iface %p", iface);

	if (cfg->ipv4auto.state == NET_IPV4_AUTOCONF_ASSIGNED) {
		cfg->ipv4auto.state = NET_IPV4_AUTOCONF_RENEW;
	} else {
		cfg->ipv4auto.state = NET_IPV4_AUTOCONF_INIT;
	}

	ipv4_autoconf_start_timer(iface, &cfg->ipv4auto);
}

void net_ipv4_autoconf_reset(struct net_if *iface)
{
	struct net_if_config *cfg;

	cfg = net_if_get_config(iface);
	if (!cfg) {
		return;
	}

	/* Initialize interface and start probing */
	if (cfg->ipv4auto.state == NET_IPV4_AUTOCONF_ASSIGNED) {
		net_if_ipv4_addr_rm(iface, &cfg->ipv4auto.current_ip);
	}

	NET_DBG("Autoconf reset for %p", iface);

	/* Cancel any ongoing probing/announcing attempt*/
	sys_slist_find_and_remove(&ipv4auto_ifaces, &cfg->ipv4auto.node);

	if (sys_slist_is_empty(&ipv4auto_ifaces)) {
		k_work_cancel_delayable(&ipv4auto_timer);
	}
}

void net_ipv4_autoconf_init(void)
{
	k_work_init_delayable(&ipv4auto_timer, ipv4_autoconf_timeout);
}
