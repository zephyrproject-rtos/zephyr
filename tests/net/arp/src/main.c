/* main.c - Application main entry point */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_ARP_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/linker/sections.h>

#include <zephyr/tc_util.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/dummy.h>
#include <zephyr/ztest.h>
#include <zephyr/random/random.h>

#include "arp.h"

#define NET_LOG_ENABLED 1
#include "net_private.h"

static bool req_test;

static char *app_data = "0123456789";

static bool entry_found;
static struct net_eth_addr *expected_hwaddr;

static struct net_pkt *pending_pkt;

static struct net_eth_addr eth_hwaddr = { { 0x42, 0x11, 0x69, 0xde, 0xfa, 0xec } };

static int send_status = -EINVAL;

struct net_arp_context {
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
};

int net_arp_dev_init(const struct device *dev)
{
	struct net_arp_context *net_arp_context = dev->data;

	net_arp_context = net_arp_context;

	return 0;
}

static uint8_t *net_arp_get_mac(const struct device *dev)
{
	struct net_arp_context *context = dev->data;

	if (context->mac_addr[2] == 0x00) {
		/* 00-00-5E-00-53-xx Documentation RFC 7042 */
		context->mac_addr[0] = 0x00;
		context->mac_addr[1] = 0x00;
		context->mac_addr[2] = 0x5E;
		context->mac_addr[3] = 0x00;
		context->mac_addr[4] = 0x53;
		context->mac_addr[5] = sys_rand8_get();
	}

	return context->mac_addr;
}

static void net_arp_iface_init(struct net_if *iface)
{
	uint8_t *mac = net_arp_get_mac(net_if_get_device(iface));

	net_if_set_link_addr(iface, mac, 6, NET_LINK_ETHERNET);
}

static int tester_send(const struct device *dev, struct net_pkt *pkt)
{
	struct net_eth_hdr *hdr;

	if (!pkt->buffer) {
		printk("No data to send!\n");
		return -ENODATA;
	}

	hdr = (struct net_eth_hdr *)net_pkt_data(pkt);

	if (ntohs(hdr->type) == NET_ETH_PTYPE_ARP) {
		/* First frag has eth hdr */
		struct net_arp_hdr *arp_hdr =
			(struct net_arp_hdr *)pkt->frags->frags;

		if (ntohs(arp_hdr->opcode) == NET_ARP_REPLY) {
			if (!req_test && pkt != pending_pkt) {
				printk("Pending data but to be sent is wrong, "
				       "expecting %p but got %p\n",
				       pending_pkt, pkt);
				return -EINVAL;
			}

			if (!req_test && memcmp(&hdr->dst, &eth_hwaddr,
						sizeof(struct net_eth_addr))) {
				char out[sizeof("xx:xx:xx:xx:xx:xx")];

				snprintk(out, sizeof(out), "%s",
					 net_sprint_ll_addr(
						 (uint8_t *)&hdr->dst,
						 sizeof(struct net_eth_addr)));
				printk("Invalid dst hwaddr %s, should be %s\n",
				       out,
				       net_sprint_ll_addr(
					       (uint8_t *)&eth_hwaddr,
					       sizeof(struct net_eth_addr)));
				send_status = -EINVAL;
				return send_status;
			}

		} else if (ntohs(arp_hdr->opcode) == NET_ARP_REQUEST) {
			if (memcmp(&hdr->src, &eth_hwaddr,
				   sizeof(struct net_eth_addr))) {
				char out[sizeof("xx:xx:xx:xx:xx:xx")];

				snprintk(out, sizeof(out), "%s",
					 net_sprint_ll_addr(
						 (uint8_t *)&hdr->src,
						 sizeof(struct net_eth_addr)));
				printk("Invalid src hwaddr %s, should be %s\n",
				       out,
				       net_sprint_ll_addr(
					       (uint8_t *)&eth_hwaddr,
					       sizeof(struct net_eth_addr)));
				send_status = -EINVAL;
				return send_status;
			}
		}
	}

	send_status = 0;

	return 0;
}

static inline struct net_pkt *prepare_arp_reply(struct net_if *iface,
						struct net_pkt *req,
						struct net_eth_addr *addr,
						struct net_eth_hdr **eth_rep)
{
	struct net_pkt *pkt;
	struct net_arp_hdr *hdr;
	struct net_eth_hdr *eth;

