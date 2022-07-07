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
#include <zephyr/sys/printk.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_pkt.h>

#ifdef CONFIG_NET_MGMT_EVENT_INFO

#include <zephyr/net/net_event.h>

/* Maximum size of "struct net_event_ipv6_addr" or
 * "struct net_event_ipv6_nbr" or "struct net_event_ipv6_route".
 * NOTE: Update comments here and calculate which struct occupies max size.
 */

#ifdef CONFIG_NET_L2_WIFI_MGMT

#include <zephyr/net/wifi_mgmt.h>
#define NET_EVENT_INFO_MAX_SIZE sizeof(struct wifi_scan_result)

#else

#if defined(CONFIG_NET_DHCPV4)
#define NET_EVENT_INFO_MAX_SIZE sizeof(struct net_if_dhcpv4)
#else
#define NET_EVENT_INFO_MAX_SIZE sizeof(struct net_event_ipv6_route)
#endif

#endif /* CONFIG_NET_L2_WIFI_MGMT */
#endif /* CONFIG_NET_MGMT_EVENT_INFO */

#include "connection.h"

extern void net_if_init(void);
extern void net_if_post_init(void);
extern void net_if_carrier_down(struct net_if *iface);
extern void net_if_stats_reset(struct net_if *iface);
extern void net_if_stats_reset_all(void);
extern void net_process_rx_packet(struct net_pkt *pkt);
extern void net_process_tx_packet(struct net_pkt *pkt);

#if defined(CONFIG_NET_NATIVE) || defined(CONFIG_NET_OFFLOAD)
extern void net_context_init(void);
extern const char *net_context_state(struct net_context *context);
extern void net_pkt_init(void);
extern void net_tc_tx_init(void);
extern void net_tc_rx_init(void);
#else
static inline void net_context_init(void) { }
static inline void net_pkt_init(void) { }
static inline void net_tc_tx_init(void) { }
static inline void net_tc_rx_init(void) { }
static inline const char *net_context_state(struct net_context *context)
{
	ARG_UNUSED(context);
	return NULL;
}
#endif

#if defined(CONFIG_NET_NATIVE)
enum net_verdict net_ipv4_input(struct net_pkt *pkt);
enum net_verdict net_ipv6_input(struct net_pkt *pkt, bool is_loopback);
#else
static inline enum net_verdict net_ipv4_input(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	return NET_CONTINUE;
}

static inline enum net_verdict net_ipv6_input(struct net_pkt *pkt,
					      bool is_loopback)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(is_loopback);

	return NET_CONTINUE;
}
#endif
extern bool net_tc_submit_to_tx_queue(uint8_t tc, struct net_pkt *pkt);
extern void net_tc_submit_to_rx_queue(uint8_t tc, struct net_pkt *pkt);
extern enum net_verdict net_promisc_mode_input(struct net_pkt *pkt);

char *net_sprint_addr(sa_family_t af, const void *addr);

#define net_sprint_ipv4_addr(_addr) net_sprint_addr(AF_INET, _addr)

#define net_sprint_ipv6_addr(_addr) net_sprint_addr(AF_INET6, _addr)

#if defined(CONFIG_COAP)
/**
 * @brief CoAP init function declaration. It belongs here because we don't want
 * to expose it as a public API -- it should only be called once, and only by
 * net_core.
 */
extern void net_coap_init(void);
#else
static inline void net_coap_init(void)
{
	return;
}
#endif



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
#define net_gptp_recv(iface, pkt) NET_DROP
#endif /* CONFIG_NET_GPTP */

#if defined(CONFIG_NET_IPV6_FRAGMENT)
int net_ipv6_send_fragmented_pkt(struct net_if *iface, struct net_pkt *pkt,
				 uint16_t pkt_len);
#endif

extern const char *net_proto2str(int family, int proto);
extern char *net_byte_to_hex(char *ptr, uint8_t byte, char base, bool pad);
extern char *net_sprint_ll_addr_buf(const uint8_t *ll, uint8_t ll_len,
				    char *buf, int buflen);
