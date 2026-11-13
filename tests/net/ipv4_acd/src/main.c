/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/ztest.h>

#include "arp.h"

/* RFC 5227: probing ends ANNOUNCE_WAIT seconds after the last of the
 * PROBE_NUM probes, each sent at most PROBE_MAX seconds after the previous.
 */
#define ACD_PROBE_NUM            3
#define ACD_PROBE_MAX            K_SECONDS(3)
#define ACD_ANNOUNCE_WAIT_MS     2000
#define ACD_ANNOUNCE_NUM         2
#define ACD_ANNOUNCE_INTERVAL_MS 2000

/* Long enough for a blocked thread to show up as a deadlock. */
#define DEADLOCK_TIMEOUT K_SECONDS(10)

static K_SEM_DEFINE(probe_sem, 0, K_SEM_MAX_LIMIT);
static K_SEM_DEFINE(announce_sem, 0, K_SEM_MAX_LIMIT);

/* Address whose ARP probes and announcements are counted by the driver. */
static struct net_in_addr probe_target;

/* 00-00-5E-00-53-xx Documentation RFC 7042 */
static uint8_t mac_addr[sizeof(struct net_eth_addr)] = {0x00, 0x00, 0x5E, 0x00, 0x53, 0x01};

static void acd_test_iface_init(struct net_if *iface)
{
	net_if_set_link_addr(iface, mac_addr, sizeof(mac_addr), NET_LINK_ETHERNET);
}

static int acd_test_send(const struct device *dev, struct net_pkt *pkt)
{
	const struct net_eth_hdr *hdr = (const struct net_eth_hdr *)net_pkt_data(pkt);
	const struct net_arp_hdr *arp_hdr;

	ARG_UNUSED(dev);

	if (net_ntohs(hdr->type) != NET_ETH_PTYPE_ARP) {
		return 0;
	}

	if (IS_ENABLED(CONFIG_NET_L2_ETHERNET_RESERVE_HEADER)) {
		arp_hdr = (const struct net_arp_hdr *)(net_pkt_data(pkt) +
						       sizeof(struct net_eth_hdr));
	} else {
		arp_hdr = (const struct net_arp_hdr *)pkt->frags->frags->data;
	}

	if (net_ntohs(arp_hdr->opcode) != NET_ARP_REQUEST ||
	    !net_ipv4_addr_cmp_raw(arp_hdr->dst_ipaddr, probe_target.s4_addr)) {
		return 0;
	}

	/* A probe has an all-zero sender address, an announcement has the
	 * address itself as the sender.
	 */
	if (net_ipv4_is_addr_unspecified_raw(arp_hdr->src_ipaddr)) {
		k_sem_give(&probe_sem);
	} else if (net_ipv4_addr_cmp_raw(arp_hdr->src_ipaddr, probe_target.s4_addr)) {
		k_sem_give(&announce_sem);
	}

	return 0;
}

static DEVICE_API(ethernet, acd_test_api) = {
	.iface_api.init = acd_test_iface_init,
	.send = acd_test_send,
};

NET_DEVICE_INIT(acd_test, "acd_test", NULL, NULL, NULL, NULL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&acd_test_api, ETHERNET_L2, NET_L2_GET_CTX_TYPE(ETHERNET_L2), NET_ETH_MTU);

struct locked_removal {
	struct net_if *iface;
	struct net_in_addr addr;
	k_timeout_t hold;
	struct k_sem locked;
};

static void lock_and_remove(void *p1, void *p2, void *p3)
{
	struct locked_removal *rm = p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	net_if_lock(rm->iface);
	k_sem_give(&rm->locked);
	k_sleep(rm->hold);
	(void)net_if_ipv4_addr_rm(rm->iface, &rm->addr);
	net_if_unlock(rm->iface);
}

/* Remove the address in another thread that holds the interface lock for
 * rm->hold before removing it and so before taking the ACD lock. Returns once
 * that thread holds the interface lock.
 */
static k_tid_t start_locked_removal(struct locked_removal *rm)
{
	static K_THREAD_STACK_DEFINE(stack, 2048);
	static struct k_thread thread;
	k_tid_t tid;

	k_sem_init(&rm->locked, 0, 1);

	tid = k_thread_create(&thread, stack, K_THREAD_STACK_SIZEOF(stack), lock_and_remove, rm,
			      NULL, NULL, K_PRIO_PREEMPT(1), 0, K_NO_WAIT);

	zassert_ok(k_sem_take(&rm->locked, K_SECONDS(1)), "Interface lock not taken");

	return tid;
}

static struct net_if *add_probed_addr(const char *addr_str, struct net_in_addr *addr)
{
	struct net_if *iface = net_if_get_first_by_type(&NET_L2_GET_NAME(ETHERNET));

	zassert_not_null(iface, "No Ethernet interface");
	zassert_true(net_if_is_up(iface), "Interface is down");
	zassert_ok(net_addr_pton(NET_AF_INET, addr_str, addr));

	k_sem_reset(&probe_sem);
	k_sem_reset(&announce_sem);
	net_ipaddr_copy(&probe_target, addr);

	zassert_not_null(net_if_ipv4_addr_add(iface, addr, NET_ADDR_MANUAL, 0), "Cannot add %s",
			 addr_str);

