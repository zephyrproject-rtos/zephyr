/*
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_quic_echo_server_svc_sample, LOG_LEVEL_DBG);

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/posix/poll.h>
#include <zephyr/posix/arpa/inet.h>
#include <zephyr/posix/sys/socket.h>
#include <zephyr/net/socket_service.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/quic.h>

/* from samples/net/common/ */
#include "net_sample_common.h"
#include "quic_certificate.h"

#define MY_PORT 4422

#define MAX_SERVICES 1
#define ECHO_BUF_SIZE 1500
#define ECHO_QUEUE_SIZE 16
#define SENDER_THREAD_STACK_SIZE 4096
#define POLL_TIMEOUT_MS 5000

/* Echo buffer entry */
struct echo_entry {
	void *fifo_reserved;
	int stream_fd;
	int len;   /* len == -1 indicates FIN marker (close the stream) */
	char data[ECHO_BUF_SIZE];
};

K_MUTEX_DEFINE(lock);
K_FIFO_DEFINE(echo_fifo);
K_MEM_SLAB_DEFINE_STATIC(echo_slab, sizeof(struct echo_entry), ECHO_QUEUE_SIZE, 4);

static struct pollfd sockfd_conn;
static struct pollfd sockfd_stream[MAX_SERVICES];
static int stream_socket = -1;
static int connection_socket = -1;

static void receive_data(struct net_socket_service_event *pev);
static void quic_accept_handler(struct net_socket_service_event *pev);
static void restart_quic_echo_service(void);
static void conn_accept_handler(struct net_socket_service_event *pev);

static void stream_service_handler(struct net_socket_service_event *pev)
{
	receive_data(pev);
}

/* Service for handling incoming data on established QUIC stream */
NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(service_stream,
				      stream_service_handler,
				      MAX_SERVICES);

/* Service for handling incoming QUIC connections */
NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(service_quic, quic_accept_handler, 1);

/* Add new service */
NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(service_conn, conn_accept_handler, 1);

static void stream_list_init(void)
{
	k_mutex_lock(&lock, K_FOREVER);
	ARRAY_FOR_EACH_PTR(sockfd_stream, sockfd) {
		sockfd->fd = -1;
	}
	k_mutex_unlock(&lock);
}

static void stream_list_deinit(void)
{
	(void)net_socket_service_unregister(&service_stream);

	k_mutex_lock(&lock, K_FOREVER);
	ARRAY_FOR_EACH_PTR(sockfd_stream, sockfd) {
		if (sockfd->fd != -1) {
			close(sockfd->fd);
			sockfd->fd = -1;
		}
	}
	k_mutex_unlock(&lock);
}

static void stream_list_try_add(int sock)
{
	int ret;

	k_mutex_lock(&lock, K_FOREVER);
	ARRAY_FOR_EACH_PTR(sockfd_stream, sockfd) {
		if (sockfd->fd == -1) {
			sockfd->fd = sock;
			sockfd->events = POLLIN;

			ret = net_socket_service_register(&service_stream, sockfd_stream,
							  ARRAY_SIZE(sockfd_stream), NULL);
			if (ret < 0) {
				LOG_ERR("Cannot register socket service handler (%d). "
					"Attempting to restart service.", ret);
				restart_quic_echo_service();
			}

			stream_socket = sock;
			k_mutex_unlock(&lock);
			return;
		}
	}
	k_mutex_unlock(&lock);

	LOG_WRN("Max capacity reached");
	close(sock);
}

static void stream_list_remove(int stream)
{
	int ret;

	k_mutex_lock(&lock, K_FOREVER);
	ARRAY_FOR_EACH_PTR(sockfd_stream, sockfd) {
		if (sockfd->fd == stream) {
			close(stream);
			sockfd->fd = -1;
			ret = net_socket_service_register(&service_stream, sockfd_stream,
							  ARRAY_SIZE(sockfd_stream), NULL);
			if (ret < 0) {
				LOG_ERR("Cannot register socket service handler (%d). "
					"Attempting to restart service.", ret);
				restart_quic_echo_service();
			}

			stream_socket = -1;
			break;
		}
	}
	k_mutex_unlock(&lock);
}