	pkt = net_pkt_alloc_with_buffer(iface, sizeof(struct net_eth_hdr) +
					sizeof(struct net_eth_hdr) +
					sizeof(struct net_arp_hdr),
					AF_UNSPEC, 0, K_SECONDS(1));
	zassert_not_null(pkt, "out of mem reply");

	eth = NET_ETH_HDR(pkt);

	net_buf_add(pkt->buffer, sizeof(struct net_eth_hdr));
	net_buf_pull(pkt->buffer, sizeof(struct net_eth_hdr));

	(void)memset(&eth->dst.addr, 0xff, sizeof(struct net_eth_addr));
	memcpy(&eth->src.addr, net_if_get_link_addr(iface)->addr,
	       sizeof(struct net_eth_addr));
	eth->type = htons(NET_ETH_PTYPE_ARP);

	*eth_rep = eth;

	net_buf_add(pkt->buffer, sizeof(struct net_eth_hdr));
	net_buf_pull(pkt->buffer, sizeof(struct net_eth_hdr));

	hdr = NET_ARP_HDR(pkt);

	hdr->hwtype = htons(NET_ARP_HTYPE_ETH);
	hdr->protocol = htons(NET_ETH_PTYPE_IP);
	hdr->hwlen = sizeof(struct net_eth_addr);
	hdr->protolen = sizeof(struct in_addr);
	hdr->opcode = htons(NET_ARP_REPLY);

	memcpy(&hdr->dst_hwaddr.addr, &eth->src.addr,
	       sizeof(struct net_eth_addr));
	memcpy(&hdr->src_hwaddr.addr, addr,
	       sizeof(struct net_eth_addr));

	net_ipv4_addr_copy_raw(hdr->dst_ipaddr, NET_ARP_HDR(req)->src_ipaddr);
	net_ipv4_addr_copy_raw(hdr->src_ipaddr, NET_ARP_HDR(req)->dst_ipaddr);

	net_buf_add(pkt->buffer, sizeof(struct net_arp_hdr));

	net_pkt_set_ll_proto_type(pkt, NET_ETH_PTYPE_ARP);

	return pkt;
}

static inline struct net_pkt *prepare_arp_request(struct net_if *iface,
						  struct net_pkt *req,
						  struct net_eth_addr *addr,
						  struct net_eth_hdr **eth_hdr)
{
	struct net_pkt *pkt;
	struct net_arp_hdr *hdr, *req_hdr;
	struct net_eth_hdr *eth, *eth_req;

	pkt = net_pkt_alloc_with_buffer(iface, sizeof(struct net_eth_hdr) +
					sizeof(struct net_arp_hdr),
					AF_UNSPEC, 0, K_SECONDS(1));
	zassert_not_null(pkt, "out of mem request");

	eth_req = NET_ETH_HDR(req);
	eth = NET_ETH_HDR(pkt);

	net_buf_add(req->buffer, sizeof(struct net_eth_hdr));
	net_buf_pull(req->buffer, sizeof(struct net_eth_hdr));

	req_hdr = NET_ARP_HDR(req);

	(void)memset(&eth->dst.addr, 0xff, sizeof(struct net_eth_addr));
	memcpy(&eth->src.addr, addr, sizeof(struct net_eth_addr));

	eth->type = htons(NET_ETH_PTYPE_ARP);
	*eth_hdr = eth;

	net_buf_add(pkt->buffer, sizeof(struct net_eth_hdr));
	net_buf_pull(pkt->buffer, sizeof(struct net_eth_hdr));

	hdr = NET_ARP_HDR(pkt);

	hdr->hwtype = htons(NET_ARP_HTYPE_ETH);
	hdr->protocol = htons(NET_ETH_PTYPE_IP);
	hdr->hwlen = sizeof(struct net_eth_addr);
	hdr->protolen = sizeof(struct in_addr);
	hdr->opcode = htons(NET_ARP_REQUEST);

	(void)memset(&hdr->dst_hwaddr.addr, 0x00, sizeof(struct net_eth_addr));
	memcpy(&hdr->src_hwaddr.addr, addr, sizeof(struct net_eth_addr));

	net_ipv4_addr_copy_raw(hdr->src_ipaddr, req_hdr->src_ipaddr);
	net_ipv4_addr_copy_raw(hdr->dst_ipaddr, req_hdr->dst_ipaddr);

	net_buf_add(pkt->buffer, sizeof(struct net_arp_hdr));

	net_pkt_set_ll_proto_type(pkt, NET_ETH_PTYPE_ARP);

	return pkt;
}

