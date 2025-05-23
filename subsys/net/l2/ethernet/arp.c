/** @file
 * @brief ARP related functions
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_arp, CONFIG_NET_ARP_LOG_LEVEL);

#include <errno.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_stats.h>
#include <zephyr/net/net_mgmt.h>

#include "arp.h"
#include "ipv4.h"
#include "net_private.h"

#define NET_BUF_TIMEOUT K_MSEC(100)
#define ARP_REQUEST_TIMEOUT (2 * MSEC_PER_SEC)

static bool arp_cache_initialized;
static struct arp_entry arp_entries[CONFIG_NET_ARP_TABLE_SIZE];

static sys_slist_t arp_free_entries;
static sys_slist_t arp_pending_entries;
static sys_slist_t arp_table;

static struct k_work_delayable arp_request_timer;

static struct k_mutex arp_mutex;

#if defined(CONFIG_NET_ARP_GRATUITOUS_TRANSMISSION)
static struct net_mgmt_event_callback iface_event_cb;
static struct net_mgmt_event_callback ipv4_event_cb;
static struct k_work_delayable arp_gratuitous_work;
#endif /* defined(CONFIG_NET_ARP_GRATUITOUS_TRANSMISSION) */

static void arp_entry_cleanup(struct arp_entry *entry, bool pending)
{
	NET_DBG("entry %p", entry);

	if (pending) {
		struct net_pkt *pkt;

		while (!k_fifo_is_empty(&entry->pending_queue)) {
			pkt = k_fifo_get(&entry->pending_queue, K_FOREVER);
			NET_DBG("Releasing pending pkt %p (ref %ld)",
				pkt,
				atomic_get(&pkt->atomic_ref) - 1);
			net_pkt_unref(pkt);
		}
	}

	entry->iface = NULL;

	(void)memset(&entry->ip, 0, sizeof(struct in_addr));
	(void)memset(&entry->eth, 0, sizeof(struct net_eth_addr));
}

static struct arp_entry *arp_entry_find(sys_slist_t *list,
					struct net_if *iface,
					struct in_addr *dst,
					sys_snode_t **previous)
{
	struct arp_entry *entry;

	SYS_SLIST_FOR_EACH_CONTAINER(list, entry, node) {
		NET_DBG("iface %d (%p) dst %s",
			net_if_get_by_iface(iface), iface,
			net_sprint_ipv4_addr(&entry->ip));

		if (entry->iface == iface &&
		    net_ipv4_addr_cmp(&entry->ip, dst)) {
			NET_DBG("found dst %s",
				net_sprint_ipv4_addr(dst));

			return entry;
		}

		if (previous) {
			*previous = &entry->node;
		}
	}

	return NULL;
}

static inline struct arp_entry *arp_entry_find_move_first(struct net_if *iface,
							  struct in_addr *dst)
{
	sys_snode_t *prev = NULL;
	struct arp_entry *entry;

	NET_DBG("dst %s", net_sprint_ipv4_addr(dst));

	entry = arp_entry_find(&arp_table, iface, dst, &prev);
	if (entry) {
		/* Let's assume the target is going to be accessed
		 * more than once here in a short time frame. So we
		 * place the entry first in position into the table
		 * in order to reduce subsequent find.
		 */
		if (&entry->node != sys_slist_peek_head(&arp_table)) {
			sys_slist_remove(&arp_table, prev, &entry->node);
			sys_slist_prepend(&arp_table, &entry->node);
		}
	}

	return entry;
}

static inline
struct arp_entry *arp_entry_find_pending(struct net_if *iface,
					 struct in_addr *dst)
{
	NET_DBG("dst %s", net_sprint_ipv4_addr(dst));

	return arp_entry_find(&arp_pending_entries, iface, dst, NULL);
}

static struct arp_entry *arp_entry_get_pending(struct net_if *iface,
					       struct in_addr *dst)
{
	sys_snode_t *prev = NULL;
	struct arp_entry *entry;

	NET_DBG("dst %s", net_sprint_ipv4_addr(dst));

	entry = arp_entry_find(&arp_pending_entries, iface, dst, &prev);
	if (entry) {
		/* We remove the entry from the pending list */
		sys_slist_remove(&arp_pending_entries, prev, &entry->node);
	}

	if (sys_slist_is_empty(&arp_pending_entries)) {
		k_work_cancel_delayable(&arp_request_timer);
	}

	return entry;
}

static struct arp_entry *arp_entry_get_free(void)
{
	sys_snode_t *node;

	node = sys_slist_peek_head(&arp_free_entries);
	if (!node) {
		return NULL;
	}

