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

#ifndef _NETZ_H_
#define _NETZ_H_

#include <net/ip_buf.h>
#include <net/net_core.h>

#include "app_buf.h"

struct __deprecated netz_ctx_t;

struct netz_ctx_t {
	struct net_context *net_ctx;
	int connected;

	int rx_timeout;
	int tx_retry_timeout;

	struct net_addr host;
	int host_port;

	struct net_addr remote;
	int remote_port;

	enum ip_protocol proto;
};

#define TCP_COMMON_TIMEOUT	20

/**
 * @brief NETZ_CTX_INIT		Initializes a netz context with default values
 */
#define NETZ_CTX_INIT {	.rx_timeout = TCP_COMMON_TIMEOUT,		\
			.tx_retry_timeout = TCP_COMMON_TIMEOUT,		\
			.net_ctx = NULL,				\
			.connected = 0,					\
			.proto = IPPROTO_UDP,				\
			.host_port = 0,					\
			.remote_port = 0}

/**
 * @brief NET_ADDR_IPV4_INIT	Initializes a net_addr structure with
 *				an IPv4 address specified by a1, a2, a3 and a4
 */
#define NET_ADDR_IPV4_INIT(a1, a2, a3, a4) {.in_addr.in4_u.u4_addr8[0] = (a1),\
					    .in_addr.in4_u.u4_addr8[1] = (a2),\
					    .in_addr.in4_u.u4_addr8[2] = (a3),\
					    .in_addr.in4_u.u4_addr8[3] = (a4),\
					    .family = AF_INET}

/**
 * @brief netz_host		Sets the host IPv4 address (no IPv6 support)
 * @param ctx			netz context structure
 * @param host			Network address
 */
void __deprecated netz_host(struct netz_ctx_t *ctx, struct net_addr *host);

/**
 * @brief netz_host_ipv4	Sets the host IPv4 address (no IPv6 support)
 * @param ctx			netz context structure
 * @param a1			Byte 0 of the IPv4 address
 * @param a2			Byte 1 of the IPv4 address
 * @param a3			Byte 2 of the IPv4 address
 * @param a4			Byte 3 of the IPv4 address
 */
void __deprecated netz_host_ipv4(struct netz_ctx_t *ctx, uint8_t a1,
				 uint8_t a2, uint8_t a3, uint8_t a4);

/**
 * @brief netz_netmask		Sets the host's netmask address
 * @param ctx			netz context structure
 * @param netmask		Network address to be used as netmask
 */
void __deprecated netz_netmask(struct netz_ctx_t *ctx,
			       struct net_addr *netmask);

/**
 * @brief netz_netmask_ipv4	Sets the host's netmask IPv4 address
 * @param ctx			netz context structure
 * @param n1			Byte 0 of the IPv4 address
 * @param n2			Byte 1 of the IPv4 address
 * @param n3			Byte 2 of the IPv4 address
 * @param n4			Byte 3 of the IPv4 address
 */
void __deprecated netz_netmask_ipv4(struct netz_ctx_t *ctx, uint8_t n1,
				    uint8_t n2, uint8_t n3, uint8_t n4);
/**
 * @brief netz_remote		Sets the address of the remote peer
 * @param ctx			netz context structure
 * @param remote		Network address of the remote peer
 * @param port			Port number of the remote peer
 */
void __deprecated netz_remote(struct netz_ctx_t *ctx, struct net_addr *remote,
			      int port);

/**
 * @brief netz_remote_ipv4	Sets the IPv4 address of the remote peer
 * @param ctx			netz context structure
 * @param a1			Byte 0 of the IPv4 address
 * @param a2			Byte 1 of the IPv4 address
 * @param a3			Byte 2 of the IPv4 address
 * @param a4			Byte 3 of the IPv4 address
 */

void __deprecated netz_remote_ipv4(struct netz_ctx_t *ctx, uint8_t a1,
				   uint8_t a2, uint8_t a3, uint8_t a4,
				   int port);

/**
 * @brief netz_tcp		Initializes the netz context & connects
 *				to the remote peer
 * @param ctx			netz context structure
 * @return			0 on success
 * @return			-EINVAL if a null context was obtained
 * @return			Read netz_tx return codes
 */
int __deprecated netz_tcp(struct netz_ctx_t *ctx);

/**
 * @brief netz_udp		Initializes the context for UDP transfers
 * @param ctx			netz context structure
 * @return			0 on success
 * @return			-EINVAL if a null context was obtained
 */
int __deprecated netz_udp(struct netz_ctx_t *ctx);

/**
 * @brief netz_tx		TCP/UDP data transmission
 * @param ctx			netz context structure
 * @param buf			Buffer that contains the data to be sent
 * @return			0 on success
 * @return			-EINVAL if no network buffer is available
 * @return			-EIO if a TCP error was detected
 */
int __deprecated netz_tx(struct netz_ctx_t *ctx, struct app_buf_t *buf);

/**
 * @brief netz_rx		TCP/UDP data reception
 * @param ctx			netz context structure
 * @param buf			Buffer that contains the received data
 * @return			0 on success
 * @return			-EIO on TCP or network buffer error
 * @return			-ENOMEM if the space in buf is not enough
 *				to store the received data
 */
int __deprecated netz_rx(struct netz_ctx_t *ctx, struct app_buf_t *buf);

#endif
