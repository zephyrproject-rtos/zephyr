/* tcp.c - TCP specific code for echo server */

/*
 * Copyright (c) 2017 Intel Corporation.
 * Copyright (c) 2018 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_DECLARE(net_echo_server_sample, LOG_LEVEL_DBG);

#include <zephyr.h>
#include <errno.h>
#include <stdio.h>

#include <net/socket.h>
#include <net/tls_credentials.h>

#include "common.h"
#include "certificate.h"

#define MAX_CLIENT_QUEUE 1

static void process_tcp4(void);
static void process_tcp6(void);

K_THREAD_DEFINE(tcp4_thread_id, STACK_SIZE,
		process_tcp4, NULL, NULL, NULL,
		THREAD_PRIORITY, 0, K_FOREVER);

K_THREAD_DEFINE(tcp6_thread_id, STACK_SIZE,
		process_tcp6, NULL, NULL, NULL,
		THREAD_PRIORITY, 0, K_FOREVER);

static ssize_t sendall(int sock, const void *buf, size_t len)
{
	while (len) {
		ssize_t out_len = send(sock, buf, len, 0);

		if (out_len < 0) {
			return out_len;
		}
		buf = (const char *)buf + out_len;
		len -= out_len;
	}

	return 0;
}

static int start_tcp_proto(struct data *data, struct sockaddr *bind_addr,
			   socklen_t bind_addrlen)
{
	int ret;

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	data->tcp.sock = socket(bind_addr->sa_family, SOCK_STREAM,
				IPPROTO_TLS_1_2);
#else
	data->tcp.sock = socket(bind_addr->sa_family, SOCK_STREAM, IPPROTO_TCP);
#endif
	if (data->tcp.sock < 0) {
		LOG_ERR("Failed to create TCP socket (%s): %d", data->proto,
			errno);
		return -errno;
	}

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	sec_tag_t sec_tag_list[] = {
		SERVER_CERTIFICATE_TAG,
	};

	ret = setsockopt(data->tcp.sock, SOL_TLS, TLS_SEC_TAG_LIST,
			 sec_tag_list, sizeof(sec_tag_list));
	if (ret < 0) {
		LOG_ERR("Failed to set TCP secure option (%s): %d", data->proto,
			errno);
		ret = -errno;
	}
#endif

	ret = bind(data->tcp.sock, bind_addr, bind_addrlen);
	if (ret < 0) {
		LOG_ERR("Failed to bind TCP socket (%s): %d", data->proto,
			errno);
		return -errno;
	}

	ret = listen(data->tcp.sock, MAX_CLIENT_QUEUE);
	if (ret < 0) {
		LOG_ERR("Failed to listen on TCP socket (%s): %d", data->proto,
			errno);
		ret = -errno;
	}

	return ret;
}

static int process_tcp(struct data *data)
{
	int ret = 0;
	int client;
	int received;
	int offset = 0;
	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);

	LOG_INF("Waiting for TCP connection on port %d (%s)...",
		MY_PORT, data->proto);

	client = accept(data->tcp.sock, (struct sockaddr *)&client_addr,
			&client_addr_len);
	if (client < 0) {
		LOG_ERR("Error in accept (%s): %d - stopping server",
			data->proto, errno);
		return -errno;
	}

	LOG_INF("TCP (%s): Accepted connection", data->proto);

	do {
		received = recv(client, data->tcp.recv_buffer + offset,
				sizeof(data->tcp.recv_buffer) - offset, 0);

		if (received == 0) {
			/* Connection closed */
			LOG_INF("TCP (%s): Connection closed", data->proto);
			ret = 0;
			break;
		} else if (received < 0) {
			/* Socket error */
			LOG_ERR("TCP (%s): Connection error %d", data->proto,
				errno);
			ret = -errno;
			break;
		}

		offset += received;

#if !defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
		/* To prevent fragmentation of the response, reply only if
		 * buffer is full or there is no more data to read
		 */
		if (offset == sizeof(data->tcp.recv_buffer) ||
		    (recv(client, data->tcp.recv_buffer  + offset,
			  sizeof(data->tcp.recv_buffer)  - offset,
			  MSG_PEEK | MSG_DONTWAIT) < 0 &&
		     (errno == EAGAIN || errno == EWOULDBLOCK))) {
#endif
			ret = sendall(client, data->tcp.recv_buffer, offset);
			if (ret < 0) {
				LOG_ERR("TCP (%s): Failed to send, "
					"closing socket", data->proto);
				ret = 0;
				break;
			}

			LOG_DBG("TCP (%s): Received and replied with %d bytes",
				data->proto, offset);

			if (++data->tcp.counter % 1000 == 0U) {
				LOG_INF("%s TCP: Sent %u packets", data->proto,
					data->tcp.counter);
			}

			offset = 0;
#if !defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
		}
#endif
	} while (true);

	close(client);

	return ret;
}

static void process_tcp4(void)
{
	int ret;
	struct sockaddr_in addr4;

	(void)memset(&addr4, 0, sizeof(addr4));
	addr4.sin_family = AF_INET;
	addr4.sin_port = htons(MY_PORT);

	ret = start_tcp_proto(&conf.ipv4, (struct sockaddr *)&addr4,
			      sizeof(addr4));
	if (ret < 0) {
		quit();
		return;
	}

	while (ret == 0) {
		ret = process_tcp(&conf.ipv4);
		if (ret < 0) {
			quit();
		}
	}
}

static void process_tcp6(void)
{
	int ret;
	struct sockaddr_in6 addr6;

	(void)memset(&addr6, 0, sizeof(addr6));
	addr6.sin6_family = AF_INET6;
	addr6.sin6_port = htons(MY_PORT);

	ret = start_tcp_proto(&conf.ipv6, (struct sockaddr *)&addr6,
			      sizeof(addr6));
	if (ret < 0) {
		quit();
		return;
	}

	while (ret == 0) {
		ret = process_tcp(&conf.ipv6);
		if (ret != 0) {
			quit();
		}
	}
}

void start_tcp(void)
{
	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		k_thread_start(tcp6_thread_id);
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		k_thread_start(tcp4_thread_id);
	}
}

void stop_tcp(void)
{
	/* Not very graceful way to close a thread, but as we may be blocked
	 * in accept or recv call it seems to be necessary
	 */
	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		k_thread_abort(tcp6_thread_id);
		if (conf.ipv6.tcp.sock > 0) {
			(void)close(conf.ipv6.tcp.sock);
		}
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		k_thread_abort(tcp4_thread_id);
		if (conf.ipv4.tcp.sock > 0) {
			(void)close(conf.ipv4.tcp.sock);
		}
	}
}
