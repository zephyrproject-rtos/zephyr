/*
 * Copyright (c) 2015 Intel Corporation
 * Copyright (c) 2023 Arm Limited (or its affiliates). All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_zperf, CONFIG_NET_ZPERF_LOG_LEVEL);

#include <zephyr/kernel.h>

#include <zephyr/linker/sections.h>
#include <zephyr/toolchain.h>

#include <zephyr/net/socket.h>
#include <zephyr/net/zperf.h>

#include "zperf_internal.h"
#include "zperf_session.h"

/* To get net_sprint_ipv{4|6}_addr() */
#define NET_LOG_ENABLED 1
#include "net_private.h"

#if defined(CONFIG_NET_TC_THREAD_COOPERATIVE)
#define TCP_RECEIVER_THREAD_PRIORITY K_PRIO_COOP(8)
#else
#define TCP_RECEIVER_THREAD_PRIORITY K_PRIO_PREEMPT(8)
#endif

#define TCP_RECEIVER_STACK_SIZE 2048

#define SOCK_ID_IPV4_LISTEN 0
#define SOCK_ID_IPV6_LISTEN 1
#define SOCK_ID_MAX         (CONFIG_NET_ZPERF_MAX_SESSIONS + 2)

#define TCP_RECEIVER_BUF_SIZE 1500
#define POLL_TIMEOUT_MS 100

static K_THREAD_STACK_DEFINE(tcp_receiver_stack_area, TCP_RECEIVER_STACK_SIZE);
static struct k_thread tcp_receiver_thread_data;

static zperf_callback tcp_session_cb;
static void *tcp_user_data;
static bool tcp_server_running;
static bool tcp_server_stop;
static uint16_t tcp_server_port;
static struct sockaddr tcp_server_addr;
static K_SEM_DEFINE(tcp_server_run, 0, 1);

static void tcp_received(const struct sockaddr *addr, size_t datalen)
{
	struct session *session;
	int64_t time;

	time = k_uptime_ticks();

	session = get_session(addr, SESSION_TCP);
	if (!session) {
		NET_ERR("Cannot get a session!");
		return;
	}

	switch (session->state) {
	case STATE_COMPLETED:
	case STATE_NULL:
		zperf_reset_session_stats(session);
		session->start_time = k_uptime_ticks();
		session->state = STATE_ONGOING;

		if (tcp_session_cb != NULL) {
			tcp_session_cb(ZPERF_SESSION_STARTED, NULL,
				       tcp_user_data);
		}

		__fallthrough;
	case STATE_ONGOING:
		session->counter++;
		session->length += datalen;

		if (datalen == 0) { /* EOF */
			struct zperf_results results = { 0 };

			session->state = STATE_COMPLETED;

			results.total_len = session->length;
			results.time_in_us = k_ticks_to_us_ceil32(
						time - session->start_time);

			if (tcp_session_cb != NULL) {
				tcp_session_cb(ZPERF_SESSION_FINISHED, &results,
					       tcp_user_data);
			}
		}
		break;
	default:
		NET_ERR("Unsupported case");
	}
}

static int tcp_bind_listen_connection(struct zsock_pollfd *pollfd,
				      struct sockaddr *address)
{
	uint16_t port;
	int ret;

	if (address->sa_family == AF_INET) {
		port = ntohs(net_sin(address)->sin_port);
	} else {
		port = ntohs(net_sin6(address)->sin6_port);
	}

	ret = zsock_bind(pollfd->fd, address, sizeof(*address));
	if (ret < 0) {
		NET_ERR("Cannot bind IPv%d TCP port %d (%d)",
			(address->sa_family == AF_INET ? 4 : 6), port, errno);
		goto out;
	}

	ret = zsock_listen(pollfd->fd, 1);
	if (ret < 0) {
		NET_ERR("Cannot listen IPv%d TCP (%d)",
			(address->sa_family == AF_INET ? 4 : 6), errno);
		goto out;
	}

	pollfd->events = ZSOCK_POLLIN;

out:
	return ret;
}

static void tcp_session_error_report(void)
{
	if (tcp_session_cb != NULL) {
		tcp_session_cb(ZPERF_SESSION_ERROR, NULL, tcp_user_data);
	}
}

