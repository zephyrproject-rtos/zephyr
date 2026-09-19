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
/* The address the replies carry as the server identifier, which is the one
 * a client in REQUESTING expects a NAK to come from.
 */
static const struct net_in_addr server_id_addr = { { { 10, 184, 9, 1 } } };
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
/* An option code the client has no handler for, so it skips it. */
#define TEST_OPTION_UNKNOWN 250

struct dhcp_client_msg {
	uint32_t xid;
	uint8_t type;
	bool has_ciaddr;
	bool to_broadcast;
	bool has_requested_ip;
	bool has_server_id;
	uint8_t req_options[MAX_REQ_OPTIONS];
	uint8_t req_options_cnt;
};

static uint32_t offer_xid;
static uint32_t request_xid;
/* How many discovers to swallow before answering, so that the client is made
 * to retransmit, and what identifiers those retransmissions carried.
 */
static int discovers_to_drop;
static int discovers_seen;
static uint32_t discover_xids[4];
static bool strict_dhcp_server;
static bool discover_req_included_dns;
static bool init_reboot_req_included_dns;
static bool init_reboot_request_seen;
static bool reject_init_reboot;
static bool drop_init_reboot;
static uint8_t init_reboot_request_count;
/* Whether to hand out a lease short enough for the client to reach
 * RENEWING and REBINDING within the test, and how the server then answers
 * the REQUESTs sent from those states.
 */
static bool short_lease;
static bool drop_requests;
static bool nak_requests;
static bool no_router_option;
static bool zero_router_option;
/* Whether the handler for the leased address going away starts or stops the
 * client, the way an application watching for it might.
 */
static bool restart_on_addr_del;
static bool stop_on_addr_del;
/* Whether the handler for the leased address arriving stops the client. */
static bool stop_on_addr_add;
static enum { RENEWAL_ACK, RENEWAL_DROP, RENEWAL_NAK } renewal_reply;

#define SHORT_LEASE_T1   10
#define SHORT_LEASE_T2   20
#define SHORT_LEASE_TIME 300
/* Long enough to see T2 pass. native_sim fast-forwards idle time. */
#define SHORT_LEASE_WAIT K_SECONDS(SHORT_LEASE_T2 + 5)
#define LEASE_EXPIRY_WAIT K_SECONDS(SHORT_LEASE_TIME + 5)
/* Long enough for the client to retransmit a discover the test swallowed. */
#define DISCOVER_RETRY_WAIT \
	K_SECONDS(DHCPV4_INITIAL_RETRY_TIMEOUT + CONFIG_NET_DHCPV4_INITIAL_DELAY_MAX + 2)

static const struct net_in_addr leased_addr = { { { 10, 237, 72, 158 } } };
/* A gateway the tests install by hand, which no reply ever names. */
static const struct net_in_addr static_gw = { { { 192, 0, 2, 7 } } };

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
#define EVT_DHCP_RENEW_REQ  BIT(19)
#define EVT_DHCP_REBIND_REQ BIT(20)

#define EVT_DNS_ALL_DEL (EVT_DNS_SERVER1_DEL | EVT_DNS_SERVER2_DEL | EVT_DNS_SERVER3_DEL)

static K_EVENT_DEFINE(events);

/* The order the removals were announced in. The address is meant to be the
 * last thing an application hears about.
 */
static uint32_t event_seq;
static uint32_t addr_del_seq;
static uint32_t dns_del_seq;
static uint32_t start_seq;
static uint32_t stop_seq;

