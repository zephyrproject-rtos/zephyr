/** @file
 @brief Network stack private header

 This is not to be included by the application.
 */

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

#include <errno.h>

extern void net_nbuf_init(void);
extern int net_if_init(void);
extern void net_context_init(void);

extern char *net_byte_to_hex(uint8_t *ptr, uint8_t byte, char base, bool pad);
extern char *net_sprint_ll_addr_buf(uint8_t *ll, uint8_t ll_len,
				    char *buf, int buflen);
extern char *net_sprint_ip_addr_buf(uint8_t *ip, int ip_len,
				    char *buf, int buflen);

#if NET_DEBUG > 0
static inline char *net_sprint_ll_addr(uint8_t *ll, uint8_t ll_len)
{
	static char buf[sizeof("xx:xx:xx:xx:xx:xx:xx:xx")];

	return net_sprint_ll_addr_buf(ll, ll_len, (char *)buf, sizeof(buf));
}

static inline char *net_sprint_ip_addr_ptr(uint8_t *ptr, uint8_t len)
{
	static char buf[sizeof("xxxx:xxxx:xxxx:xxxx:xxxx:xxxx")];

	return net_sprint_ip_addr_buf(ptr, len, (char *)buf, sizeof(buf));
}

static inline char *net_sprint_ipv6_addr(struct in6_addr *addr)
{
#if defined(CONFIG_NET_IPV6)
	return net_sprint_ip_addr_ptr(addr->s6_addr, 16);
#else
	return NULL;
#endif
}

static inline char *net_sprint_ipv4_addr(struct in_addr *addr)
{
#if defined(CONFIG_NET_IPV4)
	return net_sprint_ip_addr_ptr(addr->s4_addr, 4);
#else
	return NULL;
#endif
}

static inline char *net_sprint_ip_addr(struct net_addr *addr)
{
	switch (addr->family) {
	case AF_INET6:
#if defined(CONFIG_NET_IPV6)
		return net_sprint_ipv6_addr(&addr->in6_addr);
#else
		break;
#endif
	case AF_INET:
#if defined(CONFIG_NET_IPV4)
		return net_sprint_ipv4_addr(&addr->in_addr);
#else
		break;
#endif
	default:
		break;
	}

	return NULL;
}

#else /* NET_DEBUG */

static inline char *net_sprint_ll_addr(uint8_t *ll, uint8_t ll_len)
{
	return NULL;
}

static inline char *net_sprint_ip_addr_ptr(uint8_t *ptr, uint8_t len)
{
	return NULL;
}

static inline char *net_sprint_ipv6_addr(struct in6_addr *addr)
{
	return NULL;
}

static inline char *net_sprint_ipv4_addr(struct in_addr *addr)
{
	return NULL;
}

static inline char *net_sprint_ip_addr(struct net_addr *addr)
{
	return NULL;
}
#endif /* NET_DEBUG */
