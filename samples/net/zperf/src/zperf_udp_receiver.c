/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <linker/sections.h>
#include <toolchain.h>

#include <zephyr.h>
#include <misc/printk.h>

#include <net/net_core.h>
#include <net/net_pkt.h>

#include "zperf.h"
#include "zperf_internal.h"
#include "shell_utils.h"
#include "zperf_session.h"

/* To get net_sprint_ipv{4|6}_addr() */
#define NET_LOG_ENABLED 1
#include "net_private.h"

#define TAG CMD_STR_UDP_DOWNLOAD" "

#define RX_THREAD_STACK_SIZE 1024
#define MY_SRC_PORT 50001

/* Static data */
static K_THREAD_STACK_DEFINE(zperf_rx_stack, RX_THREAD_STACK_SIZE);
static struct k_thread zperf_rx_thread_data;

static struct sockaddr_in6 *in6_addr_my;
static struct sockaddr_in *in4_addr_my;

#define MAX_DBG_PRINT 64

static inline void set_dst_addr(sa_family_t family,
				struct net_pkt *pkt,
				struct sockaddr *dst_addr)
{
#if defined(CONFIG_NET_IPV6)
	if (family == AF_INET6) {
		net_ipaddr_copy(&net_sin6(dst_addr)->sin6_addr,
				&NET_IPV6_HDR(pkt)->src);
		net_sin6(dst_addr)->sin6_family = AF_INET6;
		net_sin6(dst_addr)->sin6_port = NET_UDP_HDR(pkt)->src_port;
	}
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
	if (family == AF_INET) {
		net_ipaddr_copy(&net_sin(dst_addr)->sin_addr,
				&NET_IPV4_HDR(pkt)->src);
		net_sin(dst_addr)->sin_family = AF_INET;
		net_sin(dst_addr)->sin_port = NET_UDP_HDR(pkt)->src_port;
	}
#endif /* CONFIG_NET_IPV4 */
}

static inline struct net_pkt *build_reply_pkt(struct net_context *context,
					      struct net_pkt *pkt,
					      struct zperf_udp_datagram *hdr,
					      struct zperf_server_hdr *stat)
{
	struct net_pkt *reply_pkt;
	struct net_buf *frag;

	printk(TAG "received %d bytes\n",  net_pkt_appdatalen(pkt));

	reply_pkt = net_pkt_get_tx(context, K_FOREVER);
	frag = net_pkt_get_data(context, K_FOREVER);

	net_pkt_frag_add(reply_pkt, frag);

	net_pkt_append_be32(reply_pkt, hdr->id);
	net_pkt_append_be32(reply_pkt, hdr->tv_sec);
	net_pkt_append_be32(reply_pkt, hdr->tv_usec);

	net_pkt_append_be32(reply_pkt, stat->flags);
	net_pkt_append_be32(reply_pkt, stat->total_len1);
	net_pkt_append_be32(reply_pkt, stat->total_len2);
	net_pkt_append_be32(reply_pkt, stat->stop_sec);
	net_pkt_append_be32(reply_pkt, stat->stop_usec);
	net_pkt_append_be32(reply_pkt, stat->error_cnt);
	net_pkt_append_be32(reply_pkt, stat->outorder_cnt);
	net_pkt_append_be32(reply_pkt, stat->datagrams);
	net_pkt_append_be32(reply_pkt, stat->jitter1);
	net_pkt_append_be32(reply_pkt, stat->jitter2);

	return reply_pkt;
}

/* Send statistics to the remote client */
static int zperf_receiver_send_stat(struct net_context *context,
				    struct net_pkt *pkt,
				    struct zperf_udp_datagram *hdr,
				    struct zperf_server_hdr *stat)
{
	struct net_pkt *reply_pkt;
	struct sockaddr dst_addr;
	int ret;

	set_dst_addr(net_pkt_family(pkt), pkt, &dst_addr);

	reply_pkt = build_reply_pkt(context, pkt, hdr, stat);

	net_pkt_unref(pkt);

