/** @file
 @brief Network stack private header

 This is not to be included by the application.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <misc/printk.h>
#include <net/net_context.h>
#include <net/nbuf.h>

extern void net_nbuf_init(void);
extern void net_if_init(void);
extern void net_context_init(void);
extern void net_ipv6_init(void);

extern char *net_byte_to_hex(uint8_t *ptr, uint8_t byte, char base, bool pad);
extern char *net_sprint_ll_addr_buf(const uint8_t *ll, uint8_t ll_len,
				    char *buf, int buflen);
extern char *net_sprint_ip_addr_buf(const uint8_t *ip, int ip_len,
				    char *buf, int buflen);
extern uint16_t net_calc_chksum(struct net_buf *buf, uint8_t proto);

#if defined(CONFIG_NET_IPV4)
extern uint16_t net_calc_chksum_ipv4(struct net_buf *buf);
#endif /* CONFIG_NET_IPV4 */

static inline uint16_t net_calc_chksum_icmpv6(struct net_buf *buf)
{
	return net_calc_chksum(buf, IPPROTO_ICMPV6);
}

static inline uint16_t net_calc_chksum_icmpv4(struct net_buf *buf)
{
	return net_calc_chksum(buf, IPPROTO_ICMP);
}

static inline uint16_t net_calc_chksum_udp(struct net_buf *buf)
{
	return net_calc_chksum(buf, IPPROTO_UDP);
}

static inline uint16_t net_calc_chksum_tcp(struct net_buf *buf)
{
	return net_calc_chksum(buf, IPPROTO_TCP);
}

#if NET_LOG_ENABLED > 0
static inline char *net_sprint_ll_addr(const uint8_t *ll, uint8_t ll_len)
{
	static char buf[sizeof("xx:xx:xx:xx:xx:xx:xx:xx")];

	return net_sprint_ll_addr_buf(ll, ll_len, (char *)buf, sizeof(buf));
}

static inline char *net_sprint_ip_addr_ptr(const uint8_t *ptr, uint8_t len)
{
	static char buf[NET_IPV6_ADDR_LEN];

	return net_sprint_ip_addr_buf(ptr, len, (char *)buf, sizeof(buf));
}

static inline char *net_sprint_ipv6_addr(const struct in6_addr *addr)
{
#if defined(CONFIG_NET_IPV6)
	return net_sprint_ip_addr_ptr(addr->s6_addr, 16);
#else
	return NULL;
#endif
}

static inline char *net_sprint_ipv4_addr(const struct in_addr *addr)
{
#if defined(CONFIG_NET_IPV4)
	return net_sprint_ip_addr_ptr(addr->s4_addr, 4);
#else
	return NULL;
#endif
}

static inline char *net_sprint_ip_addr(const struct net_addr *addr)
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

static inline void net_hexdump(const char *str, const uint8_t *packet,
			       size_t length)
{
	int n = 0;

	if (!length) {
		SYS_LOG_DBG("%s zero-length packet", str);
		return;
	}

	while (length--) {
		if (n % 16 == 0) {
			printk("%s %08X ", str, n);
		}

		printk("%02X ", *packet++);

		n++;
		if (n % 8 == 0) {
			if (n % 16 == 0) {
				printk("\n");
			} else {
				printk(" ");
			}
		}
	}

	if (n % 16) {
		printk("\n");
	}
}

/* Hexdump from all fragments */
static inline void net_hexdump_frags(const char *str, struct net_buf *buf)
{
	struct net_buf *frag = buf->frags;

	while (frag) {
		net_hexdump(str, frag->data, net_nbuf_get_len(frag));
		frag = frag->frags;
	}
}

#else /* NET_LOG_ENABLED */

static inline char *net_sprint_ll_addr(const uint8_t *ll, uint8_t ll_len)
{
	ARG_UNUSED(ll);
	ARG_UNUSED(ll_len);

	return NULL;
}

static inline char *net_sprint_ip_addr_ptr(const uint8_t *ptr, uint8_t len)
{
	ARG_UNUSED(ptr);
	ARG_UNUSED(len);

	return NULL;
}

static inline char *net_sprint_ipv6_addr(const struct in6_addr *addr)
{
	ARG_UNUSED(addr);

	return NULL;
}

static inline char *net_sprint_ipv4_addr(const struct in_addr *addr)
{
	ARG_UNUSED(addr);

	return NULL;
}

static inline char *net_sprint_ip_addr(const struct net_addr *addr)
{
	ARG_UNUSED(addr);

	return NULL;
}

#define net_hexdump(str, packet, length)
#define net_hexdump_frags(...)

#endif /* NET_LOG_ENABLED */
