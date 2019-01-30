/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_DECLARE(net_zperf_sample, LOG_LEVEL_DBG);

#include <linker/sections.h>
#include <toolchain.h>

#include <zephyr.h>
#include <misc/printk.h>

#include <net/net_core.h>
#include <net/net_pkt.h>
#include <net/udp.h>

#include "zperf.h"
#include "zperf_internal.h"
#include "shell_utils.h"
#include "zperf_session.h"

/* To get net_sprint_ipv{4|6}_addr() */
#define NET_LOG_ENABLED 1
#include "net_private.h"

static struct sockaddr_in6 *in6_addr_my;
static struct sockaddr_in *in4_addr_my;

static inline void set_dst_addr(const struct shell *shell,
				sa_family_t family,
				struct net_pkt *pkt,
				struct sockaddr *dst_addr)
{
	struct net_udp_hdr hdr, *udp_hdr;

	udp_hdr = net_udp_get_hdr(pkt, &hdr);
	if (!udp_hdr) {
		shell_fprintf(shell, SHELL_WARNING,
			      "Invalid UDP data\n");
		return;
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && family == AF_INET6) {
		net_ipaddr_copy(&net_sin6(dst_addr)->sin6_addr,
				&NET_IPV6_HDR(pkt)->src);
		net_sin6(dst_addr)->sin6_family = AF_INET6;
		net_sin6(dst_addr)->sin6_port = udp_hdr->src_port;
	}

	if (IS_ENABLED(CONFIG_NET_IPV4) && family == AF_INET) {
		net_ipaddr_copy(&net_sin(dst_addr)->sin_addr,
				&NET_IPV4_HDR(pkt)->src);
		net_sin(dst_addr)->sin_family = AF_INET;
		net_sin(dst_addr)->sin_port = udp_hdr->src_port;
	}
}

static inline struct net_pkt *build_reply_pkt(const struct shell *shell,
					      struct net_context *context,
					      struct net_pkt *pkt,
					      struct zperf_udp_datagram *hdr,
					      struct zperf_server_hdr *stat)
{
	struct net_pkt *reply_pkt;
	struct net_buf *frag;

	shell_fprintf(shell, SHELL_NORMAL,
		      "Received %d bytes\n", net_pkt_appdatalen(pkt));

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
static int zperf_receiver_send_stat(const struct shell *shell,
				    struct net_context *context,
				    struct net_pkt *pkt,
				    struct zperf_udp_datagram *hdr,
				    struct zperf_server_hdr *stat)
{
	struct net_pkt *reply_pkt;
	struct sockaddr dst_addr;
	int ret;

	set_dst_addr(shell, net_pkt_family(pkt), pkt, &dst_addr);

	reply_pkt = build_reply_pkt(shell, context, pkt, hdr, stat);

	net_pkt_unref(pkt);

	ret = net_context_sendto(reply_pkt, &dst_addr,
				 net_pkt_family(pkt) == AF_INET6 ?
				 sizeof(struct sockaddr_in6) :
				 sizeof(struct sockaddr_in),
				 NULL, 0, NULL, NULL);
	if (ret < 0) {
		shell_fprintf(shell, SHELL_WARNING,
			      " Cannot send data to peer (%d)", ret);
		net_pkt_unref(reply_pkt);
	}

	return ret;
}

static void udp_received(struct net_context *context,
			 struct net_pkt *pkt,
			 union net_ip_header *ip_hdr,
			 union net_proto_header *proto_hdr,
			 int status,
			 void *user_data)
{
	const struct shell *shell = user_data;
	struct zperf_udp_datagram hdr;
	struct session *session;
	struct net_buf *frag;
	u16_t offset, pos;
	s32_t transit_time;
	u32_t time;
	s32_t id;

	if (!pkt) {
		return;
	}

	frag = pkt->frags;

	if (net_pkt_appdatalen(pkt) < sizeof(struct zperf_udp_datagram)) {
		shell_fprintf(shell, SHELL_WARNING,
			      "Short iperf packet!\n");
		net_pkt_unref(pkt);
		return;
	}

	time = k_cycle_get_32();

	session = get_session(pkt, SESSION_UDP);
	if (!session) {
		shell_fprintf(shell, SHELL_WARNING,
			      "Cannot get a session!\n");
		return;
	}

	offset = net_pkt_appdata(pkt) - net_pkt_ip_data(pkt);

	frag = net_frag_read_be32(frag, offset, &pos, (u32_t *)&hdr.id);
	frag = net_frag_read_be32(frag, pos, &pos, &hdr.tv_sec);
	frag = net_frag_read_be32(frag, pos, &pos, &hdr.tv_usec);

	id = hdr.id;

	/* Check last packet flags */
	if (id < 0) {
		shell_fprintf(shell, SHELL_NORMAL, "End of session!\n");

		if (session->state == STATE_COMPLETED) {
			/* Session is already completed: Resend the stat packet
			 * and continue
			 */
			if (zperf_receiver_send_stat(shell, context, pkt, &hdr,
						     &session->stat) < 0) {
				shell_fprintf(shell, SHELL_WARNING,
					      "Failed to send the packet\n");

				net_pkt_unref(pkt);
			}
		} else {
			session->state = STATE_LAST_PACKET_RECEIVED;
			id = -id;
		}

	} else if (session->state != STATE_ONGOING) {
		/* Start a new session! */
		shell_fprintf(shell, SHELL_NORMAL,
			      "New session started.\n");

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
			rate_in_kbps = 0U;

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

			if (zperf_receiver_send_stat(shell, context, pkt, &hdr,
						     &session->stat) < 0) {
				shell_fprintf(shell, SHELL_WARNING,
					    "Failed to send the packet\n");

				net_pkt_unref(pkt);
			}

			shell_fprintf(shell, SHELL_NORMAL,
				      " duration:\t\t");
			print_number(shell, duration, TIME_US, TIME_US_UNIT);
			shell_fprintf(shell, SHELL_NORMAL, "\n");

			shell_fprintf(shell, SHELL_NORMAL,
				      " received packets:\t%u\n",
				      session->counter);
			shell_fprintf(shell, SHELL_NORMAL,
				      " nb packets lost:\t%u\n",
				      session->outorder);
			shell_fprintf(shell, SHELL_NORMAL,
				      " nb packets outorder:\t%u\n",
				      session->error);

			shell_fprintf(shell, SHELL_NORMAL,
				      " jitter:\t\t\t");
			print_number(shell, session->jitter, TIME_US,
				     TIME_US_UNIT);
			shell_fprintf(shell, SHELL_NORMAL, "\n");

			shell_fprintf(shell, SHELL_NORMAL,
				      " rate:\t\t\t");
			print_number(shell, rate_in_kbps, KBPS, KBPS_UNIT);
			shell_fprintf(shell, SHELL_NORMAL, "\n");
		}
	} else {
		net_pkt_unref(pkt);
	}
}

