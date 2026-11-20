/* SPDX-FileCopyrightText: Copyright (c) 2026 Meta Platforms, Inc. */
/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Bounded HTTP/1.x offer client built directly on Zephyr sockets.
 *
 * The contract matches the Linux diagnostic client in linuxapp/03_libpeer: IPv4
 * literal or localhost, no DNS, TLS, redirects, or chunked encoding, exactly
 * one application/sdp Content-Type and one bounded Content-Length, and a
 * complete CRLF-delimited SDP body. Every wait is bounded by one total deadline
 * and a cancellation callback.
 */
#include "signaling_client_zephyr.h"

#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include "platform.h"

#define SIGNALING_URL_MAX_BYTES       255U
#define SIGNALING_AUTHORITY_MAX_BYTES 64U
#define SIGNALING_HOST_MAX_BYTES      16U /* dotted IPv4 plus NUL */
#define SIGNALING_REQUEST_MAX_BYTES   512U
#define SIGNALING_POLL_SLICE_MS       20

struct signaling_endpoint {
	struct net_sockaddr_in address;
	char authority[SIGNALING_AUTHORITY_MAX_BYTES];
	const char *path;
};

static bool is_cancelled(const struct signaling_request *request)
{
	return request->cancelled != NULL && request->cancelled(request->user_data);
}

static bool is_lower(char c)
{
	return c >= 'a' && c <= 'z';
}

static char to_lower(char c)
{
	return c >= 'A' && c <= 'Z' ? (char)(c - 'A' + 'a') : c;
}

static bool equals_nocase(const char *text, size_t length, const char *literal)
{
	if (strlen(literal) != length) {
		return false;
	}
	for (size_t i = 0U; i < length; i++) {
		if (to_lower(text[i]) != to_lower(literal[i])) {
			return false;
		}
	}
	return true;
}

bool signaling_sdp_is_complete(const char *sdp, size_t length)
{
	size_t line_start = 0U;
	bool media = false;

	if (sdp == NULL || length < 5U || length > SIGNALING_SDP_MAX_BYTES ||
	    memcmp(sdp, "v=0\r\n", 5) != 0) {
		return false;
	}
	for (size_t i = 0U; i < length; i++) {
		unsigned char c = (unsigned char)sdp[i];

		if (c == '\r') {
			if (i + 1U >= length || sdp[i + 1U] != '\n') {
				return false;
			}
		} else if (c == '\n') {
			size_t line_length = i - 1U - line_start;

			if (sdp[i - 1U] != '\r' || line_length < 2U || !is_lower(sdp[line_start]) ||
			    sdp[line_start + 1U] != '=') {
				return false;
			}
			if (sdp[line_start] == 'm') {
				media = true;
			}
			line_start = i + 1U;
		} else if (c < 0x20U || c > 0x7eU) {
			return false;
		}
	}
	return media && line_start == length;
}

static enum signaling_result parse_port(const char *text, size_t length, uint16_t *port)
{
	uint32_t value = 0U;

	if (length == 0U || length > 5U) {
		return SIGNALING_BAD_URL;
	}
	for (size_t i = 0U; i < length; i++) {
		if (text[i] < '0' || text[i] > '9') {
			return SIGNALING_BAD_URL;
		}
		value = value * 10U + (uint32_t)(text[i] - '0');
	}
	if (value < 1U || value > UINT16_MAX) {
		return SIGNALING_BAD_URL;
	}
	*port = (uint16_t)value;
	return SIGNALING_OK;
}

static enum signaling_result parse_url(const char *url, struct signaling_endpoint *endpoint)
{
	static const char scheme[] = "http://";
	const size_t scheme_length = sizeof(scheme) - 1U;
	size_t length = strnlen(url, SIGNALING_URL_MAX_BYTES + 1U);
	const char *authority;
	const char *colon;
	size_t authority_length;
	size_t host_length;
	char host[SIGNALING_HOST_MAX_BYTES];
	uint16_t port = 80U;

	if (length > SIGNALING_URL_MAX_BYTES || strncmp(url, scheme, scheme_length) != 0) {
		return SIGNALING_BAD_URL;
	}
	for (size_t i = 0U; i < length; i++) {
		unsigned char c = (unsigned char)url[i];

		if (c <= 0x20U || c >= 0x7fU || c == '#' || c == '@') {
			return SIGNALING_BAD_URL;
		}
	}
	authority = url + scheme_length;
	endpoint->path = strchr(authority, '/');
	if (endpoint->path == NULL || endpoint->path == authority) {
		return SIGNALING_BAD_URL;
	}
	authority_length = (size_t)(endpoint->path - authority);
	if (authority_length >= sizeof(endpoint->authority)) {
		return SIGNALING_BAD_URL;
	}
	memcpy(endpoint->authority, authority, authority_length);
	endpoint->authority[authority_length] = '\0';

