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
#include <linker/sections.h>

#include <tc_util.h>
#include <ztest.h>

#define NET_LOG_ENABLED 1
#include "net_private.h"

#if defined(CONFIG_NET_IPV6)
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

/* IPv6 TCP (224 bytes) */
static const unsigned char v6_tcp_pkt1[224] = {
/* Ethernet header starts here */
0x00, 0xe0, 0x4c, 0x36, 0x03, 0xa1, 0x00, 0x25, /* ..L6...% */
0x11, 0x4a, 0x86, 0xb3, 0x86, 0xdd,
/* IPv6 header starts here */
0x60, 0x00, /* .J....`. */
0x00, 0x00, 0x00, 0xaa, 0x06, 0x80, 0xfe, 0x80, /* ........ */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf9, 0x21, /* .......! */
0x19, 0xda, 0xf8, 0x9d, 0xf9, 0xa0, 0xfe, 0x80, /* ........ */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xe0, /* ........ */
0x4c, 0xff, 0xfe, 0x36, 0x03, 0xa1,
/* TCP header starts here */
0xf2, 0x50, /* L..6...P */
0x10, 0x92, 0x84, 0x19, 0x69, 0xb5, 0xb5, 0x20, /* ....i..  */
0x2d, 0x50, 0x50, 0x18, 0xfd, 0x20, 0x60, 0xfd, /* -PP.. `. */
0x00, 0x00, 0x37, 0xd9, 0x7a, 0x92, 0x2c, 0x0d, /* ..7.z.,. */
0x5b, 0x2a, 0x96, 0xa9, 0x7f, 0xf7, 0x4f, 0x40, /* [*....O@ */
0x1c, 0xdb, 0x1d, 0x72, 0xe6, 0xbd, 0x16, 0xe4, /* ...r.... */
0xb2, 0x07, 0xb8, 0xc1, 0x78, 0xab, 0xd9, 0xd4, /* ....x... */
0x8c, 0x00, 0xd9, 0x93, 0x83, 0xeb, 0x93, 0x85, /* ........ */
0xa3, 0x6b, 0x84, 0xf2, 0x2c, 0x47, 0x02, 0xbb, /* .k..,G.. */
0xbf, 0xa4, 0x17, 0xe1, 0x2d, 0x0b, 0xd5, 0xcc, /* ....-... */
0x16, 0xee, 0x8f, 0x20, 0xcb, 0xf8, 0x7d, 0xd2, /* ... ..}. */
0x4d, 0x4e, 0xf4, 0xbd, 0x18, 0x94, 0xb4, 0xa3, /* MN...... */
0x36, 0xf8, 0x11, 0x55, 0xf9, 0x52, 0x5a, 0x75, /* 6..U.RZu */
0x8d, 0x8e, 0x86, 0x2e, 0x95, 0x3d, 0x53, 0x1c, /* .....=S. */
0x86, 0xad, 0xc2, 0x82, 0x00, 0xb5, 0xcd, 0x78, /* .......x */
0x2e, 0xfc, 0x06, 0xa7, 0x63, 0x1f, 0xea, 0xf0, /* ....c... */
0xd5, 0x37, 0xe9, 0x80, 0x54, 0xe9, 0xd1, 0xd7, /* .7..T... */
0x44, 0x75, 0x8d, 0xe6, 0x83, 0x51, 0x59, 0x84, /* Du...QY. */
0x14, 0xa0, 0xe3, 0xcc, 0x31, 0xa5, 0x98, 0x69, /* ....1..i */
0x2d, 0x5c, 0xb4, 0x6e, 0x15, 0xd6, 0x57, 0xf2, /* -\.n..W. */
0x1f, 0xad, 0x94, 0xea, 0x2d, 0x2a, 0x34, 0x19, /* ....-*4. */
0xb8, 0xad, 0xf6, 0x77, 0x3b, 0x51, 0x0d, 0x18  /* ...w;Q.. */
};

