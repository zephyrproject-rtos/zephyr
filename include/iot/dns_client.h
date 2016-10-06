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

#ifndef _DNS_CLIENT_H_
#define _DNS_CLIENT_H_

enum dns_query_type {
	DNS_QUERY_TYPE_A = 1,	 /* IPv4 */
	DNS_QUERY_TYPE_AAAA = 28 /* IPv6 */
};

#include <net/net_context.h>
#include <net/net_ip.h>

/**
 * @brief dns_init		DNS resolver initialization routine
 * @details			This routine must be called before any other
 *				dns routine.
 * @return			0, always.
 *				Note: new versions may return error codes.
 */
int dns_init(void);

/**
 * @brief dns4_resolve		Retrieves the IPv4 addresses associated to the
 *				domain name 'name'.
 * @details			This routine obtains the IPv4 addresses
 *				associated to the domain name 'name'.
 *				The DNS server is specified by the sockaddr
 *				structure.
 *				Depending on the DNS server used, one or more
 *				IP addresses may be recovered by this routine.
 *				NOTE: You can use an IPv6 DNS server to look-up
 *				for IPv4 addresses or an IPv4 server to look-up
 *				for IPv6 address. Domain name services are not
 *				tied to any specific routing or transport
 *				technology.
 * @param [in] ctx		Previously initialized network context.
 * @param [out] addresses	An array of IPv4 addresses.
 * @param [out] items		Number of IPv4 addresses stored in 'addresses'.
 * @param [in] elements		Available positions in the addresses array.
 * @param [in] name		C-string containing the Domain Name to resolve,
 *				i.e. 'example.com'.
 * @param [in] dns_server	IP address and port number of the DNS server.
 * @param [in] timeout		RX/TX timeout, for example: TICKS_UNLIMITED.
 *				This timeout is also used when a buffer is
 *				required from the pool.
 * @return			0 on success
 *
 *				Number of returned addresses may be less than
 *				the one reported by the DNS server. So, it is
 *				considered a success because we are 'resolving'
 *				the 'name'.
 * @return			-EIO on network error.
 * @return			-EINVAL if an invalid parameter was passed as
 *				an argument to this routine. This value is also
 *				returned if the application received a malformed
 *				packet from the DNS server.
 * @return			-ENOMEM if there are no buffers available.
 */
int dns4_resolve(struct net_context *ctx, struct in_addr *addresses,
		 int *items, int elements, char *name,
		 struct sockaddr *dns_server, uint32_t timeout);

/**
 * @brief dns6_resolve		Retrieves the IPv6 addresses associated to the
 *				domain name 'name'.
 * @details			See 'details' at the 'dns4_resolve' routine.
 * @param [in] ctx		Previously initialized network context.
 * @param [out] addresses	An array of IPv6 addresses.
 * @param [out] items		Number of IPv6 addresses stored in 'addresses'.
 * @param [in] elements		Available positions in the addresses array.
 * @param [in] name		C-string containing the Domain Name to resolve,
 *				i.e. 'example.com'.
 * @param [in] dns_server	IP address and port number of the DNS server.
 * @param [in] timeout		RX/TX timeout, for example: TICKS_UNLIMITED.
 *				This timeout is also used when a buffer is
 *				required from the pool.
 * @return			0 on success
 *
 *				Number of returned addresses may be less than
 *				the one reported by the DNS server. So, it is
 *				considered a success because we are 'resolving'
 *				the 'name'.
 * @return			-EIO on network error
 * @return			-EINVAL if an invalid parameter was passed as
 *				an argument to this routine. This value is also
 *				returned if the application received a malformed
 *				packet from the DNS server.
 * @return			-ENOMEM if there are no buffers available
 */
int dns6_resolve(struct net_context *ctx, struct in6_addr *addresses,
		 int *items, int elements, char *name,
		 struct sockaddr *dns_server, uint32_t timeout);

/**
 * @brief dns4_resolve_quick	Retrives one IPv4 address associated to the
 *				domain name 'name'.
 * @details			See 'details' at the 'dns4_resolve' routine.
 * @param [in] ctx		Previously initialized network context.
 * @param [out] address		IPv4 address.
 * @param [in] name		C-string containing the Domain Name to resolve,
 *				i.e. 'example.com'.
 * @param [in] dns_server	IP address and port number of the DNS server.
 * @param [in] timeout		RX/TX timeout, for example: TICKS_UNLIMITED.
 *				This timeout is also used when a buffer is
 *				required from the pool.
 * @return			0 on success
 *
 *				Number of returned addresses may be less than
 *				the one reported by the DNS server. So, it is
 *				considered a success because we are 'resolving'
 *				the 'name'.
 * @return			-EIO on network error
 * @return			-EINVAL if an invalid parameter was passed as
 *				an argument to this routine. This value is also
 *				returned if the application received a malformed
 *				packet from the DNS server.
 * @return			-ENOMEM if there are no buffers available
 */
static inline
int dns4_resolve_quick(struct net_context *ctx, struct in_addr *address,
		       char *name, struct sockaddr *dns_server,
		       uint32_t timeout)
{
	int elements = 1;
	int items;

	return dns4_resolve(ctx, address, &items, elements, name,
			    dns_server, timeout);

}

/**
 * @brief dns6_resolve_quick	Retrives one IPv6 address associated to the
 *				domain name 'name'.
 * @details			See 'details' at the 'dns4_resolve' routine.
 * @param [in] ctx		Previously initialized network context.
 * @param [out] address		IPv6 address.
 * @param [in] name		C-string containing the Domain Name to resolve,
 *				i.e. 'example.com'.
 * @param [in] dns_server	IP address and port number of the DNS server.
 * @param [in] timeout		RX/TX timeout, for example: TICKS_UNLIMITED.
 *				This timeout is also used when a buffer is
 *				required from the pool.
 * @return			0 on success
 *
 *				Number of returned addresses may be less than
 *				the one reported by the DNS server. So, it is
 *				considered a success because we are 'resolving'
 *				the 'name'.
 * @return			-EIO on network error
 * @return			-EINVAL if an invalid parameter was passed as
 *				an argument to this routine. This value is also
 *				returned if the application received a malformed
 *				packet from the DNS server.
 * @return			-ENOMEM if there are no buffers available
 */
static inline
int dns6_resolve_quick(struct net_context *ctx, struct in6_addr *address,
		       char *name, struct sockaddr *dns_server,
		       uint32_t timeout)
{
	int elements = 1;
	int items;

	return dns6_resolve(ctx, address, &items, elements, name,
			    dns_server, timeout);
}

#endif