	/* We remove the node from the free list */
	sys_slist_remove(&arp_free_entries, NULL, node);

	return CONTAINER_OF(node, struct arp_entry, node);
}

static struct arp_entry *arp_entry_get_last_from_table(void)
{
	sys_snode_t *node;

	/* We assume last entry is the oldest one,
	 * so is the preferred one to be taken out.
	 */

	node = sys_slist_peek_tail(&arp_table);
	if (!node) {
		return NULL;
	}

	sys_slist_find_and_remove(&arp_table, node);

	return CONTAINER_OF(node, struct arp_entry, node);
}


static void arp_entry_register_pending(struct arp_entry *entry)
{
	NET_DBG("dst %s", net_sprint_ipv4_addr(&entry->ip));

	sys_slist_append(&arp_pending_entries, &entry->node);

	entry->req_start = k_uptime_get_32();

	/* Let's start the timer if necessary */
	if (!k_work_delayable_remaining_get(&arp_request_timer)) {
		k_work_reschedule(&arp_request_timer,
				  K_MSEC(ARP_REQUEST_TIMEOUT));
	}
}

static void arp_request_timeout(struct k_work *work)
{
	uint32_t current = k_uptime_get_32();
	struct arp_entry *entry, *next;

	ARG_UNUSED(work);

	k_mutex_lock(&arp_mutex, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&arp_pending_entries,
					  entry, next, node) {
		if ((int32_t)(entry->req_start +
			    ARP_REQUEST_TIMEOUT - current) > 0) {
			break;
		}

		arp_entry_cleanup(entry, true);

		sys_slist_remove(&arp_pending_entries, NULL, &entry->node);
		sys_slist_append(&arp_free_entries, &entry->node);

		entry = NULL;
	}

	if (entry) {
		k_work_reschedule(&arp_request_timer,
				  K_MSEC(entry->req_start +
					 ARP_REQUEST_TIMEOUT - current));
	}

	k_mutex_unlock(&arp_mutex);
}

static inline struct in_addr *if_get_addr(struct net_if *iface,
					  struct in_addr *addr)
{
	struct net_if_ipv4 *ipv4 = iface->config.ip.ipv4;

	if (!ipv4) {
		return NULL;
	}

	ARRAY_FOR_EACH(ipv4->unicast, i) {
		if (ipv4->unicast[i].ipv4.is_used &&
		    ipv4->unicast[i].ipv4.address.family == AF_INET &&
		    ipv4->unicast[i].ipv4.addr_state == NET_ADDR_PREFERRED &&
		    (!addr ||
		     net_ipv4_addr_cmp(addr,
				       &ipv4->unicast[i].ipv4.address.in_addr))) {
			return &ipv4->unicast[i].ipv4.address.in_addr;
		}
	}

	return NULL;
}

static inline struct net_pkt *arp_prepare(struct net_if *iface,
					  struct in_addr *next_addr,
					  struct arp_entry *entry,
					  struct net_pkt *pending,
					  struct in_addr *current_ip)
{
	struct net_arp_hdr *hdr;
	struct in_addr *my_addr;
	struct net_pkt *pkt;

	if (current_ip) {
		/* This is the IPv4 autoconf case where we have already
		 * things setup so no need to allocate new net_pkt
		 */
		pkt = pending;
	} else {
		pkt = net_pkt_alloc_with_buffer(iface,
						sizeof(struct net_arp_hdr),
						AF_UNSPEC, 0, NET_BUF_TIMEOUT);
		if (!pkt) {
			return NULL;
		}

		/* Avoid recursive loop with network packet capturing */
		if (IS_ENABLED(CONFIG_NET_CAPTURE) && pending) {
			net_pkt_set_captured(pkt, net_pkt_is_captured(pending));
		}

		if (IS_ENABLED(CONFIG_NET_VLAN) && pending) {
			net_pkt_set_vlan_tag(pkt, net_pkt_vlan_tag(pending));
		}
	}

	net_pkt_set_ll_proto_type(pkt, NET_ETH_PTYPE_ARP);
	net_pkt_set_family(pkt, AF_INET);

	net_buf_add(pkt->buffer, sizeof(struct net_arp_hdr));

	hdr = NET_ARP_HDR(pkt);

