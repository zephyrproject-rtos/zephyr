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
#ifndef __ZPERF_INTERNAL_H
#define __ZPERF_INTERNAL_H

#include <limits.h>
#include <misc/byteorder.h>

/* Constants */
#define PACKET_SIZE_MAX      1024

/* Macro */
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

/* Types */
typedef struct zperf_udp_datagram {
	int32_t id;
	uint32_t tv_sec;
	uint32_t tv_usec;
} zperf_udp_datagram;

typedef struct zperf_server_hdr {
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
} zperf_server_hdr;

/* Inline functions */
static inline uint32_t time_delta(uint32_t ts, uint32_t t)
{
	return (t >= ts) ? (t - ts) : (ULONG_MAX - ts + t);
}

/* byte order */
#define z_htonl(val) sys_cpu_to_be32(val)
#define z_ntohl(val) sys_be32_to_cpu(val)

/* internal functions */
extern void zperf_udp_upload(struct net_context *net_context,
		unsigned int duration_in_ms, unsigned int packet_size,
		unsigned int rate_in_kbps, zperf_results *results);
extern void zperf_upload_fin(struct net_context *net_context,
		uint32_t nb_packets, uint32_t end_time, uint32_t packet_size,
		zperf_results *results);
extern void zperf_upload_decode_stat(struct net_buf *net_stat,
		zperf_results *results);
extern void zperf_receiver_init(int port);
#ifdef CONFIG_NETWORKING_WITH_TCP
extern void zperf_tcp_receiver_init(int port);
extern void zperf_tcp_upload(struct net_context *net_context,
		unsigned int duration_in_ms, unsigned int packet_size,
		zperf_results *results);
#endif
extern void connect_ap(char *ssid);
#endif /* __ZPERF_INTERNAL_H */
