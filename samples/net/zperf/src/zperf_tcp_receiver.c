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
#include <net/net_core.h>
#include <net/net_socket.h>
#include <net/ip_buf.h>
#include <net/net_ip.h>
#include <sections.h>
#include <toolchain.h>
#include <zephyr.h>

#include "zperf.h"
#include "zperf_internal.h"
#include "shell_utils.h"
#include "zperf_session.h"

#define TAG CMD_STR_TCP_DOWNLOAD" "
#define TCP_RX_FIBER_STACK_SIZE 1024

/* Static data */
static char __noinit __stack zperf_tcp_rx_fiber_stack[TCP_RX_FIBER_STACK_SIZE];

static struct net_addr in_addr_any = {
#ifdef CONFIG_NETWORKING_WITH_IPV6
	.family = AF_INET6,
	.in6_addr = IN6ADDR_ANY_INIT
#else
	.family = AF_INET, .in_addr = { { { 0 } } }
#endif
};

static struct net_addr in_addr_my = {
#ifdef CONFIG_NETWORKING_WITH_IPV6
	.family = AF_INET6,
	.in6_addr = IN6ADDR_ANY_INIT
#else
	.family = AF_INET, .in_addr = { { { 0 } } }
#endif
};

/* TCP RX fiber entry point */
static void zperf_tcp_rx_fiber(int port)
{
	struct net_context *net_context = net_context_get(IPPROTO_TCP, &in_addr_any,
			0, &in_addr_my, port);

	if (!net_context) {
		printk(TAG "ERROR! Cannot get network context.\n");
		return;
	}

	while (1) {
		struct net_buf *buf = net_receive(net_context, TICKS_UNLIMITED);
		struct session *session = NULL;
		uint32_t time = sys_cycle_get_32();

		if (buf == NULL) {
			printk(TAG "buf is null\n");
			continue;
		}

		session = get_session(buf, SESSION_UDP);
		if (session == NULL) {
			printk(TAG "ERROR! cannot get a session!\n");
			/* free buffer */
			ip_buf_unref(buf);
			continue;
		}

		switch (session->state) {
		case STATE_NULL:
		case STATE_COMPLETED:
			printk(TAG "New session started\n");
			zperf_reset_session_stats(session);
			session->start_time =  sys_cycle_get_32();
			session->state = STATE_ONGOING;
		case STATE_ONGOING:
			session->counter++;
			session->length += ip_buf_appdatalen(buf);

			if (uip_closed(buf)) {
				session->state = STATE_COMPLETED;
				uint32_t rate_in_kbps;
				uint32_t duration = HW_CYCLES_TO_USEC(
					time_delta(session->start_time, time));

				/* Compute baud rate */
				if (duration != 0) {
					rate_in_kbps = (uint32_t) ((
							(uint64_t) session->length
							* (uint64_t) 8
							* (uint64_t) USEC_PER_SEC)
							/ ((uint64_t) duration * 1024));
				} else {
					rate_in_kbps = 0;
				}

				printk(TAG "TCP session ended\n");
				printk(TAG " duration:\t\t");
				print_number(duration, TIME_US, TIME_US_UNIT);
				printk("\n");
				printk(TAG " rate:\t\t\t");
				print_number(rate_in_kbps, KBPS, KBPS_UNIT);
				printk("\n");
			}
			break;
		case STATE_LAST_PACKET_RECEIVED:
			break;

		default:
			printk(TAG "Error! Unsupported case\n");
		}

		/* free buffer */
		ip_buf_unref(buf);
	}
}

void zperf_tcp_receiver_init(int port)
{
	fiber_start(zperf_tcp_rx_fiber_stack, sizeof(zperf_tcp_rx_fiber_stack),
			(nano_fiber_entry_t) zperf_tcp_rx_fiber, port, 0, 7, 0);
}
