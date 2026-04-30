/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include <zephyr/net/quic.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/ztest.h>

#include "certificate.h"

#define SERVER_IPV4_ADDR "127.0.0.1"
#define SERVER_PORT 18443
#define ALT_SVC_DISABLED_PORT 18444
#define RECV_TIMEOUT_S 1
#define SHORT_POLL_TIMEOUT_MS 150
#define MAX_RESPONSE_SIZE 256
#define MAX_TRACKED_STREAMS 8

#define H3_FRAME_DATA 0x00
#define H3_FRAME_HEADERS 0x01
#define H3_FRAME_SETTINGS 0x04

#define H3_UNI_STREAM_CONTROL 0x00
#define H3_UNI_STREAM_PUSH 0x01
#define H3_UNI_STREAM_QPACK_ENCODER 0x02
#define H3_UNI_STREAM_QPACK_DECODER 0x03

#define QPACK_STATIC_PATH_SLASH 1
#define QPACK_STATIC_METHOD_GET 17
#define QPACK_STATIC_METHOD_POST 20

#define TEST_STATIC_PAYLOAD "Hello, H3!"
#define TEST_DYNAMIC_POST_PAYLOAD "Test dynamic POST"

static const sec_tag_t server_sec_tags[] = {
	SERVER_CERTIFICATE_TAG,
};

static const sec_tag_t client_sec_tags[] = {
	CA_CERTIFICATE_TAG,
};

static const char *const alpn_list[] = {
	"h3",
	NULL,
};

static uint16_t test_http_service_port = SERVER_PORT;
static uint16_t alt_svc_disabled_service_port = ALT_SVC_DISABLED_PORT;

static const struct http_service_config test_http_service_cfg = {
	.http_ver = HTTP_VERSION_1 | HTTP_VERSION_3,
	.h3_alt_svc_policy = HTTP_H3_ALT_SVC_ENABLE,
};

static const struct http_service_config alt_svc_disabled_service_cfg = {
	.http_ver = HTTP_VERSION_1 | HTTP_VERSION_3,
	.h3_alt_svc_policy = HTTP_H3_ALT_SVC_DISABLE,
};

HTTPS_SERVICE_DEFINE(test_http_service, SERVER_IPV4_ADDR,
		     &test_http_service_port, 1, 1, NULL, NULL,
		     &test_http_service_cfg,
		     server_sec_tags, sizeof(server_sec_tags));

static const char static_resource_payload[] = TEST_STATIC_PAYLOAD;

static struct http_resource_detail_static static_resource_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_STATIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_GET),
	},
	.static_data = static_resource_payload,
	.static_data_len = sizeof(static_resource_payload) - 1,
};

HTTPS_SERVICE_DEFINE_EMPTY(alt_svc_disabled_service, SERVER_IPV4_ADDR,
			   &alt_svc_disabled_service_port, 1, 1, NULL,
			   &static_resource_detail.common,
			   &alt_svc_disabled_service_cfg,
			   server_sec_tags, sizeof(server_sec_tags));

HTTP_RESOURCE_DEFINE(static_resource, test_http_service, "/", &static_resource_detail);

static uint8_t dynamic_payload[64];
static size_t dynamic_bytes_received;
static bool dynamic_more_seen;
static bool dynamic_final_seen;
static bool dynamic_complete;

static int dynamic_cb(struct http_client_ctx *client,
		      enum http_transaction_status status,
		      const struct http_request_ctx *request_ctx,
		      struct http_response_ctx *response_ctx,
		      void *user_data)
{
	ARG_UNUSED(client);
	ARG_UNUSED(user_data);

	if (status == HTTP_SERVER_TRANSACTION_ABORTED) {
		return 0;
	}

	if (status == HTTP_SERVER_TRANSACTION_COMPLETE) {
		dynamic_complete = true;
		return 0;
	}

	if (request_ctx->data_len > 0) {
		zassert_true(dynamic_bytes_received + request_ctx->data_len <=
			     sizeof(dynamic_payload),
			     "Dynamic payload buffer too small");

		memcpy(dynamic_payload + dynamic_bytes_received,
		       request_ctx->data, request_ctx->data_len);
		dynamic_bytes_received += request_ctx->data_len;
	}

	if (status == HTTP_SERVER_REQUEST_DATA_MORE) {
		dynamic_more_seen = true;
		return 0;
	}

	dynamic_final_seen = true;

	response_ctx->status = 200;
	response_ctx->final_chunk = true;
	return 0;
}

static struct http_resource_detail_dynamic dynamic_resource_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_DYNAMIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_POST),
		.content_type = "text/plain",
	},
	.cb = dynamic_cb,
	.user_data = NULL,
};

