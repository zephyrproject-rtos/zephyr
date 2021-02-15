/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_DECLARE(net_zperf_sample, LOG_LEVEL_DBG);

#include <zephyr.h>

#include <sys/printk.h>

#include <net/net_core.h>
#include <net/net_ip.h>
#include <net/net_pkt.h>

#include "zperf.h"
#include "zperf_internal.h"

static uint8_t sample_packet[sizeof(struct zperf_udp_datagram) +
			  sizeof(struct zperf_client_hdr_v1) +
			  PACKET_SIZE_MAX];

static inline void zperf_upload_decode_stat(const struct shell *shell,
					    struct net_pkt *pkt,
					    struct zperf_results *results)
{
	NET_PKT_DATA_ACCESS_DEFINE(zperf_udp, struct zperf_udp_datagram);
	NET_PKT_DATA_ACCESS_DEFINE(zperf_stat, struct zperf_server_hdr);
	struct zperf_udp_datagram *hdr;
	struct zperf_server_hdr *stat;

	hdr = (struct zperf_udp_datagram *)
		net_pkt_get_data(pkt, &zperf_udp);
	if (!hdr) {
		shell_fprintf(shell, SHELL_WARNING,
			      "Network packet too short\n");
		return;
	}

	net_pkt_acknowledge_data(pkt, &zperf_udp);

	stat = (struct zperf_server_hdr *)
		net_pkt_get_data(pkt, &zperf_stat);
	if (!stat) {
		shell_fprintf(shell, SHELL_WARNING,
			      "Network packet too short\n");
		return;
	}

	results->nb_packets_rcvd = ntohl(UNALIGNED_GET(&stat->datagrams));
	results->nb_packets_lost = ntohl(UNALIGNED_GET(&stat->error_cnt));
	results->nb_packets_outorder =
		ntohl(UNALIGNED_GET(&stat->outorder_cnt));
	results->nb_bytes_sent = ntohl(UNALIGNED_GET(&stat->total_len2));
	results->time_in_us = ntohl(UNALIGNED_GET(&stat->stop_usec)) +
		ntohl(UNALIGNED_GET(&stat->stop_sec)) * USEC_PER_SEC;
	results->jitter_in_us = ntohl(UNALIGNED_GET(&stat->jitter2)) +
		ntohl(UNALIGNED_GET(&stat->jitter1)) * USEC_PER_SEC;
}

static void stat_received(struct net_context *context,
			  struct net_pkt *pkt,
			  union net_ip_header *ip_hdr,
			  union net_proto_header *proto_hdr,
			  int status,
			  void *user_data)
{
	struct net_pkt **stat = user_data;

	*stat = pkt;
}

static inline void zperf_upload_fin(const struct shell *shell,
				    struct net_context *context,
				    uint32_t nb_packets,
				    uint64_t end_time,
				    uint32_t packet_size,
				    struct zperf_results *results)
{
	struct net_pkt *stat = NULL;
	struct zperf_udp_datagram *datagram;
	struct zperf_client_hdr_v1 *hdr;
	uint32_t secs = k_ticks_to_ms_ceil32(end_time) / 1000U;
	uint32_t usecs = k_ticks_to_us_ceil32(end_time) - secs * USEC_PER_SEC;
	int loop = 2;
	int ret;

	while (!stat && loop-- > 0) {
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
		ret = net_context_send(context, sample_packet, packet_size,
				       NULL, K_NO_WAIT, NULL);
		if (ret < 0) {
			shell_fprintf(shell, SHELL_WARNING,
				      "Failed to send the packet (%d)\n",
				      ret);
			continue;
		}

		/* Receive statistics */
		stat = NULL;

		ret = net_context_recv(context, stat_received,
				       K_SECONDS(2), &stat);
		if (ret == -ETIMEDOUT) {
			break;
		}
	}

	/* Decode statistics */
	if (stat) {
		zperf_upload_decode_stat(shell, stat, results);

		net_pkt_unref(stat);
	}

	/* Drain RX */
	while (stat) {
		stat = NULL;

		ret = net_context_recv(context, stat_received,
				       K_NO_WAIT, &stat);
		if (ret == -ETIMEDOUT) {
			break;
		}

		if (stat) {
			shell_fprintf(shell, SHELL_WARNING,
				      "Drain one spurious stat packet!\n");

			net_pkt_unref(stat);
		}
	}
}

