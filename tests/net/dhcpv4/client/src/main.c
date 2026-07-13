/* main.c - Application main entry point */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_DHCPV4_LOG_LEVEL);

#include <zephyr/kernel.h>
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
#include <zephyr/net/dhcpv4.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/dummy.h>
#include <zephyr/sys/util.h>

#include "ipv4.h"
#include "udp_internal.h"
#include "dhcpv4/dhcpv4_internal.h"

#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

#define NET_LOG_ENABLED 1
#include "net_private.h"

/* Sample DHCP offer (420 bytes) */
static const unsigned char offer[] = {
0x02, 0x01, 0x06, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x0a, 0xed, 0x48, 0x9e, 0x0a, 0xb8,
0x09, 0x01, 0x0a, 0xed, 0x48, 0x02, 0x00, 0x00,
0x5E, 0x00, 0x53, 0x01, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* Magic cookie: DHCP */
0x63, 0x82, 0x53, 0x63,
/* [0] Pad option */
0x00,
/* [53] DHCP Message Type: OFFER */
0x35, 0x01, 0x02,
/* [1] Subnet Mask: 255.255.255.0 */
0x01, 0x04, 0xff, 0xff, 0xff, 0x00,
/* [58] Renewal Time Value: (21600s) 6 hours */
0x3a, 0x04, 0x00, 0x00, 0x54, 0x60,
/* [59] Rebinding Time Value: (37800s) 1 hour 30 min */
0x3b, 0x04, 0x00, 0x00, 0x93, 0xa8,
/* [51] IP Address Lease Time: (43200s) 12 hours */
0x33, 0x04, 0x00, 0x00, 0xa8, 0xc0,
/* [54] DHCP Server Identifier: 10.184.9.1 */
0x36, 0x04, 0x0a, 0xb8, 0x09, 0x01,
/* [3] Router: 10.237.72.1 */
0x03, 0x04, 0x0a, 0xed, 0x48, 0x01,
/* [15] Domain Name: fi.intel.com */
0x0f, 0x0d, 0x66, 0x69, 0x2e, 0x69, 0x6e, 0x74, 0x65, 0x6c, 0x2e, 0x63, 0x6f, 0x6d, 0x00,
/* [6] Domain Name Server: 10.248.2.1 163.33.253.68 10.184.9.1 */
0x06, 0x0c, 0x0a, 0xf8, 0x02, 0x01, 0xa3, 0x21, 0xfd, 0x44, 0x0a, 0xb8, 0x09, 0x01,
/* [119] Domain Search Option: fi.intel.com ger.corp.intel.com corp.intel.com intel.com */
0x77, 0x3d, 0x02, 0x66, 0x69, 0x05, 0x69, 0x6e,
0x74, 0x65, 0x6c, 0x03, 0x63, 0x6f, 0x6d, 0x00,
0x03, 0x67, 0x65, 0x72, 0x04, 0x63, 0x6f, 0x72,
0x70, 0x05, 0x69, 0x6e, 0x74, 0x65, 0x6c, 0x03,
0x63, 0x6f, 0x6d, 0x00, 0x04, 0x63, 0x6f, 0x72,
0x70, 0x05, 0x69, 0x6e, 0x74, 0x65, 0x6c, 0x03,
0x63, 0x6f, 0x6d, 0x00, 0x05, 0x69, 0x6e, 0x74,
0x65, 0x6c, 0x03, 0x63, 0x6f, 0x6d, 0x00,
/* [44] NetBIOS Name Servers: 163.33.7.86, 143.182.250.105 */
0x2c, 0x08, 0xa3, 0x21, 0x07, 0x56, 0x8f, 0xb6, 0xfa, 0x69,
/* [43] Encapsulated vendor specific information */
0x2b, 0x0a,
	    /* [1]: "string" */
	    0x01, 0x07, 0x73, 0x74, 0x72, 0x69, 0x6e, 0x67, 0x00,
	    /* End marker */
	    0xff,
/* [43] Encapsulated vendor specific information */
0x2b, 0x0f,
	    /* [2]: single byte of value 1 */
	    0x02, 0x01, 0x01,
	    /* [3]: zero-length option */
	    0x03, 0x00,
	    /* [254]: invalid option (size longer than remainder of opt 43 size) */
	    0xfe, 0x10, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe,
/* [43] Too short encapsulated vendor option (only single byte) */
0x2b, 0x01,
	    /* [254]: invalid option (no length in opt 43) */
	    0xfe,
/* [70] POP3 Server: 198.51.100.16 */
0x46, 0x04, 0xc6, 0x33, 0x64, 0x10,
/* End marker */
0xff
};

