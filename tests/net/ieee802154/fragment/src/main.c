/* main.c - Application main entry point */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <linker/sections.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <device.h>
#include <init.h>
#include <misc/printk.h>
#include <net/net_core.h>
#include <net/net_pkt.h>
#include <net/net_ip.h>

#include <tc_util.h>

#include "6lo.h"
#include "ieee802154_fragment.h"

#define NET_LOG_ENABLED 1
#include "net_private.h"

#define DEBUG 0

/**
  * IPv6 Source and Destination address
  * Example addresses are based on SAC (Source Address Compression),
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
		"0123456789012345678901234567890123456789"
		"0123456789012345678901234567890123456789"
		"0123456789012345678901234567890123456789"
		"0123456789012345678901234567890123456789"
		"0123456789012345678901234567890123456789"
		"0123456789012345678901234567890123456789"
		"0123456789012345678901234567890123456789"
		"0123456789012345678901234567890123456789"
		"0123456789012345678901234567890123456789"
		"0123456789012345678901234567890123456789"
		"0123456789012345678901234567890123456789"
		"0123456789012345678901234567890123456789"
		"0123456789012345678901234567890123456789"
		"0123456789012345678901234567890123456789"
		"0123456789012345678901234567890123456789"
		"0123456789012345678901234567890123456789"
		"0123456789012345678901234567890123456789"
		"0123456789012345678901234567890123456789"
		"0123456789012345678901234567890123456789"
		"0123456789012345678901234567890123456789"
		"0123456789012345678901234567890123456789"
		"0123456789012345678901234567890123456789"
		"0123456789012345678901234567890123456789"
		"0123456789012345678901234567890123456789"
		"0123456789012345678901234567890123456789"
		"0123456789012345678901234567890123456789"
		"0123456789012345678901234567890123456789";

struct net_fragment_data {
	struct net_ipv6_hdr ipv6;
	struct net_udp_hdr udp;
	int len;
	bool iphc;
} __packed;


int net_fragment_dev_init(struct device *dev)
{
	struct net_fragment_context *net_fragment_context = dev->driver_data;

	net_fragment_context = net_fragment_context;

	return 0;
}

static void net_fragment_iface_init(struct net_if *iface)
{
	u8_t mac[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xaa, 0xbb};

	net_if_set_link_addr(iface, mac, 8, NET_LINK_IEEE802154);
}

static int tester_send(struct net_if *iface, struct net_pkt *pkt)
{
	net_pkt_unref(pkt);
	return NET_OK;
}

static struct net_if_api net_fragment_if_api = {
	.init = net_fragment_iface_init,
	.send = tester_send,
};

NET_DEVICE_INIT(net_fragment_test, "net_fragment_test",
		net_fragment_dev_init, NULL, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&net_fragment_if_api, DUMMY_L2,
		NET_L2_GET_CTX_TYPE(DUMMY_L2), 127);

static bool compare_data(struct net_pkt *pkt, struct net_fragment_data *data)
{
	struct net_buf *frag;
	u8_t bytes, pos, compare, offset = 0;
	int remaining = data->len;

	if (net_pkt_get_len(pkt) != (NET_IPV6UDPH_LEN + remaining)) {
		printk("mismatch lengths, expected %d received %zd\n",
			NET_IPV6UDPH_LEN + remaining,
			net_pkt_get_len(pkt));
		return false;
	}

	frag = pkt->frags;

	if (memcmp(frag->data, (u8_t *)data, NET_IPV6UDPH_LEN)) {
		printk("mismatch headers\n");
		return false;
	}

	pos = 0;
	offset = NET_IPV6UDPH_LEN;

	while (remaining > 0 && frag) {

		bytes = frag->len - offset;
		compare = remaining > bytes ? bytes : remaining;

		if (memcmp(frag->data + offset, user_data + pos, compare)) {
			printk("data mismatch\n");
			return false;
		}

		pos += compare;
		remaining -= compare;
		frag = frag->frags;
		offset = 0;
	}

	return true;
}

static struct net_pkt *create_pkt(struct net_fragment_data *data)
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

	net_pkt_set_ll_reserve(pkt, 0);
	net_pkt_set_iface(pkt, net_if_get_default());
	net_pkt_set_ip_hdr_len(pkt, NET_IPV6H_LEN);

	frag = net_pkt_get_frag(pkt, K_FOREVER);
	if (!frag) {
		net_pkt_unref(pkt);
		return NULL;
	}

	memcpy(frag->data, (u8_t *) data, NET_IPV6UDPH_LEN);
	net_buf_add(frag, NET_IPV6UDPH_LEN);

	pos = 0;
	remaining = data->len;

	len = NET_UDPH_LEN + remaining;
	/* length is not set in net_fragment_data data pointer, calculate and set
	 * in ipv6, udp and in data pointer too (it's required in comparison) */
	frag->data[4] = len >> 8;
	frag->data[5] = (u8_t) len;
	frag->data[44] = len >> 8;
	frag->data[45] = (u8_t) len;

	data->ipv6.len = htons(len);
	data->udp.len = htons(len);

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

