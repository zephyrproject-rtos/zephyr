/* main.c - Application main entry point */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_L2_IEEE802154_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/linker/sections.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/printk.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/dummy.h>

#include <zephyr/tc_util.h>

#include "6lo.h"
#include "ieee802154_6lo_fragment.h"

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
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }

#define src_sam00 \
		{ 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }

#define src_sam01 \
		{ 0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xaa }

#define src_sam10 \
		{ 0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
		  0x00, 0x00, 0x00, 0xff, 0xfe, 0x00, 0x00, 0xbb }

#define dst_m1_dam00 \
		{ 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
		  0x00, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 }

#define dst_m1_dam01 \
		{ 0xff, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
		  0x00, 0x00, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55 }

#define dst_m1_dam10 \
		{ 0xff, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x22, 0x33 }

#define dst_m1_dam11 \
		{ 0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11 }

#define dst_dam00 \
		{ 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }

#define dst_dam01 \
		{ 0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xaa }

#define dst_dam10 \
		{ 0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
		  0x00, 0x00, 0x00, 0xff, 0xfe, 0x00, 0x00, 0xbb }


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


int net_fragment_dev_init(const struct device *dev)
{
	return 0;
}

static void net_fragment_iface_init(struct net_if *iface)
{
	static uint8_t mac[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xaa, 0xbb};

	net_if_set_link_addr(iface, mac, 8, NET_LINK_IEEE802154);
}

static int tester_send(const struct device *dev, struct net_pkt *pkt)
{
	return 0;
}

static struct dummy_api net_fragment_if_api = {
	.iface_api.init = net_fragment_iface_init,
	.send = tester_send,
};

NET_DEVICE_INIT(net_fragment_test, "net_fragment_test",
		net_fragment_dev_init, NULL, NULL, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&net_fragment_if_api, DUMMY_L2,
		NET_L2_GET_CTX_TYPE(DUMMY_L2), 127);

static bool compare_data(struct net_pkt *pkt, struct net_fragment_data *data)
{
	int remaining = data->len;
	uint32_t bytes, pos, compare, offset;
	struct net_buf *frag;

	if (net_pkt_get_len(pkt) != (NET_IPV6UDPH_LEN + remaining)) {
		printk("mismatch lengths, expected %d received %zd\n",
			NET_IPV6UDPH_LEN + remaining,
			net_pkt_get_len(pkt));
		return false;
	}

	frag = pkt->frags;

	if (memcmp(frag->data, (uint8_t *)data, NET_IPV6UDPH_LEN)) {
		printk("mismatch headers\n");
		return false;
	}

	pos = 0U;
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
		offset = 0U;
	}

	return true;
}

static struct net_pkt *create_pkt(struct net_fragment_data *data)
{
	static uint16_t dummy_short_addr;
	struct net_pkt *pkt;
	struct net_buf *buf;
	uint32_t bytes, pos;
	uint16_t len;
	int remaining;

	pkt = net_pkt_alloc_on_iface(
		net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY)), K_FOREVER);
	if (!pkt) {
		return NULL;
	}

	net_pkt_set_ip_hdr_len(pkt, NET_IPV6H_LEN);

	buf = net_pkt_get_frag(pkt, NET_IPV6UDPH_LEN, K_FOREVER);
	if (!buf) {
		net_pkt_unref(pkt);
		return NULL;
	}

	memcpy(buf->data, (uint8_t *) data, NET_IPV6UDPH_LEN);
	net_buf_add(buf, NET_IPV6UDPH_LEN);

	pos = 0U;
	remaining = data->len;

	len = NET_UDPH_LEN + remaining;
	/* length is not set in net_fragment_data data pointer, calculate and set
	 * in ipv6, udp and in data pointer too (it's required in comparison) */
	buf->data[4] = len >> 8;
	buf->data[5] = (uint8_t) len;
	buf->data[44] = len >> 8;
	buf->data[45] = (uint8_t) len;

	data->ipv6.len = htons(len);
	data->udp.len = htons(len);

	while (remaining > 0) {
		uint8_t copy;
		bytes = net_buf_tailroom(buf);
		copy = remaining > bytes ? bytes : remaining;
		memcpy(net_buf_add(buf, copy), &user_data[pos], copy);

		pos += bytes;
		remaining -= bytes;

		if (net_buf_tailroom(buf) - (bytes - copy)) {
			net_pkt_unref(pkt);
			return NULL;
		}

		net_pkt_frag_add(pkt, buf);

		if (remaining > 0) {
			buf = net_pkt_get_frag(pkt, CONFIG_NET_BUF_DATA_SIZE, K_FOREVER);
		}
	}

	/* Setup link layer addresses. */
	net_pkt_lladdr_dst(pkt)->addr = (uint8_t *)&dummy_short_addr;
	net_pkt_lladdr_dst(pkt)->len = sizeof(dummy_short_addr);
	net_pkt_lladdr_dst(pkt)->type = NET_LINK_IEEE802154;

	memcpy(net_pkt_lladdr_src(pkt),
	       net_if_get_link_addr(
		       net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY))),
	       sizeof(struct net_linkaddr));

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

