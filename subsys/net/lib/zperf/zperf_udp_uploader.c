/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_zperf, CONFIG_NET_ZPERF_LOG_LEVEL);

#include <zephyr/kernel.h>

#include <zephyr/net/socket.h>
#include <zephyr/net/zperf.h>

#include "zperf_internal.h"

static uint8_t sample_packet[sizeof(struct zperf_udp_datagram) +
			     sizeof(struct zperf_client_hdr_v1) +
			     PACKET_SIZE_MAX];

static struct zperf_async_upload_context udp_async_upload_ctx;

static inline void zperf_upload_decode_stat(const uint8_t *data,
					    size_t datalen,
					    struct zperf_results *results)
{
	struct zperf_server_hdr *stat;

	if (datalen < sizeof(struct zperf_udp_datagram) +
		      sizeof(struct zperf_server_hdr)) {
		NET_WARN("Network packet too short");
	}

	stat = (struct zperf_server_hdr *)
			(data + sizeof(struct zperf_udp_datagram));

	results->nb_packets_rcvd = ntohl(UNALIGNED_GET(&stat->datagrams));
	results->nb_packets_lost = ntohl(UNALIGNED_GET(&stat->error_cnt));
	results->nb_packets_outorder =
		ntohl(UNALIGNED_GET(&stat->outorder_cnt));
	results->total_len = (((uint64_t)ntohl(UNALIGNED_GET(&stat->total_len1))) << 32) +
		ntohl(UNALIGNED_GET(&stat->total_len2));
	results->time_in_us = ntohl(UNALIGNED_GET(&stat->stop_usec)) +
		ntohl(UNALIGNED_GET(&stat->stop_sec)) * USEC_PER_SEC;
	results->jitter_in_us = ntohl(UNALIGNED_GET(&stat->jitter2)) +
		ntohl(UNALIGNED_GET(&stat->jitter1)) * USEC_PER_SEC;
}

static inline int zperf_upload_fin(int sock,
				   uint32_t nb_packets,
				   uint64_t end_time,
				   uint32_t packet_size,
				   struct zperf_results *results,
				   bool is_mcast_pkt)
{
	uint8_t stats[sizeof(struct zperf_udp_datagram) +
		      sizeof(struct zperf_server_hdr)] = { 0 };
	struct zperf_udp_datagram *datagram;
	struct zperf_client_hdr_v1 *hdr;
	uint32_t secs = k_ticks_to_ms_ceil32(end_time) / 1000U;
	uint32_t usecs = k_ticks_to_us_ceil32(end_time) - secs * USEC_PER_SEC;
	int loop = 2;
	int ret = 0;
	struct timeval rcvtimeo = {
		.tv_sec = 2,
		.tv_usec = 0,
	};

	while (ret <= 0 && loop-- > 0) {
		datagram = (struct zperf_udp_datagram *)sample_packet;

		/* Fill the packet header */
		datagram->id = htonl(-nb_packets);
		datagram->tv_sec = htonl(secs);
		datagram->tv_usec = htonl(usecs);

		hdr = (struct zperf_client_hdr_v1 *)(sample_packet +
						     sizeof(*datagram));

		/* According to iperf documentation (in include/Settings.hpp),
		 * if the flags == 0, then the other values are ignored.
		 * But even if the values in the header are ignored, try
		 * to set there some meaningful values.
		 */
		hdr->flags = 0;
		hdr->num_of_threads = htonl(1);
		hdr->port = 0;
		hdr->buffer_len = sizeof(sample_packet) -
			sizeof(*datagram) - sizeof(*hdr);
		hdr->bandwidth = 0;
		hdr->num_of_bytes = htonl(packet_size);

		/* Send the packet */
		ret = zsock_send(sock, sample_packet, packet_size, 0);
		if (ret < 0) {
			NET_ERR("Failed to send the packet (%d)", errno);
			continue;
		}

		/* Multicast only send the negative sequence number packet
		 * and doesn't wait for a server ack
		 */
		if (!is_mcast_pkt) {
			/* Receive statistics */
			ret = zsock_setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &rcvtimeo,
					       sizeof(rcvtimeo));
			if (ret < 0) {
				NET_ERR("setsockopt error (%d)", errno);
				continue;
			}

			ret = zsock_recv(sock, stats, sizeof(stats), 0);
			if (ret == -EAGAIN) {
				NET_WARN("Stats receive timeout");
			} else if (ret < 0) {
				NET_ERR("Failed to receive packet (%d)", errno);
			}
		}
	}

	/* Decode statistics */
	if (ret > 0) {
		zperf_upload_decode_stat(stats, ret, results);
	} else {
		return ret;
	}

	/* Drain RX */
	while (true) {
		ret = zsock_recv(sock, stats, sizeof(stats), ZSOCK_MSG_DONTWAIT);
		if (ret < 0) {
			break;
		}

		NET_WARN("Drain one spurious stat packet!");
	}

	return 0;
}

