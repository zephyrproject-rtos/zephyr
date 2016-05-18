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

#define TAG CMD_STR_UDP_DOWNLOAD" "

#define RX_FIBER_STACK_SIZE 1024

/* Static data */
static char __noinit __stack zperf_rx_fiber_stack[RX_FIBER_STACK_SIZE];

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

/* Send statistic to the remote client */
static int zperf_receiver_send_stat(struct net_context *net_context,
		struct net_buf *buf, zperf_server_hdr *stat)
{
	/* Fill the packet header */
	memcpy(ip_buf_appdata(buf) + sizeof(zperf_udp_datagram), stat,
			sizeof(zperf_server_hdr));

	/* Send the packet */
	return net_reply(net_context, buf);
}

/* RX fiber entry point */
static void zperf_rx_fiber(int port)
{
	struct net_context *net_context = net_context_get(IPPROTO_UDP, &in_addr_any,
			0, &in_addr_my, port);

	if (!net_context) {
		printk(TAG "ERROR! Cannot get network context.\n");
		return;
	}

	while (1) {
		struct net_buf *buf = net_receive(net_context, TICKS_UNLIMITED);

		if (buf == NULL) {
			continue;
		} else if (ip_buf_appdatalen(buf) < sizeof(zperf_udp_datagram)) {
			/* Release the buffer */
			ip_buf_unref(buf);
		} else {
			uint32_t time = sys_cycle_get_32();
			int32_t id;
			int32_t transit_time;

			struct session *session = get_session(buf, SESSION_UDP);

			if (session == NULL) {
				printk(TAG "ERROR! cannot get a session!\n");
				continue;
			}

			zperf_udp_datagram *hdr = (zperf_udp_datagram *) ip_buf_appdata(buf);

			id = z_ntohl(hdr->id);

			/* Check last packet flags */
			if (id < 0) {
				printk(TAG "End of session!\n");
				if (session->state == STATE_COMPLETED) {
					/* Session is already completed: Resend the stat buffer
					 * and continue
					 */
					if (zperf_receiver_send_stat(net_context, buf,
							&session->stat) < 0) {
						printk(TAG "ERROR! Failed to send the buffer\n");

						/* Free the buffer */
						ip_buf_unref(buf);
					}
					continue;
				} else {
					session->state = STATE_LAST_PACKET_RECEIVED;
					id = -id;
				}
			} else if (session->state != STATE_ONGOING) {
				/* Start a new session! */
				printk(TAG "New session started.\n");
				zperf_reset_session_stats(session);
				session->state = STATE_ONGOING;
				session->start_time = time;
			}

			/* Check header id */
			if (id != session->next_id) {
				if (id < session->next_id) {
					session->outorder++;
				} else {
					session->error += id - session->next_id;
					session->next_id = id + 1;
				}
			} else {
				session->next_id++;
			}

			/* Update counter */
			session->counter++;
			session->length += ip_buf_appdatalen(buf);

			/* Compute jitter */
			transit_time = time_delta(HW_CYCLES_TO_USEC(time),
			z_ntohl(hdr->tv_sec) * USEC_PER_SEC + z_ntohl(hdr->tv_usec));
			if (session->last_transit_time != 0) {
				int32_t delta_transit = transit_time
						- session->last_transit_time;
				delta_transit =
					(delta_transit < 0) ? -delta_transit : delta_transit;
				session->jitter += (delta_transit - session->jitter) / 16;
			}
			session->last_transit_time = transit_time;

			/* If necessary send statistic */
			if (session->state == STATE_LAST_PACKET_RECEIVED) {
				uint32_t rate_in_kbps;
				uint32_t duration = HW_CYCLES_TO_USEC(
						time_delta(session->start_time, time));

				/* Update state machine */
				session->state = STATE_COMPLETED;

				/* Compute baud rate */
				if (duration != 0)
					rate_in_kbps = (uint32_t) (((uint64_t) session->length
							* (uint64_t) 8 * (uint64_t) USEC_PER_SEC)
							/ ((uint64_t) duration * 1024));
				else
					rate_in_kbps = 0;

				/* Fill static */
				session->stat.flags = z_htonl(0x80000000);
				session->stat.total_len1 = z_htonl(session->length >> 32);
				session->stat.total_len2 = z_htonl(
						session->length % 0xFFFFFFFF);
				session->stat.stop_sec = z_htonl(duration / USEC_PER_SEC);
				session->stat.stop_usec = z_htonl(duration % USEC_PER_SEC);
				session->stat.error_cnt = z_htonl(session->error);
				session->stat.outorder_cnt = z_htonl(session->outorder);
				session->stat.datagrams = z_htonl(session->counter);
				session->stat.jitter1 = 0;
				session->stat.jitter2 = z_htonl(session->jitter);

				if (zperf_receiver_send_stat(net_context, buf, &session->stat)
						< 0) {
					printk(TAG "ERROR! Failed to send the buffer\n");

					/* Free the buffer */
					ip_buf_unref(buf);
				}

				printk(TAG " duration:\t\t");
				print_number(duration, TIME_US, TIME_US_UNIT);
				printk("\n");

				printk(TAG " received packets:\t%u\n", session->counter);
				printk(TAG " nb packets lost:\t%u\n", session->outorder);
				printk(TAG " nb packets outorder:\t%u\n", session->error);

				printk(TAG " jitter:\t\t\t");
				print_number(session->jitter, TIME_US, TIME_US_UNIT);
				printk("\n");

				printk(TAG " rate:\t\t\t");
				print_number(rate_in_kbps, KBPS, KBPS_UNIT);
				printk("\n");
			} else {
				/* Free the buffer */
				ip_buf_unref(buf);
			}
		}
	}
}

void zperf_receiver_init(int port)
{
	fiber_start(zperf_rx_fiber_stack, sizeof(zperf_rx_fiber_stack),
			(nano_fiber_entry_t) zperf_rx_fiber, port, 0, 7, 0);
}
