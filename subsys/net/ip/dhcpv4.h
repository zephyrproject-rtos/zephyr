/** @file
 *
 * @brief DHCPv4 handler.
 *
 * This is not to be included by the application.
 */

/*
 * Copyright (c) 2017 ARM Ltd.
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INTERNAL_DHCPV4_H
#define __INTERNAL_DHCPV4_H

struct dhcp_msg {
	uint8_t op;		/* Message type, 1:BOOTREQUEST, 2:BOOTREPLY */
	uint8_t htype;		/* Hardware Address Type */
	uint8_t hlen;		/* Hardware Address length */
	uint8_t hops;		/* used by relay agents when booting via relay
				 * agent, client sets zero
				 */
	uint32_t xid;		/* Transaction ID, random number */
	uint16_t secs;		/* Seconds elapsed since client began address
				 * acquisition or renewal process
				 */
	uint16_t flags;		/* Broadcast or Unicast */
	uint8_t ciaddr[4];		/* Client IP Address */
	uint8_t yiaddr[4];		/* your (client) IP address */
	uint8_t siaddr[4];		/* IP address of next server to use in bootstrap
				 * returned in DHCPOFFER, DHCPACK by server
				 */
	uint8_t giaddr[4];		/* Relat agent IP address */
	uint8_t chaddr[16];	/* Client hardware address */
} __packed;

#define SIZE_OF_SNAME		64
#define SIZE_OF_FILE		128
#define SIZE_OF_MAGIC_COOKIE	4

#define DHCPV4_MSG_BROADCAST	0x8000
#define DHCPV4_MSG_UNICAST	0x0000

#define DHCPV4_MSG_BOOT_REQUEST	1
#define DHCPV4_MSG_BOOT_REPLY	2

#define HARDWARE_ETHERNET_TYPE	1
#define HARDWARE_ETHERNET_LEN	6

#define DHCPV4_SERVER_PORT	67
#define DHCPV4_CLIENT_PORT	68

/* These enumerations represent RFC2131 defined msy type codes, hence
 * they should not be renumbered.
 */
enum dhcpv4_msg_type {
	DHCPV4_MSG_TYPE_DISCOVER	= 1,
	DHCPV4_MSG_TYPE_OFFER		= 2,
	DHCPV4_MSG_TYPE_REQUEST		= 3,
	DHCPV4_MSG_TYPE_DECLINE		= 4,
	DHCPV4_MSG_TYPE_ACK		= 5,
	DHCPV4_MSG_TYPE_NAK		= 6,
	DHCPV4_MSG_TYPE_RELEASE		= 7,
	DHCPV4_MSG_TYPE_INFORM		= 8,
};

#define DHCPV4_OPTIONS_SUBNET_MASK	1
#define DHCPV4_OPTIONS_ROUTER		3
#define DHCPV4_OPTIONS_DNS_SERVER	6
#define DHCPV4_OPTIONS_HOST_NAME	12
#define DHCPV4_OPTIONS_REQ_IPADDR	50
#define DHCPV4_OPTIONS_LEASE_TIME	51
#define DHCPV4_OPTIONS_MSG_TYPE		53
#define DHCPV4_OPTIONS_SERVER_ID	54
#define DHCPV4_OPTIONS_REQ_LIST		55
#define DHCPV4_OPTIONS_RENEWAL		58
#define DHCPV4_OPTIONS_REBINDING	59
#define DHCPV4_OPTIONS_END		255

/* Useful size macros */
#define DHCPV4_OLV_MSG_HOST_NAME	2
#define DHCPV4_OLV_MSG_REQ_IPADDR	6
#define DHCPV4_OLV_MSG_TYPE_SIZE	3
#define DHCPV4_OLV_MSG_SERVER_ID	6
#define DHCPV4_OLV_MSG_REQ_LIST		5

#define DHCPV4_OLV_END_SIZE		1

#define DHCPV4_MESSAGE_SIZE		(sizeof(struct dhcp_msg) +	\
					 SIZE_OF_SNAME + SIZE_OF_FILE + \
					 SIZE_OF_MAGIC_COOKIE +		\
					 DHCPV4_OLV_MSG_TYPE_SIZE +	\
					 DHCPV4_OLV_END_SIZE)


/* TODO:
 * 1) Support for UNICAST flag (some dhcpv4 servers will not reply if
 *    DISCOVER message contains BROADCAST FLAG).
 * 2) Support T2(Rebind) timer.
 */

/* Maximum number of REQUEST or RENEWAL retransmits before reverting
 * to DISCOVER.
 */
#define DHCPV4_MAX_NUMBER_OF_ATTEMPTS	3

/* Initial message retry timeout (s).  This timeout increases
 * exponentially on each retransmit.
 * RFC2131 4.1
 */
#define DHCPV4_INITIAL_RETRY_TIMEOUT 4

/* Initial minimum delay in INIT state before sending the
 * initial DISCOVER message. MAx value is defined with
 * CONFIG_NET_DHCPV4_INITIAL_DELAY_MAX. Default max value
 * should be 10.
 * RFC2131 4.1.1
 */
#define DHCPV4_INITIAL_DELAY_MIN 1

#if defined(CONFIG_NET_DHCPV4)

int net_dhcpv4_init(void);

#else

#define net_dhcpv4_init() 0

#endif /* CONFIG_NET_DHCPV4 */

#endif /* __INTERNAL_DHCPV4_H */