	/* If entry is not set, then we are just about to send
	 * an ARP request using the data in pending net_pkt.
	 * This can happen if there is already a pending ARP
	 * request and we want to send it again.
	 */
	if (entry) {
		if (!net_pkt_ipv4_acd(pkt)) {
			net_pkt_ref(pending);
			k_fifo_put(&entry->pending_queue, pending);
		}

		entry->iface = net_pkt_iface(pkt);

		net_ipaddr_copy(&entry->ip, next_addr);

		(void)net_linkaddr_set(net_pkt_lladdr_src(pkt),
				       net_if_get_link_addr(entry->iface)->addr,
				       sizeof(struct net_eth_addr));

		arp_entry_register_pending(entry);
	} else {
		(void)net_linkaddr_set(net_pkt_lladdr_src(pkt),
				       net_if_get_link_addr(iface)->addr,
				       sizeof(struct net_eth_addr));
	}

	(void)net_linkaddr_set(net_pkt_lladdr_dst(pkt),
			       (const uint8_t *)net_eth_broadcast_addr(),
			       sizeof(struct net_eth_addr));

	hdr->hwtype = htons(NET_ARP_HTYPE_ETH);
	hdr->protocol = htons(NET_ETH_PTYPE_IP);
	hdr->hwlen = sizeof(struct net_eth_addr);
	hdr->protolen = sizeof(struct in_addr);
	hdr->opcode = htons(NET_ARP_REQUEST);

	(void)memset(&hdr->dst_hwaddr.addr, 0x00, sizeof(struct net_eth_addr));

	net_ipv4_addr_copy_raw(hdr->dst_ipaddr, (uint8_t *)next_addr);

	memcpy(hdr->src_hwaddr.addr, net_pkt_lladdr_src(pkt)->addr,
	       sizeof(struct net_eth_addr));

	if (net_pkt_ipv4_acd(pkt)) {
		my_addr = current_ip;
	} else if (!entry) {
		my_addr = (struct in_addr *)NET_IPV4_HDR(pending)->src;
	} else {
		my_addr = if_get_addr(entry->iface, current_ip);
	}

	if (my_addr) {
		net_ipv4_addr_copy_raw(hdr->src_ipaddr, (uint8_t *)my_addr);
	} else {
		(void)memset(&hdr->src_ipaddr, 0, sizeof(struct in_addr));
	}

	NET_DBG("Generating request for %s", net_sprint_ipv4_addr(next_addr));
	return pkt;
}

int net_arp_prepare(struct net_pkt *pkt,
		    struct in_addr *request_ip,
		    struct in_addr *current_ip,
		    struct net_pkt **arp_pkt)
{
	bool is_ipv4_ll_used = false;
	struct arp_entry *entry;
	struct in_addr *addr;

	if (!pkt || !pkt->buffer) {
		return -EINVAL;
	}

	if (net_pkt_ipv4_acd(pkt)) {
		*arp_pkt = arp_prepare(net_pkt_iface(pkt), request_ip, NULL,
				       pkt, current_ip);
		return *arp_pkt ? NET_ARP_PKT_REPLACED : -ENOMEM;
	}

	if (IS_ENABLED(CONFIG_NET_IPV4_AUTO)) {
		is_ipv4_ll_used = net_ipv4_is_ll_addr((struct in_addr *)
						&NET_IPV4_HDR(pkt)->src) ||
				  net_ipv4_is_ll_addr((struct in_addr *)
						&NET_IPV4_HDR(pkt)->dst);
	}

	/* Is the destination in the local network, if not route via
	 * the gateway address.
	 */
	if (!current_ip && !is_ipv4_ll_used &&
	    !net_if_ipv4_addr_mask_cmp(net_pkt_iface(pkt), request_ip)) {
		struct net_if_ipv4 *ipv4 = net_pkt_iface(pkt)->config.ip.ipv4;

		if (ipv4) {
			addr = &ipv4->gw;
			if (net_ipv4_is_addr_unspecified(addr)) {
				NET_ERR("Gateway not set for iface %d, could not "
					"send ARP request for %s",
					net_if_get_by_iface(net_pkt_iface(pkt)),
					net_sprint_ipv4_addr(request_ip));

				return -EINVAL;
			}
		} else {
			addr = request_ip;
		}
	} else {
		addr = request_ip;
	}

	k_mutex_lock(&arp_mutex, K_FOREVER);