HTTP_RESOURCE_DEFINE(dynamic_resource, test_http_service, "/dynamic",
		     &dynamic_resource_detail);

static int streaming_dynamic_cb(struct http_client_ctx *client,
				enum http_transaction_status status,
				const struct http_request_ctx *request_ctx,
				struct http_response_ctx *response_ctx,
				void *user_data)
{
	ARG_UNUSED(client);
	ARG_UNUSED(user_data);

	if (status == HTTP_SERVER_TRANSACTION_ABORTED ||
	    status == HTTP_SERVER_TRANSACTION_COMPLETE) {
		return 0;
	}

	response_ctx->status = 200;
	response_ctx->body = request_ctx->data;
	response_ctx->body_len = request_ctx->data_len;
	response_ctx->final_chunk = (status == HTTP_SERVER_REQUEST_DATA_FINAL);

	return 0;
}

static struct http_resource_detail_dynamic streaming_resource_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_DYNAMIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_POST),
		.content_type = "text/plain",
	},
	.cb = streaming_dynamic_cb,
	.user_data = NULL,
};

HTTP_RESOURCE_DEFINE(streaming_resource, test_http_service, "/streaming",
		     &streaming_resource_detail);

static int client_conn_fd = -1;
static int tracked_stream_fds[MAX_TRACKED_STREAMS];
static size_t tracked_stream_count;

static void track_stream_fd(int fd)
{
	zassert_true(tracked_stream_count < ARRAY_SIZE(tracked_stream_fds),
		     "Too many tracked stream fds");
	tracked_stream_fds[tracked_stream_count++] = fd;
}

static void untrack_stream_fd(int fd)
{
	for (size_t i = 0; i < tracked_stream_count; i++) {
		if (tracked_stream_fds[i] != fd) {
			continue;
		}

		tracked_stream_fds[i] = tracked_stream_fds[tracked_stream_count - 1];
		tracked_stream_fds[tracked_stream_count - 1] = -1;
		tracked_stream_count--;
		break;
	}
}

static void wait_for_socket_event(int fd, short events, int timeout_ms,
				  const char *what)
{
	struct zsock_pollfd pfd = {
		.fd = fd,
		.events = events,
	};
	int ret;

	ret = zsock_poll(&pfd, 1, timeout_ms);
	zassert_true(ret > 0, "Timed out waiting for %s", what);
	zassert_true((pfd.revents & events) != 0,
		     "Unexpected poll events 0x%x while waiting for %s",
		     pfd.revents, what);
}

static void init_ipv4_addr(struct net_sockaddr_in *addr, const char *ip, uint16_t port)
{
	int ret;

	memset(addr, 0, sizeof(*addr));
	addr->sin_family = NET_AF_INET;
	addr->sin_port = net_htons(port);

	ret = zsock_inet_pton(NET_AF_INET, ip, &addr->sin_addr);
	zassert_equal(ret, 1, "inet_pton failed for %s", ip);
}

static size_t h3_encode_varint(uint8_t *buf, uint64_t value)
{
	if (value <= 0x3fU) {
		buf[0] = (uint8_t)value;
		return 1;
	}

	if (value <= 0x3fffu) {
		buf[0] = (uint8_t)((value >> 8) | 0x40U);
		buf[1] = (uint8_t)(value & 0xffU);
		return 2;
	}

	zassert_unreachable("Unexpected varint value %llu", value);
	return 0;
}

static size_t h3_decode_varint(const uint8_t *buf, size_t len, uint64_t *value)
{
	uint8_t prefix;
	size_t encoded_len;

	zassert_true(len > 0, "Need at least one byte for varint");

	prefix = buf[0] >> 6;
	encoded_len = 1U << prefix;
	zassert_true(encoded_len <= len, "Truncated varint");

	*value = buf[0] & 0x3fU;

	for (size_t i = 1; i < encoded_len; i++) {
		*value = (*value << 8) | buf[i];
	}

	return encoded_len;
}

static size_t build_headers_frame(uint8_t *buf,
				  enum http_method method,
				  const char *path)
{
	size_t path_len = strlen(path);
	size_t pos = 0;
	uint8_t method_index;

	switch (method) {
	case HTTP_GET:
		method_index = 0xc0U | QPACK_STATIC_METHOD_GET;
		break;
	case HTTP_POST:
		method_index = 0xc0U | QPACK_STATIC_METHOD_POST;
		break;
	default:
		zassert_unreachable("Unsupported method %d", method);
		return 0;
	}

	buf[pos++] = 0x00U;
	buf[pos++] = 0x00U;
	buf[pos++] = method_index;

	if (strcmp(path, "/") == 0) {
		buf[pos++] = 0xc0U | QPACK_STATIC_PATH_SLASH;
	} else {
		zassert_true(path_len <= 0x7fU, "Path too long");
		buf[pos++] = 0x50U | QPACK_STATIC_PATH_SLASH;
		buf[pos++] = (uint8_t)path_len;
		memcpy(buf + pos, path, path_len);
		pos += path_len;
	}

	memmove(buf + 2, buf, pos);
	buf[0] = H3_FRAME_HEADERS;
	buf[1] = (uint8_t)pos;

	return pos + 2;
}