	ret = net_context_sendto(reply_pkt, &dst_addr,
				 net_pkt_family(pkt) == AF_INET6 ?
				 sizeof(struct sockaddr_in6) :
				 sizeof(struct sockaddr_in),
				 NULL, 0, NULL, NULL);
	if (ret < 0) {
		printk(TAG " Cannot send data to peer (%d)", ret);
		net_pkt_unref(reply_pkt);
	}

	return ret;
}

static void udp_received(struct net_context *context,
			 struct net_pkt *pkt,
			 int status,
			 void *user_data)
{
	struct net_buf *frag = pkt->frags;
	struct zperf_udp_datagram hdr;
	struct session *session;
	u16_t offset, pos;
	s32_t transit_time;
	u32_t time;
	s32_t id;

	if (!pkt) {
		return;
	}

	if (net_pkt_appdatalen(pkt) < sizeof(struct zperf_udp_datagram)) {
		net_pkt_unref(pkt);
		return;
	}

	time = k_cycle_get_32();

	session = get_session(pkt, SESSION_UDP);
	if (!session) {
		printk(TAG "ERROR! cannot get a session!\n");
		return;
	}

	offset = net_pkt_appdata(pkt) - net_pkt_ip_data(pkt);

	frag = net_frag_read_be32(frag, offset, &pos, (u32_t *)&hdr.id);
	frag = net_frag_read_be32(frag, pos, &pos, &hdr.tv_sec);
	frag = net_frag_read_be32(frag, pos, &pos, &hdr.tv_usec);

	id = hdr.id;

	/* Check last packet flags */
	if (id < 0) {
		printk(TAG "End of session!\n");

		if (session->state == STATE_COMPLETED) {
			/* Session is already completed: Resend the stat packet
			 * and continue
			 */
			if (zperf_receiver_send_stat(context, pkt, &hdr,
						     &session->stat) < 0) {
				printk(TAG "ERROR! Failed to send the "
				       "packet\n");

				net_pkt_unref(pkt);
			}
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
	session->length += net_pkt_appdatalen(pkt);

	/* Compute jitter */
	transit_time = time_delta(HW_CYCLES_TO_USEC(time),
				  hdr.tv_sec * USEC_PER_SEC +
				  hdr.tv_usec);
	if (session->last_transit_time != 0) {
		s32_t delta_transit = transit_time -
			session->last_transit_time;

		delta_transit =
			(delta_transit < 0) ? -delta_transit : delta_transit;

		session->jitter += (delta_transit - session->jitter) / 16;
	}

	session->last_transit_time = transit_time;

	/* If necessary send statistics */
	if (session->state == STATE_LAST_PACKET_RECEIVED) {
		u32_t rate_in_kbps;
		u32_t duration = HW_CYCLES_TO_USEC(
			time_delta(session->start_time, time));

		/* Update state machine */
		session->state = STATE_COMPLETED;

		/* Compute baud rate */
		if (duration != 0) {
			rate_in_kbps = (u32_t)
				(((u64_t)session->length * (u64_t)8 *
				  (u64_t)USEC_PER_SEC) /
				 ((u64_t)duration * 1024));
		} else {
			rate_in_kbps = 0;

			/* Fill statistics */
			session->stat.flags = 0x80000000;
			session->stat.total_len1 = session->length >> 32;
			session->stat.total_len2 =
				session->length % 0xFFFFFFFF;
			session->stat.stop_sec = duration / USEC_PER_SEC;
			session->stat.stop_usec = duration % USEC_PER_SEC;
			session->stat.error_cnt = session->error;
			session->stat.outorder_cnt = session->outorder;
			session->stat.datagrams = session->counter;
			session->stat.jitter1 = 0;
			session->stat.jitter2 = session->jitter;

			if (zperf_receiver_send_stat(context, pkt, &hdr,
						     &session->stat) < 0) {
				printk(TAG "ERROR! Failed to send the "
				       "packet\n");

				net_pkt_unref(pkt);
			}

			printk(TAG " duration:\t\t");
			print_number(duration, TIME_US, TIME_US_UNIT);
			printk("\n");

			printk(TAG " received packets:\t%u\n",
			       session->counter);
			printk(TAG " nb packets lost:\t%u\n",
			       session->outorder);
			printk(TAG " nb packets outorder:\t%u\n",
			       session->error);

			printk(TAG " jitter:\t\t\t");
			print_number(session->jitter, TIME_US, TIME_US_UNIT);
			printk("\n");

			printk(TAG " rate:\t\t\t");
			print_number(rate_in_kbps, KBPS, KBPS_UNIT);
			printk("\n");
		}
	} else {
		net_pkt_unref(pkt);
	}
}

/* RX thread entry point */
static void zperf_rx_thread(int port)
{
	struct net_context *context4 = NULL, *context6 = NULL;
	int ret, fail = 0;

	printk(TAG "Listening to port %d\n", port);

#if defined(CONFIG_NET_IPV4) && defined(MY_IP4ADDR)
	ret = net_context_get(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &context4);
	if (ret < 0) {
		printk(TAG "ERROR! Cannot get IPv4 network context.\n");
		return;
	}

	ret = zperf_get_ipv4_addr(MY_IP4ADDR, &in4_addr_my->sin_addr, TAG);
	if (ret < 0) {
		printk(TAG "ERROR! Unable to set IPv4\n");
		return;
	}

	printk(TAG "Binding to %s\n",
	       net_sprint_ipv4_addr(&in4_addr_my->sin_addr));

	in4_addr_my->sin_port = htons(port);
#endif

#if defined(CONFIG_NET_IPV6) && defined(MY_IP6ADDR)
	ret = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_UDP, &context6);
	if (ret < 0) {
		printk(TAG "ERROR! Cannot get IPv6 network context.\n");
		return;
	}

	ret = zperf_get_ipv6_addr(MY_IP6ADDR, MY_PREFIX_LEN_STR,
				  &in6_addr_my->sin6_addr, TAG);
	if (ret < 0) {
		printk(TAG "ERROR! Unable to set IPv6\n");
		return;
	}

	printk(TAG "Binding to %s\n",
	       net_sprint_ipv6_addr(&in6_addr_my->sin6_addr));

	in6_addr_my->sin6_port = htons(port);
#endif

	if (context6) {
		ret = net_context_bind(context6,
				       (struct sockaddr *)in6_addr_my,
				       sizeof(struct sockaddr_in6));
		if (ret < 0) {
			printk(TAG "Cannot bind IPv6 UDP port %d (%d)\n",
			       ntohs(in6_addr_my->sin6_port), ret);

			fail++;
		}
	}

	if (context4) {
		ret = net_context_bind(context4,
				       (struct sockaddr *)in4_addr_my,
				       sizeof(struct sockaddr_in));
		if (ret < 0) {
			printk(TAG "Cannot bind IPv4 UDP port %d (%d)\n",
			       ntohs(in4_addr_my->sin_port), ret);

			fail++;
		}
	}

	if (fail > 1) {
		return;
	}

#if defined(CONFIG_NET_IPV6)
	ret = net_context_recv(context6, udp_received, K_NO_WAIT, NULL);
	if (ret < 0) {
		printk(TAG "Cannot receive IPv6 UDP packets\n");
	}
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
	ret = net_context_recv(context4, udp_received, K_NO_WAIT, NULL);
	if (ret < 0) {
		printk(TAG "Cannot receive IPv4 UDP packets\n");
	}
#endif /* CONFIG_NET_IPV4 */

	k_sleep(K_FOREVER);
}

void zperf_receiver_init(int port)
{
#if defined(CONFIG_NET_IPV6)
	in6_addr_my = zperf_get_sin6();
#endif
#if defined(CONFIG_NET_IPV4)
	in4_addr_my = zperf_get_sin();
#endif

	k_thread_create(&zperf_rx_thread_data, zperf_rx_stack,
			K_THREAD_STACK_SIZEOF(zperf_rx_stack),
			(k_thread_entry_t)zperf_rx_thread,
			INT_TO_POINTER(port), 0, 0,
			K_PRIO_COOP(7), 0, K_NO_WAIT);
}