/* Sample DHCPv4 ACK */
static const unsigned char ack[] = {
0x02, 0x01, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x0a, 0xed, 0x48, 0x9e, 0x00, 0x00, 0x00, 0x00,
0x0a, 0xed, 0x48, 0x03, 0x00, 0x00, 0x5E, 0x00,
0x53, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00,
/* Magic cookie: DHCP */
0x63, 0x82, 0x53, 0x63,
/* [0] Pad option */
0x00,
/* [53] DHCP Message Type: ACK */
0x35, 0x01, 0x05,
/* [58] Renewal Time Value: (21600s) 6 hours */
0x3a, 0x04, 0x00, 0x00, 0x54, 0x60,
/* [59] Rebinding Time Value: (37800s) 1 hour 30 min */
0x3b, 0x04, 0x00, 0x00, 0x93, 0xa8,
/* [51] IP Address Lease Time: (43200s) 12 hours */
0x33, 0x04, 0x00, 0x00, 0xa8, 0xc0,
/* [54] DHCP Server Identifier: 10.184.9.1 */
0x36, 0x04, 0x0a, 0xb8, 0x09, 0x01,
/* [1] Subnet Mask: 255.255.255.0 */
0x01, 0x04, 0xff, 0xff, 0xff, 0x00,
/* [3] Router: 10.237.72.1 */
0x03, 0x04, 0x0a, 0xed, 0x48, 0x01,
/* [15] Domain Name: fi.intel.com */
0x0f, 0x0d, 0x66, 0x69, 0x2e, 0x69, 0x6e, 0x74, 0x65, 0x6c, 0x2e, 0x63, 0x6f, 0x6d, 0x00,
/* [6] Domain Name Server: 10.248.2.1 163.33.253.68 10.184.9.1 */
0x06, 0x0c, 0x0a, 0xf8, 0x02, 0x01, 0xa3, 0x21, 0xfd, 0x44, 0x0a, 0xb8, 0x09, 0x01,
/* [119] Domain Search Option: fi.intel.com ger.corp.intel.com corp.intel.com intel.com */
0x77, 0x3d, 0x02, 0x66, 0x69, 0x05, 0x69, 0x6e,
0x74, 0x65, 0x6c, 0x03, 0x63, 0x6f, 0x6d, 0x00,
0x03, 0x67, 0x65, 0x72, 0x04, 0x63, 0x6f, 0x72,
0x70, 0x05, 0x69, 0x6e, 0x74, 0x65, 0x6c, 0x03,
0x63, 0x6f, 0x6d, 0x00, 0x04, 0x63, 0x6f, 0x72,
0x70, 0x05, 0x69, 0x6e, 0x74, 0x65, 0x6c, 0x03,
0x63, 0x6f, 0x6d, 0x00, 0x05, 0x69, 0x6e, 0x74,
0x65, 0x6c, 0x03, 0x63, 0x6f, 0x6d, 0x00,
/* [44] NetBIOS Name Servers: 163.33.7.86, 143.182.250.105 */
0x2c, 0x08, 0xa3, 0x21, 0x07, 0x56, 0x8f, 0xb6, 0xfa, 0x69,
/* [43] Encapsulated vendor specific information */
0x2b, 0x0a,
	    /* [1]: "string" */
	    0x01, 0x07, 0x73, 0x74, 0x72, 0x69, 0x6e, 0x67, 0x00,
	    /* End marker */
	    0xff,
/* [43] Encapsulated vendor specific information */
0x2b, 0x0f,
	    /* [2]: single byte of value 1 */
	    0x02, 0x01, 0x01,
	    /* [3]: zero-length option */
	    0x03, 0x00,
	    /* [254]: invalid option (size longer than remainder of opt 43 size) */
	    0xfe, 0x10, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe,
/* [43] Too short encapsulated vendor option (only single byte) */
0x2b, 0x01,
	    /* [254]: invalid option (no length in opt 43) */
	    0xfe,
/* [70] POP3 Server: 198.51.100.16 */
0x46, 0x04, 0xc6, 0x33, 0x64, 0x10,
/* End marker */
0xff
};

/* Same as ack[] but without option 6 (DNS server). */
static const unsigned char ack_no_dns[] = {
0x02, 0x01, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x0a, 0xed, 0x48, 0x9e, 0x00, 0x00, 0x00, 0x00,
0x0a, 0xed, 0x48, 0x03, 0x00, 0x00, 0x5E, 0x00,
0x53, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00,
/* Magic cookie: DHCP */
0x63, 0x82, 0x53, 0x63,
/* [0] Pad option */
0x00,
/* [53] DHCP Message Type: ACK */
0x35, 0x01, 0x05,
/* [58] Renewal Time Value: (21600s) 6 hours */
0x3a, 0x04, 0x00, 0x00, 0x54, 0x60,
/* [59] Rebinding Time Value: (37800s) 1 hour 30 min */
0x3b, 0x04, 0x00, 0x00, 0x93, 0xa8,
/* [51] IP Address Lease Time: (43200s) 12 hours */
0x33, 0x04, 0x00, 0x00, 0xa8, 0xc0,
/* [54] DHCP Server Identifier: 10.184.9.1 */
0x36, 0x04, 0x0a, 0xb8, 0x09, 0x01,
/* [1] Subnet Mask: 255.255.255.0 */
0x01, 0x04, 0xff, 0xff, 0xff, 0x00,
/* [3] Router: 10.237.72.1 */
0x03, 0x04, 0x0a, 0xed, 0x48, 0x01,
/* [15] Domain Name: fi.intel.com */
0x0f, 0x0d, 0x66, 0x69, 0x2e, 0x69, 0x6e, 0x74, 0x65, 0x6c, 0x2e, 0x63, 0x6f, 0x6d, 0x00,
/* [119] Domain Search Option: fi.intel.com ger.corp.intel.com corp.intel.com intel.com */
0x77, 0x3d, 0x02, 0x66, 0x69, 0x05, 0x69, 0x6e,
0x74, 0x65, 0x6c, 0x03, 0x63, 0x6f, 0x6d, 0x00,
0x03, 0x67, 0x65, 0x72, 0x04, 0x63, 0x6f, 0x72,
0x70, 0x05, 0x69, 0x6e, 0x74, 0x65, 0x6c, 0x03,
0x63, 0x6f, 0x6d, 0x00, 0x04, 0x63, 0x6f, 0x72,
0x70, 0x05, 0x69, 0x6e, 0x74, 0x65, 0x6c, 0x03,
0x63, 0x6f, 0x6d, 0x00, 0x05, 0x69, 0x6e, 0x74,
0x65, 0x6c, 0x03, 0x63, 0x6f, 0x6d, 0x00,
/* [44] NetBIOS Name Servers: 163.33.7.86, 143.182.250.105 */
0x2c, 0x08, 0xa3, 0x21, 0x07, 0x56, 0x8f, 0xb6, 0xfa, 0x69,
/* [43] Encapsulated vendor specific information */
0x2b, 0x0a,
	    /* [1]: "string" */
	    0x01, 0x07, 0x73, 0x74, 0x72, 0x69, 0x6e, 0x67, 0x00,
	    /* End marker */
	    0xff,
/* [43] Encapsulated vendor specific information */
0x2b, 0x0f,
	    /* [2]: single byte of value 1 */
	    0x02, 0x01, 0x01,
	    /* [3]: zero-length option */
	    0x03, 0x00,
	    /* [254]: invalid option (size longer than remainder of opt 43 size) */
	    0xfe, 0x10, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe,
/* [43] Too short encapsulated vendor option (only single byte) */
0x2b, 0x01,
	    /* [254]: invalid option (no length in opt 43) */
	    0xfe,
/* [70] POP3 Server: 198.51.100.16 */
0x46, 0x04, 0xc6, 0x33, 0x64, 0x10,
/* End marker */
0xff
};