extern uint16_t net_calc_chksum(struct net_pkt *pkt, uint8_t proto);

/**
 * @brief Deliver the incoming packet through the recv_cb of the net_context
 *        to the upper layers
 *
 * @param conn		Network connection
 * @param pkt		Network packet
 * @param ip_hdr	Pointer to IP header, optional
 * @param proto_hdr	Pointer to transport layer protocol header, optional
 * @param user_data	User data passed as an argument
 *
 * @return NET_OK	if the packet is consumed through the recv_cb
 *         NET_DROP	if the recv_cb isn't set
 */
enum net_verdict net_context_packet_received(struct net_conn *conn,
					     struct net_pkt *pkt,
					     union net_ip_header *ip_hdr,
					     union net_proto_header *proto_hdr,
					     void *user_data);

#if defined(CONFIG_NET_IPV4)
extern uint16_t net_calc_chksum_ipv4(struct net_pkt *pkt);
#endif /* CONFIG_NET_IPV4 */

#if defined(CONFIG_NET_IPV4_IGMP)
uint16_t net_calc_chksum_igmp(uint8_t *data, size_t len);
enum net_verdict net_ipv4_igmp_input(struct net_pkt *pkt,
				     struct net_ipv4_hdr *ip_hdr);
#else
#define net_ipv4_igmp_input(...)
#define net_calc_chksum_igmp(data, len) 0U
#endif /* CONFIG_NET_IPV4_IGMP */

static inline uint16_t net_calc_chksum_icmpv6(struct net_pkt *pkt)
{
	return net_calc_chksum(pkt, IPPROTO_ICMPV6);
}

static inline uint16_t net_calc_chksum_icmpv4(struct net_pkt *pkt)
{
	return net_calc_chksum(pkt, IPPROTO_ICMP);
}

static inline uint16_t net_calc_chksum_udp(struct net_pkt *pkt)
{
	uint16_t chksum = net_calc_chksum(pkt, IPPROTO_UDP);

	return chksum == 0U ? 0xffff : chksum;
}

static inline uint16_t net_calc_verify_chksum_udp(struct net_pkt *pkt)
{
	return net_calc_chksum(pkt, IPPROTO_UDP);
}

static inline uint16_t net_calc_chksum_tcp(struct net_pkt *pkt)
{
	return net_calc_chksum(pkt, IPPROTO_TCP);
}

static inline char *net_sprint_ll_addr(const uint8_t *ll, uint8_t ll_len)
{
	static char buf[sizeof("xx:xx:xx:xx:xx:xx:xx:xx")];

	return net_sprint_ll_addr_buf(ll, ll_len, (char *)buf, sizeof(buf));
}

static inline void net_hexdump(const char *str,
			       const uint8_t *packet, size_t length)
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
	char pkt_str[sizeof("0x") + sizeof(intptr_t) * 2];

	if (str && str[0]) {
		LOG_DBG("%s", str);
	}

	snprintk(pkt_str, sizeof(pkt_str), "%p", pkt);

	while (buf) {
		LOG_HEXDUMP_DBG(buf->data, buf->len, pkt_str);
		buf = buf->frags;
	}
}

static inline void net_pkt_print_buffer_info(struct net_pkt *pkt, const char *str)
{
	struct net_buf *buf = pkt->buffer;

	if (str) {
		printk("%s", str);
	}

	printk("%p[%ld]", pkt, atomic_get(&pkt->atomic_ref));

	if (buf) {
		printk("->");
	}

	while (buf) {
		printk("%p[%ld/%u (%u/%u)]", buf, atomic_get(&pkt->atomic_ref),
		       buf->len, net_buf_max_len(buf), buf->size);

		buf = buf->frags;
		if (buf) {
			printk("->");
		}
	}

	printk("\n");
}
