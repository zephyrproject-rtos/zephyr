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
#include "zperf_session.h"

static uint8_t sample_packet[sizeof(struct zperf_udp_datagram) +
			     sizeof(struct zperf_client_hdr_v1) +
			     PACKET_SIZE_MAX];

#if !defined(CONFIG_ZPERF_SESSION_PER_THREAD)
static struct zperf_async_upload_context udp_async_upload_ctx;
#endif /* CONFIG_ZPERF_SESSION_PER_THREAD */

static inline void zperf_upload_decode_stat(const uint8_t *data,
					    size_t datalen,
					    struct zperf_results *results)
{
	struct zperf_server_hdr *stat;
	uint32_t flags;

	if (datalen < sizeof(struct zperf_udp_datagram) +
		      sizeof(struct zperf_server_hdr)) {
		NET_WARN("Network packet too short");
	}

	stat = (struct zperf_server_hdr *)
			(data + sizeof(struct zperf_udp_datagram));
	flags = ntohl(UNALIGNED_GET(&stat->flags));
	if (!(flags & ZPERF_FLAGS_VERSION1)) {
		NET_WARN("Unexpected response flags");
	}

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
				   uint64_t end_time_us,
				   uint32_t packet_size,
				   struct zperf_results *results,
				   bool is_mcast_pkt)
{
	uint8_t stats[sizeof(struct zperf_udp_datagram) +
		      sizeof(struct zperf_server_hdr)] = { 0 };
	struct zperf_udp_datagram *datagram;
	struct zperf_client_hdr_v1 *hdr;
	uint32_t secs = end_time_us / USEC_PER_SEC;
	uint32_t usecs = end_time_us % USEC_PER_SEC;
	int loop = CONFIG_NET_ZPERF_UDP_REPORT_RETANSMISSION_COUNT;
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

		/* For multicast, do not wait for a server ack. Keep resending FIN
		 * for the configured number of attempts by forcing another loop
		 * iteration.
		 */
		if (is_mcast_pkt) {
			ret = 0;
			continue;
		} else {
			/* Receive statistics */
			ret = zsock_setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &rcvtimeo,
					       sizeof(rcvtimeo));
			if (ret < 0) {
				NET_ERR("setsockopt error (%d)", errno);
				continue;
			}

			ret = zsock_recv(sock, stats, sizeof(stats), 0);
			if (ret < 0 && errno == EAGAIN) {
				NET_WARN("Stats receive timeout");
			} else if (ret < 0) {
				NET_ERR("Failed to receive packet (%d)", errno);
			}
		}
	}

	/* In multicast, we never expect a stats reply. Stop here. */
	if (is_mcast_pkt) {
		return 0;
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
	size_t header_size =
		sizeof(struct zperf_udp_datagram) + sizeof(struct zperf_client_hdr_v1);
	uint32_t duration_in_ms = param->duration_ms;
	uint32_t packet_size = param->packet_size;
	uint32_t rate_in_kbps = param->rate_kbps;
	uint32_t packet_duration_us = zperf_packet_duration(packet_size, rate_in_kbps);
	uint32_t packet_duration = k_us_to_ticks_ceil32(packet_duration_us);
	uint32_t delay = packet_duration;
	uint64_t data_offset = 0U;
	uint32_t nb_packets = 0U;
	uint64_t usecs64;
	int64_t start_time, end_time;
	int64_t print_time, last_loop_time;
	uint32_t print_period;
	bool is_mcast_pkt = false;
	int ret;

	if (packet_size > PACKET_SIZE_MAX) {
		NET_WARN("Packet size too large! max size: %u", PACKET_SIZE_MAX);
		packet_size = PACKET_SIZE_MAX;
	} else if (packet_size < sizeof(struct zperf_udp_datagram)) {
		NET_WARN("Packet size set to the min size: %zu", header_size);
		packet_size = header_size;
	}

	/* Start the loop */
	start_time = k_uptime_ticks();
	last_loop_time = start_time;
	end_time = start_time + k_ms_to_ticks_ceil64(duration_in_ms);

	/* Print log every seconds */
	print_period = k_ms_to_ticks_ceil32(MSEC_PER_SEC);
	print_time = start_time + print_period;

	/* Default data payload */
	(void)memset(sample_packet, 'z', sizeof(sample_packet));

	do {
		struct zperf_udp_datagram *datagram;
		struct zperf_client_hdr_v1 *hdr;
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

		usecs64 = param->unix_offset_us + k_ticks_to_us_floor64(loop_time - start_time);
		secs = usecs64 / USEC_PER_SEC;
		usecs = usecs64 % USEC_PER_SEC;

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

		/* Load custom data payload if requested */
		if (param->data_loader != NULL) {
			ret = param->data_loader(param->data_loader_ctx, data_offset,
				sample_packet + header_size, packet_size - header_size);
			if (ret < 0) {
				NET_ERR("Failed to load data for offset %llu", data_offset);
				return ret;
			}
		}
		data_offset += packet_size - header_size;

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
	usecs64 = param->unix_offset_us + k_ticks_to_us_floor64(end_time - start_time);

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
	ret = zperf_upload_fin(sock, nb_packets, usecs64, packet_size, results, is_mcast_pkt);
	if (ret < 0) {
		return ret;
	}

	/* Add result coming from the client */
	results->nb_packets_sent = nb_packets;
	results->client_time_in_us =
				k_ticks_to_us_ceil64(end_time - start_time);
	results->packet_size = packet_size;
	results->is_multicast = is_mcast_pkt;

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
#ifdef CONFIG_ZPERF_SESSION_PER_THREAD
	struct session *ses;
	struct zperf_async_upload_context *upload_ctx;
	struct zperf_results *result;

	ses = CONTAINER_OF(work, struct session, async_upload_ctx.work);
	upload_ctx = &ses->async_upload_ctx;

	if (ses->wait_for_start) {
		NET_INFO("[%d] %s waiting for start", ses->id, "UDP");

		/* Wait for the start event to be set */
		k_event_wait(ses->zperf->start_event, START_EVENT, true, K_FOREVER);

		NET_INFO("[%d] %s starting", ses->id, "UDP");
	}

	NET_DBG("[%d] thread %p priority %d name %s", ses->id, k_current_get(),
		k_thread_priority_get(k_current_get()),
		k_thread_name_get(k_current_get()));

	result = &ses->result;

	ses->in_progress = true;
#else
	struct zperf_async_upload_context *upload_ctx = &udp_async_upload_ctx;
	struct zperf_results result_storage = { 0 };
	struct zperf_results *result = &result_storage;
#endif /* CONFIG_ZPERF_SESSION_PER_THREAD */

	int ret;

	upload_ctx->callback(ZPERF_SESSION_STARTED, NULL,
			     upload_ctx->user_data);

	ret = zperf_udp_upload(&upload_ctx->param, result);
	if (ret < 0) {
		upload_ctx->callback(ZPERF_SESSION_ERROR, NULL,
				     upload_ctx->user_data);
	} else {
		upload_ctx->callback(ZPERF_SESSION_FINISHED, result,
				     upload_ctx->user_data);
	}
}

int zperf_udp_upload_async(const struct zperf_upload_params *param,
			   zperf_callback callback, void *user_data)
{
	if (param == NULL || callback == NULL) {
		return -EINVAL;
	}

#ifdef CONFIG_ZPERF_SESSION_PER_THREAD
	struct k_work_q *queue;
	struct zperf_work *zperf;
	struct session *ses;
	k_tid_t tid;

	ses = get_free_session(&param->peer_addr, SESSION_UDP);
	if (ses == NULL) {
		NET_ERR("Cannot get a session!");
		return -ENOENT;
	}

	if (k_work_is_pending(&ses->async_upload_ctx.work)) {
		NET_ERR("[%d] upload already in progress", ses->id);
		return -EBUSY;
	}

	memcpy(&ses->async_upload_ctx.param, param, sizeof(*param));

	ses->proto = SESSION_UDP;
	ses->async_upload_ctx.callback = callback;
	ses->async_upload_ctx.user_data = user_data;

	zperf = get_queue(SESSION_UDP, ses->id);

	queue = zperf->queue;
	if (queue == NULL) {
		NET_ERR("Cannot get a work queue!");
		return -ENOENT;
	}

	tid = k_work_queue_thread_get(queue);
	k_thread_priority_set(tid, ses->async_upload_ctx.param.options.thread_priority);

	k_work_init(&ses->async_upload_ctx.work, udp_upload_async_work);

	ses->start_time = k_uptime_ticks();
	ses->zperf = zperf;
	ses->wait_for_start = param->options.wait_for_start;

	zperf_async_work_submit(SESSION_UDP, ses->id, &ses->async_upload_ctx.work);

	NET_DBG("[%d] thread %p priority %d name %s", ses->id, k_current_get(),
		k_thread_priority_get(k_current_get()),
		k_thread_name_get(k_current_get()));

#else /* CONFIG_ZPERF_SESSION_PER_THREAD */

	if (k_work_is_pending(&udp_async_upload_ctx.work)) {
		return -EBUSY;
	}

	memcpy(&udp_async_upload_ctx.param, param, sizeof(*param));
	udp_async_upload_ctx.callback = callback;
	udp_async_upload_ctx.user_data = user_data;

	zperf_async_work_submit(SESSION_UDP, -1, &udp_async_upload_ctx.work);

#endif /* CONFIG_ZPERF_SESSION_PER_THREAD */

	return 0;
}

void zperf_udp_uploader_init(void)
{
#if !defined(CONFIG_ZPERF_SESSION_PER_THREAD)
	k_work_init(&udp_async_upload_ctx.work, udp_upload_async_work);
#endif /* !CONFIG_ZPERF_SESSION_PER_THREAD */
}
