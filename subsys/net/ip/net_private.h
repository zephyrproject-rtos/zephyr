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
#include <net/net_pkt.h>

extern void net_pkt_init(void);
extern void net_if_init(struct k_sem *startup_sync);
extern void net_if_post_init(void);
extern void net_context_init(void);
enum net_verdict net_ipv4_process_pkt(struct net_pkt *pkt);
enum net_verdict net_ipv6_process_pkt(struct net_pkt *pkt);
extern void net_ipv6_init(void);

#if defined(CONFIG_NET_IPV6_FRAGMENT)
int net_ipv6_send_fragmented_pkt(struct net_if *iface, struct net_pkt *pkt,
				 u16_t pkt_len);
#endif

extern const char *net_proto2str(enum net_ip_protocol proto);
extern char *net_byte_to_hex(char *ptr, u8_t byte, char base, bool pad);
extern char *net_sprint_ll_addr_buf(const u8_t *ll, u8_t ll_len,
				    char *buf, int buflen);
extern u16_t net_calc_chksum(struct net_pkt *pkt, u8_t proto);

#if defined(CONFIG_NET_IPV4)
extern u16_t net_calc_chksum_ipv4(struct net_pkt *pkt);
#endif /* CONFIG_NET_IPV4 */

static inline u16_t net_calc_chksum_icmpv6(struct net_pkt *pkt)
{
	return net_calc_chksum(pkt, IPPROTO_ICMPV6);
}

static inline u16_t net_calc_chksum_icmpv4(struct net_pkt *pkt)
{
	return net_calc_chksum(pkt, IPPROTO_ICMP);
}

static inline u16_t net_calc_chksum_udp(struct net_pkt *pkt)
{
	return net_calc_chksum(pkt, IPPROTO_UDP);
}

static inline u16_t net_calc_chksum_tcp(struct net_pkt *pkt)
{
	return net_calc_chksum(pkt, IPPROTO_TCP);
}

#if NET_LOG_ENABLED > 0
static inline char *net_sprint_ll_addr(const u8_t *ll, u8_t ll_len)
{
	static char buf[sizeof("xx:xx:xx:xx:xx:xx:xx:xx")];

	return net_sprint_ll_addr_buf(ll, ll_len, (char *)buf, sizeof(buf));
}

static inline char *net_sprint_ipv6_addr(const struct in6_addr *addr)
{
#if defined(CONFIG_NET_IPV6)
	static char buf[NET_IPV6_ADDR_LEN];

	return net_addr_ntop(AF_INET6, addr, (char *)buf, sizeof(buf));
#else
	return NULL;
#endif
}

static inline char *net_sprint_ipv4_addr(const struct in_addr *addr)
{
#if defined(CONFIG_NET_IPV4)
	static char buf[NET_IPV4_ADDR_LEN];

	return net_addr_ntop(AF_INET, addr, (char *)buf, sizeof(buf));
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

static inline void net_hexdump(const char *str, const u8_t *packet,
			       size_t length)
{
	char output[sizeof("xxxxyyyy xxxxyyyy")];
	int n = 0, k = 0;
	u8_t byte;

	if (!length) {
		SYS_LOG_DBG("%s zero-length packet", str);
		return;
	}

	while (length--) {
		if (n % 16 == 0) {
			printk("%s %08X ", str, n);
		}

		byte = *packet++;

		printk("%02X ", byte);

		if (byte < 0x20 || byte > 0x7f) {
			output[k++] = '.';
		} else {
			output[k++] = byte;
		}

		n++;
		if (n % 8 == 0) {
			if (n % 16 == 0) {
				output[k] = '\0';
				printk(" [%s]\n", output);
				k = 0;
			} else {
				printk(" ");
			}
		}
	}

	if (n % 16) {
		int i;

		output[k] = '\0';

		for (i = 0; i < (16 - (n % 16)); i++) {
			printk("   ");
		}
		if ((n % 16) < 8) {
			printk(" "); /* one extra delimiter after 8 chars */
		}

		printk(" [%s]\n", output);
	}
}

/* Hexdump from all fragments */
static inline void net_hexdump_frags(const char *str, struct net_pkt *pkt)
{
	struct net_buf *frag = pkt->frags;

	while (frag) {
		net_hexdump(str, frag->data, frag->len);
		frag = frag->frags;
	}
}

/* Print fragment chain */
static inline void net_print_frags(const char *str, struct net_pkt *pkt)
{
	struct net_buf *frag = pkt->frags;

	if (str) {
		printk("%s", str);
	}

	printk("%p[%d]", pkt, pkt->ref);

	if (frag) {
		printk("->");
	}

	while (frag) {
		printk("%p[%d/%d]", frag, frag->ref, frag->len);

		frag = frag->frags;
		if (frag) {
			printk("->");
		}
	}

	printk("\n");
}

#else /* NET_LOG_ENABLED */

static inline char *net_sprint_ll_addr(const u8_t *ll, u8_t ll_len)
{
	ARG_UNUSED(ll);
	ARG_UNUSED(ll_len);

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

#define net_print_frags(...)

#endif /* NET_LOG_ENABLED */