static const struct net_in_addr server_addr = { { { 192, 0, 2, 1 } } };
static const struct net_in_addr client_addr = { { { 255, 255, 255, 255 } } };

#define SERVER_PORT		67
#define CLIENT_PORT		68
#define MSG_TYPE		53
#define DISCOVER		1
#define REQUEST			3
#define OPTION_DNS_SERVER	6
#define OPTION_REQ_IPADDR	50
#define OPTION_SERVER_ID	54
#define OPTION_REQ_LIST		55
#define OPTION_DOMAIN		15
#define OPTION_POP3		70
#define OPTION_VENDOR_STRING	1
#define OPTION_VENDOR_BYTE	2
#define OPTION_VENDOR_EMPTY	3
#define OPTION_INVALID		254

#define MAX_REQ_OPTIONS 16

struct dhcp_client_msg {
	uint32_t xid;
	uint8_t type;
	bool has_requested_ip;
	bool has_server_id;
	uint8_t req_options[MAX_REQ_OPTIONS];
	uint8_t req_options_cnt;
};

static uint32_t offer_xid;
static uint32_t request_xid;
static bool strict_dhcp_server;
static bool discover_req_included_dns;
static bool init_reboot_req_included_dns;
static bool reject_init_reboot;

#define EVT_ADDR_ADD        BIT(0)
#define EVT_ADDR_DEL        BIT(1)
#define EVT_DNS_SERVER1_ADD BIT(2)
#define EVT_DNS_SERVER2_ADD BIT(3)
#define EVT_DNS_SERVER3_ADD BIT(4)
#define EVT_DHCP_START      BIT(5)
#define EVT_DHCP_BOUND      BIT(6)
#define EVT_DHCP_STOP       BIT(7)
#define EVT_OPTION_DOMAIN   BIT(8)
#define EVT_OPTION_POP3     BIT(9)
#define EVT_VENDOR_STRING   BIT(10)
#define EVT_VENDOR_BYTE     BIT(11)
#define EVT_VENDOR_EMPTY    BIT(12)
#define EVT_DHCP_OFFER      BIT(13)
#define EVT_DHCP_ACK        BIT(14)
#define EVT_DNS_SERVER1_DEL BIT(15)
#define EVT_DNS_SERVER2_DEL BIT(16)
#define EVT_DNS_SERVER3_DEL BIT(17)
#define EVT_DHCP_NAK        BIT(18)

static K_EVENT_DEFINE(events);

static void dhcp_test_reset_iface(struct net_if *iface)
{
	net_dhcpv4_stop(iface);
	iface->config.dhcpv4.requested_ip.s_addr = 0;
	k_event_set(&events, 0U);
	offer_xid = 0U;
	request_xid = 0U;
	strict_dhcp_server = false;
	discover_req_included_dns = false;
	init_reboot_req_included_dns = false;
	reject_init_reboot = false;
}

static void dhcpv4_tests_before(void *fixture)
{
	struct net_if *iface;

	ARG_UNUSED(fixture);

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));
	zassert_not_null(iface, "Interface not available");

	dhcp_test_reset_iface(iface);
}

#define WAIT_TIME K_SECONDS(CONFIG_NET_DHCPV4_INITIAL_DELAY_MAX + 1)

struct net_dhcpv4_context {
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
};

static int net_dhcpv4_dev_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static uint8_t *net_dhcpv4_get_mac(const struct device *dev)
{
	struct net_dhcpv4_context *context = dev->data;

	if (context->mac_addr[2] == 0x00) {
		/* 00-00-5E-00-53-xx Documentation RFC 7042 */
		context->mac_addr[0] = 0x00;
		context->mac_addr[1] = 0x00;
		context->mac_addr[2] = 0x5E;
		context->mac_addr[3] = 0x00;
		context->mac_addr[4] = 0x53;
		context->mac_addr[5] = 0x01;
	}

	return context->mac_addr;
}

static void net_dhcpv4_iface_init(struct net_if *iface)
{
	uint8_t *mac = net_dhcpv4_get_mac(net_if_get_device(iface));

	net_if_set_link_addr(iface, mac, 6, NET_LINK_ETHERNET);
}

struct net_pkt *prepare_dhcp_offer(struct net_if *iface, uint32_t xid)
{
	struct net_pkt *pkt;

	pkt = net_pkt_alloc_with_buffer(iface, sizeof(offer), NET_AF_INET,
					NET_IPPROTO_UDP, K_FOREVER);
	if (!pkt) {
		return NULL;
	}

	net_pkt_set_ipv4_ttl(pkt, 0xFF);

	if (net_ipv4_create(pkt, &server_addr, &client_addr) ||
	    net_udp_create(pkt, net_htons(SERVER_PORT), net_htons(CLIENT_PORT))) {
		goto fail;
	}

	if (net_pkt_write(pkt, offer, 4)) {
		goto fail;
	}

	/* Update xid from the client request */
	if (net_pkt_write_be32(pkt, xid)) {
		goto fail;
	}

