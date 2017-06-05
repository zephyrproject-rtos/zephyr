/* main.c - Application main entry point */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sections.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <device.h>
#include <init.h>
#include <net/net_core.h>
#include <net/net_pkt.h>
#include <net/net_ip.h>

#include <tc_util.h>

#include "6lo.h"
#include "icmpv6.h"

#define NET_LOG_ENABLED 1
#include "net_private.h"

#define DEBUG 0

#define SIZE_OF_SMALL_DATA 40
#define SIZE_OF_LARGE_DATA 120

/**
  * IPv6 Source and Destination address
  * Example addresses are based on SAC (Source Adddress Compression),
  * SAM (Source Address Mode), DAC (Destination Address Compression),
  * DAM (Destination Address Mode) and also if the destination address
  * is Multicast address.
  */

#define src_sac1_sam00 \
		{ { { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
		      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } } }

#define src_sam00 \
		{ { { 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
		      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } } }

#define src_sam01 \
		{ { { 0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
		      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xaa } } }

#define src_sam10 \
		{ { { 0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
		      0x00, 0x00, 0x00, 0xff, 0xfe, 0x00, 0x00, 0xbb } } }
#define src_sam11 \
		{ { { 0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
		      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xaa, 0xbb } } }

#define dst_m1_dam00 \
		{ { { 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
		      0x00, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 } } }

#define dst_m1_dam01 \
		{ { { 0xff, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
		      0x00, 0x00, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55 } } }

#define dst_m1_dam10 \
		{ { { 0xff, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
		      0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x22, 0x33 } } }

#define dst_m1_dam11 \
		{ { { 0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
		      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11 } } }

#define dst_dam00 \
		{ { { 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
		      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } } }

#define dst_dam01 \
		{ { { 0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
		      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xaa } } }

#define dst_dam10 \
		{ { { 0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
		      0x00, 0x00, 0x00, 0xff, 0xfe, 0x00, 0x00, 0xbb } } }
#define dst_dam11 \
		{ { { 0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
		      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xbb, 0xaa } } }

u8_t src_mac[8] = { 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0xaa, 0xbb };
u8_t dst_mac[8] = { 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0xbb, 0xaa };

/* Source and Destination addresses are contect related addresses. */
#if defined(CONFIG_NET_6LO_CONTEXT)
/* CONFIG_NET_MAX_6LO_CONTEXTS=2, defined in prj.conf, If you want
 * to increase this value, then add extra contexts here.
 */
#define ctx1_prefix \
		{ { { 0xaa, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
		      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } } }

/* 6CO contexts */
static struct net_icmpv6_nd_opt_6co ctx1 = {
	.type = 0x22,
	.len = 0x02,
	.context_len = 0x40,
	.flag = 0x11,
	.reserved = 0,
	.lifetime = 0x1234,
	.prefix = ctx1_prefix
};

#define ctx2_prefix \
		{ { { 0xcc, 0xdd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
		      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } } }

static struct net_icmpv6_nd_opt_6co ctx2 = {
	.type = 0x22,
	.len = 0x03,
	.context_len = 0x80,
	.flag = 0x12,
	.reserved = 0,
	.lifetime = 0x1234,
	.prefix = ctx2_prefix
};

#define src_sac1_sam01 \
		{ { { 0xaa, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
		      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xaa } } }
#define dst_dac1_dam01 \
		{ { { 0xaa, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
		      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xaa } } }
#define src_sac1_sam10 \
		{ { { 0xcc, 0xdd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
		      0x00, 0x00, 0x00, 0xff, 0xfe, 0x00, 0x00, 0xbb } } }
#define dst_dac1_dam10 \
		{ { { 0xcc, 0xdd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
		      0x00, 0x00, 0x00, 0xff, 0xfe, 0x00, 0x00, 0xbb } } }
#define src_sac1_sam11 \
		{ { { 0xaa, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
		      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xaa, 0xbb } } }
#define dst_dac1_dam11 \
		{ { { 0xcc, 0xdd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
		      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xbb, 0xaa } } }
#endif

/* UDP Ports */
/* 4 bit compressible udp ports */
#define udp_src_port_4bit 0xf0b1
#define udp_dst_port_4bit 0xf0b2