static size_t build_data_frame(uint8_t *buf, const uint8_t *payload, size_t payload_len)
{
	size_t pos = 0;

	pos += h3_encode_varint(buf + pos, H3_FRAME_DATA);
	pos += h3_encode_varint(buf + pos, payload_len);
	memcpy(buf + pos, payload, payload_len);
	pos += payload_len;

	return pos;
}

static size_t build_control_settings_stream(uint8_t *buf)
{
	size_t pos = 0;

	buf[pos++] = H3_UNI_STREAM_CONTROL;
	pos += h3_encode_varint(buf + pos, H3_FRAME_SETTINGS);
	pos += h3_encode_varint(buf + pos, 0);

	return pos;
}

static void setup_tls_credentials_once(void)
{
	static bool credentials_added;
	int ret;

	if (credentials_added) {
		return;
	}

	ret = tls_credential_add(CA_CERTIFICATE_TAG, TLS_CREDENTIAL_CA_CERTIFICATE,
				 ca_certificate, sizeof(ca_certificate));
	zassert_true(ret == 0 || ret == -EEXIST,
		     "Failed to add CA certificate (%d)", ret);

	ret = tls_credential_add(SERVER_CERTIFICATE_TAG,
				 TLS_CREDENTIAL_PUBLIC_CERTIFICATE,
				 server_certificate,
				 sizeof(server_certificate));
	zassert_true(ret == 0 || ret == -EEXIST,
		     "Failed to add server certificate (%d)", ret);

	ret = tls_credential_add(SERVER_CERTIFICATE_TAG, TLS_CREDENTIAL_PRIVATE_KEY,
				 server_private_key, sizeof(server_private_key));
	zassert_true(ret == 0 || ret == -EEXIST,
		     "Failed to add server key (%d)", ret);

	credentials_added = true;
}

static void connect_h3_client(void)
{
	struct net_sockaddr_in server_addr;
	int ret;

	init_ipv4_addr(&server_addr, SERVER_IPV4_ADDR, SERVER_PORT);

	client_conn_fd = quic_connection_open((struct net_sockaddr *)&server_addr, NULL);
	zassert_true(client_conn_fd >= 0, "Failed to open client QUIC connection (%d)",
		     client_conn_fd);

	ret = zsock_setsockopt(client_conn_fd, ZSOCK_SOL_TLS, ZSOCK_TLS_SEC_TAG_LIST,
			       client_sec_tags, sizeof(client_sec_tags));
	zassert_ok(ret, "Failed to set client sec tags (%d)", errno);

	ret = zsock_setsockopt(client_conn_fd, ZSOCK_SOL_TLS, ZSOCK_TLS_ALPN_LIST,
			       alpn_list, sizeof(alpn_list));
	zassert_ok(ret, "Failed to set client ALPN (%d)", errno);
}

static int connect_https_client(uint16_t port)
{
	struct net_sockaddr_in server_addr;
	int fd;
	int ret;

	init_ipv4_addr(&server_addr, SERVER_IPV4_ADDR, port);

	fd = zsock_socket(NET_AF_INET, NET_SOCK_STREAM, NET_IPPROTO_TLS_1_2);
	zassert_true(fd >= 0, "Failed to open HTTPS client socket (%d)", errno);

	ret = zsock_setsockopt(fd, ZSOCK_SOL_TLS, ZSOCK_TLS_SEC_TAG_LIST,
			       client_sec_tags, sizeof(client_sec_tags));
	zassert_ok(ret, "Failed to set HTTPS client sec tags (%d)", errno);

	ret = zsock_setsockopt(fd, ZSOCK_SOL_TLS, ZSOCK_TLS_HOSTNAME,
			       "localhost", sizeof("localhost"));
	zassert_ok(ret, "Failed to set HTTPS client hostname (%d)", errno);

	ret = zsock_connect(fd, (struct net_sockaddr *)&server_addr, sizeof(server_addr));
	zassert_ok(ret, "Failed to connect HTTPS client (%d)", errno);

	return fd;
}

static int open_client_stream(uint8_t direction)
{
	int fd;

	fd = quic_stream_open(client_conn_fd, QUIC_STREAM_CLIENT, direction, 0);
	zassert_true(fd >= 0, "Failed to open client stream (%d)", fd);
	track_stream_fd(fd);

	return fd;
}