static void dhcp_test_reset_iface(struct net_if *iface)
{
	/* Before anything that removes an address: a test that failed part
	 * way through may have left one of these set, and the removals below
	 * would then run it against the client this hook is resetting.
	 */
	restart_on_addr_del = false;
	stop_on_addr_del = false;
	stop_on_addr_add = false;

	net_dhcpv4_stop(iface);

	/* Undo what a failed test may have left behind. */
	(void)net_if_ipv4_addr_rm(iface, &leased_addr);
	net_if_ipv4_set_gw(iface, net_ipv4_unspecified_address());
	if (!net_if_is_up(iface)) {
		(void)net_if_up(iface);
	}

	iface->config.dhcpv4.requested_ip.s_addr = 0;
	iface->config.dhcpv4.gw.s_addr = 0;
	iface->config.dhcpv4.gw_before.s_addr = 0;

	/* Let the net_mgmt thread deliver the events raised above. */
	k_sleep(K_MSEC(10));
	k_event_set(&events, 0U);
	event_seq = 0;
	addr_del_seq = 0;
	dns_del_seq = 0;
	start_seq = 0;
	stop_seq = 0;
	offer_xid = 0U;
	request_xid = 0U;
	discovers_to_drop = 0;
	discovers_seen = 0;
	memset(discover_xids, 0, sizeof(discover_xids));
	strict_dhcp_server = false;
	discover_req_included_dns = false;
	init_reboot_req_included_dns = false;
	init_reboot_request_seen = false;
	reject_init_reboot = false;
	drop_init_reboot = false;
	init_reboot_request_count = 0U;
	short_lease = false;
	drop_requests = false;
	nak_requests = false;
	no_router_option = false;
	zero_router_option = false;
	renewal_reply = RENEWAL_ACK;
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

/* Offset of the value of option @p code in the DHCP message @p msg, or -1. */
static int dhcp_option_value_offset(const unsigned char *msg, size_t len, uint8_t code)
{
	/* Options follow the fixed header and the magic cookie. */
	size_t pos = sizeof(struct dhcp_msg) + SIZE_OF_SNAME + SIZE_OF_FILE +
		     SIZE_OF_MAGIC_COOKIE;

	while (pos + 1 < len) {
		uint8_t opt = msg[pos];

		if (opt == DHCPV4_OPTIONS_PAD) {
			pos++;
			continue;
		}

		if (opt == DHCPV4_OPTIONS_END) {
			break;
		}

		if (opt == code) {
			return pos + 2;
		}

		pos += 2 + msg[pos + 1];
	}

	return -1;
}

/* Rename an option in a reply already written to @p pkt, so that the client
 * skips it as one it does not know.
 */
static int dhcp_hide_option(struct net_pkt *pkt, const unsigned char *msg,
			    size_t len, uint8_t code)
{
	int offset = dhcp_option_value_offset(msg, len, code);
	int ret = 0;

	if (offset < 0) {
		return -ENOENT;
	}

	net_pkt_cursor_init(pkt);
	net_pkt_set_overwrite(pkt, true);

	if (net_pkt_skip(pkt, NET_IPV4UDPH_LEN + offset - 2) ||
	    net_pkt_write_u8(pkt, TEST_OPTION_UNKNOWN)) {
		ret = -EINVAL;
	}

	net_pkt_set_overwrite(pkt, false);

	return ret;
}

/* Overwrite a four byte option value in a reply already written to @p pkt. */
static int dhcp_patch_option_be32(struct net_pkt *pkt, const unsigned char *msg,
				  size_t len, uint8_t code, uint32_t value)
{
	int offset = dhcp_option_value_offset(msg, len, code);
	int ret = 0;

	if (offset < 0) {
		return -ENOENT;
	}

	net_pkt_cursor_init(pkt);
	net_pkt_set_overwrite(pkt, true);

	if (net_pkt_skip(pkt, NET_IPV4UDPH_LEN + offset) ||
	    net_pkt_write_be32(pkt, value)) {
		ret = -EINVAL;
	}

	net_pkt_set_overwrite(pkt, false);

	return ret;
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

	if (no_router_option &&
	    dhcp_hide_option(pkt, offer, sizeof(offer), DHCPV4_OPTIONS_ROUTER) != 0) {
		goto fail;
	}

	if (zero_router_option &&
	    dhcp_patch_option_be32(pkt, offer, sizeof(offer), DHCPV4_OPTIONS_ROUTER,
				   0U) != 0) {
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

	if (no_router_option &&
	    dhcp_hide_option(pkt, reply, reply_len, DHCPV4_OPTIONS_ROUTER) != 0) {
		goto fail;
	}

	if (zero_router_option &&
	    dhcp_patch_option_be32(pkt, reply, reply_len, DHCPV4_OPTIONS_ROUTER,
				   0U) != 0) {
		goto fail;
	}

	if (short_lease &&
	    (dhcp_patch_option_be32(pkt, reply, reply_len, DHCPV4_OPTIONS_RENEWAL,
				    SHORT_LEASE_T1) != 0 ||
	     dhcp_patch_option_be32(pkt, reply, reply_len, DHCPV4_OPTIONS_REBINDING,
				    SHORT_LEASE_T2) != 0 ||
	     dhcp_patch_option_be32(pkt, reply, reply_len, DHCPV4_OPTIONS_LEASE_TIME,
				    SHORT_LEASE_TIME) != 0)) {
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

	if (net_ipv4_create(pkt, &server_id_addr, &client_addr) ||
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
	struct net_in_addr ciaddr;

	memset(msg, 0, sizeof(*msg));

	msg->to_broadcast = net_ipv4_addr_cmp_raw(NET_IPV4_HDR(pkt)->dst,
						  net_ipv4_broadcast_address()->s4_addr);

	if (net_pkt_skip(pkt, NET_IPV4UDPH_LEN + 4)) {
		return -EINVAL;
	}

	if (net_pkt_read_be32(pkt, &msg->xid)) {
		return -EINVAL;
	}

	/* secs and flags */
	if (net_pkt_skip(pkt, 4)) {
		return -EINVAL;
	}

	if (net_pkt_read(pkt, &ciaddr, sizeof(ciaddr))) {
		return -EINVAL;
	}

	msg->has_ciaddr = ciaddr.s_addr != NET_INADDR_ANY;

	/* yiaddr, siaddr, giaddr, chaddr, sname, file, magic cookie */
	if (net_pkt_skip(pkt, 28 + 64 + 128 + 4)) {
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
	bool is_renewal;
	bool is_requesting;

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

		if (discovers_seen < ARRAY_SIZE(discover_xids)) {
			discover_xids[discovers_seen] = msg.xid;
		}
		discovers_seen++;

		if (discovers_to_drop > 0) {
			/* Say nothing, so that the client retransmits. */
			discovers_to_drop--;
			return 0;
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
		init_reboot_request_seen |= is_init_reboot;
		if (is_init_reboot) {
			init_reboot_request_count++;
		}

		/* RFC 2131 4.3.2: a client in RENEWING or REBINDING fills in
		 * ciaddr and leaves out both the requested address and the
		 * server identifier. Only the former unicasts to its server.
		 */
		is_renewal = msg.has_ciaddr && !msg.has_requested_ip && !msg.has_server_id;
		if (is_renewal) {
			k_event_post(&events, msg.to_broadcast ? EVT_DHCP_REBIND_REQ :
								 EVT_DHCP_RENEW_REQ);

			if (renewal_reply == RENEWAL_DROP) {
				/* Say nothing, so that the client walks from T1
				 * through T2 to the end of the lease on its own.
				 */
				return 0;
			}
		}

		/* A request that names both the address and the server is the
		 * one that follows an offer.
		 */
		is_requesting = msg.has_requested_ip && msg.has_server_id;

		if (drop_requests && is_requesting) {
			/* Say nothing, so that the client never binds. */
			return 0;
		}

		if (drop_init_reboot && is_init_reboot) {
			/* Emulate a server (e.g. on a different network) that
			 * silently ignores the foreign-subnet REQUEST instead of
			 * NAKing it, forcing the client to time out and fall back
			 * to DISCOVER.
			 */
			return 0;
		}

		if (strict_dhcp_server && is_init_reboot) {
			init_reboot_req_included_dns = dns_requested;
			include_dns = dns_requested;
		} else {
			include_dns = true;
		}

		nak_reply = (reject_init_reboot && is_init_reboot) ||
			    (nak_requests && is_requesting) ||
			    (renewal_reply == RENEWAL_NAK && is_renewal);

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
	const struct net_in_addr dns_addrs[3] = {
		{ { { 10, 248, 2, 1 } } },
		{ { { 163, 33, 253, 68 } } },
		{ { { 10, 184, 9, 1 } } },
	};

	ARG_UNUSED(user_data);

	switch (nm_event) {
	case NET_EVENT_IPV4_ADDR_ADD:
		zassert_equal(info_length, sizeof(struct net_in_addr));
		zassert_mem_equal(info, &leased_addr, sizeof(struct net_in_addr));

		if (stop_on_addr_add) {
			stop_on_addr_add = false;
			net_dhcpv4_stop(iface);
		}

		k_event_post(&events, EVT_ADDR_ADD);
		break;
	case NET_EVENT_IPV4_ADDR_DEL:
		/* Only the leased address counts; anything else is not what
		 * these tests are watching for.
		 */
		if (info_length == sizeof(struct net_in_addr) &&
		    net_ipv4_addr_cmp(info, &leased_addr)) {
			addr_del_seq = ++event_seq;

			if (restart_on_addr_del) {
				restart_on_addr_del = false;
				net_dhcpv4_start(iface);
			}

			if (stop_on_addr_del) {
				stop_on_addr_del = false;
				net_dhcpv4_stop(iface);
			}

			k_event_post(&events, EVT_ADDR_DEL);
		}
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
		dns_del_seq = ++event_seq;
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
		start_seq = ++event_seq;
		k_event_post(&events, EVT_DHCP_START);
		break;
	case NET_EVENT_IPV4_DHCP_BOUND:
		k_event_post(&events, EVT_DHCP_BOUND);
		break;
	case NET_EVENT_IPV4_DHCP_STOP:
		stop_seq = ++event_seq;
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

ZTEST(dhcpv4_tests, test_init_reboot_hint)
{
	struct net_if *iface;
	uint32_t evt;

	Z_TEST_SKIP_IFNDEF(CONFIG_NET_DHCPV4_INIT_REBOOT);

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));
	zassert_not_null(iface, "Interface not available");

	zassert_ok(net_dhcpv4_set_reboot_hint(iface, &leased_addr));

	net_dhcpv4_start(iface);

	evt = k_event_wait(&events, EVT_DHCP_OFFER | EVT_DHCP_ACK, false, WAIT_TIME);
	zassert_equal(evt, EVT_DHCP_ACK, "Expected INIT-REBOOT ACK only %08x", evt);
	zassert_true(init_reboot_request_seen, "INIT-REBOOT REQUEST was not sent");

	net_dhcpv4_stop(iface);
}

ZTEST(dhcpv4_tests, test_init_reboot_nak_restarts_discovery)
{
	struct net_if *iface;
	uint32_t evt;

	Z_TEST_SKIP_IFNDEF(CONFIG_NET_DHCPV4_INIT_REBOOT);

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));
	zassert_not_null(iface, "Interface not available");

	iface->config.dhcpv4.requested_ip = leased_addr;
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

ZTEST(dhcpv4_tests, test_init_reboot_unanswered_falls_back_to_discover)
{
	/* Worst-case INIT-REBOOT backoff is 4 + 8 + ... = 4 * (2^N - 1) seconds
	 * for N attempts, plus per-attempt randomisation and the follow-up
	 * DISCOVER round. native_sim fast-forwards idle time, so this large
	 * timeout costs no wall-clock.
	 */
	const k_timeout_t fallback_wait = K_SECONDS(
		4 * (BIT(DHCPV4_INIT_REBOOT_MAX_ATTEMPTS) - 1) +
		DHCPV4_INIT_REBOOT_MAX_ATTEMPTS + 5);
	struct net_if *iface;
	uint32_t evt;

	Z_TEST_SKIP_IFNDEF(CONFIG_NET_DHCPV4_INIT_REBOOT);

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));
	zassert_not_null(iface, "Interface not available");

	/* Emulate a network change: a stale lease drives INIT-REBOOT, but the
	 * new network's server silently drops the foreign-subnet REQUEST.
	 */
	zassert_ok(net_dhcpv4_set_reboot_hint(iface, &leased_addr));
	drop_init_reboot = true;

	net_dhcpv4_start(iface);

	/* An OFFER is only produced in response to a DISCOVER, so its arrival
	 * proves the client abandoned INIT-REBOOT and restarted configuration
	 * rather than retransmitting the REQUEST for the full attempt budget.
	 */
	evt = k_event_wait_all(&events, EVT_DHCP_OFFER | EVT_DHCP_ACK, false,
			       fallback_wait);
	zassert_equal(evt, EVT_DHCP_OFFER | EVT_DHCP_ACK,
		      "Unanswered INIT-REBOOT did not fall back to DISCOVER %08x", evt);
	zassert_true(init_reboot_request_seen, "INIT-REBOOT REQUEST was not sent");
	zassert_equal(init_reboot_request_count, DHCPV4_INIT_REBOOT_MAX_ATTEMPTS,
		      "Expected %d INIT-REBOOT attempt(s) before fallback, got %d",
		      DHCPV4_INIT_REBOOT_MAX_ATTEMPTS, init_reboot_request_count);

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
			       EVT_ADDR_DEL | EVT_DNS_ALL_DEL,
			       false, WAIT_TIME);
	zassert_equal(evt,
		      EVT_ADDR_DEL | EVT_DNS_ALL_DEL,
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
/* RFC 2131 4.1: one transaction identifier for every message of a
 * transaction. A retransmitted discover is the same transaction, so a server
 * that has already seen the first one has to be able to recognise the second
 * as a repeat rather than as another client asking.
 *
 * Every discover is dropped, so the exchange never completes and this leaves
 * neither a lease nor a set of DNS servers behind for whatever runs next.
 */
ZTEST(dhcpv4_tests, test_discover_retransmission_keeps_xid)
{
	/* Long enough for two retransmissions: the backoff is four seconds,
	 * then eight, plus a second of randomisation each. native_sim
	 * fast-forwards idle time, so this costs no wall clock.
	 */
	const k_timeout_t settle = K_SECONDS(4 + 8 + 4);
	struct net_if *iface;

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));
	zassert_not_null(iface, "Interface not available");

	discovers_to_drop = ARRAY_SIZE(discover_xids);

	net_dhcpv4_start(iface);
	k_sleep(settle);
	net_dhcpv4_stop(iface);

	zassert_true(discovers_seen >= 3,
		     "Expected the client to retransmit, saw %d discover(s)",
		     discovers_seen);

	zassert_not_equal(discover_xids[0], 0U, "Transaction identifier is zero");

	for (int i = 1; i < MIN(discovers_seen, ARRAY_SIZE(discover_xids)); i++) {
		zassert_equal(discover_xids[i], discover_xids[0],
			      "Discover %d carries xid 0x%08x, the first carried "
			      "0x%08x; a retransmission is the same transaction",
			      i, discover_xids[i], discover_xids[0]);
	}
}

/* Bind with a lease short enough to reach RENEWING and REBINDING within
 * the test.
 */
static void bind_with_short_lease(struct net_if *iface)
{
	uint32_t evt;

	short_lease = true;

	net_dhcpv4_start(iface);

	evt = k_event_wait_all(&events, EVT_DHCP_BOUND | EVT_ADDR_ADD, false, WAIT_TIME);
	zassert_equal(evt, EVT_DHCP_BOUND | EVT_ADDR_ADD, "Missing DHCP bound %08x", evt);
	zassert_not_null(net_if_ipv4_addr_lookup_by_iface(iface, &leased_addr),
			 "Leased address not on the interface");
	zassert_not_equal(net_if_ipv4_get_gw(iface).s_addr, NET_INADDR_ANY,
			  "Gateway not set by the lease");
}

/* Bind with a short lease, then leave the renewal REQUESTs unanswered until
 * the client has moved on to @p state.
 */
static void bind_and_reach(struct net_if *iface, enum net_dhcpv4_state state)
{
	uint32_t evt;

	renewal_reply = RENEWAL_DROP;
	bind_with_short_lease(iface);

	switch (state) {
	case NET_DHCPV4_RENEWING:
		evt = k_event_wait(&events, EVT_DHCP_RENEW_REQ, false, SHORT_LEASE_WAIT);
		zassert_equal(evt, EVT_DHCP_RENEW_REQ, "Client did not renew at T1");
		break;
	case NET_DHCPV4_REBINDING:
		evt = k_event_wait(&events, EVT_DHCP_REBIND_REQ, false, SHORT_LEASE_WAIT);
		zassert_equal(evt, EVT_DHCP_REBIND_REQ, "Client did not rebind at T2");
		break;
	default:
		zassert_unreachable("Unsupported state %s", net_dhcpv4_state_name(state));
	}

	zassert_equal(iface->config.dhcpv4.state, state, "Client in state %s, expected %s",
		      net_dhcpv4_state_name(iface->config.dhcpv4.state),
		      net_dhcpv4_state_name(state));
	zassert_not_null(net_if_ipv4_addr_lookup_by_iface(iface, &leased_addr),
			 "Leased address lost before the lease was given up");
}

/* The address is the last thing an application hears about, whichever path
 * gave the lease up.
 */
static void check_teardown_order(void)
{
	zassert_not_equal(dns_del_seq, 0, "DNS servers not announced gone");
	zassert_true(dns_del_seq < addr_del_seq,
		     "The address was announced gone before the DNS servers");
}

/* The client binds again as soon as the lease is gone, possibly before
 * this thread runs, so the gap cannot be observed directly. The interface
 * raises the second add event only for an address that had really gone.
 */
static void check_bound_again(struct net_if *iface, const char *what, k_timeout_t timeout)
{
	uint32_t evt;

	evt = k_event_wait_all(&events,
			       EVT_DHCP_OFFER | EVT_DHCP_ACK | EVT_DHCP_BOUND | EVT_ADDR_ADD,
			       false, timeout);
	zassert_equal(evt, EVT_DHCP_OFFER | EVT_DHCP_ACK | EVT_DHCP_BOUND | EVT_ADDR_ADD,
		      "Client did not bind again after %s %08x", what, evt);
	zassert_not_null(net_if_ipv4_addr_lookup_by_iface(iface, &leased_addr),
			 "New lease not on the interface");
	zassert_not_equal(net_if_ipv4_get_gw(iface).s_addr, NET_INADDR_ANY,
			  "Gateway not set by the new lease");
}

/* Restarting from REBINDING takes the leased address off the interface, so
 * the fresh start can bind again, here on a network that refuses the
 * remembered address, the way a move between subnets does.
 */
ZTEST(dhcpv4_tests, test_restart_in_rebinding_drops_lease)
{
	struct net_if *iface;
	uint32_t evt;

	Z_TEST_SKIP_IFNDEF(CONFIG_NET_DHCPV4_INIT_REBOOT);

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));
	zassert_not_null(iface, "Interface not available");

	bind_and_reach(iface, NET_DHCPV4_REBINDING);

	k_event_set(&events, 0U);
	reject_init_reboot = true;

	/* Swallow the first discover, so that the client cannot bind again
	 * before the interface has been looked at.
	 */
	discovers_to_drop = 1;

	net_dhcpv4_restart(iface);

	evt = k_event_wait_all(&events,
			       EVT_DHCP_STOP | EVT_ADDR_DEL | EVT_DNS_ALL_DEL | EVT_DHCP_NAK,
			       false, WAIT_TIME);
	zassert_equal(evt, EVT_DHCP_STOP | EVT_ADDR_DEL | EVT_DNS_ALL_DEL | EVT_DHCP_NAK,
		      "Restart in REBINDING left the lease on the interface %08x", evt);
	zassert_equal(net_if_ipv4_get_gw(iface).s_addr, NET_INADDR_ANY,
		      "Restart in REBINDING left the gateway");
	check_teardown_order();

	check_bound_again(iface, "restart", DISCOVER_RETRY_WAIT);

	net_dhcpv4_stop(iface);
}