	if (net_pkt_write(pkt, offer + 8, sizeof(offer) - 8)) {
		goto fail;
	}

	net_pkt_cursor_init(pkt);

	net_ipv4_finalize(pkt, NET_IPPROTO_UDP);

	offer_xid = xid;

	return pkt;

fail:
	net_pkt_unref(pkt);
	return NULL;
}

struct net_pkt *prepare_dhcp_ack(struct net_if *iface, uint32_t xid, bool include_dns)
{
	const unsigned char *reply = include_dns ? ack : ack_no_dns;
	size_t reply_len = include_dns ? sizeof(ack) : sizeof(ack_no_dns);
	struct net_pkt *pkt;

	pkt = net_pkt_alloc_with_buffer(iface, reply_len, NET_AF_INET,
					NET_IPPROTO_UDP, K_FOREVER);
	if (!pkt) {
		return NULL;
	}

	net_pkt_set_ipv4_ttl(pkt, 0xFF);

	if (net_ipv4_create(pkt, &server_addr, &client_addr) ||
	    net_udp_create(pkt, net_htons(SERVER_PORT), net_htons(CLIENT_PORT))) {
		goto fail;
	}

	if (net_pkt_write(pkt, reply, 4)) {
		goto fail;
	}

	/* Update xid from the client request */
	if (net_pkt_write_be32(pkt, xid)) {
		goto fail;
	}

	if (net_pkt_write(pkt, reply + 8, reply_len - 8)) {
		goto fail;
	}

	net_pkt_cursor_init(pkt);

	net_ipv4_finalize(pkt, NET_IPPROTO_UDP);

	return pkt;

fail:
	net_pkt_unref(pkt);
	return NULL;
}

static struct net_pkt *prepare_dhcp_nak(struct net_if *iface, uint32_t xid)
{
	static const uint8_t cookie[] = { 0x63, 0x82, 0x53, 0x63 };
	struct dhcp_msg msg = { 0 };
	uint8_t empty_buf[SIZE_OF_FILE] = { 0 };
	struct net_pkt *pkt;

	pkt = net_pkt_alloc_with_buffer(iface, DHCPV4_MESSAGE_SIZE, NET_AF_INET,
					NET_IPPROTO_UDP, K_FOREVER);
	if (pkt == NULL) {
		return NULL;
	}

	net_pkt_set_ipv4_ttl(pkt, 0xFF);

	if (net_ipv4_create(pkt, &server_addr, &client_addr) ||
	    net_udp_create(pkt, net_htons(SERVER_PORT), net_htons(CLIENT_PORT))) {
		goto fail;
	}

	msg.op = DHCPV4_MSG_BOOT_REPLY;
	msg.htype = HARDWARE_ETHERNET_TYPE;
	msg.hlen = net_if_get_link_addr(iface)->len;
	msg.xid = net_htonl(xid);
	memcpy(msg.chaddr, net_if_get_link_addr(iface)->addr,
	       net_if_get_link_addr(iface)->len);

	if (net_pkt_write(pkt, &msg, sizeof(msg)) ||
	    net_pkt_write(pkt, empty_buf, SIZE_OF_SNAME) ||
	    net_pkt_write(pkt, empty_buf, SIZE_OF_FILE) ||
	    net_pkt_write(pkt, cookie, sizeof(cookie)) ||
	    net_pkt_write_u8(pkt, DHCPV4_OPTIONS_MSG_TYPE) ||
	    net_pkt_write_u8(pkt, 1) ||
	    net_pkt_write_u8(pkt, NET_DHCPV4_MSG_TYPE_NAK) ||
	    net_pkt_write_u8(pkt, DHCPV4_OPTIONS_END)) {
		goto fail;
	}

	net_pkt_cursor_init(pkt);
	net_ipv4_finalize(pkt, NET_IPPROTO_UDP);

	return pkt;

fail:
	net_pkt_unref(pkt);
	return NULL;
}

static bool dhcp_msg_req_list_contains(const struct dhcp_client_msg *msg, uint8_t option)
{
	for (uint8_t i = 0; i < msg->req_options_cnt; i++) {
		if (msg->req_options[i] == option) {
			return true;
		}
	}

	return false;
}

static int parse_dhcp_client_message(struct net_pkt *pkt, struct dhcp_client_msg *msg)
{
	memset(msg, 0, sizeof(*msg));

	if (net_pkt_skip(pkt, NET_IPV4UDPH_LEN + 4)) {
		return -EINVAL;
	}

	if (net_pkt_read_be32(pkt, &msg->xid)) {
		return -EINVAL;
	}

	if (net_pkt_skip(pkt, 36 + 64 + 128 + 4)) {
		return -EINVAL;
	}

	while (true) {
		uint8_t opt;
		uint8_t len;

		if (net_pkt_read_u8(pkt, &opt)) {
			return -EINVAL;
		}

		if (opt == 0) {
			continue;
		}

		if (opt == 255) {
			break;
		}

		if (net_pkt_read_u8(pkt, &len)) {
			return -EINVAL;
		}

		if (opt == MSG_TYPE) {
			if (len != 1U) {
				return -EINVAL;
			}

			if (net_pkt_read_u8(pkt, &msg->type)) {
				return -EINVAL;
			}

			if (msg->type == NET_DHCPV4_MSG_TYPE_REQUEST) {
				request_xid = msg->xid;
			}

			continue;
		}

		if (opt == OPTION_REQ_LIST) {
			uint8_t to_read = MIN(len, MAX_REQ_OPTIONS - msg->req_options_cnt);

			for (uint8_t i = 0; i < to_read; i++) {
				if (net_pkt_read_u8(pkt,
						    &msg->req_options[msg->req_options_cnt++])) {
					return -EINVAL;
				}
			}

			if (len > to_read && net_pkt_skip(pkt, len - to_read)) {
				return -EINVAL;
			}

			continue;
		}

		if (opt == OPTION_REQ_IPADDR && len == 4U) {
			msg->has_requested_ip = true;
		} else if (opt == OPTION_SERVER_ID && len == 4U) {
			msg->has_server_id = true;
		}

		if (len > 0U && net_pkt_skip(pkt, len)) {
			return -EINVAL;
		}
	}

	return msg->type != 0U ? 0 : -EINVAL;
}