/* 8 bit compressible udp ports */
#define udp_src_port_8bit   0xf111
#define udp_dst_port_8bit_y 0xf022 /* compressible */

#define udp_src_port_8bit_y 0xf011 /* compressible */
#define udp_dst_port_8bit   0xf122

/* uncompressible ports */
#define udp_src_port_16bit 0xff11
#define udp_dst_port_16bit 0xff22

static const char user_data[] =
		"0123456789012345678901234567890123456789"
		"0123456789012345678901234567890123456789"
		"0123456789012345678901234567890123456789";

struct net_6lo_data {
	struct net_ipv6_hdr ipv6;

	union {
		struct net_udp_hdr udp;
		struct net_icmp_hdr icmp;
	} nh;

	bool nh_udp;
	bool nh_icmp;
	bool iphc;
	bool small;
} __packed;


int net_6lo_dev_init(struct device *dev)
{
	struct net_6lo_context *net_6lo_context = dev->driver_data;

	net_6lo_context = net_6lo_context;

	return 0;
}

static void net_6lo_iface_init(struct net_if *iface)
{
	net_if_set_link_addr(iface, src_mac, 8, NET_LINK_IEEE802154);
}

static int tester_send(struct net_if *iface, struct net_pkt *pkt)
{
	net_pkt_unref(pkt);
	return NET_OK;
}

static struct net_if_api net_6lo_if_api = {
	.init = net_6lo_iface_init,
	.send = tester_send,
};

NET_DEVICE_INIT(net_6lo_test, "net_6lo_test",
		net_6lo_dev_init, NULL, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&net_6lo_if_api, DUMMY_L2, NET_L2_GET_CTX_TYPE(DUMMY_L2), 127);

static bool compare_data(struct net_pkt *pkt, struct net_6lo_data *data)
{
	struct net_buf *frag;
	u8_t bytes;
	u8_t compare;
	u8_t pos = 0;
	u8_t offset = 0;
	int remaining = data->small ? SIZE_OF_SMALL_DATA : SIZE_OF_LARGE_DATA;

	if (data->nh_udp) {
		if (net_pkt_get_len(pkt) !=
		    (NET_IPV6UDPH_LEN + remaining)) {

			TC_PRINT("mismatch lengths, expected %d received %zu\n",
				 NET_IPV6UDPH_LEN + remaining,
				 net_pkt_get_len(pkt));

			return false;
		}
	} else if (data->nh_icmp) {
		if (net_pkt_get_len(pkt) !=
		    (NET_IPV6ICMPH_LEN + remaining)) {

			TC_PRINT("mismatch lengths, expected %d received %zu\n",
				 NET_IPV6ICMPH_LEN + remaining,
				 net_pkt_get_len(pkt));

			return false;
		}
	} else {
		if (net_pkt_get_len(pkt) !=
		    (NET_IPV6H_LEN + remaining)) {

			TC_PRINT("mismatch lengths, expected %d received %zu\n",
				 NET_IPV6H_LEN + remaining,
				 net_pkt_get_len(pkt));

			return false;
		}
	}

	frag = pkt->frags;

	if (data->nh_udp) {
		if (memcmp(frag->data, (u8_t *)data, NET_IPV6UDPH_LEN)) {
			TC_PRINT("mismatch headers\n");
			return false;
		}
	} else	if (data->nh_icmp) {
		if (memcmp(frag->data, (u8_t *)data, NET_IPV6ICMPH_LEN)) {
			TC_PRINT("mismatch headers\n");
			return false;
		}
	} else {
		if (memcmp(frag->data, (u8_t *)data, NET_IPV6H_LEN)) {
			TC_PRINT("mismatch headers\n");
			return false;
		}
	}

	if (data->nh_udp) {
		offset = NET_IPV6UDPH_LEN;
	} else if (data->nh_icmp) {
		offset = NET_IPV6ICMPH_LEN;
	} else {
		offset = NET_IPV6H_LEN;
	}

	while (remaining > 0 && frag) {

		bytes = frag->len - offset;
		compare = remaining > bytes ? bytes : remaining;

		if (memcmp(frag->data + offset, user_data + pos, compare)) {
			TC_PRINT("data mismatch\n");
			return false;
		}

		pos += compare;
		remaining -= compare;
		frag = frag->frags;
		offset = 0;
	}

	return true;
}

