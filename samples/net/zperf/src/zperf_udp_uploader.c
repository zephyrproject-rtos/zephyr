/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <misc/printk.h>

#include <net/net_core.h>
#include <net/net_ip.h>
#include <net/nbuf.h>

#include "zperf.h"
#include "zperf_internal.h"

#define TAG CMD_STR_UDP_UPLOAD" "

static char sample_packet[PACKET_SIZE_MAX];

static inline void zperf_upload_decode_stat(struct net_buf *buf,
					    struct zperf_results *results)
{
	struct net_buf *frag = buf->frags;
	struct zperf_server_hdr hdr;
	uint16_t offset;
	uint16_t pos;

	offset = net_nbuf_udp_data(buf) - net_nbuf_ip_data(buf);
	offset += sizeof(struct net_udp_hdr) +
		sizeof(struct zperf_udp_datagram);

	/* Decode stat */
	if (!buf) {
		printk(TAG "ERROR! Failed to receive statistic\n");
		return;
	} else if (net_nbuf_appdatalen(buf) <
		   (sizeof(struct zperf_server_hdr) +
		    sizeof(struct zperf_udp_datagram))) {
		printk(TAG "ERROR! Statistics too small\n");
		return;
	}

	frag = net_nbuf_read_be32(frag, offset, &pos, (uint32_t *)&hdr.flags);
	frag = net_nbuf_read_be32(frag, pos, &pos, (uint32_t *)&hdr.total_len1);
	frag = net_nbuf_read_be32(frag, pos, &pos, (uint32_t *)&hdr.total_len2);
	frag = net_nbuf_read_be32(frag, pos, &pos, (uint32_t *)&hdr.stop_sec);
	frag = net_nbuf_read_be32(frag, pos, &pos, (uint32_t *)&hdr.stop_usec);
	frag = net_nbuf_read_be32(frag, pos, &pos, (uint32_t *)&hdr.error_cnt);
	frag = net_nbuf_read_be32(frag, pos, &pos,
				  (uint32_t *)&hdr.outorder_cnt);
	frag = net_nbuf_read_be32(frag, pos, &pos, (uint32_t *)&hdr.datagrams);
	frag = net_nbuf_read_be32(frag, pos, &pos, (uint32_t *)&hdr.jitter1);
	frag = net_nbuf_read_be32(frag, pos, &pos, (uint32_t *)&hdr.jitter2);

	results->nb_packets_rcvd = hdr.datagrams;
	results->nb_packets_lost = hdr.error_cnt;
	results->nb_packets_outorder = hdr.outorder_cnt;
	results->nb_bytes_sent = hdr.total_len2;
	results->time_in_us = hdr.stop_usec + hdr.stop_sec * USEC_PER_SEC;
	results->jitter_in_us = hdr.jitter2 + hdr.jitter1 * USEC_PER_SEC;
}

static void stat_received(struct net_context *context,
			  struct net_buf *buf,
			  int status,
			  void *user_data)
{
	struct net_buf **stat = user_data;

	*stat = buf;
}

static inline void zperf_upload_fin(struct net_context *context,
				    uint32_t nb_packets,
				    uint32_t end_time,
				    uint32_t packet_size,
				    struct zperf_results *results)
{
	struct net_buf *stat = NULL;
	struct zperf_udp_datagram datagram;
	int loop = 2;
	int ret;

	while (!stat && loop-- > 0) {
		struct net_buf *buf, *frag;
		bool status;

		buf = net_nbuf_get_tx(context, K_FOREVER);
		if (!buf) {
			printk(TAG "ERROR! Failed to retrieve a buffer\n");
			continue;
		}

		frag = net_nbuf_get_data(context, K_FOREVER);
		if (!frag) {
			printk(TAG "ERROR! Failed to retrieve a fragment\n");
			continue;
		}

		net_buf_frag_add(buf, frag);

		/* Fill the packet header */
		datagram.id = htonl(-nb_packets);
		datagram.tv_sec = htonl(HW_CYCLES_TO_SEC(end_time));
		datagram.tv_usec = htonl(HW_CYCLES_TO_USEC(end_time) %
					    USEC_PER_SEC);

		status = net_nbuf_append(buf, sizeof(datagram),
					 (uint8_t *)&datagram, K_FOREVER);
		if (!status) {
			printk(TAG "ERROR! Cannot append datagram data\n");
			break;
		}

		/* Fill the remain part of the datagram */
		if (packet_size > sizeof(struct zperf_udp_datagram)) {
			int size = packet_size -
				sizeof(struct zperf_udp_datagram);
			uint16_t pos;

			frag = net_nbuf_write(buf, net_buf_frag_last(buf),
					     sizeof(struct zperf_udp_datagram),
					      &pos, size,
					      (uint8_t *)sample_packet,
					      K_FOREVER);
		}

		/* Send the packet */
		ret = net_context_send(buf, NULL, K_NO_WAIT, NULL, NULL);
		if (ret < 0) {
			printk(TAG "ERROR! Failed to send the buffer (%d)\n",
			       ret);
			net_nbuf_unref(buf);
			continue;
		}

		/* Receive statistics */
		stat = NULL;

		ret = net_context_recv(context, stat_received,
				       2 * MSEC_PER_SEC, &stat);
		if (ret == -ETIMEDOUT) {
			break;
		}
	}

