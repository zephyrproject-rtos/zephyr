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

#include <errno.h>
#include <misc/printk.h>
#include <net/ip_buf.h>
#include <net/net_ip.h>
#include <net/net_core.h>
#include <net/net_socket.h>
#include <zephyr.h>

#include "zperf.h"
#include "zperf_internal.h"

#define TAG CMD_STR_TCP_UPLOAD" "

void zperf_tcp_upload(struct net_context *net_context,
		unsigned int duration_in_ms,
		unsigned int packet_size,
		zperf_results *results)
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
	start_time = sys_cycle_get_32();
	last_print_time = start_time;
	last_loop_time = start_time;
	printk(TAG "New session started\n");
	do {
		uint32_t loop_time;
		uint8_t *ptr = NULL;
		int ret = 0;

		/* Timestamps */
		loop_time = sys_cycle_get_32();
		last_loop_time = loop_time;

		/* Get a new TX buffer */
		struct net_buf *buf = ip_buf_get_tx(net_context);

		if (!buf) {
			printk(TAG "ERROR! Failed to retrieve a buffer\n");
			continue;
		}

		/* Fill in the TCP payload */
		ptr = net_buf_add(buf, packet_size);
		memset(ptr, 'z', packet_size);

		/* If test time is elapsed, send a last packet with specific flag
		 * to request uIP to close the TCP connection
		 */
		if (time_elapsed) {
			uip_flags(buf) |= UIP_CLOSE;
		}

		/* Send the packet */
again:
		ret = net_send(buf);
		if (ret < 0) {
			if (ret == -EINPROGRESS || ret == -EAGAIN) {
				nb_errors++;
				fiber_sleep(100);
				goto again;
			} else {
				printk("ERROR! Failed to send the buffer\n");
				nb_errors++;
			}
		} else {
			nb_packets++;
			/* if test time is elapsed and are here, that means TCP connection
			 * has been closed by uIP as requested. So exit the loop.
			 */
			if (time_elapsed) {
				finished = 1;
			}
		}

		if (!time_elapsed && time_delta(start_time, last_loop_time) > duration)
			time_elapsed = 1;

		ip_buf_unref(buf);
		fiber_yield();
	} while (!finished);

	end_time = sys_cycle_get_32();

	/* Add result coming from the client */
	results->nb_packets_sent = nb_packets;
	results->client_time_in_us = HW_CYCLES_TO_USEC(
			time_delta(start_time, end_time));
	results->packet_size = packet_size;
	results->nb_packets_errors = nb_errors;
}