static struct net_pkt *create_pkt(struct net_6lo_data *data)
{
	struct net_pkt *pkt;
	struct net_buf *frag;
	u8_t bytes, pos;
	u16_t len;
	int remaining;

	pkt = net_pkt_get_reserve_tx(0, K_FOREVER);
	if (!pkt) {
		return NULL;
	}

	net_pkt_set_iface(pkt, net_if_get_default());
	net_pkt_set_ip_hdr_len(pkt, NET_IPV6H_LEN);

	net_pkt_ll_src(pkt)->addr = src_mac;
	net_pkt_ll_src(pkt)->len = 8;

	net_pkt_ll_dst(pkt)->addr = dst_mac;
	net_pkt_ll_dst(pkt)->len = 8;

	frag = net_pkt_get_frag(pkt, K_FOREVER);
	if (!frag) {
		net_pkt_unref(pkt);
		return NULL;
	}

	if (data->nh_udp) {
		memcpy(frag->data, (u8_t *) data, NET_IPV6UDPH_LEN);
		net_buf_add(frag, NET_IPV6UDPH_LEN);
	} else if (data->nh_icmp) {
		memcpy(frag->data, (u8_t *) data, NET_IPV6ICMPH_LEN);
		net_buf_add(frag, NET_IPV6ICMPH_LEN);

	} else {
		memcpy(frag->data, (u8_t *) data, NET_IPV6H_LEN);
		net_buf_add(frag, NET_IPV6H_LEN);
	}

	pos = 0;
	remaining = data->small ? SIZE_OF_SMALL_DATA : SIZE_OF_LARGE_DATA;

	if (data->nh_udp) {
		len = NET_UDPH_LEN + remaining;
	} else if (data->nh_icmp) {
		len = NET_ICMPH_LEN + remaining;
	} else {
		len = remaining;
	}

	/* length is not set in net_6lo_data data pointer, calculate and set
	 * in ipv6, udp and in data pointer too (it's required in comparison) */
	frag->data[4] = len >> 8;
	frag->data[5] = (u8_t) len;

	data->ipv6.len[0] = len >> 8;
	data->ipv6.len[1] = (u8_t) len;

	if (data->nh_udp) {
		frag->data[44] = len >> 8;
		frag->data[45] = (u8_t) len;

		data->nh.udp.len = htons(len);
	}

	while (remaining > 0) {
		u8_t copy;

		bytes = net_buf_tailroom(frag);
		copy = remaining > bytes ? bytes : remaining;
		memcpy(net_buf_add(frag, copy), &user_data[pos], copy);

		pos += bytes;
		remaining -= bytes;

		if (net_buf_tailroom(frag) - (bytes - copy)) {
			net_pkt_unref(pkt);
			return NULL;
		}

		net_pkt_frag_add(pkt, frag);

		if (remaining > 0) {
			frag = net_pkt_get_frag(pkt, K_FOREVER);
		}
	}

	return pkt;
}

static struct net_6lo_data test_data_1 = {
	.ipv6.vtc = 0x60,
	.ipv6.tcflow = 0x00,
	.ipv6.flow = 0x00,
	.ipv6.len = { 0x00, 0x00 },
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sam00,
	.ipv6.dst = dst_dam00,
	.nh.udp.src_port = htons(udp_src_port_4bit),
	.nh.udp.dst_port = htons(udp_dst_port_4bit),
	.nh.udp.len = 0x00,
	.nh.udp.chksum = 0x00,
	.nh_udp = true,
	.nh_icmp = false,
	.small = true,
	.iphc = true
};

static struct net_6lo_data test_data_2 = {
	.ipv6.vtc = 0x60,
	.ipv6.tcflow = 0x20,
	.ipv6.flow = 0x3412,
	.ipv6.len = { 0x00, 0x00 },
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sam01,
	.ipv6.dst = dst_dam01,
	.nh.udp.src_port = htons(udp_src_port_8bit_y),
	.nh.udp.dst_port = htons(udp_dst_port_8bit),
	.nh.udp.len = 0x00,
	.nh.udp.chksum = 0x00,
	.nh_udp = true,
	.nh_icmp = false,
	.small = false,
	.iphc = true
};