/* IPv6 UDP (179 bytes) */
static const unsigned char v6_udp_pkt1[179] = {
/* Ethernet header starts here */
0x33, 0x33, 0x00, 0x00, 0x00, 0xfb, 0x7c, 0x01, /* 33....|. */
0x91, 0x69, 0xb5, 0xbb, 0x86, 0xdd,
/* IPv6 header starts here */
0x60, 0x09, /* .i....`. */
0xe5, 0x0e, 0x00, 0x7d, 0x11, 0xff, 0xfe, 0x80, /* ...}.... */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0xa7, /* ........ */
0xae, 0x7d, 0xf8, 0x04, 0x0c, 0xa5, 0xff, 0x02, /* .}...... */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ........ */
0x00, 0x00, 0x00, 0x00, 0x00, 0xfb,
/* UDP header starts here */
0x14, 0xe9, /* ........ */
0x14, 0xe9, 0x00, 0x7d, 0xad, 0x58, 0x00, 0x00, /* ...}.X.. */
0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, /* ........ */
0x00, 0x01, 0x08, 0x5f, 0x68, 0x6f, 0x6d, 0x65, /* ..._home */
0x6b, 0x69, 0x74, 0x04, 0x5f, 0x74, 0x63, 0x70, /* kit._tcp */
0x05, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x00, 0x00, /* .local.. */
0x0c, 0x80, 0x01, 0x08, 0x5f, 0x61, 0x69, 0x72, /* ...._air */
0x70, 0x6c, 0x61, 0x79, 0xc0, 0x15, 0x00, 0x0c, /* play.... */
0x80, 0x01, 0x05, 0x5f, 0x72, 0x61, 0x6f, 0x70, /* ..._raop */
0xc0, 0x15, 0x00, 0x0c, 0x80, 0x01, 0x0c, 0x5f, /* ......._ */
0x73, 0x6c, 0x65, 0x65, 0x70, 0x2d, 0x70, 0x72, /* sleep-pr */
0x6f, 0x78, 0x79, 0x04, 0x5f, 0x75, 0x64, 0x70, /* oxy._udp */
0xc0, 0x1a, 0x00, 0x0c, 0x80, 0x01, 0x00, 0x00, /* ........ */
0x29, 0x05, 0xa0, 0x00, 0x00, 0x11, 0x94, 0x00, /* )....... */
0x12, 0x00, 0x04, 0x00, 0x0e, 0x00, 0x7a, 0x7e, /* ......z~ */
0x01, 0x91, 0x69, 0xb5, 0xbb, 0x7c, 0x01, 0x91, /* ..i..|.. */
0x69, 0xb5, 0xbb                                /* i.. */
};

#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
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

/* IPv4 TCP (90 bytes) */
static const unsigned char v4_tcp_pkt1[90] = {
/* Ethernet header starts here */
0x00, 0x25, 0x11, 0x4a, 0x86, 0xb3, 0x00, 0xe0, /* .%.J.... */
0x4c, 0x36, 0x03, 0xa1, 0x08, 0x00,
0x45, 0x10, /* L6....E. */
/* IPv4 header starts here */
0x00, 0x4c, 0x99, 0x3c, 0x40, 0x00, 0x40, 0x06, /* .L.<@.@. */
0x1c, 0x5a, 0xc0, 0xa8, 0x01, 0xdb, 0xc0, 0xa8, /* .Z...... */
0x01, 0xda,
/* TCP header starts here */
0x00, 0x16, 0xcc, 0x1d, 0xa4, 0xeb, /* ........ */
0xe4, 0x82, 0x43, 0x60, 0x23, 0x46, 0x50, 0x18, /* ..C`#FP. */
0x01, 0x46, 0x85, 0x44, 0x00, 0x00, 0xe7, 0xe7, /* .F.D.... */
0x3f, 0x13, 0xe2, 0xc1, 0x09, 0x37, 0x90, 0x37, /* ?....7.7 */
0x02, 0x0e, 0x17, 0x12, 0xb6, 0x31, 0xd6, 0x22, /* .....1." */
0xb9, 0x04, 0x71, 0x57, 0x21, 0x1a, 0xe2, 0x5d, /* ..qW!..] */
0xef, 0xfc, 0xc3, 0x66, 0x47, 0x7f, 0xf3, 0x5b, /* ...fG..[ */
0xc7, 0x9b                                      /* .. */
};

/* IPv4 UDP (192 bytes) */
static const unsigned char v4_udp_pkt1[192] = {
/* Ethernet header starts here */
0x00, 0xe0, 0x4c, 0x36, 0x03, 0xa1, 0x00, 0x25, /* ..L6...% */
0x11, 0x4a, 0x86, 0xb3, 0x08, 0x00,
/* IPv4 header starts here */
0x45, 0x00, /* .J....E. */
0x00, 0xb2, 0x06, 0x85, 0x00, 0x00, 0x80, 0x11, /* ........ */
0xae, 0xb1, 0xc0, 0xa8, 0x01, 0xd9, 0xc0, 0xa8, /* ........ */
0x01, 0xdb,
/* UDP header starts here */
0xfb, 0xaa, 0x10, 0x92, 0x00, 0x9e, /* ........ */
0xd2, 0x30, 0x57, 0x6d, 0x3d, 0xfc, 0x78, 0x76, /* .0Wm=.xv */
0xf7, 0xa5, 0xd8, 0x21, 0x92, 0x34, 0x7e, 0xe9, /* ...!.4~. */
0x5f, 0x7f, 0xe4, 0xbe, 0x39, 0x54, 0x51, 0x04, /* _...9TQ. */
0xd5, 0x00, 0x19, 0x76, 0x3b, 0xf3, 0xdc, 0x6a, /* ...v;..j */
0xbd, 0xaa, 0x8a, 0x7e, 0xc3, 0xf5, 0x57, 0xea, /* ...~..W. */
0x28, 0xb5, 0xea, 0x34, 0x41, 0x0c, 0xbc, 0x1b, /* (..4A... */
0x3d, 0xfb, 0xf3, 0x71, 0x58, 0xda, 0x51, 0x01, /* =..qX.Q. */
0x39, 0xcc, 0x03, 0xdf, 0x5e, 0xef, 0x43, 0x16, /* 9...^.C. */
0x8d, 0xe7, 0xe5, 0xae, 0x49, 0xa4, 0x9b, 0xa1, /* ....I... */
0xe1, 0x78, 0xc5, 0x25, 0x96, 0xec, 0x2f, 0xda, /* .x.%../. */
0x70, 0xa5, 0x57, 0x4f, 0xd0, 0xb8, 0x32, 0xcd, /* p.WO..2. */
0x20, 0xd3, 0x51, 0x2f, 0x2f, 0x09, 0x6b, 0x7f, /*  .Q//.k. */
0xce, 0x55, 0x2c, 0x3b, 0x77, 0x20, 0x75, 0xb4, /* .U,;w u. */
0x73, 0x3f, 0x51, 0xba, 0x68, 0x7b, 0x7f, 0xfe, /* s?Q.h{.. */
0x54, 0x72, 0xc1, 0x20, 0x14, 0x9c, 0xd3, 0x45, /* Tr. ...E */
0xf3, 0x30, 0x5f, 0x06, 0xdc, 0xac, 0x00, 0x67, /* .0_....g */
0x56, 0xb7, 0x3f, 0xdd, 0x7d, 0xd7, 0xb0, 0xc3, /* V.?.}... */
0xd8, 0x45, 0x73, 0xa6, 0x6f, 0xb8, 0x14, 0x13, /* .Es.o... */
0xaf, 0xcf, 0xcf, 0x53, 0x93, 0xf1, 0xa0, 0x1a  /* ...S.... */
};

