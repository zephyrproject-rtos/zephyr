/* main.c - Application main entry point */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_6LO_LOG_LEVEL);

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
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/dummy.h>

#include <zephyr/tc_util.h>

#include "6lo.h"
#include "icmpv6.h"

#define NET_LOG_ENABLED 1
#include "net_private.h"

#define DEBUG 0

#define SIZE_OF_SMALL_DATA 40
#define SIZE_OF_LARGE_DATA 120

 /* IPv6 Source and Destination address
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
#define src_sam11 \
		{ 0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xaa, 0xbb }

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
#define dst_dam11 \
		{ 0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xbb, 0xaa }

uint8_t src_mac[8] = { 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0xaa, 0xbb };
uint8_t dst_mac[8] = { 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0xbb, 0xaa };

/* Source and Destination addresses are context related addresses. */
#if defined(CONFIG_NET_6LO_CONTEXT)
/* CONFIG_NET_MAX_6LO_CONTEXTS=2, defined in prj.conf, If you want
 * to increase this value, then add extra contexts here.
 */
#define ctx1_prefix \
		{ 0xaa, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }

/* 6CO contexts */
static struct net_icmpv6_nd_opt_6co ctx1 = {
	.context_len = 0x40,
	.flag = 0x11,
	.reserved = 0,
	.lifetime = 0x1234,
	.prefix = ctx1_prefix
};

#define ctx2_prefix \
		{ 0xcc, 0xdd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }

static struct net_icmpv6_nd_opt_6co ctx2 = {
	.context_len = 0x80,
	.flag = 0x12,
	.reserved = 0,
	.lifetime = 0x1234,
	.prefix = ctx2_prefix
};

#define src_sac1_sam01 \
		{ 0xaa, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xaa }
#define dst_dac1_dam01 \
		{ 0xaa, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xaa }
#define src_sac1_sam10 \
		{ 0xcc, 0xdd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
		  0x00, 0x00, 0x00, 0xff, 0xfe, 0x00, 0x00, 0xbb }
#define dst_dac1_dam10 \
		{ 0xcc, 0xdd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
		  0x00, 0x00, 0x00, 0xff, 0xfe, 0x00, 0x00, 0xbb }
#define src_sac1_sam11 \
		{ 0xaa, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xaa, 0xbb }
#define dst_dac1_dam11 \
		{ 0xcc, 0xdd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xbb, 0xaa }
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

/* Inlinded data size */
#define TF_00 4
#define TF_01 3
#define TF_10 1
#define TF_11 0

#define CID_0 0
#define CID_1 1
#define NHC_0 1
#define NHC_1 0

#define HLIM_1 0
#define HLOM_0 1

#define SAC0_SAM00     16
#define SAC0_SAM01     8
#define SAC0_SAM10     2
#define SAC0_SAM11     0
#define SAC1_SAM00     0
#define SAC1_SAM01     8
#define SAC1_SAM10     2
#define SAC1_SAM11     0

#define M0_DAC0_DAM00  16
#define M0_DAC0_DAM01  8
#define M0_DAC0_DAM10  2
#define M0_DAC0_DAM11  0
#define M0_DAC1_DAM01  8
#define M0_DAC1_DAM10  2
#define M0_DAC1_DAM11  0
#define M1_DAC0_DAM00  16
#define M1_DAC0_DAM01  6
#define M1_DAC0_DAM10  4
#define M1_DAC0_DAM11  1
#define M1_DAC1_DAM00  6

#define UDP_CHKSUM_0   2
#define UDP_CHKSUM_1   0

#define UDP_P00        4
#define UDP_P01        3
#define UDP_P10        3
#define UDP_P11        1


#define IPHC_SIZE      2
#define NHC_SIZE       1

#define IPV6_DISPATCH_DIFF -1

static const char user_data[] =
		"0123456789012345678901234567890123456789"
		"0123456789012345678901234567890123456789"
		"0123456789012345678901234567890123456789";

#if defined(CONFIG_NET_BUF_FIXED_DATA_SIZE)
#define TEST_FRAG_LEN CONFIG_NET_BUF_DATA_SIZE
#else
#define TEST_FRAG_LEN 128
#endif /* CONFIG_NET_BUF_FIXED_DATA_SIZE */

struct user_data_small {
	char data[SIZE_OF_SMALL_DATA];
};

struct user_data_large {
	char data[SIZE_OF_LARGE_DATA];
};


struct net_6lo_data {
	struct net_ipv6_hdr ipv6;

	union {
		struct net_udp_hdr udp;
		struct net_icmp_hdr icmp;
	} nh;

