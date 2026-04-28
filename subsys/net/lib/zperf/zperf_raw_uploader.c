/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_zperf, CONFIG_NET_ZPERF_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/zperf.h>

#include "zperf_internal.h"

#if defined(CONFIG_NET_ZPERF_RAW_TX)

/* Buffer for raw packet: user-provided header + payload */
static uint8_t raw_packet_buffer[PACKET_SIZE_MAX];

struct zperf_raw_async_upload_context {
	struct k_work work;
	struct zperf_raw_upload_params param;
	uint8_t hdr_storage[CONFIG_NET_ZPERF_RAW_TX_MAX_HDR_SIZE];
	zperf_callback callback;
	void *user_data;
};

static struct zperf_raw_async_upload_context raw_async_upload_ctx;

/**
 * @brief Internal raw packet TX upload implementation
 *
 * Buffer structure for raw TX:
 * [User-provided header] + [Payload 'z']
 *
 * The user provides everything (vendor metadata + frame header) as a single
 * blob. Zperf appends 'z' payload bytes to reach the desired packet_size.
 * This is generic and works with any frame format (802.11, Ethernet, etc.).
 *
 * @param param Upload parameters
 * @param results Pointer to store results
 *
 * @return 0 on success, negative errno on failure
 */
static int raw_upload(const struct zperf_raw_upload_params *param,
		      struct zperf_results *results)
{
	uint32_t duration_in_ms = param->duration_ms;
	uint32_t packet_size = param->packet_size;
	uint32_t rate_in_kbps = param->rate_kbps;
	uint32_t packet_duration_us;
	uint32_t packet_duration;
	uint32_t delay;
	uint32_t nb_packets = 0U;
	int64_t start_time, end_time;
	int64_t last_loop_time;
	int raw_sock;
	struct net_sockaddr_ll raw_addr;
	ssize_t sent_bytes;
	int ret;

	if (packet_size > PACKET_SIZE_MAX) {
		NET_WARN("Packet size too large! max size: %u", PACKET_SIZE_MAX);
		packet_size = PACKET_SIZE_MAX;
	}

	if (packet_size < param->hdr_len) {
		NET_ERR("Packet size (%u) must be >= header length (%u)",
			packet_size, param->hdr_len);
		return -EINVAL;
	}

	/* Rate limiting based on total packet size */
	packet_duration_us = zperf_packet_duration(packet_size, rate_in_kbps);
	packet_duration = k_us_to_ticks_ceil32(packet_duration_us);
	delay = packet_duration;

	/* Create raw packet socket (proto=0 for TX only) */
	raw_sock = zsock_socket(NET_AF_PACKET, NET_SOCK_RAW, 0);
	if (raw_sock < 0) {
		NET_ERR("Cannot create raw socket (%d)", errno);
		return -errno;
	}

	/* Set up net_sockaddr_ll structure */
	memset(&raw_addr, 0, sizeof(raw_addr));
	raw_addr.sll_family = NET_AF_PACKET;
	raw_addr.sll_ifindex = param->if_index;

	/* Bind to the interface */
	ret = zsock_bind(raw_sock, (struct net_sockaddr *)&raw_addr, sizeof(raw_addr));
	if (ret < 0) {
		NET_ERR("Failed to bind raw socket (%d)", errno);
		zsock_close(raw_sock);
		return -errno;
	}

	/*
	 * Build buffer: [User-provided header] + [Payload 'z']
	 * User provides everything (vendor metadata + frame header).
	 * We just append 'z' payload to reach packet_size.
	 */
	memset(raw_packet_buffer, 'z', packet_size);

	/* Copy user-provided header (vendor metadata + frame header) */
	if (param->hdr && param->hdr_len > 0) {
		memcpy(raw_packet_buffer, param->hdr, param->hdr_len);
	}

	/* Rest is already filled with 'z' from memset above */

	/* Start the transmission loop */
	start_time = k_uptime_ticks();
	last_loop_time = start_time;
	end_time = start_time + k_ms_to_ticks_ceil64(duration_in_ms);