static int tester_send(const struct device *dev, struct net_pkt *pkt)
{
	struct net_pkt *rpkt;
	struct dhcp_client_msg msg;
	bool dns_requested;
	bool is_init_reboot;

	ARG_UNUSED(dev);

	(void)memset(&msg, 0, sizeof(msg));

	if (!pkt->frags) {
		TC_PRINT("No data to send!\n");

		return -ENODATA;
	}

	if (parse_dhcp_client_message(pkt, &msg) < 0) {
		return -EINVAL;
	}

	if (msg.type == DISCOVER) {
		if (strict_dhcp_server) {
			discover_req_included_dns =
				dhcp_msg_req_list_contains(&msg, OPTION_DNS_SERVER);
		}

		/* Reply with DHCPv4 offer message */
		rpkt = prepare_dhcp_offer(net_pkt_iface(pkt), msg.xid);
		if (!rpkt) {
			return -EINVAL;
		}
		k_event_post(&events, EVT_DHCP_OFFER);
	} else if (msg.type == REQUEST) {
		bool include_dns;
		bool nak_reply;

		dns_requested = dhcp_msg_req_list_contains(&msg, OPTION_DNS_SERVER);
		is_init_reboot = msg.has_requested_ip && !msg.has_server_id;

		if (strict_dhcp_server && is_init_reboot) {
			init_reboot_req_included_dns = dns_requested;
			include_dns = dns_requested;
		} else {
			include_dns = true;
		}

		nak_reply = reject_init_reboot && is_init_reboot;

		if (nak_reply) {
			rpkt = prepare_dhcp_nak(net_pkt_iface(pkt), msg.xid);
		} else {
			rpkt = prepare_dhcp_ack(net_pkt_iface(pkt), msg.xid, include_dns);
		}
		if (!rpkt) {
			return -EINVAL;
		}
		k_event_post(&events, nak_reply ? EVT_DHCP_NAK : EVT_DHCP_ACK);
	} else {
		/* Invalid message type received */
		return -EINVAL;
	}

	if (net_recv_data(net_pkt_iface(rpkt), rpkt)) {
		net_pkt_unref(rpkt);

		return -EINVAL;
	}

	return 0;
}

struct net_dhcpv4_context net_dhcpv4_context_data;

static struct dummy_api net_dhcpv4_if_api = {
	.iface_api.init = net_dhcpv4_iface_init,
	.send = tester_send,
};

NET_DEVICE_INIT(net_dhcpv4_test, "net_dhcpv4_test",
		net_dhcpv4_dev_init, NULL,
		&net_dhcpv4_context_data, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&net_dhcpv4_if_api, DUMMY_L2,
		NET_L2_GET_CTX_TYPE(DUMMY_L2), 127);

#ifdef CONFIG_NET_DHCPV4_OPTION_CALLBACKS
static struct net_dhcpv4_option_callback opt_domain_cb;
static struct net_dhcpv4_option_callback opt_pop3_cb;
static struct net_dhcpv4_option_callback opt_invalid_cb;
static uint8_t buffer[15];
#endif
#ifdef CONFIG_NET_DHCPV4_OPTION_CALLBACKS_VENDOR_SPECIFIC
static struct net_dhcpv4_option_callback opt_vs_string_cb;
static struct net_dhcpv4_option_callback opt_vs_byte_cb;
static struct net_dhcpv4_option_callback opt_vs_empty_cb;
static struct net_dhcpv4_option_callback opt_vs_invalid_cb;
#endif

static void receiver_cb(uint64_t nm_event, struct net_if *iface, void *info, size_t info_length,
			void *user_data)
{
	const struct net_in_addr ip_addr = { { { 10, 237, 72, 158 } } };
	const struct net_in_addr dns_addrs[3] = {
		{ { { 10, 248, 2, 1 } } },
		{ { { 163, 33, 253, 68 } } },
		{ { { 10, 184, 9, 1 } } },
	};

	ARG_UNUSED(iface);
	ARG_UNUSED(user_data);

	switch (nm_event) {
	case NET_EVENT_IPV4_ADDR_ADD:
		zassert_equal(info_length, sizeof(struct net_in_addr));
		zassert_mem_equal(info, &ip_addr, sizeof(struct net_in_addr));
		k_event_post(&events, EVT_ADDR_ADD);
		break;
	case NET_EVENT_IPV4_ADDR_DEL:
		k_event_post(&events, EVT_ADDR_DEL);
		break;
	case NET_EVENT_DNS_SERVER_ADD:
		zassert_equal(info_length, sizeof(struct net_sockaddr));
		if (net_sin(info)->sin_addr.s_addr == dns_addrs[0].s_addr) {
			k_event_post(&events, EVT_DNS_SERVER1_ADD);
		} else if (net_sin(info)->sin_addr.s_addr == dns_addrs[1].s_addr) {
			k_event_post(&events, EVT_DNS_SERVER2_ADD);
		} else if (net_sin(info)->sin_addr.s_addr == dns_addrs[2].s_addr) {
			k_event_post(&events, EVT_DNS_SERVER3_ADD);
		} else {
			zassert_unreachable("Unknown DNS server");
		}
		break;
	case NET_EVENT_DNS_SERVER_DEL:
		zassert_equal(info_length, sizeof(struct net_sockaddr));
		if (net_sin(info)->sin_addr.s_addr == dns_addrs[0].s_addr) {
			k_event_post(&events, EVT_DNS_SERVER1_DEL);
		} else if (net_sin(info)->sin_addr.s_addr == dns_addrs[1].s_addr) {
			k_event_post(&events, EVT_DNS_SERVER2_DEL);
		} else if (net_sin(info)->sin_addr.s_addr == dns_addrs[2].s_addr) {
			k_event_post(&events, EVT_DNS_SERVER3_DEL);
		} else {
			zassert_unreachable("Unknown DNS server");
		}
		break;
	case NET_EVENT_IPV4_DHCP_START:
		k_event_post(&events, EVT_DHCP_START);
		break;
	case NET_EVENT_IPV4_DHCP_BOUND:
		k_event_post(&events, EVT_DHCP_BOUND);
		break;
	case NET_EVENT_IPV4_DHCP_STOP:
		k_event_post(&events, EVT_DHCP_STOP);
		break;
	}
}

