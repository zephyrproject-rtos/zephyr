/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <errno.h>
#include <misc/printk.h>

#include <net/nbuf.h>
#include <net/net_ip.h>
#include <net/net_core.h>

#include "zperf.h"
#include "zperf_internal.h"

#define TAG CMD_STR_TCP_UPLOAD" "

static char sample_packet[PACKET_SIZE_MAX];

void zperf_tcp_upload(struct net_context *ctx,
		      unsigned int duration_in_ms,
		      unsigned int packet_size,
		      struct zperf_results *results)
{
	uint32_t duration = MSEC_TO_HW_CYCLES(duration_in_ms);
	uint32_t nb_packets = 0, nb_errors = 0;
	uint32_t start_time, last_print_time, last_loop_time, end_time;
	uint8_t time_elapsed = 0, finished = 0;

	if (packet_size > PACKET_SIZE_MAX) {
		printk(TAG "WARNING! packet size too large! max size: %u\n",
		       PACKET_SIZE_MAX);
		packet_size = PACKET_SIZE_MAX;
	}

	/* Start the loop */
	start_time = k_cycle_get_32();
	last_print_time = start_time;
	last_loop_time = start_time;
	printk(TAG "New session started\n");

	memset(sample_packet, 'z', sizeof(sample_packet));

	do {
		int ret = 0;
		struct net_buf *buf, *frag;
		uint32_t loop_time;
		bool st;

		/* Timestamps */
		loop_time = k_cycle_get_32();
		last_loop_time = loop_time;

		buf = net_nbuf_get_tx(ctx, K_FOREVER);
		if (!buf) {
			printk(TAG "ERROR! Failed to retrieve a buffer\n");
			break;
		}

		frag = net_nbuf_get_data(ctx, K_FOREVER);
		if (!frag) {
			net_nbuf_unref(buf);
			printk(TAG "ERROR! Failed to retrieve a fragment\n");
			break;
		}

		net_buf_frag_add(buf, frag);

		/* Fill in the TCP payload */
		st = net_nbuf_append(buf, sizeof(sample_packet),
				     sample_packet, K_FOREVER);
		if (!st) {
			printk(TAG "ERROR! Failed to fill packet\n");

			net_nbuf_unref(buf);
			nb_errors++;
			break;
		}

		/* Send the packet */
		ret = net_context_send(buf, NULL, K_NO_WAIT, NULL, NULL);
		if (ret < 0) {
			printk(TAG "ERROR! Failed to send the buffer (%d)\n",
			       ret);

			net_nbuf_unref(buf);
			nb_errors++;
			break;
		} else {
			nb_packets++;

			if (time_elapsed) {
				finished = 1;
			}
		}

		if (!time_elapsed && time_delta(start_time,
						last_loop_time) > duration) {
			time_elapsed = 1;
		}

		k_yield();
	} while (!finished);

	end_time = k_cycle_get_32();

	/* Add result coming from the client */
	results->nb_packets_sent = nb_packets;
	results->client_time_in_us =
		HW_CYCLES_TO_USEC(time_delta(start_time, end_time));
	results->packet_size = packet_size;
	results->nb_packets_errors = nb_errors;

	net_context_put(ctx);
}