/* The interface going down ends the lease from any bound state, not only
 * from BOUND: the address comes off, and the client is left ready to probe
 * for it again once the link is back.
 */
static void check_if_down_drops_lease(enum net_dhcpv4_state state)
{
	struct net_if *iface;
	uint32_t evt;

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));
	zassert_not_null(iface, "Interface not available");

	bind_and_reach(iface, state);

	k_event_set(&events, 0U);

	zassert_ok(net_if_down(iface), "Failed to bring interface down");

	evt = k_event_wait_all(&events,
			       EVT_ADDR_DEL | EVT_DNS_ALL_DEL,
			       false, WAIT_TIME);
	zassert_equal(evt,
		      EVT_ADDR_DEL | EVT_DNS_ALL_DEL,
		      "Interface down in %s left the lease on the interface",
		      net_dhcpv4_state_name(state));
	zassert_is_null(net_if_ipv4_addr_lookup_by_iface(iface, &leased_addr),
			"Leased address still on the interface after interface down");
	zassert_equal(net_if_ipv4_get_gw(iface).s_addr, NET_INADDR_ANY,
		      "Gateway left set after interface down");

	check_teardown_order();
	zassert_equal(iface->config.dhcpv4.state, NET_DHCPV4_INIT_REBOOT,
		      "Client left in state %s after interface down",
		      net_dhcpv4_state_name(iface->config.dhcpv4.state));

	k_event_set(&events, 0U);

	zassert_ok(net_if_up(iface), "Failed to bring interface up");

	evt = k_event_wait_all(&events, EVT_DHCP_BOUND | EVT_ADDR_ADD, false,
			       WAIT_TIME);
	zassert_equal(evt, EVT_DHCP_BOUND | EVT_ADDR_ADD,
		      "Client did not bind again after interface up %08x", evt);

	net_dhcpv4_stop(iface);
}

