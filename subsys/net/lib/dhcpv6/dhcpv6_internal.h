/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 *  @brief DHCPv6 internal header
 *
 * This header should not be included by the application.
 */

#ifndef DHCPV6_INTERNAL_H_
#define DHCPV6_INTERNAL_H_

#include <zephyr/net/dhcpv6.h>

#define DHCPV6_DUID_TYPE_SIZE 2
#define DHVPV6_DUID_LL_HW_TYPE_SIZE 2
#define DHCPV6_DUID_LL_HEADER_SIZE (DHCPV6_DUID_TYPE_SIZE + \
				    DHVPV6_DUID_LL_HW_TYPE_SIZE)

#define DHCPV6_MSG_TYPE_SIZE 1
#define DHCPV6_HEADER_SIZE (DHCPV6_MSG_TYPE_SIZE + DHCPV6_TID_SIZE)

#define DHCPV6_OPTION_CODE_SIZE 2
#define DHCPV6_OPTION_LENGTH_SIZE 2
#define DHCPV6_OPTION_HEADER_SIZE (DHCPV6_OPTION_CODE_SIZE + \
				   DHCPV6_OPTION_LENGTH_SIZE)

#define DHCPV6_OPTION_PREFERENCE_SIZE 1
#define DHCPV6_OPTION_ELAPSED_TIME_SIZE 2
#define DHCPV6_OPTION_ELAPSED_TIME_SIZE 2
#define DHCPV6_OPTION_IA_NA_HEADER_SIZE 12
#define DHCPV6_OPTION_IAADDR_HEADER_SIZE 24
#define DHCPV6_OPTION_IA_PD_HEADER_SIZE 12
#define DHCPV6_OPTION_IAPREFIX_HEADER_SIZE 25
#define DHCPV6_OPTION_IAADDR_HEADER_SIZE 24
#define DHCPV6_OPTION_IAPREFIX_HEADER_SIZE 25
#define DHCPV6_OPTION_STATUS_CODE_HEADER_SIZE 2

#define DHCPV6_INFINITY UINT32_MAX
#define DHCPV6_MAX_SERVER_PREFERENCE 255

#define DHCPV6_HARDWARE_ETHERNET_TYPE 1

#define DHCPV6_CLIENT_PORT 546
#define DHCPV6_SERVER_PORT 547

/* DHCPv6 Transmission/retransmission timeouts */
#define DHCPV6_SOL_MAX_DELAY 1000 /* Max delay of first Solicit, milliseconds */
#define DHCPV6_SOL_TIMEOUT 1000 /* Initial Solicit timeout, milliseconds */
#define DHCPV6_SOL_MAX_RT 3600000 /* Max Solicit timeout value, milliseconds */
#define DHCPV6_REQ_TIMEOUT 1000 /* Initial Request timeout, milliseconds */
#define DHCPV6_REQ_MAX_RT 30000 /* Max Request timeout value, milliseconds */
#define DHCPV6_REQ_MAX_RC 10 /* Max Request retry attempts */
#define DHCPV6_CNF_MAX_DELAY 1000 /* Max delay of first Confirm, milliseconds */
#define DHCPV6_CNF_TIMEOUT 1000 /* Initial Confirm timeout, milliseconds */
#define DHCPV6_CNF_MAX_RT 4000 /* Max Confirm timeout, milliseconds */
#define DHCPV6_CNF_MAX_RD 10000 /* Max Confirm duration, milliseconds */
#define DHCPV6_REN_TIMEOUT 10000 /* Initial Renew timeout, milliseconds */
#define DHCPV6_REN_MAX_RT 600000 /* Max Renew timeout value, milliseconds */
#define DHCPV6_REB_TIMEOUT 10000 /* Initial Rebind timeout, milliseconds */
#define DHCPV6_REB_MAX_RT 600000 /* Max Rebind timeout value, milliseconds */

/* DUID structures */
struct dhcpv6_duid_llt {
	uint16_t hw_type;
	uint32_t time;
	uint8_t ll_addr[];
} __packed;

struct dhcpv6_duid_en {
	uint32_t enterprise_number;
	uint8_t identifier[];
} __packed;

struct dhcpv6_duid_ll {
	uint16_t hw_type;
	uint8_t ll_addr[];
} __packed;

struct dhcpv6_duid_uuid {
	uint8_t uuid[16];
} __packed;

struct dhcpv6_msg_hdr {
	uint8_t type; /* Message type */
	uint8_t tid[3]; /* Transaction ID */
} __packed;

struct dhcpv6_iaaddr {
	uint32_t preferred_lifetime;
	uint32_t valid_lifetime;
	struct in6_addr addr;
	uint16_t status;
};

struct dhcpv6_ia_na {
	uint32_t iaid;
	uint32_t t1;
	uint32_t t2;
	uint16_t status;
	struct dhcpv6_iaaddr iaaddr;
};