#endif /* CONFIG_NET_IPV4 */

void test_utils(void)
{
	k_thread_priority_set(k_current_get(), K_PRIO_COOP(7));

	struct net_pkt *pkt;
	struct net_buf *frag;
	u16_t chksum, orig_chksum;
	int hdr_len;

#if defined(CONFIG_NET_IPV6)
	int i, chunk, datalen, total = 0;

	/* Packet fits to one fragment */
	pkt = net_pkt_get_reserve_rx(0, K_SECONDS(1));
	zassert_not_null(pkt, "Out of mem");

	frag = net_pkt_get_reserve_rx_data(10, K_SECONDS(1));
	zassert_not_null(frag, "Out of mem");

	net_pkt_frag_add(pkt, frag);

	memcpy(net_buf_add(frag, sizeof(pkt1)), pkt1, sizeof(pkt1));
	if (frag->len != sizeof(pkt1)) {
		printk("Fragment len %d invalid, should be %zd\n",
		       frag->len, sizeof(pkt1));
		zassert_true(0, "exiting");
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
		zassert_true(0, "exiting");
	}
	net_pkt_unref(pkt);

	/* Then a case where there will be two fragments */
	pkt = net_pkt_get_reserve_rx(0, K_SECONDS(1));
	zassert_not_null(pkt, "Out of mem");

	frag = net_pkt_get_reserve_rx_data(10, K_SECONDS(1));
	zassert_not_null(frag, "Out of mem");

	net_pkt_frag_add(pkt, frag);
	memcpy(net_buf_add(frag, sizeof(pkt2) / 2), pkt2, sizeof(pkt2) / 2);

	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv6_hdr));
	net_pkt_set_family(pkt, AF_INET6);
	net_pkt_set_ipv6_ext_len(pkt, 0);

	hdr_len = net_pkt_ip_hdr_len(pkt);
	orig_chksum = (frag->data[hdr_len + 2] << 8) + frag->data[hdr_len + 3];
	frag->data[hdr_len + 2] = 0;
	frag->data[hdr_len + 3] = 0;

	frag = net_pkt_get_reserve_rx_data(10, K_SECONDS(1));
	zassert_not_null(frag, "Out of mem");

	net_pkt_frag_add(pkt, frag);
	memcpy(net_buf_add(frag, sizeof(pkt2) - sizeof(pkt2) / 2),
	       pkt2 + sizeof(pkt2) / 2, sizeof(pkt2) - sizeof(pkt2) / 2);

	chksum = ntohs(~net_calc_chksum(pkt, IPPROTO_ICMPV6));
	if (chksum != orig_chksum) {
		printk("Invalid chksum 0x%x in pkt2, should be 0x%x\n",
		       chksum, orig_chksum);
		zassert_true(0, "exiting");
	}
	net_pkt_unref(pkt);

	/* Then a case where there will be two fragments but odd data size */
	pkt = net_pkt_get_reserve_rx(0, K_SECONDS(1));
	zassert_not_null(pkt, "Out of mem");

	frag = net_pkt_get_reserve_rx_data(10, K_SECONDS(1));
	zassert_not_null(frag, "Out of mem");

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

	frag = net_pkt_get_reserve_rx_data(10, K_SECONDS(1));
	zassert_not_null(frag, "Out of mem");

	net_pkt_frag_add(pkt, frag);
	memcpy(net_buf_add(frag, sizeof(pkt3) - sizeof(pkt3) / 2),
	       pkt3 + sizeof(pkt3) / 2, sizeof(pkt3) - sizeof(pkt3) / 2);
	printk("Second fragment will have %zd bytes\n",
	       sizeof(pkt3) - sizeof(pkt3) / 2);

	chksum = ntohs(~net_calc_chksum(pkt, IPPROTO_ICMPV6));
	if (chksum != orig_chksum) {
		printk("Invalid chksum 0x%x in pkt3, should be 0x%x\n",
		       chksum, orig_chksum);
		zassert_true(0, "exiting");
	}
	net_pkt_unref(pkt);

	/* Then a case where there will be several fragments */
	pkt = net_pkt_get_reserve_rx(0, K_SECONDS(1));
	zassert_not_null(frag, "Out of mem");

	frag = net_pkt_get_reserve_rx_data(10, K_SECONDS(1));
	zassert_not_null(frag, "Out of mem");

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
		frag = net_pkt_get_reserve_rx_data(10, K_SECONDS(1));
		zassert_not_null(frag, "Out of mem");

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
		frag = net_pkt_get_reserve_rx_data(10, K_SECONDS(1));
		zassert_not_null(frag, "Out of mem");

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
		zassert_true(0, "exiting");
	}

	chksum = ntohs(~net_calc_chksum(pkt, IPPROTO_ICMPV6));
	if (chksum != orig_chksum) {
		printk("Invalid chksum 0x%x in pkt3, should be 0x%x\n",
		       chksum, orig_chksum);
		zassert_true(0, "exiting");
	}
	net_pkt_unref(pkt);