NET_MGMT_REGISTER_EVENT_HANDLER(rx_cb, NET_EVENT_IPV4_ADDR_ADD | NET_EVENT_IPV4_ADDR_DEL,
				receiver_cb, NULL);
NET_MGMT_REGISTER_EVENT_HANDLER(dns_cb, NET_EVENT_DNS_SERVER_ADD | NET_EVENT_DNS_SERVER_DEL,
				receiver_cb, NULL);
NET_MGMT_REGISTER_EVENT_HANDLER(dhcp_cb,
				NET_EVENT_IPV4_DHCP_START | NET_EVENT_IPV4_DHCP_BOUND |
					NET_EVENT_IPV4_DHCP_STOP,
				receiver_cb, NULL);

#ifdef CONFIG_NET_DHCPV4_OPTION_CALLBACKS

static void option_domain_cb(struct net_dhcpv4_option_callback *cb,
			     size_t length,
			     enum net_dhcpv4_msg_type msg_type,
			     struct net_if *iface)
{
	static const char expectation[] = "fi.intel.com";

	ARG_UNUSED(msg_type);
	ARG_UNUSED(iface);

	zassert_equal(cb->option, OPTION_DOMAIN, "Unexpected option value");
	zassert_equal(length, sizeof(expectation), "Incorrect data length");
	zassert_mem_equal(buffer, expectation, sizeof(expectation),
			  "Incorrect buffer contents");

	k_event_post(&events, EVT_OPTION_DOMAIN);
}

static void option_pop3_cb(struct net_dhcpv4_option_callback *cb,
			   size_t length,
			   enum net_dhcpv4_msg_type msg_type,
			   struct net_if *iface)
{
	static const uint8_t expectation[4] = { 198, 51, 100, 16 };

	ARG_UNUSED(msg_type);
	ARG_UNUSED(iface);

	zassert_equal(cb->option, OPTION_POP3, "Unexpected option value");
	zassert_equal(length, sizeof(expectation), "Incorrect data length");
	zassert_mem_equal(buffer, expectation, sizeof(expectation),
			  "Incorrect buffer contents");

	k_event_post(&events, EVT_OPTION_POP3);
}

static void option_invalid_cb(struct net_dhcpv4_option_callback *cb,
			      size_t length,
			      enum net_dhcpv4_msg_type msg_type,
			      struct net_if *iface)
{
	ARG_UNUSED(cb);
	ARG_UNUSED(length);
	ARG_UNUSED(msg_type);
	ARG_UNUSED(iface);

	/* This function should never be called. If it is, the parser took a wrong turn. */
	zassert_unreachable("Unexpected callback - incorrect parsing of vendor specific options");
}

#ifdef CONFIG_NET_DHCPV4_OPTION_CALLBACKS_VENDOR_SPECIFIC

static void vendor_specific_string_cb(struct net_dhcpv4_option_callback *cb,
				      size_t length,
				      enum net_dhcpv4_msg_type msg_type,
				      struct net_if *iface)
{
	static const char expectation[] = "string";

	ARG_UNUSED(msg_type);
	ARG_UNUSED(iface);

	zassert_equal(cb->option, OPTION_VENDOR_STRING,
		      "Unexpected vendor specific option value");
	zassert_equal(length, sizeof(expectation), "Incorrect data length");
	zassert_mem_equal(buffer, expectation, sizeof(expectation), "Incorrect buffer contents");

	k_event_post(&events, EVT_VENDOR_STRING);
}

static void vendor_specific_byte_cb(struct net_dhcpv4_option_callback *cb,
				    size_t length,
				    enum net_dhcpv4_msg_type msg_type,
				    struct net_if *iface)
{
	ARG_UNUSED(msg_type);
	ARG_UNUSED(iface);

	zassert_equal(cb->option, OPTION_VENDOR_BYTE,
		      "Unexpected vendor specific option value");
	zassert_equal(length, 1, "Incorrect data length");
	zassert_equal(buffer[0], 1, "Incorrect buffer contents");

	k_event_post(&events, EVT_VENDOR_BYTE);
}

static void vendor_specific_empty_cb(struct net_dhcpv4_option_callback *cb,
				     size_t length,
				     enum net_dhcpv4_msg_type msg_type,
				     struct net_if *iface)
{
	ARG_UNUSED(msg_type);
	ARG_UNUSED(iface);

	zassert_equal(cb->option, OPTION_VENDOR_EMPTY,
		      "Unexpected vendor specific option value");
	zassert_equal(length, 0, "Incorrect data length");

	k_event_post(&events, EVT_VENDOR_EMPTY);
}

#endif /* CONFIG_NET_DHCPV4_OPTION_CALLBACKS_VENDOR_SPECIFIC */

#endif /* CONFIG_NET_DHCPV4_OPTION_CALLBACKS */

