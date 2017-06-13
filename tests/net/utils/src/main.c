/* main.c - Application main entry point */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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
#include <net/ethernet.h>
#include <sections.h>

#include <tc_util.h>

#define NET_LOG_ENABLED 1
#include "net_private.h"

/* ICMPv6 frame (104 bytes) */
static const unsigned char pkt1[104] = {
/* IPv6 header starts here */
0x60, 0x0c, 0x21, 0x63, 0x00, 0x40, 0x3a, 0x40, /* `.!c.@:@ */
0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, /*  ....... */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, /* ........ */
0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, /*  ....... */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, /* ........ */
/* ICMPv6 header starts here */
0x80, 0x00, 0x65, 0x6a, 0x68, 0x47, 0x00, 0x01, /* ..ejhG.. */
0x95, 0x5f, 0x3c, 0x57, 0x00, 0x00, 0x00, 0x00, /* ._<W.... */
0xbc, 0xd3, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, /* ........ */
0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, /* ........ */
0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, /* ........ */
0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, /*  !"#$%&' */
0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, /* ()*+,-./ */
0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37  /* 01234567 */
};

/* ICMPv6 Frame (176 bytes) */
static const unsigned char pkt2[176] = {
/* IPv6 header starts here */
0x60, 0x0c, 0x21, 0x63, 0x00, 0x88, 0x3a, 0x40, /* `.!c..:@ */
0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, /*  ....... */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, /* ........ */
0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, /*  ....... */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, /* ........ */
/* ICMPv6 header starts here */
0x80, 0x00, 0x38, 0xbf, 0x32, 0x8d, 0x00, 0x01, /* ..8.2... */
0x78, 0x78, 0x3c, 0x57, 0x00, 0x00, 0x00, 0x00, /* xx<W.... */
0x63, 0xdb, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, /* c....... */
0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, /* ........ */
0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, /* ........ */
0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, /*  !"#$%&' */
0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, /* ()*+,-./ */
0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, /* 01234567 */
0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, /* 89:;<=>? */
0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, /* @ABCDEFG */
0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, /* HIJKLMNO */
0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, /* PQRSTUVW */
0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, /* XYZ[\]^_ */
0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, /* `abcdefg */
0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, /* hijklmno */
0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, /* pqrstuvw */
0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f  /* xyz{|}~. */
};

/* Frame (199 bytes) */
static const unsigned char pkt3[199] = {
/* IPv6 header starts here */
0x60, 0x0c, 0x21, 0x63, 0x00, 0x9f, 0x3a, 0x40, /* `.!c..:@ */
0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, /*  ....... */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, /* ........ */
0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, /*  ....... */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, /* ........ */
/* ICMPv6 header starts here */
0x80, 0x00, 0x57, 0xac, 0x32, 0xeb, 0x00, 0x01, /* ..W.2... */
0x8f, 0x79, 0x3c, 0x57, 0x00, 0x00, 0x00, 0x00, /* .y<W.... */
0xa8, 0x78, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, /* .x...... */
0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, /* ........ */
0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, /* ........ */
0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, /*  !"#$%&' */
0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, /* ()*+,-./ */
0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, /* 01234567 */
0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, /* 89:;<=>? */
0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, /* @ABCDEFG */
0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, /* HIJKLMNO */
0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, /* PQRSTUVW */
0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, /* XYZ[\]^_ */
0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, /* `abcdefg */
0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, /* hijklmno */
0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, /* pqrstuvw */
0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, /* xyz{|}~. */
0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, /* ........ */
0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, /* ........ */
0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96        /* ....... */
};

/* ICMP reply (98 bytes) */
static const unsigned char pkt4[98] = {
/* Ethernet header starts here */
0x1a, 0xc9, 0xb7, 0xb6, 0x46, 0x70, 0x10, 0x00, /* ....Fp.. */
0x00, 0x00, 0x00, 0x68, 0x08, 0x00,
/* IPv4 header starts here */
0x45, 0x00, /* ...h..E. */
0x00, 0x54, 0x33, 0x35, 0x40, 0x00, 0x40, 0x01, /* .T35@.@. */
0xf6, 0xf5, 0xc0, 0x00, 0x02, 0x01, 0x0a, 0xed, /* ........ */
0x43, 0x90,
/* ICMP header starts here */
0x00, 0x00, 0x14, 0xe2, 0x59, 0xe2, /* C.....Y. */
0x00, 0x01, 0x68, 0x4b, 0x44, 0x57, 0x00, 0x00, /* ..hKDW.. */
0x00, 0x00, 0x1a, 0xc5, 0x0b, 0x00, 0x00, 0x00, /* ........ */
0x00, 0x00, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, /* ........ */
0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, /* ........ */
0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, /* .. !"#$% */
0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, /* &'()*+,- */
0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, /* ./012345 */
0x36, 0x37                                      /* 67 */
};

