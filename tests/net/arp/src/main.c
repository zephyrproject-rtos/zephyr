/* main.c - Application main entry point */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <linker/sections.h>

#include <tc_util.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <device.h>
#include <init.h>
#include <net/net_core.h>
#include <net/net_pkt.h>
#include <net/net_ip.h>
#include <net/arp.h>
#include <ztest.h>

#define NET_LOG_ENABLED 1
#include "net_private.h"

static bool req_test;

static char *app_data = "0123456789";

struct net_arp_context {
	u8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
};

int net_arp_dev_init(struct device *dev)
{
	struct net_arp_context *net_arp_context = dev->driver_data;

	net_arp_context = net_arp_context;

	return 0;
}

static u8_t *net_arp_get_mac(struct device *dev)
{
	struct net_arp_context *context = dev->driver_data;

	if (context->mac_addr[2] == 0x00) {
		/* 00-00-5E-00-53-xx Documentation RFC 7042 */
		context->mac_addr[0] = 0x00;
		context->mac_addr[1] = 0x00;
		context->mac_addr[2] = 0x5E;
		context->mac_addr[3] = 0x00;
		context->mac_addr[4] = 0x53;
		context->mac_addr[5] = sys_rand32_get();
	}

	return context->mac_addr;
}

static void net_arp_iface_init(struct net_if *iface)
{
	u8_t *mac = net_arp_get_mac(net_if_get_device(iface));

	net_if_set_link_addr(iface, mac, 6, NET_LINK_ETHERNET);
}

static struct net_pkt *pending_pkt;

static struct net_eth_addr hwaddr = { { 0x42, 0x11, 0x69, 0xde, 0xfa, 0xec } };

static int send_status = -EINVAL;

static int tester_send(struct net_if *iface, struct net_pkt *pkt)
{
	struct net_eth_hdr *hdr;

	if (!pkt->frags) {
		printk("No data to send!\n");
		return -ENODATA;
	}

	if (net_pkt_ll_reserve(pkt) != sizeof(struct net_eth_hdr)) {
		printk("No ethernet header in pkt %p", pkt);

		send_status = -EINVAL;
		return send_status;
	}

	hdr = (struct net_eth_hdr *)net_pkt_ll(pkt);

	if (ntohs(hdr->type) == NET_ETH_PTYPE_ARP) {
		struct net_arp_hdr *arp_hdr = NET_ARP_HDR(pkt);

		if (ntohs(arp_hdr->opcode) == NET_ARP_REPLY) {
			if (!req_test && pkt != pending_pkt) {
				printk("Pending data but to be sent is wrong, "
				       "expecting %p but got %p\n",
				       pending_pkt, pkt);
				return -EINVAL;
			}

			if (!req_test && memcmp(&hdr->dst, &hwaddr,
						sizeof(struct net_eth_addr))) {
				char out[sizeof("xx:xx:xx:xx:xx:xx")];

				snprintk(out, sizeof(out), "%s",
					 net_sprint_ll_addr(
						 (u8_t *)&hdr->dst,
						 sizeof(struct net_eth_addr)));
				printk("Invalid hwaddr %s, should be %s\n",
				       out,
				       net_sprint_ll_addr(
					       (u8_t *)&hwaddr,
					       sizeof(struct net_eth_addr)));
				send_status = -EINVAL;
				return send_status;
			}

		} else if (ntohs(arp_hdr->opcode) == NET_ARP_REQUEST) {
			if (memcmp(&hdr->src, &hwaddr,
				   sizeof(struct net_eth_addr))) {
				char out[sizeof("xx:xx:xx:xx:xx:xx")];

				snprintk(out, sizeof(out), "%s",
					 net_sprint_ll_addr(
						 (u8_t *)&hdr->src,
						 sizeof(struct net_eth_addr)));
				printk("Invalid hwaddr %s, should be %s\n",
				       out,
				       net_sprint_ll_addr(
					       (u8_t *)&hwaddr,
					       sizeof(struct net_eth_addr)));
				send_status = -EINVAL;
				return send_status;
			}
		}
	}
	net_pkt_unref(pkt);

	send_status = 0;

	return 0;
}