#endif /* CONFIG_NET_IPV6 */

	/* Another packet that fits to one fragment.
	 * This one has ethernet header before IPv4 data.
	 */
#if defined(CONFIG_NET_IPV4)
	pkt = net_pkt_get_reserve_rx(0, K_SECONDS(1));
	zassert_not_null(pkt, "Out of mem");

	frag = net_pkt_get_reserve_rx_data(sizeof(struct net_eth_hdr),
					   K_SECONDS(1));
	zassert_not_null(frag, "Out of mem");

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
		zassert_true(0, "exiting");
	}
	net_pkt_unref(pkt);

	/* Another packet that fits to one fragment and which has correct
	 * checksum. This one has ethernet header before IPv4 data.
	 */
	pkt = net_pkt_get_reserve_rx(0, K_SECONDS(1));
	zassert_not_null(pkt, "Out of mem");

	frag = net_pkt_get_reserve_rx_data(sizeof(struct net_eth_hdr),
					   K_SECONDS(1));
	zassert_not_null(frag, "Out of mem");

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
		zassert_true(0, "exiting");
	}
	net_pkt_unref(pkt);
#endif /* CONFIG_NET_IPV4 */
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

static bool check_net_addr(struct net_addr_test_data *data)
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

void test_net_addr(void)
{
	int count, pass;

	for (count = 0, pass = 0; count < ARRAY_SIZE(tests); count++) {
		TC_START(tests[count].name);

		if (check_net_addr(tests[count].data)) {
			TC_END(PASS, "passed\n");
			pass++;
		} else {
			TC_END(FAIL, "failed\n");
		}
	}

	zassert_equal(pass, ARRAY_SIZE(tests), "check_net_addr error");
}