	/* If the destination address is already known, we do not need
	 * to send any ARP packet.
	 */
	entry = arp_entry_find_move_first(net_pkt_iface(pkt), addr);
	if (!entry) {
		struct net_pkt *req;

		entry = arp_entry_find_pending(net_pkt_iface(pkt), addr);
		if (!entry) {
			/* No pending, let's try to get a new entry */
			entry = arp_entry_get_free();
			if (!entry) {
				/* Then let's take one from table? */
				entry = arp_entry_get_last_from_table();
			}
		} else {
			/* There is a pending ARP request already, check if this packet is already
			 * in the pending list and if so, resend the request, otherwise just
			 * append the packet to the request fifo list.
			 * Ensure the packet reference is incremented to account for the queue
			 * holding the reference.
			 */
			pkt = net_pkt_ref(pkt);
			if (k_queue_unique_append(&entry->pending_queue._queue, pkt)) {
				NET_DBG("Pending ARP request for %s, queuing pkt %p",
					net_sprint_ipv4_addr(addr), pkt);
				k_mutex_unlock(&arp_mutex);
				return NET_ARP_PKT_QUEUED;
			}

			/* Queueing the packet failed, undo the net_pkt_ref */
			net_pkt_unref(pkt);
			entry = NULL;
		}

		req = arp_prepare(net_pkt_iface(pkt), addr, entry, pkt,
				  current_ip);

		if (!entry) {
			/* We cannot send the packet, the ARP cache is full
			 * or there is already a pending query to this IP
			 * address, so this packet must be discarded.
			 */
			NET_DBG("Resending ARP %p", req);
		}

		if (!req && entry) {
			/* Add the arp entry back to arp_free_entries, to avoid the
			 * arp entry is leak due to ARP packet allocated failed.
			 */
			sys_slist_prepend(&arp_free_entries, &entry->node);
		}

		k_mutex_unlock(&arp_mutex);
		*arp_pkt = req;
		return req ? NET_ARP_PKT_REPLACED : -ENOMEM;
	}

	k_mutex_unlock(&arp_mutex);

	(void)net_linkaddr_set(net_pkt_lladdr_src(pkt),
			       net_if_get_link_addr(entry->iface)->addr,
			       sizeof(struct net_eth_addr));

	(void)net_linkaddr_set(net_pkt_lladdr_dst(pkt),
			       (const uint8_t *)&entry->eth, sizeof(struct net_eth_addr));

	NET_DBG("ARP using ll %s for IP %s",
		net_sprint_ll_addr(net_pkt_lladdr_dst(pkt)->addr,
				   sizeof(struct net_eth_addr)),
		net_sprint_ipv4_addr(NET_IPV4_HDR(pkt)->dst));

	return NET_ARP_COMPLETE;
}

static void arp_gratuitous(struct net_if *iface,
			   struct in_addr *src,
			   struct net_eth_addr *hwaddr)
{
	sys_snode_t *prev = NULL;
	struct arp_entry *entry;

	entry = arp_entry_find(&arp_table, iface, src, &prev);
	if (entry) {
		NET_DBG("Gratuitous ARP hwaddr %s -> %s",
			net_sprint_ll_addr((const uint8_t *)&entry->eth,
					   sizeof(struct net_eth_addr)),
			net_sprint_ll_addr((const uint8_t *)hwaddr,
					   sizeof(struct net_eth_addr)));

		memcpy(&entry->eth, hwaddr, sizeof(struct net_eth_addr));
	}
}

#if defined(CONFIG_NET_ARP_GRATUITOUS_TRANSMISSION)
static void arp_gratuitous_send(struct net_if *iface,
				struct in_addr *ipaddr)
{
	struct net_arp_hdr *hdr;
	struct net_pkt *pkt;

	pkt = net_pkt_alloc_with_buffer(iface, sizeof(struct net_arp_hdr),
					AF_UNSPEC, 0, NET_BUF_TIMEOUT);
	if (!pkt) {
		return;
	}

	net_buf_add(pkt->buffer, sizeof(struct net_arp_hdr));
	net_pkt_set_vlan_tag(pkt, net_eth_get_vlan_tag(iface));
	net_pkt_set_ll_proto_type(pkt, NET_ETH_PTYPE_ARP);

	hdr = NET_ARP_HDR(pkt);

	hdr->hwtype = htons(NET_ARP_HTYPE_ETH);
	hdr->protocol = htons(NET_ETH_PTYPE_IP);
	hdr->hwlen = sizeof(struct net_eth_addr);
	hdr->protolen = sizeof(struct in_addr);
	hdr->opcode = htons(NET_ARP_REQUEST);

	memcpy(&hdr->dst_hwaddr.addr, net_eth_broadcast_addr(),
	       sizeof(struct net_eth_addr));
	memcpy(&hdr->src_hwaddr.addr, net_if_get_link_addr(iface)->addr,
	       sizeof(struct net_eth_addr));

	net_ipv4_addr_copy_raw(hdr->dst_ipaddr, (uint8_t *)ipaddr);
	net_ipv4_addr_copy_raw(hdr->src_ipaddr, (uint8_t *)ipaddr);