	colon = memchr(authority, ':', authority_length);
	host_length = colon != NULL ? (size_t)(colon - authority) : authority_length;
	if (host_length == 0U || host_length >= sizeof(host)) {
		return SIGNALING_BAD_URL;
	}
	if (colon != NULL &&
	    parse_port(colon + 1, authority_length - host_length - 1U, &port) != SIGNALING_OK) {
		return SIGNALING_BAD_URL;
	}
	memcpy(host, authority, host_length);
	host[host_length] = '\0';
	if (strcmp(host, "localhost") == 0) {
		strcpy(host, "127.0.0.1");
	}

	memset(&endpoint->address, 0, sizeof(endpoint->address));
	endpoint->address.sin_family = NET_AF_INET;
	endpoint->address.sin_port = net_htons(port);
	if (zsock_inet_pton(NET_AF_INET, host, &endpoint->address.sin_addr) != 1) {
		return SIGNALING_BAD_URL;
	}
	return SIGNALING_OK;
}

static enum signaling_result wait_for(int fd, int16_t events, int64_t deadline,
				      const struct signaling_request *request)
{
	for (;;) {
		struct zsock_pollfd pfd = {.fd = fd, .events = events};
		int64_t remaining;
		int ret;

		if (is_cancelled(request)) {
			return SIGNALING_CANCELLED;
		}
		remaining = deadline - k_uptime_get();
		if (remaining <= 0) {
			return SIGNALING_TIMEOUT;
		}
		ret = zsock_poll(&pfd, 1, (int)MIN(remaining, (int64_t)SIGNALING_POLL_SLICE_MS));
		if (ret < 0) {
			if (errno == EINTR) {
				continue;
			}
			return SIGNALING_IO_ERROR;
		}
		if (ret > 0) {
			if ((pfd.revents & events) != 0) {
				return SIGNALING_OK;
			}
			if ((pfd.revents & (ZSOCK_POLLERR | ZSOCK_POLLHUP | ZSOCK_POLLNVAL)) != 0) {
				return SIGNALING_IO_ERROR;
			}
		}
	}
}

static enum signaling_result connect_error(int error)
{
	switch (error) {
	case ECONNREFUSED:
	case ECONNRESET:
	case ECONNABORTED:
	case ENETUNREACH:
	case EHOSTUNREACH:
	case EADDRNOTAVAIL:
	case ENETDOWN:
		return SIGNALING_UNREACHABLE;
	case ETIMEDOUT:
		return SIGNALING_TIMEOUT;
	default:
		return SIGNALING_IO_ERROR;
	}
}

static enum signaling_result connect_to(int fd, const struct signaling_endpoint *endpoint,
					int64_t deadline, const struct signaling_request *request)
{
	enum signaling_result result;
	net_socklen_t size = sizeof(int);
	char address[NET_IPV4_ADDR_LEN];
	int error = 0;

	(void)net_addr_ntop(NET_AF_INET, &endpoint->address.sin_addr, address, sizeof(address));
	platform_log("info", "signaling_connect", "target=%s port=%u", address,
		     (unsigned int)net_ntohs(endpoint->address.sin_port));
	if (zsock_connect(fd, (const struct net_sockaddr *)&endpoint->address,
			  sizeof(endpoint->address)) == 0) {
		platform_log("info", "signaling_connect", "status=connected_immediately");
		return SIGNALING_OK;
	}
	if (errno != EINPROGRESS) {
		platform_log("error", "signaling_connect", "status=connect_failed errno=%d", errno);
		return connect_error(errno);
	}
	platform_log("info", "signaling_connect", "status=in_progress");
	result = wait_for(fd, ZSOCK_POLLOUT, deadline, request);
	if (result != SIGNALING_OK && result != SIGNALING_IO_ERROR) {
		platform_log("error", "signaling_connect", "status=wait_failed result=%s",
			     signaling_result_str(result));
		return result;
	}
	if (zsock_getsockopt(fd, ZSOCK_SOL_SOCKET, ZSOCK_SO_ERROR, &error, &size) != 0) {
		platform_log("error", "signaling_connect", "status=getsockopt_failed errno=%d",
			     errno);
		return SIGNALING_IO_ERROR;
	}
	if (error != 0) {
		platform_log("error", "signaling_connect", "status=socket_error errno=%d", error);
		return connect_error(error);
	}
	platform_log("info", "signaling_connect", "status=connected poll_result=%s",
		     signaling_result_str(result));
	return result;
}