/* ICMP request (98 bytes) */
static const unsigned char pkt5[98] = {
/* Ethernet header starts here */
0x10, 0x00, 0x00, 0x00, 0x00, 0x68, 0x1a, 0xc9, /* .....h.. */
0xb7, 0xb6, 0x46, 0x70, 0x08, 0x00,
/* IPv4 header starts here */
0x45, 0x00, /* ..Fp..E. */
0x00, 0x54, 0x33, 0x35, 0x40, 0x00, 0x40, 0x01, /* .T35@.@. */
0xf6, 0xf5, 0x0a, 0xed, 0x43, 0x90, 0xc0, 0x00, /* ....C... */
0x02, 0x01,
/* ICMP header starts here */
0x08, 0x00, 0x0c, 0xe2, 0x59, 0xe2, /* ......Y. */
0x00, 0x01, 0x68, 0x4b, 0x44, 0x57, 0x00, 0x00, /* ..hKDW.. */
0x00, 0x00, 0x1a, 0xc5, 0x0b, 0x00, 0x00, 0x00, /* ........ */
0x00, 0x00, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, /* ........ */
0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, /* ........ */
0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, /* .. !"#$% */
0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, /* &'()*+,- */
0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, /* ./012345 */
0x36, 0x37                                      /* 67 */
};

