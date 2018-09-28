/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <errno.h>
#include <misc/printk.h>

#include <net/net_pkt.h>
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
	u32_t duration = MSEC_TO_HW_CYCLES(duration_in_ms);
	u32_t nb_packets = 0, nb_errors = 0;
	u32_t start_time, last_print_time, last_loop_time, end_time;
	u8_t time_elapsed = 0, finished = 0;

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

	(void)memset(sample_packet, 'z', sizeof(sample_packet));

	do {
		int ret = 0;
		struct net_pkt *pkt;
		struct net_buf *frag;
		u32_t loop_time;

		/* Timestamps */
		loop_time = k_cycle_get_32();
		last_loop_time = loop_time;

		pkt = net_pkt_get_tx(ctx, K_FOREVER);
		if (!pkt) {
			printk(TAG "ERROR! Failed to retrieve a packet\n");
			break;
		}

		frag = net_pkt_get_data(ctx, K_FOREVER);
		if (!frag) {
			net_pkt_unref(pkt);
			printk(TAG "ERROR! Failed to retrieve a fragment\n");
			break;
		}

		net_pkt_frag_add(pkt, frag);

		/* Fill in the TCP payload */
		net_pkt_append(pkt, sizeof(sample_packet),
			       sample_packet, K_FOREVER);

		/* Send the packet */
		ret = net_context_send(pkt, NULL, K_NO_WAIT, NULL, NULL);
		if (ret < 0) {
			printk(TAG "ERROR! Failed to send the packet (%d)\n",
			       ret);

			net_pkt_unref(pkt);
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
