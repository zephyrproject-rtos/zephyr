/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DNS_CLIENT_H_
#define _DNS_CLIENT_H_

#include <net/net_context.h>
#include <net/net_ip.h>

/**
 * @brief DNS Client library
 * @defgroup dns_client DNS Client Library
 * @{
 */

/**
 * DNS query type enum
 */
enum dns_query_type {
	DNS_QUERY_TYPE_A = 1,	 /* IPv4 */
	DNS_QUERY_TYPE_AAAA = 28 /* IPv6 */
};

/**
 * DNS client context structure
 */
struct dns_context {
	/* rx_sem and rx buffer, for internal use only */
	struct k_sem rx_sem;
	struct net_buf *rx_buf;

	/** Previously initialized network context */
	struct net_context *net_ctx;

	/** An array of IPv4 or IPv6 address */
	union {
		struct in_addr *ipv4;
		struct in6_addr *ipv6;
	} address;

	/** IP address and port number of the DNS server. */
	struct sockaddr *dns_server;

	/** RX/TX timeout.
	 *  This timeout is also used when a buffer is
	 *  required from the dns data internal pool.
	 */
	int32_t timeout;

	/** C-string containing the Domain Name to resolve,
	 *  i.e. 'example.com'.
	 */
	const char *name;

	/** Query type: IPv4 or IPv6 */
	uint16_t query_type;

	/** Available positions in the 'address' array. */
	uint8_t elements;

	/** Number of IPv4 addresses stored in 'address'. */
	uint8_t items;
};

/**
 * DNS resolver initialization routine
 *
 * This routine must be called before any other dns routine.
 *
 * @param ctx DNS Client structure
 * @retval 0, new versions may return error codes.
 */
int dns_init(struct dns_context *ctx);

/**
 * Retrieves the IP addresses associated to the domain name 'client->name'.
 *
 * This routine obtains the IP addresses associated to the domain name
 * 'client->name'. The DNS server is specified by the sockaddr structure
 * inside 'client'. Depending on the DNS server used, one or more IP
 * addresses may be recovered by this routine.
 *
 * NOTE: You can use an IPv6 DNS server to look-up for IPv4 addresses or
 * an IPv4 server to look-up for IPv6 address. Domain name services are not
 * tied to any specific routing or transport technology.
 *
 * @param ctx DNS Client structure
 * @retval 0 on success. The number of returned addresses (client->items)
 * may be less than the one reported by the DNS server. However, this situation
 * is considered a success because we are 'resolving' the 'name'.
 * Workaround: increase 'client->elements'.
 * @retval -EIO on network error.
 * @retval -EINVAL if an invalid parameter was passed as an argument to
 * this routine. This value is also returned if the application received
 * a malformed packet from the DNS server.
 * @retval -ENOMEM if there are no buffers available.
 */
int dns_resolve(struct dns_context *ctx);

/**
 * @}
 */

#endif