	/* Decode statistics */
	if (stat) {
		zperf_upload_decode_stat(stat, results);

		net_nbuf_unref(stat);
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
			printk(TAG "Drain one spurious stat packet!\n");

			net_nbuf_unref(stat);
		}
	}
}

void zperf_udp_upload(struct net_context *context,
		      unsigned int duration_in_ms,
		      unsigned int packet_size,
		      unsigned int rate_in_kbps,
		      struct zperf_results *results)
{
	uint32_t packet_duration = (uint32_t)(((uint64_t) packet_size *
					       SEC_TO_HW_CYCLES(1) * 8) /
					      (uint64_t)(rate_in_kbps * 1024));
	uint32_t duration = MSEC_TO_HW_CYCLES(duration_in_ms);
	uint32_t print_interval = SEC_TO_HW_CYCLES(1);
	uint32_t delay = packet_duration;
	uint32_t nb_packets = 0;
	uint32_t start_time, last_print_time, last_loop_time, end_time;

	if (packet_size > PACKET_SIZE_MAX) {
		printk(TAG "WARNING! packet size too large! max size: %u\n",
		       PACKET_SIZE_MAX);
		packet_size = PACKET_SIZE_MAX;
	} else if (packet_size < sizeof(struct zperf_udp_datagram)) {
		printk(TAG "WARNING! packet size set to the min size: %zu\n",
		       sizeof(struct zperf_udp_datagram));
		packet_size = sizeof(struct zperf_udp_datagram);
	}

	/* Start the loop */
	start_time = k_cycle_get_32();
	last_print_time = start_time;
	last_loop_time = start_time;

	memset(sample_packet, 'z', sizeof(sample_packet));

	do {
		struct zperf_udp_datagram datagram;
		struct net_buf *buf, *frag;
		uint32_t loop_time;
		int32_t adjust;
		bool status;
		int ret;

		/* Timestamp */
		loop_time = k_cycle_get_32();

		/* Algorithm to maintain a given baud rate */
		if (last_loop_time != loop_time) {
			adjust = packet_duration - time_delta(last_loop_time,
							      loop_time);
		} else {
			/* It's the first iteration so no need for adjustment
			 */
			adjust = 0;
		}

		if (adjust >= 0 || -adjust < delay) {
			delay += adjust;
		} else {
			delay = 0; /* delay should never be a negative value */
		}

		last_loop_time = loop_time;

		buf = net_nbuf_get_tx(context, K_FOREVER);
		if (!buf) {
			printk(TAG "ERROR! Failed to retrieve a buffer\n");
			continue;
		}

		frag = net_nbuf_get_data(context, K_FOREVER);
		if (!frag) {
			printk(TAG "ERROR! Failed to retrieve a frag\n");
			continue;
		}

		net_buf_frag_add(buf, frag);

		/* Fill the packet header */
		datagram.id = htonl(nb_packets);
		datagram.tv_sec = htonl(HW_CYCLES_TO_SEC(loop_time));
		datagram.tv_usec =
			htonl(HW_CYCLES_TO_USEC(loop_time) % USEC_PER_SEC);

		status = net_nbuf_append(buf, sizeof(datagram),
					 (uint8_t *)&datagram, K_FOREVER);
		if (!status) {
			printk(TAG "ERROR! Cannot append datagram data\n");
			break;
		}

		/* Fill the remain part of the datagram */
		if (packet_size > sizeof(struct zperf_udp_datagram)) {
			int size = packet_size -
				sizeof(struct zperf_udp_datagram);
			uint16_t pos;

			frag = net_nbuf_write(buf, net_buf_frag_last(buf),
					     sizeof(struct zperf_udp_datagram),
					      &pos, size, sample_packet,
					      K_FOREVER);
		}

		/* Send the packet */
		ret = net_context_send(buf, NULL, K_NO_WAIT, NULL, NULL);
		if (ret < 0) {
			printk(TAG "ERROR! Failed to send the buffer (%d)\n",
				ret);

			net_nbuf_unref(buf);
			break;
		} else {
			nb_packets++;
		}

		/* Print log every seconds */
		if (time_delta(last_print_time, loop_time) > print_interval) {
			printk(TAG "nb_packets=%u\tdelay=%u\tadjust=%d\n",
			       nb_packets, delay, adjust);
			last_print_time = loop_time;
		}

		/* Wait */
		while (time_delta(loop_time, k_cycle_get_32()) < delay) {
			k_yield();
		}

	} while (time_delta(start_time, last_loop_time) < duration);

	end_time = k_cycle_get_32();

	zperf_upload_fin(context, nb_packets, end_time, packet_size, results);

	/* Add result coming from the client */
	results->nb_packets_sent = nb_packets;
	results->client_time_in_us =
		HW_CYCLES_TO_USEC(time_delta(start_time, end_time));
	results->packet_size = packet_size;
}