static enum signaling_result transmit(int fd, const char *bytes, size_t size, int64_t deadline,
				      const struct signaling_request *request)
{
	while (size > 0U) {
		enum signaling_result result = wait_for(fd, ZSOCK_POLLOUT, deadline, request);
		ssize_t count;

		if (result != SIGNALING_OK) {
			return result;
		}
		count = zsock_send(fd, bytes, size, ZSOCK_MSG_DONTWAIT);
		if (count > 0) {
			bytes += count;
			size -= (size_t)count;
		} else if (count == 0 || (errno != EINTR && errno != EAGAIN)) {
			return SIGNALING_IO_ERROR;
		}
	}
	return SIGNALING_OK;
}

/*
 * Returns SIGNALING_TRUNCATED when the peer closes before @p size bytes could
 * be read.
 */
static enum signaling_result receive_some(int fd, char *buffer, size_t size, int64_t deadline,
					  const struct signaling_request *request, size_t *received)
{
	for (;;) {
		enum signaling_result result = wait_for(fd, ZSOCK_POLLIN, deadline, request);
		ssize_t count;

		if (result != SIGNALING_OK) {
			return result;
		}
		count = zsock_recv(fd, buffer, size, ZSOCK_MSG_DONTWAIT);
		if (count > 0) {
			*received = (size_t)count;
			return SIGNALING_OK;
		}
		if (count == 0) {
			return SIGNALING_TRUNCATED;
		}
		if (errno != EINTR && errno != EAGAIN) {
			return SIGNALING_IO_ERROR;
		}
	}
}

static bool has_line_break(const char *text, size_t length)
{
	return memchr(text, '\r', length) != NULL || memchr(text, '\n', length) != NULL;
}

static const char *find_crlf(const char *text, size_t length)
{
	for (size_t i = 0U; i + 1U < length; i++) {
		if (text[i] == '\r' && text[i + 1U] == '\n') {
			return &text[i];
		}
	}
	return NULL;
}

static enum signaling_result parse_content_length(const char *value, size_t length,
						  size_t *body_length)
{
	uint64_t parsed = 0U;

	if (length == 0U) {
		return SIGNALING_BAD_CONTENT_LENGTH;
	}
	for (size_t i = 0U; i < length; i++) {
		if (value[i] < '0' || value[i] > '9') {
			return SIGNALING_BAD_CONTENT_LENGTH;
		}
		if (parsed <= SIGNALING_SDP_MAX_BYTES) {
			parsed = parsed * 10U + (uint64_t)(value[i] - '0');
		}
	}
	if (parsed > SIGNALING_SDP_MAX_BYTES) {
		return SIGNALING_OVERSIZE;
	}
	if (parsed == 0U) {
		return SIGNALING_BAD_CONTENT_LENGTH;
	}
	*body_length = (size_t)parsed;
	return SIGNALING_OK;
}

static enum signaling_result check_header_line(const char *line, size_t length, bool *have_length,
					       bool *have_type, size_t *body_length)
{
	const char *colon = memchr(line, ':', length);
	const char *value;
	size_t name_length;
	size_t value_length;

	if (colon == NULL || colon == line || *line == ' ' || *line == '\t' ||
	    has_line_break(line, length)) {
		return SIGNALING_BAD_HEADER;
	}
	name_length = (size_t)(colon - line);
	for (size_t i = 0U; i < name_length; i++) {
		if (line[i] == ' ' || line[i] == '\t') {
			return SIGNALING_BAD_HEADER;
		}
	}
	value = colon + 1;
	value_length = length - name_length - 1U;
	while (value_length > 0U && (*value == ' ' || *value == '\t')) {
		value++;
		value_length--;
	}
	while (value_length > 0U &&
	       (value[value_length - 1U] == ' ' || value[value_length - 1U] == '\t')) {
		value_length--;
	}

	if (equals_nocase(line, name_length, "Content-Length")) {
		if (*have_length) {
			return SIGNALING_BAD_CONTENT_LENGTH;
		}
		*have_length = true;
		return parse_content_length(value, value_length, body_length);
	}
	if (equals_nocase(line, name_length, "Content-Type")) {
		if (*have_type || !equals_nocase(value, value_length, "application/sdp")) {
			return SIGNALING_BAD_CONTENT_TYPE;
		}
		*have_type = true;
		return SIGNALING_OK;
	}
	if (equals_nocase(line, name_length, "Transfer-Encoding")) {
		return SIGNALING_BAD_HEADER;
	}
	return SIGNALING_OK;
}