static void setup_eth_header(struct net_if *iface, struct net_pkt *pkt,
			     const struct net_eth_addr *hwaddr, uint16_t type)
{
	struct net_eth_hdr *hdr = (struct net_eth_hdr *)net_pkt_data(pkt);

	memcpy(&hdr->dst.addr, hwaddr, sizeof(struct net_eth_addr));
	memcpy(&hdr->src.addr, net_if_get_link_addr(iface)->addr,
	       sizeof(struct net_eth_addr));

	hdr->type = htons(type);
}

struct net_arp_context net_arp_context_data;

#if defined(CONFIG_NET_ARP) && defined(CONFIG_NET_L2_ETHERNET)
static const struct ethernet_api net_arp_if_api = {
	.iface_api.init = net_arp_iface_init,
	.send = tester_send,
};

#define _ETH_L2_LAYER ETHERNET_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(ETHERNET_L2)
#else
static const struct dummy_api net_arp_if_api = {
	.iface_api.init = net_arp_iface_init,
	.send = tester_send,
};

#define _ETH_L2_LAYER DUMMY_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(DUMMY_L2)
#endif

NET_DEVICE_INIT(net_arp_test, "net_arp_test",
		net_arp_dev_init, NULL,
		&net_arp_context_data, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&net_arp_if_api, _ETH_L2_LAYER, _ETH_L2_CTX_TYPE, 127);

static void arp_cb(struct arp_entry *entry, void *user_data)
{
	struct in_addr *addr = user_data;

	if (memcmp(&entry->ip, addr, sizeof(struct in_addr)) == 0 &&
	    memcmp(&entry->eth, expected_hwaddr,
		   sizeof(struct net_eth_addr)) == 0) {
		entry_found = true;
	}
}