static void send_all(int fd, const uint8_t *buf, size_t len)
{
	size_t sent = 0;

	while (sent < len) {
		ssize_t ret;

		ret = zsock_send(fd, buf + sent, len - sent, 0);
		zassert_true(ret > 0, "send failed on fd %d (%d)", fd, errno);
		sent += (size_t)ret;
	}
}

static void shutdown_stream_send(int fd)
{
	int ret;

	ret = zsock_shutdown(fd, ZSOCK_SHUT_WR);
	zassert_ok(ret, "shutdown(SHUT_WR) failed on fd %d (%d)", fd, errno);
}

static size_t read_stream_to_eof(int fd, uint8_t *buf, size_t max_len)
{
	size_t total = 0;

	while (total < max_len) {
		ssize_t ret;

		wait_for_socket_event(fd, ZSOCK_POLLIN | ZSOCK_POLLHUP,
				      RECV_TIMEOUT_S * MSEC_PER_SEC,
				      "response data or stream close");
		ret = zsock_recv(fd, buf + total, max_len - total, 0);
		zassert_true(ret >= 0, "recv failed on fd %d (%d)", fd, errno);
		if (ret == 0) {
			break;
		}

		total += (size_t)ret;
	}

	return total;
}

static size_t send_https_get_and_read(uint16_t port, uint8_t *buf, size_t max_len)
{
	char request[96];
	int fd;
	int ret;
	size_t total = 0;

	ret = snprintk(request, sizeof(request),
		       "GET / HTTP/1.1\r\n"
		       "Host: localhost:%u\r\n"
		       "Connection: close\r\n"
		       "\r\n",
		       port);
	zassert_true(ret > 0 && ret < sizeof(request),
		     "Failed to build HTTPS request for port %u", port);

	fd = connect_https_client(port);
	send_all(fd, (const uint8_t *)request, (size_t)ret);

	while (total < max_len) {
		ssize_t recv_len;

		wait_for_socket_event(fd, ZSOCK_POLLIN | ZSOCK_POLLHUP,
				      RECV_TIMEOUT_S * MSEC_PER_SEC,
				      "HTTPS response data or socket close");
		recv_len = zsock_recv(fd, buf + total, max_len - total, 0);
		zassert_true(recv_len >= 0, "HTTPS recv failed (%d)", errno);
		if (recv_len == 0) {
			break;
		}

		total += (size_t)recv_len;
	}

	ret = zsock_close(fd);
	zassert_ok(ret, "Failed to close HTTPS client socket (%d)", errno);

	return total;
}

static void expect_http1_alt_svc_response(uint16_t port, bool expect_alt_svc,
					  uint16_t expected_h3_port,
					  uint16_t unexpected_h3_port)
{
	char response[512];
	char alt_svc[sizeof("Alt-Svc: h3=\":65535\"; ma=-2147483648\r\n")];
	size_t response_len;
	int ret;

	memset(response, 0, sizeof(response));
	response_len = send_https_get_and_read(port, (uint8_t *)response,
					       sizeof(response) - 1);
	zassert_true(response_len > 0, "Empty HTTPS response for port %u", port);
	zassert_not_null(strstr(response, "HTTP/1.1 200 OK\r\n"),
			 "Missing HTTP/1.1 status line for port %u", port);
	zassert_not_null(strstr(response, TEST_STATIC_PAYLOAD),
			 "Missing static payload for port %u", port);

	if (!expect_alt_svc) {
		zassert_is_null(strstr(response, "Alt-Svc: "),
				"Unexpected Alt-Svc header for port %u", port);
		return;
	}

	ret = snprintk(alt_svc, sizeof(alt_svc),
		       "Alt-Svc: h3=\":%u\"; ma=%d\r\n",
		       expected_h3_port,
		       CONFIG_HTTP_SERVER_TO_H3_UPGRADE_MAX_AGE_SECS);
	zassert_true(ret > 0 && ret < sizeof(alt_svc),
		     "Failed to build expected Alt-Svc header");
	zassert_not_null(strstr(response, alt_svc),
			 "Missing expected Alt-Svc header for port %u", port);

	if (unexpected_h3_port != 0U) {
		ret = snprintk(alt_svc, sizeof(alt_svc),
			       "Alt-Svc: h3=\":%u\"; ma=%d\r\n",
			       unexpected_h3_port,
			       CONFIG_HTTP_SERVER_TO_H3_UPGRADE_MAX_AGE_SECS);
		zassert_true(ret > 0 && ret < sizeof(alt_svc),
			     "Failed to build unexpected Alt-Svc header");
		zassert_is_null(strstr(response, alt_svc),
				"Unexpected Alt-Svc header for port %u", port);
	}
}