static struct net_6lo_data test_data_3 = {
	.ipv6.vtc = 0x60,
	.ipv6.tcflow = 0x21,
	.ipv6.flow = 0x3412,
	.ipv6.len = { 0x00, 0x00 },
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sam10,
	.ipv6.dst = dst_dam10,
	.nh.udp.src_port = htons(udp_src_port_8bit),
	.nh.udp.dst_port = htons(udp_dst_port_8bit_y),
	.nh.udp.len = 0x00,
	.nh.udp.chksum = 0x00,
	.nh_udp = true,
	.nh_icmp = false,
	.small = true,
	.iphc = true
};

static struct net_6lo_data test_data_4 = {
	.ipv6.vtc = 0x61,
	.ipv6.tcflow = 0x20,
	.ipv6.flow = 0x00,
	.ipv6.len = { 0x00, 0x00 },
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sam00,
	.ipv6.dst = dst_m1_dam00,
	.nh.udp.src_port = htons(udp_src_port_16bit),
	.nh.udp.dst_port = htons(udp_dst_port_16bit),
	.nh.udp.len = 0x00,
	.nh.udp.chksum = 0x00,
	.nh_udp = true,
	.nh_icmp = false,
	.small = false,
	.iphc = true
};

static struct net_6lo_data test_data_5 = {
	.ipv6.vtc = 0x61,
	.ipv6.tcflow = 0x23,
	.ipv6.flow = 0x4567,
	.ipv6.len = { 0x00, 0x00 },
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sam01,
	.ipv6.dst = dst_m1_dam01,
	.nh.udp.src_port = htons(udp_src_port_16bit),
	.nh.udp.dst_port = htons(udp_dst_port_16bit),
	.nh.udp.len = 0x00,
	.nh.udp.chksum = 0x00,
	.nh_udp = true,
	.nh_icmp = false,
	.small = true,
	.iphc = true
};

static struct net_6lo_data test_data_6 = {
	.ipv6.vtc = 0x60,
	.ipv6.tcflow = 0x0,
	.ipv6.flow = 0x0,
	.ipv6.len = { 0x00, 0x00 },
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sam10,
	.ipv6.dst = dst_m1_dam10,
	.nh.udp.src_port = htons(udp_src_port_8bit),
	.nh.udp.dst_port = htons(udp_dst_port_8bit),
	.nh.udp.len = 0x00,
	.nh.udp.chksum = 0x00,
	.nh_udp = true,
	.nh_icmp = false,
	.small = true,
	.iphc = true
};

static struct net_6lo_data test_data_7 = {
	.ipv6.vtc = 0x60,
	.ipv6.tcflow = 0x0,
	.ipv6.flow = 0x0,
	.ipv6.len = { 0x00, 0x00 },
	.ipv6.nexthdr = 0,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sam10,
	.ipv6.dst = dst_m1_dam10,
	.nh_udp = false,
	.nh_icmp = false,
	.small = true,
	.iphc = true
};

static struct net_6lo_data test_data_8 = {
	.ipv6.vtc = 0x60,
	.ipv6.tcflow = 0x0,
	.ipv6.flow = 0x0,
	.ipv6.len = { 0x00, 0x00 },
	.ipv6.nexthdr = IPPROTO_ICMPV6,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sam10,
	.ipv6.dst = dst_m1_dam10,
	.nh.icmp.type = NET_ICMPV6_ECHO_REQUEST,
	.nh.icmp.code = 0,
	.nh.icmp.chksum = 0,
	.nh_udp = false,
	.nh_icmp = true,
	.small = true,
	.iphc = true
};

static struct net_6lo_data test_data_9 = {
	.ipv6.vtc = 0x61,
	.ipv6.tcflow = 0x20,
	.ipv6.flow = 0x00,
	.ipv6.len = { 0x00, 0x00 },
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sac1_sam00,
	.ipv6.dst = dst_m1_dam00,
	.nh.udp.src_port = htons(udp_src_port_16bit),
	.nh.udp.dst_port = htons(udp_dst_port_16bit),
	.nh.udp.len = 0x00,
	.nh.udp.chksum = 0x00,
	.nh_udp = true,
	.nh_icmp = false,
	.small = true,
	.iphc = false
};

