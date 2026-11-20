/* SPDX-FileCopyrightText: Copyright (c) 2026 Meta Platforms, Inc. */
/* SPDX-License-Identifier: Apache-2.0 */
#include "signaling_test_server.h"

#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include "platform.h"
#include "signaling_client_zephyr.h"

#define SERVER_REQUEST_HEADER_BYTES 2048U
#define SERVER_ACCEPT_TIMEOUT_MS    5000
#define SERVER_REQUEST_TIMEOUT_MS   5000
#define SERVER_STACK_SIZE           16384U
#define SERVER_POLL_SLICE_MS        20

static struct signaling_test_script script;
static struct k_thread server_thread;
static int listener = -1;
static char host_authority[24];
static char request_headers[SERVER_REQUEST_HEADER_BYTES + 1U];
static char offer_body[SIGNALING_SDP_MAX_BYTES + 1U];
static char answer_body[SIGNALING_ANSWER_BUFFER_BYTES];
static int server_result;
static bool server_started;

K_THREAD_STACK_DEFINE(server_stack, SERVER_STACK_SIZE);

static int wait_ready(int fd, int16_t events, int64_t deadline)
{
	while (k_uptime_get() < deadline) {
		struct zsock_pollfd pfd = {.fd = fd, .events = events};
		int ret = zsock_poll(&pfd, 1, SERVER_POLL_SLICE_MS);

		if (ret > 0) {
			return (pfd.revents & events) != 0 ? 0 : -ECONNRESET;
		}
		if (ret < 0 && errno != EINTR) {
			return -errno;
		}
	}
	return -ETIMEDOUT;
}

static int send_all(int fd, const char *bytes, size_t size, size_t chunk, uint32_t delay_ms)
{
	while (size > 0U) {
		size_t count = chunk != 0U ? MIN(size, chunk) : size;
		ssize_t sent = zsock_send(fd, bytes, count, 0);

		if (sent <= 0) {
			return -EIO;
		}
		bytes += sent;
		size -= (size_t)sent;
		if (delay_ms != 0U && size > 0U) {
			k_msleep(delay_ms);
		}
	}
	return 0;
}

static bool equals_nocase(const char *left, size_t left_length, const char *right)
{
	if (left_length != strlen(right)) {
		return false;
	}
	for (size_t i = 0U; i < left_length; i++) {
		char a = left[i];
		char b = right[i];

		a = (a >= 'A' && a <= 'Z') ? (char)(a + 32) : a;
		b = (b >= 'A' && b <= 'Z') ? (char)(b + 32) : b;
		if (a != b) {
			return false;
		}
	}
	return true;
}

/* Find a header in a CRLF block. Returns its trimmed value, or NULL when absent. */
static const char *find_header(const char *headers, size_t length, const char *name,
			       size_t *value_length)
{
	const char *line = memchr(headers, '\n', length);

	while (line != NULL && (size_t)(line + 1 - headers) < length) {
		const char *start = line + 1;
		const char *end = memchr(start, '\r', length - (size_t)(start - headers));
		const char *colon;

		if (end == NULL || end == start) {
			return NULL;
		}
		colon = memchr(start, ':', (size_t)(end - start));
		if (colon != NULL && equals_nocase(start, (size_t)(colon - start), name)) {
			const char *value = colon + 1;

			while (value < end && *value == ' ') {
				value++;
			}
			*value_length = (size_t)(end - value);
			return value;
		}
		line = end + 1;
	}
	return NULL;
}

static int parse_length(const char *value, size_t length)
{
	int parsed = 0;

	if (length == 0U || length > 5U) {
		return -EINVAL;
	}
	for (size_t i = 0U; i < length; i++) {
		if (value[i] < '0' || value[i] > '9') {
			return -EINVAL;
		}
		parsed = parsed * 10 + (value[i] - '0');
	}
	return parsed;
}