static bool run_tests(void)
{
	struct net_pkt *pkt;
	struct net_buf *frag;
	u16_t chksum, orig_chksum;
	int hdr_len, i, chunk, datalen, total = 0;

	/* Packet fits to one fragment */
	pkt = net_pkt_get_reserve_rx(0, K_FOREVER);
	frag = net_pkt_get_reserve_rx_data(10, K_FOREVER);
	net_pkt_frag_add(pkt, frag);

	memcpy(net_buf_add(frag, sizeof(pkt1)), pkt1, sizeof(pkt1));
	if (frag->len != sizeof(pkt1)) {
		printk("Fragment len %d invalid, should be %zd\n",
		       frag->len, sizeof(pkt1));
		return false;
	}

	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv6_hdr));
	net_pkt_set_family(pkt, AF_INET6);
	net_pkt_set_ipv6_ext_len(pkt, 0);

	/* We need to zero the ICMP checksum */
	hdr_len = net_pkt_ip_hdr_len(pkt);
	orig_chksum = (frag->data[hdr_len + 2] << 8) + frag->data[hdr_len + 3];
	frag->data[hdr_len + 2] = 0;
	frag->data[hdr_len + 3] = 0;

	chksum = ntohs(~net_calc_chksum(pkt, IPPROTO_ICMPV6));
	if (chksum != orig_chksum) {
		printk("Invalid chksum 0x%x in pkt1, should be 0x%x\n",
		       chksum, orig_chksum);
		return false;
	}
	net_pkt_unref(pkt);

	/* Then a case where there will be two fragments */
	pkt = net_pkt_get_reserve_rx(0, K_FOREVER);
	frag = net_pkt_get_reserve_rx_data(10, K_FOREVER);
	net_pkt_frag_add(pkt, frag);
	memcpy(net_buf_add(frag, sizeof(pkt2) / 2), pkt2, sizeof(pkt2) / 2);

	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv6_hdr));
	net_pkt_set_family(pkt, AF_INET6);
	net_pkt_set_ipv6_ext_len(pkt, 0);

	hdr_len = net_pkt_ip_hdr_len(pkt);
	orig_chksum = (frag->data[hdr_len + 2] << 8) + frag->data[hdr_len + 3];
	frag->data[hdr_len + 2] = 0;
	frag->data[hdr_len + 3] = 0;

	frag = net_pkt_get_reserve_rx_data(10, K_FOREVER);
	net_pkt_frag_add(pkt, frag);
	memcpy(net_buf_add(frag, sizeof(pkt2) - sizeof(pkt2) / 2),
	       pkt2 + sizeof(pkt2) / 2, sizeof(pkt2) - sizeof(pkt2) / 2);

	chksum = ntohs(~net_calc_chksum(pkt, IPPROTO_ICMPV6));
	if (chksum != orig_chksum) {
		printk("Invalid chksum 0x%x in pkt2, should be 0x%x\n",
		       chksum, orig_chksum);
		return false;
	}
	net_pkt_unref(pkt);

	/* Then a case where there will be two fragments but odd data size */
	pkt = net_pkt_get_reserve_rx(0, K_FOREVER);
	frag = net_pkt_get_reserve_rx_data(10, K_FOREVER);
	net_pkt_frag_add(pkt, frag);
	memcpy(net_buf_add(frag, sizeof(pkt3) / 2), pkt3, sizeof(pkt3) / 2);
	printk("First fragment will have %zd bytes\n", sizeof(pkt3) / 2);

	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv6_hdr));
	net_pkt_set_family(pkt, AF_INET6);
	net_pkt_set_ipv6_ext_len(pkt, 0);

	hdr_len = net_pkt_ip_hdr_len(pkt);
	orig_chksum = (frag->data[hdr_len + 2] << 8) + frag->data[hdr_len + 3];
	frag->data[hdr_len + 2] = 0;
	frag->data[hdr_len + 3] = 0;

	frag = net_pkt_get_reserve_rx_data(10, K_FOREVER);
	net_pkt_frag_add(pkt, frag);
	memcpy(net_buf_add(frag, sizeof(pkt3) - sizeof(pkt3) / 2),
	       pkt3 + sizeof(pkt3) / 2, sizeof(pkt3) - sizeof(pkt3) / 2);
	printk("Second fragment will have %zd bytes\n",
	       sizeof(pkt3) - sizeof(pkt3) / 2);

	chksum = ntohs(~net_calc_chksum(pkt, IPPROTO_ICMPV6));
	if (chksum != orig_chksum) {
		printk("Invalid chksum 0x%x in pkt3, should be 0x%x\n",
		       chksum, orig_chksum);
		return false;
	}
	net_pkt_unref(pkt);

	/* Then a case where there will be several fragments */
	pkt = net_pkt_get_reserve_rx(0, K_FOREVER);
	frag = net_pkt_get_reserve_rx_data(10, K_FOREVER);
	net_pkt_frag_add(pkt, frag);
	memcpy(net_buf_add(frag, sizeof(struct net_ipv6_hdr)), pkt3,
	       sizeof(struct net_ipv6_hdr));
	printk("[0] IPv6 fragment will have %d bytes\n", frag->len);

	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv6_hdr));
	net_pkt_set_family(pkt, AF_INET6);
	net_pkt_set_ipv6_ext_len(pkt, 0);

	chunk = 29;
	datalen = sizeof(pkt3) - sizeof(struct net_ipv6_hdr);

	for (i = 0; i < datalen/chunk; i++) {
		/* Next fragments will contain the data in odd sizes */
		frag = net_pkt_get_reserve_rx_data(10, K_FOREVER);
		net_pkt_frag_add(pkt, frag);
		memcpy(net_buf_add(frag, chunk),
		       pkt3 + sizeof(struct net_ipv6_hdr) + i * chunk, chunk);
		total += chunk;
		printk("[%d] fragment will have %d bytes, icmp data %d\n",
		       i + 1, frag->len, total);

		if (i == 0) {
			/* First fragment will contain the ICMP header
			 * and we must clear the checksum field.
			 */
			orig_chksum = (frag->data[2] << 8) +
				frag->data[3];
			frag->data[2] = 0;
			frag->data[3] = 0;
		}
	}
	if ((datalen - total) > 0) {
		frag = net_pkt_get_reserve_rx_data(10, K_FOREVER);
		net_pkt_frag_add(pkt, frag);
		memcpy(net_buf_add(frag, datalen - total),
		       pkt3 + sizeof(struct net_ipv6_hdr) + i * chunk,
		       datalen - total);
		total += datalen - total;
		printk("[%d] last fragment will have %d bytes, icmp data %d\n",
		       i + 1, frag->len, total);
	}

	if ((total + sizeof(struct net_ipv6_hdr)) != sizeof(pkt3) ||
	    (total + sizeof(struct net_ipv6_hdr)) !=
				net_pkt_get_len(pkt)) {
		printk("pkt3 size differs from fragment sizes, "
		       "pkt3 size %zd frags size %zd calc total %zd\n",
		       sizeof(pkt3), net_pkt_get_len(pkt),
		       total + sizeof(struct net_ipv6_hdr));
		return false;
	}

	chksum = ntohs(~net_calc_chksum(pkt, IPPROTO_ICMPV6));
	if (chksum != orig_chksum) {
		printk("Invalid chksum 0x%x in pkt3, should be 0x%x\n",
		       chksum, orig_chksum);
		return false;
	}
	net_pkt_unref(pkt);

	/* Another packet that fits to one fragment.
	 * This one has ethernet header before IPv4 data.
	 */
	pkt = net_pkt_get_reserve_rx(0, K_FOREVER);
	frag = net_pkt_get_reserve_rx_data(sizeof(struct net_eth_hdr),
					    K_FOREVER);
	net_pkt_frag_add(pkt, frag);

	net_pkt_set_ll_reserve(pkt, sizeof(struct net_eth_hdr));
	memcpy(net_pkt_ll(pkt), pkt4, sizeof(pkt4));
	net_buf_add(frag, sizeof(pkt4) - sizeof(struct net_eth_hdr));

	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv4_hdr));
	net_pkt_set_family(pkt, AF_INET);
	net_pkt_set_ipv6_ext_len(pkt, 0);

	hdr_len = net_pkt_ip_hdr_len(pkt);
	orig_chksum = (frag->data[hdr_len + 2] << 8) + frag->data[hdr_len + 3];
	frag->data[hdr_len + 2] = 0;
	frag->data[hdr_len + 3] = 0;

	chksum = ntohs(~net_calc_chksum(pkt, IPPROTO_ICMP));
	if (chksum != orig_chksum) {
		printk("Invalid chksum 0x%x in pkt4, should be 0x%x\n",
		       chksum, orig_chksum);
		return false;
	}
	net_pkt_unref(pkt);

	/* Another packet that fits to one fragment and which has correct
	 * checksum. This one has ethernet header before IPv4 data.
	 */
	pkt = net_pkt_get_reserve_rx(0, K_FOREVER);
	frag = net_pkt_get_reserve_rx_data(sizeof(struct net_eth_hdr),
					    K_FOREVER);
	net_pkt_frag_add(pkt, frag);

	net_pkt_set_ll_reserve(pkt, sizeof(struct net_eth_hdr));
	memcpy(net_pkt_ll(pkt), pkt5, sizeof(pkt5));
	net_buf_add(frag, sizeof(pkt5) - sizeof(struct net_eth_hdr));

	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv4_hdr));
	net_pkt_set_family(pkt, AF_INET);
	net_pkt_set_ipv6_ext_len(pkt, 0);

	hdr_len = net_pkt_ip_hdr_len(pkt);
	orig_chksum = (frag->data[hdr_len + 2] << 8) + frag->data[hdr_len + 3];
	frag->data[hdr_len + 2] = 0;
	frag->data[hdr_len + 3] = 0;

	chksum = ntohs(~net_calc_chksum(pkt, IPPROTO_ICMP));
	if (chksum != orig_chksum) {
		printk("Invalid chksum 0x%x in pkt5, should be 0x%x\n",
		       chksum, orig_chksum);
		return false;
	}
	net_pkt_unref(pkt);

	return true;
}