static void receive_data(struct net_socket_service_event *pev)
{
	struct pollfd *pfd = &pev->event;
	int stream = pfd->fd;
	struct sockaddr_in6 addr;
	char addr_str[INET6_ADDRSTRLEN] = {0};
	socklen_t addrlen = sizeof(addr);
	struct echo_entry *entry;
	int len;

	/* Allocate entry from slab. If queue is full, sleep briefly to let
	 * the sender thread drain some entries. This provides natural
	 * backpressure: we don't call recv() while waiting, so flow control
	 * window doesn't advance, which tells the client to slow down.
	 */
	if (k_mem_slab_alloc(&echo_slab, (void **)&entry, K_NO_WAIT) != 0) {
		/* Sleep briefly to give sender thread time to drain entries
		 * and avoid spinning in a tight loop.
		 */
		k_sleep(K_MSEC(10));
		return;
	}

	len = recvfrom(stream, entry->data, sizeof(entry->data), 0,
		       (struct sockaddr *)&addr, &addrlen);
	if (len <= 0) {
		if (len < 0) {
			k_mem_slab_free(&echo_slab, entry);
			LOG_ERR("recv: %d", -errno);
			stream_list_remove(stream);
		} else {
			/* FIN received, queue a FIN marker so sender thread
			 * closes the stream after echoing all pending data
			 */
			LOG_INF("FIN received from peer, queuing close");
			(void)net_socket_service_unregister(&service_stream);
			entry->stream_fd = stream;
			entry->len = -1;  /* FIN marker */
			k_fifo_put(&echo_fifo, entry);
		}

		inet_ntop(addr.sin6_family, &addr.sin6_addr, addr_str, sizeof(addr_str));
		if (addr_str[0] != '\0') {
			LOG_INF("Connection from %s closed", addr_str);
		} else {
			LOG_INF("Connection closed");
		}

		return;
	}

	entry->stream_fd = stream;
	entry->len = len;

	k_fifo_put(&echo_fifo, entry);
}

/* Sender thread which processes echo queue */
static void echo_sender_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		struct echo_entry *entry;
		char *p;
		int len, out_len;

		entry = k_fifo_get(&echo_fifo, K_FOREVER);
		if (entry == NULL) {
			continue;
		}

		/* Check for FIN marker, close stream after all data echoed */
		if (entry->len == -1) {
			LOG_INF("Processing FIN marker, closing stream fd=%d",
				entry->stream_fd);
			stream_list_remove(entry->stream_fd);
			k_mem_slab_free(&echo_slab, entry);
			continue;
		}

		p = entry->data;
		len = entry->len;

		while (len > 0) {
			out_len = send(entry->stream_fd, p, len, 0);
			if (out_len < 0) {
				struct pollfd pfd;
				int ret;

				if (errno != EAGAIN && errno != EWOULDBLOCK) {
					LOG_ERR("send: %d", -errno);
					break;
				}

				/* TX buffer full, wait with poll */
				pfd.fd = entry->stream_fd;
				pfd.events = POLLOUT;

				ret = poll(&pfd, 1, POLL_TIMEOUT_MS);
				if (ret < 0) {
					LOG_ERR("poll: %d", -errno);
					break;
				}

				if (ret == 0) {
					LOG_WRN_RATELIMIT("TX poll timeout");
					/* Timeout, retry anyway in case of missed signal */
				}

				if (!(pfd.revents & POLLOUT)) {
					/* Poll returned but not for POLLOUT, check for errors */
					if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
						LOG_ERR("poll error: revents=0x%x", pfd.revents);
						break;
					}

					/* No POLLOUT yet, keep waiting */
					continue;
				}

				/* POLLOUT set, retry send */
				continue;
			}

			p += out_len;
			len -= out_len;
		}

		k_mem_slab_free(&echo_slab, entry);
	}
}

K_THREAD_DEFINE(echo_sender_tid, SENDER_THREAD_STACK_SIZE, echo_sender_thread,
		NULL, NULL, NULL, 7, 0, 0);

/* Does the second accept() to get the stream fd */
static void conn_accept_handler(struct net_socket_service_event *pev)
{
	int stream;
	struct sockaddr_in6 peer_addr;
	socklen_t addrlen = sizeof(peer_addr);
	int conn_fd = pev->event.fd;

	stream = accept(conn_fd, (struct sockaddr *)&peer_addr, &addrlen);
	if (stream < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			/* Connection closed, clean up */
			LOG_INF("Connection fd=%d closed", conn_fd);
			(void)net_socket_service_unregister(&service_conn);
			close(conn_fd);
			k_mutex_lock(&lock, K_FOREVER);
			connection_socket = -1;
			k_mutex_unlock(&lock);
		} else {
			LOG_ERR("stream accept: %d", -errno);
		}
		return;
	}

	LOG_INF("Stream fd=%d accepted on conn fd=%d", stream, conn_fd);
	stream_list_try_add(stream);
}