void test_addr_parse(void)
{
	struct sockaddr addr;
	bool ret;
	int i;
#if defined(CONFIG_NET_IPV4)
	static const struct {
		const char *address;
		int len;
		struct sockaddr_in result;
		bool verdict;
	} parse_ipv4_entries[] = {
		{
			.address = "192.0.2.1:80",
			.len = sizeof("192.0.2.1:80") - 1,
			.result = {
				.sin_family = AF_INET,
				.sin_port = htons(80),
				.sin_addr = {
					.s4_addr[0] = 192,
					.s4_addr[1] = 0,
					.s4_addr[2] = 2,
					.s4_addr[3] = 1
				}
			},
			.verdict = true
		},
		{
			.address = "192.0.2.2",
			.len = sizeof("192.0.2.2") - 1,
			.result = {
				.sin_family = AF_INET,
				.sin_port = 0,
				.sin_addr = {
					.s4_addr[0] = 192,
					.s4_addr[1] = 0,
					.s4_addr[2] = 2,
					.s4_addr[3] = 2
				}
			},
			.verdict = true
		},
		{
			.address = "192.0.2.3/foobar",
			.len = sizeof("192.0.2.3/foobar") - 8,
			.result = {
				.sin_family = AF_INET,
				.sin_port = 0,
				.sin_addr = {
					.s4_addr[0] = 192,
					.s4_addr[1] = 0,
					.s4_addr[2] = 2,
					.s4_addr[3] = 3
				}
			},
			.verdict = true
		},
		{
			.address = "255.255.255.255:0",
			.len = sizeof("255.255.255.255:0") - 1,
			.result = {
				.sin_family = AF_INET,
				.sin_port = 0,
				.sin_addr = {
					.s4_addr[0] = 255,
					.s4_addr[1] = 255,
					.s4_addr[2] = 255,
					.s4_addr[3] = 255
				}
			},
			.verdict = true
		},
		{
			.address = "127.0.0.42:65535",
			.len = sizeof("127.0.0.42:65535") - 1,
			.result = {
				.sin_family = AF_INET,
				.sin_port = htons(65535),
				.sin_addr = {
					.s4_addr[0] = 127,
					.s4_addr[1] = 0,
					.s4_addr[2] = 0,
					.s4_addr[3] = 42
				}
			},
			.verdict = true
		},
		{
			.address = "192.0.2.3:80/foobar",
			.len = sizeof("192.0.2.3:80/foobar") - 1,
			.verdict = false
		},
		{
			.address = "192.168.1.1:65536/foobar",
			.len = sizeof("192.168.1.1:65536") - 1,
			.verdict = false
		},
		{
			.address = "192.0.2.3:80/foobar",
			.len = sizeof("192.0.2.3") - 1,
			.result = {
				.sin_family = AF_INET,
				.sin_port = 0,
				.sin_addr = {
					.s4_addr[0] = 192,
					.s4_addr[1] = 0,
					.s4_addr[2] = 2,
					.s4_addr[3] = 3
				}
			},
			.verdict = true
		},
		{
			.address = "192.0.2.3/foobar",
			.len = sizeof("192.0.2.3/foobar") - 1,
			.verdict = false
		},
		{
			.address = "192.0.2.3:80:80",
			.len = sizeof("192.0.2.3:80:80") - 1,
			.verdict = false
		},
		{
			.address = "192.0.2.1:80000",
			.len = sizeof("192.0.2.1:80000") - 1,
			.verdict = false
		},
		{
			.address = "192.168.0.1",
			.len = sizeof("192.168.0.1:80000") - 1,
			.result = {
				.sin_family = AF_INET,
				.sin_port = 0,
				.sin_addr = {
					.s4_addr[0] = 192,
					.s4_addr[1] = 168,
					.s4_addr[2] = 0,
					.s4_addr[3] = 1
				}
			},
			.verdict = true
		},
		{
			.address = "a.b.c.d",
			.verdict = false
		},
	};
#endif
#if defined(CONFIG_NET_IPV6)
	static const struct {
		const char *address;
		int len;
		struct sockaddr_in6 result;
		bool verdict;
	} parse_ipv6_entries[] = {
		{
			.address = "[2001:db8::2]:80",
			.len = sizeof("[2001:db8::2]:80") - 1,
			.result = {
				.sin6_family = AF_INET6,
				.sin6_port = htons(80),
				.sin6_addr = {
					.s6_addr16[0] = ntohs(0x2001),
					.s6_addr16[1] = ntohs(0xdb8),
					.s6_addr16[3] = 0,
					.s6_addr16[4] = 0,
					.s6_addr16[5] = 0,
					.s6_addr16[6] = 0,
					.s6_addr16[7] = ntohs(2)
				}
			},
			.verdict = true
		},
		{
			.address = "[2001:db8::a]/barfoo",
			.len = sizeof("[2001:db8::a]/barfoo") - 8,
			.result = {
				.sin6_family = AF_INET6,
				.sin6_port = 0,
				.sin6_addr = {
					.s6_addr16[0] = ntohs(0x2001),
					.s6_addr16[1] = ntohs(0xdb8),
					.s6_addr16[3] = 0,
					.s6_addr16[4] = 0,
					.s6_addr16[5] = 0,
					.s6_addr16[6] = 0,
					.s6_addr16[7] = ntohs(0xa)
				}
			},
			.verdict = true
		},
		{
			.address = "[2001:db8::a]",
			.len = sizeof("[2001:db8::a]") - 1,
			.result = {
				.sin6_family = AF_INET6,
				.sin6_port = 0,
				.sin6_addr = {
					.s6_addr16[0] = ntohs(0x2001),
					.s6_addr16[1] = ntohs(0xdb8),
					.s6_addr16[3] = 0,
					.s6_addr16[4] = 0,
					.s6_addr16[5] = 0,
					.s6_addr16[6] = 0,
					.s6_addr16[7] = ntohs(0xa)
				}
			},
			.verdict = true
		},
		{
			.address = "[2001:db8:3:4:5:6:7:8]:65535",
			.len = sizeof("[2001:db8:3:4:5:6:7:8]:65535") - 1,
			.result = {
				.sin6_family = AF_INET6,
				.sin6_port = 65535,
				.sin6_addr = {
					.s6_addr16[0] = ntohs(0x2001),
					.s6_addr16[1] = ntohs(0xdb8),
					.s6_addr16[2] = ntohs(3),
					.s6_addr16[3] = ntohs(4),
					.s6_addr16[4] = ntohs(5),
					.s6_addr16[5] = ntohs(6),
					.s6_addr16[6] = ntohs(7),
					.s6_addr16[7] = ntohs(8),
				}
			},
			.verdict = true
		},
		{
			.address = "[::]:0",
			.len = sizeof("[::]:0") - 1,
			.result = {
				.sin6_family = AF_INET6,
				.sin6_port = 0,
				.sin6_addr = {
					.s6_addr16[0] = 0,
					.s6_addr16[1] = 0,
					.s6_addr16[2] = 0,
					.s6_addr16[3] = 0,
					.s6_addr16[4] = 0,
					.s6_addr16[5] = 0,
					.s6_addr16[6] = 0,
					.s6_addr16[7] = 0,
				}
			},
			.verdict = true
		},
		{
			.address = "2001:db8::42",
			.len = sizeof("2001:db8::42") - 1,
			.result = {
				.sin6_family = AF_INET6,
				.sin6_port = 0,
				.sin6_addr = {
					.s6_addr16[0] = ntohs(0x2001),
					.s6_addr16[1] = ntohs(0xdb8),
					.s6_addr16[3] = 0,
					.s6_addr16[4] = 0,
					.s6_addr16[5] = 0,
					.s6_addr16[6] = 0,
					.s6_addr16[7] = ntohs(0x42)
				}
			},
			.verdict = true
		},
		{
			.address = "[2001:db8::192.0.2.1]:80000",
			.len = sizeof("[2001:db8::192.0.2.1]:80000") - 1,
			.verdict = false
		},
		{
			.address = "[2001:db8::1]:80",
			.len = sizeof("[2001:db8::1") - 1,
			.verdict = false
		},
		{
			.address = "[2001:db8::1]:65536",
			.len = sizeof("[2001:db8::1]:65536") - 1,
			.verdict = false
		},
		{
			.address = "[2001:db8::1]:80",
			.len = sizeof("2001:db8::1") - 1,
			.verdict = false
		},
		{
			.address = "[2001:db8::1]:a",
			.len = sizeof("[2001:db8::1]:a") - 1,
			.verdict = false
		},
		{
			.address = "[2001:db8::1]:10-12",
			.len = sizeof("[2001:db8::1]:10-12") - 1,
			.verdict = false
		},
		{
			.address = "[2001:db8::]:80/url/continues",
			.len = sizeof("[2001:db8::]") - 1,
			.result = {
				.sin6_family = AF_INET6,
				.sin6_port = 0,
				.sin6_addr = {
					.s6_addr16[0] = ntohs(0x2001),
					.s6_addr16[1] = ntohs(0xdb8),
					.s6_addr16[3] = 0,
					.s6_addr16[4] = 0,
					.s6_addr16[5] = 0,
					.s6_addr16[6] = 0,
					.s6_addr16[7] = 0,
				}
			},
			.verdict = true
		},
		{
			.address = "[2001:db8::200]:080",
			.len = sizeof("[2001:db8:433:2]:80000") - 1,
			.result = {
				.sin6_family = AF_INET6,
				.sin6_port = htons(80),
				.sin6_addr = {
					.s6_addr16[0] = ntohs(0x2001),
					.s6_addr16[1] = ntohs(0xdb8),
					.s6_addr16[3] = 0,
					.s6_addr16[4] = 0,
					.s6_addr16[5] = 0,
					.s6_addr16[6] = 0,
					.s6_addr16[7] = ntohs(0x200)
				}
			},
			.verdict = true
		},
		{
			.address = "[2001:db8::]:8080/another/url",
			.len = sizeof("[2001:db8::]:8080/another/url") - 1,
			.verdict = false
		},
		{
			.address = "[2001:db8::1",
			.len = sizeof("[2001:db8::1") - 1,
			.verdict = false
		},
		{
			.address = "[2001:db8::1]:-1",
			.len = sizeof("[2001:db8::1]:-1") - 1,
			.verdict = false
		},
		{
			/* Valid although user probably did not mean this */
			.address = "2001:db8::1:80",
			.len = sizeof("2001:db8::1:80") - 1,
			.result = {
				.sin6_family = AF_INET6,
				.sin6_port = 0,
				.sin6_addr = {
					.s6_addr16[0] = ntohs(0x2001),
					.s6_addr16[1] = ntohs(0xdb8),
					.s6_addr16[3] = 0,
					.s6_addr16[4] = 0,
					.s6_addr16[5] = 0,
					.s6_addr16[6] = ntohs(0x01),
					.s6_addr16[7] = ntohs(0x80)
				}
			},
			.verdict = true
		},
	};
#endif

#if defined(CONFIG_NET_IPV4)
	for (i = 0; i < ARRAY_SIZE(parse_ipv4_entries) - 1; i++) {
		(void)memset(&addr, 0, sizeof(addr));

		ret = net_ipaddr_parse(
			parse_ipv4_entries[i].address,
			parse_ipv4_entries[i].len,
			&addr);
		if (ret != parse_ipv4_entries[i].verdict) {
			printk("IPv4 entry [%d] \"%s\" failed\n", i,
				parse_ipv4_entries[i].address);
			zassert_true(false, "failure");
		}

		if (ret == true) {
			zassert_true(
				net_ipv4_addr_cmp(
				      &net_sin(&addr)->sin_addr,
				      &parse_ipv4_entries[i].result.sin_addr),
				parse_ipv4_entries[i].address);
			zassert_true(net_sin(&addr)->sin_port ==
				     parse_ipv4_entries[i].result.sin_port,
				     "IPv4 port");
			zassert_true(net_sin(&addr)->sin_family ==
				     parse_ipv4_entries[i].result.sin_family,
				     "IPv4 family");
		}
	}
#endif
#if defined(CONFIG_NET_IPV6)
	for (i = 0; i < ARRAY_SIZE(parse_ipv6_entries) - 1; i++) {
		(void)memset(&addr, 0, sizeof(addr));

		ret = net_ipaddr_parse(
			parse_ipv6_entries[i].address,
			parse_ipv6_entries[i].len,
			&addr);
		if (ret != parse_ipv6_entries[i].verdict) {
			printk("IPv6 entry [%d] \"%s\" failed\n", i,
			       parse_ipv6_entries[i].address);
			zassert_true(false, "failure");
		}

		if (ret == true) {
			zassert_true(
				net_ipv6_addr_cmp(
				      &net_sin6(&addr)->sin6_addr,
				      &parse_ipv6_entries[i].result.sin6_addr),
				parse_ipv6_entries[i].address);
			zassert_true(net_sin6(&addr)->sin6_port ==
				     parse_ipv6_entries[i].result.sin6_port,
				     "IPv6 port");
			zassert_true(net_sin6(&addr)->sin6_family ==
				     parse_ipv6_entries[i].result.sin6_family,
				     "IPv6 family");
		}
	}
#endif
}

