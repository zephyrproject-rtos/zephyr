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
#include <zephyr/net/socket_service.h>
#include <zephyr/net/zperf.h>

#include "zperf_internal.h"
#include "zperf_session.h"

/* To get net_sprint_ipv{4|6}_addr() */
#define NET_LOG_ENABLED 1
#include "net_private.h"

#define SOCK_ID_IPV4_LISTEN 0
#define SOCK_ID_IPV6_LISTEN 1
#define SOCK_ID_MAX         (CONFIG_NET_ZPERF_MAX_SESSIONS + 2)

#define TCP_RECEIVER_BUF_SIZE 1500

static zperf_callback tcp_session_cb;
static void *tcp_user_data;
static bool tcp_server_running;
static uint16_t tcp_server_port;
static struct sockaddr tcp_server_addr;

static struct zsock_pollfd fds[SOCK_ID_MAX];
static struct sockaddr sock_addr[SOCK_ID_MAX];

static void tcp_svc_handler(struct k_work *work);

NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(svc_tcp, NULL, tcp_svc_handler,
				      SOCK_ID_MAX);

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
			results.time_in_us = k_ticks_to_us_ceil64(
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

static void tcp_session_error_report(void)
{
	if (tcp_session_cb != NULL) {
		tcp_session_cb(ZPERF_SESSION_ERROR, NULL, tcp_user_data);
	}
}

static void tcp_receiver_cleanup(void)
{
	int i;

	(void)net_socket_service_unregister(&svc_tcp);

	for (i = 0; i < ARRAY_SIZE(fds); i++) {
		if (fds[i].fd >= 0) {
			zsock_close(fds[i].fd);
			fds[i].fd = -1;
			memset(&sock_addr[i], 0, sizeof(struct sockaddr));
		}
	}

	tcp_server_running = false;
	tcp_session_cb = NULL;

	zperf_session_reset(SESSION_TCP);
}

static int tcp_recv_data(struct net_socket_service_event *pev)
{
	static uint8_t buf[TCP_RECEIVER_BUF_SIZE];
	int i, ret = 0;
	int family, sock, sock_error;
	struct sockaddr addr_incoming_conn;
	socklen_t optlen = sizeof(int);
	socklen_t addrlen = sizeof(struct sockaddr);

	if (!tcp_server_running) {
		return -ENOENT;
	}

	if ((pev->event.revents & ZSOCK_POLLERR) ||
	    (pev->event.revents & ZSOCK_POLLNVAL)) {
		(void)zsock_getsockopt(pev->event.fd, SOL_SOCKET,
				       SO_DOMAIN, &family, &optlen);
		(void)zsock_getsockopt(pev->event.fd, SOL_SOCKET,
				       SO_ERROR, &sock_error, &optlen);
		NET_ERR("TCP receiver IPv%d socket error (%d)",
			family == AF_INET ? 4 : 6, sock_error);
		ret = -sock_error;
		goto error;
	}

	if (!(pev->event.revents & ZSOCK_POLLIN)) {
		return 0;
	}

	/* What is the index to first accepted socket */
	i = SOCK_ID_IPV6_LISTEN + 1;

	/* Check first if we need to accept a connection */
	if (fds[SOCK_ID_IPV4_LISTEN].fd == pev->event.fd ||
	    fds[SOCK_ID_IPV6_LISTEN].fd == pev->event.fd) {
		sock = zsock_accept(pev->event.fd,
				    &addr_incoming_conn,
				    &addrlen);
		if (sock < 0) {
			ret = -errno;
			(void)zsock_getsockopt(pev->event.fd, SOL_SOCKET,
					       SO_DOMAIN, &family, &optlen);
			NET_ERR("TCP receiver IPv%d accept error (%d)",
				family == AF_INET ? 4 : 6, ret);
			goto error;
		}

		for (; i < SOCK_ID_MAX; i++) {
			if (fds[i].fd < 0) {
				break;
			}
		}

		if (i == SOCK_ID_MAX) {
			/* Too many connections. */
			NET_ERR("Dropping TCP connection, reached maximum limit.");
			zsock_close(sock);
		} else {
			fds[i].fd = sock;
			fds[i].events = ZSOCK_POLLIN;
			memcpy(&sock_addr[i], &addr_incoming_conn, addrlen);

			(void)net_socket_service_register(&svc_tcp, fds,
							  ARRAY_SIZE(fds),
							  NULL);
		}

	} else {
		ret = zsock_recv(pev->event.fd, buf, sizeof(buf), 0);
		if (ret < 0) {
			(void)zsock_getsockopt(pev->event.fd, SOL_SOCKET,
					       SO_DOMAIN, &family, &optlen);
			NET_ERR("recv failed on IPv%d socket (%d)",
				family == AF_INET ? 4 : 6,
				errno);
			tcp_session_error_report();
			/* This will close the zperf session */
			ret = 0;
		}

		for (; i < SOCK_ID_MAX; i++) {
			if (fds[i].fd == pev->event.fd) {
				break;
			}
		}

		if (i == SOCK_ID_MAX) {
			NET_ERR("Descriptor %d not found.", pev->event.fd);
		} else {
			tcp_received(&sock_addr[i], ret);
			if (ret == 0) {
				zsock_close(fds[i].fd);
				fds[i].fd = -1;
				memset(&sock_addr[i], 0, sizeof(struct sockaddr));

				(void)net_socket_service_register(&svc_tcp, fds,
								  ARRAY_SIZE(fds),
								  NULL);
			}
		}
	}

	return ret;

error:
	tcp_session_error_report();

	return ret;
}

static void tcp_svc_handler(struct k_work *work)
{
	struct net_socket_service_event *pev =
		CONTAINER_OF(work, struct net_socket_service_event, work);
	int ret;

	ret = tcp_recv_data(pev);
	if (ret < 0) {
		tcp_receiver_cleanup();
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
			address->sa_family == AF_INET ? 4 : 6, port, errno);
		goto out;
	}

	ret = zsock_listen(pollfd->fd, 1);
	if (ret < 0) {
		NET_ERR("Cannot listen IPv%d TCP (%d)",
			address->sa_family == AF_INET ? 4 : 6, errno);
		goto out;
	}

	pollfd->events = ZSOCK_POLLIN;

out:
	return ret;
}