static void quic_accept_handler(struct net_socket_service_event *pev)
{
	int conn_fd;
	char addr_str[INET6_ADDRSTRLEN];
	struct sockaddr_in6 client_addr;
	socklen_t client_addr_len = sizeof(client_addr);

	conn_fd = accept(pev->event.fd, (struct sockaddr *)&client_addr, &client_addr_len);
	if (conn_fd < 0) {
		LOG_ERR("accept: %d. Restarting service.", -errno);
		restart_quic_echo_service();
		return;
	}

	inet_ntop(client_addr.sin6_family, &client_addr.sin6_addr, addr_str, sizeof(addr_str));
	LOG_INF("Connection from %s (conn fd=%d)", addr_str, conn_fd);

	k_mutex_lock(&lock, K_FOREVER);
	connection_socket = conn_fd;
	k_mutex_unlock(&lock);

	sockfd_conn = (struct pollfd){ .fd = conn_fd, .events = POLLIN };
	net_socket_service_register(&service_conn, &sockfd_conn, 1, NULL);
}

static int setup_quic_socket(struct sockaddr_in6 *addr)
{
	int sock;

	sec_tag_t sec_tag_list[] = {
		QUIC_SERVER_CERTIFICATE_TAG,
	};

	static const char * const alpn_list[] = {
		"echo-quic",
		NULL
	};

	/* Common socket setup for QUIC can be found in samples/net/common/quic.c */
	sock = setup_quic(NULL, (struct sockaddr *)addr, QUIC_STREAM_SERVER,
			  sec_tag_list, sizeof(sec_tag_list),
			  (const char **)alpn_list, sizeof(alpn_list));
	if (sock < 0) {
		LOG_ERR("socket: %d", -errno);
		return -errno;
	}

	return sock;
}

static void init_tls_credentials(void)
{
	int ret;

	ret = tls_credential_add(QUIC_SERVER_CERTIFICATE_TAG,
				 TLS_CREDENTIAL_PUBLIC_CERTIFICATE,
				 quic_server_certificate,
				 sizeof(quic_server_certificate));
	if (ret < 0 && ret != -EALREADY && ret != -EEXIST) {
		LOG_ERR("Failed to register public certificate: %d", ret);
	}


	ret = tls_credential_add(QUIC_SERVER_CERTIFICATE_TAG,
				 TLS_CREDENTIAL_PRIVATE_KEY,
				 quic_server_private_key,
				 sizeof(quic_server_private_key));
	if (ret < 0 && ret != -EALREADY && ret != -EEXIST) {
		LOG_ERR("Failed to register private key: %d", ret);
	}
}

static int start_quic_echo_service(void)
{
	struct pollfd sockfd_accept;
	int connection_sock, ret;
	struct sockaddr_in6 addr = {
		.sin6_family = AF_INET6,
		.sin6_addr = IN6ADDR_ANY_INIT,
		.sin6_port = htons(MY_PORT),
	};

	init_tls_credentials();
	stream_list_init();

	connection_sock = setup_quic_socket(&addr);
	if (connection_sock < 0) {
		LOG_ERR("Failed to setup QUIC listening socket");
		return connection_sock;
	}

	/* Register QUIC listening socket to service handler */
	sockfd_accept = (struct pollfd){ .fd = connection_sock, .events = POLLIN };
	ret = net_socket_service_register(&service_quic, &sockfd_accept, 1, NULL);
	if (ret < 0) {
		LOG_ERR("Cannot register socket service handler (%d)", ret);
		goto cleanup;
	}

	k_mutex_lock(&lock, K_FOREVER);
	connection_socket = connection_sock;
	k_mutex_unlock(&lock);

	LOG_INF("Single-threaded QUIC echo server waits "
		"for a connection on port %d", MY_PORT);

	return 0;

cleanup:
	close(connection_sock);

	return -1;
}

static int stop_quic_echo_service(void)
{
	stream_list_deinit();

	(void)net_socket_service_unregister(&service_quic);
	(void)net_socket_service_unregister(&service_stream);
	(void)net_socket_service_unregister(&service_conn);

	k_mutex_lock(&lock, K_FOREVER);

	if (stream_socket >= 0) {
		close(stream_socket);
		stream_socket = -1;
	}

	if (connection_socket >= 0) {
		close(connection_socket);
		connection_socket = -1;
	}

	k_mutex_unlock(&lock);

	return 0;
}

static void restart_quic_echo_service(void)
{
	stop_quic_echo_service();
	start_quic_echo_service();
}

SYS_INIT(start_quic_echo_service, APPLICATION, 99);