/* Receive and validate one offer request. Returns the body length or a negative errno. */
static int receive_request(int fd)
{
	int64_t deadline = k_uptime_get() + SERVER_REQUEST_TIMEOUT_MS;
	size_t used = 0U;
	size_t header_length = 0U;
	const char *value;
	size_t value_length;
	size_t body_length;
	size_t have;
	int length;

	while (header_length == 0U) {
		ssize_t count;
		int ret = wait_ready(fd, ZSOCK_POLLIN, deadline);

		if (ret != 0) {
			return ret;
		}
		if (used == SERVER_REQUEST_HEADER_BYTES) {
			return -E2BIG;
		}
		count = zsock_recv(fd, request_headers + used, SERVER_REQUEST_HEADER_BYTES - used,
				   0);
		if (count <= 0) {
			return -ECONNRESET;
		}
		used += (size_t)count;
		request_headers[used] = '\0';
		for (size_t i = 0U; i + 3U < used; i++) {
			if (memcmp(request_headers + i, "\r\n\r\n", 4) == 0) {
				header_length = i + 4U;
				break;
			}
		}
	}

	if (strncmp(request_headers, "POST /offer HTTP/1.1\r\n", 22) != 0) {
		return -EBADMSG;
	}
	value = find_header(request_headers, header_length, "Content-Type", &value_length);
	if (value == NULL || !equals_nocase(value, value_length, "application/sdp")) {
		return -EBADMSG;
	}
	value = find_header(request_headers, header_length, "Host", &value_length);
	if (value == NULL || value_length != strlen(host_authority) ||
	    memcmp(value, host_authority, value_length) != 0) {
		return -EBADMSG;
	}
	value = find_header(request_headers, header_length, "Content-Length", &value_length);
	length = value != NULL ? parse_length(value, value_length) : -EBADMSG;
	if (length <= 0 || (size_t)length > SIGNALING_SDP_MAX_BYTES) {
		return -EBADMSG;
	}
	body_length = (size_t)length;

	have = used - header_length;
	if (have > body_length) {
		return -EBADMSG;
	}
	memcpy(offer_body, request_headers + header_length, have);
	while (have < body_length) {
		ssize_t count;
		int ret = wait_ready(fd, ZSOCK_POLLIN, deadline);

		if (ret != 0) {
			return ret;
		}
		count = zsock_recv(fd, offer_body + have, body_length - have, 0);
		if (count <= 0) {
			return -ECONNRESET;
		}
		have += (size_t)count;
	}
	offer_body[body_length] = '\0';
	if (strlen(offer_body) != body_length ||
	    !signaling_sdp_is_complete(offer_body, body_length)) {
		return -EBADMSG;
	}
	if (script.expected_offer != NULL && strcmp(script.expected_offer, offer_body) != 0) {
		return -EBADMSG;
	}
	return (int)body_length;
}

static int respond_with_answer(int fd)
{
	static const char failure[] = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n"
				      "Connection: close\r\n\r\n";
	char header[160];
	int length = script.answer(offer_body, answer_body, sizeof(answer_body), script.user_data);
	int count;

	if (length <= 0) {
		(void)send_all(fd, failure, sizeof(failure) - 1U, 0U, 0U);
		return length < 0 ? length : -EIO;
	}
	count = snprintk(header, sizeof(header),
			 "HTTP/1.1 200 OK\r\nContent-Type: application/sdp\r\n"
			 "Content-Length: %d\r\nConnection: close\r\n\r\n",
			 length);
	/* A client that hangs up early is not a server failure. */
	if (send_all(fd, header, (size_t)count, 0U, 0U) == 0) {
		(void)send_all(fd, answer_body, (size_t)length, 0U, 0U);
	}
	return 0;
}

static void hold_connection(int fd)
{
	int64_t deadline = k_uptime_get() + script.hold_ms;

	while (k_uptime_get() < deadline) {
		struct zsock_pollfd pfd = {.fd = fd, .events = ZSOCK_POLLIN};
		char byte;

		if (zsock_poll(&pfd, 1, SERVER_POLL_SLICE_MS) > 0 &&
		    (zsock_recv(fd, &byte, sizeof(byte), ZSOCK_MSG_DONTWAIT) == 0 ||
		     (pfd.revents & (ZSOCK_POLLERR | ZSOCK_POLLHUP)) != 0)) {
			return;
		}
	}
}