	int hdr_diff;

	bool nh_udp;
	bool nh_icmp;
	bool iphc;
	bool small;
} __packed;


int net_6lo_dev_init(const struct device *dev)
{
	struct net_6lo_context *net_6lo_context = dev->data;

	net_6lo_context = net_6lo_context;

	return 0;
}

static void net_6lo_iface_init(struct net_if *iface)
{
	net_if_set_link_addr(iface, src_mac, 8, NET_LINK_IEEE802154);
}

static int tester_send(const struct device *dev, struct net_pkt *pkt)
{
	return 0;
}

static struct dummy_api net_6lo_if_api = {
	.iface_api.init = net_6lo_iface_init,
	.send = tester_send,
};

NET_DEVICE_INIT(net_6lo_test, "net_6lo_test",
		net_6lo_dev_init, NULL, NULL, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&net_6lo_if_api, DUMMY_L2, NET_L2_GET_CTX_TYPE(DUMMY_L2), 127);

static bool compare_ipv6_hdr(struct net_pkt *pkt, struct net_6lo_data *data)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(ipv6_access, struct net_ipv6_hdr);
	struct net_ipv6_hdr *ipv6_hdr = net_pkt_get_data(pkt, &ipv6_access);
	int res;

	if (!ipv6_hdr) {
		TC_PRINT("Failed to read IPv6 HDR\n");
		return false;
	}

	net_pkt_acknowledge_data(pkt, &ipv6_access);

	res = memcmp((uint8_t *)ipv6_hdr, (uint8_t *)&data->ipv6,
		      sizeof(struct net_ipv6_hdr));
	if (res) {
		TC_PRINT("Mismatch IPv6 HDR\n");
		return false;
	}

	return true;
}

static bool compare_udp_hdr(struct net_pkt *pkt, struct net_6lo_data *data)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(udp_access, struct net_udp_hdr);
	struct net_udp_hdr *udp_hdr = net_pkt_get_data(pkt, &udp_access);
	int res;

	if (!udp_hdr) {
		TC_PRINT("Failed to read UDP HDR\n");
		return false;
	}

	net_pkt_acknowledge_data(pkt, &udp_access);

	res = memcmp((uint8_t *)udp_hdr, (uint8_t *)&data->nh.udp,
		      sizeof(struct net_udp_hdr));
	if (res) {
		TC_PRINT("Mismatch UDP HDR\n");
		return false;
	}

	return true;
}

static bool compare_icmp_hdr(struct net_pkt *pkt, struct net_6lo_data *data)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(icmp_access, struct net_icmp_hdr);
	struct net_icmp_hdr *icmp_hdr = net_pkt_get_data(pkt, &icmp_access);
	int res;

	if (!icmp_hdr) {
		TC_PRINT("Failed to read ICMP HDR\n");
		return false;
	}

	net_pkt_acknowledge_data(pkt, &icmp_access);

	res = memcmp((uint8_t *)icmp_hdr, (uint8_t *)&data->nh.icmp,
		      sizeof(struct net_icmp_hdr));
	if (res) {
		TC_PRINT("Mismatch ICMP HDR\n");
		return false;
	}

	return true;
}

static bool compare_data_small(struct net_pkt *pkt, const char *data)
{
	NET_PKT_DATA_ACCESS_DEFINE(data_access, struct user_data_small);
	struct user_data_small *test_data = net_pkt_get_data(pkt, &data_access);
	int res;

	if (!test_data) {
		TC_PRINT("Failed to read user data\n");
		return false;
	}

	net_pkt_acknowledge_data(pkt, &data_access);

	res = memcmp(test_data->data, data, sizeof(struct user_data_small));
	if (res) {
		TC_PRINT("User data mismatch\n");
		return false;
	}

	return true;
}

static bool compare_data_large(struct net_pkt *pkt, const char *data)
{
	NET_PKT_DATA_ACCESS_DEFINE(data_access, struct user_data_large);
	struct user_data_large *test_data = net_pkt_get_data(pkt, &data_access);
	int res;

	if (!test_data) {
		TC_PRINT("Failed to read user data\n");
		return false;
	}

	net_pkt_acknowledge_data(pkt, &data_access);

	res = memcmp(test_data->data, data, sizeof(struct user_data_large));
	if (res) {
		TC_PRINT("User data mismatch\n");
		return false;
	}

	return true;
}

