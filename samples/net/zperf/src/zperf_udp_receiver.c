/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_zperf_sample, LOG_LEVEL_DBG);

#include <zephyr/linker/sections.h>
#include <zephyr/toolchain.h>

#include <zephyr/zephyr.h>
#include <zephyr/sys/printk.h>

#include <zephyr/net/socket.h>

#include "zperf.h"
#include "zperf_internal.h"
#include "shell_utils.h"
#include "zperf_session.h"

/* To get net_sprint_ipv{4|6}_addr() */
#define NET_LOG_ENABLED 1
#include "net_private.h"

static struct sockaddr_in6 *in6_addr_my;
static struct sockaddr_in *in4_addr_my;

#if IS_ENABLED(CONFIG_NET_TC_THREAD_COOPERATIVE)
#define UDP_RECEIVER_THREAD_PRIORITY K_PRIO_COOP(8)
#else
#define UDP_RECEIVER_THREAD_PRIORITY K_PRIO_PREEMPT(8)
#endif

#define UDP_RECEIVER_STACK_SIZE 2048

#define SOCK_ID_IPV4 0
#define SOCK_ID_IPV6 1
#define SOCK_ID_MAX 2

#define UDP_RECEIVER_BUF_SIZE 1500

K_THREAD_STACK_DEFINE(udp_receiver_stack_area, UDP_RECEIVER_STACK_SIZE);
struct k_thread udp_receiver_thread_data;

static inline void build_reply(struct zperf_udp_datagram *hdr,
			       struct zperf_server_hdr *stat,
			       uint8_t *buf)
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

static int zperf_receiver_send_stat(const struct shell *sh,
				    int sock, const struct sockaddr *addr,
				    struct zperf_udp_datagram *hdr,
				    struct zperf_server_hdr *stat)
{
	uint8_t reply[BUF_SIZE];
	int ret;

	build_reply(hdr, stat, reply);

	ret = sendto(sock, reply, sizeof(reply), 0, addr,
		     addr->sa_family == AF_INET6 ?
		     sizeof(struct sockaddr_in6) :
		     sizeof(struct sockaddr_in));
	if (ret < 0) {
		shell_fprintf(sh, SHELL_WARNING,
			      " Cannot send data to peer (%d)", errno);
	}

	return ret;
}