static void tcp_server_session(void)
{
	static uint8_t buf[TCP_RECEIVER_BUF_SIZE];
	static struct zsock_pollfd fds[SOCK_ID_MAX];
	static struct sockaddr sock_addr[SOCK_ID_MAX];
	int ret;

	for (int i = 0; i < ARRAY_SIZE(fds); i++) {
		fds[i].fd = -1;
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		struct sockaddr_in *in4_addr = zperf_get_sin();
		const struct in_addr *addr = NULL;

		fds[SOCK_ID_IPV4_LISTEN].fd = zsock_socket(AF_INET, SOCK_STREAM,
							   IPPROTO_TCP);
		if (fds[SOCK_ID_IPV4_LISTEN].fd < 0) {
			NET_ERR("Cannot create IPv4 network socket.");
			goto error;
		}

		addr = &net_sin(&tcp_server_addr)->sin_addr;

		if (!net_ipv4_is_addr_unspecified(addr)) {
			memcpy(&in4_addr->sin_addr, addr,
				sizeof(struct in_addr));
		} else if (strlen(MY_IP4ADDR ? MY_IP4ADDR : "")) {
			/* Use Setting IP */
			ret = zperf_get_ipv4_addr(MY_IP4ADDR,
						  &in4_addr->sin_addr);
			if (ret < 0) {
				NET_WARN("Unable to set IPv4");
				goto use_existing_ipv4;
			}
		} else {
use_existing_ipv4:
			/* Use existing IP */
			addr = zperf_get_default_if_in4_addr();
			if (!addr) {
				NET_ERR("Unable to get IPv4 by default");
				goto error;
			}
			memcpy(&in4_addr->sin_addr, addr,
				sizeof(struct in_addr));
		}

		in4_addr->sin_port = htons(tcp_server_port);

		NET_INFO("Binding to %s",
			 net_sprint_ipv4_addr(&in4_addr->sin_addr));

		memcpy(net_sin(&sock_addr[SOCK_ID_IPV4_LISTEN]), in4_addr,
		       sizeof(struct sockaddr_in));

		ret = tcp_bind_listen_connection(
				&fds[SOCK_ID_IPV4_LISTEN],
				&sock_addr[SOCK_ID_IPV4_LISTEN]);
		if (ret < 0) {
			goto error;
		}
	}

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		struct sockaddr_in6 *in6_addr = zperf_get_sin6();
		const struct in6_addr *addr = NULL;

		fds[SOCK_ID_IPV6_LISTEN].fd = zsock_socket(AF_INET6, SOCK_STREAM,
							   IPPROTO_TCP);
		if (fds[SOCK_ID_IPV6_LISTEN].fd < 0) {
			NET_ERR("Cannot create IPv6 network socket.");
			goto error;
		}

		addr = &net_sin6(&tcp_server_addr)->sin6_addr;

		if (!net_ipv6_is_addr_unspecified(addr)) {
			memcpy(&in6_addr->sin6_addr, addr,
			       sizeof(struct in6_addr));
		} else if (strlen(MY_IP6ADDR ? MY_IP6ADDR : "")) {
			/* Use Setting IP */
			ret = zperf_get_ipv6_addr(MY_IP6ADDR,
						  MY_PREFIX_LEN_STR,
						  &in6_addr->sin6_addr);
			if (ret < 0) {
				NET_WARN("Unable to set IPv6");
				goto use_existing_ipv6;
			}
		} else {
use_existing_ipv6:
			/* Use existing IP */
			addr = zperf_get_default_if_in6_addr();
			if (!addr) {
				NET_ERR("Unable to get IPv6 by default");
				goto error;
			}
			memcpy(&in6_addr->sin6_addr, addr,
				sizeof(struct in6_addr));
		}

		in6_addr->sin6_port = htons(tcp_server_port);

		NET_INFO("Binding to %s",
			 net_sprint_ipv6_addr(&in6_addr->sin6_addr));

		memcpy(net_sin6(&sock_addr[SOCK_ID_IPV6_LISTEN]), in6_addr,
		       sizeof(struct sockaddr_in6));

		ret = tcp_bind_listen_connection(
				&fds[SOCK_ID_IPV6_LISTEN],
				&sock_addr[SOCK_ID_IPV6_LISTEN]);
		if (ret < 0) {
			goto error;
		}
	}

	NET_INFO("Listening on port %d", tcp_server_port);

	while (true) {
		ret = zsock_poll(fds, ARRAY_SIZE(fds), POLL_TIMEOUT_MS);
		if (ret < 0) {
			NET_ERR("TCP receiver poll error (%d)", errno);
			goto error;
		}

		if (tcp_server_stop) {
			goto cleanup;
		}

		if (ret == 0) {
			continue;
		}

		for (int i = 0; i < ARRAY_SIZE(fds); i++) {
			if ((fds[i].revents & ZSOCK_POLLERR) ||
			    (fds[i].revents & ZSOCK_POLLNVAL)) {
				NET_ERR("TCP receiver IPv%d socket error",
					(sock_addr[i].sa_family == AF_INET
						? 4 : 6));
				goto error;
			}

			if (!(fds[i].revents & ZSOCK_POLLIN)) {
				continue;
			}

			if ((i >= SOCK_ID_IPV4_LISTEN) && (i <= SOCK_ID_IPV6_LISTEN)) {
				int j = SOCK_ID_IPV6_LISTEN + 1;
				struct sockaddr addr_incoming_conn;
				socklen_t addrlen = sizeof(struct sockaddr);
				int sock = zsock_accept(fds[i].fd,
							&addr_incoming_conn,
							&addrlen);

				if (sock < 0) {
					NET_ERR("TCP receiver IPv%d accept error",
						(sock_addr[i].sa_family == AF_INET
							? 4 : 6));
					goto error;
				}

				for (; j < SOCK_ID_MAX; j++) {
					if (fds[j].fd < 0) {
						break;
					}
				}

				if (j == SOCK_ID_MAX) {
					/* Too many connections. */
					NET_ERR("Dropping TCP connection, reached maximum limit.");
					zsock_close(sock);
				} else {
					fds[j].fd = sock;
					fds[j].events = ZSOCK_POLLIN;
					memcpy(&sock_addr[j],
					       &addr_incoming_conn,
					       addrlen);
				}
			} else if ((i > SOCK_ID_IPV6_LISTEN) && (i < SOCK_ID_MAX)) {
				ret = zsock_recv(fds[i].fd, buf, sizeof(buf), 0);
				if (ret < 0) {
					NET_ERR("recv failed on IPv%d socket (%d)",
						(sock_addr[i].sa_family == AF_INET
							? 4 : 6),
						errno);
					tcp_session_error_report();
					/* This will close the zperf session */
					ret = 0;
				}

				tcp_received(&sock_addr[i], ret);

				if (ret == 0) {
					zsock_close(fds[i].fd);
					fds[i].fd = -1;
					memset(&sock_addr[i], 0,
					sizeof(struct sockaddr));
				}
			} else {
				goto error;
			}
		}
	}