void test_net_pkt_addr_parse(void)
{
#if defined(CONFIG_NET_IPV6)
	static struct ipv6_test_data {
		const unsigned char *payload;
		int payload_len;
		struct sockaddr_in6 src;
		struct sockaddr_in6 dst;
	} ipv6_test_data_set[] = {
		{
			.payload = v6_udp_pkt1,
			.payload_len = sizeof(v6_udp_pkt1),
			.src = {
				.sin6_family = AF_INET6,
				.sin6_port = htons(5353),
				.sin6_addr = {
					.s6_addr16[0] = ntohs(0xfe80),
					.s6_addr16[1] = 0,
					.s6_addr16[2] = 0,
					.s6_addr16[3] = 0,
					.s6_addr16[4] = ntohs(0x14a7),
					.s6_addr16[5] = ntohs(0xae7d),
					.s6_addr16[6] = ntohs(0xf804),
					.s6_addr16[7] = ntohs(0x0ca5),
				},
			},
			.dst = {
				.sin6_family = AF_INET6,
				.sin6_port = htons(5353),
				.sin6_addr = {
					.s6_addr16[0] = ntohs(0xff02),
					.s6_addr16[1] = 0,
					.s6_addr16[2] = 0,
					.s6_addr16[3] = 0,
					.s6_addr16[4] = 0,
					.s6_addr16[5] = 0,
					.s6_addr16[6] = 0,
					.s6_addr16[7] = ntohs(0x00fb),
				},
			},

		},
		{
			.payload = v6_tcp_pkt1,
			.payload_len = sizeof(v6_tcp_pkt1),
			.src = {
				.sin6_family = AF_INET6,
				.sin6_port = htons(62032),
				.sin6_addr = {
					.s6_addr16[0] = ntohs(0xfe80),
					.s6_addr16[1] = 0,
					.s6_addr16[2] = 0,
					.s6_addr16[3] = 0,
					.s6_addr16[4] = ntohs(0xf921),
					.s6_addr16[5] = ntohs(0x19da),
					.s6_addr16[6] = ntohs(0xf89d),
					.s6_addr16[7] = ntohs(0xf9a0),
				},
			},
			.dst = {
				.sin6_family = AF_INET6,
				.sin6_port = htons(4242),
				.sin6_addr = {
					.s6_addr16[0] = ntohs(0xfe80),
					.s6_addr16[1] = 0,
					.s6_addr16[2] = 0,
					.s6_addr16[3] = 0,
					.s6_addr16[4] = ntohs(0x02e0),
					.s6_addr16[5] = ntohs(0x4cff),
					.s6_addr16[6] = ntohs(0xfe36),
					.s6_addr16[7] = ntohs(0x03a1),
				},
			},

		},
	};
#endif

#if defined(CONFIG_NET_IPV4)
	static struct ipv4_test_data {
		const unsigned char *payload;
		int payload_len;
		struct sockaddr_in src;
		struct sockaddr_in dst;
	} ipv4_test_data_set[] = {
		{
			.payload = v4_tcp_pkt1,
			.payload_len = sizeof(v4_tcp_pkt1),
			.src = {
				.sin_family = AF_INET,
				.sin_port = htons(22),
				.sin_addr = {
					.s4_addr[0] = 192,
					.s4_addr[1] = 168,
					.s4_addr[2] = 1,
					.s4_addr[3] = 219,
				},
			},
			.dst = {
				.sin_family = AF_INET,
				.sin_port = htons(52253),
				.sin_addr = {
					.s4_addr[0] = 192,
					.s4_addr[1] = 168,
					.s4_addr[2] = 1,
					.s4_addr[3] = 218,
				},
			},

		},
		{
			.payload = v4_udp_pkt1,
			.payload_len = sizeof(v4_udp_pkt1),
			.src = {
				.sin_family = AF_INET,
				.sin_port = htons(64426),
				.sin_addr = {
					.s4_addr[0] = 192,
					.s4_addr[1] = 168,
					.s4_addr[2] = 1,
					.s4_addr[3] = 217,
				},
			},
			.dst = {
				.sin_family = AF_INET,
				.sin_port = htons(4242),
				.sin_addr = {
					.s4_addr[0] = 192,
					.s4_addr[1] = 168,
					.s4_addr[2] = 1,
					.s4_addr[3] = 219,
				},
			},

		},
	};
#endif

#if defined(CONFIG_NET_IPV6)
	for (int i = 0; i < ARRAY_SIZE(ipv6_test_data_set); i++) {
		struct net_pkt *pkt;
		struct net_buf *frag;
		struct sockaddr_in6 addr;
		struct ipv6_test_data *data = &ipv6_test_data_set[i];

		pkt = net_pkt_get_reserve_rx(0, K_SECONDS(1));
		zassert_not_null(pkt, "Out of mem");

		frag = net_pkt_get_reserve_rx_data(sizeof(struct net_eth_hdr),
						   K_SECONDS(1));
		zassert_not_null(frag, "Out of mem");

		net_pkt_frag_add(pkt, frag);

		/* Copy ll header */
		net_pkt_set_ll_reserve(pkt, sizeof(struct net_eth_hdr));
		memcpy(net_pkt_ll(pkt), data->payload,
		       sizeof(struct net_eth_hdr));

		/* Copy remain data */
		net_pkt_append(pkt,
			       data->payload_len - sizeof(struct net_eth_hdr),
			       data->payload + sizeof(struct net_eth_hdr),
			       K_FOREVER);

		net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv6_hdr));
		net_pkt_set_family(pkt, AF_INET6);

		zassert_equal(net_pkt_get_src_addr(pkt,
						   (struct sockaddr *)&addr,
						   sizeof(addr)),
			      0,
			      "parse failed");
		zassert_equal(addr.sin6_port, data->src.sin6_port,
			      "wrong src port");
		zassert_true(net_ipv6_addr_cmp(&addr.sin6_addr,
					       &data->src.sin6_addr),
			     "wrong src addr");

		zassert_equal(net_pkt_get_dst_addr(pkt,
						   (struct sockaddr *)&addr,
						   sizeof(addr)),
			      0,
			      "parse failed");
		zassert_equal(addr.sin6_port, data->dst.sin6_port,
			      "wrong dst port");
		zassert_true(net_ipv6_addr_cmp(&addr.sin6_addr,
					       &data->dst.sin6_addr),
			     "unexpected dst addr");

		net_pkt_unref(pkt);
	}
