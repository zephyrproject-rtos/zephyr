/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file zperf.h
 *
 * @brief Zperf API
 * @defgroup zperf Zperf API
 * @ingroup networking
 * @{
 */

#ifndef ZEPHYR_INCLUDE_NET_ZPERF_H_
#define ZEPHYR_INCLUDE_NET_ZPERF_H_

#include <zephyr/net/net_ip.h>

#ifdef __cplusplus
extern "C" {
#endif

struct zperf_upload_params {
	struct sockaddr peer_addr;
	uint32_t duration_ms;
	uint32_t rate_kbps;
	uint16_t packet_size;
	struct {
		uint8_t tos;
	} options;
};

struct zperf_results {
	uint32_t nb_packets_sent;
	uint32_t nb_packets_rcvd;
	uint32_t nb_packets_lost;
	uint32_t nb_packets_outorder;
	uint32_t total_len;
	uint32_t time_in_us;
	uint32_t jitter_in_us;
	uint32_t client_time_in_us;
	uint32_t packet_size;
	uint32_t nb_packets_errors;
};

/**
 * @brief Synchronous UDP upload operation. The function blocks until the upload
 *        is complete.
 *
 * @param param Upload parameters.
 * @param result Session results.
 *
 * @return 0 if session completed successfully, a negative error code otherwise.
 */
int zperf_udp_upload(const struct zperf_upload_params *param,
		     struct zperf_results *result);

/**
 * @brief Synchronous TCP upload operation. The function blocks until the upload
 *        is complete.
 *
 * @param param Upload parameters.
 * @param result Session results.
 *
 * @return 0 if session completed successfully, a negative error code otherwise.
 */
int zperf_tcp_upload(const struct zperf_upload_params *param,
		     struct zperf_results *result);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_ZPERF_H_ */