error:
	tcp_session_error_report();

cleanup:
	for (int i = 0; i < ARRAY_SIZE(fds); i++) {
		if (fds[i].fd >= 0) {
			zsock_close(fds[i].fd);
			memset(&sock_addr[i], 0, sizeof(struct sockaddr));
		}
	}
}

void tcp_receiver_thread(void *ptr1, void *ptr2, void *ptr3)
{
	ARG_UNUSED(ptr1);
	ARG_UNUSED(ptr2);
	ARG_UNUSED(ptr3);

	while (true) {
		k_sem_take(&tcp_server_run, K_FOREVER);

		tcp_server_session();

		tcp_server_running = false;
	}
}

void zperf_tcp_receiver_init(void)
{
	k_thread_create(&tcp_receiver_thread_data,
			tcp_receiver_stack_area,
			K_THREAD_STACK_SIZEOF(tcp_receiver_stack_area),
			tcp_receiver_thread,
			NULL, NULL, NULL,
			TCP_RECEIVER_THREAD_PRIORITY,
			IS_ENABLED(CONFIG_USERSPACE) ? K_USER |
						       K_INHERIT_PERMS : 0,
			K_NO_WAIT);
}

int zperf_tcp_download(const struct zperf_download_params *param,
		       zperf_callback callback, void *user_data)
{
	if (param == NULL || callback == NULL) {
		return -EINVAL;
	}

	if (tcp_server_running) {
		return -EALREADY;
	}

	tcp_session_cb = callback;
	tcp_user_data = user_data;
	tcp_server_port = param->port;
	tcp_server_running = true;
	tcp_server_stop = false;
	memcpy(&tcp_server_addr, &param->addr, sizeof(struct sockaddr));

	k_sem_give(&tcp_server_run);

	return 0;
}

int zperf_tcp_download_stop(void)
{
	if (!tcp_server_running) {
		return -EALREADY;
	}

	tcp_server_stop = true;
	tcp_session_cb = NULL;

	return 0;
}