static struct net_6lo_data test_data_10 = {
	.ipv6.vtc = 0x61,
	.ipv6.tcflow = 0x20,
	.ipv6.flow = 0x00,
	.ipv6.len = { 0x00, 0x00 },
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sac1_sam00,
	.ipv6.dst = dst_m1_dam00,
	.nh.udp.src_port = htons(udp_src_port_16bit),
	.nh.udp.dst_port = htons(udp_dst_port_16bit),
	.nh.udp.len = 0x00,
	.nh.udp.chksum = 0x00,
	.nh_udp = false,
	.nh_icmp = false,
	.small = false,
	.iphc = false
};

static struct net_6lo_data test_data_11 = {
	.ipv6.vtc = 0x61,
	.ipv6.tcflow = 0x20,
	.ipv6.flow = 0x00,
	.ipv6.len = { 0x00, 0x00 },
	.ipv6.nexthdr = 0,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sac1_sam00,
	.ipv6.dst = dst_m1_dam00,
	.nh_udp = false,
	.nh_icmp = false,
	.small = false,
	.iphc = false
};

static struct net_6lo_data test_data_12 = {
	.ipv6.vtc = 0x61,
	.ipv6.tcflow = 0x20,
	.ipv6.flow = 0x00,
	.ipv6.len = { 0x00, 0x00 },
	.ipv6.nexthdr = IPPROTO_ICMPV6,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sac1_sam00,
	.ipv6.dst = dst_m1_dam00,
	.nh.icmp.type = NET_ICMPV6_ECHO_REQUEST,
	.nh.icmp.code = 0,
	.nh.icmp.chksum = 0,
	.nh_udp = false,
	.nh_icmp = true,
	.small = false,
	.iphc = false
};

static struct net_6lo_data test_data_13 = {
	.ipv6.vtc = 0x60,
	.ipv6.tcflow = 0x21,
	.ipv6.flow = 0x3412,
	.ipv6.len = { 0x00, 0x00 },
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sam11,
	.ipv6.dst = dst_dam11,
	.nh.udp.src_port = htons(udp_src_port_8bit),
	.nh.udp.dst_port = htons(udp_dst_port_8bit_y),
	.nh.udp.len = 0x00,
	.nh.udp.chksum = 0x00,
	.nh_udp = true,
	.nh_icmp = false,
	.small = true,
	.iphc = true
};

#if defined(CONFIG_NET_6LO_CONTEXT)
static struct net_6lo_data test_data_14 = {
	.ipv6.vtc = 0x60,
	.ipv6.tcflow = 0x20,
	.ipv6.flow = 0x3412,
	.ipv6.len = { 0x00, 0x00 },
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sac1_sam01,
	.ipv6.dst = dst_dac1_dam01,
	.nh.udp.src_port = htons(udp_src_port_8bit_y),
	.nh.udp.dst_port = htons(udp_dst_port_8bit),
	.nh.udp.len = 0x00,
	.nh.udp.chksum = 0x00,
	.nh_udp = true,
	.nh_icmp = false,
	.small = false,
	.iphc = true
};

static struct net_6lo_data test_data_15 = {
	.ipv6.vtc = 0x60,
	.ipv6.tcflow = 0x21,
	.ipv6.flow = 0x3412,
	.ipv6.len = { 0x00, 0x00 },
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sac1_sam10,
	.ipv6.dst = dst_dac1_dam10,
	.nh.udp.src_port = htons(udp_src_port_8bit),
	.nh.udp.dst_port = htons(udp_dst_port_8bit_y),
	.nh.udp.len = 0x00,
	.nh.udp.chksum = 0x00,
	.nh_udp = true,
	.nh_icmp = false,
	.small = true,
	.iphc = true
};

static struct net_6lo_data test_data_16 = {
	.ipv6.vtc = 0x60,
	.ipv6.tcflow = 0x21,
	.ipv6.flow = 0x3412,
	.ipv6.len = { 0x00, 0x00 },
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sac1_sam11,
	.ipv6.dst = dst_dac1_dam11,
	.nh.udp.src_port = htons(udp_src_port_8bit),
	.nh.udp.dst_port = htons(udp_dst_port_8bit_y),
	.nh.udp.len = 0x00,
	.nh.udp.chksum = 0x00,
	.nh_udp = true,
	.nh_icmp = false,
	.small = true,
	.iphc = true
};

