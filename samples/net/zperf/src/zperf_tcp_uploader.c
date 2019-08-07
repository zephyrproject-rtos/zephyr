/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_DECLARE(net_zperf_sample, LOG_LEVEL_DBG);

#include <zephyr.h>

#include <errno.h>
#include <sys/printk.h>

#include <net/net_pkt.h>
#include <net/net_ip.h>
#include <net/net_core.h>

#include "zperf.h"
#include "zperf_internal.h"

static char sample_packet[PACKET_SIZE_MAX];

void zperf_tcp_upload(const struct shell *shell,
		      struct net_context *ctx,
		      unsigned int duration_in_ms,
		      unsigned int packet_size,
		      struct zperf_results *results)
{
	u32_t duration = MSEC_TO_HW_CYCLES(duration_in_ms);
	u32_t nb_packets = 0U, nb_errors = 0U;
	u32_t start_time, last_print_time, end_time;
	u8_t time_elapsed = 0U, finished = 0U;

	if (packet_size > PACKET_SIZE_MAX) {
		shell_fprintf(shell, SHELL_WARNING,
			      "Packet size too large! max size: %u\n",
			      PACKET_SIZE_MAX);
		packet_size = PACKET_SIZE_MAX;
	}

	/* Start the loop */
	start_time = k_cycle_get_32();
	last_print_time = start_time;

	shell_fprintf(shell, SHELL_NORMAL,
		      "New session started\n");

	(void)memset(sample_packet, 'z', sizeof(sample_packet));

	/* Set the "flags" field in start of the packet to be 0.
	 * As the protocol is not properly described anywhere, it is
	 * not certain if this is a proper thing to do.
	 */
	(void)memset(sample_packet, 0, sizeof(uint32_t));

	do {
		int ret = 0;
		u32_t loop_time;

		/* Timestamps */
		loop_time = k_cycle_get_32();

		/* Send the packet */
		ret = net_context_send(ctx, sample_packet,
				       packet_size, NULL,
				       K_NO_WAIT, NULL);
		if (ret < 0) {
			shell_fprintf(shell, SHELL_WARNING,
				      "Failed to send the packet (%d)\n",
				      ret);
			nb_errors++;
			break;
		} else {
			nb_packets++;

			if (time_elapsed) {
				finished = 1U;
			}
		}

		if (!time_elapsed && time_delta(start_time,
						loop_time) > duration) {
			time_elapsed = 1U;
		}

#if defined(CONFIG_ARCH_POSIX)
		k_busy_wait(K_MSEC(100) * USEC_PER_MSEC);
#else
		k_yield();
#endif
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
