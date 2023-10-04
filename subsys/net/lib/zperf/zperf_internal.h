/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __ZPERF_INTERNAL_H
#define __ZPERF_INTERNAL_H

#include <limits.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/zperf.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/__assert.h>

#define IP6PREFIX_STR2(s) #s
#define IP6PREFIX_STR(p) IP6PREFIX_STR2(p)

#define MY_PREFIX_LEN 64
#define MY_PREFIX_LEN_STR IP6PREFIX_STR(MY_PREFIX_LEN)

/* Note that you can set local endpoint address in config file */
#if defined(CONFIG_NET_IPV6) && defined(CONFIG_NET_CONFIG_SETTINGS)
#define MY_IP6ADDR CONFIG_NET_CONFIG_MY_IPV6_ADDR
#define DST_IP6ADDR CONFIG_NET_CONFIG_PEER_IPV6_ADDR
#define MY_IP6ADDR_SET
#else
#define MY_IP6ADDR NULL
#define DST_IP6ADDR NULL
#endif

#if defined(CONFIG_NET_IPV4) && defined(CONFIG_NET_CONFIG_SETTINGS)
#define MY_IP4ADDR CONFIG_NET_CONFIG_MY_IPV4_ADDR
#define DST_IP4ADDR CONFIG_NET_CONFIG_PEER_IPV4_ADDR
#define MY_IP4ADDR_SET
#else
#define MY_IP4ADDR NULL
#define DST_IP4ADDR NULL
#endif

#define PACKET_SIZE_MAX CONFIG_NET_ZPERF_MAX_PACKET_SIZE

#define MY_SRC_PORT 50000
#define DEF_PORT 5001
#define DEF_PORT_STR STRINGIFY(DEF_PORT)

#define ZPERF_VERSION "1.1"

struct zperf_udp_datagram {
	int32_t id;
	uint32_t tv_sec;
	uint32_t tv_usec;
} __packed;

BUILD_ASSERT(sizeof(struct zperf_udp_datagram) <= PACKET_SIZE_MAX, "Invalid PACKET_SIZE_MAX");

struct zperf_client_hdr_v1 {
	int32_t flags;
	int32_t num_of_threads;
	int32_t port;
	int32_t buffer_len;
	int32_t bandwidth;
	int32_t num_of_bytes;
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

struct zperf_async_upload_context {
	struct k_work work;
	struct zperf_upload_params param;
	zperf_callback callback;
	void *user_data;
};

static inline uint32_t time_delta(uint32_t ts, uint32_t t)
{
	return (t >= ts) ? (t - ts) : (ULONG_MAX - ts + t);
}

int zperf_get_ipv6_addr(char *host, char *prefix_str, struct in6_addr *addr);
struct sockaddr_in6 *zperf_get_sin6(void);

int zperf_get_ipv4_addr(char *host, struct in_addr *addr);
struct sockaddr_in *zperf_get_sin(void);

extern void connect_ap(char *ssid);

const struct in_addr *zperf_get_default_if_in4_addr(void);
const struct in6_addr *zperf_get_default_if_in6_addr(void);

int zperf_prepare_upload_sock(const struct sockaddr *peer_addr, int tos,
			      int proto);

uint32_t zperf_packet_duration(uint32_t packet_size, uint32_t rate_in_kbps);

void zperf_async_work_submit(struct k_work *work);
void zperf_udp_uploader_init(void);
void zperf_tcp_uploader_init(void);
void zperf_udp_receiver_init(void);
void zperf_tcp_receiver_init(void);

void zperf_shell_init(void);

#endif /* __ZPERF_INTERNAL_H */