	(void)net_linkaddr_set(net_pkt_lladdr_src(pkt),
			       net_if_get_link_addr(iface)->addr,
			       sizeof(struct net_eth_addr));

	(void)net_linkaddr_set(net_pkt_lladdr_dst(pkt),
			       (uint8_t *)net_eth_broadcast_addr(),
			       sizeof(struct net_eth_addr));

	NET_DBG("Sending gratuitous ARP pkt %p", pkt);

	/* send without timeout, so we do not risk being blocked by tx when
	 * being flooded
	 */
	if (net_if_try_send_data(iface, pkt, K_NO_WAIT) == NET_DROP) {
		net_pkt_unref(pkt);
	}
}

static void notify_all_ipv4_addr(struct net_if *iface)
{
	struct net_if_ipv4 *ipv4 = iface->config.ip.ipv4;
	int i;

	if (!ipv4) {
		return;
	}

	for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		if (ipv4->unicast[i].ipv4.is_used &&
		    ipv4->unicast[i].ipv4.address.family == AF_INET &&
		    ipv4->unicast[i].ipv4.addr_state == NET_ADDR_PREFERRED) {
			arp_gratuitous_send(iface,
					    &ipv4->unicast[i].ipv4.address.in_addr);
		}
	}
}

static void iface_event_handler(struct net_mgmt_event_callback *cb,
				uint32_t mgmt_event, struct net_if *iface)
{
	ARG_UNUSED(cb);

	if (!(net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET) ||
	      net_eth_is_vlan_interface(iface))) {
		return;
	}

	if (mgmt_event != NET_EVENT_IF_UP) {
		return;
	}

	notify_all_ipv4_addr(iface);
}

static void ipv4_event_handler(struct net_mgmt_event_callback *cb,
			       uint32_t mgmt_event, struct net_if *iface)
{
	struct in_addr *ipaddr;

	if (!(net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET) ||
	      net_eth_is_vlan_interface(iface))) {
		return;
	}

	if (!net_if_is_up(iface)) {
		return;
	}

	if (mgmt_event != NET_EVENT_IPV4_ADDR_ADD) {
		return;
	}

	if (cb->info_length != sizeof(struct in_addr)) {
		return;
	}

	ipaddr = (struct in_addr *)cb->info;

	arp_gratuitous_send(iface, ipaddr);
}

static void iface_cb(struct net_if *iface, void *user_data)
{
	ARG_UNUSED(user_data);

	if (!(net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET) ||
	      net_eth_is_vlan_interface(iface))) {
		return;
	}

	if (!net_if_is_up(iface)) {
		return;
	}

	notify_all_ipv4_addr(iface);
}

static void arp_gratuitous_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	net_if_foreach(iface_cb, NULL);

	k_work_reschedule(&arp_gratuitous_work,
			  K_SECONDS(CONFIG_NET_ARP_GRATUITOUS_INTERVAL));
}
#endif /* defined(CONFIG_NET_ARP_GRATUITOUS_TRANSMISSION) */