static inline struct in_addr *if_get_addr(struct net_if *iface)
{
	int i;

	for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		if (iface->ipv4.unicast[i].is_used &&
		    iface->ipv4.unicast[i].address.family == AF_INET &&
		    iface->ipv4.unicast[i].addr_state == NET_ADDR_PREFERRED) {
			return &iface->ipv4.unicast[i].address.in_addr;
		}
	}

	return NULL;
}

static inline struct net_pkt *prepare_arp_reply(struct net_if *iface,
						struct net_pkt *req,
						struct net_eth_addr *addr)
{
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct net_arp_hdr *hdr;
	struct net_eth_hdr *eth;

	pkt = net_pkt_get_reserve_tx(sizeof(struct net_eth_hdr), K_FOREVER);
	if (!pkt) {
		goto fail;
	}

	frag = net_pkt_get_frag(pkt, K_FOREVER);
	if (!frag) {
		goto fail;
	}

	net_pkt_frag_add(pkt, frag);
	net_pkt_set_iface(pkt, iface);

	hdr = NET_ARP_HDR(pkt);
	eth = NET_ETH_HDR(pkt);

	eth->type = htons(NET_ETH_PTYPE_ARP);

	memset(&eth->dst.addr, 0xff, sizeof(struct net_eth_addr));
	memcpy(&eth->src.addr, net_if_get_link_addr(iface)->addr,
	       sizeof(struct net_eth_addr));

	hdr->hwtype = htons(NET_ARP_HTYPE_ETH);
	hdr->protocol = htons(NET_ETH_PTYPE_IP);
	hdr->hwlen = sizeof(struct net_eth_addr);
	hdr->protolen = sizeof(struct in_addr);
	hdr->opcode = htons(NET_ARP_REPLY);

	memcpy(&hdr->dst_hwaddr.addr, &eth->src.addr,
	       sizeof(struct net_eth_addr));
	memcpy(&hdr->src_hwaddr.addr, addr,
	       sizeof(struct net_eth_addr));

	net_ipaddr_copy(&hdr->dst_ipaddr, &NET_ARP_HDR(req)->src_ipaddr);
	net_ipaddr_copy(&hdr->src_ipaddr, &NET_ARP_HDR(req)->dst_ipaddr);

	net_buf_add(frag, sizeof(struct net_arp_hdr));

	return pkt;

fail:
	net_pkt_unref(pkt);
	return NULL;
}

static inline struct net_pkt *prepare_arp_request(struct net_if *iface,
						  struct net_pkt *req,
						  struct net_eth_addr *addr)
{
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct net_arp_hdr *hdr, *req_hdr;
	struct net_eth_hdr *eth, *eth_req;

	pkt = net_pkt_get_reserve_rx(sizeof(struct net_eth_hdr),
				      K_FOREVER);
	if (!pkt) {
		goto fail;
	}

	frag = net_pkt_get_frag(pkt, K_FOREVER);
	if (!frag) {
		goto fail;
	}

	net_pkt_frag_add(pkt, frag);
	net_pkt_set_iface(pkt, iface);

	hdr = NET_ARP_HDR(pkt);
	eth = NET_ETH_HDR(pkt);
	req_hdr = NET_ARP_HDR(req);
	eth_req = NET_ETH_HDR(req);

	eth->type = htons(NET_ETH_PTYPE_ARP);

	memset(&eth->dst.addr, 0xff, sizeof(struct net_eth_addr));
	memcpy(&eth->src.addr, addr, sizeof(struct net_eth_addr));

	hdr->hwtype = htons(NET_ARP_HTYPE_ETH);
	hdr->protocol = htons(NET_ETH_PTYPE_IP);
	hdr->hwlen = sizeof(struct net_eth_addr);
	hdr->protolen = sizeof(struct in_addr);
	hdr->opcode = htons(NET_ARP_REQUEST);

	memset(&hdr->dst_hwaddr.addr, 0x00, sizeof(struct net_eth_addr));
	memcpy(&hdr->src_hwaddr.addr, addr, sizeof(struct net_eth_addr));

	net_ipaddr_copy(&hdr->src_ipaddr, &req_hdr->src_ipaddr);
	net_ipaddr_copy(&hdr->dst_ipaddr, &req_hdr->dst_ipaddr);

	net_buf_add(frag, sizeof(struct net_arp_hdr));

	return pkt;

fail:
	net_pkt_unref(pkt);
	return NULL;
}