static struct net_6lo_data test_data_17 = {
	.ipv6.vtc = 0x60,
	.ipv6.tcflow = 0x20,
	.ipv6.flow = 0x3412,
	.ipv6.len = { 0x00, 0x00 },
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sam01,
	.ipv6.dst = dst_dac1_dam01,
	.nh.udp.src_port = htons(udp_src_port_8bit_y),
	.nh.udp.dst_port = htons(udp_dst_port_8bit),
	.nh.udp.len = 0x00,
	.nh.udp.chksum = 0x00,
	.nh_udp = true,
	.nh_icmp = false,
	.small = false,
	.iphc = true
};

static struct net_6lo_data test_data_18 = {
	.ipv6.vtc = 0x60,
	.ipv6.tcflow = 0x20,
	.ipv6.flow = 0x3412,
	.ipv6.len = { 0x00, 0x00 },
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sac1_sam01,
	.ipv6.dst = dst_dam01,
	.nh.udp.src_port = htons(udp_src_port_8bit_y),
	.nh.udp.dst_port = htons(udp_dst_port_8bit),
	.nh.udp.len = 0x00,
	.nh.udp.chksum = 0x00,
	.nh_udp = true,
	.nh_icmp = false,
	.small = false,
	.iphc = true
};

static struct net_6lo_data test_data_19 = {
	.ipv6.vtc = 0x61,
	.ipv6.tcflow = 0x23,
	.ipv6.flow = 0x4567,
	.ipv6.len = { 0x00, 0x00 },
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sac1_sam01,
	.ipv6.dst = dst_m1_dam00,
	.nh.udp.src_port = htons(udp_src_port_16bit),
	.nh.udp.dst_port = htons(udp_dst_port_16bit),
	.nh.udp.len = 0x00,
	.nh.udp.chksum = 0x00,
	.nh_udp = true,
	.nh_icmp = false,
	.small = true,
	.iphc = true
};
static struct net_6lo_data test_data_20 = {
	.ipv6.vtc = 0x61,
	.ipv6.tcflow = 0x23,
	.ipv6.flow = 0x4567,
	.ipv6.len = { 0x00, 0x00 },
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sac1_sam01,
	.ipv6.dst = dst_m1_dam01,
	.nh.udp.src_port = htons(udp_src_port_16bit),
	.nh.udp.dst_port = htons(udp_dst_port_16bit),
	.nh.udp.len = 0x00,
	.nh.udp.chksum = 0x00,
	.nh_udp = true,
	.nh_icmp = false,
	.small = true,
	.iphc = true
};

static struct net_6lo_data test_data_21 = {
	.ipv6.vtc = 0x60,
	.ipv6.tcflow = 0x0,
	.ipv6.flow = 0x0,
	.ipv6.len = { 0x00, 0x00 },
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sac1_sam10,
	.ipv6.dst = dst_m1_dam10,
	.nh.udp.src_port = htons(udp_src_port_8bit),
	.nh.udp.dst_port = htons(udp_dst_port_8bit),
	.nh.udp.len = 0x00,
	.nh.udp.chksum = 0x00,
	.nh_udp = true,
	.nh_icmp = false,
	.small = true,
	.iphc = true
};

static struct net_6lo_data test_data_22 = {
	.ipv6.vtc = 0x60,
	.ipv6.tcflow = 0x0,
	.ipv6.flow = 0x0,
	.ipv6.len = { 0x00, 0x00 },
	.ipv6.nexthdr = 0,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sac1_sam11,
	.ipv6.dst = dst_m1_dam10,
	.nh_udp = false,
	.nh_icmp = false,
	.small = true,
	.iphc = true
};

static struct net_6lo_data test_data_23 = {
	.ipv6.vtc = 0x60,
	.ipv6.tcflow = 0x20,
	.ipv6.flow = 0x3412,
	.ipv6.len = { 0x00, 0x00 },
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sam00,
	.ipv6.dst = dst_dac1_dam01,
	.nh.udp.src_port = htons(udp_src_port_8bit_y),
	.nh.udp.dst_port = htons(udp_dst_port_8bit),
	.nh.udp.len = 0x00,
	.nh.udp.chksum = 0x00,
	.nh_udp = true,
	.nh_icmp = false,
	.small = false,
	.iphc = true
};
#endif