void net_arp_update(struct net_if *iface,
		    struct in_addr *src,
		    struct net_eth_addr *hwaddr,
		    bool gratuitous,
		    bool force)
{
	struct arp_entry *entry;
	struct net_pkt *pkt;

	NET_DBG("iface %d (%p) src %s", net_if_get_by_iface(iface), iface,
		net_sprint_ipv4_addr(src));
	net_if_tx_lock(iface);
	k_mutex_lock(&arp_mutex, K_FOREVER);

	entry = arp_entry_get_pending(iface, src);
	if (!entry) {
		if (IS_ENABLED(CONFIG_NET_ARP_GRATUITOUS) && gratuitous) {
			arp_gratuitous(iface, src, hwaddr);
		}

		if (force) {
			sys_snode_t *prev = NULL;
			struct arp_entry *arp_ent;

			arp_ent = arp_entry_find(&arp_table, iface, src, &prev);
			if (arp_ent) {
				memcpy(&arp_ent->eth, hwaddr,
				       sizeof(struct net_eth_addr));
			} else {
				/* Add new entry as it was not found and force
				 * was set.
				 */
				arp_ent = arp_entry_get_free();
				if (!arp_ent) {
					/* Then let's take one from table? */
					arp_ent = arp_entry_get_last_from_table();
				}

				if (arp_ent) {
					arp_ent->req_start = k_uptime_get_32();
					arp_ent->iface = iface;
					net_ipaddr_copy(&arp_ent->ip, src);
					memcpy(&arp_ent->eth, hwaddr, sizeof(arp_ent->eth));
					sys_slist_prepend(&arp_table, &arp_ent->node);
				}
			}
		}

		k_mutex_unlock(&arp_mutex);
		net_if_tx_unlock(iface);
		return;
	}

	memcpy(&entry->eth, hwaddr, sizeof(struct net_eth_addr));

	/* Inserting entry into the table */
	sys_slist_prepend(&arp_table, &entry->node);

	while (!k_fifo_is_empty(&entry->pending_queue)) {
		int ret;

		pkt = k_fifo_get(&entry->pending_queue, K_FOREVER);

		/* Set the dst in the pending packet */
		(void)net_linkaddr_set(net_pkt_lladdr_dst(pkt),
				       (const uint8_t *)&NET_ETH_HDR(pkt)->dst.addr,
				       sizeof(struct net_eth_addr));

		NET_DBG("iface %d (%p) dst %s pending %p frag %p ptype 0x%04x",
			net_if_get_by_iface(iface), iface,
			net_sprint_ipv4_addr(&entry->ip),
			pkt, pkt->frags, net_pkt_ll_proto_type(pkt));

		/* We directly send the packet without first queueing it.
		 * The pkt has already been queued for sending, once by
		 * net_if and second time in the ARP queue. We must not
		 * queue it twice in net_if so that the statistics of
		 * the pkt are not counted twice and the packet filter
		 * callbacks are only called once.
		 */
		ret = net_if_l2(iface)->send(iface, pkt);
		if (ret < 0) {
			net_pkt_unref(pkt);
		}
	}

	k_mutex_unlock(&arp_mutex);
	net_if_tx_unlock(iface);
}

static inline struct net_pkt *arp_prepare_reply(struct net_if *iface,
						struct net_pkt *req,
						struct net_eth_addr *dst_addr)
{
	struct net_arp_hdr *hdr, *query;
	struct net_pkt *pkt;

	pkt = net_pkt_alloc_with_buffer(iface, sizeof(struct net_arp_hdr),
					AF_UNSPEC, 0, NET_BUF_TIMEOUT);
	if (!pkt) {
		return NULL;
	}

	net_buf_add(pkt->buffer, sizeof(struct net_arp_hdr));

	hdr = NET_ARP_HDR(pkt);
	query = NET_ARP_HDR(req);

	if (IS_ENABLED(CONFIG_NET_VLAN)) {
		net_pkt_set_vlan_tag(pkt, net_pkt_vlan_tag(req));
	}

	hdr->hwtype = htons(NET_ARP_HTYPE_ETH);
	hdr->protocol = htons(NET_ETH_PTYPE_IP);
	hdr->hwlen = sizeof(struct net_eth_addr);
	hdr->protolen = sizeof(struct in_addr);
	hdr->opcode = htons(NET_ARP_REPLY);

	memcpy(&hdr->dst_hwaddr.addr, &dst_addr->addr,
	       sizeof(struct net_eth_addr));
	memcpy(&hdr->src_hwaddr.addr, net_if_get_link_addr(iface)->addr,
	       sizeof(struct net_eth_addr));

	net_ipv4_addr_copy_raw(hdr->dst_ipaddr, query->src_ipaddr);
	net_ipv4_addr_copy_raw(hdr->src_ipaddr, query->dst_ipaddr);

	(void)net_linkaddr_set(net_pkt_lladdr_src(pkt),
			       net_if_get_link_addr(iface)->addr,
			       sizeof(struct net_eth_addr));

	(void)net_linkaddr_set(net_pkt_lladdr_dst(pkt),
			       (uint8_t *)&hdr->dst_hwaddr.addr,
			       sizeof(struct net_eth_addr));

	net_pkt_set_ll_proto_type(pkt, NET_ETH_PTYPE_ARP);
	net_pkt_set_family(pkt, AF_INET);

	return pkt;
}

static bool arp_hdr_check(struct net_arp_hdr *arp_hdr)
{
	if (ntohs(arp_hdr->hwtype) != NET_ARP_HTYPE_ETH ||
	    ntohs(arp_hdr->protocol) != NET_ETH_PTYPE_IP ||
	    arp_hdr->hwlen != sizeof(struct net_eth_addr) ||
	    arp_hdr->protolen != NET_ARP_IPV4_PTYPE_SIZE ||
	    net_ipv4_is_addr_loopback((struct in_addr *)arp_hdr->src_ipaddr)) {
		NET_DBG("DROP: Invalid ARP header");
		return false;
	}

	return true;
}