static bool compare_pkt(struct net_pkt *pkt, struct net_6lo_data *data)
{
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

	net_pkt_set_overwrite(pkt, true);

	if (!compare_ipv6_hdr(pkt, data)) {
		return false;
	}

	if (data->nh_udp) {
		if (!compare_udp_hdr(pkt, data)) {
			return false;
		}
	} else	if (data->nh_icmp) {
		if (!compare_icmp_hdr(pkt, data)) {
			return false;
		}
	}

	if (data->small) {
		if (!compare_data_small(pkt, user_data)) {
			return false;
		}
	} else {
		if (!compare_data_large(pkt, user_data)) {
			return false;
		}
	}

	return true;
}

static struct net_pkt *create_pkt(struct net_6lo_data *data)
{
	struct net_pkt *pkt;
	struct net_buf *frag;
	uint8_t bytes, pos;
	uint16_t len;
	int remaining;

	pkt = net_pkt_alloc_on_iface(
		net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY)), K_FOREVER);
	if (!pkt) {
		return NULL;
	}

	net_pkt_set_ip_hdr_len(pkt, NET_IPV6H_LEN);

	net_pkt_lladdr_src(pkt)->addr = src_mac;
	net_pkt_lladdr_src(pkt)->len = 8U;

	net_pkt_lladdr_dst(pkt)->addr = dst_mac;
	net_pkt_lladdr_dst(pkt)->len = 8U;

	frag = net_pkt_get_frag(pkt, NET_IPV6UDPH_LEN, K_FOREVER);
	if (!frag) {
		net_pkt_unref(pkt);
		return NULL;
	}

	if (data->nh_udp) {
		memcpy(frag->data, (uint8_t *) data, NET_IPV6UDPH_LEN);
		net_buf_add(frag, NET_IPV6UDPH_LEN);
	} else if (data->nh_icmp) {
		memcpy(frag->data, (uint8_t *) data, NET_IPV6ICMPH_LEN);
		net_buf_add(frag, NET_IPV6ICMPH_LEN);

	} else {
		memcpy(frag->data, (uint8_t *) data, NET_IPV6H_LEN);
		net_buf_add(frag, NET_IPV6H_LEN);
	}

	pos = 0U;
	remaining = data->small ? SIZE_OF_SMALL_DATA : SIZE_OF_LARGE_DATA;

	if (data->nh_udp) {
		len = NET_UDPH_LEN + remaining;
	} else if (data->nh_icmp) {
		len = NET_ICMPH_LEN + remaining;
	} else {
		len = remaining;
	}

	/* length is not set in net_6lo_data data pointer, calculate and set
	 * in ipv6, udp and in data pointer too (it's required in comparison)
	 */
	frag->data[4] = len >> 8;
	frag->data[5] = (uint8_t) len;

	data->ipv6.len = htons(len);

	if (data->nh_udp) {
		frag->data[44] = len >> 8;
		frag->data[45] = (uint8_t) len;

		data->nh.udp.len = htons(len);
	}

	while (remaining > 0) {
		uint8_t copy;

		bytes = net_buf_tailroom(frag);
		copy = remaining > bytes ? bytes : remaining;
		memcpy(net_buf_add(frag, copy), &user_data[pos], copy);

		pos += copy;
		remaining -= copy;

		if (net_buf_tailroom(frag) - (bytes - copy)) {
			net_pkt_unref(pkt);
			return NULL;
		}

		net_pkt_frag_add(pkt, frag);

		if (remaining > 0) {
			frag = net_pkt_get_frag(pkt, TEST_FRAG_LEN, K_FOREVER);
		}
	}

	return pkt;
}

static struct net_6lo_data test_data_1 = {
	.ipv6.vtc = 0x60,
	.ipv6.tcflow = 0x00,
	.ipv6.flow = 0x00,
	.ipv6.len = 0,
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sam00,
	.ipv6.dst = dst_dam00,
	.nh.udp.src_port = htons(udp_src_port_4bit),
	.nh.udp.dst_port = htons(udp_dst_port_4bit),
	.nh.udp.len = 0x00,
	.nh.udp.chksum = 0x00,
	.hdr_diff = NET_IPV6UDPH_LEN - IPHC_SIZE - NHC_SIZE -
		    (TF_11 + NHC_1 + CID_0 + SAC0_SAM00 + M0_DAC0_DAM00) -
		    UDP_CHKSUM_0 - UDP_P11,
	.nh_udp = true,
	.nh_icmp = false,
	.small = true,
	.iphc = true
};