#endif

#if defined(CONFIG_NET_IPV4)
	for (int i = 0; i < ARRAY_SIZE(ipv4_test_data_set); i++) {
		struct net_pkt *pkt;
		struct net_buf *frag;
		struct sockaddr_in addr;
		struct ipv4_test_data *data = &ipv4_test_data_set[i];

		pkt = net_pkt_get_reserve_rx(0, K_SECONDS(1));
		zassert_not_null(pkt, "Out of mem");

		frag = net_pkt_get_reserve_rx_data(sizeof(struct net_eth_hdr),
						   K_SECONDS(1));
		zassert_not_null(frag, "Out of mem");

		net_pkt_frag_add(pkt, frag);

		/* Copy ll header */
		net_pkt_set_ll_reserve(pkt, sizeof(struct net_eth_hdr));
		memcpy(net_pkt_ll(pkt), data->payload,
		       sizeof(struct net_eth_hdr));

		/* Copy remain data */
		net_pkt_append(pkt,
			       data->payload_len - sizeof(struct net_eth_hdr),
			       data->payload + sizeof(struct net_eth_hdr),
			       K_FOREVER);

		net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv4_hdr));
		net_pkt_set_family(pkt, AF_INET);

		zassert_equal(net_pkt_get_src_addr(pkt,
						   (struct sockaddr *)&addr,
						   sizeof(addr)),
			      0,
			      "parse failed");
		zassert_equal(addr.sin_port, data->src.sin_port,
			      "wrong src port");
		zassert_true(net_ipv4_addr_cmp(&addr.sin_addr,
					       &data->src.sin_addr),
			     "wrong src addr");

		zassert_equal(net_pkt_get_dst_addr(pkt,
						   (struct sockaddr *)&addr,
						   sizeof(addr)),
			      0,
			      "parse failed");
		zassert_equal(addr.sin_port, data->dst.sin_port,
			      "wrong dst port");
		zassert_true(net_ipv4_addr_cmp(&addr.sin_addr,
					       &data->dst.sin_addr),
			     "unexpected dst addr");

		net_pkt_unref(pkt);
	}
#endif
}

void test_main(void)
{
	ztest_test_suite(test_utils_fn,
			 ztest_unit_test(test_utils),
			 ztest_unit_test(test_net_addr),
			 ztest_unit_test(test_addr_parse),
			 ztest_unit_test(test_net_pkt_addr_parse));

	ztest_run_test_suite(test_utils_fn);
}