	do {
		int64_t loop_time;
		int32_t adjust;

		/* Timestamp */
		loop_time = k_uptime_ticks();

		/* Algorithm to maintain a given baud rate */
		if (last_loop_time != loop_time) {
			adjust = packet_duration;
			adjust -= (int32_t)(loop_time - last_loop_time);
		} else {
			adjust = 0;
		}

		if ((adjust >= 0) || (-adjust < (int32_t)delay)) {
			delay += adjust;
		} else {
			delay = 0U;
		}

		last_loop_time = loop_time;

		/* Send the raw packet */
		sent_bytes = zsock_sendto(raw_sock, raw_packet_buffer, packet_size, 0,
					  (struct net_sockaddr *)&raw_addr, sizeof(raw_addr));

		if (sent_bytes < 0) {
			NET_DBG("Failed to send raw packet (%d)", errno);
			results->nb_packets_errors++;
		} else {
			nb_packets++;
		}

		/* Wait to maintain rate */
#if defined(CONFIG_ARCH_POSIX)
		k_busy_wait(USEC_PER_MSEC);
#else
		if (delay > 0) {
			k_sleep(K_TICKS(delay));
		}
#endif
	} while (last_loop_time < end_time);

	end_time = k_uptime_ticks();

	zsock_close(raw_sock);

	/* Fill in results */
	results->nb_packets_sent = nb_packets;
	results->client_time_in_us = k_ticks_to_us_ceil64(end_time - start_time);
	results->packet_size = packet_size;
	results->nb_packets_rcvd = 0;        /* TX only, no RX stats */
	results->nb_packets_lost = 0;
	results->nb_packets_outorder = 0;
	results->total_len = (uint64_t)nb_packets * packet_size;
	results->time_in_us = results->client_time_in_us;
	results->jitter_in_us = 0;
	results->is_multicast = false;

	return 0;
}

int zperf_raw_upload(const struct zperf_raw_upload_params *param,
		     struct zperf_results *result)
{
	if (param == NULL || result == NULL) {
		return -EINVAL;
	}

	if (param->if_index <= 0) {
		NET_ERR("Invalid interface index");
		return -EINVAL;
	}

	if (param->hdr_len > CONFIG_NET_ZPERF_RAW_TX_MAX_HDR_SIZE) {
		NET_ERR("Header length exceeds maximum (%d > %d)",
			param->hdr_len, CONFIG_NET_ZPERF_RAW_TX_MAX_HDR_SIZE);
		return -EINVAL;
	}

	memset(result, 0, sizeof(*result));

	return raw_upload(param, result);
}

static void raw_upload_async_work(struct k_work *work)
{
	struct zperf_raw_async_upload_context *upload_ctx =
		CONTAINER_OF(work, struct zperf_raw_async_upload_context, work);
	struct zperf_results result = { 0 };
	int ret;

	upload_ctx->callback(ZPERF_SESSION_STARTED, NULL, upload_ctx->user_data);

	ret = raw_upload(&upload_ctx->param, &result);
	if (ret < 0) {
		upload_ctx->callback(ZPERF_SESSION_ERROR, NULL, upload_ctx->user_data);
	} else {
		upload_ctx->callback(ZPERF_SESSION_FINISHED, &result, upload_ctx->user_data);
	}
}

int zperf_raw_upload_async(const struct zperf_raw_upload_params *param,
			   zperf_callback callback, void *user_data)
{
	if (param == NULL || callback == NULL) {
		return -EINVAL;
	}

	if (param->if_index <= 0) {
		NET_ERR("Invalid interface index");
		return -EINVAL;
	}

	if (param->hdr_len > CONFIG_NET_ZPERF_RAW_TX_MAX_HDR_SIZE) {
		NET_ERR("Header length exceeds maximum (%d > %d)",
			param->hdr_len, CONFIG_NET_ZPERF_RAW_TX_MAX_HDR_SIZE);
		return -EINVAL;
	}

	if (k_work_is_pending(&raw_async_upload_ctx.work)) {
		return -EBUSY;
	}

	memcpy(&raw_async_upload_ctx.param, param, sizeof(*param));

	/* Store metadata in local buffer since caller's buffer may go out of scope */
	if (param->hdr && param->hdr_len > 0) {
		memcpy(raw_async_upload_ctx.hdr_storage, param->hdr, param->hdr_len);
		raw_async_upload_ctx.param.hdr = raw_async_upload_ctx.hdr_storage;
	}

	raw_async_upload_ctx.callback = callback;
	raw_async_upload_ctx.user_data = user_data;

	zperf_async_work_submit(SESSION_RAW, -1, &raw_async_upload_ctx.work);

	return 0;
}

void zperf_raw_uploader_init(void)
{
	k_work_init(&raw_async_upload_ctx.work, raw_upload_async_work);
}

#endif /* CONFIG_NET_ZPERF_RAW_TX */