static struct net_6lo_data test_data_2 = {
	.ipv6.vtc = 0x60,
	.ipv6.tcflow = 0x20,
	.ipv6.flow = 0x3412,
	.ipv6.len = 0,
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sam01,
	.ipv6.dst = dst_dam01,
	.nh.udp.src_port = htons(udp_src_port_8bit_y),
	.nh.udp.dst_port = htons(udp_dst_port_8bit),
	.nh.udp.len = 0x00,
	.nh.udp.chksum = 0x00,
	.hdr_diff = NET_IPV6UDPH_LEN - IPHC_SIZE - NHC_SIZE -
		    (TF_01 + NHC_1 + CID_0 + SAC0_SAM01 + M0_DAC0_DAM01) -
		    UDP_CHKSUM_0 - UDP_P10,
	.nh_udp = true,
	.nh_icmp = false,
	.small = false,
	.iphc = true
};

static struct net_6lo_data test_data_3 = {
	.ipv6.vtc = 0x60,
	.ipv6.tcflow = 0x21,
	.ipv6.flow = 0x3412,
	.ipv6.len = 0,
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sam10,
	.ipv6.dst = dst_dam10,
	.nh.udp.src_port = htons(udp_src_port_8bit),
	.nh.udp.dst_port = htons(udp_dst_port_8bit_y),
	.nh.udp.len = 0x00,
	.nh.udp.chksum = 0x00,
	.hdr_diff = NET_IPV6UDPH_LEN - IPHC_SIZE - NHC_SIZE -
		    (TF_01 + NHC_1 + CID_0 + SAC0_SAM10 + M0_DAC0_DAM10) -
		    UDP_CHKSUM_0 - UDP_P01,
	.nh_udp = true,
	.nh_icmp = false,
	.small = true,
	.iphc = true
};

static struct net_6lo_data test_data_4 = {
	.ipv6.vtc = 0x61,
	.ipv6.tcflow = 0x20,
	.ipv6.flow = 0x00,
	.ipv6.len = 0,
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sam00,
	.ipv6.dst = dst_m1_dam00,
	.nh.udp.src_port = htons(udp_src_port_16bit),
	.nh.udp.dst_port = htons(udp_dst_port_16bit),
	.nh.udp.len = 0x00,
	.nh.udp.chksum = 0x00,
	.hdr_diff = NET_IPV6UDPH_LEN - IPHC_SIZE - NHC_SIZE -
		    (TF_10 + NHC_1 + CID_0 + SAC0_SAM00 + M1_DAC0_DAM00) -
		    UDP_CHKSUM_0 - UDP_P00,
	.nh_udp = true,
	.nh_icmp = false,
	.small = false,
	.iphc = true
};

static struct net_6lo_data test_data_5 = {
	.ipv6.vtc = 0x61,
	.ipv6.tcflow = 0x23,
	.ipv6.flow = 0x4567,
	.ipv6.len = 0,
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sam01,
	.ipv6.dst = dst_m1_dam01,
	.nh.udp.src_port = htons(udp_src_port_16bit),
	.nh.udp.dst_port = htons(udp_dst_port_16bit),
	.nh.udp.len = 0x00,
	.nh.udp.chksum = 0x00,
	.hdr_diff = NET_IPV6UDPH_LEN - IPHC_SIZE - NHC_SIZE -
		    (TF_00 + NHC_1 + CID_0 + SAC0_SAM01 + M1_DAC0_DAM01) -
		    UDP_CHKSUM_0 - UDP_P00,
	.nh_udp = true,
	.nh_icmp = false,
	.small = true,
	.iphc = true
};

static struct net_6lo_data test_data_6 = {
	.ipv6.vtc = 0x60,
	.ipv6.tcflow = 0x0,
	.ipv6.flow = 0x0,
	.ipv6.len = 0,
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sam10,
	.ipv6.dst = dst_m1_dam10,
	.nh.udp.src_port = htons(udp_src_port_8bit),
	.nh.udp.dst_port = htons(udp_dst_port_8bit),
	.nh.udp.len = 0x00,
	.nh.udp.chksum = 0x00,
	.hdr_diff = NET_IPV6UDPH_LEN - IPHC_SIZE - NHC_SIZE -
		    (TF_11 + NHC_1 + CID_0 + SAC0_SAM10 + M1_DAC0_DAM10) -
		    UDP_CHKSUM_0 - UDP_P00,
	.nh_udp = true,
	.nh_icmp = false,
	.small = true,
	.iphc = true
};

static struct net_6lo_data test_data_7 = {
	.ipv6.vtc = 0x60,
	.ipv6.tcflow = 0x0,
	.ipv6.flow = 0x0,
	.ipv6.len = 0,
	.ipv6.nexthdr = 0,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sam10,
	.ipv6.dst = dst_m1_dam10,
	.hdr_diff = NET_IPV6H_LEN - IPHC_SIZE -
		    (TF_11 + NHC_0 + CID_0 + SAC0_SAM10 + M1_DAC0_DAM10),
	.nh_udp = false,
	.nh_icmp = false,
	.small = true,
	.iphc = true
};

