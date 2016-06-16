/* main.c - Application main entry point */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <zephyr.h>
#include <sections.h>

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <device.h>
#include <init.h>
#include <misc/printk.h>
#include <net/net_core.h>
#include <net/nbuf.h>
#include <net/net_ip.h>

#include <tc_util.h>

#include "6lo.h"

#define NET_DEBUG 1
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
#define udp_src_port_4bit 0xb1f0
#define udp_dst_port_4bit 0xb2f0

/* 8 bit compressible udp ports */
#define udp_src_port_8bit   0x11f1
#define udp_dst_port_8bit_y 0x22f0

#define udp_src_port_8bit_y 0x11f0
#define udp_dst_port_8bit   0x22f1

/* uncompressible ports */
#define udp_src_port_16bit 0x11ff
#define udp_dst_port_16bit 0x22ff

static const char user_data[] =
		"0123456789012345678901234567890123456789"
		"0123456789012345678901234567890123456789"
		"0123456789012345678901234567890123456789";

struct net_6lo_data {
	struct net_ipv6_hdr ipv6;
	struct net_udp_hdr udp;
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
	uint8_t mac[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xaa, 0xbb};

	net_if_set_link_addr(iface, mac, 8);
}

static int tester_send(struct net_if *iface, struct net_buf *buf)
{
	net_nbuf_unref(buf);
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

static bool compare_data(struct net_buf *buf, struct net_6lo_data *data)
{
	struct net_buf *frag;
	uint8_t bytes, pos, compare, offset = 0;
	int remaining = data->small ? SIZE_OF_SMALL_DATA : SIZE_OF_LARGE_DATA;

	if (net_buf_frags_len(buf->frags) != (NET_IPV6UDPH_LEN + remaining)) {
		printk("mismatch lengths, expected %d received %d\n",
			NET_IPV6UDPH_LEN + remaining,
			net_buf_frags_len(buf->frags));
		return false;
	}

	frag = buf->frags;

	if (memcmp(frag->data, (uint8_t *)data, NET_IPV6UDPH_LEN)) {
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

static struct net_buf *create_buf(struct net_6lo_data *data)
{
	struct net_buf *buf, *frag;
	uint8_t bytes, pos;
	uint16_t len;
	int remaining;

	buf = net_nbuf_get_reserve_tx(0);
	if (!buf) {
		return NULL;
	}

	net_nbuf_set_iface(buf, net_if_get_default());
	net_nbuf_set_ip_hdr_len(buf, NET_IPV6H_LEN);

	frag = net_nbuf_get_reserve_data(0);
	if (!frag) {
		net_nbuf_unref(buf);
		return NULL;
	}

	memcpy(frag->data, (uint8_t *) data, NET_IPV6UDPH_LEN);
	net_buf_add(frag, NET_IPV6UDPH_LEN);

	pos = 0;
	remaining = data->small ? SIZE_OF_SMALL_DATA : SIZE_OF_LARGE_DATA;

	len = NET_UDPH_LEN + remaining;
	/* length is not set in net_6lo_data data pointer, calculate and set
	 * in ipv6, udp and in data pointer too (it's required in comparison) */
	frag->data[4] = len >> 8;
	frag->data[5] = (uint8_t) len;
	frag->data[44] = len >> 8;
	frag->data[4] = (uint8_t) len;

	data->ipv6.len[0] = len >> 8;
	data->ipv6.len[1] = (uint8_t) len;
	data->udp.len = len;

	while (remaining > 0) {
		uint8_t copy;

		bytes = net_buf_tailroom(frag);
		copy = remaining > bytes ? bytes : remaining;
		memcpy(net_buf_add(frag, copy), &user_data[pos], copy);

		pos += bytes;
		remaining -= bytes;

		if (net_buf_tailroom(frag) - (bytes - copy)) {
			net_nbuf_unref(buf);
			return NULL;
		}

		net_buf_frag_add(buf, frag);

		if (remaining > 0) {
			frag = net_nbuf_get_reserve_data(0);
		}
	}

	return buf;
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
	.udp.src_port = udp_src_port_4bit,
	.udp.dst_port = udp_dst_port_4bit,
	.udp.len = 0x00,
	.udp.chksum = 0x00,
	.small = true
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
	.udp.src_port = udp_src_port_8bit_y,
	.udp.dst_port = udp_dst_port_8bit,
	.udp.len = 0x00,
	.udp.chksum = 0x00,
	.small = false
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
	.udp.src_port = udp_src_port_8bit,
	.udp.dst_port = udp_dst_port_8bit_y,
	.udp.len = 0x00,
	.udp.chksum = 0x00,
	.small = true
};

static struct net_6lo_data test_data_4 = {
	.ipv6.vtc = 0x61,
	.ipv6.tcflow = 0x20,
	.ipv6.flow = 0x00,
	.ipv6.len = { 0x00, 0x00 },
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sac1_sam00,
	.ipv6.dst = dst_m1_dam00,
	.udp.src_port = udp_src_port_16bit,
	.udp.dst_port = udp_dst_port_16bit,
	.udp.len = 0x00,
	.udp.chksum = 0x00,
	.small = false
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
	.udp.src_port = udp_src_port_16bit,
	.udp.dst_port = udp_dst_port_16bit,
	.udp.len = 0x00,
	.udp.chksum = 0x00,
	.small = true
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
	.udp.src_port = udp_src_port_8bit,
	.udp.dst_port = udp_dst_port_8bit,
	.udp.len = 0x00,
	.udp.chksum = 0x00,
	.small = true
};

static int test_6lo(struct net_6lo_data *data)
{
	struct net_buf *buf;
	int result = TC_FAIL;

	buf = create_buf(data);
	if (!buf) {
		TC_PRINT("%s: failed to create buffer\n", __func__);
		goto end;
	}

#if DEBUG > 0
	printk("length before compression %d\n", net_buf_frags_len(buf->frags));
	net_hexdump_frags("before-compression", buf);
#endif

	buf = net_6lo_compress(buf);
	if (!buf) {
		TC_PRINT("compression failed\n");
		goto end;
	}

#if DEBUG > 0
	printk("length after compression %d\n", net_buf_frags_len(buf->frags));
	net_hexdump_frags("after-compression", buf);
#endif

	buf = net_6lo_uncompress(buf);
	if (!buf) {
		TC_PRINT("uncompression failed\n");
		goto end;
	}

#if DEBUG > 0
	printk("length after uncompression %d\n",
	       net_buf_frags_len(buf->frags));
	net_hexdump_frags("after-uncompression", buf);
#endif

	if (compare_data(buf, data)) {
		result = TC_PASS;
	}

end:
	net_nbuf_unref(buf);
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
	{ "test_6lo_sac1_sam00_m1_dam00", &test_data_4},
	{ "test_6lo_sam01_m1_dam01", &test_data_5},
	{ "test_6lo_sam10_m1_dam10", &test_data_6},
};

static void main_fiber(void)
{
	int count, pass;

	for (count = 0, pass = 0; count < ARRAY_SIZE(tests); count++) {
		TC_START(tests[count].name);

		if (test_6lo(tests[1].data)) {
			TC_END(FAIL, "failed\n");
		} else {
			TC_END(PASS, "passed\n");
			pass++;
		}
	}

	TC_END_REPORT(((pass != ARRAY_SIZE(tests)) ? TC_FAIL : TC_PASS));
}

#if defined(CONFIG_NANOKERNEL)
#define STACKSIZE 2000
char __noinit __stack fiberStack[STACKSIZE];
#endif

void main(void)
{
#if defined(CONFIG_MICROKERNEL)
	main_fiber();
#else
	task_fiber_start(&fiberStack[0], STACKSIZE,
				(nano_fiber_entry_t)main_fiber, 0, 0, 7, 0);
#endif
}
