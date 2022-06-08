/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_zperf_sample, LOG_LEVEL_DBG);

#include <zephyr/zephyr.h>

#include <zephyr/linker/sections.h>
#include <zephyr/toolchain.h>

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

static bool init_done;

#if IS_ENABLED(CONFIG_NET_TC_THREAD_COOPERATIVE)
#define TCP_RECEIVER_THREAD_PRIORITY K_PRIO_COOP(8)
#else
#define TCP_RECEIVER_THREAD_PRIORITY K_PRIO_PREEMPT(8)
#endif

#define TCP_RECEIVER_STACK_SIZE 2048

#define SOCK_ID_IPV4_LISTEN 0
#define SOCK_ID_IPV4_DATA 1
#define SOCK_ID_IPV6_LISTEN 2
#define SOCK_ID_IPV6_DATA 3
#define SOCK_ID_MAX 4

#define TCP_RECEIVER_BUF_SIZE 1500

K_THREAD_STACK_DEFINE(tcp_receiver_stack_area, TCP_RECEIVER_STACK_SIZE);
struct k_thread tcp_receiver_thread_data;

static void tcp_received(const struct shell *sh, int sock, size_t datalen)
{
	struct session *session;
	int64_t time;

	time = k_uptime_ticks();

	session = get_tcp_session(sock);
	if (!session) {
		shell_fprintf(sh, SHELL_WARNING, "Cannot get a session!\n");
		return;
	}

	switch (session->state) {
	case STATE_COMPLETED:
		break;
	case STATE_NULL:
		shell_fprintf(sh, SHELL_NORMAL,
			      "New TCP session started\n");
		zperf_reset_session_stats(session);
		session->start_time = k_uptime_ticks();
		session->state = STATE_ONGOING;
		__fallthrough;
	case STATE_ONGOING:
		session->counter++;
		session->length += datalen;

		if (datalen == 0) { /* EOF */
			uint32_t rate_in_kbps;
			uint32_t duration;

			duration = k_ticks_to_us_ceil32(time -
							session->start_time);

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

			shell_fprintf(sh, SHELL_NORMAL,
				      "TCP session ended\n");

			shell_fprintf(sh, SHELL_NORMAL,
				      " Duration:\t\t");
			print_number(sh, duration, TIME_US, TIME_US_UNIT);
			shell_fprintf(sh, SHELL_NORMAL, "\n");

			shell_fprintf(sh, SHELL_NORMAL, " rate:\t\t\t");
			print_number(sh, rate_in_kbps, KBPS, KBPS_UNIT);
			shell_fprintf(sh, SHELL_NORMAL, "\n");

			zperf_tcp_stopped();

			session->state = STATE_NULL;
		}


		break;
	case STATE_LAST_PACKET_RECEIVED:
		break;
	default:
		shell_fprintf(sh, SHELL_WARNING, "Unsupported case\n");
	}
}