static struct net_6lo_data test_data_8 = {
	.ipv6.vtc = 0x60,
	.ipv6.tcflow = 0x0,
	.ipv6.flow = 0x0,
	.ipv6.len = 0,
	.ipv6.nexthdr = IPPROTO_ICMPV6,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sam10,
	.ipv6.dst = dst_m1_dam10,
	.nh.icmp.type = NET_ICMPV6_ECHO_REQUEST,
	.nh.icmp.code = 0,
	.nh.icmp.chksum = 0,
	.hdr_diff = NET_IPV6H_LEN - IPHC_SIZE -
		    (TF_11 + NHC_0 + CID_0 + SAC0_SAM10 + M1_DAC0_DAM10),
	.nh_udp = false,
	.nh_icmp = true,
	.small = true,
	.iphc = true
};

static struct net_6lo_data test_data_9 = {
	.ipv6.vtc = 0x61,
	.ipv6.tcflow = 0x20,
	.ipv6.flow = 0x00,
	.ipv6.len = 0,
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sac1_sam00,
	.ipv6.dst = dst_m1_dam00,
	.nh.udp.src_port = htons(udp_src_port_16bit),
	.nh.udp.dst_port = htons(udp_dst_port_16bit),
	.nh.udp.len = 0x00,
	.nh.udp.chksum = 0x00,
	.hdr_diff = IPV6_DISPATCH_DIFF,
	.nh_udp = true,
	.nh_icmp = false,
	.small = true,
	.iphc = false
};

static struct net_6lo_data test_data_10 = {
	.ipv6.vtc = 0x61,
	.ipv6.tcflow = 0x20,
	.ipv6.flow = 0x00,
	.ipv6.len = 0,
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sac1_sam00,
	.ipv6.dst = dst_m1_dam00,
	.nh.udp.src_port = htons(udp_src_port_16bit),
	.nh.udp.dst_port = htons(udp_dst_port_16bit),
	.nh.udp.len = 0x00,
	.nh.udp.chksum = 0x00,
	.hdr_diff = IPV6_DISPATCH_DIFF,
	.nh_udp = false,
	.nh_icmp = false,
	.small = false,
	.iphc = false
};

static struct net_6lo_data test_data_11 = {
	.ipv6.vtc = 0x61,
	.ipv6.tcflow = 0x20,
	.ipv6.flow = 0x00,
	.ipv6.len = 0,
	.ipv6.nexthdr = 0,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sac1_sam00,
	.ipv6.dst = dst_m1_dam00,
	.hdr_diff = IPV6_DISPATCH_DIFF,
	.nh_udp = false,
	.nh_icmp = false,
	.small = false,
	.iphc = false
};

static struct net_6lo_data test_data_12 = {
	.ipv6.vtc = 0x61,
	.ipv6.tcflow = 0x20,
	.ipv6.flow = 0x00,
	.ipv6.len = 0,
	.ipv6.nexthdr = IPPROTO_ICMPV6,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sac1_sam00,
	.ipv6.dst = dst_m1_dam00,
	.nh.icmp.type = NET_ICMPV6_ECHO_REQUEST,
	.nh.icmp.code = 0,
	.nh.icmp.chksum = 0,
	.hdr_diff = IPV6_DISPATCH_DIFF,
	.nh_udp = false,
	.nh_icmp = true,
	.small = false,
	.iphc = false
};

static struct net_6lo_data test_data_13 = {
	.ipv6.vtc = 0x60,
	.ipv6.tcflow = 0x21,
	.ipv6.flow = 0x3412,
	.ipv6.len = 0,
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sam11,
	.ipv6.dst = dst_dam11,
	.nh.udp.src_port = htons(udp_src_port_8bit),
	.nh.udp.dst_port = htons(udp_dst_port_8bit_y),
	.nh.udp.len = 0x00,
	.nh.udp.chksum = 0x00,
	.hdr_diff = NET_IPV6UDPH_LEN - IPHC_SIZE - NHC_SIZE -
		    (TF_01 + NHC_1 + CID_0 + SAC0_SAM11 + M0_DAC0_DAM11) -
		    UDP_CHKSUM_0 - UDP_P01,
	.nh_udp = true,
	.nh_icmp = false,
	.small = true,
	.iphc = true
};