ZTEST(dhcpv4_tests, test_if_down_in_renewing_drops_lease)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_NET_DHCPV4_INIT_REBOOT);
	Z_TEST_SKIP_IFNDEF(CONFIG_NET_DHCPV4_RESTART_ON_IF_UP);

	check_if_down_drops_lease(NET_DHCPV4_RENEWING);
}

ZTEST(dhcpv4_tests, test_if_down_in_rebinding_drops_lease)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_NET_DHCPV4_INIT_REBOOT);
	Z_TEST_SKIP_IFNDEF(CONFIG_NET_DHCPV4_RESTART_ON_IF_UP);

	check_if_down_drops_lease(NET_DHCPV4_REBINDING);
}

/* A NAK to the renewal ends the lease: the address comes off the interface
 * before the client goes looking for a new one.
 */
ZTEST(dhcpv4_tests, test_nak_in_renewing_drops_lease)
{
	struct net_if *iface;
	uint32_t evt;

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));
	zassert_not_null(iface, "Interface not available");

	renewal_reply = RENEWAL_NAK;
	bind_with_short_lease(iface);

	k_event_set(&events, 0U);

	discovers_to_drop = 1;

	evt = k_event_wait_all(&events,
			       EVT_DHCP_RENEW_REQ | EVT_DHCP_NAK | EVT_ADDR_DEL |
			       EVT_DNS_ALL_DEL,
			       false, SHORT_LEASE_WAIT);
	zassert_equal(evt,
		      EVT_DHCP_RENEW_REQ | EVT_DHCP_NAK | EVT_ADDR_DEL |
		      EVT_DNS_ALL_DEL,
		      "NAK in RENEWING left the lease on the interface %08x", evt);
	zassert_equal(net_if_ipv4_get_gw(iface).s_addr, NET_INADDR_ANY,
		      "NAK in RENEWING left the gateway");
	check_teardown_order();

	check_bound_again(iface, "NAK", DISCOVER_RETRY_WAIT);

	net_dhcpv4_stop(iface);
}