static void setup_eth_header(struct net_if *iface, struct net_pkt *pkt,
			     struct net_eth_addr *hwaddr, u16_t type)
{
	struct net_eth_hdr *hdr = (struct net_eth_hdr *)net_pkt_ll(pkt);

	memcpy(&hdr->dst.addr, hwaddr, sizeof(struct net_eth_addr));
	memcpy(&hdr->src.addr, net_if_get_link_addr(iface)->addr,
	       sizeof(struct net_eth_addr));

	hdr->type = htons(type);
}

struct net_arp_context net_arp_context_data;

static struct net_if_api net_arp_if_api = {
	.init = net_arp_iface_init,
	.send = tester_send,
};

#if defined(CONFIG_NET_ARP) && defined(CONFIG_NET_L2_ETHERNET)
#define _ETH_L2_LAYER ETHERNET_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(ETHERNET_L2)
#else
#define _ETH_L2_LAYER DUMMY_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(DUMMY_L2)
#endif

NET_DEVICE_INIT(net_arp_test, "net_arp_test",
		net_arp_dev_init, &net_arp_context_data, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&net_arp_if_api, _ETH_L2_LAYER, _ETH_L2_CTX_TYPE, 127);

void run_tests(void)
{
	k_thread_priority_set(k_current_get(), K_PRIO_COOP(7));

	struct net_pkt *pkt, *pkt2;
	struct net_buf *frag;
	struct net_if *iface;
	struct net_if_addr *ifaddr;
	struct net_arp_hdr *arp_hdr;
	struct net_ipv4_hdr *ipv4;
	struct net_eth_hdr *eth_hdr;
	int len;

	struct in_addr dst = { { { 192, 168, 0, 2 } } };
	struct in_addr dst_far = { { { 10, 11, 12, 13 } } };
	struct in_addr dst_far2 = { { { 172, 16, 14, 186 } } };
	struct in_addr src = { { { 192, 168, 0, 1 } } };
	struct in_addr netmask = { { { 255, 255, 255, 0 } } };
	struct in_addr gw = { { { 192, 168, 0, 42 } } };

	net_arp_init();

	iface = net_if_get_default();

	net_if_ipv4_set_gw(iface, &gw);
	net_if_ipv4_set_netmask(iface, &netmask);

	/* Unicast test */
	ifaddr = net_if_ipv4_addr_add(iface,
				      &src,
				      NET_ADDR_MANUAL,
				      0);
	ifaddr->addr_state = NET_ADDR_PREFERRED;

	/* Application data for testing */
	pkt = net_pkt_get_reserve_tx(sizeof(struct net_eth_hdr), K_FOREVER);

	/**TESTPOINTS: Check if out of memory*/
	zassert_not_null(pkt, "Out of mem TX");

	frag = net_pkt_get_frag(pkt, K_FOREVER);

	zassert_not_null(frag, "Out of mem DATA");

	net_pkt_frag_add(pkt, frag);

	net_pkt_set_iface(pkt, iface);

	setup_eth_header(iface, pkt, &hwaddr, NET_ETH_PTYPE_IP);

	len = strlen(app_data);

	if (net_pkt_ll_reserve(pkt) != sizeof(struct net_eth_hdr)) {
		printk("LL reserve invalid, should be %zd was %d\n",
		       sizeof(struct net_eth_hdr),
		       net_pkt_ll_reserve(pkt));
		zassert_true(0, "exiting");
	}

	ipv4 = (struct net_ipv4_hdr *)net_buf_add(frag,
						  sizeof(struct net_ipv4_hdr));
	net_ipaddr_copy(&ipv4->src, &src);
	net_ipaddr_copy(&ipv4->dst, &dst);

	memcpy(net_buf_add(frag, len), app_data, len);

	pkt2 = net_arp_prepare(pkt);

	/* pkt2 is the ARP packet and pkt is the IPv4 packet and it was
	 * stored in ARP table.
	 */

	/**TESTPOINTS: Check packets*/
	zassert_not_equal((void *)(pkt2), (void *)(pkt),
		/* The packets cannot be the same as the ARP cache has
		 * still room for the pkt.
		 */
		"ARP cache should still have free space");

	zassert_not_null(pkt2, "ARP pkt is empty");

	/* The ARP cache should now have a link to pending net_pkt
	 * that is to be sent after we have got an ARP reply.
	 */
	zassert_not_null(pkt->frags,
		"Pending pkt fragment is NULL");

	pending_pkt = pkt;

	/* pkt2 should contain the arp header, verify it */
	if (memcmp(net_pkt_ll(pkt2), net_eth_broadcast_addr(),
		   sizeof(struct net_eth_addr))) {
		printk("ARP ETH dest address invalid\n");
		net_hexdump("ETH dest wrong  ", net_pkt_ll(pkt2),
			    sizeof(struct net_eth_addr));
		net_hexdump("ETH dest correct",
			    (u8_t *)net_eth_broadcast_addr(),
			    sizeof(struct net_eth_addr));
		zassert_true(0, "exiting");
	}

	if (memcmp(net_pkt_ll(pkt2) + sizeof(struct net_eth_addr),
		   iface->link_addr.addr,
		   sizeof(struct net_eth_addr))) {
		printk("ARP ETH source address invalid\n");
		net_hexdump("ETH src correct",
			    iface->link_addr.addr,
			    sizeof(struct net_eth_addr));
		net_hexdump("ETH src wrong  ",
			    net_pkt_ll(pkt2) +	sizeof(struct net_eth_addr),
			    sizeof(struct net_eth_addr));
		zassert_true(0, "exiting");
	}

	arp_hdr = NET_ARP_HDR(pkt2);
	eth_hdr = NET_ETH_HDR(pkt2);

	if (eth_hdr->type != htons(NET_ETH_PTYPE_ARP)) {
		printk("ETH type 0x%x, should be 0x%x\n",
		       eth_hdr->type, htons(NET_ETH_PTYPE_ARP));
		zassert_true(0, "exiting");
	}

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

	if (!net_ipv4_addr_cmp(&arp_hdr->dst_ipaddr,
			       &NET_IPV4_HDR(pkt)->dst)) {
		char out[sizeof("xxx.xxx.xxx.xxx")];

		snprintk(out, sizeof(out), "%s",
			 net_sprint_ipv4_addr(&arp_hdr->dst_ipaddr));
		printk("ARP IP dest invalid %s, should be %s", out,
		       net_sprint_ipv4_addr(&NET_IPV4_HDR(pkt)->dst));
		zassert_true(0, "exiting");
	}

	if (!net_ipv4_addr_cmp(&arp_hdr->src_ipaddr,
			       &NET_IPV4_HDR(pkt)->src)) {
		char out[sizeof("xxx.xxx.xxx.xxx")];

		snprintk(out, sizeof(out), "%s",
			 net_sprint_ipv4_addr(&arp_hdr->src_ipaddr));
		printk("ARP IP src invalid %s, should be %s", out,
		       net_sprint_ipv4_addr(&NET_IPV4_HDR(pkt)->src));
		zassert_true(0, "exiting");
	}

	/* We could have send the new ARP request but for this test we
	 * just free it.
	 */
	net_pkt_unref(pkt2);

	zassert_equal(pkt->ref, 2,
		"ARP cache should own the original packet");

	/* Then a case where target is not in the same subnet */
	net_ipaddr_copy(&ipv4->dst, &dst_far);

	pkt2 = net_arp_prepare(pkt);

	zassert_not_equal((void *)(pkt2), (void *)(pkt),
		"ARP cache should not find anything");

	/**TESTPOINTS: Check if packets not empty*/
	zassert_not_null(pkt2,
		"ARP pkt2 is empty");

	arp_hdr = NET_ARP_HDR(pkt2);

	if (!net_ipv4_addr_cmp(&arp_hdr->dst_ipaddr, &iface->ipv4.gw)) {
		char out[sizeof("xxx.xxx.xxx.xxx")];

		snprintk(out, sizeof(out), "%s",
			 net_sprint_ipv4_addr(&arp_hdr->dst_ipaddr));
		printk("ARP IP dst invalid %s, should be %s\n", out,
			 net_sprint_ipv4_addr(&iface->ipv4.gw));
		zassert_true(0, "exiting");
	}

	net_pkt_unref(pkt2);

	/* Try to find the same destination again, this should fail as there
	 * is a pending request in ARP cache.
	 */
	net_ipaddr_copy(&ipv4->dst, &dst_far);

	/* Make sure prepare will not free the pkt because it will be
	 * needed in the later test case.
	 */
	net_pkt_ref(pkt);

	pkt2 = net_arp_prepare(pkt);

	zassert_not_null(pkt2,
		"ARP cache is not sending the request again");

	net_pkt_unref(pkt2);

	/* Try to find the different destination, this should fail too
	 * as the cache table should be full.
	 */
	net_ipaddr_copy(&ipv4->dst, &dst_far2);

	/* Make sure prepare will not free the pkt because it will be
	 * needed in the next test case.
	 */
	net_pkt_ref(pkt);

	pkt2 = net_arp_prepare(pkt);

	zassert_not_null(pkt2,
		"ARP cache did not send a req");

	/* Restore the original address so that following test case can
	 * work properly.
	 */
	net_ipaddr_copy(&ipv4->dst, &dst);

	/* The arp request packet is now verified, create an arp reply.
	 * The previous value of pkt is stored in arp table and is not lost.
	 */
	pkt = net_pkt_get_reserve_rx(sizeof(struct net_eth_hdr), K_FOREVER);

	zassert_not_null(pkt,
		"Out of mem RX reply");

	printk("%d pkt %p\n", __LINE__, pkt);

	frag = net_pkt_get_frag(pkt, K_FOREVER);

	zassert_not_null(frag,
		"Out of mem DATA reply");

	printk("%d frag %p\n", __LINE__, frag);

	net_pkt_frag_add(pkt, frag);

	net_pkt_set_iface(pkt, iface);

	arp_hdr = NET_ARP_HDR(pkt);
	net_buf_add(frag, sizeof(struct net_arp_hdr));

	net_ipaddr_copy(&arp_hdr->dst_ipaddr, &dst);
	net_ipaddr_copy(&arp_hdr->src_ipaddr, &src);

	pkt2 = prepare_arp_reply(iface, pkt, &hwaddr);

	zassert_not_null(pkt2,
		"ARP reply generation failed.");

	/* The pending packet should now be sent */
	switch (net_arp_input(pkt2)) {
	case NET_OK:
	case NET_CONTINUE:
		break;
	case NET_DROP:
		break;
	}

	/* Yielding so that network interface TX thread can proceed. */
	k_yield();

	/**TESTPOINTS: Check ARP reply*/
	zassert_false(send_status < 0,
		"ARP reply was not sent");

	zassert_equal(pkt->ref, 1,
		"ARP cache should no longer own the original packet");

	net_pkt_unref(pkt);

	/* Then feed in ARP request */
	pkt = net_pkt_get_reserve_rx(sizeof(struct net_eth_hdr), K_FOREVER);

	/**TESTPOINTS: Check if out of memory*/
	zassert_not_null(pkt,
		"Out of mem RX request");

	frag = net_pkt_get_frag(pkt, K_FOREVER);

	zassert_not_null(frag,
		"Out of mem DATA request");

	net_pkt_frag_add(pkt, frag);

	net_pkt_set_iface(pkt, iface);
	send_status = -EINVAL;

	arp_hdr = NET_ARP_HDR(pkt);
	net_buf_add(frag, sizeof(struct net_arp_hdr));

	net_ipaddr_copy(&arp_hdr->dst_ipaddr, &src);
	net_ipaddr_copy(&arp_hdr->src_ipaddr, &dst);
	setup_eth_header(iface, pkt, &hwaddr, NET_ETH_PTYPE_ARP);

	pkt2 = prepare_arp_request(iface, pkt, &hwaddr);

	/**TESTPOINT: Check if ARP request generation failed*/
	zassert_not_null(pkt2,
		"ARP request generation failed.");

	req_test = true;

	switch (net_arp_input(pkt2)) {
	case NET_OK:
	case NET_CONTINUE:
		break;
	case NET_DROP:
		break;
	}

	/* Yielding so that network interface TX thread can proceed. */
	k_yield();

	/**TESTPOINT: Check if ARP request sent*/
	zassert_false(send_status < 0,
		"ARP req was not sent");

	net_pkt_unref(pkt);
}

void test_main(void)
{
	ztest_test_suite(test_arp_fn,
		ztest_unit_test(run_tests));
	ztest_run_test_suite(test_arp_fn);
}
