/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_zperf_sample, LOG_LEVEL_DBG);

#include <zephyr/zephyr.h>

#include <errno.h>
#include <zephyr/sys/printk.h>

#include <zephyr/net/socket.h>

#include "zperf.h"
#include "zperf_internal.h"

static char sample_packet[PACKET_SIZE_MAX];

void zperf_tcp_upload(const struct shell *sh,
		      int sock,
		      unsigned int duration_in_ms,
		      unsigned int packet_size,
		      struct zperf_results *results)
{
	int64_t duration = sys_clock_timeout_end_calc(K_MSEC(duration_in_ms));
	int64_t start_time, last_print_time, end_time, remaining;
	uint32_t nb_packets = 0U, nb_errors = 0U;
	uint32_t alloc_errors = 0U;

	if (packet_size > PACKET_SIZE_MAX) {
		shell_fprintf(sh, SHELL_WARNING,
			      "Packet size too large! max size: %u\n",
			      PACKET_SIZE_MAX);
		packet_size = PACKET_SIZE_MAX;
	}

	/* Start the loop */
	start_time = k_uptime_ticks();
	last_print_time = start_time;

	shell_fprintf(sh, SHELL_NORMAL,
		      "New session started\n");

	(void)memset(sample_packet, 'z', sizeof(sample_packet));

	/* Set the "flags" field in start of the packet to be 0.
	 * As the protocol is not properly described anywhere, it is
	 * not certain if this is a proper thing to do.
	 */
	(void)memset(sample_packet, 0, sizeof(uint32_t));

	do {
		int ret = 0;

		/* Send the packet */
		ret = send(sock, sample_packet, packet_size, 0);
		if (ret < 0) {
			if (nb_errors == 0 && ret != -ENOMEM) {
				shell_fprintf(sh, SHELL_WARNING,
					      "Failed to send the packet (%d)\n",
					      errno);
			}

			nb_errors++;

			if (errno == -ENOMEM) {
				/* Ignore memory errors as we just run out of
				 * buffers which is kind of expected if the
				 * buffer count is not optimized for the test
				 * and device.
				 */
				alloc_errors++;
			} else {
				break;
			}
		} else {
			nb_packets++;
		}

#if defined(CONFIG_ARCH_POSIX)
		k_busy_wait(100 * USEC_PER_MSEC);
#else
		k_yield();
#endif

		remaining = duration - k_uptime_ticks();
	} while (remaining > 0);

	end_time = k_uptime_ticks();

	/* Add result coming from the client */
	results->nb_packets_sent = nb_packets;
	results->client_time_in_us =
				k_ticks_to_us_ceil32(end_time - start_time);
	results->packet_size = packet_size;
	results->nb_packets_errors = nb_errors;

	if (alloc_errors > 0) {
		shell_fprintf(sh, SHELL_WARNING,
			      "There was %u network buffer allocation "
			      "errors during send.\nConsider increasing the "
			      "value of CONFIG_NET_BUF_TX_COUNT and\n"
			      "optionally CONFIG_NET_PKT_TX_COUNT Kconfig "
			      "options.\n",
			      alloc_errors);
	}
}