struct net_addr_test_data {
	sa_family_t family;
	bool pton;

	struct {
		char c_addr[16];
		char c_verify[16];
		struct in_addr addr;
		struct in_addr verify;
	} ipv4;

	struct {
		char c_addr[46];
		char c_verify[46];
		struct in6_addr addr;
		struct in6_addr verify;
	} ipv6;
};

static struct net_addr_test_data ipv4_pton_1 = {
	.family = AF_INET,
	.pton = true,
	.ipv4.c_addr = "192.0.0.1",
	.ipv4.verify.s4_addr = { 192, 0, 0, 1 },
};

static struct net_addr_test_data ipv4_pton_2 = {
	.family = AF_INET,
	.pton = true,
	.ipv4.c_addr = "192.1.0.0",
	.ipv4.verify.s4_addr = { 192, 1, 0, 0 },
};

static struct net_addr_test_data ipv4_pton_3 = {
	.family = AF_INET,
	.pton = true,
	.ipv4.c_addr = "192.0.0.0",
	.ipv4.verify.s4_addr = { 192, 0, 0, 0 },
};

static struct net_addr_test_data ipv4_pton_4 = {
	.family = AF_INET,
	.pton = true,
	.ipv4.c_addr = "255.255.255.255",
	.ipv4.verify.s4_addr = { 255, 255, 255, 255 },
};