void zperf_udp_receiver_init(const struct shell *shell, int port)
{
	struct net_context *context4 = NULL;
	struct net_context *context6 = NULL;
	const struct in_addr *in4_addr = NULL;
	const struct in6_addr *in6_addr = NULL;
	int ret;

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		in6_addr_my = zperf_get_sin6();
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		in4_addr_my = zperf_get_sin();
	}


	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		ret = net_context_get(AF_INET, SOCK_DGRAM, IPPROTO_UDP,
				      &context4);
		if (ret < 0) {
			shell_fprintf(shell, SHELL_WARNING,
				      "Cannot get IPv4 network context.\n");
			return;
		}

		if (MY_IP4ADDR && strlen(MY_IP4ADDR)) {
			/* Use setting IP */
			ret = zperf_get_ipv4_addr(shell, MY_IP4ADDR,
						  &in4_addr_my->sin_addr);
			if (ret < 0) {
				shell_fprintf(shell, SHELL_WARNING,
					      "Unable to set IPv4\n");
				return;
			}
		} else {
			/* Use existing IP */
			in4_addr = zperf_get_default_if_in4_addr();
			if (!in4_addr) {
				shell_fprintf(shell, SHELL_WARNING,
					      "Unable to get IPv4 by default\n");
				return;
			}
			memcpy(&in4_addr_my->sin_addr, in4_addr,
				sizeof(struct in_addr));
		}

		shell_fprintf(shell, SHELL_NORMAL, "Binding to %s\n",
			      net_sprint_ipv4_addr(&in4_addr_my->sin_addr));

		in4_addr_my->sin_port = htons(port);

		if (context4) {
			ret = net_context_bind(context4,
					       (struct sockaddr *)in4_addr_my,
					       sizeof(struct sockaddr_in));
			if (ret < 0) {
				shell_fprintf(shell, SHELL_WARNING,
					 "Cannot bind IPv4 UDP port %d (%d)\n",
					      ntohs(in4_addr_my->sin_port),
					      ret);
				return;
			}
		}
	}

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		ret = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_UDP,
				      &context6);
		if (ret < 0) {
			shell_fprintf(shell, SHELL_WARNING,
				      "Cannot get IPv6 network context.\n");
			return;
		}

		if (MY_IP6ADDR && strlen(MY_IP6ADDR)) {
			/* Use setting IP */
			ret = zperf_get_ipv6_addr(shell, MY_IP6ADDR,
						  MY_PREFIX_LEN_STR,
						  &in6_addr_my->sin6_addr);
			if (ret < 0) {
				shell_fprintf(shell, SHELL_WARNING,
					      "Unable to set IPv6\n");
				return;
			}
		} else {
			/* Use existing IP */
			in6_addr = zperf_get_default_if_in6_addr();
			if (!in6_addr) {
				shell_fprintf(shell, SHELL_WARNING,
					      "Unable to get IPv4 by default\n");
				return;
			}
			memcpy(&in6_addr_my->sin6_addr, in6_addr,
				sizeof(struct in6_addr));
		}

		shell_fprintf(shell, SHELL_NORMAL, "Binding to %s\n",
			      net_sprint_ipv6_addr(&in6_addr_my->sin6_addr));

		in6_addr_my->sin6_port = htons(port);

		if (context6) {
			ret = net_context_bind(context6,
					       (struct sockaddr *)in6_addr_my,
					       sizeof(struct sockaddr_in6));
			if (ret < 0) {
				shell_fprintf(shell, SHELL_WARNING,
					 "Cannot bind IPv6 UDP port %d (%d)\n",
					      ntohs(in6_addr_my->sin6_port),
					      ret);
				return;
			}
		}
	}

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		ret = net_context_recv(context6, udp_received, K_NO_WAIT,
				       (void *)shell);
		if (ret < 0) {
			shell_fprintf(shell, SHELL_WARNING,
				      "Cannot receive IPv6 UDP packets\n");
			return;
		}
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		ret = net_context_recv(context4, udp_received, K_NO_WAIT,
				       (void *)shell);
		if (ret < 0) {
			shell_fprintf(shell, SHELL_WARNING,
				      "Cannot receive IPv4 UDP packets\n");
			return;
		}
	}

	shell_fprintf(shell, SHELL_NORMAL,
		      "Listening on port %d\n", port);
}