static int udp_upload(int sock, int port,
		      const struct zperf_upload_params *param,
		      struct zperf_results *results)
{
	uint32_t duration_in_ms = param->duration_ms;
	uint32_t packet_size = param->packet_size;
	uint32_t rate_in_kbps = param->rate_kbps;
	uint32_t packet_duration_us = zperf_packet_duration(packet_size, rate_in_kbps);
	uint32_t packet_duration = k_us_to_ticks_ceil32(packet_duration_us);
	uint32_t delay = packet_duration;
	uint32_t nb_packets = 0U;
	int64_t start_time, end_time;
	int64_t print_time, last_loop_time;
	uint32_t print_period;
	bool is_mcast_pkt = false;
	int ret;

	if (packet_size > PACKET_SIZE_MAX) {
		NET_WARN("Packet size too large! max size: %u",
			 PACKET_SIZE_MAX);
		packet_size = PACKET_SIZE_MAX;
	} else if (packet_size < sizeof(struct zperf_udp_datagram)) {
		NET_WARN("Packet size set to the min size: %zu",
			 sizeof(struct zperf_udp_datagram));
		packet_size = sizeof(struct zperf_udp_datagram);
	}

	/* Start the loop */
	start_time = k_uptime_ticks();
	last_loop_time = start_time;
	end_time = start_time + k_ms_to_ticks_ceil64(duration_in_ms);

	/* Print log every seconds */
	print_period = k_ms_to_ticks_ceil32(MSEC_PER_SEC);
	print_time = start_time + print_period;

	(void)memset(sample_packet, 'z', sizeof(sample_packet));

	do {
		struct zperf_udp_datagram *datagram;
		struct zperf_client_hdr_v1 *hdr;
		uint64_t usecs64;
		uint32_t secs, usecs;
		int64_t loop_time;
		int32_t adjust;

		/* Timestamp */
		loop_time = k_uptime_ticks();

		/* Algorithm to maintain a given baud rate */
		if (last_loop_time != loop_time) {
			adjust = packet_duration;
			adjust -= (int32_t)(loop_time - last_loop_time);
		} else {
			/* It's the first iteration so no need for adjustment
			 */
			adjust = 0;
		}

		if ((adjust >= 0) || (-adjust < delay)) {
			delay += adjust;
		} else {
			delay = 0U; /* delay should never be negative */
		}

		last_loop_time = loop_time;

		usecs64 = k_ticks_to_us_floor64(loop_time);
		secs = usecs64 / USEC_PER_SEC;
		usecs = usecs64 - (uint64_t)secs * USEC_PER_SEC;

		/* Fill the packet header */
		datagram = (struct zperf_udp_datagram *)sample_packet;

		datagram->id = htonl(nb_packets);
		datagram->tv_sec = htonl(secs);
		datagram->tv_usec = htonl(usecs);

		hdr = (struct zperf_client_hdr_v1 *)(sample_packet +
						     sizeof(*datagram));
		hdr->flags = 0;
		hdr->num_of_threads = htonl(1);
		hdr->port = htonl(port);
		hdr->buffer_len = sizeof(sample_packet) -
			sizeof(*datagram) - sizeof(*hdr);
		hdr->bandwidth = htonl(rate_in_kbps);
		hdr->num_of_bytes = htonl(packet_size);

		/* Send the packet */
		ret = zsock_send(sock, sample_packet, packet_size, 0);
		if (ret < 0) {
			NET_ERR("Failed to send the packet (%d)", errno);
			return -errno;
		} else {
			nb_packets++;
		}

		if (IS_ENABLED(CONFIG_NET_ZPERF_LOG_LEVEL_DBG)) {
			if (print_time >= loop_time) {
				NET_DBG("nb_packets=%u\tdelay=%u\tadjust=%d",
					nb_packets, (unsigned int)delay,
					(int)adjust);
				print_time += print_period;
			}
		}

		/* Wait */
#if defined(CONFIG_ARCH_POSIX)
		k_busy_wait(USEC_PER_MSEC);
#else
		if (delay != 0) {
			k_sleep(K_TICKS(delay));
		}
#endif
	} while (last_loop_time < end_time);