enum net_verdict net_arp_input(struct net_pkt *pkt,
			       struct net_eth_addr *src,
			       struct net_eth_addr *dst)
{
	struct net_eth_addr *dst_hw_addr;
	struct net_arp_hdr *arp_hdr;
	struct net_pkt *reply;
	struct in_addr *addr;

	if (net_pkt_get_len(pkt) < sizeof(struct net_arp_hdr)) {
		NET_DBG("DROP: Too short ARP msg (%zu bytes, min %zu bytes)",
			net_pkt_get_len(pkt), sizeof(struct net_arp_hdr));
		return NET_DROP;
	}

	arp_hdr = NET_ARP_HDR(pkt);
	if (!arp_hdr_check(arp_hdr)) {
		return NET_DROP;
	}

	switch (ntohs(arp_hdr->opcode)) {
	case NET_ARP_REQUEST:
		/* If ARP request sender hw address is our address,
		 * we must drop the packet.
		 */
		if (memcmp(&arp_hdr->src_hwaddr,
			   net_if_get_link_addr(net_pkt_iface(pkt))->addr,
			   sizeof(struct net_eth_addr)) == 0) {
			return NET_DROP;
		}

		if (IS_ENABLED(CONFIG_NET_ARP_GRATUITOUS)) {
			if (net_eth_is_addr_broadcast(dst) &&
			    (net_eth_is_addr_broadcast(&arp_hdr->dst_hwaddr) ||
			     net_eth_is_addr_all_zeroes(&arp_hdr->dst_hwaddr)) &&
			    net_ipv4_addr_cmp_raw(arp_hdr->dst_ipaddr,
						  arp_hdr->src_ipaddr)) {
				/* If the IP address is in our cache,
				 * then update it here.
				 */
				net_arp_update(net_pkt_iface(pkt),
					       (struct in_addr *)arp_hdr->src_ipaddr,
					       &arp_hdr->src_hwaddr,
					       true, false);
				break;
			}
		}

		/* Discard ARP request if Ethernet address is broadcast
		 * and Source IP address is Multicast address.
		 */
		if (memcmp(dst, net_eth_broadcast_addr(),
			   sizeof(struct net_eth_addr)) == 0 &&
		    net_ipv4_is_addr_mcast((struct in_addr *)arp_hdr->src_ipaddr)) {
			NET_DBG("DROP: eth addr is bcast, src addr is mcast");
			return NET_DROP;
		}

		/* Someone wants to know our ll address */
		addr = if_get_addr(net_pkt_iface(pkt),
				   (struct in_addr *)arp_hdr->dst_ipaddr);
		if (!addr) {
			/* Not for us so drop the packet silently */
			return NET_DROP;
		}

		NET_DBG("ARP request from %s [%s] for %s",
			net_sprint_ipv4_addr(&arp_hdr->src_ipaddr),
			net_sprint_ll_addr((uint8_t *)&arp_hdr->src_hwaddr,
					   arp_hdr->hwlen),
			net_sprint_ipv4_addr(&arp_hdr->dst_ipaddr));

		/* Update the ARP cache if the sender MAC address has
		 * changed. In this case the target MAC address is all zeros
		 * and the target IP address is our address.
		 */
		if (net_eth_is_addr_unspecified(&arp_hdr->dst_hwaddr)) {
			NET_DBG("Updating ARP cache for %s [%s] iface %d",
				net_sprint_ipv4_addr(&arp_hdr->src_ipaddr),
				net_sprint_ll_addr((uint8_t *)&arp_hdr->src_hwaddr,
						   arp_hdr->hwlen),
				net_if_get_by_iface(net_pkt_iface(pkt)));

			net_arp_update(net_pkt_iface(pkt),
				       (struct in_addr *)arp_hdr->src_ipaddr,
				       &arp_hdr->src_hwaddr,
				       false, true);

			dst_hw_addr = &arp_hdr->src_hwaddr;
		} else {
			dst_hw_addr = src;
		}

		/* Send reply */
		reply = arp_prepare_reply(net_pkt_iface(pkt), pkt, dst_hw_addr);
		if (reply) {
			net_if_try_queue_tx(net_pkt_iface(reply), reply, K_NO_WAIT);
		} else {
			NET_DBG("Cannot send ARP reply");
		}
		break;

	case NET_ARP_REPLY:
		if (net_ipv4_is_my_addr((struct in_addr *)arp_hdr->dst_ipaddr)) {
			NET_DBG("Received ll %s for IP %s",
				net_sprint_ll_addr(arp_hdr->src_hwaddr.addr,
						   sizeof(struct net_eth_addr)),
				net_sprint_ipv4_addr(arp_hdr->src_ipaddr));
			net_arp_update(net_pkt_iface(pkt),
				       (struct in_addr *)arp_hdr->src_ipaddr,
				       &arp_hdr->src_hwaddr,
				       false, false);
		}

		break;
	}