static void accept_server_uni_streams(bool *seen_control,
				       bool *seen_qpack_encoder,
				       bool *seen_qpack_decoder)
{
	for (int i = 0; i < 3; i++) {
		int fd;

		wait_for_socket_event(client_conn_fd, ZSOCK_POLLIN,
				      RECV_TIMEOUT_S * MSEC_PER_SEC,
				      "server uni stream");
		fd = zsock_accept(client_conn_fd, NULL, NULL);
		zassert_true(fd >= 0, "Failed to accept server uni stream (%d)", errno);
		track_stream_fd(fd);
	}

	*seen_control = true;
	*seen_qpack_encoder = true;
	*seen_qpack_decoder = true;
}

static void start_h3_session(bool split_control_stream)
{
	uint8_t control_buf[8];
	bool seen_control = false;
	bool seen_qpack_encoder = false;
	bool seen_qpack_decoder = false;
	size_t control_len;
	int control_fd;

	control_fd = open_client_stream(QUIC_STREAM_UNIDIRECTIONAL);
	control_len = build_control_settings_stream(control_buf);

	if (split_control_stream) {
		k_msleep(50);
		send_all(control_fd, control_buf, 1);
		k_msleep(50);
		send_all(control_fd, control_buf + 1, control_len - 1);
	} else {
		send_all(control_fd, control_buf, control_len);
	}

	accept_server_uni_streams(&seen_control, &seen_qpack_encoder,
				    &seen_qpack_decoder);

	zassert_true(seen_control, "Did not accept server control stream");
	zassert_true(seen_qpack_encoder, "Did not accept server QPACK encoder stream");
	zassert_true(seen_qpack_decoder, "Did not accept server QPACK decoder stream");
}

static void expect_static_get_response(int stream_fd)
{
	uint8_t response[MAX_RESPONSE_SIZE];
	size_t offset = 0;
	size_t response_len;
	uint64_t frame_type;
	uint64_t frame_len;
	size_t consumed;

	response_len = read_stream_to_eof(stream_fd, response, sizeof(response));
	zassert_true(response_len > 0, "Empty H3 response");

	consumed = h3_decode_varint(response + offset, response_len - offset, &frame_type);
	offset += consumed;
	zassert_equal(frame_type, H3_FRAME_HEADERS, "Expected HEADERS frame");

	consumed = h3_decode_varint(response + offset, response_len - offset, &frame_len);
	offset += consumed;
	zassert_true(frame_len > 0, "Empty HEADERS payload");
	zassert_true(offset + frame_len <= response_len, "HEADERS frame exceeds buffer");
	offset += frame_len;

	consumed = h3_decode_varint(response + offset, response_len - offset, &frame_type);
	offset += consumed;
	zassert_equal(frame_type, H3_FRAME_DATA, "Expected DATA frame");

	consumed = h3_decode_varint(response + offset, response_len - offset, &frame_len);
	offset += consumed;
	zassert_equal(frame_len, strlen(TEST_STATIC_PAYLOAD), "Unexpected DATA length");
	zassert_mem_equal(response + offset, TEST_STATIC_PAYLOAD, frame_len,
			  "Unexpected DATA payload");
}

static void expect_headers_only_response(int stream_fd)
{
	uint8_t response[MAX_RESPONSE_SIZE];
	size_t offset = 0;
	size_t response_len;
	uint64_t frame_type;
	uint64_t frame_len;
	size_t consumed;

	response_len = read_stream_to_eof(stream_fd, response, sizeof(response));
	zassert_true(response_len > 0, "Empty H3 response");

	consumed = h3_decode_varint(response + offset, response_len - offset, &frame_type);
	offset += consumed;
	zassert_equal(frame_type, H3_FRAME_HEADERS, "Expected HEADERS frame");

	consumed = h3_decode_varint(response + offset, response_len - offset, &frame_len);
	offset += consumed;
	zassert_true(frame_len > 0, "Empty HEADERS payload");
	zassert_true(offset + frame_len <= response_len, "HEADERS frame exceeds buffer");
	offset += frame_len;

	zassert_equal(offset, response_len, "Unexpected extra response bytes");
}