static struct net_6lo_data test_data_14 = {
	.ipv6.vtc = 0x60,
	.ipv6.tcflow = 0x00,
	.ipv6.flow = 0x00,
	.ipv6.len = 0,
	.ipv6.nexthdr = NET_IPV6_NEXTHDR_NONE,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sac1_sam00,
	.ipv6.dst = dst_m1_dam11,
	.hdr_diff = NET_IPV6H_LEN - IPHC_SIZE  -
		    (TF_11 + NHC_0 + CID_0 + SAC1_SAM00 + M1_DAC0_DAM11),
	.nh_udp = false,
	.nh_icmp = false,
	.small = true,
	.iphc = true
};

#if defined(CONFIG_NET_6LO_CONTEXT)
static struct net_6lo_data test_data_15 = {
	.ipv6.vtc = 0x60,
	.ipv6.tcflow = 0x20,
	.ipv6.flow = 0x3412,
	.ipv6.len = 0,
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sac1_sam01,
	.ipv6.dst = dst_dac1_dam01,
	.nh.udp.src_port = htons(udp_src_port_8bit_y),
	.nh.udp.dst_port = htons(udp_dst_port_8bit),
	.nh.udp.len = 0x00,
	.nh.udp.chksum = 0x00,
	.hdr_diff = NET_IPV6UDPH_LEN - IPHC_SIZE - NHC_SIZE -
		    (TF_01 + NHC_1 + CID_1 + SAC1_SAM01 + M0_DAC1_DAM01) -
		    UDP_CHKSUM_0 - UDP_P10,
	.nh_udp = true,
	.nh_icmp = false,
	.small = false,
	.iphc = true
};

static struct net_6lo_data test_data_16 = {
	.ipv6.vtc = 0x60,
	.ipv6.tcflow = 0x21,
	.ipv6.flow = 0x3412,
	.ipv6.len = 0,
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sac1_sam10,
	.ipv6.dst = dst_dac1_dam10,
	.nh.udp.src_port = htons(udp_src_port_8bit),
	.nh.udp.dst_port = htons(udp_dst_port_8bit_y),
	.nh.udp.len = 0x00,
	.nh.udp.chksum = 0x00,
	.hdr_diff = NET_IPV6UDPH_LEN - IPHC_SIZE - NHC_SIZE -
		    (TF_01 + NHC_1 + CID_1 + SAC1_SAM10 + M0_DAC1_DAM10) -
		    UDP_CHKSUM_0 - UDP_P01,
	.nh_udp = true,
	.nh_icmp = false,
	.small = true,
	.iphc = true
};

static struct net_6lo_data test_data_17 = {
	.ipv6.vtc = 0x60,
	.ipv6.tcflow = 0x21,
	.ipv6.flow = 0x3412,
	.ipv6.len = 0,
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sac1_sam11,
	.ipv6.dst = dst_dac1_dam11,
	.nh.udp.src_port = htons(udp_src_port_8bit),
	.nh.udp.dst_port = htons(udp_dst_port_8bit_y),
	.nh.udp.len = 0x00,
	.nh.udp.chksum = 0x00,
	.hdr_diff = NET_IPV6UDPH_LEN - IPHC_SIZE - NHC_SIZE -
		    (TF_01 + NHC_1 + CID_1 + SAC1_SAM11 + M0_DAC1_DAM11) -
		    UDP_CHKSUM_0 - UDP_P10,
	.nh_udp = true,
	.nh_icmp = false,
	.small = true,
	.iphc = true
};

static struct net_6lo_data test_data_18 = {
	.ipv6.vtc = 0x60,
	.ipv6.tcflow = 0x20,
	.ipv6.flow = 0x3412,
	.ipv6.len = 0,
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sam01,
	.ipv6.dst = dst_dac1_dam01,
	.nh.udp.src_port = htons(udp_src_port_8bit_y),
	.nh.udp.dst_port = htons(udp_dst_port_8bit),
	.nh.udp.len = 0x00,
	.nh.udp.chksum = 0x00,
	.hdr_diff = NET_IPV6UDPH_LEN - IPHC_SIZE - NHC_SIZE -
		    (TF_01 + NHC_1 + CID_1 + SAC0_SAM01 + M0_DAC1_DAM01) -
		    UDP_CHKSUM_0 - UDP_P10,
	.nh_udp = true,
	.nh_icmp = false,
	.small = false,
	.iphc = true
};

