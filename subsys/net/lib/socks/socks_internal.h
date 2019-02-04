/*
 * Copyright (c) 2018 Antmicro Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_NET_LIB_SOCKS_SOCKS_INTERNAL_H_
#define ZEPHYR_SUBSYS_NET_LIB_SOCKS_SOCKS_INTERNAL_H_

#define SOCKS5_PKT_MAGIC			0x05
#define SOCKS5_PKT_RSV				0x00

#define SOCKS5_AUTH_METHOD_NOAUTH		0x00
#define SOCKS5_AUTH_METHOD_GSSAPI		0x01
#define SOCKS5_AUTH_METHOD_USERPASS		0x02
#define SOCKS5_AUTH_METHOD_NONEG		0xFF

#define SOCKS5_CMD_CONNECT			0x01
#define SOCKS5_CMD_BIND				0x02
#define SOCKS5_CMD_UDP_ASSOCIATE		0x03

#define SOCKS5_ATYP_IPV4			0x01
#define SOCKS5_ATYP_DOMAINNAME			0x03
#define SOCKS5_ATYP_IPV6			0x04

#define SOCKS5_CMD_RESP_SUCCESS			0x00
#define SOCKS5_CMD_RESP_FAILURE			0x01
#define SOCKS5_CMD_RESP_NOT_ALLOWED		0x02
#define SOCKS5_CMD_RESP_NET_UNREACHABLE		0x03
#define SOCKS5_CMD_RESP_HOST_UNREACHABLE	0x04
#define SOCKS5_CMD_RESP_REFUSED			0x05
#define SOCKS5_CMD_RESP_TTL_EXPIRED		0x06
#define SOCKS5_CMD_RESP_CMD_NOT_SUPPORTED	0x07
#define SOCKS5_CMD_RESP_ATYP_NOT_SUPPORTED	0x08

struct socks5_method_request_common {
	u8_t ver;
	u8_t nmethods;
} __packed;

struct socks5_method_request {
	struct socks5_method_request_common r;
	u8_t methods[255];
} __packed;

struct socks5_method_response {
	u8_t ver;
	u8_t method;
} __packed;

struct socks5_ipv4_addr {
	u8_t addr[4];
	u16_t port;
} __packed;

struct socks5_ipv6_addr {
	u8_t addr[16];
	u16_t port;
} __packed;

struct socks5_command_request_common {
	u8_t ver;
	u8_t cmd;
	u8_t rsv;
	u8_t atyp;
} __packed;

struct socks5_command_request {
	struct socks5_command_request_common r;
	union {
		struct socks5_ipv4_addr ipv4_addr;
		struct socks5_ipv6_addr ipv6_addr;
	};
} __packed;

struct socks5_command_response_common {
	u8_t ver;
	u8_t rep;
	u8_t rsv;
	u8_t atyp;
} __packed;

struct socks5_command_response {
	struct socks5_command_response_common r;
	union {
		struct socks5_ipv4_addr ipv4_addr;
		struct socks5_ipv6_addr ipv6_addr;
	};
} __packed;

#endif /* ZEPHYR_SUBSYS_NET_LIB_SOCKS_SOCKS_INTERNAL_H_ */
