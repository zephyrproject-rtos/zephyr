/*
 * Copyright (c) 2017 Matthias Boesl
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief IPv4 address conflict detection
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_ipv4_acd, CONFIG_NET_IPV4_ACD_LOG_LEVEL);

#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_l2.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/slist.h>

#include "ipv4.h"
#include "net_private.h"
#include "../l2/ethernet/arp.h"

static K_MUTEX_DEFINE(lock);

/* Address conflict detection timer. */
static struct k_work_delayable ipv4_acd_timer;

/* List of IPv4 addresses under an active conflict detection. */
static sys_slist_t active_acd_timers;

#define BUF_ALLOC_TIMEOUT K_MSEC(100)

/* Initial random delay*/
#define IPV4_ACD_PROBE_WAIT 1

/* Number of probe packets */
#define IPV4_ACD_PROBE_NUM 3

/* Minimum delay till repeated probe */
#define IPV4_ACD_PROBE_MIN 1

/* Maximum delay till repeated probe */
#define IPV4_ACD_PROBE_MAX 2

/* Delay before announcing */
#define IPV4_ACD_ANNOUNCE_WAIT 2

/* Number of announcement packets */
#define IPV4_ACD_ANNOUNCE_NUM 2

/* Time between announcement packets */
#define IPV4_ACD_ANNOUNCE_INTERVAL 2

/* Max conflicts before rate limiting */
#define IPV4_ACD_MAX_CONFLICTS 10

/* Delay between successive attempts */
#define IPV4_ACD_RATE_LIMIT_INTERVAL 60

/* Minimum interval between defensive ARPs */
#define IPV4_ACD_DEFEND_INTERVAL 10

enum ipv4_acd_state {
	IPV4_ACD_PROBE,    /* Probing state */
	IPV4_ACD_ANNOUNCE, /* Announce state */
};

static struct net_pkt *ipv4_acd_prepare_arp(struct net_if *iface,
					    struct in_addr *sender_ip,
					    struct in_addr *target_ip)
{
	struct net_pkt *pkt, *arp;
	int ret;

	/* We provide AF_UNSPEC to the allocator: this packet does not
	 * need space for any IPv4 header.
	 */
	pkt = net_pkt_alloc_with_buffer(iface, sizeof(struct net_arp_hdr),
					AF_UNSPEC, 0, BUF_ALLOC_TIMEOUT);
	if (!pkt) {
		return NULL;
	}

	net_pkt_set_family(pkt, AF_INET);
	net_pkt_set_ipv4_acd(pkt, true);

	ret = net_arp_prepare(pkt, target_ip, sender_ip, &arp);
	if (ret == NET_ARP_PKT_REPLACED) {
		pkt = arp;
	} else if (ret < 0)  {
		pkt = NULL;
	}
	return pkt;
}

static void ipv4_acd_send_probe(struct net_if_addr *ifaddr)
{
	struct net_if *iface = net_if_get_by_index(ifaddr->ifindex);
	struct in_addr unspecified = { 0 };
	struct net_pkt *pkt;

	pkt = ipv4_acd_prepare_arp(iface, &unspecified, &ifaddr->address.in_addr);
	if (!pkt) {
		NET_DBG("Failed to prepare probe %p", iface);
		return;
	}

	if (net_if_send_data(iface, pkt) == NET_DROP) {
		net_pkt_unref(pkt);
	}
}

static void ipv4_acd_send_announcement(struct net_if_addr *ifaddr)
{
	struct net_if *iface = net_if_get_by_index(ifaddr->ifindex);
	struct net_pkt *pkt;

	pkt = ipv4_acd_prepare_arp(iface, &ifaddr->address.in_addr,
				   &ifaddr->address.in_addr);
	if (!pkt) {
		NET_DBG("Failed to prepare announcement %p", iface);
		return;
	}

	if (net_if_send_data(iface, pkt) == NET_DROP) {
		net_pkt_unref(pkt);
	}
}

static void acd_timer_reschedule(void)
{
	k_timepoint_t expiry = sys_timepoint_calc(K_FOREVER);
	k_timeout_t timeout;
	sys_snode_t *node;

	SYS_SLIST_FOR_EACH_NODE(&active_acd_timers, node) {
		struct net_if_addr *ifaddr =
			CONTAINER_OF(node, struct net_if_addr, acd_node);

		if (sys_timepoint_cmp(ifaddr->acd_timeout, expiry) < 0) {
			expiry = ifaddr->acd_timeout;
		}
	}

	timeout = sys_timepoint_timeout(expiry);
	if (K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		k_work_cancel_delayable(&ipv4_acd_timer);
		return;
	}

	k_work_reschedule(&ipv4_acd_timer, timeout);
}