/* The lease running out in REBINDING ends it the same way. */
ZTEST(dhcpv4_tests, test_lease_expiry_drops_lease)
{
	struct net_if *iface;
	uint32_t evt;

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));
	zassert_not_null(iface, "Interface not available");

	bind_and_reach(iface, NET_DHCPV4_REBINDING);

	k_event_set(&events, 0U);

	discovers_to_drop = 1;

	evt = k_event_wait_all(&events,
			       EVT_ADDR_DEL | EVT_DNS_ALL_DEL,
			       false, LEASE_EXPIRY_WAIT);
	zassert_equal(evt,
		      EVT_ADDR_DEL | EVT_DNS_ALL_DEL,
		      "Lease expiry left the lease on the interface %08x", evt);
	zassert_equal(net_if_ipv4_get_gw(iface).s_addr, NET_INADDR_ANY,
		      "Lease expiry left the gateway");
	check_teardown_order();

	check_bound_again(iface, "lease expiry", DISCOVER_RETRY_WAIT);

	net_dhcpv4_stop(iface);
}

/* An offer configures the interface before any lease is granted, so stopping
 * the client short of one still has to take it all back.
 */
ZTEST(dhcpv4_tests, test_stop_before_bind_drops_config)
{
	struct net_if *iface;
	uint32_t evt;

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));
	zassert_not_null(iface, "Interface not available");

	drop_requests = true;

	net_dhcpv4_start(iface);

	evt = k_event_wait_all(&events,
			       EVT_DNS_SERVER1_ADD | EVT_DNS_SERVER2_ADD |
			       EVT_DNS_SERVER3_ADD,
			       false, WAIT_TIME);
	zassert_equal(evt,
		      EVT_DNS_SERVER1_ADD | EVT_DNS_SERVER2_ADD |
		      EVT_DNS_SERVER3_ADD,
		      "Offer did not configure DNS servers %08x", evt);
	zassert_not_equal(net_if_ipv4_get_gw(iface).s_addr, NET_INADDR_ANY,
			  "Offer did not configure the gateway");
	zassert_is_null(net_if_ipv4_addr_lookup_by_iface(iface, &leased_addr),
			"The client bound although the request went unanswered");

	k_event_set(&events, 0U);

	net_dhcpv4_stop(iface);

	evt = k_event_wait_all(&events, EVT_DHCP_STOP | EVT_DNS_ALL_DEL,
			       false, WAIT_TIME);
	zassert_equal(evt, EVT_DHCP_STOP | EVT_DNS_ALL_DEL,
		      "Stopping short of a lease left the DNS servers %08x", evt);
	zassert_equal(net_if_ipv4_get_gw(iface).s_addr, NET_INADDR_ANY,
		      "Stopping short of a lease left the offer's gateway");
}