static struct net_addr_test_data ipv4_pton_5 = {
	.family = AF_INET,
	.pton = true,
	.ipv4.c_addr = "0.0.0.0",
	.ipv4.verify.s4_addr = { 0, 0, 0, 0 },
};

static struct net_addr_test_data ipv4_pton_6 = {
	.family = AF_INET,
	.pton = true,
	.ipv4.c_addr = "0.0.0.1",
	.ipv4.verify.s4_addr = { 0, 0, 0, 1 },
};

static struct net_addr_test_data ipv4_pton_7 = {
	.family = AF_INET,
	.pton = true,
	.ipv4.c_addr = "0.0.1.0",
	.ipv4.verify.s4_addr = { 0, 0, 1, 0 },
};

static struct net_addr_test_data ipv4_pton_8 = {
	.family = AF_INET,
	.pton = true,
	.ipv4.c_addr = "0.1.0.0",
	.ipv4.verify.s4_addr = { 0, 1, 0, 0 },
};

static struct net_addr_test_data ipv6_pton_1 = {
	.family = AF_INET6,
	.pton = true,
	.ipv6.c_addr = "ff08::",
	.ipv6.verify.s6_addr32 = { htons(0xff08), 0, 0, 0 },
};

static struct net_addr_test_data ipv6_pton_2 = {
	.family = AF_INET6,
	.pton = true,
	.ipv6.c_addr = "::",
	.ipv6.verify.s6_addr32 = { 0, 0, 0, 0 },
};

static struct net_addr_test_data ipv6_pton_3 = {
	.family = AF_INET6,
	.pton = true,
	.ipv6.c_addr = "ff08::1",
	.ipv6.verify.s6_addr16 = { htons(0xff08), 0, 0, 0, 0, 0, 0, htons(1) },
};

static struct net_addr_test_data ipv6_pton_4 = {
	.family = AF_INET6,
	.pton = true,
	.ipv6.c_addr = "2001:db8::1",
	.ipv6.verify.s6_addr16 = { htons(0x2001), htons(0xdb8),
				   0, 0, 0, 0, 0, htons(1) },
};

static struct net_addr_test_data ipv6_pton_5 = {
	.family = AF_INET6,
	.pton = true,
	.ipv6.c_addr = "2001:db8::2:1",
	.ipv6.verify.s6_addr16 = { htons(0x2001), htons(0xdb8),
				   0, 0, 0, 0, htons(2), htons(1) },
};

static struct net_addr_test_data ipv6_pton_6 = {
	.family = AF_INET6,
	.pton = true,
	.ipv6.c_addr = "ff08:1122:3344:5566:7788:9900:aabb:ccdd",
	.ipv6.verify.s6_addr16 = { htons(0xff08), htons(0x1122),
				   htons(0x3344), htons(0x5566),
				   htons(0x7788), htons(0x9900),
				   htons(0xaabb), htons(0xccdd) },
};

/* net_addr_ntop test cases */
static struct net_addr_test_data ipv4_ntop_1 = {
	.family = AF_INET,
	.pton = false,
	.ipv4.c_verify = "192.0.0.1",
	.ipv4.addr.s4_addr = { 192, 0, 0, 1 },
};

static struct net_addr_test_data ipv4_ntop_2 = {
	.family = AF_INET,
	.pton = false,
	.ipv4.c_verify = "192.1.0.0",
	.ipv4.addr.s4_addr = { 192, 1, 0, 0 },
};

static struct net_addr_test_data ipv4_ntop_3 = {
	.family = AF_INET,
	.pton = false,
	.ipv4.c_verify = "192.0.0.0",
	.ipv4.addr.s4_addr = { 192, 0, 0, 0 },
};