ZTEST(dhcpv4_tests, test_dhcp)
{
	struct net_if *iface;
	uint32_t evt;

#ifdef CONFIG_NET_DHCPV4_OPTION_CALLBACKS
	net_dhcpv4_init_option_callback(&opt_domain_cb, option_domain_cb,
					OPTION_DOMAIN, buffer,
					sizeof(buffer));

	net_dhcpv4_add_option_callback(&opt_domain_cb);

	net_dhcpv4_init_option_callback(&opt_pop3_cb, option_pop3_cb,
					OPTION_POP3, buffer,
					sizeof(buffer));

	net_dhcpv4_add_option_callback(&opt_pop3_cb);

	net_dhcpv4_init_option_callback(&opt_invalid_cb, option_invalid_cb,
					OPTION_INVALID, buffer,
					sizeof(buffer));

	net_dhcpv4_add_option_callback(&opt_invalid_cb);
#endif /* CONFIG_NET_DHCPV4_OPTION_CALLBACKS */

#ifdef CONFIG_NET_DHCPV4_OPTION_CALLBACKS_VENDOR_SPECIFIC
	net_dhcpv4_init_option_vendor_callback(&opt_vs_string_cb, vendor_specific_string_cb,
					       OPTION_VENDOR_STRING, buffer,
					       sizeof(buffer));

	net_dhcpv4_add_option_vendor_callback(&opt_vs_string_cb);

	net_dhcpv4_init_option_vendor_callback(&opt_vs_byte_cb, vendor_specific_byte_cb,
					       OPTION_VENDOR_BYTE, buffer,
					       sizeof(buffer));

	net_dhcpv4_add_option_vendor_callback(&opt_vs_byte_cb);

	net_dhcpv4_init_option_vendor_callback(&opt_vs_empty_cb, vendor_specific_empty_cb,
					       OPTION_VENDOR_EMPTY, buffer,
					       sizeof(buffer));

	net_dhcpv4_add_option_vendor_callback(&opt_vs_empty_cb);

	net_dhcpv4_init_option_vendor_callback(&opt_vs_invalid_cb, option_invalid_cb,
					       OPTION_INVALID, buffer,
					       sizeof(buffer));

	net_dhcpv4_add_option_vendor_callback(&opt_vs_invalid_cb);


#endif /* CONFIG_NET_DHCPV4_OPTION_CALLBACKS_VENDOR_SPECIFIC */

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));
	if (!iface) {
		zassert_true(false, "Interface not available");
	}

	for (int loop = 0; loop < 2; ++loop) {
		LOG_DBG("Running DHCPv4 loop %d", loop);
		net_dhcpv4_start(iface);

		evt = k_event_wait(&events, EVT_DHCP_START, false, WAIT_TIME);
		zassert_equal(evt, EVT_DHCP_START, "Missing DHCP start");

#ifdef CONFIG_NET_DHCPV4_OPTION_CALLBACKS
		evt = k_event_wait_all(&events, EVT_OPTION_DOMAIN | EVT_OPTION_POP3, false,
				       WAIT_TIME);
		zassert_equal(evt, EVT_OPTION_DOMAIN | EVT_OPTION_POP3,
			      "Missing DHCP option(s) %08x", evt);
#endif

#ifdef CONFIG_NET_DHCPV4_OPTION_CALLBACKS_VENDOR_SPECIFIC
		evt = k_event_wait_all(&events,
				       EVT_VENDOR_STRING | EVT_VENDOR_BYTE | EVT_VENDOR_EMPTY,
				       false, WAIT_TIME);
		zassert_equal(evt, EVT_VENDOR_STRING | EVT_VENDOR_BYTE | EVT_VENDOR_EMPTY,
			      "Missing DHCP vendor option(s) %08x", evt);
#endif

		evt = k_event_wait_all(&events,
				       EVT_DNS_SERVER1_ADD | EVT_DNS_SERVER2_ADD |
				       EVT_DNS_SERVER3_ADD,
				       false, WAIT_TIME);
		zassert_equal(evt,
			      EVT_DNS_SERVER1_ADD | EVT_DNS_SERVER2_ADD |
			      EVT_DNS_SERVER3_ADD,
			      "Missing DNS server(s) %08x", evt);

		evt = k_event_wait(&events, EVT_DHCP_BOUND, false, WAIT_TIME);
		zassert_equal(evt, EVT_DHCP_BOUND, "Missing DHCP bound");

		if (loop == 0 || !IS_ENABLED(CONFIG_NET_DHCPV4_INIT_REBOOT)) {
			evt = k_event_wait_all(&events, EVT_DHCP_OFFER | EVT_DHCP_ACK, false,
					       WAIT_TIME);
			zassert_equal(evt, EVT_DHCP_OFFER | EVT_DHCP_ACK,
				      "Missing offer or ack %08x", evt);

			/* Verify that Request xid matched Offer xid. */
			zassert_equal(offer_xid, request_xid,
				      "Offer/Request xid mismatch, "
				      "Offer 0x%08x, Request 0x%08x",
				      offer_xid, request_xid);
		} else {
			/* An init-reboot was done */
			evt = k_event_wait(&events, EVT_DHCP_OFFER | EVT_DHCP_ACK, false,
					   WAIT_TIME);
			zassert_equal(evt, EVT_DHCP_ACK, "Ack only expected %08x", evt);
		}

		/* Clear all events */
		k_event_set(&events, 0U);

		net_dhcpv4_stop(iface);

		evt = k_event_wait_all(&events,
				       EVT_DHCP_STOP | EVT_ADDR_DEL |
				       EVT_DNS_SERVER1_DEL | EVT_DNS_SERVER2_DEL |
				       EVT_DNS_SERVER3_DEL,
				       false, WAIT_TIME);
		zassert_equal(evt,
			      EVT_DHCP_STOP | EVT_ADDR_DEL |
			      EVT_DNS_SERVER1_DEL | EVT_DNS_SERVER2_DEL |
			      EVT_DNS_SERVER3_DEL,
			      "Missing DHCP stop or deleted address");
	}
}