/*
 * Parse the complete header block (through the blank line). Ambiguous framing
 * and types are rejected rather than guessed: only a 200 status, one
 * Content-Type of application/sdp with no parameters, one bounded
 * Content-Length, and no Transfer-Encoding are accepted.
 */
static enum signaling_result parse_headers(const char *headers, size_t length, size_t *body_length)
{
	static const char status_11[] = "HTTP/1.1 200 ";
	static const char status_10[] = "HTTP/1.0 200 ";
	const size_t status_length = sizeof(status_11) - 1U;
	const char *line_end = find_crlf(headers, length);
	bool have_length = false;
	bool have_type = false;
	size_t position;

	if (memchr(headers, '\0', length) != NULL) {
		return SIGNALING_BAD_HEADER;
	}
	if (line_end == NULL || (size_t)(line_end - headers) < status_length ||
	    (strncmp(headers, status_11, status_length) != 0 &&
	     strncmp(headers, status_10, status_length) != 0) ||
	    has_line_break(headers, (size_t)(line_end - headers))) {
		return SIGNALING_BAD_STATUS;
	}

	position = (size_t)(line_end - headers) + 2U;
	while (position + 2U < length) {
		enum signaling_result result;

		line_end = find_crlf(headers + position, length - position);
		if (line_end == NULL) {
			return SIGNALING_BAD_HEADER;
		}
		result = check_header_line(headers + position,
					   (size_t)(line_end - (headers + position)), &have_length,
					   &have_type, body_length);
		if (result != SIGNALING_OK) {
			return result;
		}
		position = (size_t)(line_end - headers) + 2U;
	}
	if (!have_length) {
		return SIGNALING_BAD_CONTENT_LENGTH;
	}
	return have_type ? SIGNALING_OK : SIGNALING_BAD_CONTENT_TYPE;
}

static enum signaling_result receive_headers(int fd, char *headers, int64_t deadline,
					     const struct signaling_request *request, size_t *used,
					     size_t *header_length)
{
	size_t searched = 0U;

	*used = 0U;
	for (;;) {
		enum signaling_result result;
		size_t received;

		if (*used == SIGNALING_HEADER_MAX_BYTES) {
			return SIGNALING_OVERSIZE;
		}
		result = receive_some(fd, headers + *used, SIGNALING_HEADER_MAX_BYTES - *used,
				      deadline, request, &received);
		if (result != SIGNALING_OK) {
			return result;
		}
		*used += received;
		for (; searched + 3U < *used; searched++) {
			if (memcmp(headers + searched, "\r\n\r\n", 4) == 0) {
				*header_length = searched + 4U;
				return SIGNALING_OK;
			}
		}
	}
}

static enum signaling_result receive_body(int fd, const char *headers, size_t used,
					  size_t header_length, size_t body_length,
					  int64_t deadline, const struct signaling_request *request,
					  char *answer)
{
	size_t have = used - header_length;

	if (have > body_length) {
		return SIGNALING_BAD_BODY;
	}
	memcpy(answer, headers + header_length, have);
	while (have < body_length) {
		enum signaling_result result;
		size_t received;

		/* Read one byte past the declared length so trailing data is detected. */
		result = receive_some(fd, answer + have, body_length - have + 1U, deadline, request,
				      &received);
		if (result != SIGNALING_OK) {
			return result;
		}
		have += received;
		if (have > body_length) {
			return SIGNALING_BAD_BODY;
		}
	}
	answer[body_length] = '\0';
	if (strlen(answer) != body_length || !signaling_sdp_is_complete(answer, body_length)) {
		return SIGNALING_BAD_BODY;
	}
	return SIGNALING_OK;
}