/* A NAK to the request that follows an offer restarts the configuration,
 * but not before the delay that keeps a refusing server from driving a
 * discover loop.
 */
ZTEST(dhcpv4_tests, test_nak_to_request_paces_the_restart)
{
	struct net_if *iface;
	uint32_t evt;
	int seen;

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));
	zassert_not_null(iface, "Interface not available");

	nak_requests = true;

	net_dhcpv4_start(iface);

	evt = k_event_wait(&events, EVT_DHCP_NAK, false, WAIT_TIME);
	zassert_equal(evt, EVT_DHCP_NAK, "Request was not refused %08x", evt);

	seen = discovers_seen;

	/* The delay is the constant plus up to two seconds of jitter, so look
	 * just before it can have elapsed and well after it must have.
	 */
	k_sleep(K_SECONDS(DHCPV4_RESTART_DELAY - 1));
	zassert_equal(discovers_seen, seen,
		      "Client restarted without waiting, saw %d discover(s)",
		      discovers_seen - seen);

	k_sleep(K_SECONDS(5));
	zassert_true(discovers_seen > seen, "Client did not restart after the delay");

	net_dhcpv4_stop(iface);
}

/* The lease's router replaces a gateway that was already there, and giving
 * the lease up puts that one back.
 */
ZTEST(dhcpv4_tests, test_lease_gateway_gives_back_the_previous_one)
{
	struct net_if *iface;
	uint32_t evt;

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));
	zassert_not_null(iface, "Interface not available");

	net_if_ipv4_set_gw(iface, &static_gw);

	net_dhcpv4_start(iface);

	evt = k_event_wait_all(&events, EVT_DHCP_BOUND | EVT_ADDR_ADD, false, WAIT_TIME);
	zassert_equal(evt, EVT_DHCP_BOUND | EVT_ADDR_ADD, "Missing DHCP bound %08x", evt);
	zassert_not_equal(net_if_ipv4_get_gw(iface).s_addr, static_gw.s_addr,
			  "The lease did not install its own router");

	net_dhcpv4_stop(iface);

	zassert_equal(net_if_ipv4_get_gw(iface).s_addr, static_gw.s_addr,
		      "Giving the lease up did not give back the earlier gateway");
}

/* A gateway the client did not install is not the client's to take, however
 * the exchange ends.
 */
ZTEST(dhcpv4_tests, test_offer_without_router_keeps_the_gateway)
{
	struct net_if *iface;
	uint32_t evt;

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));
	zassert_not_null(iface, "Interface not available");

	net_if_ipv4_set_gw(iface, &static_gw);
	no_router_option = true;

	net_dhcpv4_start(iface);

	evt = k_event_wait_all(&events, EVT_DHCP_BOUND | EVT_ADDR_ADD, false, WAIT_TIME);
	zassert_equal(evt, EVT_DHCP_BOUND | EVT_ADDR_ADD, "Missing DHCP bound %08x", evt);
	zassert_equal(net_if_ipv4_get_gw(iface).s_addr, static_gw.s_addr,
		      "A reply without a router option cleared the gateway");

	net_dhcpv4_stop(iface);

	zassert_equal(net_if_ipv4_get_gw(iface).s_addr, static_gw.s_addr,
		      "Stopping cleared a gateway the client never set");
}

/* A router option naming no router installs nothing, so a gateway that was
 * already there stays.
 */
ZTEST(dhcpv4_tests, test_router_option_naming_no_router_keeps_the_gateway)
{
	struct net_if *iface;
	uint32_t evt;

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));
	zassert_not_null(iface, "Interface not available");

	net_if_ipv4_set_gw(iface, &static_gw);
	zero_router_option = true;

	net_dhcpv4_start(iface);

	evt = k_event_wait_all(&events, EVT_DHCP_BOUND | EVT_ADDR_ADD, false, WAIT_TIME);
	zassert_equal(evt, EVT_DHCP_BOUND | EVT_ADDR_ADD, "Missing DHCP bound %08x", evt);
	zassert_equal(net_if_ipv4_get_gw(iface).s_addr, static_gw.s_addr,
		      "A router option naming no router replaced the gateway");

	net_dhcpv4_stop(iface);

	zassert_equal(net_if_ipv4_get_gw(iface).s_addr, static_gw.s_addr,
		      "Stopping cleared a gateway the client never set");
}

/* A reply for the exchange the client was running when it was stopped
 * configures nothing.
 */