ZTEST(arp_fn_tests, test_arp)
{
	if (IS_ENABLED(CONFIG_NET_TC_THREAD_COOPERATIVE)) {
		k_thread_priority_set(k_current_get(),
				K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1));
	} else {
		k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(9));
	}

	struct net_eth_hdr *eth_hdr = NULL;
	struct net_eth_addr dst_lladdr;
	struct net_pkt *pkt;
	struct net_pkt *pkt2;
	struct net_pkt *pkt_arp;
	struct net_if *iface;
	struct net_if_addr *ifaddr;
	struct net_arp_hdr *arp_hdr;
	struct net_ipv4_hdr *ipv4;
	int len, ret;

	struct in_addr dst = { { { 192, 0, 2, 2 } } };
	struct in_addr dst_far = { { { 10, 11, 12, 13 } } };
	struct in_addr dst_far2 = { { { 172, 16, 14, 186 } } };
	struct in_addr src = { { { 192, 0, 2, 1 } } };
	struct in_addr netmask = { { { 255, 255, 255, 0 } } };
	struct in_addr gw = { { { 192, 0, 2, 42 } } };

	net_arp_init();

	(void)memset(&dst_lladdr, 0xff, sizeof(struct net_eth_addr));

	iface = net_if_lookup_by_dev(DEVICE_GET(net_arp_test));

	net_if_ipv4_set_gw(iface, &gw);

	/* Unicast test */
	ifaddr = net_if_ipv4_addr_add(iface,
				      &src,
				      NET_ADDR_MANUAL,
				      0);
	zassert_not_null(ifaddr, "Cannot add address");
	ifaddr->addr_state = NET_ADDR_PREFERRED;

	net_if_ipv4_set_netmask_by_addr(iface, &src, &netmask);

	len = strlen(app_data);

	/* Application data for testing */
	pkt = net_pkt_alloc_with_buffer(iface, sizeof(struct net_ipv4_hdr) +
		len, AF_INET, 0, K_SECONDS(1));
	zassert_not_null(pkt, "out of mem");

	(void)net_linkaddr_set(net_pkt_lladdr_src(pkt),
			       net_if_get_link_addr(iface)->addr,
			       sizeof(struct net_eth_addr));

	ipv4 = (struct net_ipv4_hdr *)net_buf_add(pkt->buffer,
						  sizeof(struct net_ipv4_hdr));
	net_ipv4_addr_copy_raw(ipv4->src, (uint8_t *)&src);
	net_ipv4_addr_copy_raw(ipv4->dst, (uint8_t *)&dst);

	net_pkt_set_ll_proto_type(pkt, NET_ETH_PTYPE_IP);

	memcpy(net_buf_add(pkt->buffer, len), app_data, len);

	/* Duplicate packet */
	pkt2 = net_pkt_clone(pkt, K_SECONDS(1));
	zassert_not_null(pkt2, "out of mem");

	/* First ARP request */
	ret = net_arp_prepare(pkt, &dst, NULL, &pkt_arp);

	zassert_equal(NET_ARP_PKT_REPLACED, ret);

	/* pkt_arp is the ARP packet and pkt is the IPv4 packet and it was
	 * stored in ARP table.
	 */

	zassert_equal(net_pkt_ll_proto_type(pkt_arp), NET_ETH_PTYPE_ARP,
		      "ARP packet type is wrong");

	/**TESTPOINTS: Check packets*/
	zassert_not_equal((void *)(pkt_arp), (void *)(pkt),
		/* The packets cannot be the same as the ARP cache has
		 * still room for the pkt.
		 */
		"ARP cache should still have free space");

	zassert_not_null(pkt_arp, "ARP pkt is empty");

	/* The ARP cache should now have a link to pending net_pkt
	 * that is to be sent after we have got an ARP reply.
	 */
	zassert_not_null(pkt->buffer,
		"Pending pkt buffer is NULL");

	pending_pkt = pkt;

	/* pkt_arp should contain the arp header, verify it */
	arp_hdr = NET_ARP_HDR(pkt_arp);

	if (arp_hdr->hwtype != htons(NET_ARP_HTYPE_ETH)) {
		printk("ARP hwtype 0x%x, should be 0x%x\n",
		       arp_hdr->hwtype, htons(NET_ARP_HTYPE_ETH));
		zassert_true(0, "exiting");
	}

	if (arp_hdr->protocol != htons(NET_ETH_PTYPE_IP)) {
		printk("ARP protocol 0x%x, should be 0x%x\n",
		       arp_hdr->protocol, htons(NET_ETH_PTYPE_IP));
		zassert_true(0, "exiting");
	}

	if (arp_hdr->hwlen != sizeof(struct net_eth_addr)) {
		printk("ARP hwlen 0x%x, should be 0x%zx\n",
		       arp_hdr->hwlen, sizeof(struct net_eth_addr));
		zassert_true(0, "exiting");
	}

	if (arp_hdr->protolen != sizeof(struct in_addr)) {
		printk("ARP IP addr len 0x%x, should be 0x%zx\n",
		       arp_hdr->protolen, sizeof(struct in_addr));
		zassert_true(0, "exiting");
	}

	if (arp_hdr->opcode != htons(NET_ARP_REQUEST)) {
		printk("ARP opcode 0x%x, should be 0x%x\n",
		       arp_hdr->opcode, htons(NET_ARP_REQUEST));
		zassert_true(0, "exiting");
	}

	if (!net_ipv4_addr_cmp_raw(arp_hdr->dst_ipaddr,
				   NET_IPV4_HDR(pkt)->dst)) {
		printk("ARP IP dest invalid %s, should be %s",
			net_sprint_ipv4_addr(&arp_hdr->dst_ipaddr),
			net_sprint_ipv4_addr(&NET_IPV4_HDR(pkt)->dst));
		zassert_true(0, "exiting");
	}

	if (!net_ipv4_addr_cmp_raw(arp_hdr->src_ipaddr,
				   NET_IPV4_HDR(pkt)->src)) {
		printk("ARP IP src invalid %s, should be %s",
			net_sprint_ipv4_addr(&arp_hdr->src_ipaddr),
			net_sprint_ipv4_addr(&NET_IPV4_HDR(pkt)->src));
		zassert_true(0, "exiting");
	}

	/* We could have send the new ARP request but for this test we
	 * just free it.
	 */
	net_pkt_unref(pkt_arp);

	zassert_equal(atomic_get(&pkt->atomic_ref), 2,
		"ARP cache should own the original packet");


	/* A second packet going to the same destination */
	pkt_arp = NULL;
	ret = net_arp_prepare(pkt2, &dst, NULL, &pkt_arp);

	/* Packet should have been queued, not generating an ARP request */
	zassert_equal(NET_ARP_PKT_QUEUED, ret);
	zassert_is_null(pkt_arp, "ARP packet should not have been generated");

	zassert_equal(atomic_get(&pkt2->atomic_ref), 2,
		"ARP cache should own the original packet");

	/* Done with the duplicate packet */
	net_pkt_unref(pkt2);

	/* Then a case where target is not in the same subnet */
	net_ipv4_addr_copy_raw(ipv4->dst, (uint8_t *)&dst_far);

	ret = net_arp_prepare(pkt, &dst_far, NULL, &pkt_arp);

	zassert_equal(NET_ARP_PKT_REPLACED, ret);

	zassert_not_equal((void *)(pkt_arp), (void *)(pkt),
		"ARP cache should not find anything");

	/**TESTPOINTS: Check if packets not empty*/
	zassert_not_null(pkt_arp,
		"ARP pkt_arp is empty");

	arp_hdr = NET_ARP_HDR(pkt_arp);

	if (!net_ipv4_addr_cmp_raw(arp_hdr->dst_ipaddr,
				   (uint8_t *)&iface->config.ip.ipv4->gw)) {
		printk("ARP IP dst invalid %s, should be %s\n",
			net_sprint_ipv4_addr(&arp_hdr->dst_ipaddr),
			net_sprint_ipv4_addr(&iface->config.ip.ipv4->gw));
		zassert_true(0, "exiting");
	}

	net_pkt_unref(pkt_arp);

	/* Try to find the same destination again, this should fail as there
	 * is a pending request in ARP cache.
	 */
	net_ipv4_addr_copy_raw(ipv4->dst, (uint8_t *)&dst_far);

	/* Make sure prepare will not free the pkt because it will be
	 * needed in the later test case.
	 */
	net_pkt_ref(pkt);

	ret = net_arp_prepare(pkt, &dst_far, NULL, &pkt_arp);

	zassert_equal(NET_ARP_PKT_REPLACED, ret);

	zassert_not_null(pkt_arp,
		"ARP cache is not sending the request again");

	net_pkt_unref(pkt_arp);

	ret = net_arp_prepare(pkt, &dst_far, NULL, &pkt_arp);

	zassert_equal(NET_ARP_PKT_REPLACED, ret);

	zassert_not_null(pkt_arp,
		"ARP cache is not sending the request again");

	net_pkt_unref(pkt_arp);

	/* Try to find the different destination, this should fail too
	 * as the cache table should be full.
	 */
	net_ipv4_addr_copy_raw(ipv4->dst, (uint8_t *)&dst_far2);

	/* Make sure prepare will not free the pkt because it will be
	 * needed in the next test case.
	 */
	net_pkt_ref(pkt);

	ret = net_arp_prepare(pkt, &dst_far2, NULL, &pkt_arp);

	zassert_equal(NET_ARP_PKT_REPLACED, ret);

	zassert_not_null(pkt_arp,
		"ARP cache did not send a req");

	/* Restore the original address so that following test case can
	 * work properly.
	 */
	net_ipv4_addr_copy_raw(ipv4->dst, (uint8_t *)&dst);

	/* The arp request packet is now verified, create an arp reply.
	 * The previous value of pkt is stored in arp table and is not lost.
	 */
	pkt = net_pkt_alloc_with_buffer(iface, sizeof(struct net_eth_hdr) +
					sizeof(struct net_arp_hdr),
					AF_UNSPEC, 0, K_SECONDS(1));
	zassert_not_null(pkt, "out of mem reply");

	arp_hdr = NET_ARP_HDR(pkt);
	net_buf_add(pkt->buffer, sizeof(struct net_arp_hdr));

	net_ipv4_addr_copy_raw(arp_hdr->dst_ipaddr, (uint8_t *)&dst);
	net_ipv4_addr_copy_raw(arp_hdr->src_ipaddr, (uint8_t *)&src);

	net_pkt_set_ll_proto_type(pkt, NET_ETH_PTYPE_ARP);

	pkt_arp = prepare_arp_reply(iface, pkt, &eth_hwaddr, &eth_hdr);

	zassert_not_null(pkt_arp, "ARP reply generation failed.");

	/* The pending packet should now be sent */
	switch (net_arp_input(pkt_arp,
			      (struct net_eth_addr *)net_pkt_lladdr_src(pkt_arp)->addr,
			      &dst_lladdr)) {
	case NET_OK:
	case NET_CONTINUE:
		break;
	case NET_DROP:
		break;
	}

	/* Yielding so that network interface TX thread can proceed. */
	k_yield();

	/**TESTPOINTS: Check ARP reply*/
	zassert_false(send_status < 0, "ARP reply was not sent");

	zassert_equal(atomic_get(&pkt->atomic_ref), 1,
		      "ARP cache should no longer own the original packet");

	net_pkt_unref(pkt);

	/* Then feed in ARP request */
	pkt = net_pkt_alloc_with_buffer(iface, sizeof(struct net_eth_hdr) +
					sizeof(struct net_arp_hdr),
					AF_UNSPEC, 0, K_SECONDS(1));
	zassert_not_null(pkt, "out of mem reply");

	send_status = -EINVAL;

	setup_eth_header(iface, pkt, &eth_hwaddr, NET_ETH_PTYPE_ARP);

	arp_hdr = (struct net_arp_hdr *)(pkt->buffer->data +
					 (sizeof(struct net_eth_hdr)));
	net_buf_add(pkt->buffer, sizeof(struct net_arp_hdr));

	net_ipv4_addr_copy_raw(arp_hdr->dst_ipaddr, (uint8_t *)&src);
	net_ipv4_addr_copy_raw(arp_hdr->src_ipaddr, (uint8_t *)&dst);

	net_pkt_set_ll_proto_type(pkt, NET_ETH_PTYPE_ARP);

	pkt_arp = prepare_arp_request(iface, pkt, &eth_hwaddr, &eth_hdr);

	/**TESTPOINT: Check if ARP request generation failed*/
	zassert_not_null(pkt_arp, "ARP request generation failed.");

	req_test = true;

	switch (net_arp_input(pkt_arp,
			      (struct net_eth_addr *)net_pkt_lladdr_src(pkt_arp)->addr,
			      &dst_lladdr)) {
	case NET_OK:
	case NET_CONTINUE:
		break;
	case NET_DROP:
		break;
	}

	/* Yielding so that network interface TX thread can proceed. */
	k_yield();

	/**TESTPOINT: Check if ARP request sent*/
	zassert_false(send_status < 0, "ARP req was not sent");

	net_pkt_unref(pkt);

	/**TESTPOINT: Check gratuitous ARP */
	if (IS_ENABLED(CONFIG_NET_ARP_GRATUITOUS)) {
		struct net_eth_addr new_hwaddr = {
			{ 0x11, 0x12, 0x13, 0x14, 0x15, 0x16 }
		};
		enum net_verdict verdict;

		/* First make sure that we have an entry in cache */
		entry_found = false;
		expected_hwaddr = &eth_hwaddr;
		net_arp_foreach(arp_cb, &dst);
		zassert_true(entry_found, "Entry not found");

		pkt = net_pkt_alloc_with_buffer(iface,
						sizeof(struct net_eth_hdr) +
						sizeof(struct net_arp_hdr),
						AF_UNSPEC, 0, K_SECONDS(1));
		zassert_not_null(pkt, "out of mem request");

		setup_eth_header(iface, pkt, net_eth_broadcast_addr(),
				 NET_ETH_PTYPE_ARP);

		eth_hdr = (struct net_eth_hdr *)net_pkt_data(pkt);
		net_buf_add(pkt->buffer, sizeof(struct net_eth_hdr));
		net_buf_pull(pkt->buffer, sizeof(struct net_eth_hdr));
		arp_hdr = NET_ARP_HDR(pkt);

		arp_hdr->hwtype = htons(NET_ARP_HTYPE_ETH);
		arp_hdr->protocol = htons(NET_ETH_PTYPE_IP);
		arp_hdr->hwlen = sizeof(struct net_eth_addr);
		arp_hdr->protolen = sizeof(struct in_addr);
		arp_hdr->opcode = htons(NET_ARP_REQUEST);
		memcpy(&arp_hdr->src_hwaddr, &new_hwaddr, 6);
		memcpy(&arp_hdr->dst_hwaddr, net_eth_broadcast_addr(), 6);
		net_ipv4_addr_copy_raw(arp_hdr->dst_ipaddr, (uint8_t *)&dst);
		net_ipv4_addr_copy_raw(arp_hdr->src_ipaddr, (uint8_t *)&dst);

		net_buf_add(pkt->buffer, sizeof(struct net_arp_hdr));

		net_pkt_set_ll_proto_type(pkt, NET_ETH_PTYPE_ARP);

		verdict = net_arp_input(pkt,
					(struct net_eth_addr *)net_pkt_lladdr_src(pkt)->addr,
					&dst_lladdr);

		zassert_not_equal(verdict, NET_DROP, "Gratuitous ARP failed");

		/* Then check that the HW address is changed for an existing
		 * entry.
		 */
		entry_found = false;
		expected_hwaddr = &new_hwaddr;
		net_arp_foreach(arp_cb, &dst);
		zassert_true(entry_found, "Changed entry not found");

		net_pkt_unref(pkt);
	}
}

ZTEST_SUITE(arp_fn_tests, NULL, NULL, NULL, NULL, NULL);