static void ipv4_acd_manage_timeout(struct net_if_addr *ifaddr)
{
	switch (ifaddr->acd_state) {
	case IPV4_ACD_PROBE:
		if (ifaddr->acd_count < IPV4_ACD_PROBE_NUM) {
			uint32_t delay;

			NET_DBG("Sending probe for %s",
				net_sprint_ipv4_addr(&ifaddr->address.in_addr));

			ipv4_acd_send_probe(ifaddr);

			ifaddr->acd_count++;
			if (ifaddr->acd_count < IPV4_ACD_PROBE_NUM) {
				delay = sys_rand32_get();
				delay %= MSEC_PER_SEC * (IPV4_ACD_PROBE_MAX - IPV4_ACD_PROBE_MIN);
				delay += MSEC_PER_SEC * IPV4_ACD_PROBE_MIN;
			} else {
				delay = MSEC_PER_SEC * IPV4_ACD_ANNOUNCE_WAIT;

			}

			ifaddr->acd_timeout = sys_timepoint_calc(K_MSEC(delay));

			break;
		}

		net_if_ipv4_acd_succeeded(net_if_get_by_index(ifaddr->ifindex),
					  ifaddr);

		ifaddr->acd_state = IPV4_ACD_ANNOUNCE;
		ifaddr->acd_count = 0;
		__fallthrough;
	case IPV4_ACD_ANNOUNCE:
		if (ifaddr->acd_count < IPV4_ACD_ANNOUNCE_NUM) {
			NET_DBG("Sending announcement for %s",
				net_sprint_ipv4_addr(&ifaddr->address.in_addr));

			ipv4_acd_send_announcement(ifaddr);

			ifaddr->acd_count++;
			ifaddr->acd_timeout = sys_timepoint_calc(
					K_SECONDS(IPV4_ACD_ANNOUNCE_INTERVAL));

			break;
		}

		NET_DBG("IPv4 conflict detection done for %s",
			net_sprint_ipv4_addr(&ifaddr->address.in_addr));

		/* Timeout will be used to determine whether DEFEND_INTERVAL
		 * has expired in case of conflicts.
		 */
		ifaddr->acd_timeout = sys_timepoint_calc(K_NO_WAIT);

		sys_slist_find_and_remove(&active_acd_timers, &ifaddr->acd_node);
		break;
	default:
		break;
	}
}