ZTEST(dhcpv4_tests, test_reply_after_stop_is_ignored)
{
	struct net_if *iface;
	struct net_pkt *pkt;
	uint32_t evt;

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));
	zassert_not_null(iface, "Interface not available");

	net_dhcpv4_start(iface);

	evt = k_event_wait_all(&events, EVT_DHCP_BOUND | EVT_ADDR_ADD, false, WAIT_TIME);
	zassert_equal(evt, EVT_DHCP_BOUND | EVT_ADDR_ADD, "Missing DHCP bound %08x", evt);

	net_dhcpv4_stop(iface);

	evt = k_event_wait(&events, EVT_DHCP_STOP, false, WAIT_TIME);
	zassert_equal(evt, EVT_DHCP_STOP, "Missing DHCP stop");

	k_event_set(&events, 0U);

	/* The reply belongs to the exchange the client was running, so only
	 * the stopped state can make it drop the reply.
	 */
	zassert_equal(iface->config.dhcpv4.xid, request_xid,
		      "The late reply would have been dropped on its identifier");

	pkt = prepare_dhcp_ack(iface, request_xid, true);
	zassert_not_null(pkt, "Failed to build the late ack");
	zassert_ok(net_recv_data(iface, pkt), "Failed to deliver the late ack");

	evt = k_event_wait(&events,
			   EVT_ADDR_ADD | EVT_DHCP_BOUND | EVT_DNS_SERVER1_ADD,
			   false, WAIT_TIME);
	zassert_equal(evt, 0U, "A reply after the stop was acted on %08x", evt);
	zassert_equal(iface->config.dhcpv4.state, NET_DHCPV4_DISABLED,
		      "A reply after the stop left the client in state %s",
		      net_dhcpv4_state_name(iface->config.dhcpv4.state));
	zassert_equal(net_if_ipv4_get_gw(iface).s_addr, NET_INADDR_ANY,
		      "A reply after the stop set the gateway");

	/* An offer carries the same configuration and takes a different path
	 * through the handler.
	 */
	pkt = prepare_dhcp_offer(iface, request_xid);
	zassert_not_null(pkt, "Failed to build the late offer");
	zassert_ok(net_recv_data(iface, pkt), "Failed to deliver the late offer");

	evt = k_event_wait(&events,
			   EVT_ADDR_ADD | EVT_DHCP_BOUND | EVT_DNS_SERVER1_ADD,
			   false, WAIT_TIME);
	zassert_equal(evt, 0U, "An offer after the stop was acted on %08x", evt);
	zassert_equal(net_if_ipv4_get_gw(iface).s_addr, NET_INADDR_ANY,
		      "An offer after the stop set the gateway");

	/* A NAK is the reply that would otherwise put a stopped client into a
	 * state no timer serves.
	 */
	pkt = prepare_dhcp_nak(iface, request_xid);
	zassert_not_null(pkt, "Failed to build the late nak");
	zassert_ok(net_recv_data(iface, pkt), "Failed to deliver the late nak");

	k_sleep(K_MSEC(100));

	zassert_equal(iface->config.dhcpv4.state, NET_DHCPV4_DISABLED,
		      "A NAK after the stop left the client in state %s",
		      net_dhcpv4_state_name(iface->config.dhcpv4.state));

	/* The client is still one that can be started. */
	net_dhcpv4_start(iface);

	evt = k_event_wait_all(&events, EVT_DHCP_BOUND | EVT_ADDR_ADD, false, WAIT_TIME);
	zassert_equal(evt, EVT_DHCP_BOUND | EVT_ADDR_ADD,
		      "Client did not start again after a NAK while stopped %08x", evt);

	net_dhcpv4_stop(iface);
}

/* An application that starts the client again when it hears the leased
 * address go away gets a client that works. In the scenario where the
 * callbacks run where the event is raised, this re-enters the client from
 * inside the stop that raised it.
 */
ZTEST(dhcpv4_tests, test_restart_from_the_address_removed_callback)
{
	struct net_if *iface;
	uint32_t evt;

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));
	zassert_not_null(iface, "Interface not available");

	net_dhcpv4_start(iface);

	evt = k_event_wait_all(&events, EVT_DHCP_BOUND | EVT_ADDR_ADD, false, WAIT_TIME);
	zassert_equal(evt, EVT_DHCP_BOUND | EVT_ADDR_ADD, "Missing DHCP bound %08x", evt);

	k_event_set(&events, 0U);
	event_seq = 0;
	addr_del_seq = 0;
	dns_del_seq = 0;
	start_seq = 0;
	stop_seq = 0;
	restart_on_addr_del = true;

	net_dhcpv4_stop(iface);

	evt = k_event_wait_all(&events, EVT_DHCP_BOUND | EVT_ADDR_ADD, false, WAIT_TIME);
	zassert_equal(evt, EVT_DHCP_BOUND | EVT_ADDR_ADD,
		      "Client did not bind after being started from the callback %08x", evt);

	zassert_not_equal(stop_seq, 0U, "Stopping the client was not announced");

	/* The counters record delivery, which only follows the order the two
	 * were raised in where the callbacks run where the event is raised.
	 */
	if (IS_ENABLED(CONFIG_NET_MGMT_EVENT_DIRECT)) {
		zassert_true(stop_seq < start_seq,
			     "The client was started again before it was said to have stopped");
	}

	net_dhcpv4_stop(iface);
}

/* The other way round: an application that stops the client when it hears
 * the leased address go away gets a client that stays stopped. The restart
 * the teardown was part of is abandoned where it stands, rather than putting
 * a discover on the wire for a client the application has shut down.
 */