static struct net_addr_test_data ipv4_ntop_4 = {
	.family = AF_INET,
	.pton = false,
	.ipv4.c_verify = "255.255.255.255",
	.ipv4.addr.s4_addr = { 255, 255, 255, 255 },
};

static struct net_addr_test_data ipv4_ntop_5 = {
	.family = AF_INET,
	.pton = false,
	.ipv4.c_verify = "0.0.0.0",
	.ipv4.addr.s4_addr = { 0, 0, 0, 0 },
};

static struct net_addr_test_data ipv4_ntop_6 = {
	.family = AF_INET,
	.pton = false,
	.ipv4.c_verify = "0.0.0.1",
	.ipv4.addr.s4_addr = { 0, 0, 0, 1 },
};

static struct net_addr_test_data ipv4_ntop_7 = {
	.family = AF_INET,
	.pton = false,
	.ipv4.c_verify = "0.0.1.0",
	.ipv4.addr.s4_addr = { 0, 0, 1, 0 },
};

static struct net_addr_test_data ipv4_ntop_8 = {
	.family = AF_INET,
	.pton = false,
	.ipv4.c_verify = "0.1.0.0",
	.ipv4.addr.s4_addr = { 0, 1, 0, 0 },
};

static struct net_addr_test_data ipv6_ntop_1 = {
	.family = AF_INET6,
	.pton = false,
	.ipv6.c_verify = "ff08::",
	.ipv6.addr.s6_addr32 = { htons(0xff08), 0, 0, 0 },
};

static struct net_addr_test_data ipv6_ntop_2 = {
	.family = AF_INET6,
	.pton = false,
	.ipv6.c_verify = "::",
	.ipv6.addr.s6_addr32 = { 0, 0, 0, 0 },
};

static struct net_addr_test_data ipv6_ntop_3 = {
	.family = AF_INET6,
	.pton = false,
	.ipv6.c_verify = "ff08::1",
	.ipv6.addr.s6_addr16 = { htons(0xff08), 0, 0, 0, 0, 0, 0, htons(1) },
};

static struct net_addr_test_data ipv6_ntop_4 = {
	.family = AF_INET6,
	.pton = false,
	.ipv6.c_verify = "2001:db8::1",
	.ipv6.addr.s6_addr16 = { htons(0x2001), htons(0xdb8),
				 0, 0, 0, 0, 0, htons(1) },
};

static struct net_addr_test_data ipv6_ntop_5 = {
	.family = AF_INET6,
	.pton = false,
	.ipv6.c_verify = "2001:db8::2:1",
	.ipv6.addr.s6_addr16 = { htons(0x2001), htons(0xdb8),
				 0, 0, 0, 0, htons(2), htons(1) },
};

static struct net_addr_test_data ipv6_ntop_6 = {
	.family = AF_INET6,
	.pton = false,
	.ipv6.c_verify = "ff08:1122:3344:5566:7788:9900:aabb:ccdd",
	.ipv6.addr.s6_addr16 = { htons(0xff08), htons(0x1122),
				 htons(0x3344), htons(0x5566),
				 htons(0x7788), htons(0x9900),
				 htons(0xaabb), htons(0xccdd) },
};

