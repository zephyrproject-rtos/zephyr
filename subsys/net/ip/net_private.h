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
bool net_header_fits(struct net_pkt *pkt, u8_t *hdr, size_t hdr_size);

struct net_icmp_hdr *net_pkt_icmp_data(struct net_pkt *pkt);
u8_t *net_pkt_icmp_opt_data(struct net_pkt *pkt, size_t opt_len);

/* Check if ICMP header can be directly accessed from memory.
 * If returned value is NULL, then the header was split into
 * multiple fragments and user must use net_pkt_read/write() etc to get/set
 * the ICMP header values.
 * If returned value is not NULL, then the first fragment will
 * hold the ICMP header and returned value will point to start of ICMP header
 * inside net_pkt.
 */
static inline
struct net_icmp_hdr *net_icmp_header_fits(struct net_pkt *pkt,
					  struct net_icmp_hdr *hdr)
{
	if (net_header_fits(pkt, (u8_t *)hdr, sizeof(*hdr))) {
		return hdr;
	}

	return NULL;
}

/* Header may be split between data fragments. In most cases,
 * net_udp_get_hdr() should be used instead.
 */
struct net_udp_hdr *net_pkt_udp_data(struct net_pkt *pkt);

static inline
struct net_udp_hdr *net_udp_header_fits(struct net_pkt *pkt,
					struct net_udp_hdr *hdr)
{
	if (net_header_fits(pkt, (u8_t *)hdr, sizeof(*hdr))) {
		return hdr;
	}

	return NULL;
}

/* Header may be split between data fragments. In most cases,
 * net_tcp_get_hdr() should be used instead.
 */
struct net_tcp_hdr *net_pkt_tcp_data(struct net_pkt *pkt);

static inline
struct net_tcp_hdr *net_tcp_header_fits(struct net_pkt *pkt,
					struct net_tcp_hdr *hdr)
{
	if (net_header_fits(pkt, (u8_t *)hdr, sizeof(*hdr))) {
		return hdr;
	}

	return NULL;
}

void net_pkt_set_appdata_values(struct net_pkt *pkt,
				enum net_ip_protocol proto);

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
static inline void net_hexdump_frags(const char *str,
				     struct net_pkt *pkt, bool full)
{
	struct net_buf *frag = pkt->frags;

	ARG_UNUSED(full);

	if (str && str[0]) {
		LOG_DBG("%s", str);
	}

	while (frag) {
		LOG_HEXDUMP_DBG(frag->data, frag->len, "");
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

	printk("%p[%d]", pkt, atomic_get(&pkt->atomic_ref));

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