ZTEST(dhcpv4_tests, test_stop_from_the_address_removed_callback)
{
	struct net_if *iface;
	uint32_t evt;
	int seen;

	/* Only where the callback runs inside the removal can it be heard
	 * before the restart carries on.
	 */
	Z_TEST_SKIP_IFNDEF(CONFIG_NET_MGMT_EVENT_DIRECT);

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));
	zassert_not_null(iface, "Interface not available");

	bind_and_reach(iface, NET_DHCPV4_REBINDING);

	k_event_set(&events, 0U);

	/* From REBINDING the client sends requests, so the only discover
	 * that can follow is the one the lease running out restarts with.
	 */
	seen = discovers_seen;
	stop_on_addr_del = true;

	evt = k_event_wait_all(&events, EVT_ADDR_DEL, false, LEASE_EXPIRY_WAIT);
	zassert_equal(evt, EVT_ADDR_DEL, "Lease expiry left the lease on the interface %08x",
		      evt);

	/* Long enough for a client that carried on to have sent one. */
	k_sleep(DISCOVER_RETRY_WAIT);

	zassert_equal(discovers_seen, seen, "Discover sent for a client the callback stopped");
	zassert_equal(iface->config.dhcpv4.state, NET_DHCPV4_DISABLED,
		      "Client in state %s, expected DISABLED",
		      net_dhcpv4_state_name(iface->config.dhcpv4.state));
	zassert_is_null(net_if_ipv4_addr_lookup_by_iface(iface, &leased_addr),
			"Stopped client left the leased address behind");
}

/* An ACK puts the address on the interface before the lease is entered, so a
 * callback that stops the client there leaves a reply with no lease to enter.
 * The address it added goes with it.
 */
ZTEST(dhcpv4_tests, test_stop_from_the_address_added_callback)
{
	struct net_if *iface;
	uint32_t evt;

	/* Only where the callback runs inside the addition can it be heard
	 * before the lease is entered.
	 */
	Z_TEST_SKIP_IFNDEF(CONFIG_NET_MGMT_EVENT_DIRECT);

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));
	zassert_not_null(iface, "Interface not available");

	stop_on_addr_add = true;

	net_dhcpv4_start(iface);

	evt = k_event_wait_all(&events, EVT_ADDR_ADD | EVT_ADDR_DEL | EVT_DHCP_STOP,
			       false, WAIT_TIME);
	zassert_equal(evt, EVT_ADDR_ADD | EVT_ADDR_DEL | EVT_DHCP_STOP,
		      "Client did not stop where the address was added %08x", evt);

	zassert_equal(k_event_wait(&events, EVT_DHCP_BOUND, false, K_NO_WAIT), 0U,
		      "Client entered a lease the callback had stopped it out of");
	zassert_equal(iface->config.dhcpv4.state, NET_DHCPV4_DISABLED,
		      "Client in state %s, expected DISABLED",
		      net_dhcpv4_state_name(iface->config.dhcpv4.state));
	zassert_is_null(net_if_ipv4_addr_lookup_by_iface(iface, &leased_addr),
			"Address added by a reply with no lease to enter stayed behind");
}

/* A gateway installed over the lease's own belongs to whoever put it there,
 * so the lease does not take it away.
 */
ZTEST(dhcpv4_tests, test_gateway_replaced_during_the_lease_is_left_alone)
{
	const struct net_in_addr other_gw = { { { 192, 0, 2, 9 } } };
	struct net_if *iface;
	uint32_t evt;

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));
	zassert_not_null(iface, "Interface not available");

	net_dhcpv4_start(iface);

	evt = k_event_wait_all(&events, EVT_DHCP_BOUND | EVT_ADDR_ADD, false, WAIT_TIME);
	zassert_equal(evt, EVT_DHCP_BOUND | EVT_ADDR_ADD, "Missing DHCP bound %08x", evt);

	net_if_ipv4_set_gw(iface, &other_gw);

	net_dhcpv4_stop(iface);

	zassert_equal(net_if_ipv4_get_gw(iface).s_addr, other_gw.s_addr,
		      "Giving the lease up took a gateway the client had not installed");
}

/* A renewal that the server answers keeps everything the lease configured:
 * it is not a path that abandons one.
 */
ZTEST(dhcpv4_tests, test_renewal_keeps_the_lease)
{
	struct net_if *iface;
	uint32_t evt;

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));
	zassert_not_null(iface, "Interface not available");

	short_lease = true;

	net_dhcpv4_start(iface);

	evt = k_event_wait_all(&events, EVT_DHCP_BOUND | EVT_ADDR_ADD, false, WAIT_TIME);
	zassert_equal(evt, EVT_DHCP_BOUND | EVT_ADDR_ADD, "Missing DHCP bound %08x", evt);

	k_event_set(&events, 0U);

	evt = k_event_wait_all(&events, EVT_DHCP_RENEW_REQ | EVT_DHCP_ACK | EVT_DHCP_BOUND,
			       false, SHORT_LEASE_WAIT);
	zassert_equal(evt, EVT_DHCP_RENEW_REQ | EVT_DHCP_ACK | EVT_DHCP_BOUND,
		      "The renewal at T1 was not answered %08x", evt);
	zassert_equal(iface->config.dhcpv4.state, NET_DHCPV4_BOUND,
		      "Client left in state %s after a renewal",
		      net_dhcpv4_state_name(iface->config.dhcpv4.state));

	evt = k_event_wait(&events, EVT_ADDR_DEL | EVT_DNS_ALL_DEL, false, K_SECONDS(2));
	zassert_equal(evt, 0U, "A renewal gave part of the lease up %08x", evt);
	zassert_not_null(net_if_ipv4_addr_lookup_by_iface(iface, &leased_addr),
			 "A renewal took the address off the interface");
	zassert_not_equal(net_if_ipv4_get_gw(iface).s_addr, NET_INADDR_ANY,
			  "A renewal took the gateway away");

	net_dhcpv4_stop(iface);
}

ZTEST_SUITE(dhcpv4_tests, NULL, NULL, dhcpv4_tests_before, NULL, NULL);
