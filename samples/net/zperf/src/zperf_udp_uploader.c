/*
 * Copyright (c) 2015 Intel Corporation
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

#include <misc/printk.h>
#include <net/ip_buf.h>
#include <net/net_ip.h>
#include <net/net_core.h>
#include <net/net_socket.h>
#include <zephyr.h>

#include "zperf.h"
#include "zperf_internal.h"

#define TAG CMD_STR_UDP_UPLOAD" "

void zperf_udp_upload(struct net_context *net_context,
		unsigned int duration_in_ms,
		unsigned int packet_size, unsigned int rate_in_kbps,
		zperf_results *results)
{
	uint32_t print_interval = SEC_TO_HW_CYCLES(1);
	uint32_t duration = MSEC_TO_HW_CYCLES(duration_in_ms);
	uint32_t packet_duration = (uint32_t) (((uint64_t) packet_size
			* SEC_TO_HW_CYCLES(1) * 8) / (uint64_t) (rate_in_kbps * 1024));
	uint32_t nb_packets = 0;
	uint32_t start_time, last_print_time, last_loop_time, end_time;
	uint32_t delay = packet_duration;

	if (packet_size > PACKET_SIZE_MAX) {
		printk(TAG "WARNING! packet size too large! max size: %u\n",
				PACKET_SIZE_MAX);
		packet_size = PACKET_SIZE_MAX;
	} else if (packet_size < sizeof(zperf_udp_datagram)) {
		printk(TAG "WARNING! packet size set to the min size: %zu\n",
				sizeof(zperf_udp_datagram));
		packet_size = sizeof(zperf_udp_datagram);
	}

	/* Start the loop */
	start_time = sys_cycle_get_32();
	last_print_time = start_time;
	last_loop_time = start_time;

	do {
		int32_t adjust;
		uint32_t loop_time;
		zperf_udp_datagram *datagram;

		/* Timestamp */
		loop_time = sys_cycle_get_32();

		/* Algorithm to maintain a given baud rate */
		if (last_loop_time != loop_time) {
			adjust = packet_duration - time_delta(last_loop_time, loop_time);
		} else {
			adjust = 0; /* It's the first iteration so no need for adjustment */
		}
		if (adjust >= 0 || -adjust < delay) {
			delay += adjust;
		} else {
			delay = 0; /* delay should never be a negative value */
		}

		last_loop_time = loop_time;

		/* Get a new TX buffer */
		struct net_buf *buf = ip_buf_get_tx(net_context);

		if (!buf) {
			printk(TAG "ERROR! Failed to retrieve a buffer\n");
			continue;
		}

		/* Fill the packet header */
		datagram = (zperf_udp_datagram *) net_buf_add(buf,
				sizeof(zperf_udp_datagram));

		datagram->id = z_htonl(nb_packets);
		datagram->tv_sec = z_htonl(HW_CYCLES_TO_SEC(loop_time));
		datagram->tv_usec = z_htonl(
				HW_CYCLES_TO_USEC(loop_time) % USEC_PER_SEC);

		/* Fill the remain part of the datagram */
		if (packet_size > sizeof(zperf_udp_datagram)) {
			int size = packet_size - sizeof(zperf_udp_datagram);
			uint8_t *ptr = net_buf_add(buf, size);

			memset(ptr, 'z', size);
		}

		/* Send the packet */
		if (net_send(buf) < 0) {
			printk(TAG "ERROR! Failed to send the buffer\n");
			ip_buf_unref(buf);
			continue;
		} else {
			nb_packets++;
		}

		/* Print log every seconds */
		if (time_delta(last_print_time, loop_time) > print_interval) {
			printk(TAG "nb_packets=%u\tdelay=%u\tadjust=%d\n", nb_packets,
					delay, adjust);
			last_print_time = loop_time;
		}

		/* Wait */
		while (time_delta(loop_time, sys_cycle_get_32()) < delay) {
			fiber_yield();
		}
	} while (time_delta(start_time, last_loop_time) < duration);

	end_time = sys_cycle_get_32();
	zperf_upload_fin(net_context, nb_packets, end_time, packet_size, results);

	/* Add result coming from the client */
	results->nb_packets_sent = nb_packets;
	results->client_time_in_us = HW_CYCLES_TO_USEC(
			time_delta(start_time, end_time));
	results->packet_size = packet_size;
}

void zperf_upload_fin(struct net_context *net_context, uint32_t nb_packets,
		uint32_t end_time, uint32_t packet_size, zperf_results *results)
{
	struct net_buf *net_stat = NULL;
	int loop = 2;

	while (net_stat == NULL && loop-- > 0) {
		zperf_udp_datagram *datagram;

		/* Get a new TX buffer */
		struct net_buf *buf = ip_buf_get_tx(net_context);

		if (!buf) {
			printk(TAG "ERROR! Failed to retrieve a buffer\n");
			continue;
		}

		/* Fill the packet header */
		datagram = (zperf_udp_datagram *) net_buf_add(buf,
				sizeof(zperf_udp_datagram));

		datagram->id = z_htonl(-nb_packets);
		datagram->tv_sec = z_htonl(HW_CYCLES_TO_SEC(end_time));
		datagram->tv_usec = z_htonl(HW_CYCLES_TO_USEC(end_time) % USEC_PER_SEC);

		/* Fill the remain part of the datagram */
		if (packet_size > sizeof(zperf_udp_datagram)) {
			int size = packet_size - sizeof(zperf_udp_datagram);
			uint8_t *ptr = net_buf_add(buf, size);

			memset(ptr, 'z', size);
		}

		/* Send the packet */
		if (net_send(buf) < 0) {
			printk(TAG "ERROR! Failed to send the buffer\n");
			ip_buf_unref(buf);
			continue;
		}

		/* Receive statistic */
		net_stat = net_receive(net_context, 2 * sys_clock_ticks_per_sec);
	}

	/* Decode statistic */
	if (net_stat != NULL) {
		zperf_upload_decode_stat(net_stat, results);

		/* Free the buffer */
		ip_buf_unref(net_stat);
	}

	/* Drain RX fifo */
	while (net_stat != NULL) {
		net_stat = net_receive(net_context, 2 * sys_clock_ticks_per_sec);
		ip_buf_unref(net_stat);
		if (net_stat != NULL)
			printk(TAG "Drain one spurious stat packet!\n");
	}
}

void zperf_upload_decode_stat(struct net_buf *net_stat,
		zperf_results *results)
{
	/* Decode stat */
	if (net_stat == NULL) {
		printk(TAG "ERROR! Failed to receive statistic\n");
	} else if (ip_buf_appdatalen(net_stat)
			< sizeof(zperf_server_hdr) + sizeof(zperf_udp_datagram)) {
		printk(TAG "ERROR! Statistic too small\n");
	} else {
		zperf_server_hdr *hdr;

		hdr = (zperf_server_hdr *) (ip_buf_appdata(net_stat)
				+ sizeof(zperf_udp_datagram));

		/* Fill the results struct */
		results->nb_packets_rcvd = z_ntohl(hdr->datagrams);
		results->nb_packets_lost = z_ntohl(hdr->error_cnt);
		results->nb_packets_outorder = z_ntohl(hdr->outorder_cnt);
		results->nb_bytes_sent = z_ntohl(hdr->total_len2);
		results->time_in_us = z_ntohl(
				hdr->stop_usec) + z_ntohl(hdr->stop_sec) * USEC_PER_SEC;
		results->jitter_in_us = z_ntohl(
				hdr->jitter2) + z_ntohl(hdr->jitter1) * USEC_PER_SEC;
	}
}