static enum signaling_result run_exchange(int fd, const struct signaling_request *request,
					  const struct signaling_endpoint *endpoint,
					  size_t offer_length, int64_t deadline, char *answer,
					  size_t *answer_length)
{
	char headers[SIGNALING_HEADER_MAX_BYTES];
	char line[SIGNALING_REQUEST_MAX_BYTES];
	enum signaling_result result;
	size_t body_length = 0U;
	size_t header_length;
	size_t used;
	int count;

	result = connect_to(fd, endpoint, deadline, request);
	if (result != SIGNALING_OK) {
		return result;
	}
	count = snprintk(line, sizeof(line),
			 "POST %s HTTP/1.1\r\nHost: %s\r\nContent-Type: application/sdp\r\n"
			 "Accept: application/sdp\r\nContent-Length: %zu\r\n"
			 "Connection: close\r\n\r\n",
			 endpoint->path, endpoint->authority, offer_length);
	if (count <= 0 || (size_t)count >= sizeof(line)) {
		return SIGNALING_BAD_URL;
	}
	result = transmit(fd, line, (size_t)count, deadline, request);
	if (result == SIGNALING_OK) {
		result = transmit(fd, request->offer, offer_length, deadline, request);
	}
	if (result != SIGNALING_OK) {
		return result;
	}

	result = receive_headers(fd, headers, deadline, request, &used, &header_length);
	if (result != SIGNALING_OK) {
		return result;
	}
	result = parse_headers(headers, header_length, &body_length);
	if (result != SIGNALING_OK) {
		return result;
	}
	result = receive_body(fd, headers, used, header_length, body_length, deadline, request,
			      answer);
	if (result == SIGNALING_OK) {
		*answer_length = body_length;
	}
	return result;
}

enum signaling_result signaling_exchange(const struct signaling_request *request, char *answer,
					 size_t capacity, size_t *answer_length)
{
	struct signaling_endpoint endpoint;
	enum signaling_result result;
	size_t offer_length;
	int64_t deadline;
	uint32_t timeout_ms;
	int fd;

	if (request == NULL || request->url == NULL || request->offer == NULL || answer == NULL ||
	    answer_length == NULL || capacity < SIGNALING_ANSWER_BUFFER_BYTES) {
		return SIGNALING_BAD_ARGUMENT;
	}
	result = parse_url(request->url, &endpoint);
	if (result != SIGNALING_OK) {
		return result;
	}
	offer_length = strnlen(request->offer, SIGNALING_SDP_MAX_BYTES + 1U);
	if (!signaling_sdp_is_complete(request->offer, offer_length)) {
		return SIGNALING_BAD_OFFER;
	}

	timeout_ms = request->timeout_ms != 0U ? request->timeout_ms
					       : CONFIG_ZEPHYRTC_SIGNALING_TIMEOUT_MS;
	deadline = k_uptime_get() + timeout_ms;
	fd = zsock_socket(NET_AF_INET, NET_SOCK_STREAM, NET_IPPROTO_TCP);
	if (fd < 0) {
		return SIGNALING_IO_ERROR;
	}
	if (zsock_fcntl(fd, ZVFS_F_SETFL, ZVFS_O_NONBLOCK) < 0) {
		(void)zsock_close(fd);
		return SIGNALING_IO_ERROR;
	}
	result =
		run_exchange(fd, request, &endpoint, offer_length, deadline, answer, answer_length);
	(void)zsock_close(fd);
	return result;
}

const char *signaling_result_str(enum signaling_result result)
{
	switch (result) {
	case SIGNALING_OK:
		return "ok";
	case SIGNALING_BAD_ARGUMENT:
		return "bad_argument";
	case SIGNALING_BAD_URL:
		return "bad_url";
	case SIGNALING_BAD_OFFER:
		return "bad_offer";
	case SIGNALING_UNREACHABLE:
		return "unreachable";
	case SIGNALING_TIMEOUT:
		return "timeout";
	case SIGNALING_CANCELLED:
		return "cancelled";
	case SIGNALING_IO_ERROR:
		return "io_error";
	case SIGNALING_BAD_STATUS:
		return "bad_status";
	case SIGNALING_BAD_HEADER:
		return "bad_header";
	case SIGNALING_BAD_CONTENT_TYPE:
		return "bad_content_type";
	case SIGNALING_BAD_CONTENT_LENGTH:
		return "bad_content_length";
	case SIGNALING_OVERSIZE:
		return "oversize";
	case SIGNALING_TRUNCATED:
		return "truncated";
	case SIGNALING_BAD_BODY:
		return "bad_body";
	}
	return "unknown";
}
