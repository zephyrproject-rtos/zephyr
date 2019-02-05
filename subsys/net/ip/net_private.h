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

#ifdef CONFIG_NET_MGMT_EVENT_INFO

#include <net/net_event.h>

/* Maximum size of "struct net_event_ipv6_addr" or
 * "struct net_event_ipv6_nbr" or "struct net_event_ipv6_route".
 * NOTE: Update comments here and calculate which struct occupies max size.
 */

#ifdef CONFIG_NET_L2_WIFI_MGMT

#include <net/wifi_mgmt.h>
#define NET_EVENT_INFO_MAX_SIZE sizeof(struct wifi_scan_result)

#else

#define NET_EVENT_INFO_MAX_SIZE sizeof(struct net_event_ipv6_route)

#endif /* CONFIG_NET_L2_WIFI_MGMT */
#endif /* CONFIG_NET_MGMT_EVENT_INFO */

#include "connection.h"

extern void net_pkt_init(void);
extern void net_if_init(void);
extern void net_if_post_init(void);
extern void net_if_carrier_down(struct net_if *iface);
extern void net_context_init(void);
enum net_verdict net_ipv4_input(struct net_pkt *pkt);
enum net_verdict net_ipv6_input(struct net_pkt *pkt, bool is_loopback);
extern void net_tc_tx_init(void);
extern void net_tc_rx_init(void);
extern void net_tc_submit_to_tx_queue(u8_t tc, struct net_pkt *pkt);
extern void net_tc_submit_to_rx_queue(u8_t tc, struct net_pkt *pkt);
extern enum net_verdict net_promisc_mode_input(struct net_pkt *pkt);

char *net_sprint_addr(sa_family_t af, const void *addr);

#define net_sprint_ipv4_addr(_addr) net_sprint_addr(AF_INET, _addr)

#define net_sprint_ipv6_addr(_addr) net_sprint_addr(AF_INET6, _addr)

#if defined(CONFIG_NET_GPTP)
/**
 * @brief Initialize Precision Time Protocol Layer.
 */
void net_gptp_init(void);

/**
 * @brief Process a ptp message.
 *
 * @param buf Buffer with a valid PTP Ethernet type.
 *
 * @return Return the policy for network buffer.
 */
enum net_verdict net_gptp_recv(struct net_if *iface, struct net_pkt *pkt);
#else
#define net_gptp_init()
#define net_gptp_recv(iface, pkt)
#endif /* CONFIG_NET_GPTP */

#if defined(CONFIG_NET_IPV6_FRAGMENT)
int net_ipv6_send_fragmented_pkt(struct net_if *iface, struct net_pkt *pkt,
				 u16_t pkt_len);
#endif

extern const char *net_proto2str(int family, int proto);
extern char *net_byte_to_hex(char *ptr, u8_t byte, char base, bool pad);
extern char *net_sprint_ll_addr_buf(const u8_t *ll, u8_t ll_len,
				    char *buf, int buflen);
extern u16_t net_calc_chksum(struct net_pkt *pkt, u8_t proto);

enum net_verdict net_context_packet_received(struct net_conn *conn,
					     struct net_pkt *pkt,
					     union net_ip_header *ip_hdr,
					     union net_proto_header *proto_hdr,
					     void *user_data);

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

static inline char *net_sprint_ll_addr(const u8_t *ll, u8_t ll_len)
{
	static char buf[sizeof("xx:xx:xx:xx:xx:xx:xx:xx")];

	return net_sprint_ll_addr_buf(ll, ll_len, (char *)buf, sizeof(buf));
}

static inline void net_hexdump(const char *str,
			       const u8_t *packet, size_t length)
{
	if (!length) {
		LOG_DBG("%s zero-length packet", str);
		return;
	}

	LOG_HEXDUMP_DBG(packet, length, str);
}


/* Hexdump from all fragments */
static inline void net_pkt_hexdump(struct net_pkt *pkt, const char *str)
{
	struct net_buf *buf = pkt->buffer;

	if (str && str[0]) {
		LOG_DBG("%s", str);
	}

	while (buf) {
		LOG_HEXDUMP_DBG(buf->data, buf->len, "");
		buf = buf->frags;
	}
}

static inline void net_pkt_print_buffer_info(struct net_pkt *pkt, const char *str)
{
	struct net_buf *buf = pkt->buffer;

	if (str) {
		printk("%s", str);
	}

	printk("%p[%d]", pkt, atomic_get(&pkt->atomic_ref));

	if (buf) {
		printk("->");
	}

	while (buf) {
		printk("%p[%d/%u (%u)]",
		       buf, atomic_get(&pkt->atomic_ref), buf->len, buf->size);

		buf = buf->frags;
		if (buf) {
			printk("->");
		}
	}

	printk("\n");
}