	net_pkt_unref(pkt);

	return NET_OK;
}

void net_arp_clear_cache(struct net_if *iface)
{
	sys_snode_t *prev = NULL;
	struct arp_entry *entry, *next;

	NET_DBG("Flushing ARP table");

	k_mutex_lock(&arp_mutex, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&arp_table, entry, next, node) {
		if (iface && iface != entry->iface) {
			prev = &entry->node;
			continue;
		}

		arp_entry_cleanup(entry, false);

		sys_slist_remove(&arp_table, prev, &entry->node);
		sys_slist_prepend(&arp_free_entries, &entry->node);
	}

	prev = NULL;

	NET_DBG("Flushing ARP pending requests");

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&arp_pending_entries,
					  entry, next, node) {
		if (iface && iface != entry->iface) {
			prev = &entry->node;
			continue;
		}

		arp_entry_cleanup(entry, true);

		sys_slist_remove(&arp_pending_entries, prev, &entry->node);
		sys_slist_prepend(&arp_free_entries, &entry->node);
	}

	if (sys_slist_is_empty(&arp_pending_entries)) {
		k_work_cancel_delayable(&arp_request_timer);
	}

	k_mutex_unlock(&arp_mutex);
}

int net_arp_clear_pending(struct net_if *iface, struct in_addr *dst)
{
	struct arp_entry *entry = arp_entry_find_pending(iface, dst);

	if (!entry) {
		return -ENOENT;
	}

	arp_entry_cleanup(entry, true);

	return 0;
}

int net_arp_foreach(net_arp_cb_t cb, void *user_data)
{
	int ret = 0;
	struct arp_entry *entry;

	k_mutex_lock(&arp_mutex, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER(&arp_table, entry, node) {
		ret++;
		cb(entry, user_data);
	}

	k_mutex_unlock(&arp_mutex);

	return ret;
}

void net_arp_init(void)
{
	int i;

	if (arp_cache_initialized) {
		return;
	}

	sys_slist_init(&arp_free_entries);
	sys_slist_init(&arp_pending_entries);
	sys_slist_init(&arp_table);

	for (i = 0; i < CONFIG_NET_ARP_TABLE_SIZE; i++) {
		/* Inserting entry as free with initialised packet queue */
		k_fifo_init(&arp_entries[i].pending_queue);
		sys_slist_prepend(&arp_free_entries, &arp_entries[i].node);
	}

	k_work_init_delayable(&arp_request_timer, arp_request_timeout);

	k_mutex_init(&arp_mutex);

	arp_cache_initialized = true;

#if defined(CONFIG_NET_ARP_GRATUITOUS_TRANSMISSION)
	net_mgmt_init_event_callback(&iface_event_cb, iface_event_handler,
				     NET_EVENT_IF_UP);
	net_mgmt_init_event_callback(&ipv4_event_cb, ipv4_event_handler,
				     NET_EVENT_IPV4_ADDR_ADD);

	net_mgmt_add_event_callback(&iface_event_cb);
	net_mgmt_add_event_callback(&ipv4_event_cb);

	k_work_init_delayable(&arp_gratuitous_work,
			      arp_gratuitous_work_handler);
	k_work_reschedule(&arp_gratuitous_work,
			  K_SECONDS(CONFIG_NET_ARP_GRATUITOUS_INTERVAL));
#endif /* defined(CONFIG_NET_ARP_GRATUITOUS_TRANSMISSION) */
}

static enum net_verdict arp_recv(struct net_if *iface,
				 uint16_t ptype,
				 struct net_pkt *pkt)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(ptype);

	net_pkt_set_family(pkt, AF_INET);

	NET_DBG("ARP packet from %s received",
		net_sprint_ll_addr(net_pkt_lladdr_src(pkt)->addr,
				   sizeof(struct net_eth_addr)));

	if (IS_ENABLED(CONFIG_NET_IPV4_ACD) &&
	    net_ipv4_acd_input(iface, pkt) == NET_DROP) {
		return NET_DROP;
	}

	return net_arp_input(pkt,
			     (struct net_eth_addr *)net_pkt_lladdr_src(pkt)->addr,
			     (struct net_eth_addr *)net_pkt_lladdr_dst(pkt)->addr);
}

ETH_NET_L3_REGISTER(ARP, NET_ETH_PTYPE_ARP, arp_recv);
