/*
 * Copyright (c) 2026 Philipp Steiner <philipp.steiner1987@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <zephyr/net/ethernet.h>
#include <zephyr/net/ethernet_vlan.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/udp.h>
#include <zephyr/sys/byteorder.h>

#include "msg.h"

#define PTP_EVENT_PORT 319U
#define TEST_VLAN_TAG  100U

BUILD_ASSERT(CONFIG_NET_BUF_DATA_SIZE >=
	     (NET_IPV4H_LEN + NET_UDPH_LEN + sizeof(struct ptp_sync_msg)));

static void fake_eth_iface_init(struct net_if *iface)
{
	static uint8_t mac_addr[] = { 0x00, 0x00, 0x5e, 0x00, 0x53, 0x01 };

	net_if_set_link_addr(iface, mac_addr, sizeof(mac_addr), NET_LINK_ETHERNET);
}

static int fake_eth_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static enum ethernet_hw_caps fake_eth_caps(const struct device *dev,
					   struct net_if *iface)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(iface);

	return ETHERNET_HW_VLAN;
}

static int fake_eth_send(const struct device *dev, struct net_pkt *pkt)
{
	ARG_UNUSED(dev);

	net_pkt_unref(pkt);

	return 0;
}

static const struct ethernet_api fake_eth_api = {
	.iface_api.init = fake_eth_iface_init,
	.get_capabilities = fake_eth_caps,
	.send = fake_eth_send,
};

ETH_NET_DEVICE_INIT(fake_eth, "fake_eth", fake_eth_init, NULL, NULL, NULL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &fake_eth_api,
		    NET_ETH_MTU);

static void add_fragment(struct net_pkt *pkt, const void *data, size_t len)
{
	struct net_buf *frag;

	frag = net_pkt_get_frag(pkt, len, K_NO_WAIT);
	zassert_not_null(frag, "failed to allocate packet fragment");

	net_buf_add_mem(frag, data, len);
	net_pkt_frag_add(pkt, frag);
}

static void add_eth_header_fragment(struct net_pkt *pkt, uint16_t vlan_tci)
{
	uint8_t hdr[sizeof(struct net_eth_vlan_hdr)] = { 0 };
	size_t hdr_len;

	if (vlan_tci == NET_VLAN_TAG_UNSPEC) {
		hdr_len = sizeof(struct net_eth_hdr);
		sys_put_be16(NET_ETH_PTYPE_IP, &hdr[12]);
	} else {
		hdr_len = sizeof(struct net_eth_vlan_hdr);
		sys_put_be16(NET_ETH_PTYPE_VLAN, &hdr[12]);
		sys_put_be16(vlan_tci, &hdr[14]);
		sys_put_be16(NET_ETH_PTYPE_IP, &hdr[16]);
	}

	add_fragment(pkt, hdr, hdr_len);
}

static void add_ipv4_udp_ptp_sync_fragment(struct net_pkt *pkt)
{
	const size_t ptp_len = sizeof(struct ptp_sync_msg);
	const size_t udp_len = NET_UDPH_LEN + ptp_len;
	const size_t frag_len = NET_IPV4H_LEN + udp_len;
	struct ptp_sync_msg *sync;
	struct net_udp_hdr *udp;
	struct net_buf *frag;
	uint8_t *data;

	frag = net_pkt_get_frag(pkt, frag_len, K_NO_WAIT);
	zassert_not_null(frag, "failed to allocate IPv4/UDP/PTP fragment");

	data = net_buf_add(frag, frag_len);
	memset(data, 0, frag_len);

	udp = (struct net_udp_hdr *)(data + NET_IPV4H_LEN);
	udp->src_port = net_htons(PTP_EVENT_PORT);
	udp->dst_port = net_htons(PTP_EVENT_PORT);
	udp->len = net_htons(udp_len);

	sync = (struct ptp_sync_msg *)(data + NET_IPV4H_LEN + NET_UDPH_LEN);
	sync->hdr.type_major_sdo_id = PTP_MSG_SYNC;
	sync->hdr.version = 2;
	sync->hdr.msg_length = net_htons(ptp_len);
	sync->hdr.sequence_id = net_htons(1);

	net_pkt_frag_add(pkt, frag);
}

static struct net_pkt *build_udp_ptp_pkt(uint16_t vlan_tci)
{
	struct net_if *iface;
	struct net_pkt *pkt;

	iface = net_if_get_default();
	zassert_not_null(iface, "no default network interface");

	pkt = net_pkt_rx_alloc_on_iface(iface, K_NO_WAIT);
	zassert_not_null(pkt, "failed to allocate packet");

	net_pkt_set_family(pkt, NET_AF_INET);
	net_pkt_set_ip_hdr_len(pkt, NET_IPV4H_LEN);
	net_pkt_set_vlan_tci(pkt, vlan_tci);

	add_eth_header_fragment(pkt, vlan_tci);
	add_ipv4_udp_ptp_sync_fragment(pkt);

	return pkt;
}

ZTEST(ptp_msg_parse, test_untagged_udp_ptp_packet_with_vlan_enabled)
{
	struct net_pkt *pkt;
	struct ptp_msg *msg;

	pkt = build_udp_ptp_pkt(NET_VLAN_TAG_UNSPEC);
	msg = ptp_msg_from_pkt(pkt);

	zassert_not_null(msg, "untagged UDP PTP packet was not parsed");
	zassert_equal(ptp_msg_type(msg), PTP_MSG_SYNC);
	zassert_equal(net_ntohs(msg->header.msg_length), sizeof(struct ptp_sync_msg));

	net_pkt_unref(pkt);
}

ZTEST_SUITE(ptp_msg_parse, NULL, NULL, NULL, NULL, NULL);