static void expect_streaming_response(int stream_fd, const uint8_t *expected_body,
				      size_t expected_body_len)
{
	uint8_t response[MAX_RESPONSE_SIZE];
	uint8_t body[MAX_RESPONSE_SIZE];
	size_t response_len;
	size_t body_len = 0;
	size_t offset = 0;
	int headers_frames = 0;

	response_len = read_stream_to_eof(stream_fd, response, sizeof(response));
	zassert_true(response_len > 0, "Empty H3 response");

	while (offset < response_len) {
		uint64_t frame_type;
		uint64_t frame_len;
		size_t consumed;

		consumed = h3_decode_varint(response + offset, response_len - offset,
					    &frame_type);
		offset += consumed;
		consumed = h3_decode_varint(response + offset, response_len - offset,
					    &frame_len);
		offset += consumed;

		zassert_true(offset + frame_len <= response_len,
			     "Frame exceeds response buffer");

		switch (frame_type) {
		case H3_FRAME_HEADERS:
			headers_frames++;
			break;
		case H3_FRAME_DATA:
			zassert_true(body_len + frame_len <= sizeof(body),
				     "Streaming body buffer too small");
			memcpy(body + body_len, response + offset, frame_len);
			body_len += frame_len;
			break;
		default:
			zassert_unreachable("Unexpected frame type 0x%llx", frame_type);
			break;
		}

		offset += frame_len;
	}

	zassert_equal(headers_frames, 1, "Expected exactly one HEADERS frame");
	zassert_equal(body_len, expected_body_len, "Unexpected streamed body length");
	zassert_mem_equal(body, expected_body, expected_body_len,
			  "Unexpected streamed body payload");
}

static void send_get_and_expect_static(bool split_headers_frame)
{
	uint8_t request[32];
	size_t request_len;
	int stream_fd;
	int ret;

	stream_fd = open_client_stream(QUIC_STREAM_BIDIRECTIONAL);
	request_len = build_headers_frame(request, HTTP_GET, "/");

	if (split_headers_frame) {
		struct zsock_pollfd pfd = {
			.fd = stream_fd,
			.events = ZSOCK_POLLIN | ZSOCK_POLLHUP,
		};

		send_all(stream_fd, request, 1);

		ret = zsock_poll(&pfd, 1, SHORT_POLL_TIMEOUT_MS);
		zassert_equal(ret, 0, "Unexpected response before full HEADERS frame");

		send_all(stream_fd, request + 1, request_len - 1);
	} else {
		send_all(stream_fd, request, request_len);
	}

	expect_static_get_response(stream_fd);

	ret = zsock_close(stream_fd);
	zassert_ok(ret, "Failed to close GET stream (%d)", errno);
	untrack_stream_fd(stream_fd);
}

static void *http_server_h3_setup(void)
{
	int ret;

	setup_tls_credentials_once();

	ret = http_server_start();
	zassert_ok(ret, "Failed to start HTTP server (%d)", ret);

	return NULL;
}

static void http_server_h3_before(void *fixture)
{
	ARG_UNUSED(fixture);

	memset(dynamic_payload, 0, sizeof(dynamic_payload));
	dynamic_bytes_received = 0;
	dynamic_more_seen = false;
	dynamic_final_seen = false;
	dynamic_complete = false;
	tracked_stream_count = 0;
	for (size_t i = 0; i < ARRAY_SIZE(tracked_stream_fds); i++) {
		tracked_stream_fds[i] = -1;
	}

	connect_h3_client();
}

static void http_server_h3_after(void *fixture)
{
	ARG_UNUSED(fixture);

	for (size_t i = 0; i < tracked_stream_count; i++) {
		if (tracked_stream_fds[i] < 0) {
			continue;
		}

		(void)zsock_close(tracked_stream_fds[i]);
	}

	tracked_stream_count = 0;

	if (client_conn_fd >= 0) {
		(void)quic_connection_close(client_conn_fd);
		client_conn_fd = -1;
	}

	k_msleep(100);
}

static void http_server_h3_teardown(void *fixture)
{
	ARG_UNUSED(fixture);

	(void)http_server_stop();
}

ZTEST(server_h3_tests, test_h3_accepts_server_uni_streams_and_static_get)
{
	start_h3_session(false);
	send_get_and_expect_static(false);
}