static void ipv4_acd_timeout(struct k_work *work)
{
	sys_snode_t *current, *next;

	ARG_UNUSED(work);

	k_mutex_lock(&lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_NODE_SAFE(&active_acd_timers, current, next) {
		struct net_if_addr *ifaddr =
			CONTAINER_OF(current, struct net_if_addr, acd_node);

		if (sys_timepoint_expired(ifaddr->acd_timeout)) {
			ipv4_acd_manage_timeout(ifaddr);
		}
	}

	acd_timer_reschedule();

	k_mutex_unlock(&lock);
}

static void acd_start_timer(struct net_if *iface, struct net_if_addr *ifaddr)
{
	uint32_t delay;

	sys_slist_find_and_remove(&active_acd_timers, &ifaddr->acd_node);
	sys_slist_append(&active_acd_timers, &ifaddr->acd_node);

	if (iface->config.ip.ipv4->conflict_cnt >= IPV4_ACD_MAX_CONFLICTS) {
		NET_DBG("Rate limiting");
		delay = MSEC_PER_SEC * IPV4_ACD_RATE_LIMIT_INTERVAL;
	} else {
		/* Initial probe should be delayed by a random time interval
		 * between 0 and PROBE_WAIT.
		 */
		delay = sys_rand32_get() % (MSEC_PER_SEC * IPV4_ACD_PROBE_WAIT);
	}

	ifaddr->acd_timeout = sys_timepoint_calc(K_MSEC(delay));

	acd_timer_reschedule();
}

enum net_verdict net_ipv4_acd_input(struct net_if *iface, struct net_pkt *pkt)
{
	sys_snode_t *current, *next;
	struct net_arp_hdr *arp_hdr;
	struct net_if_ipv4 *ipv4;

	if (net_pkt_get_len(pkt) < sizeof(struct net_arp_hdr)) {
		NET_DBG("Invalid ARP header (len %zu, min %zu bytes)",
			net_pkt_get_len(pkt), sizeof(struct net_arp_hdr));
		return NET_DROP;
	}

	arp_hdr = NET_ARP_HDR(pkt);

	k_mutex_lock(&lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_NODE_SAFE(&active_acd_timers, current, next) {
		struct net_if_addr *ifaddr =
			CONTAINER_OF(current, struct net_if_addr, acd_node);
		struct net_if *addr_iface = net_if_get_by_index(ifaddr->ifindex);
		struct net_linkaddr *ll_addr;

		if (iface != addr_iface) {
			continue;
		}

		if (ifaddr->acd_state != IPV4_ACD_PROBE) {
			continue;
		}

		ll_addr = net_if_get_link_addr(addr_iface);

		/* RFC 5227, ch. 2.1.1 Probe Details:
		 * - ARP Request/Reply with Sender IP address match OR,
		 * - ARP Probe where Target IP address match with different sender HW address,
		 * indicate a conflict.
		 * ARP Probe has an all-zero sender IP address
		 */
		if (net_ipv4_addr_cmp_raw(arp_hdr->src_ipaddr,
					  (uint8_t *)&ifaddr->address.in_addr) ||
		    (net_ipv4_addr_cmp_raw(arp_hdr->dst_ipaddr,
					  (uint8_t *)&ifaddr->address.in_addr) &&
				 (memcmp(&arp_hdr->src_hwaddr, ll_addr->addr, ll_addr->len) != 0) &&
				 (net_ipv4_addr_cmp_raw(arp_hdr->src_ipaddr,
						(uint8_t *)&(struct in_addr)INADDR_ANY_INIT)))) {
			NET_DBG("Conflict detected from %s for %s",
				net_sprint_ll_addr((uint8_t *)&arp_hdr->src_hwaddr,
						   arp_hdr->hwlen),
				net_sprint_ipv4_addr(&ifaddr->address.in_addr));

			iface->config.ip.ipv4->conflict_cnt++;

			net_if_ipv4_acd_failed(addr_iface, ifaddr);

			k_mutex_unlock(&lock);

			return NET_DROP;
		}
	}

	k_mutex_unlock(&lock);

	ipv4 = iface->config.ip.ipv4;
	if (ipv4 == NULL) {
		goto out;
	}

	/* Passive conflict detection - try to defend already confirmed
	 * addresses.
	 */
	ARRAY_FOR_EACH(ipv4->unicast, i) {
		struct net_if_addr *ifaddr = &ipv4->unicast[i].ipv4;
		struct net_linkaddr *ll_addr = net_if_get_link_addr(iface);

		if (!ifaddr->is_used) {
			continue;
		}

		if (net_ipv4_addr_cmp_raw(arp_hdr->src_ipaddr,
					  (uint8_t *)&ifaddr->address.in_addr) &&
		    memcmp(&arp_hdr->src_hwaddr, ll_addr->addr, ll_addr->len) != 0) {
			NET_DBG("Conflict detected from %s for %s",
				net_sprint_ll_addr((uint8_t *)&arp_hdr->src_hwaddr,
						   arp_hdr->hwlen),
				net_sprint_ipv4_addr(&ifaddr->address.in_addr));

			ipv4->conflict_cnt++;

			/* In case timer has expired, we're past DEFEND_INTERVAL
			 * and can try to defend again
			 */
			if (sys_timepoint_expired(ifaddr->acd_timeout)) {
				NET_DBG("Defending address %s",
					net_sprint_ipv4_addr(&ifaddr->address.in_addr));
				ipv4_acd_send_announcement(ifaddr);
				ifaddr->acd_timeout = sys_timepoint_calc(
					K_SECONDS(IPV4_ACD_DEFEND_INTERVAL));
			} else {
				NET_DBG("Reporting conflict on %s",
					net_sprint_ipv4_addr(&ifaddr->address.in_addr));
				/* Otherwise report the conflict and let the
				 * application decide.
				 */
				net_mgmt_event_notify_with_info(
					NET_EVENT_IPV4_ACD_CONFLICT, iface,
					&ifaddr->address.in_addr,
					sizeof(struct in_addr));
			}

			break;
		}
	}

out:
	return NET_CONTINUE;
}

void net_ipv4_acd_init(void)
{
	k_work_init_delayable(&ipv4_acd_timer, ipv4_acd_timeout);
}

int net_ipv4_acd_start(struct net_if *iface, struct net_if_addr *ifaddr)
{
	/* Address conflict detection is based on ARP, so can only be done on
	 * supporting interfaces.
	 */
	if (!(net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET) ||
	      net_eth_is_vlan_interface(iface))) {
		net_if_ipv4_acd_succeeded(iface, ifaddr);
		return 0;
	}

	k_mutex_lock(&lock, K_FOREVER);

	ifaddr->ifindex = net_if_get_by_iface(iface);
	ifaddr->acd_state = IPV4_ACD_PROBE;
	ifaddr->acd_count = 0;

	acd_start_timer(iface, ifaddr);

	k_mutex_unlock(&lock);

	return 0;
}

void net_ipv4_acd_cancel(struct net_if *iface, struct net_if_addr *ifaddr)
{
	/* Address conflict detection is based on ARP, so can only be done on
	 * supporting interfaces.
	 */
	if (!(net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET) ||
	      net_eth_is_vlan_interface(iface))) {
		return;
	}

	k_mutex_lock(&lock, K_FOREVER);

	sys_slist_find_and_remove(&active_acd_timers, &ifaddr->acd_node);
	acd_timer_reschedule();

	k_mutex_unlock(&lock);
}
