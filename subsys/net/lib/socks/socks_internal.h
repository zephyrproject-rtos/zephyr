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

#define SOCKS_TIMEOUT				K_SECONDS(1)

struct socks5_method_request {
	u8_t ver;
	u8_t nmethods;
	u8_t methods[255];
};

struct socks5_method_response {
	u8_t ver;
	u8_t method;
};

struct socks5_ipv4_addr {
	u8_t addr[4];
	u16_t port;
};

struct socks5_ipv6_addr {
	u8_t addr[16];
	u16_t port;
};

struct socks5_domain_addr {
	u8_t len;
	/* FIXME this will be 'len' bytes long, and the port
	 * will be just after that
	 */
	u8_t addr[255];
	u16_t port;
};

struct socks5_command_request {
	u8_t ver;
	u8_t cmd;
	u8_t rsv;
	u8_t atyp;
	/* Rest is separated */
};

struct socks5_command_response {
	u8_t ver;
	u8_t rep;
	u8_t rsv;
	u8_t atyp;
	/* Rest is separated */
};

#endif /* ZEPHYR_SUBSYS_NET_LIB_SOCKS_SOCKS_INTERNAL_H_ */