static void udp_received(const struct shell *sh, int sock,
			 const struct sockaddr *addr, uint8_t *data,
			 size_t datalen)
{
	struct zperf_udp_datagram *hdr;
	struct session *session;
	int32_t transit_time;
	int64_t time;
	int32_t id;

	if (datalen < sizeof(struct zperf_udp_datagram)) {
		shell_fprintf(sh, SHELL_WARNING,
			      "Short iperf packet!\n");
		return;
	}

	hdr = (struct zperf_udp_datagram *)data;
	time = k_uptime_ticks();

	session = get_session(addr, SESSION_UDP);
	if (!session) {
		shell_fprintf(sh, SHELL_WARNING,
			      "Cannot get a session!\n");
		return;
	}

	id = ntohl(hdr->id);

	switch (session->state) {
	case STATE_COMPLETED:
	case STATE_NULL:
		if (id < 0) {
			/* Session is already completed: Resend the stat packet
			 * and continue
			 */
			if (zperf_receiver_send_stat(sh, sock, addr, hdr,
						     &session->stat) < 0) {
				shell_fprintf(sh, SHELL_WARNING,
					      "Failed to send the packet\n");
			}
		} else {
			/* Start a new session! */
			shell_fprintf(sh, SHELL_NORMAL,
				      "New session started.\n");

			zperf_reset_session_stats(session);
			session->state = STATE_ONGOING;
			session->start_time = time;
		}
		break;
	case STATE_ONGOING:
		if (id < 0) { /* Negative id means session end. */
			uint32_t rate_in_kbps;
			uint32_t duration;

			shell_fprintf(sh, SHELL_NORMAL, "End of session!\n");

			duration = k_ticks_to_us_ceil32(time -
							session->start_time);

			/* Update state machine */
			session->state = STATE_COMPLETED;

			/* Compute baud rate */
			if (duration != 0U) {
				rate_in_kbps = (uint32_t)
					((session->length * 8ULL *
					  (uint64_t)USEC_PER_SEC) /
					 ((uint64_t)duration * 1024ULL));
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

			if (zperf_receiver_send_stat(sh, sock, addr, hdr,
						     &session->stat) < 0) {
				shell_fprintf(sh, SHELL_WARNING,
					    "Failed to send the packet\n");
			}

			shell_fprintf(sh, SHELL_NORMAL,
				      " duration:\t\t");
			print_number(sh, duration, TIME_US, TIME_US_UNIT);
			shell_fprintf(sh, SHELL_NORMAL, "\n");

			shell_fprintf(sh, SHELL_NORMAL,
				      " received packets:\t%u\n",
				      session->counter);
			shell_fprintf(sh, SHELL_NORMAL,
				      " nb packets lost:\t%u\n",
				      session->outorder);
			shell_fprintf(sh, SHELL_NORMAL,
				      " nb packets outorder:\t%u\n",
				      session->error);

			shell_fprintf(sh, SHELL_NORMAL,
				      " jitter:\t\t\t");
			print_number(sh, session->jitter, TIME_US,
				     TIME_US_UNIT);
			shell_fprintf(sh, SHELL_NORMAL, "\n");

			shell_fprintf(sh, SHELL_NORMAL,
				      " rate:\t\t\t");
			print_number(sh, rate_in_kbps, KBPS, KBPS_UNIT);
			shell_fprintf(sh, SHELL_NORMAL, "\n");
		} else {
			/* Update counter */
			session->counter++;
			session->length += datalen;

			/* Compute jitter */
			transit_time = time_delta(
				k_ticks_to_us_ceil32(time),
				ntohl(hdr->tv_sec) * USEC_PER_SEC +
				ntohl(hdr->tv_usec));
			if (session->last_transit_time != 0) {
				int32_t delta_transit = transit_time -
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
}

void udp_receiver_thread(void *ptr1, void *ptr2, void *ptr3)
{
	ARG_UNUSED(ptr3);

	static uint8_t buf[UDP_RECEIVER_BUF_SIZE];
	const struct shell *sh = ptr1;
	int port = POINTER_TO_INT(ptr2);
	struct pollfd fds[SOCK_ID_MAX] = { 0 };
	int ret;

	for (int i = 0; i < ARRAY_SIZE(fds); i++) {
		fds[i].fd = -1;
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		const struct in_addr *in4_addr = NULL;

		in4_addr_my = zperf_get_sin();

		fds[SOCK_ID_IPV4].fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (fds[SOCK_ID_IPV4].fd < 0) {
			shell_fprintf(sh, SHELL_WARNING,
				      "Cannot create IPv4 network socket.\n");
			goto cleanup;
		}

		if (MY_IP4ADDR && strlen(MY_IP4ADDR)) {
			/* Use setting IP */
			ret = zperf_get_ipv4_addr(sh, MY_IP4ADDR,
						  &in4_addr_my->sin_addr);
			if (ret < 0) {
				shell_fprintf(sh, SHELL_WARNING,
					      "Unable to set IPv4\n");
				goto use_existing_ipv4;
			}
		} else {
		use_existing_ipv4:
			/* Use existing IP */
			in4_addr = zperf_get_default_if_in4_addr();
			if (!in4_addr) {
				shell_fprintf(sh, SHELL_WARNING,
					      "Unable to get IPv4 by default\n");
				goto cleanup;
			}
			memcpy(&in4_addr_my->sin_addr, in4_addr,
				sizeof(struct in_addr));
		}

		shell_fprintf(sh, SHELL_NORMAL, "Binding to %s\n",
			      net_sprint_ipv4_addr(&in4_addr_my->sin_addr));

		in4_addr_my->sin_port = htons(port);

		ret = bind(fds[SOCK_ID_IPV4].fd,
			   (struct sockaddr *)in4_addr_my,
			   sizeof(struct sockaddr_in));
		if (ret < 0) {
			shell_fprintf(sh, SHELL_WARNING,
				      "Cannot bind IPv4 UDP port %d (%d)\n",
				      ntohs(in4_addr_my->sin_port),
				      errno);
			goto cleanup;
		}

		fds[SOCK_ID_IPV4].events = POLLIN;
	}

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		const struct in6_addr *in6_addr = NULL;

		in6_addr_my = zperf_get_sin6();

		fds[SOCK_ID_IPV6].fd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
		if (fds[SOCK_ID_IPV6].fd < 0) {
			shell_fprintf(sh, SHELL_WARNING,
				      "Cannot create IPv4 network socket.\n");
			goto cleanup;
		}

		if (MY_IP6ADDR && strlen(MY_IP6ADDR)) {
			/* Use setting IP */
			ret = zperf_get_ipv6_addr(sh, MY_IP6ADDR,
						  MY_PREFIX_LEN_STR,
						  &in6_addr_my->sin6_addr);
			if (ret < 0) {
				shell_fprintf(sh, SHELL_WARNING,
					      "Unable to set IPv6\n");
				goto use_existing_ipv6;
			}
		} else {
		use_existing_ipv6:
			/* Use existing IP */
			in6_addr = zperf_get_default_if_in6_addr();
			if (!in6_addr) {
				shell_fprintf(sh, SHELL_WARNING,
					      "Unable to get IPv4 by default\n");
				goto cleanup;
			}
			memcpy(&in6_addr_my->sin6_addr, in6_addr,
				sizeof(struct in6_addr));
		}

		shell_fprintf(sh, SHELL_NORMAL, "Binding to %s\n",
			      net_sprint_ipv6_addr(&in6_addr_my->sin6_addr));

		in6_addr_my->sin6_port = htons(port);

		ret = bind(fds[SOCK_ID_IPV6].fd,
			   (struct sockaddr *)in6_addr_my,
			   sizeof(struct sockaddr_in6));
		if (ret < 0) {
			shell_fprintf(sh, SHELL_WARNING,
				      "Cannot bind IPv6 UDP port %d (%d)\n",
				      ntohs(in6_addr_my->sin6_port),
				      ret);
			goto cleanup;
		}

		fds[SOCK_ID_IPV6].events = POLLIN;
	}

	shell_fprintf(sh, SHELL_NORMAL,
		      "Listening on port %d\n", port);

	while (true) {
		ret = poll(fds, ARRAY_SIZE(fds), -1);
		if (ret < 0) {
			shell_fprintf(sh, SHELL_WARNING,
				      "UDP receiver poll error (%d)\n",
				      errno);
			goto cleanup;
		}

		for (int i = 0; i < ARRAY_SIZE(fds); i++) {
			struct sockaddr addr;
			socklen_t addrlen = sizeof(addr);

			if ((fds[i].revents & POLLERR) ||
			    (fds[i].revents & POLLNVAL)) {
				shell_fprintf(
					sh, SHELL_WARNING,
					"UDP receiver IPv%d socket error\n",
					(i == SOCK_ID_IPV4) ? 4 : 6);
				goto cleanup;
			}

			if (!(fds[i].revents & POLLIN)) {
				continue;
			}

			ret = recvfrom(fds[i].fd, buf, sizeof(buf), 0, &addr,
				       &addrlen);
			if (ret < 0) {
				shell_fprintf(
					sh, SHELL_WARNING,
					"recv failed on IPv%d socket (%d)\n",
					(i == SOCK_ID_IPV4) ? 4 : 6, errno);
				goto cleanup;
			}

			udp_received(sh, fds[i].fd, &addr, buf, ret);
		}
	}

cleanup:
	for (int i = 0; i < ARRAY_SIZE(fds); i++) {
		if (fds[i].fd >= 0) {
			close(fds[i].fd);
		}
	}
}

void zperf_udp_receiver_init(const struct shell *sh, int port)
{
	k_thread_create(&udp_receiver_thread_data,
			udp_receiver_stack_area,
			K_THREAD_STACK_SIZEOF(udp_receiver_stack_area),
			udp_receiver_thread,
			(void *)sh, INT_TO_POINTER(port), NULL,
			UDP_RECEIVER_THREAD_PRIORITY,
			IS_ENABLED(CONFIG_USERSPACE) ? K_USER |
						       K_INHERIT_PERMS : 0,
			K_NO_WAIT);
}