static const struct {
	const char *name;
	struct net_addr_test_data *data;
} tests[] = {
	/* IPv4 net_addr_pton */
	{ "test_ipv4_pton_1", &ipv4_pton_1},
	{ "test_ipv4_pton_2", &ipv4_pton_2},
	{ "test_ipv4_pton_3", &ipv4_pton_3},
	{ "test_ipv4_pton_4", &ipv4_pton_4},
	{ "test_ipv4_pton_5", &ipv4_pton_5},
	{ "test_ipv4_pton_6", &ipv4_pton_6},
	{ "test_ipv4_pton_7", &ipv4_pton_7},
	{ "test_ipv4_pton_8", &ipv4_pton_8},

	/* IPv6 net_addr_pton */
	{ "test_ipv6_pton_1", &ipv6_pton_1},
	{ "test_ipv6_pton_2", &ipv6_pton_2},
	{ "test_ipv6_pton_3", &ipv6_pton_3},
	{ "test_ipv6_pton_4", &ipv6_pton_4},
	{ "test_ipv6_pton_5", &ipv6_pton_5},
	{ "test_ipv6_pton_6", &ipv6_pton_6},

	/* IPv4 net_addr_ntop */
	{ "test_ipv4_ntop_1", &ipv4_ntop_1},
	{ "test_ipv4_ntop_2", &ipv4_ntop_2},
	{ "test_ipv4_ntop_3", &ipv4_ntop_3},
	{ "test_ipv4_ntop_4", &ipv4_ntop_4},
	{ "test_ipv4_ntop_5", &ipv4_ntop_5},
	{ "test_ipv4_ntop_6", &ipv4_ntop_6},
	{ "test_ipv4_ntop_7", &ipv4_ntop_7},
	{ "test_ipv4_ntop_8", &ipv4_ntop_8},

	/* IPv6 net_addr_ntop */
	{ "test_ipv6_ntop_1", &ipv6_ntop_1},
	{ "test_ipv6_ntop_2", &ipv6_ntop_2},
	{ "test_ipv6_ntop_3", &ipv6_ntop_3},
	{ "test_ipv6_ntop_4", &ipv6_ntop_4},
	{ "test_ipv6_ntop_5", &ipv6_ntop_5},
	{ "test_ipv6_ntop_6", &ipv6_ntop_6},
};

static bool test_net_addr(struct net_addr_test_data *data)
{
	switch (data->family) {
	case AF_INET:
		if (data->pton) {
			if (net_addr_pton(AF_INET, (char *)data->ipv4.c_addr,
					  &data->ipv4.addr) < 0) {
				printk("Failed to convert %s\n",
				       data->ipv4.c_addr);

				return false;
			}

			if (!net_ipv4_addr_cmp(&data->ipv4.addr,
					       &data->ipv4.verify)) {
				printk("Failed to verify %s\n",
				       data->ipv4.c_addr);

				return false;
			}
		} else {
			if (!net_addr_ntop(AF_INET, &data->ipv4.addr,
					   data->ipv4.c_addr,
					   sizeof(data->ipv4.c_addr))) {
				printk("Failed to convert %s\n",
				       net_sprint_ipv4_addr(&data->ipv4.addr));

				return false;
			}

			if (strcmp(data->ipv4.c_addr, data->ipv4.c_verify)) {
				printk("Failed to verify %s\n",
				       data->ipv4.c_addr);
				printk("against %s\n",
				       data->ipv4.c_verify);

				return false;
			}
		}

		break;

	case AF_INET6:
		if (data->pton) {
			if (net_addr_pton(AF_INET6, (char *)data->ipv6.c_addr,
					  &data->ipv6.addr) < 0) {
				printk("Failed to convert %s\n",
				       data->ipv6.c_addr);

				return false;
			}

			if (!net_ipv6_addr_cmp(&data->ipv6.addr,
					       &data->ipv6.verify)) {
				printk("Failed to verify %s\n",
				       net_sprint_ipv6_addr(&data->ipv6.addr));
				printk("against %s\n",
				       net_sprint_ipv6_addr(
							&data->ipv6.verify));

				return false;
			}
		} else {
			if (!net_addr_ntop(AF_INET6, &data->ipv6.addr,
					   data->ipv6.c_addr,
					   sizeof(data->ipv6.c_addr))) {
				printk("Failed to convert %s\n",
				       net_sprint_ipv6_addr(&data->ipv6.addr));

				return false;
			}

			if (strcmp(data->ipv6.c_addr, data->ipv6.c_verify)) {
				printk("Failed to verify %s\n",
				       data->ipv6.c_addr);
				printk("against %s\n",
				       data->ipv6.c_verify);

				return false;
			}

		}

		break;
	}

	return true;
}

static bool run_net_addr_tests(void)
{
	int count, pass;

	for (count = 0, pass = 0; count < ARRAY_SIZE(tests); count++) {
		TC_START(tests[count].name);

		if (test_net_addr(tests[count].data)) {
			TC_END(PASS, "passed\n");
			pass++;
		} else {
			TC_END(FAIL, "failed\n");
		}
	}

	return (pass != ARRAY_SIZE(tests)) ? false : true;
}

void main(void)
{
	k_thread_priority_set(k_current_get(), K_PRIO_COOP(7));
	if (run_tests() && run_net_addr_tests()) {
		TC_END_REPORT(TC_PASS);
	} else {
		TC_END_REPORT(TC_FAIL);
	}
}