static struct net_fragment_data test_data_1 = {
	.ipv6.vtc = 0x60,
	.ipv6.tcflow = 0x00,
	.ipv6.flow = 0x00,
	.ipv6.len = 0,
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sam00,
	.ipv6.dst = dst_dam00,
	.udp.src_port = htons(udp_src_port_4bit),
	.udp.dst_port = htons(udp_dst_port_4bit),
	.udp.len = 0x00,
	.udp.chksum = 0x00,
	.len = 70,
	.iphc = true
};

static struct net_fragment_data test_data_2 = {
	.ipv6.vtc = 0x60,
	.ipv6.tcflow = 0x20,
	.ipv6.flow = 0x3412,
	.ipv6.len = 0,
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sam01,
	.ipv6.dst = dst_dam01,
	.udp.src_port = htons(udp_src_port_8bit_y),
	.udp.dst_port = htons(udp_dst_port_8bit),
	.udp.len = 0x00,
	.udp.chksum = 0x00,
	.len = 200,
	.iphc = true
};

static struct net_fragment_data test_data_3 = {
	.ipv6.vtc = 0x60,
	.ipv6.tcflow = 0x21,
	.ipv6.flow = 0x3412,
	.ipv6.len = 0,
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sam10,
	.ipv6.dst = dst_dam10,
	.udp.src_port = htons(udp_src_port_8bit),
	.udp.dst_port = htons(udp_dst_port_8bit_y),
	.udp.len = 0x00,
	.udp.chksum = 0x00,
	.len = 300,
	.iphc = true
};

static struct net_fragment_data test_data_4 = {
	.ipv6.vtc = 0x61,
	.ipv6.tcflow = 0x20,
	.ipv6.flow = 0x00,
	.ipv6.len = 0,
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sam00,
	.ipv6.dst = dst_m1_dam00,
	.udp.src_port = htons(udp_src_port_16bit),
	.udp.dst_port = htons(udp_dst_port_16bit),
	.udp.len = 0x00,
	.udp.chksum = 0x00,
	.len = 400,
	.iphc = true
};

static struct net_fragment_data test_data_5 = {
	.ipv6.vtc = 0x61,
	.ipv6.tcflow = 0x23,
	.ipv6.flow = 0x4567,
	.ipv6.len = 0,
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sam01,
	.ipv6.dst = dst_m1_dam01,
	.udp.src_port = htons(udp_src_port_16bit),
	.udp.dst_port = htons(udp_dst_port_16bit),
	.udp.len = 0x00,
	.udp.chksum = 0x00,
	.len = 500,
	.iphc = true
};

static struct net_fragment_data test_data_6 = {
	.ipv6.vtc = 0x60,
	.ipv6.tcflow = 0x0,
	.ipv6.flow = 0x0,
	.ipv6.len = 0,
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sam10,
	.ipv6.dst = dst_m1_dam10,
	.udp.src_port = htons(udp_src_port_8bit),
	.udp.dst_port = htons(udp_dst_port_8bit),
	.udp.len = 0x00,
	.udp.chksum = 0x00,
	.len = 1200,
	.iphc = true
};

static struct net_fragment_data test_data_7 = {
	.ipv6.vtc = 0x61,
	.ipv6.tcflow = 0x20,
	.ipv6.flow = 0x00,
	.ipv6.len = 0,
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sac1_sam00,
	.ipv6.dst = dst_m1_dam00,
	.udp.src_port = htons(udp_src_port_16bit),
	.udp.dst_port = htons(udp_dst_port_16bit),
	.udp.len = 0x00,
	.udp.chksum = 0x00,
	.len = 70,
	.iphc = false
};