static int zperf_tcp_receiver_init(void)
{
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
			ret = -errno;
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
				goto use_any_ipv4;
			}
		} else {
use_any_ipv4:
			in4_addr->sin_addr.s_addr = INADDR_ANY;
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
			ret = -errno;
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
				goto use_any_ipv6;
			}
		} else {
use_any_ipv6:
			memcpy(&in6_addr->sin6_addr, net_ipv6_unspecified_address(),
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

	ret = net_socket_service_register(&svc_tcp, fds,
					  ARRAY_SIZE(fds), NULL);
	if (ret < 0) {
		LOG_ERR("Cannot register socket service handler (%d)", ret);
	}

error:
	return ret;
}

int zperf_tcp_download(const struct zperf_download_params *param,
		       zperf_callback callback, void *user_data)
{
	int ret;

	if (param == NULL || callback == NULL) {
		return -EINVAL;
	}

	if (tcp_server_running) {
		return -EALREADY;
	}

	tcp_session_cb = callback;
	tcp_user_data = user_data;
	tcp_server_port = param->port;
	memcpy(&tcp_server_addr, &param->addr, sizeof(struct sockaddr));

	ret = zperf_tcp_receiver_init();
	if (ret < 0) {
		tcp_receiver_cleanup();
		return ret;
	}

	tcp_server_running = true;

	return 0;
}

int zperf_tcp_download_stop(void)
{
	if (!tcp_server_running) {
		return -EALREADY;
	}

	tcp_receiver_cleanup();

	return 0;
}