void tcp_receiver_thread(void *ptr1, void *ptr2, void *ptr3)
{
	ARG_UNUSED(ptr3);

	static uint8_t buf[TCP_RECEIVER_BUF_SIZE];
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

		fds[SOCK_ID_IPV4_LISTEN].fd = socket(AF_INET, SOCK_STREAM,
						     IPPROTO_TCP);
		if (fds[SOCK_ID_IPV4_LISTEN].fd < 0) {
			shell_fprintf(sh, SHELL_WARNING,
				      "Cannot create IPv4 network socket.\n");
			goto cleanup;
		}

		if (MY_IP4ADDR && strlen(MY_IP4ADDR)) {
			/* Use Setting IP */
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
				return;
			}
			memcpy(&in4_addr_my->sin_addr, in4_addr,
				sizeof(struct in_addr));
		}

		in4_addr_my->sin_port = htons(port);

		shell_fprintf(sh, SHELL_NORMAL, "Binding to %s\n",
			      net_sprint_ipv4_addr(&in4_addr_my->sin_addr));

		ret = bind(fds[SOCK_ID_IPV4_LISTEN].fd,
			   (struct sockaddr *)in4_addr_my,
			   sizeof(struct sockaddr_in));
		if (ret < 0) {
			shell_fprintf(sh, SHELL_WARNING,
				      "Cannot bind IPv4 UDP port %d (%d)\n",
				      ntohs(in4_addr_my->sin_port),
				      errno);
			goto cleanup;
		}

		ret = listen(fds[SOCK_ID_IPV4_LISTEN].fd, 1);
		if (ret < 0) {
			shell_fprintf(sh, SHELL_WARNING,
				      "Cannot listen IPv4 TCP (%d)", errno);
			goto cleanup;
		}

		fds[SOCK_ID_IPV4_LISTEN].events = POLLIN;
	}

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		const struct in6_addr *in6_addr = NULL;

		in6_addr_my = zperf_get_sin6();

		fds[SOCK_ID_IPV6_LISTEN].fd = socket(AF_INET6, SOCK_STREAM,
						     IPPROTO_TCP);
		if (fds[SOCK_ID_IPV6_LISTEN].fd < 0) {
			shell_fprintf(sh, SHELL_WARNING,
				      "Cannot create IPv6 network socket.\n");
			goto cleanup;
		}

		if (MY_IP6ADDR && strlen(MY_IP6ADDR)) {
			/* Use Setting IP */
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
				return;
			}
			memcpy(&in6_addr_my->sin6_addr, in6_addr,
				sizeof(struct in6_addr));
		}

		in6_addr_my->sin6_port = htons(port);

		shell_fprintf(sh, SHELL_NORMAL, "Binding to %s\n",
			      net_sprint_ipv6_addr(&in6_addr_my->sin6_addr));

		ret = bind(fds[SOCK_ID_IPV6_LISTEN].fd,
			   (struct sockaddr *)in6_addr_my,
			   sizeof(struct sockaddr_in6));
		if (ret < 0) {
			shell_fprintf(sh, SHELL_WARNING,
				      "Cannot bind IPv6 UDP port %d (%d)\n",
				      ntohs(in6_addr_my->sin6_port),
				      errno);
			goto cleanup;
		}

		ret = listen(fds[SOCK_ID_IPV6_LISTEN].fd, 1);
		if (ret < 0) {
			shell_fprintf(sh, SHELL_WARNING,
				      "Cannot listen IPv6 TCP (%d)", errno);
			goto cleanup;
		}

		fds[SOCK_ID_IPV6_LISTEN].events = POLLIN;
	}

	shell_fprintf(sh, SHELL_NORMAL,
		      "Listening on port %d\n", port);

	/* TODO Investigate started/stopped logic */
	zperf_tcp_started();
	init_done = true;

	while (true) {
		ret = poll(fds, ARRAY_SIZE(fds), -1);
		if (ret < 0) {
			shell_fprintf(sh, SHELL_WARNING,
				      "TCP receiver poll error (%d)\n",
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
					"TCP receiver IPv%d socket error\n",
					(i <= SOCK_ID_IPV4_DATA) ? 4 : 6);
				goto cleanup;
			}

			if (!(fds[i].revents & POLLIN)) {
				continue;
			}

			switch (i) {
			case SOCK_ID_IPV4_LISTEN:
			case SOCK_ID_IPV6_LISTEN:{
				int sock = accept(fds[i].fd, &addr, &addrlen);

				if (sock < 0) {
					shell_fprintf(
						sh, SHELL_WARNING,
						"TCP receiver IPv%d accept error\n",
						(i <= SOCK_ID_IPV4_DATA) ? 4 : 6);
					goto cleanup;
				}

				if (i == SOCK_ID_IPV4_LISTEN &&
				    fds[SOCK_ID_IPV4_DATA].fd < 0) {
					fds[SOCK_ID_IPV4_DATA].fd = sock;
					fds[SOCK_ID_IPV4_DATA].events = POLLIN;
				} else if (i == SOCK_ID_IPV6_LISTEN &&
					   fds[SOCK_ID_IPV6_DATA].fd < 0) {
					fds[SOCK_ID_IPV6_DATA].fd = sock;
					fds[SOCK_ID_IPV6_DATA].events = POLLIN;
				} else {
					/* Too many connections. */
					close(sock);
					break;
				}

				break;
			}

			case SOCK_ID_IPV4_DATA:
			case SOCK_ID_IPV6_DATA:
				ret = recv(fds[i].fd, buf, sizeof(buf), 0);
				if (ret < 0) {
					shell_fprintf(
						sh, SHELL_WARNING,
						"recv failed on IPv%d socket (%d)\n",
						(i <= SOCK_ID_IPV4_DATA) ? 4 : 6,
						errno);
					goto cleanup;
				}

				tcp_received(sh, fds[i].fd, ret);

				if (ret == 0) {
					close(fds[i].fd);
					fds[i].fd = -1;
				}

				break;
			}
		}
	}

cleanup:
	for (int i = 0; i < ARRAY_SIZE(fds); i++) {
		if (fds[i].fd >= 0) {
			close(fds[i].fd);
		}
	}
}

void zperf_tcp_receiver_init(const struct shell *sh, int port)
{
	if (init_done) {
		zperf_tcp_started();
		return;
	}

	k_thread_create(&tcp_receiver_thread_data,
			tcp_receiver_stack_area,
			K_THREAD_STACK_SIZEOF(tcp_receiver_stack_area),
			tcp_receiver_thread,
			(void *)sh, INT_TO_POINTER(port), NULL,
			TCP_RECEIVER_THREAD_PRIORITY,
			IS_ENABLED(CONFIG_USERSPACE) ? K_USER |
						       K_INHERIT_PERMS : 0,
			K_NO_WAIT);
}