ZTEST(server_h3_tests, test_h3_post_waits_for_data_without_closing_stream)
{
	static const uint8_t payload[] = TEST_DYNAMIC_POST_PAYLOAD;
	uint8_t headers[32];
	uint8_t data_frame[64];
	size_t headers_len;
	size_t data_len;
	struct zsock_pollfd pfd;
	int stream_fd;
	int ret;

	start_h3_session(false);

	stream_fd = open_client_stream(QUIC_STREAM_BIDIRECTIONAL);
	headers_len = build_headers_frame(headers, HTTP_POST, "/dynamic");

	send_all(stream_fd, headers, headers_len);

	pfd.fd = stream_fd;
	pfd.events = ZSOCK_POLLIN | ZSOCK_POLLHUP;
	pfd.revents = 0;

	ret = zsock_poll(&pfd, 1, SHORT_POLL_TIMEOUT_MS);
	zassert_equal(ret, 0, "POST HEADERS-only request should stay open");
	zassert_equal(dynamic_bytes_received, 0, "Unexpected DATA before DATA frame");

	data_len = build_data_frame(data_frame, payload, sizeof(payload) - 1);
	send_all(stream_fd, data_frame, data_len);

	pfd.revents = 0;
	ret = zsock_poll(&pfd, 1, SHORT_POLL_TIMEOUT_MS);
	zassert_false(ret > 0 && (pfd.revents & ZSOCK_POLLHUP),
		      "POST DATA without FIN unexpectedly closed the stream (events 0x%x)",
		      pfd.revents);

	zassert_true(dynamic_more_seen, "Dynamic callback did not observe DATA_MORE");
	zassert_equal(dynamic_bytes_received, sizeof(payload) - 1,
		      "Dynamic callback received wrong byte count");
	zassert_mem_equal(dynamic_payload, payload, sizeof(payload) - 1,
			  "Dynamic callback received wrong payload");
	zassert_false(dynamic_final_seen, "Unexpected final callback before FIN");
	zassert_false(dynamic_complete, "Unexpected dynamic transaction completion");

	shutdown_stream_send(stream_fd);
	expect_headers_only_response(stream_fd);

	zassert_true(dynamic_final_seen, "Dynamic callback did not observe DATA_FINAL");
	zassert_true(dynamic_complete, "Dynamic callback did not observe completion");

	ret = zsock_close(stream_fd);
	zassert_ok(ret, "Failed to close POST stream (%d)", errno);
	untrack_stream_fd(stream_fd);
}

ZTEST(server_h3_tests, test_h3_partial_headers_frame_waits_for_more_data)
{
	start_h3_session(false);
	send_get_and_expect_static(true);
}

ZTEST(server_h3_tests, test_h3_partial_headers_are_isolated_per_stream)
{
	uint8_t headers[32];
	size_t headers_len;
	struct zsock_pollfd pfd;
	int stream_a_fd;
	int stream_b_fd;
	int ret;

	start_h3_session(false);

	headers_len = build_headers_frame(headers, HTTP_GET, "/");

	stream_a_fd = open_client_stream(QUIC_STREAM_BIDIRECTIONAL);
	send_all(stream_a_fd, headers, 1);

	pfd.fd = stream_a_fd;
	pfd.events = ZSOCK_POLLIN | ZSOCK_POLLHUP;
	pfd.revents = 0;

	ret = zsock_poll(&pfd, 1, SHORT_POLL_TIMEOUT_MS);
	zassert_equal(ret, 0, "Partial HEADERS stream should still be waiting");

	stream_b_fd = open_client_stream(QUIC_STREAM_BIDIRECTIONAL);
	send_all(stream_b_fd, headers, headers_len);
	expect_static_get_response(stream_b_fd);

	ret = zsock_close(stream_b_fd);
	zassert_ok(ret, "Failed to close second GET stream (%d)", errno);
	untrack_stream_fd(stream_b_fd);

	send_all(stream_a_fd, headers + 1, headers_len - 1);
	expect_static_get_response(stream_a_fd);

	ret = zsock_close(stream_a_fd);
	zassert_ok(ret, "Failed to close first GET stream (%d)", errno);
	untrack_stream_fd(stream_a_fd);
}

ZTEST(server_h3_tests, test_h3_headers_only_fin_completes_dynamic_request)
{
	uint8_t headers[32];
	size_t headers_len;
	struct zsock_pollfd pfd;
	int stream_fd;
	int ret;

	start_h3_session(false);

	stream_fd = open_client_stream(QUIC_STREAM_BIDIRECTIONAL);
	headers_len = build_headers_frame(headers, HTTP_POST, "/dynamic");

	send_all(stream_fd, headers, headers_len);

	pfd.fd = stream_fd;
	pfd.events = ZSOCK_POLLIN | ZSOCK_POLLHUP;
	pfd.revents = 0;

	ret = zsock_poll(&pfd, 1, SHORT_POLL_TIMEOUT_MS);
	zassert_equal(ret, 0, "POST HEADERS-only request should stay open");

	shutdown_stream_send(stream_fd);
	expect_headers_only_response(stream_fd);

	zassert_false(dynamic_more_seen, "Unexpected DATA_MORE callback");
	zassert_true(dynamic_final_seen, "Dynamic callback did not observe DATA_FINAL");
	zassert_true(dynamic_complete, "Dynamic callback did not observe completion");
	zassert_equal(dynamic_bytes_received, 0, "Unexpected DATA bytes for empty body");

	ret = zsock_close(stream_fd);
	zassert_ok(ret, "Failed to close POST stream (%d)", errno);
	untrack_stream_fd(stream_fd);
}