	end_time = k_uptime_ticks();

	if (param->peer_addr.sa_family == AF_INET) {
		if (net_ipv4_is_addr_mcast(&net_sin(&param->peer_addr)->sin_addr)) {
			is_mcast_pkt = true;
		}
	} else if (param->peer_addr.sa_family == AF_INET6) {
		if (net_ipv6_is_addr_mcast(&net_sin6(&param->peer_addr)->sin6_addr)) {
			is_mcast_pkt = true;
		}
	} else {
		return -EINVAL;
	}
	ret = zperf_upload_fin(sock, nb_packets, end_time, packet_size,
			       results, is_mcast_pkt);
	if (ret < 0) {
		return ret;
	}

	/* Add result coming from the client */
	results->nb_packets_sent = nb_packets;
	results->client_time_in_us =
				k_ticks_to_us_ceil64(end_time - start_time);
	results->packet_size = packet_size;

	return 0;
}

int zperf_udp_upload(const struct zperf_upload_params *param,
		     struct zperf_results *result)
{
	int port = 0;
	int sock;
	int ret;
	struct ifreq req;

	if (param == NULL || result == NULL) {
		return -EINVAL;
	}

	if (param->peer_addr.sa_family == AF_INET) {
		port = ntohs(net_sin(&param->peer_addr)->sin_port);
	} else if (param->peer_addr.sa_family == AF_INET6) {
		port = ntohs(net_sin6(&param->peer_addr)->sin6_port);
	} else {
		NET_ERR("Invalid address family (%d)",
			param->peer_addr.sa_family);
		return -EINVAL;
	}

	sock = zperf_prepare_upload_sock(&param->peer_addr, param->options.tos,
					 param->options.priority, 0, IPPROTO_UDP);
	if (sock < 0) {
		return sock;
	}

	if (param->if_name[0]) {
		(void)memset(req.ifr_name, 0, sizeof(req.ifr_name));
		strncpy(req.ifr_name, param->if_name, IFNAMSIZ);
		req.ifr_name[IFNAMSIZ - 1] = 0;

		if (zsock_setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, &req,
				     sizeof(struct ifreq)) != 0) {
			NET_WARN("setsockopt SO_BINDTODEVICE error (%d)", -errno);
		}
	}

	ret = udp_upload(sock, port, param, result);

	zsock_close(sock);

	return ret;
}

static void udp_upload_async_work(struct k_work *work)
{
	struct zperf_async_upload_context *upload_ctx =
		&udp_async_upload_ctx;
	struct zperf_results result;
	int ret;

	upload_ctx->callback(ZPERF_SESSION_STARTED, NULL,
			     upload_ctx->user_data);

	ret = zperf_udp_upload(&upload_ctx->param, &result);
	if (ret < 0) {
		upload_ctx->callback(ZPERF_SESSION_ERROR, NULL,
				     upload_ctx->user_data);
	} else {
		upload_ctx->callback(ZPERF_SESSION_FINISHED, &result,
				     upload_ctx->user_data);
	}
}

int zperf_udp_upload_async(const struct zperf_upload_params *param,
			   zperf_callback callback, void *user_data)
{
	if (param == NULL || callback == NULL) {
		return -EINVAL;
	}

	if (k_work_is_pending(&udp_async_upload_ctx.work)) {
		return -EBUSY;
	}

	memcpy(&udp_async_upload_ctx.param, param, sizeof(*param));
	udp_async_upload_ctx.callback = callback;
	udp_async_upload_ctx.user_data = user_data;

	zperf_async_work_submit(&udp_async_upload_ctx.work);

	return 0;
}

void zperf_udp_uploader_init(void)
{
	k_work_init(&udp_async_upload_ctx.work, udp_upload_async_work);
}
