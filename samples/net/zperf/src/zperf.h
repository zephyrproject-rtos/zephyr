/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __ZPERF_H
#define __ZPERF_H

#define VERSION "1.1"

struct zperf_results {
	u32_t nb_packets_sent;
	u32_t nb_packets_rcvd;
	u32_t nb_packets_lost;
	u32_t nb_packets_outorder;
	u32_t nb_bytes_sent;
	u32_t time_in_us;
	u32_t jitter_in_us;
	u32_t client_time_in_us;
	u32_t packet_size;
	u32_t nb_packets_errors;
};

typedef void (*zperf_callback)(int status, struct zperf_results *);

#define IPV4_STR_LEN_MAX 15
#define IPV4_STR_LEN_MIN 7

#endif /* __ZPERF_H */
