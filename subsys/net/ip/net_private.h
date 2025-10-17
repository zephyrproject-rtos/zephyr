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
#include <zephyr/net/icmp.h>

#ifdef CONFIG_NET_MGMT_EVENT_INFO

#include <zephyr/net/net_event.h>

#ifdef CONFIG_NET_L2_WIFI_MGMT
/* For struct wifi_scan_result */
#include <zephyr/net/wifi_mgmt.h>
#endif /* CONFIG_NET_L2_WIFI_MGMT */

#define DEFAULT_NET_EVENT_INFO_SIZE 32
/* NOTE: Update this union with all *big* event info structs */
union net_mgmt_events {
#if defined(CONFIG_NET_DHCPV4)
	struct net_if_dhcpv4 dhcpv4;
#endif /* CONFIG_NET_DHCPV4 */
#if defined(CONFIG_NET_DHCPV6)
	struct net_if_dhcpv6 dhcpv6;
#endif /* CONFIG_NET_DHCPV6 */
#if defined(CONFIG_NET_L2_WIFI_MGMT)
	union wifi_mgmt_events wifi;
#endif /* CONFIG_NET_L2_WIFI_MGMT */
#if defined(CONFIG_NET_IPV6)
	struct net_event_ipv6_prefix ipv6_prefix;
#if defined(CONFIG_NET_IPV6_MLD)
	struct net_event_ipv6_route ipv6_route;
#endif /* CONFIG_NET_IPV6_MLD */
#endif /* CONFIG_NET_IPV6 */
#if defined(CONFIG_NET_HOSTNAME_ENABLE)
	struct net_event_l4_hostname hostname;
#endif /* CONFIG_NET_HOSTNAME_ENABLE */
	char default_event[DEFAULT_NET_EVENT_INFO_SIZE];
};

#define NET_EVENT_INFO_MAX_SIZE sizeof(union net_mgmt_events)

#endif

#include "connection.h"

extern void net_if_init(void);
extern void net_if_post_init(void);
extern void net_if_stats_reset(struct net_if *iface);
extern void net_if_stats_reset_all(void);
extern const char *net_if_oper_state2str(enum net_if_oper_state state);
extern void net_process_rx_packet(struct net_pkt *pkt);
extern void net_process_tx_packet(struct net_pkt *pkt);

extern struct net_if_addr *net_if_ipv4_addr_get_first_by_index(int ifindex);

extern int net_icmp_call_ipv4_handlers(struct net_pkt *pkt,
				       struct net_ipv4_hdr *ipv4_hdr,
				       struct net_icmp_hdr *icmp_hdr);
extern int net_icmp_call_ipv6_handlers(struct net_pkt *pkt,
				       struct net_ipv6_hdr *ipv6_hdr,
				       struct net_icmp_hdr *icmp_hdr);

extern struct net_if *net_ipip_get_virtual_interface(struct net_if *input_iface);

#if defined(CONFIG_NET_STATISTICS_VIA_PROMETHEUS)
extern void net_stats_prometheus_init(struct net_if *iface);
#else
static inline void net_stats_prometheus_init(struct net_if *iface)
{
	ARG_UNUSED(iface);
}
#endif /* CONFIG_NET_STATISTICS_VIA_PROMETHEUS */

#if defined(CONFIG_NET_SOCKETS_SERVICE)
extern void socket_service_init(void);
#else
static inline void socket_service_init(void) { }
#endif

#if defined(CONFIG_NET_NATIVE) || defined(CONFIG_NET_OFFLOAD)
extern void net_context_init(void);
extern const char *net_context_state(struct net_context *context);
extern bool net_context_is_reuseaddr_set(struct net_context *context);
extern bool net_context_is_reuseport_set(struct net_context *context);
extern bool net_context_is_v6only_set(struct net_context *context);
extern bool net_context_is_recv_pktinfo_set(struct net_context *context);
extern bool net_context_is_recv_hoplimit_set(struct net_context *context);
extern bool net_context_is_timestamping_set(struct net_context *context);
extern void net_pkt_init(void);
int net_context_get_local_addr(struct net_context *context,
			       struct sockaddr *addr,
			       socklen_t *addrlen);
#else
static inline void net_context_init(void) { }
static inline void net_pkt_init(void) { }
static inline const char *net_context_state(struct net_context *context)
{
	ARG_UNUSED(context);
	return NULL;
}
static inline bool net_context_is_reuseaddr_set(struct net_context *context)
{
	ARG_UNUSED(context);
	return false;
}
static inline bool net_context_is_reuseport_set(struct net_context *context)
{
	ARG_UNUSED(context);
	return false;
}
static inline bool net_context_is_recv_pktinfo_set(struct net_context *context)
{
	ARG_UNUSED(context);
	return false;
}
static inline bool net_context_is_recv_hoplimit_set(struct net_context *context)
{
	ARG_UNUSED(context);
	return false;
}
static inline bool net_context_is_timestamping_set(struct net_context *context)
{
	ARG_UNUSED(context);
	return false;
}

static inline int net_context_get_local_addr(struct net_context *context,
					     struct sockaddr *addr,
					     socklen_t *addrlen)
{
	ARG_UNUSED(context);
	ARG_UNUSED(addr);
	ARG_UNUSED(addrlen);

	return -ENOTSUP;
}
#endif