static struct net_6lo_data test_data_19 = {
	.ipv6.vtc = 0x60,
	.ipv6.tcflow = 0x20,
	.ipv6.flow = 0x3412,
	.ipv6.len = 0,
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sac1_sam01,
	.ipv6.dst = dst_dam01,
	.nh.udp.src_port = htons(udp_src_port_8bit_y),
	.nh.udp.dst_port = htons(udp_dst_port_8bit),
	.nh.udp.len = 0x00,
	.nh.udp.chksum = 0x00,
	.hdr_diff = NET_IPV6UDPH_LEN - IPHC_SIZE - NHC_SIZE -
		    (TF_01 + NHC_1 + CID_1 + SAC1_SAM01 + M0_DAC0_DAM01) -
		    UDP_CHKSUM_0 - UDP_P10,
	.nh_udp = true,
	.nh_icmp = false,
	.small = false,
	.iphc = true
};

static struct net_6lo_data test_data_20 = {
	.ipv6.vtc = 0x61,
	.ipv6.tcflow = 0x23,
	.ipv6.flow = 0x4567,
	.ipv6.len = 0,
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sac1_sam01,
	.ipv6.dst = dst_m1_dam00,
	.nh.udp.src_port = htons(udp_src_port_16bit),
	.nh.udp.dst_port = htons(udp_dst_port_16bit),
	.nh.udp.len = 0x00,
	.nh.udp.chksum = 0x00,
	.hdr_diff = NET_IPV6UDPH_LEN - IPHC_SIZE - NHC_SIZE -
		    (TF_00 + NHC_1 + CID_1 + SAC1_SAM01 + M1_DAC0_DAM00) -
		    UDP_CHKSUM_0 - UDP_P00,
	.nh_udp = true,
	.nh_icmp = false,
	.small = true,
	.iphc = true
};
static struct net_6lo_data test_data_21 = {
	.ipv6.vtc = 0x61,
	.ipv6.tcflow = 0x23,
	.ipv6.flow = 0x4567,
	.ipv6.len = 0,
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sac1_sam01,
	.ipv6.dst = dst_m1_dam01,
	.nh.udp.src_port = htons(udp_src_port_16bit),
	.nh.udp.dst_port = htons(udp_dst_port_16bit),
	.nh.udp.len = 0x00,
	.nh.udp.chksum = 0x00,
	.hdr_diff = NET_IPV6UDPH_LEN - IPHC_SIZE - NHC_SIZE -
		    (TF_00 + NHC_1 + CID_1 + SAC1_SAM01 + M1_DAC0_DAM01) -
		    UDP_CHKSUM_0 - UDP_P00,
	.nh_udp = true,
	.nh_icmp = false,
	.small = true,
	.iphc = true
};

static struct net_6lo_data test_data_22 = {
	.ipv6.vtc = 0x60,
	.ipv6.tcflow = 0x0,
	.ipv6.flow = 0x0,
	.ipv6.len = 0,
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sac1_sam10,
	.ipv6.dst = dst_m1_dam10,
	.nh.udp.src_port = htons(udp_src_port_8bit),
	.nh.udp.dst_port = htons(udp_dst_port_8bit),
	.nh.udp.len = 0x00,
	.nh.udp.chksum = 0x00,
	.hdr_diff = NET_IPV6UDPH_LEN - IPHC_SIZE - NHC_SIZE -
		    (TF_11 + NHC_1 + CID_1 + SAC1_SAM10 + M1_DAC0_DAM10) -
		    UDP_CHKSUM_0 - UDP_P00,
	.nh_udp = true,
	.nh_icmp = false,
	.small = true,
	.iphc = true
};

static struct net_6lo_data test_data_23 = {
	.ipv6.vtc = 0x60,
	.ipv6.tcflow = 0x0,
	.ipv6.flow = 0x0,
	.ipv6.len = 0,
	.ipv6.nexthdr = 0,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sac1_sam11,
	.ipv6.dst = dst_m1_dam10,
	.hdr_diff = NET_IPV6H_LEN - IPHC_SIZE -
		    (TF_11 + NHC_0 + CID_1 + SAC0_SAM11 + M1_DAC0_DAM10),
	.nh_udp = false,
	.nh_icmp = false,
	.small = true,
	.iphc = true
};

static struct net_6lo_data test_data_24 = {
	.ipv6.vtc = 0x60,
	.ipv6.tcflow = 0x20,
	.ipv6.flow = 0x3412,
	.ipv6.len = 0,
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sam00,
	.ipv6.dst = dst_dac1_dam01,
	.nh.udp.src_port = htons(udp_src_port_8bit_y),
	.nh.udp.dst_port = htons(udp_dst_port_8bit),
	.nh.udp.len = 0x00,
	.nh.udp.chksum = 0x00,
	.hdr_diff = NET_IPV6UDPH_LEN - IPHC_SIZE - NHC_SIZE -
		    (TF_01 + NHC_1 + CID_1 + SAC0_SAM00 + M0_DAC1_DAM01) -
		    UDP_CHKSUM_0 - UDP_P10,
	.nh_udp = true,
	.nh_icmp = false,
	.small = false,
	.iphc = true
};