void zperf_udp_upload(const struct shell *shell,
		      struct net_context *context,
		      int port,
		      unsigned int duration_in_ms,
		      unsigned int packet_size,
		      unsigned int rate_in_kbps,
		      struct zperf_results *results)
{
	uint32_t packet_duration = ((uint32_t)packet_size * 8U * USEC_PER_SEC) /
				   (rate_in_kbps * 1024U);
	uint64_t duration = z_timeout_end_calc(K_MSEC(duration_in_ms));
	int64_t print_interval = z_timeout_end_calc(K_SECONDS(1));
	uint64_t delay = packet_duration;
	uint32_t nb_packets = 0U;
	int64_t start_time, end_time;
	int64_t last_print_time, last_loop_time;
	int64_t remaining, print_info;

	if (packet_size > PACKET_SIZE_MAX) {
		shell_fprintf(shell, SHELL_WARNING,
			      "Packet size too large! max size: %u\n",
			      PACKET_SIZE_MAX);
		packet_size = PACKET_SIZE_MAX;
	} else if (packet_size < sizeof(struct zperf_udp_datagram)) {
		shell_fprintf(shell, SHELL_WARNING,
			      "Packet size set to the min size: %zu\n",
			      sizeof(struct zperf_udp_datagram));
		packet_size = sizeof(struct zperf_udp_datagram);
	}

	if (packet_duration > 1000U) {
		shell_fprintf(shell, SHELL_NORMAL,
			      "Packet duration %u ms\n",
			      (unsigned int)(packet_duration / 1000U));
	} else {
		shell_fprintf(shell, SHELL_NORMAL,
			      "Packet duration %u us\n",
			      (unsigned int)packet_duration);
	}

	/* Start the loop */
	start_time = k_uptime_ticks();
	last_print_time = start_time;
	last_loop_time = start_time;

	(void)memset(sample_packet, 'z', sizeof(sample_packet));

	do {
		struct zperf_udp_datagram *datagram;
		struct zperf_client_hdr_v1 *hdr;
		uint32_t secs, usecs;
		int64_t loop_time;
		int32_t adjust;
		int ret;

		/* Timestamp */
		loop_time = k_uptime_ticks();

		/* Algorithm to maintain a given baud rate */
		if (last_loop_time != loop_time) {
			adjust = (int32_t)(packet_duration -
				   k_ticks_to_us_ceil32(loop_time -
							last_loop_time));
		} else {
			/* It's the first iteration so no need for adjustment
			 */
			adjust = 0;
		}

		if (adjust >= 0) {
			delay += adjust;
		} else if ((uint64_t)-adjust < delay) {
			delay -= (uint64_t)-adjust;
		} else {
			delay = 0U; /* delay should never be negative */
		}

		last_loop_time = loop_time;

		secs = k_ticks_to_ms_ceil32(loop_time) / 1000U;
		usecs = k_ticks_to_us_ceil32(loop_time) - secs * USEC_PER_SEC;

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
		ret = net_context_send(context, sample_packet, packet_size,
				       NULL, K_NO_WAIT, NULL);
		if (ret < 0) {
			shell_fprintf(shell, SHELL_WARNING,
				      "Failed to send the packet (%d)\n",
				      ret);
			break;
		} else {
			nb_packets++;
		}

		/* Print log every seconds */
		print_info = print_interval - k_uptime_ticks();
		if (print_info <= 0) {
			shell_fprintf(shell, SHELL_WARNING,
				    "nb_packets=%u\tdelay=%u\tadjust=%d\n",
				      nb_packets, (unsigned int)delay,
				      (int)adjust);
			print_interval = z_timeout_end_calc(K_SECONDS(1));
		}

		remaining = duration - k_uptime_ticks();

		/* Wait */
#if defined(CONFIG_ARCH_POSIX)
		k_busy_wait(USEC_PER_MSEC);
#else
		if (delay != 0) {
			if (k_us_to_ticks_floor64(delay) > remaining) {
				delay = k_ticks_to_us_ceil64(remaining);
			}

			k_sleep(K_USEC(delay));
		}
#endif
	} while (remaining > 0);

	end_time = k_uptime_ticks();

	zperf_upload_fin(shell, context, nb_packets, end_time, packet_size,
			 results);

	/* Add result coming from the client */
	results->nb_packets_sent = nb_packets;
	results->client_time_in_us =
				k_ticks_to_us_ceil32(end_time - start_time);
	results->packet_size = packet_size;
}