#if defined(CONFIG_DNS_SOCKET_DISPATCHER)
extern void dns_dispatcher_init(void);
#else
static inline void dns_dispatcher_init(void) { }
#endif

#if defined(CONFIG_MDNS_RESPONDER)
extern void mdns_init_responder(void);
#else
static inline void mdns_init_responder(void) { }
#endif /* CONFIG_MDNS_RESPONDER */

#if defined(CONFIG_DNS_RESOLVER)
#include <zephyr/net/dns_resolve.h>
extern int dns_resolve_name_internal(struct dns_resolve_context *ctx,
				     const char *query,
				     enum dns_query_type type,
				     uint16_t *dns_id,
				     dns_resolve_cb_t cb,
				     void *user_data,
				     int32_t timeout,
				     bool use_cache);
#include <zephyr/net/socket_service.h>
extern int dns_resolve_init_with_svc(struct dns_resolve_context *ctx,
				     const char *servers[],
				     const struct sockaddr *servers_sa[],
				     const struct net_socket_service_desc *svc,
				     uint16_t port, int interfaces[]);
#endif /* CONFIG_DNS_RESOLVER */

#if defined(CONFIG_NET_TEST)
extern void loopback_enable_address_swap(bool swap_addresses);
#endif /* CONFIG_NET_TEST */

#if defined(CONFIG_NET_NATIVE)
enum net_verdict net_ipv4_input(struct net_pkt *pkt);
enum net_verdict net_ipv6_input(struct net_pkt *pkt);
extern void net_tc_tx_init(void);
extern void net_tc_rx_init(void);
#else
static inline enum net_verdict net_ipv4_input(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	return NET_CONTINUE;
}

static inline enum net_verdict net_ipv6_input(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	return NET_CONTINUE;
}

static inline void net_tc_tx_init(void) { }
static inline void net_tc_rx_init(void) { }
#endif
enum net_verdict net_tc_try_submit_to_tx_queue(uint8_t tc, struct net_pkt *pkt,
					       k_timeout_t timeout);
extern enum net_verdict net_tc_submit_to_rx_queue(uint8_t tc, struct net_pkt *pkt);
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

#if defined(CONFIG_NET_SOCKETS_OBJ_CORE)
struct sock_obj_type_raw_stats {
	uint64_t sent;
	uint64_t received;
};

struct sock_obj {
	struct net_socket_register *reg;
	uint64_t create_time; /* in ticks */
	k_tid_t creator;
	int fd;
	int socket_family;
	int socket_type;
	int socket_proto;
	bool init_done;
	struct k_obj_core obj_core;
	struct sock_obj_type_raw_stats stats;
};
#endif /* CONFIG_NET_SOCKETS_OBJ_CORE */

#if defined(CONFIG_NET_IPV6_PE)
/* This is needed by ipv6_pe.c when privacy extension support is enabled */
void net_if_ipv6_start_dad(struct net_if *iface,
			   struct net_if_addr *ifaddr);
#endif

#if defined(CONFIG_NET_GPTP)
/**
 * @brief Initialize Precision Time Protocol Layer.
 */
void net_gptp_init(void);
#else
#define net_gptp_init()
#endif /* CONFIG_NET_GPTP */

#if defined(CONFIG_NET_IPV4_FRAGMENT)
int net_ipv4_send_fragmented_pkt(struct net_if *iface, struct net_pkt *pkt,
				 uint16_t pkt_len, uint16_t mtu);
#endif

#if defined(CONFIG_NET_IPV6_FRAGMENT)
int net_ipv6_send_fragmented_pkt(struct net_if *iface, struct net_pkt *pkt,
				 uint16_t pkt_len, uint16_t mtu);
#endif

extern const char *net_verdict2str(enum net_verdict verdict);
extern const char *net_proto2str(int family, int proto);
extern char *net_byte_to_hex(char *ptr, uint8_t byte, char base, bool pad);
extern char *net_sprint_ll_addr_buf(const uint8_t *ll, uint8_t ll_len,
				    char *buf, int buflen);
extern uint16_t calc_chksum(uint16_t sum_in, const uint8_t *data, size_t len);
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
uint16_t net_calc_chksum_ipv4(struct net_pkt *pkt);
#else
#define net_calc_chksum_ipv4(...) 0U
#endif /* CONFIG_NET_IPV4 */

#if defined(CONFIG_NET_IPV4_IGMP)
/**
 * @brief Initialise the IGMP module for a given interface
 *
 * @param iface		Interface to init IGMP
 */
void net_ipv4_igmp_init(struct net_if *iface);
#endif /* CONFIG_NET_IPV4_IGMP */

#if defined(CONFIG_NET_IPV4_IGMP)
uint16_t net_calc_chksum_igmp(struct net_pkt *pkt);
enum net_verdict net_ipv4_igmp_input(struct net_pkt *pkt,
				     struct net_ipv4_hdr *ip_hdr);
#else
#define net_ipv4_igmp_input(...)
#define net_calc_chksum_igmp(pkt) 0U
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

/**
 * Initialize externally allocated TX packet.
 *
 * @param pkt The network packet to initialize.
 */
void net_pkt_tx_init(struct net_pkt *pkt);