struct dhcpv6_iaprefix {
	uint32_t preferred_lifetime;
	uint32_t valid_lifetime;
	struct in6_addr prefix;
	uint8_t prefix_len;
	uint16_t status;
};

struct dhcpv6_ia_pd {
	uint32_t iaid;
	uint32_t t1;
	uint32_t t2;
	uint16_t status;
	struct dhcpv6_iaprefix iaprefix;
};

/* DHCPv6 message types, RFC8415, ch. 7.3. */
enum dhcpv6_msg_type {
	DHCPV6_MSG_TYPE_SOLICIT = 1,
	DHCPV6_MSG_TYPE_ADVERTISE = 2,
	DHCPV6_MSG_TYPE_REQUEST = 3,
	DHCPV6_MSG_TYPE_CONFIRM = 4,
	DHCPV6_MSG_TYPE_RENEW = 5,
	DHCPV6_MSG_TYPE_REBIND = 6,
	DHCPV6_MSG_TYPE_REPLY = 7,
	DHCPV6_MSG_TYPE_RELEASE = 8,
	DHCPV6_MSG_TYPE_DECLINE = 9,
	DHCPV6_MSG_TYPE_RECONFIGURE = 10,
	DHCPV6_MSG_TYPE_INFORMATION_REQUEST = 11,
	DHCPV6_MSG_TYPE_RELAY_FORW = 12,
	DHCPV6_MSG_TYPE_RELAY_REPL = 13,
};

/* DHCPv6 option codes, RFC8415, ch. 21. */
enum dhcpv6_option_code {
	DHCPV6_OPTION_CODE_CLIENTID = 1,
	DHCPV6_OPTION_CODE_SERVERID = 2,
	DHCPV6_OPTION_CODE_IA_NA = 3,
	DHCPV6_OPTION_CODE_IA_TA = 4,
	DHCPV6_OPTION_CODE_IAADDR = 5,
	DHCPV6_OPTION_CODE_ORO = 6,
	DHCPV6_OPTION_CODE_PREFERENCE = 7,
	DHCPV6_OPTION_CODE_ELAPSED_TIME = 8,
	DHCPV6_OPTION_CODE_RELAY_MSG = 9,
	DHCPV6_OPTION_CODE_AUTH = 11,
	DHCPV6_OPTION_CODE_UNICAST = 12,
	DHCPV6_OPTION_CODE_STATUS_CODE = 13,
	DHCPV6_OPTION_CODE_RAPID_COMMIT = 14,
	DHCPV6_OPTION_CODE_USER_CLASS = 15,
	DHCPV6_OPTION_CODE_VENDOR_CLASS = 16,
	DHCPV6_OPTION_CODE_VENDOR_OPTS = 17,
	DHCPV6_OPTION_CODE_INTERFACE_ID = 18,
	DHCPV6_OPTION_CODE_RECONF_MSG = 19,
	DHCPV6_OPTION_CODE_RECONF_ACCEPT = 20,
	DHCPV6_OPTION_CODE_IA_PD = 25,
	DHCPV6_OPTION_CODE_IAPREFIX = 26,
	DHCPV6_OPTION_CODE_INFORMATION_REFRESH_TIME = 32,
	DHCPV6_OPTION_CODE_SOL_MAX_RT = 82,
	DHCPV6_OPTION_CODE_INF_MAX_RT = 83,
};

/* DHCPv6 option codes, RFC8415, ch. 21.13. */
enum dhcpv6_status_code {
	DHCPV6_STATUS_SUCCESS = 0,
	DHCPV6_STATUS_UNSPEC_FAIL = 1,
	DHCPV6_STATUS_NO_ADDR_AVAIL = 2,
	DHCPV6_STATUS_NO_BINDING = 3,
	DHCPV6_STATUS_NOT_ON_LINK = 4,
	DHCPV6_STATUS_USE_MULTICAST = 5,
	DHCPV6_STATUS_NO_PREFIX_AVAIL = 6,
};

/* DHCPv6 Unique Identifier types, RFC8415, ch. 11.1. */
enum dhcpv6_duid_type {
	DHCPV6_DUID_TYPE_LLT = 1, /* Based on Link-Layer Address Plus Time */
	DHCPV6_DUID_TYPE_EN = 2, /* Assigned by Vendor Based on Enterprise Number */
	DHCPV6_DUID_TYPE_LL = 3, /* Based on Link-Layer Address */
	DHCPV6_DUID_TYPE_UUID = 4, /* Based on Universally Unique Identifier */
};

#if defined(CONFIG_NET_DHCPV6)
int net_dhcpv6_init(void);
#else
static inline int net_dhcpv6_init(void)
{
	return 0;
}
#endif /* CONFIG_NET_DHCPV6 */

#endif /* DHCPV6_INTERNAL_H_ */