ZTEST(dhcpv4_tests, test_init_reboot_nak_restarts_discovery)
{
	struct net_if *iface;
	uint32_t evt;

	Z_TEST_SKIP_IFNDEF(CONFIG_NET_DHCPV4_INIT_REBOOT);

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));
	zassert_not_null(iface, "Interface not available");

	iface->config.dhcpv4.requested_ip = (struct net_in_addr){{{10, 237, 72, 158}}};
	iface->config.dhcpv4.request_server_addr.s_addr = NET_INADDR_ANY;
	reject_init_reboot = true;

	net_dhcpv4_start(iface);

	evt = k_event_wait(&events, EVT_DHCP_NAK, false, WAIT_TIME);
	zassert_equal(evt, EVT_DHCP_NAK, "Missing INIT-REBOOT NAK %08x", evt);

	evt = k_event_wait_all(&events, EVT_DHCP_OFFER | EVT_DHCP_ACK, false, WAIT_TIME);
	zassert_equal(evt, EVT_DHCP_OFFER | EVT_DHCP_ACK,
		      "INIT-REBOOT NAK did not restart discovery %08x", evt);

	net_dhcpv4_stop(iface);
}

#if IS_ENABLED(CONFIG_NET_DHCPV4_INIT_REBOOT) && \
	IS_ENABLED(CONFIG_NET_DHCPV4_RESTART_ON_IF_UP) && \
	IS_ENABLED(CONFIG_NET_DHCPV4_DNS_SERVER_VIA_INTERFACE)

ZTEST(dhcpv4_tests, test_init_reboot_dns_after_iface_down)
{
	struct net_if *iface;
	uint32_t evt;

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));
	zassert_not_null(iface, "Interface not available");

	strict_dhcp_server = true;

	net_dhcpv4_start(iface);

	evt = k_event_wait(&events, EVT_DHCP_START, false, WAIT_TIME);
	zassert_equal(evt, EVT_DHCP_START, "Missing DHCP start");

	evt = k_event_wait_all(&events,
			       EVT_DNS_SERVER1_ADD | EVT_DNS_SERVER2_ADD |
			       EVT_DNS_SERVER3_ADD,
			       false, WAIT_TIME);
	zassert_equal(evt,
		      EVT_DNS_SERVER1_ADD | EVT_DNS_SERVER2_ADD |
		      EVT_DNS_SERVER3_ADD,
		      "Missing DNS server(s) on bind %08x", evt);

	evt = k_event_wait(&events, EVT_DHCP_BOUND, false, WAIT_TIME);
	zassert_equal(evt, EVT_DHCP_BOUND, "Missing DHCP bound");

	evt = k_event_wait_all(&events, EVT_DHCP_OFFER | EVT_DHCP_ACK, false,
			       WAIT_TIME);
	zassert_equal(evt, EVT_DHCP_OFFER | EVT_DHCP_ACK,
		      "Missing offer or ack on bind %08x", evt);

	zassert_true(discover_req_included_dns,
		     "DISCOVER did not ask for DNS");

	k_event_set(&events, 0U);

	zassert_ok(net_if_down(iface), "Failed to bring interface down");

	evt = k_event_wait_all(&events,
			       EVT_ADDR_DEL | EVT_DNS_SERVER1_DEL |
			       EVT_DNS_SERVER2_DEL | EVT_DNS_SERVER3_DEL,
			       false, WAIT_TIME);
	zassert_equal(evt,
		      EVT_ADDR_DEL | EVT_DNS_SERVER1_DEL |
		      EVT_DNS_SERVER2_DEL | EVT_DNS_SERVER3_DEL,
		      "Missing events on interface down %08x", evt);

	k_event_set(&events, 0U);
	init_reboot_req_included_dns = false;

	zassert_ok(net_if_up(iface), "Failed to bring interface up");

	evt = k_event_wait(&events, EVT_DHCP_BOUND, false, WAIT_TIME);
	zassert_equal(evt, EVT_DHCP_BOUND, "Missing DHCP bound after if up");

	evt = k_event_wait_all(&events,
			       EVT_DNS_SERVER1_ADD | EVT_DNS_SERVER2_ADD |
			       EVT_DNS_SERVER3_ADD,
			       false, WAIT_TIME);
	zassert_equal(evt,
		      EVT_DNS_SERVER1_ADD | EVT_DNS_SERVER2_ADD |
		      EVT_DNS_SERVER3_ADD,
		      "DNS not restored after interface up %08x", evt);

	evt = k_event_wait(&events, EVT_DHCP_ACK, false, WAIT_TIME);
	zassert_equal(evt, EVT_DHCP_ACK, "Missing ACK after interface up %08x", evt);

	zassert_true(init_reboot_req_included_dns,
		     "INIT-REBOOT REQUEST did not ask for DNS");

	k_event_set(&events, 0U);

	net_dhcpv4_stop(iface);

	evt = k_event_wait_all(&events,
			       EVT_DHCP_STOP | EVT_ADDR_DEL |
			       EVT_DNS_SERVER1_DEL | EVT_DNS_SERVER2_DEL |
			       EVT_DNS_SERVER3_DEL,
			       false, WAIT_TIME);
	zassert_equal(evt,
		      EVT_DHCP_STOP | EVT_ADDR_DEL |
		      EVT_DNS_SERVER1_DEL | EVT_DNS_SERVER2_DEL |
		      EVT_DNS_SERVER3_DEL,
		      "Missing DHCP stop cleanup %08x", evt);
}

#endif

/**test case main entry */
ZTEST_SUITE(dhcpv4_tests, NULL, NULL, dhcpv4_tests_before, NULL, NULL);