ZTEST(server_h3_tests, test_h3_interleaved_streams_keep_request_state_separate)
{
	static const uint8_t payload[] = TEST_DYNAMIC_POST_PAYLOAD;
	uint8_t get_headers[32];
	uint8_t post_headers[32];
	uint8_t data_frame[64];
	size_t get_headers_len;
	size_t post_headers_len;
	size_t data_len;
	struct zsock_pollfd pfd;
	int post_stream_fd;
	int get_stream_fd;
	int ret;

	start_h3_session(false);

	post_stream_fd = open_client_stream(QUIC_STREAM_BIDIRECTIONAL);
	post_headers_len = build_headers_frame(post_headers, HTTP_POST, "/dynamic");
	send_all(post_stream_fd, post_headers, post_headers_len);

	pfd.fd = post_stream_fd;
	pfd.events = ZSOCK_POLLIN | ZSOCK_POLLHUP;
	pfd.revents = 0;

	ret = zsock_poll(&pfd, 1, SHORT_POLL_TIMEOUT_MS);
	zassert_equal(ret, 0, "POST HEADERS-only request should stay open");

	get_stream_fd = open_client_stream(QUIC_STREAM_BIDIRECTIONAL);
	get_headers_len = build_headers_frame(get_headers, HTTP_GET, "/");
	send_all(get_stream_fd, get_headers, get_headers_len);
	expect_static_get_response(get_stream_fd);

	ret = zsock_close(get_stream_fd);
	zassert_ok(ret, "Failed to close interleaved GET stream (%d)", errno);
	untrack_stream_fd(get_stream_fd);

	data_len = build_data_frame(data_frame, payload, sizeof(payload) - 1);
	send_all(post_stream_fd, data_frame, data_len);
	shutdown_stream_send(post_stream_fd);
	expect_headers_only_response(post_stream_fd);

	zassert_true(dynamic_more_seen, "Dynamic callback did not observe DATA_MORE");
	zassert_true(dynamic_final_seen, "Dynamic callback did not observe DATA_FINAL");
	zassert_true(dynamic_complete, "Dynamic callback did not observe completion");
	zassert_equal(dynamic_bytes_received, sizeof(payload) - 1,
		      "Dynamic callback received wrong byte count");
	zassert_mem_equal(dynamic_payload, payload, sizeof(payload) - 1,
			  "Dynamic callback received wrong payload");

	ret = zsock_close(post_stream_fd);
	zassert_ok(ret, "Failed to close interleaved POST stream (%d)", errno);
	untrack_stream_fd(post_stream_fd);
}

ZTEST(server_h3_tests, test_h3_reuses_connection_for_multiple_get_streams)
{
	start_h3_session(false);

	for (int i = 0; i < 3; i++) {
		send_get_and_expect_static(false);
	}
}

ZTEST(server_h3_tests, test_h3_multiple_data_callbacks_send_headers_once)
{
	static const uint8_t payload_a[] = "chunk-a";
	static const uint8_t payload_b[] = "chunk-b";
	static const uint8_t expected_body[] = "chunk-achunk-b";
	uint8_t headers[32];
	uint8_t data_frame[64];
	size_t headers_len;
	size_t data_len;
	int stream_fd;
	int ret;

	start_h3_session(false);

	stream_fd = open_client_stream(QUIC_STREAM_BIDIRECTIONAL);
	headers_len = build_headers_frame(headers, HTTP_POST, "/streaming");

	send_all(stream_fd, headers, headers_len);

	data_len = build_data_frame(data_frame, payload_a, sizeof(payload_a) - 1);
	send_all(stream_fd, data_frame, data_len);

	data_len = build_data_frame(data_frame, payload_b, sizeof(payload_b) - 1);
	send_all(stream_fd, data_frame, data_len);

	shutdown_stream_send(stream_fd);
	expect_streaming_response(stream_fd, expected_body, sizeof(expected_body) - 1);

	ret = zsock_close(stream_fd);
	zassert_ok(ret, "Failed to close streaming POST stream (%d)", errno);
	untrack_stream_fd(stream_fd);
}

ZTEST(server_h3_tests, test_http1_alt_svc_is_service_scoped)
{
	expect_http1_alt_svc_response(SERVER_PORT, true, SERVER_PORT,
				      ALT_SVC_DISABLED_PORT);
	expect_http1_alt_svc_response(ALT_SVC_DISABLED_PORT, false, 0U, 0U);
}

ZTEST_SUITE(server_h3_tests, NULL, http_server_h3_setup,
	    http_server_h3_before, http_server_h3_after, http_server_h3_teardown);