static uint8_t frame_buffer_data[IEEE802154_MTU];

static struct net_buf frame_buf = {
	.data = frame_buffer_data,
	.size = IEEE802154_MTU,
	.frags = NULL,
	.__buf = frame_buffer_data,
};

static bool test_fragment(struct net_fragment_data *data)
{
	struct net_pkt *rxpkt = NULL;
	struct net_pkt *f_pkt = NULL;
	int result = false;
	struct ieee802154_6lo_fragment_ctx ctx;
	struct net_buf *buf, *dfrag;
	struct net_pkt *pkt;
	int hdr_diff;

	pkt = create_pkt(data);
	if (!pkt) {
		TC_PRINT("%s: failed to create buffer\n", __func__);
		goto end;
	}

#if DEBUG > 0
	printk("length before compression %zd\n", net_pkt_get_len(pkt));
	net_pkt_hexdump(pkt, "before-compression");
#endif

	hdr_diff = net_6lo_compress(pkt, data->iphc);
	if (hdr_diff < 0) {
		TC_PRINT("compression failed\n");
		goto end;
	}

	if (!ieee802154_6lo_requires_fragmentation(pkt, 0)) {
		f_pkt = pkt;
		pkt = NULL;

		goto reassemble;
	}

	f_pkt = net_pkt_alloc(K_FOREVER);
	if (!f_pkt) {
		goto end;
	}

	ieee802154_6lo_fragment_ctx_init(&ctx, pkt, hdr_diff, data->iphc);
	frame_buf.len = 0U;

	buf = pkt->buffer;
	while (buf) {
		buf = ieee802154_6lo_fragment(&ctx, &frame_buf, data->iphc);

		dfrag = net_pkt_get_frag(f_pkt, frame_buf.len, K_FOREVER);
		if (!dfrag) {
			goto end;
		}

		memcpy(dfrag->data, frame_buf.data, frame_buf.len);
		dfrag->len = frame_buf.len;

		net_pkt_frag_add(f_pkt, dfrag);

		frame_buf.len = 0U;
	}

	net_pkt_unref(pkt);
	pkt = NULL;

reassemble:

#if DEBUG > 0
	printk("length after compression and fragmentation %zd\n",
	       net_pkt_get_len(f_pkt));
	net_pkt_hexdump(f_pkt, "after-compression");
#endif

	buf = f_pkt->buffer;
	while (buf) {
		rxpkt = net_pkt_rx_alloc(K_FOREVER);
		if (!rxpkt) {
			goto end;
		}

		dfrag = net_pkt_get_frag(rxpkt, buf->len, K_FOREVER);
		if (!dfrag) {
			goto end;
		}

		memcpy(dfrag->data, buf->data, buf->len);
		dfrag->len = buf->len;

		net_pkt_frag_add(rxpkt, dfrag);

		net_pkt_set_overwrite(rxpkt, true);

		switch (ieee802154_6lo_reassemble(rxpkt)) {
		case NET_OK:
			buf = buf->frags;
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
	net_pkt_hexdump(rxpkt, "after-uncompression");
#endif

	if (compare_data(rxpkt, data)) {
		result = true;
	}

end:
	if (pkt) {
		net_pkt_unref(pkt);
	}

	if (f_pkt) {
		net_pkt_unref(f_pkt);
	}

	if (rxpkt) {
		net_pkt_unref(rxpkt);
	}

	return result;
}

ZTEST(ieee802154_6lo_fragment, test_fragment_sam00_dam00)
{
	bool ret = test_fragment(&test_data_1);

	zassert_true(ret);
}

ZTEST(ieee802154_6lo_fragment, test_fragment_sam01_dam01)
{
	bool ret = test_fragment(&test_data_2);

	zassert_true(ret);
}

ZTEST(ieee802154_6lo_fragment, test_fragment_sam10_dam10)
{
	bool ret = test_fragment(&test_data_3);

	zassert_true(ret);
}

ZTEST(ieee802154_6lo_fragment, test_fragment_sam00_m1_dam00)
{
	bool ret = test_fragment(&test_data_4);

	zassert_true(ret);
}

ZTEST(ieee802154_6lo_fragment, test_fragment_sam01_m1_dam01)
{
	bool ret = test_fragment(&test_data_5);

	zassert_true(ret);
}

ZTEST(ieee802154_6lo_fragment, test_fragment_sam10_m1_dam10)
{
	bool ret = test_fragment(&test_data_6);

	zassert_true(ret);
}

ZTEST(ieee802154_6lo_fragment, test_fragment_ipv6_dispatch_small)
{
	bool ret = test_fragment(&test_data_7);

	zassert_true(ret);
}

ZTEST(ieee802154_6lo_fragment, test_fragment_ipv6_dispatch_big)
{
	bool ret = test_fragment(&test_data_8);

	zassert_true(ret);
}


ZTEST_SUITE(ieee802154_6lo_fragment, NULL, NULL, NULL, NULL, NULL);