	return iface;
}

static void recv_conflicting_reply(struct net_if *iface, const struct net_in_addr *addr)
{
	/* Another host that uses the address. */
	static const struct net_eth_addr peer = {{0x00, 0x00, 0x5E, 0x00, 0x53, 0x02}};
	struct net_eth_hdr eth = {0};
	struct net_arp_hdr arp = {0};
	struct net_pkt *pkt;

	memcpy(&eth.dst, mac_addr, sizeof(eth.dst));
	memcpy(&eth.src, &peer, sizeof(eth.src));
	eth.type = net_htons(NET_ETH_PTYPE_ARP);

	arp.hwtype = net_htons(NET_ARP_HTYPE_ETH);
	arp.protocol = net_htons(NET_ETH_PTYPE_IP);
	arp.hwlen = sizeof(struct net_eth_addr);
	arp.protolen = NET_ARP_IPV4_PTYPE_SIZE;
	arp.opcode = net_htons(NET_ARP_REPLY);
	memcpy(&arp.src_hwaddr, &peer, sizeof(arp.src_hwaddr));
	memcpy(arp.src_ipaddr, addr, sizeof(arp.src_ipaddr));
	memcpy(&arp.dst_hwaddr, mac_addr, sizeof(arp.dst_hwaddr));
	memcpy(arp.dst_ipaddr, addr, sizeof(arp.dst_ipaddr));

	pkt = net_pkt_rx_alloc_with_buffer(iface, sizeof(eth) + sizeof(arp), NET_AF_UNSPEC, 0,
					   K_SECONDS(1));
	zassert_not_null(pkt, "Cannot allocate packet");
	zassert_ok(net_pkt_write(pkt, &eth, sizeof(eth)));
	zassert_ok(net_pkt_write(pkt, &arp, sizeof(arp)));
	zassert_ok(net_recv_data(iface, pkt));
}

ZTEST(ipv4_acd, test_probing_ends_with_valid_address)
{
	struct net_in_addr addr;
	struct net_if_addr *ifaddr;
	struct net_if *iface;

	iface = add_probed_addr("192.0.2.3", &addr);

	ifaddr = net_if_ipv4_addr_lookup(&addr, NULL);
	zassert_not_null(ifaddr, "Address not added");
	zexpect_equal(ifaddr->addr_state, NET_ADDR_TENTATIVE, "Address not tentative");

	for (int i = 0; i < ACD_PROBE_NUM; i++) {
		zassert_ok(k_sem_take(&probe_sem, ACD_PROBE_MAX), "Probe %d not sent", i + 1);
	}

	zassert_ok(k_sem_take(&announce_sem, K_MSEC(ACD_ANNOUNCE_WAIT_MS + 1000)),
		   "First announcement not sent");
	zexpect_equal(ifaddr->addr_state, NET_ADDR_PREFERRED, "Address not valid");

	for (int i = 1; i < ACD_ANNOUNCE_NUM; i++) {
		zexpect_ok(k_sem_take(&announce_sem, K_MSEC(ACD_ANNOUNCE_INTERVAL_MS + 1000)),
			   "Announcement %d not sent", i + 1);
	}

	zexpect_true(net_if_ipv4_addr_rm(iface, &addr), "Cannot remove address");
}

/* The ACD timer marks the address valid once probing ends, while another
 * thread holding the interface lock removes the address.
 */
ZTEST(ipv4_acd, test_addr_rm_while_probing_ends)
{
	/* Hold the interface lock from before probing ends until after. */
	struct locked_removal rm = {.hold = K_MSEC(ACD_ANNOUNCE_WAIT_MS + 1000)};
	k_tid_t tid;

	rm.iface = add_probed_addr("192.0.2.1", &rm.addr);

	for (int i = 0; i < ACD_PROBE_NUM; i++) {
		zassert_ok(k_sem_take(&probe_sem, ACD_PROBE_MAX), "Probe %d not sent", i + 1);
	}

	tid = start_locked_removal(&rm);

	zassert_ok(k_thread_join(tid, DEADLOCK_TIMEOUT),
		   "Address removal and the ACD timer deadlocked");
	zexpect_is_null(net_if_ipv4_addr_lookup(&rm.addr, NULL), "Address not removed");
}

/* A conflicting ARP reply arrives during probing, while another thread
 * holding the interface lock removes the address.
 */
ZTEST(ipv4_acd, test_addr_rm_while_conflict_received)
{
	struct locked_removal rm = {.hold = K_MSEC(500)};
	k_tid_t tid;

	rm.iface = add_probed_addr("192.0.2.2", &rm.addr);

	zassert_ok(k_sem_take(&probe_sem, ACD_PROBE_MAX), "Probe not sent");

	tid = start_locked_removal(&rm);
	recv_conflicting_reply(rm.iface, &rm.addr);

	zassert_ok(k_thread_join(tid, DEADLOCK_TIMEOUT),
		   "Address removal and ARP input deadlocked");
	zexpect_is_null(net_if_ipv4_addr_lookup(&rm.addr, NULL), "Address not removed");
}

ZTEST_SUITE(ipv4_acd, NULL, NULL, NULL, NULL, NULL);