static void server_main(void *unused_0, void *unused_1, void *unused_2)
{
	int connection;
	int result;

	ARG_UNUSED(unused_0);
	ARG_UNUSED(unused_1);
	ARG_UNUSED(unused_2);

	result = wait_ready(listener, ZSOCK_POLLIN, k_uptime_get() + SERVER_ACCEPT_TIMEOUT_MS);
	if (result != 0) {
		server_result = result;
		return;
	}
	connection = zsock_accept(listener, NULL, NULL);
	if (connection < 0) {
		server_result = -errno;
		return;
	}

	result = receive_request(connection);
	if (result > 0) {
		/* The request was valid. Later failures to deliver the response are the client's.
		 */
		if (script.answer != NULL) {
			result = respond_with_answer(connection);
		} else {
			(void)send_all(connection, script.wire, script.wire_length,
				       script.chunk_bytes, script.chunk_delay_ms);
			result = 0;
		}
	}
	if (result == 0 && script.hold_open) {
		hold_connection(connection);
	}
	(void)zsock_close(connection);
	server_result = result;
}

int signaling_test_server_start(const struct signaling_test_script *new_script, uint16_t *port)
{
	struct net_sockaddr_in address = {
		.sin_family = NET_AF_INET,
		.sin_addr = NET_INADDR_LOOPBACK_INIT,
	};
	net_socklen_t size = sizeof(address);

	script = *new_script;
	server_result = -EINPROGRESS;
	listener = zsock_socket(NET_AF_INET, NET_SOCK_STREAM, NET_IPPROTO_TCP);
	if (listener < 0) {
		return -errno;
	}
	if (zsock_bind(listener, (struct net_sockaddr *)&address, sizeof(address)) != 0 ||
	    zsock_listen(listener, 1) != 0 ||
	    zsock_getsockname(listener, (struct net_sockaddr *)&address, &size) != 0) {
		int error = -errno;

		(void)zsock_close(listener);
		listener = -1;
		return error;
	}
	*port = net_ntohs(address.sin_port);
	(void)snprintk(host_authority, sizeof(host_authority), "127.0.0.1:%u", (unsigned int)*port);

	(void)k_thread_create(&server_thread, server_stack, K_THREAD_STACK_SIZEOF(server_stack),
			      server_main, NULL, NULL, NULL, K_PRIO_PREEMPT(6), 0, K_NO_WAIT);
	server_started = true;
	return 0;
}

int signaling_test_server_finish(void)
{
	if (server_started) {
		if (k_thread_join(&server_thread, K_SECONDS(15)) != 0) {
			k_thread_abort(&server_thread);
			server_result = -ETIMEDOUT;
		}
		server_started = false;
	}
	if (listener >= 0) {
		(void)zsock_close(listener);
		listener = -1;
	}
	/*
	 * The simulator clock only advances while every thread is idle. Wait so that the TIME_WAIT
	 * context left by this connection is reaped before the next case opens another.
	 */
	k_msleep(CONFIG_NET_TCP_TIME_WAIT_DELAY + 10);
	return server_result;
}

int signaling_test_closed_port(uint16_t *port)
{
	struct net_sockaddr_in address = {
		.sin_family = NET_AF_INET,
		.sin_addr = NET_INADDR_LOOPBACK_INIT,
	};
	net_socklen_t size = sizeof(address);
	int fd = zsock_socket(NET_AF_INET, NET_SOCK_STREAM, NET_IPPROTO_TCP);
	int result = 0;

	if (fd < 0) {
		return -errno;
	}
	if (zsock_bind(fd, (struct net_sockaddr *)&address, sizeof(address)) != 0 ||
	    zsock_getsockname(fd, (struct net_sockaddr *)&address, &size) != 0) {
		result = -errno;
	} else {
		*port = net_ntohs(address.sin_port);
	}
	(void)zsock_close(fd);
	return result;
}