static struct net_fragment_data test_data_8 = {
	.ipv6.vtc = 0x61,
	.ipv6.tcflow = 0x20,
	.ipv6.flow = 0x00,
	.ipv6.len = 0,
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sac1_sam00,
	.ipv6.dst = dst_m1_dam00,
	.udp.src_port = htons(udp_src_port_16bit),
	.udp.dst_port = htons(udp_dst_port_16bit),
	.udp.len = 0x00,
	.udp.chksum = 0x00,
	.len = 1200,
	.iphc = false
};

static int test_fragment(struct net_fragment_data *data)
{
	struct net_pkt *rxpkt = NULL;
	int result = TC_FAIL;
	struct net_pkt *pkt;
	struct net_buf *frag, *dfrag;

	pkt = create_pkt(data);
	if (!pkt) {
		TC_PRINT("%s: failed to create buffer\n", __func__);
		goto end;
	}

#if DEBUG > 0
	printk("length before compression %zd\n", net_pkt_get_len(pkt));
	net_hexdump_frags("before-compression", pkt, false);
#endif

	if (!net_6lo_compress(pkt, data->iphc,
			      ieee802154_fragment)) {
		TC_PRINT("compression failed\n");
		goto end;
	}

#if DEBUG > 0
	printk("length after compression and fragmentation %zd\n",
	       net_pkt_get_len(pkt));
	net_hexdump_frags("after-compression", pkt, false);
#endif

	frag = pkt->frags;

	while (frag) {
		rxpkt = net_pkt_get_reserve_rx(0, K_FOREVER);
		if (!rxpkt) {
			goto end;
		}

		net_pkt_set_ll_reserve(rxpkt, 0);

		dfrag = net_pkt_get_frag(rxpkt, K_FOREVER);
		if (!dfrag) {
			goto end;
		}

		memcpy(dfrag->data, frag->data, frag->len);
		dfrag->len = frag->len;

		net_pkt_frag_add(rxpkt, dfrag);

		switch (ieee802154_reassemble(rxpkt)) {
		case NET_OK:
			frag = frag->frags;
			break;
		case NET_CONTINUE:
			goto compare;
		case NET_DROP:
			net_pkt_unref(rxpkt);
			goto end;
		}
	}

compare:
#if DEBUG > 0
	printk("length after reassembly and uncompression %zd\n",
	       net_pkt_get_len(rxpkt));
	net_hexdump_frags("after-uncompression", rxpkt, false);
#endif

	if (compare_data(rxpkt, data)) {
		result = TC_PASS;
	}

end:
	net_pkt_unref(rxpkt);
	net_pkt_unref(pkt);

	return result;
}

/* tests names are based on traffic class, flow label, source address mode
 * (sam), destination address mode (dam), based on udp source and destination
 * ports compressible type.
 */
static const struct {
	const char *name;
	struct net_fragment_data *data;
} tests[] = {
	{ "test_fragment_sam00_dam00", &test_data_1},
	{ "test_fragment_sam01_dam01", &test_data_2},
	{ "test_fragment_sam10_dam10", &test_data_3},
	{ "test_fragment_sam00_m1_dam00", &test_data_4},
	{ "test_fragment_sam01_m1_dam01", &test_data_5},
	{ "test_fragment_sam10_m1_dam10", &test_data_6},
	{ "test_fragment_ipv6_dispatch_small", &test_data_7},
	{ "test_fragment_ipv6_dispatch_big", &test_data_8},
};

void main(void)
{
	int count, pass;
	k_thread_priority_set(k_current_get(), K_PRIO_COOP(7));

	for (count = 0, pass = 0; count < ARRAY_SIZE(tests); count++) {
		TC_START(tests[count].name);

		if (test_fragment(tests[count].data)) {
			TC_END(FAIL, "failed\n");
		} else {
			TC_END(PASS, "passed\n");
			pass++;
		}
	}

	TC_END_REPORT(((pass != ARRAY_SIZE(tests)) ? TC_FAIL : TC_PASS));
}