static struct net_6lo_data test_data_25 = {
	.ipv6.vtc = 0x60,
	.ipv6.tcflow = 0x00,
	.ipv6.flow = 0x00,
	.ipv6.len = 0,
	.ipv6.nexthdr = IPPROTO_UDP,
	.ipv6.hop_limit = 0xff,
	.ipv6.src = src_sac1_sam00,
	.ipv6.dst = dst_m1_dam00,
	.nh.udp.src_port = htons(udp_src_port_8bit_y),
	.nh.udp.dst_port = htons(udp_dst_port_8bit),
	.nh.udp.len = 0x00,
	.nh.udp.chksum = 0x00,
	.hdr_diff = NET_IPV6UDPH_LEN - IPHC_SIZE - NHC_SIZE -
		    (TF_11 + NHC_1 + CID_0 + SAC1_SAM00 + M1_DAC0_DAM00) -
		    UDP_CHKSUM_0 - UDP_P10,
	.nh_udp = true,
	.nh_icmp = false,
	.small = true,
	.iphc = true
};

#endif

static void test_6lo(struct net_6lo_data *data)
{
	struct net_pkt *pkt;
	int diff;

	pkt = create_pkt(data);
	zassert_not_null(pkt, "failed to create buffer");
#if DEBUG > 0
	TC_PRINT("length before compression %zu\n",
		 net_pkt_get_len(pkt));
	net_pkt_hexdump(pkt, "before-compression");
#endif

	net_pkt_cursor_init(pkt);

	zassert_true((net_6lo_compress(pkt, data->iphc) >= 0),
		     "compression failed");

#if DEBUG > 0
	TC_PRINT("length after compression %zu\n",
		 net_pkt_get_len(pkt));
	net_pkt_hexdump(pkt, "after-compression");
#endif

	diff = net_6lo_uncompress_hdr_diff(pkt);
	zassert_true(diff == data->hdr_diff, "unexpected HDR diff");

	zassert_true(net_6lo_uncompress(pkt),
		     "uncompression failed");
#if DEBUG > 0
	TC_PRINT("length after uncompression %zu\n",
	       net_pkt_get_len(pkt));
	net_pkt_hexdump(pkt, "after-uncompression");
#endif

	zassert_true(compare_pkt(pkt, data));

	net_pkt_unref(pkt);
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
	{ "test_6lo_sac1_sam00_m1_dam11", &test_data_14},
#if defined(CONFIG_NET_6LO_CONTEXT)
	{ "test_6lo_sac1_sam01_dac1_dam01", &test_data_15},
	{ "test_6lo_sac1_sam10_dac1_dam10", &test_data_16},
	{ "test_6lo_sac1_sam11_dac1_dam11", &test_data_17},
	{ "test_6lo_sac0_sam01_dac1_dam01", &test_data_18},
	{ "test_6lo_sac1_sam01_dac0_dam01", &test_data_19},
	{ "test_6lo_sac1_sam01_m1_dam00", &test_data_20},
	{ "test_6lo_sac1_sam01_m1_dam01", &test_data_21},
	{ "test_6lo_sac1_sam10_m1_dam10", &test_data_22},
	{ "test_6lo_sac1_sam11_m1_dam10", &test_data_23},
	{ "test_6lo_sac0_sam00_dac1_dam01", &test_data_24},
	{ "test_6lo_sac1_sam00_m1_dam00", &test_data_25},
#endif
};

ZTEST(t_6lo, test_loop)
{
	int count;

	if (IS_ENABLED(CONFIG_NET_TC_THREAD_COOPERATIVE)) {
		k_thread_priority_set(k_current_get(),
				K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1));
	} else {
		k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(9));
	}

#if defined(CONFIG_NET_6LO_CONTEXT)
	net_6lo_set_context(net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY)),
			    &ctx1);
	net_6lo_set_context(net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY)),
			    &ctx2);
#endif

	for (count = 0; count < ARRAY_SIZE(tests); count++) {
		TC_PRINT("Starting %s\n", tests[count].name);

		test_6lo(tests[count].data);
	}
	net_pkt_print();
}

/*test case main entry*/
ZTEST_SUITE(t_6lo, NULL, NULL, NULL, NULL, NULL);
