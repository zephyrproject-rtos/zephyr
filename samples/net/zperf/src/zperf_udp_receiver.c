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
#include <sys/printk.h>

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
				union net_ip_header *ip_hdr,
				struct net_udp_hdr *udp_hdr,
				struct sockaddr *dst_addr)
{
	if (IS_ENABLED(CONFIG_NET_IPV6) && family == AF_INET6) {
		net_ipaddr_copy(&net_sin6(dst_addr)->sin6_addr,
				&ip_hdr->ipv6->src);
		net_sin6(dst_addr)->sin6_family = AF_INET6;
		net_sin6(dst_addr)->sin6_port = udp_hdr->src_port;
	}

	if (IS_ENABLED(CONFIG_NET_IPV4) && family == AF_INET) {
		net_ipaddr_copy(&net_sin(dst_addr)->sin_addr,
				&ip_hdr->ipv4->src);
		net_sin(dst_addr)->sin_family = AF_INET;
		net_sin(dst_addr)->sin_port = udp_hdr->src_port;
	}
}

static inline void build_reply(struct zperf_udp_datagram *hdr,
			       struct zperf_server_hdr *stat,
			       u8_t *buf)
{
	int pos = 0;
	struct zperf_server_hdr *stat_hdr;

	memcpy(&buf[pos], hdr, sizeof(struct zperf_udp_datagram));
	pos += sizeof(struct zperf_udp_datagram);

	stat_hdr = (struct zperf_server_hdr *)&buf[pos];

	stat_hdr->flags = htonl(stat->flags);
	stat_hdr->total_len1 = htonl(stat->total_len1);
	stat_hdr->total_len2 = htonl(stat->total_len2);
	stat_hdr->stop_sec = htonl(stat->stop_sec);
	stat_hdr->stop_usec = htonl(stat->stop_usec);
	stat_hdr->error_cnt = htonl(stat->error_cnt);
	stat_hdr->outorder_cnt = htonl(stat->outorder_cnt);
	stat_hdr->datagrams = htonl(stat->datagrams);
	stat_hdr->jitter1 = htonl(stat->jitter1);
	stat_hdr->jitter2 = htonl(stat->jitter2);
}

/* Send statistics to the remote client */
#define BUF_SIZE sizeof(struct zperf_udp_datagram) +	\
	sizeof(struct zperf_server_hdr)

static int zperf_receiver_send_stat(const struct shell *shell,
				    struct net_context *context,
				    struct net_pkt *pkt,
				    union net_ip_header *ip_hdr,
				    struct net_udp_hdr *udp_hdr,
				    struct zperf_udp_datagram *hdr,
				    struct zperf_server_hdr *stat)
{
	u8_t reply[BUF_SIZE];
	struct sockaddr dst_addr;
	int ret;

	shell_fprintf(shell, SHELL_NORMAL,
		      "Received %d bytes\n", net_pkt_remaining_data(pkt));

	set_dst_addr(shell, net_pkt_family(pkt),
		     pkt, ip_hdr, udp_hdr, &dst_addr);

	build_reply(hdr, stat, reply);

	ret = net_context_sendto(context, reply, BUF_SIZE, &dst_addr,
				 net_pkt_family(pkt) == AF_INET6 ?
				 sizeof(struct sockaddr_in6) :
				 sizeof(struct sockaddr_in),
				 NULL, K_NO_WAIT, NULL);
	if (ret < 0) {
		shell_fprintf(shell, SHELL_WARNING,
			      " Cannot send data to peer (%d)", ret);
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
	NET_PKT_DATA_ACCESS_DEFINE(zperf, struct zperf_udp_datagram);
	struct net_udp_hdr *udp_hdr = proto_hdr->udp;
	const struct shell *shell = user_data;
	struct zperf_udp_datagram *hdr;
	struct session *session;
	s32_t transit_time;
	u32_t time;
	s32_t id;

	if (!pkt) {
		return;
	}

	hdr = (struct zperf_udp_datagram *)net_pkt_get_data(pkt, &zperf);
	if (!hdr) {
		shell_fprintf(shell, SHELL_WARNING,
			      "Short iperf packet!\n");
		goto out;
	}

	time = k_cycle_get_32();

	session = get_session(pkt, ip_hdr, proto_hdr, SESSION_UDP);
	if (!session) {
		shell_fprintf(shell, SHELL_WARNING,
			      "Cannot get a session!\n");
		goto out;
	}

	id = ntohl(hdr->id);

	switch (session->state) {
	case STATE_COMPLETED:
	case STATE_NULL:
		if (id < 0) {
			/* Session is already completed: Resend the stat packet
			 * and continue
			 */
			if (zperf_receiver_send_stat(shell, context, pkt,
						     ip_hdr, udp_hdr, hdr,
						     &session->stat) < 0) {
				shell_fprintf(shell, SHELL_WARNING,
					      "Failed to send the packet\n");
			}
		} else {
			/* Start a new session! */
			shell_fprintf(shell, SHELL_NORMAL,
				      "New session started.\n");

			zperf_reset_session_stats(session);
			session->state = STATE_ONGOING;
			session->start_time = time;
		}
		break;
	case STATE_ONGOING:
		if (id < 0) { /* Negative id means session end. */
			u32_t rate_in_kbps;
			u32_t duration;

			shell_fprintf(shell, SHELL_NORMAL, "End of session!\n");

			duration = HW_CYCLES_TO_USEC(
				time_delta(session->start_time, time));
			/* Update state machine */
			session->state = STATE_COMPLETED;

			/* Compute baud rate */
			if (duration != 0U) {
				rate_in_kbps = (u32_t)
					(((u64_t)session->length * (u64_t)8 *
					  (u64_t)USEC_PER_SEC) /
					 ((u64_t)duration * 1024U));
			} else {
				rate_in_kbps = 0U;
			}

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

			if (zperf_receiver_send_stat(shell, context, pkt,
						     ip_hdr, udp_hdr, hdr,
						     &session->stat) < 0) {
				shell_fprintf(shell, SHELL_WARNING,
					    "Failed to send the packet\n");
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
		} else {
			/* Update counter */
			session->counter++;
			session->length += net_pkt_remaining_data(pkt);

			/* Compute jitter */
			transit_time = time_delta(HW_CYCLES_TO_USEC(time),
						  ntohl(hdr->tv_sec) *
						  USEC_PER_SEC +
						  ntohl(hdr->tv_usec));
			if (session->last_transit_time != 0) {
				s32_t delta_transit = transit_time -
					session->last_transit_time;

				delta_transit =
					(delta_transit < 0) ?
					-delta_transit : delta_transit;

				session->jitter +=
					(delta_transit - session->jitter) / 16;
			}

			session->last_transit_time = transit_time;

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
		}
		break;
	default:
		break;
	}

out:
	net_pkt_unref(pkt);
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