static int test_6lo(struct net_6lo_data *data)
{
	struct net_pkt *pkt;
	int result = TC_FAIL;

	pkt = create_pkt(data);
	if (!pkt) {
		TC_PRINT("%s: failed to create buffer\n", __func__);
		goto end;
	}

#if DEBUG > 0
	TC_PRINT("length before compression %zu\n",
		 net_pkt_get_len(pkt));
	net_hexdump_frags("before-compression", pkt);
#endif

	if (!net_6lo_compress(pkt, data->iphc, NULL)) {
		TC_PRINT("compression failed\n");
		goto end;
	}

#if DEBUG > 0
	TC_PRINT("length after compression %zu\n",
		 net_pkt_get_len(pkt));
	net_hexdump_frags("after-compression", pkt);
#endif

	if (!net_6lo_uncompress(pkt)) {
		TC_PRINT("uncompression failed\n");
		goto end;
	}

#if DEBUG > 0
	TC_PRINT("length after uncompression %zu\n",
	       net_pkt_get_len(pkt));
	net_hexdump_frags("after-uncompression", pkt);
#endif

	if (compare_data(pkt, data)) {
		result = TC_PASS;
	}

end:
	net_pkt_unref(pkt);
	return result;
}

/* tests names are based on traffic class, flow label, source address mode
 * (sam), destination address mode (dam), based on udp source and destination
 * ports compressible type.
 */
static const struct {
	const char *name;
	struct net_6lo_data *data;
} tests[] = {
	{ "test_6lo_sam00_dam00", &test_data_1},
	{ "test_6lo_sam01_dam01", &test_data_2},
	{ "test_6lo_sam10_dam10", &test_data_3},
	{ "test_6lo_sam00_m1_dam00", &test_data_4},
	{ "test_6lo_sam01_m1_dam01", &test_data_5},
	{ "test_6lo_sam10_m1_dam10", &test_data_6},
	{ "test_6lo_sam10_m1_dam10_no_udp", &test_data_7},
	{ "test_6lo_sam10_m1_dam10_iphc", &test_data_8},
	{ "test_6lo_ipv6_dispatch_small", &test_data_9},
	{ "test_6lo_ipv6_dispatch_big", &test_data_10},
	{ "test_6lo_ipv6_dispatch_big_no_udp", &test_data_11},
	{ "test_6lo_ipv6_dispatch_big_iphc", &test_data_12},
	{ "test_6lo_sam11_dam11", &test_data_13},
#if defined(CONFIG_NET_6LO_CONTEXT)
	{ "test_6lo_sac1_sam01_dac1_dam01", &test_data_14},
	{ "test_6lo_sac1_sam10_dac1_dam10", &test_data_15},
	{ "test_6lo_sac1_sam11_dac1_dam11", &test_data_16},
	{ "test_6lo_sac0_sam01_dac1_dam01", &test_data_17},
	{ "test_6lo_sac1_sam01_dac0_dam01", &test_data_18},
	{ "test_6lo_sac1_sam01_m1_dam00", &test_data_19},
	{ "test_6lo_sac1_sam01_m1_dam01", &test_data_20},
	{ "test_6lo_sac1_sam10_m1_dam10", &test_data_21},
	{ "test_6lo_sac1_sam11_m1_dam10", &test_data_22},
	{ "test_6lo_sac0_sam00_dac1_dam01", &test_data_23},
#endif
};

static void main_thread(void)
{
	int count, pass;

#if defined(CONFIG_NET_6LO_CONTEXT)
	net_6lo_set_context(net_if_get_default(), &ctx1);
	net_6lo_set_context(net_if_get_default(), &ctx2);
#endif

	for (count = 0, pass = 0; count < ARRAY_SIZE(tests); count++) {
		TC_START(tests[count].name);

		if (test_6lo(tests[count].data)) {
			TC_END(FAIL, "failed\n");
		} else {
			TC_END(PASS, "passed\n");
			pass++;
		}
	}

	net_pkt_print();

	TC_END_REPORT(((pass != ARRAY_SIZE(tests)) ? TC_FAIL : TC_PASS));
}

#define STACKSIZE 2000
char __noinit __stack thread_stack[STACKSIZE];
static struct k_thread thread_data;

void main(void)
{
	k_thread_create(&thread_data, thread_stack, STACKSIZE,
			(k_thread_entry_t)main_thread,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, 0);
}
