/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __ZPERF_INTERNAL_H
#define __ZPERF_INTERNAL_H

#include <limits.h>
#include <net/net_ip.h>

#define IP6PREFIX_STR2(s) #s
#define IP6PREFIX_STR(p) IP6PREFIX_STR2(p)

#define MY_PREFIX_LEN 64
#define MY_PREFIX_LEN_STR IP6PREFIX_STR(MY_PREFIX_LEN)

/* Note that you can set local endpoint address in config file */
#if defined(CONFIG_NET_IPV6) && defined(CONFIG_NET_SAMPLES_IP_ADDRESSES)
#define MY_IP6ADDR CONFIG_NET_SAMPLES_MY_IPV6_ADDR
#define DST_IP6ADDR CONFIG_NET_SAMPLES_PEER_IPV6_ADDR
#endif

#if defined(CONFIG_NET_IPV4) && defined(CONFIG_NET_SAMPLES_IP_ADDRESSES)
#define MY_IP4ADDR CONFIG_NET_SAMPLES_MY_IPV4_ADDR
#define DST_IP4ADDR CONFIG_NET_SAMPLES_PEER_IPV4_ADDR
#endif

#define PACKET_SIZE_MAX      1024

#define HW_CYCLES_TO_USEC(__hw_cycle__) \
	( \
		((uint64_t)(__hw_cycle__) * (uint64_t)sys_clock_us_per_tick) / \
		((uint64_t)sys_clock_hw_cycles_per_tick) \
	)

#define HW_CYCLES_TO_SEC(__hw_cycle__) \
	( \
		((uint64_t)(HW_CYCLES_TO_USEC(__hw_cycle__))) / \
		((uint64_t)USEC_PER_SEC) \
	)

#define USEC_TO_HW_CYCLES(__usec__) \
	( \
		((uint64_t)(__usec__) * (uint64_t)sys_clock_hw_cycles_per_tick) / \
		((uint64_t)sys_clock_us_per_tick) \
	)

#define SEC_TO_HW_CYCLES(__sec__) \
	USEC_TO_HW_CYCLES((uint64_t)(__sec__) * \
	(uint64_t)USEC_PER_SEC)

#define MSEC_TO_HW_CYCLES(__msec__) \
		USEC_TO_HW_CYCLES((uint64_t)(__msec__) * \
		(uint64_t)MSEC_PER_SEC)

struct zperf_udp_datagram {
	int32_t id;
	uint32_t tv_sec;
	uint32_t tv_usec;
};

struct zperf_server_hdr {
	int32_t flags;
	int32_t total_len1;
	int32_t total_len2;
	int32_t stop_sec;
	int32_t stop_usec;
	int32_t error_cnt;
	int32_t outorder_cnt;
	int32_t datagrams;
	int32_t jitter1;
	int32_t jitter2;
};

static inline uint32_t time_delta(uint32_t ts, uint32_t t)
{
	return (t >= ts) ? (t - ts) : (ULONG_MAX - ts + t);
}

#if defined(CONFIG_NET_IPV6)
int zperf_get_ipv6_addr(char *host, char *prefix_str, struct in6_addr *addr,
			const char *str);
struct sockaddr_in6 *zperf_get_sin6(void);
#endif

#if defined(CONFIG_NET_IPV4)
int zperf_get_ipv4_addr(char *host, struct in_addr *addr, const char *str);
struct sockaddr_in *zperf_get_sin(void);
#endif

extern void zperf_udp_upload(struct net_context *net_context,
			     unsigned int duration_in_ms,
			     unsigned int packet_size,
			     unsigned int rate_in_kbps,
			     struct zperf_results *results);

extern void zperf_receiver_init(int port);

#if defined(CONFIG_NET_TCP)
extern void zperf_tcp_receiver_init(int port);
extern void zperf_tcp_uploader_init(struct k_fifo *tx_queue);
extern void zperf_tcp_upload(struct net_context *net_context,
			     unsigned int duration_in_ms,
			     unsigned int packet_size,
			     struct zperf_results *results);
#endif

extern void connect_ap(char *ssid);

#endif /* __ZPERF_INTERNAL_H */
