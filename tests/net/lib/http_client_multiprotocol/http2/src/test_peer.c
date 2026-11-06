/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Tests against a scripted peer, for frames the real server never produces. */

#include <strings.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#include "common.h"
#include "peer.h"

#define PEER_HOST "example.test"
#define BODY_BUF_SIZE (50 * 1024)
#define PATTERN_SIZE 4096
#define WINDOW_TEST_BODY_SIZE (12 * PATTERN_SIZE)
#define SHORT_TIMEOUT_MS 300
#define UPLOAD_SIZE 1000
#define BIG_UPLOAD_SIZE 100000
#define INITIAL_WINDOW 65535
#define BLOCKED_CHECK_MS 200
#define MAX_STREAMS CONFIG_HTTP_CLIENT_MAX_STREAMS

#define FLAG_ACK 0x01
#define RST_STREAM_LEN 4
#define GOAWAY_DEBUG_LEN 0

static struct http_client client;
static struct http_client_transport_tcp_ctx tcp_ctx;
static struct http_client_session_config cfg;
static struct test_response rsp;
static uint8_t body_buf[BODY_BUF_SIZE];
static uint8_t pattern[PATTERN_SIZE];
static uint8_t upload[UPLOAD_SIZE];
static uint8_t big_upload[BIG_UPLOAD_SIZE];
static struct peer_frame frame;
static struct peer_header hdrs[PEER_MAX_HEADERS];
/* Requests must outlive the call in asynchronous mode. */
static struct http_client_request req;
static int listen_fd = -1;
static int conn = -1;

/* A request blocked on flow control is sent from a thread of its own, while
 * the test thread plays the peer.
 */
static K_THREAD_STACK_DEFINE(req_stack, 4096 + CONFIG_TEST_EXTRA_STACK_SIZE);
static struct k_thread req_thread;
static bool req_thread_running;
static int32_t req_timeout;
static int req_result;

/* A second sender, for the cases with two requests in flight from two threads. */
static K_THREAD_STACK_DEFINE(req2_stack, 4096 + CONFIG_TEST_EXTRA_STACK_SIZE);
static struct k_thread req2_thread;
static bool req2_thread_running;
static struct http_client_request req2;
static struct test_response rsp2;
static uint8_t body2_buf[256];
static int req2_result;

static const struct http_header ok_headers[] = {
	{ ":status", "200" },
	{ "server", "peer" },
};

/* Open a session and accept it, leaving the HTTP/2 handshake to the caller. */
static void peer_open(bool async)
{
	struct net_sockaddr_in6 *sa = (struct net_sockaddr_in6 *)&cfg.server_addr;
	int ret;

	memset(&tcp_ctx, 0, sizeof(tcp_ctx));
	ret = http_client_init_tcp(&client, HTTP_CLIENT_VERSION_2, &tcp_ctx);
	zassert_ok(ret, "client init failed (%d)", ret);

	memset(&cfg, 0, sizeof(cfg));
	sa->sin6_family = NET_AF_INET6;
	sa->sin6_port = net_htons(PEER_PORT);
	sa->sin6_addr = net_in6addr_loopback;
	cfg.addrlen = sizeof(*sa);
	cfg.host = PEER_HOST;
	cfg.async = async;

	ret = http_client_connect(&client, &cfg);
	zassert_ok(ret, "connect failed (%d)", ret);

	conn = peer_accept(listen_fd);
	zassert_true(conn >= 0, "accept failed (%d)", conn);
}

static void peer_connect(bool async)
{
	int ret;

	peer_open(async);

	ret = peer_handshake(conn);
	zassert_ok(ret, "handshake failed (%d)", ret);
}

/* Like peer_connect(), but the client's SETTINGS stay unacknowledged. */
static void peer_connect_unacked(bool async)
{
	int ret;

	peer_open(async);

	ret = peer_recv_preface(conn);
	zassert_ok(ret, "preface failed (%d)", ret);
	ret = peer_send_frame(conn, HTTP2_SETTINGS_FRAME, 0, 0, NULL, 0);
	zassert_ok(ret, "send SETTINGS failed (%d)", ret);
}

static int start_request(struct http_client_request *r, enum http_method method,
			 const char *url, struct test_response *tr, int32_t timeout)
{
	r->method = method;
	r->url = url;
	r->response_cb = test_response_cb;

	return http_client_send_request(&client, r, timeout, tr);
}

static int start_get(const char *url, struct test_response *tr)
{
	memset(&req, 0, sizeof(req));

	return start_request(&req, HTTP_GET, url, tr, TEST_REQUEST_TIMEOUT_MS);
}

/* Read the client's HEADERS frame for stream_id and decode it. */
static int expect_request(uint32_t stream_id)
{
	int ret, count;

	ret = peer_recv_frame_type(conn, HTTP2_HEADERS_FRAME, &frame);
	zassert_ok(ret, "no HEADERS from client (%d)", ret);
	zassert_equal(frame.stream_id, stream_id, "HEADERS on stream %u", frame.stream_id);
	zassert_true(frame.flags & HTTP2_FLAG_END_HEADERS, "END_HEADERS missing");

	count = peer_decode_headers(frame.payload, frame.len, hdrs, ARRAY_SIZE(hdrs));
	zassert_true(count > 0, "header decode failed (%d)", count);

	return count;
}

/* Read a request header block that may span HEADERS and CONTINUATION frames. */
static int expect_request_block(uint32_t stream_id, int *frames)
{
	static uint8_t block[1024];
	size_t len = 0;
	int ret, count;

	*frames = 0;

	ret = peer_recv_frame_type(conn, HTTP2_HEADERS_FRAME, &frame);
	zassert_ok(ret, "no HEADERS from client (%d)", ret);

	while (true) {
		zassert_equal(frame.stream_id, stream_id, "frame on stream %u", frame.stream_id);
		zassert_true(len + frame.len <= sizeof(block), "header block too large");
		memcpy(block + len, frame.payload, frame.len);
		len += frame.len;
		(*frames)++;

		if (frame.flags & HTTP2_FLAG_END_HEADERS) {
			break;
		}

		ret = peer_recv_frame(conn, &frame);
		zassert_ok(ret, "no CONTINUATION from client (%d)", ret);
		zassert_equal(frame.type, HTTP2_CONTINUATION_FRAME, "frame type %u after HEADERS",
			      frame.type);
	}

	count = peer_decode_headers(block, len, hdrs, ARRAY_SIZE(hdrs));
	zassert_true(count > 0, "header decode failed (%d)", count);

	return count;
}

static void send_ok(uint32_t stream_id, uint8_t extra_flags)
{
	int ret;

	ret = peer_send_headers(conn, stream_id, HTTP2_FLAG_END_HEADERS | extra_flags,
				ok_headers, ARRAY_SIZE(ok_headers));
	zassert_ok(ret, "send HEADERS failed (%d)", ret);
}

static void wait_done(struct test_response *tr)
{
	int ret = test_response_wait(tr, K_SECONDS(5));

	zassert_ok(ret, "transaction did not complete");
}

static void assert_aborted(const struct test_response *tr, int error)
{
	zassert_true(tr->aborted, "abort not reported");
	zassert_equal(tr->error, error, "abort reason %d, expected %d", tr->error, error);
	zassert_equal(tr->final_count, 0, "final reported after abort");
}

/* The client failed the connection: the request was aborted with error, a
 * GOAWAY with goaway_error went out and no request can be started.
 */
static void assert_connection_error_code(int error, uint32_t goaway_error)
{
	int ret;

	wait_done(&rsp);
	assert_aborted(&rsp, error);

	ret = peer_drain(conn);
	zassert_true(peer_stats.goaway_seen, "no GOAWAY from client (%d frames)", ret);
	zassert_equal(peer_stats.goaway_error, goaway_error, "GOAWAY error code %u",
		      peer_stats.goaway_error);

	ret = start_get("/again", &rsp);
	zassert_equal(ret, -ENOTCONN, "session still open (%d)", ret);
}

static void assert_connection_error(int error)
{
	/* A malformed message is a protocol error, anything else is internal. */
	assert_connection_error_code(error, (error == -EBADMSG) ? HTTP2_ERROR_PROTOCOL_ERROR :
								  HTTP2_ERROR_INTERNAL_ERROR);
}

/* The client failed stream_id only: the request was aborted with result and
 * a RST_STREAM with error went out, while the connection stays open.
 */
static void assert_stream_error(uint32_t stream_id, uint32_t error, int result)
{
	int ret;

	wait_done(&rsp);
	assert_aborted(&rsp, result);

	ret = peer_recv_frame_type(conn, HTTP2_RST_STREAM_FRAME, &frame);
	zassert_ok(ret, "no RST_STREAM from client (%d)", ret);
	zassert_equal(peer_stats.rst_stream_id, stream_id, "RST_STREAM on stream %u",
		      peer_stats.rst_stream_id);
	zassert_equal(peer_stats.rst_error, error, "RST_STREAM error %u", peer_stats.rst_error);
	zassert_false(peer_stats.goaway_seen, "connection closed");
}

/* A fresh GET on the open connection, expected on stream_id, succeeds. */
static void assert_session_usable(uint32_t stream_id)
{
	int ret;

	test_response_init(&rsp, body_buf, sizeof(body_buf));
	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET after stream error failed (%d)", ret);
	expect_request(stream_id);
	send_ok(stream_id, HTTP2_FLAG_END_STREAM);
	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
}

static void *peer_setup(void)
{
	for (size_t i = 0; i < sizeof(pattern); i++) {
		pattern[i] = (uint8_t)(i * 3 + 1);
	}

	for (size_t i = 0; i < sizeof(upload); i++) {
		upload[i] = (uint8_t)(i * 5 + 2);
	}

	for (size_t i = 0; i < sizeof(big_upload); i++) {
		big_upload[i] = (uint8_t)(i * 11 + 3);
	}

	listen_fd = peer_listen();
	zassert_true(listen_fd >= 0, "listen failed (%d)", listen_fd);

	return NULL;
}

static void peer_teardown(void *fixture)
{
	ARG_UNUSED(fixture);

	if (listen_fd >= 0) {
		zsock_close(listen_fd);
		listen_fd = -1;
	}
}

static void peer_before(void *fixture)
{
	ARG_UNUSED(fixture);

	test_response_init(&rsp, body_buf, sizeof(body_buf));
	peer_stats_reset();
	conn = -1;
}

static void peer_after(void *fixture)
{
	ARG_UNUSED(fixture);

	(void)http_client_disconnect(&client);

	if (req_thread_running) {
		/* The disconnect above unblocks it. */
		if (k_thread_join(&req_thread, K_SECONDS(2)) != 0) {
			k_thread_abort(&req_thread);
		}

		req_thread_running = false;
	}

	if (req2_thread_running) {
		if (k_thread_join(&req2_thread, K_SECONDS(2)) != 0) {
			k_thread_abort(&req2_thread);
		}

		req2_thread_running = false;
	}

	if (conn >= 0) {
		zsock_close(conn);
		conn = -1;
	}

	k_yield();
}

static void req_thread_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	req_result = http_client_send_request(&client, &req, req_timeout, &rsp);
}

/* Send the request prepared in req from the request thread. */
static void start_request_thread(int32_t timeout)
{
	req_timeout = timeout;
	if (req.response_cb == NULL) {
		req.response_cb = test_response_cb;
	}

	k_thread_create(&req_thread, req_stack, K_THREAD_STACK_SIZEOF(req_stack), req_thread_fn,
			NULL, NULL, NULL, K_PRIO_PREEMPT(8), 0, K_NO_WAIT);
	req_thread_running = true;
}

static int join_request_thread(void)
{
	int ret = k_thread_join(&req_thread, K_SECONDS(5));

	zassert_ok(ret, "request did not return");
	req_thread_running = false;

	return req_result;
}

static void req2_thread_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	req2_result = http_client_send_request(&client, &req2, TEST_REQUEST_TIMEOUT_MS, &rsp2);
}

/* Send the request prepared in req2 from a second thread. */
static void start_request2_thread(void)
{
	test_response_init(&rsp2, body2_buf, sizeof(body2_buf));
	if (req2.response_cb == NULL) {
		req2.response_cb = test_response_cb;
	}

	k_thread_create(&req2_thread, req2_stack, K_THREAD_STACK_SIZEOF(req2_stack),
			req2_thread_fn, NULL, NULL, NULL, K_PRIO_PREEMPT(8), 0, K_NO_WAIT);
	req2_thread_running = true;
}

static int join_request2_thread(void)
{
	int ret = k_thread_join(&req2_thread, K_SECONDS(5));

	zassert_ok(ret, "second request did not return");
	req2_thread_running = false;

	return req2_result;
}

static void start_big_upload(int32_t timeout)
{
	memset(&req, 0, sizeof(req));
	req.method = HTTP_PUT;
	req.url = "/upload";
	req.payload = big_upload;
	req.payload_len = sizeof(big_upload);

	start_request_thread(timeout);
	expect_request(1);
}

/* Read DATA frames of stream 1 up to the given total number of bytes, checking
 * them against the expected content. The client must not send more than that.
 */
static void recv_data(const uint8_t *expected, size_t *got, size_t until)
{
	while (*got < until && !peer_stats.end_stream_seen) {
		int ret = peer_recv_frame_type(conn, HTTP2_DATA_FRAME, &frame);

		zassert_ok(ret, "no DATA from client after %zu bytes (%d)", *got, ret);
		zassert_equal(frame.stream_id, 1, "DATA on stream %u", frame.stream_id);
		zassert_true(*got + frame.len <= until, "client sent %zu bytes, allowed %zu",
			     *got + frame.len, until);
		zassert_mem_equal(frame.payload, expected + *got, frame.len,
				  "upload mismatch at %zu", *got);
		*got += frame.len;
	}
}

/* The client is expected to be blocked on flow control: no DATA may arrive for
 * a while. Other frames may, e. g. the acknowledgment of the handshake SETTINGS
 * that the client only gets to process once it stops sending.
 */
static void assert_client_blocked(void)
{
	size_t data_bytes = peer_stats.data_bytes;
	int ret;

	ret = peer_set_recv_timeout(conn, BLOCKED_CHECK_MS);
	zassert_ok(ret, "failed to set the receive timeout (%d)", ret);

	(void)peer_drain(conn);
	zassert_equal(peer_stats.data_bytes, data_bytes, "client sent %zu bytes while blocked",
		      peer_stats.data_bytes - data_bytes);

	ret = peer_set_recv_timeout(conn, PEER_RECV_TIMEOUT_MS);
	zassert_ok(ret, "failed to restore the receive timeout (%d)", ret);
}

static void send_window_update(uint32_t stream_id, uint32_t increment)
{
	uint8_t payload[HTTP2_WINDOW_UPDATE_FRAME_LEN];
	int ret;

	sys_put_be32(increment, payload);
	ret = peer_send_frame(conn, HTTP2_WINDOW_UPDATE_FRAME, 0, stream_id, payload,
			      sizeof(payload));
	zassert_ok(ret, "send WINDOW_UPDATE failed (%d)", ret);
}

static void send_setting(uint16_t id, uint32_t value)
{
	uint8_t payload[6];
	int ret;

	sys_put_be16(id, payload);
	sys_put_be32(value, payload + sizeof(uint16_t));
	ret = peer_send_frame(conn, HTTP2_SETTINGS_FRAME, 0, 0, payload, sizeof(payload));
	zassert_ok(ret, "send SETTINGS failed (%d)", ret);
}

static void send_goaway(uint32_t last_stream_id, uint32_t error, size_t debug_len)
{
	static uint8_t payload[HTTP2_GOAWAY_FRAME_LEN + 400];
	int ret;

	zassert_true(debug_len <= sizeof(payload) - HTTP2_GOAWAY_FRAME_LEN, "debug data too long");

	sys_put_be32(last_stream_id, payload);
	sys_put_be32(error, payload + sizeof(uint32_t));
	memset(payload + HTTP2_GOAWAY_FRAME_LEN, 'd', debug_len);

	ret = peer_send_frame(conn, HTTP2_GOAWAY_FRAME, 0, 0, payload,
			      HTTP2_GOAWAY_FRAME_LEN + debug_len);
	zassert_ok(ret, "send GOAWAY failed (%d)", ret);
}

/* The client is expected to have closed the session by itself, regularly. */
static void assert_session_closed(void)
{
	int ret;

	(void)peer_drain(conn);
	zassert_true(peer_stats.goaway_seen, "no GOAWAY from client");
	zassert_equal(peer_stats.goaway_error, HTTP2_ERROR_NO_ERROR, "GOAWAY error %u",
		      peer_stats.goaway_error);

	test_response_init(&rsp, body_buf, sizeof(body_buf));
	ret = start_get("/", &rsp);
	zassert_equal(ret, -ENOTCONN, "session still open (%d)", ret);
}

/* Wait until the client acknowledged count SETTINGS frames in total. */
static void expect_settings_acks(int count)
{
	while (peer_stats.settings_ack_frames < count) {
		int ret = peer_recv_frame_type(conn, HTTP2_SETTINGS_FRAME, &frame);

		zassert_ok(ret, "no SETTINGS ACK from client (%d)", ret);
		zassert_true(frame.flags & FLAG_ACK, "SETTINGS without ACK flag");
	}
}

static void upload_flow_control(bool async)
{
	size_t got = 0;
	int ret;

	peer_connect(async);
	start_big_upload(TEST_REQUEST_TIMEOUT_MS);

	/* The initial windows allow exactly this much. */
	recv_data(big_upload, &got, INITIAL_WINDOW);
	zassert_equal(got, INITIAL_WINDOW, "received %zu bytes", got);
	assert_client_blocked();

	/* Credit on the connection alone does not unblock the stream. */
	send_window_update(0, INITIAL_WINDOW);
	assert_client_blocked();

	send_window_update(1, INITIAL_WINDOW);
	recv_data(big_upload, &got, sizeof(big_upload));
	zassert_equal(got, sizeof(big_upload), "received %zu bytes", got);
	zassert_true(peer_stats.end_stream_seen, "END_STREAM missing");

	send_ok(1, HTTP2_FLAG_END_STREAM);

	ret = join_request_thread();
	zassert_ok(ret, "PUT failed (%d)", ret);
	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
	zassert_equal(rsp.final_count, 1, "final reported %d times", rsp.final_count);
	zassert_equal(rsp.abort_count, 0, "abort reported");
}

ZTEST(http2_client_peer, test_peer_upload_flow_control_sync)
{
	upload_flow_control(false);
}

ZTEST(http2_client_peer, test_peer_upload_flow_control_async)
{
	upload_flow_control(true);
}

ZTEST(http2_client_peer, test_peer_upload_initial_window_change)
{
	size_t got = 0;
	int ret;

	peer_connect(true);

	/* A small initial window applies to streams opened afterwards. */
	send_setting(HTTP2_SETTINGS_INITIAL_WINDOW_SIZE, 1000);
	expect_settings_acks(2);

	memset(&req, 0, sizeof(req));
	req.method = HTTP_PUT;
	req.url = "/upload";
	req.payload = big_upload;
	req.payload_len = 3000;
	start_request_thread(TEST_REQUEST_TIMEOUT_MS);
	expect_request(1);

	recv_data(big_upload, &got, 1000);
	zassert_equal(got, 1000, "received %zu bytes", got);
	assert_client_blocked();

	/* Raising it credits the difference to the open stream. */
	send_setting(HTTP2_SETTINGS_INITIAL_WINDOW_SIZE, 4000);
	recv_data(big_upload, &got, 3000);
	zassert_equal(got, 3000, "received %zu bytes", got);
	zassert_true(peer_stats.end_stream_seen, "END_STREAM missing");

	send_ok(1, HTTP2_FLAG_END_STREAM);

	ret = join_request_thread();
	zassert_ok(ret, "PUT failed (%d)", ret);
	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
}

ZTEST(http2_client_peer, test_peer_upload_timeout_while_blocked)
{
	size_t got = 0;
	int ret;

	peer_connect(false);
	start_big_upload(SHORT_TIMEOUT_MS);

	recv_data(big_upload, &got, INITIAL_WINDOW);

	/* No credit is ever granted, so the request deadline ends the wait. */
	ret = join_request_thread();
	zassert_equal(ret, -ETIMEDOUT, "unexpected result %d", ret);
	assert_aborted(&rsp, -ETIMEDOUT);
	zassert_equal(rsp.abort_count, 1, "abort reported %d times", rsp.abort_count);

	ret = peer_recv_frame_type(conn, HTTP2_RST_STREAM_FRAME, &frame);
	zassert_ok(ret, "no RST_STREAM after timeout (%d)", ret);
	zassert_equal(peer_stats.rst_error, HTTP2_ERROR_CANCEL, "RST_STREAM error %u",
		      peer_stats.rst_error);
}

ZTEST(http2_client_peer, test_peer_upload_early_response_then_close)
{
	size_t got = 0;
	int ret;

	peer_connect(true);
	start_big_upload(TEST_REQUEST_TIMEOUT_MS);

	recv_data(big_upload, &got, INITIAL_WINDOW);

	/* The response and a GOAWAY arrive together while the upload waits
	 * for credit, and end the session: the sender finds it over and
	 * leaves, and the transport gets closed, with nothing in the way of
	 * the receive context.
	 */
	send_ok(1, HTTP2_FLAG_END_STREAM);
	send_goaway(1, HTTP2_ERROR_NO_ERROR, 0);

	ret = join_request_thread();
	zassert_ok(ret, "PUT failed (%d)", ret);
	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);

	for (int i = 0; i < 100 && tcp_ctx.sock != -1; i++) {
		k_sleep(K_MSEC(10));
	}

	zassert_equal(client.session.state, HTTP_CLIENT_SESSION_CLOSED, "session state %d",
		      client.session.state);
	zassert_equal(tcp_ctx.sock, -1, "transport still open");
	(void)peer_drain(conn);
	zassert_true(peer_stats.goaway_seen, "no GOAWAY from client");
}

ZTEST(http2_client_peer, test_peer_upload_reset_while_blocked)
{
	uint8_t error[RST_STREAM_LEN];
	size_t got = 0;
	int ret;

	peer_connect(true);
	start_big_upload(TEST_REQUEST_TIMEOUT_MS);

	recv_data(big_upload, &got, INITIAL_WINDOW);

	sys_put_be32(HTTP2_ERROR_REFUSED_STREAM, error);
	ret = peer_send_frame(conn, HTTP2_RST_STREAM_FRAME, 0, 1, error, sizeof(error));
	zassert_ok(ret, "send RST_STREAM failed (%d)", ret);

	ret = join_request_thread();
	zassert_equal(ret, -ECONNRESET, "unexpected result %d", ret);
	assert_aborted(&rsp, -ECONNRESET);
	zassert_equal(rsp.abort_count, 1, "abort reported %d times", rsp.abort_count);

	/* A stream the peer has reset is not reset again. */
	(void)peer_set_recv_timeout(conn, BLOCKED_CHECK_MS);
	(void)peer_drain(conn);
	zassert_false(peer_stats.rst_stream_seen, "RST_STREAM sent for a reset stream");
}

ZTEST(http2_client_peer, test_peer_upload_early_response)
{
	uint8_t error[RST_STREAM_LEN];
	size_t got = 0;
	int ret;

	peer_connect(false);
	start_big_upload(TEST_REQUEST_TIMEOUT_MS);

	recv_data(big_upload, &got, INITIAL_WINDOW);

	/* The server answers without waiting for the rest of the request, and
	 * may then reset the stream to stop it (RFC 9113, Section 8.1).
	 */
	send_ok(1, HTTP2_FLAG_END_STREAM);
	sys_put_be32(HTTP2_ERROR_NO_ERROR, error);
	ret = peer_send_frame(conn, HTTP2_RST_STREAM_FRAME, 0, 1, error, sizeof(error));
	zassert_ok(ret, "send RST_STREAM failed (%d)", ret);

	ret = join_request_thread();
	zassert_ok(ret, "PUT failed (%d)", ret);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
	zassert_equal(rsp.final_count, 1, "final reported %d times", rsp.final_count);
	zassert_equal(rsp.abort_count, 0, "abort reported");

	/* The client lets the server know that the rest is not coming. */
	ret = peer_recv_frame_type(conn, HTTP2_RST_STREAM_FRAME, &frame);
	zassert_ok(ret, "no RST_STREAM after the early response (%d)", ret);
	zassert_equal(peer_stats.rst_error, HTTP2_ERROR_NO_ERROR, "RST_STREAM error %u",
		      peer_stats.rst_error);
}

ZTEST(http2_client_peer, test_peer_window_update_zero)
{
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	/* An increment of zero is an error of the stream it arrives on
	 * (RFC 9113, Section 6.9).
	 */
	send_window_update(1, 0);

	assert_stream_error(1, HTTP2_ERROR_PROTOCOL_ERROR, -EPROTO);
	assert_session_usable(3);
}

ZTEST(http2_client_peer, test_peer_window_update_zero_connection)
{
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	/* On the connection, it is an error of the connection. */
	send_window_update(0, 0);

	assert_connection_error_code(-EBADMSG, HTTP2_ERROR_PROTOCOL_ERROR);
}

ZTEST(http2_client_peer, test_peer_window_update_overflow_connection)
{
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	/* On top of the initial window this exceeds the largest window. */
	send_window_update(0, 0x7fffffff);

	assert_connection_error_code(-EBADMSG, HTTP2_ERROR_FLOW_CONTROL_ERROR);
}

ZTEST(http2_client_peer, test_peer_window_update_overflow_stream)
{
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	/* On top of the initial window this exceeds the largest window. */
	send_window_update(1, 0x7fffffff);

	/* An error of that stream only. */
	wait_done(&rsp);
	assert_aborted(&rsp, -EPROTO);

	ret = peer_recv_frame_type(conn, HTTP2_RST_STREAM_FRAME, &frame);
	zassert_ok(ret, "no RST_STREAM from client (%d)", ret);
	zassert_equal(peer_stats.rst_stream_id, 1, "RST_STREAM on stream %u",
		      peer_stats.rst_stream_id);
	zassert_equal(peer_stats.rst_error, HTTP2_ERROR_FLOW_CONTROL_ERROR,
		      "RST_STREAM error %u", peer_stats.rst_error);

	test_response_init(&rsp, body_buf, sizeof(body_buf));
	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET after stream error failed (%d)", ret);
	expect_request(3);
	send_ok(3, HTTP2_FLAG_END_STREAM);
	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
}

ZTEST(http2_client_peer, test_peer_settings_invalid_max_frame_size)
{
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	/* Below the smallest value a peer may set. */
	send_setting(HTTP2_SETTINGS_MAX_FRAME_SIZE, 100);

	assert_connection_error(-EBADMSG);
}

ZTEST(http2_client_peer, test_peer_settings_invalid_initial_window)
{
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	send_setting(HTTP2_SETTINGS_INITIAL_WINDOW_SIZE, 0x80000000);

	assert_connection_error_code(-EBADMSG, HTTP2_ERROR_FLOW_CONTROL_ERROR);
}

ZTEST(http2_client_peer, test_peer_settings_enable_push)
{
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	/* A server must not enable push. */
	send_setting(HTTP2_SETTINGS_ENABLE_PUSH, 1);

	assert_connection_error(-EBADMSG);
}

ZTEST(http2_client_peer, test_peer_push_promise)
{
	static const struct http_header headers[] = {
		{ ":method", "GET" },
		{ ":scheme", "http" },
		{ ":authority", "localhost" },
		{ ":path", "/pushed" },
	};
	static uint8_t payload[4 + 64];
	int ret, len;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	/* The client advertised ENABLE_PUSH=0, so a promise is a protocol error. */
	sys_put_be32(2, payload);
	len = peer_encode_headers(payload + 4, sizeof(payload) - 4, headers,
				  ARRAY_SIZE(headers));
	zassert_true(len > 0, "encode failed (%d)", len);
	ret = peer_send_frame(conn, HTTP2_PUSH_PROMISE_FRAME, HTTP2_FLAG_END_HEADERS, 1,
			      payload, 4 + len);
	zassert_ok(ret, "send PUSH_PROMISE failed (%d)", ret);

	assert_connection_error(-EBADMSG);
}

ZTEST(http2_client_peer, test_peer_goaway_idle)
{
	peer_connect(true);

	/* With nothing in flight the session is closed right away. */
	send_goaway(0, HTTP2_ERROR_NO_ERROR, 0);

	assert_session_closed();
}

ZTEST(http2_client_peer, test_peer_goaway_unprocessed)
{
	static struct http_client_request req2, req3;
	static struct test_response rsp2, rsp3;
	static uint8_t buf2[64], buf3[64];
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET 1 failed (%d)", ret);

	memset(&req2, 0, sizeof(req2));
	test_response_init(&rsp2, buf2, sizeof(buf2));
	ret = start_request(&req2, HTTP_GET, "/", &rsp2, TEST_REQUEST_TIMEOUT_MS);
	zassert_ok(ret, "GET 2 failed (%d)", ret);

	expect_request(1);
	expect_request(3);

	/* The server processed stream 1 only. */
	send_goaway(1, HTTP2_ERROR_NO_ERROR, 0);

	wait_done(&rsp2);
	assert_aborted(&rsp2, -ESHUTDOWN);

	/* New requests are refused while the first one finishes. */
	memset(&req3, 0, sizeof(req3));
	test_response_init(&rsp3, buf3, sizeof(buf3));
	ret = start_request(&req3, HTTP_GET, "/", &rsp3, TEST_REQUEST_TIMEOUT_MS);
	zassert_equal(ret, -ESHUTDOWN, "request on a closing session returned %d", ret);
	assert_aborted(&rsp3, -ESHUTDOWN);

	send_ok(1, HTTP2_FLAG_END_STREAM);
	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
	zassert_equal(rsp.final_count, 1, "final reported %d times", rsp.final_count);
	zassert_equal(rsp.abort_count, 0, "abort reported");

	/* An unprocessed stream needs no reset. */
	assert_session_closed();
	zassert_false(peer_stats.rst_stream_seen, "RST_STREAM sent for an unprocessed stream");
}

ZTEST(http2_client_peer, test_peer_goaway_sync)
{
	int ret;

	peer_connect(false);

	memset(&req, 0, sizeof(req));
	req.method = HTTP_GET;
	req.url = "/";
	start_request_thread(TEST_REQUEST_TIMEOUT_MS);
	expect_request(1);

	/* The request in flight was accepted and completes normally. */
	send_goaway(1, HTTP2_ERROR_NO_ERROR, 0);
	send_ok(1, HTTP2_FLAG_END_STREAM);

	ret = join_request_thread();
	zassert_ok(ret, "GET failed (%d)", ret);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
	zassert_equal(rsp.final_count, 1, "final reported %d times", rsp.final_count);

	assert_session_closed();
}

ZTEST(http2_client_peer, test_peer_goaway_sync_unprocessed)
{
	int ret;

	peer_connect(false);

	memset(&req, 0, sizeof(req));
	req.method = HTTP_GET;
	req.url = "/";
	start_request_thread(TEST_REQUEST_TIMEOUT_MS);
	expect_request(1);

	send_goaway(0, HTTP2_ERROR_NO_ERROR, 0);

	ret = join_request_thread();
	zassert_equal(ret, -ESHUTDOWN, "unexpected result %d", ret);
	assert_aborted(&rsp, -ESHUTDOWN);
	zassert_equal(rsp.abort_count, 1, "abort reported %d times", rsp.abort_count);

	assert_session_closed();
}

ZTEST(http2_client_peer, test_peer_goaway_debug_data)
{
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	/* Debug data larger than the receive buffer is skipped. */
	send_goaway(1, HTTP2_ERROR_ENHANCE_YOUR_CALM, 400);
	send_ok(1, HTTP2_FLAG_END_STREAM);

	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
	zassert_equal(rsp.final_count, 1, "final reported %d times", rsp.final_count);

	assert_session_closed();
}

ZTEST(http2_client_peer, test_peer_max_concurrent_streams)
{
	static struct http_client_request req2;
	static struct test_response rsp2;
	static uint8_t buf2[64];
	int ret;

	peer_connect(true);

	send_setting(HTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 1);
	expect_settings_acks(2);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);

	/* The client has room for more streams, the peer does not. */
	memset(&req2, 0, sizeof(req2));
	test_response_init(&rsp2, buf2, sizeof(buf2));
	ret = start_request(&req2, HTTP_GET, "/", &rsp2, TEST_REQUEST_TIMEOUT_MS);
	zassert_equal(ret, -EBUSY, "request beyond the peer's limit returned %d", ret);
	assert_aborted(&rsp2, -EBUSY);

	expect_request(1);
	send_ok(1, HTTP2_FLAG_END_STREAM);
	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);

	/* With the first stream gone there is room again. */
	test_response_init(&rsp, body_buf, sizeof(body_buf));
	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET after the limit cleared failed (%d)", ret);
	expect_request(3);
	send_ok(3, HTTP2_FLAG_END_STREAM);
	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
}

ZTEST(http2_client_peer, test_peer_request_headers)
{
	static const struct http_client_header fields[] = {
		{ "test-header", "abc" },
	};
	const char *value;
	int ret, count;

	peer_connect(true);

	memset(&req, 0, sizeof(req));
	req.header_fields = fields;
	req.headers_count = ARRAY_SIZE(fields);
	ret = start_request(&req, HTTP_GET, "/index.html", &rsp, TEST_REQUEST_TIMEOUT_MS);
	zassert_ok(ret, "GET failed (%d)", ret);

	count = expect_request(1);
	zassert_true(frame.flags & HTTP2_FLAG_END_STREAM, "END_STREAM missing on GET");

	value = peer_header_value(hdrs, count, ":method");
	zassert_not_null(value, ":method missing");
	zassert_str_equal(value, "GET", ":method %s", value);

	value = peer_header_value(hdrs, count, ":scheme");
	zassert_not_null(value, ":scheme missing");
	zassert_str_equal(value, "http", ":scheme %s", value);

	value = peer_header_value(hdrs, count, ":path");
	zassert_not_null(value, ":path missing");
	zassert_str_equal(value, "/index.html", ":path %s", value);

	value = peer_header_value(hdrs, count, ":authority");
	zassert_not_null(value, ":authority missing");
	zassert_str_equal(value, PEER_HOST ":8081", ":authority %s", value);

	value = peer_header_value(hdrs, count, "test-header");
	zassert_not_null(value, "test-header missing");
	zassert_str_equal(value, "abc", "test-header %s", value);

	send_ok(1, HTTP2_FLAG_END_STREAM);
	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
	zassert_equal(rsp.final_count, 1, "final reported %d times", rsp.final_count);
	zassert_equal(rsp.header_blocks, 1, "headers reported %d times", rsp.header_blocks);
}

ZTEST(http2_client_peer, test_peer_request_headers_continuation)
{
	static const char *const names[] = { "x-a", "x-b", "x-c", "x-d" };
	static char values[ARRAY_SIZE(names)][101];
	struct http_client_header fields[ARRAY_SIZE(names)];
	int ret, count, frames;

	for (int i = 0; i < ARRAY_SIZE(names); i++) {
		memset(values[i], 'a' + i, sizeof(values[i]) - 1);
		fields[i].name = names[i];
		fields[i].value = values[i];
	}

	peer_connect(true);

	/* Four 100-byte values do not fit the TX buffer in one frame. */
	memset(&req, 0, sizeof(req));
	req.header_fields = fields;
	req.headers_count = ARRAY_SIZE(fields);
	ret = start_request(&req, HTTP_GET, "/", &rsp, TEST_REQUEST_TIMEOUT_MS);
	zassert_ok(ret, "GET failed (%d)", ret);

	count = expect_request_block(1, &frames);
	zassert_true(frames > 1, "header block sent in %d frame", frames);

	for (int i = 0; i < ARRAY_SIZE(names); i++) {
		const char *value = peer_header_value(hdrs, count, fields[i].name);

		zassert_not_null(value, "%s missing", fields[i].name);
		zassert_str_equal(value, values[i], "%s value mismatch", fields[i].name);
	}

	/* A block that spilled over is ended by an empty DATA frame. */
	zassert_false(peer_stats.end_stream_seen, "END_STREAM within the header block");
	ret = peer_recv_frame_type(conn, HTTP2_DATA_FRAME, &frame);
	zassert_ok(ret, "no DATA from client (%d)", ret);
	zassert_equal(frame.len, 0, "DATA frame of %u bytes", frame.len);
	zassert_true(frame.flags & HTTP2_FLAG_END_STREAM, "END_STREAM missing");

	send_ok(1, HTTP2_FLAG_END_STREAM);
	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
}

ZTEST(http2_client_peer, test_peer_response_headers_overflow)
{
	static char names[12][8];
	static char values[12][41];
	struct http_header headers[13];
	int ret;

	headers[0].name = ":status";
	headers[0].value = "200";

	/* Twelve headers of 40 bytes exceed the 512-byte headers buffer. */
	for (int i = 0; i < 12; i++) {
		snprintk(names[i], sizeof(names[i]), "x-h%02d", i);
		memset(values[i], 'a' + i, sizeof(values[i]) - 1);
		headers[i + 1].name = names[i];
		headers[i + 1].value = values[i];
	}

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	ret = peer_send_headers(conn, 1, HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM,
				headers, ARRAY_SIZE(headers));
	zassert_ok(ret, "send HEADERS failed (%d)", ret);

	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
	zassert_equal(rsp.final_count, 1, "final reported %d times", rsp.final_count);
	zassert_true(rsp.header_blocks > 1, "headers reported in %d callback",
		     rsp.header_blocks);
	zassert_equal(rsp.header_count, 12, "%zu headers captured", rsp.header_count);

	for (int i = 0; i < 12; i++) {
		const char *value = test_response_header(&rsp, names[i]);

		zassert_not_null(value, "%s missing", names[i]);
		zassert_str_equal(value, values[i], "%s value mismatch", names[i]);
	}
}

ZTEST(http2_client_peer, test_peer_ping)
{
	static const uint8_t opaque[8] = "12345678";
	int ret;

	peer_connect(true);

	ret = peer_send_frame(conn, HTTP2_PING_FRAME, 0, 0, opaque, sizeof(opaque));
	zassert_ok(ret, "send PING failed (%d)", ret);

	ret = peer_recv_frame_type(conn, HTTP2_PING_FRAME, &frame);
	zassert_ok(ret, "no PING ACK from client (%d)", ret);
	zassert_true(frame.flags & FLAG_ACK, "PING without ACK flag");
	zassert_equal(frame.len, sizeof(opaque), "PING ACK length %u", frame.len);
	zassert_mem_equal(frame.payload, opaque, sizeof(opaque), "PING ACK data mismatch");
}

ZTEST(http2_client_peer, test_peer_window_update)
{
	int ret;

	peer_connect(true);

	ret = start_get("/big", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);
	send_ok(1, 0);

	for (size_t sent = 0; sent < WINDOW_TEST_BODY_SIZE; sent += sizeof(pattern)) {
		bool last = (sent + sizeof(pattern) >= WINDOW_TEST_BODY_SIZE);

		ret = peer_send_data(conn, 1, last ? HTTP2_FLAG_END_STREAM : 0, pattern,
				     sizeof(pattern));
		zassert_ok(ret, "send DATA failed (%d)", ret);
	}

	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
	zassert_equal(rsp.final_count, 1, "final reported %d times", rsp.final_count);
	zassert_equal(rsp.body_len, WINDOW_TEST_BODY_SIZE, "body length %zu", rsp.body_len);
	zassert_false(rsp.body_overflow, "body buffer too small");

	for (size_t off = 0; off < rsp.body_len; off += sizeof(pattern)) {
		zassert_mem_equal(rsp.body + off, pattern, sizeof(pattern),
				  "body mismatch at %zu", off);
	}

	/* Once the body exceeds half the window, both the connection and the
	 * stream window must have been replenished.
	 */
	(void)peer_drain(conn);
	if (WINDOW_TEST_BODY_SIZE > CONFIG_HTTP_CLIENT_HTTP2_WINDOW_SIZE / 2) {
		zassert_true(peer_stats.window_update_frames >= 2, "%d WINDOW_UPDATE frames",
			     peer_stats.window_update_frames);
		zassert_true(peer_stats.window_update_conn >= 32768, "connection increment %u",
			     peer_stats.window_update_conn);
		zassert_true(peer_stats.window_update_stream >= 32768, "stream increment %u",
			     peer_stats.window_update_stream);
	}
}

ZTEST(http2_client_peer, test_peer_rst_stream)
{
	uint8_t error[RST_STREAM_LEN];
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	sys_put_be32(HTTP2_ERROR_CANCEL, error);
	ret = peer_send_frame(conn, HTTP2_RST_STREAM_FRAME, 0, 1, error, sizeof(error));
	zassert_ok(ret, "send RST_STREAM failed (%d)", ret);

	wait_done(&rsp);
	assert_aborted(&rsp, -ECONNRESET);

	/* Only the stream is gone; the connection stays usable. */
	test_response_init(&rsp, body_buf, sizeof(body_buf));
	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET after reset failed (%d)", ret);
	expect_request(3);
	send_ok(3, HTTP2_FLAG_END_STREAM);
	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
}

/* A TCP transport of the test's own, whose next send can be made to fail
 * after writing only part of the data.
 */
struct failing_transport {
	int sock;
	size_t fail_after;
	bool armed;
	/* The next send stalls this long before going through, as on a peer
	 * that stopped reading. stalled is given when it starts, with the time.
	 */
	uint32_t block_ms;
	struct k_sem stalled;
	int64_t stall_start;
	/* The next close waits for a notification of the client, as the TCP
	 * transport does for one in flight. close_blocked tells if it hung.
	 */
	bool notify_on_close;
	bool close_blocked;
	/* waiting is set while a thread is inside the wait, which returns
	 * wait_delay_ms late, as a transport slow to come back.
	 */
	bool waiting;
	uint32_t wait_delay_ms;
};

static K_THREAD_STACK_DEFINE(notify_stack, 2048 + CONFIG_TEST_EXTRA_STACK_SIZE);
static struct k_thread notify_thread;

static void ft_notify_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	http_client_received_data(p1, 0);
}

static int ft_connect(struct http_client *c)
{
	struct failing_transport *ft = c->transport_ctx;
	int ret;

	ft->sock = zsock_socket(NET_AF_INET6, NET_SOCK_STREAM, NET_IPPROTO_TCP);
	if (ft->sock < 0) {
		return -errno;
	}

	ret = zsock_connect(ft->sock, (struct net_sockaddr *)&c->session.server_addr,
			    c->session.addrlen);

	return (ret < 0) ? -errno : 0;
}

static int ft_send(struct http_client *c, void *stream, const uint8_t *data, size_t len,
		   bool end_stream)
{
	struct failing_transport *ft = c->transport_ctx;
	size_t sent = 0;

	ARG_UNUSED(stream);
	ARG_UNUSED(end_stream);

	if (ft->block_ms > 0) {
		uint32_t block_ms = ft->block_ms;

		ft->block_ms = 0;
		ft->stall_start = k_uptime_get();
		k_sem_give(&ft->stalled);
		k_sleep(K_MSEC(block_ms));
	}

	if (ft->armed) {
		len = MIN(len, ft->fail_after);
	}

	while (sent < len) {
		int ret = zsock_send(ft->sock, data + sent, len - sent, 0);

		if (ret < 0) {
			return -errno;
		}

		sent += ret;
	}

	if (ft->armed) {
		ft->armed = false;
		return -EIO;
	}

	return 0;
}

static int ft_recv(struct http_client *c, void **stream, uint8_t *buf, size_t bufsize,
		   bool *end_stream)
{
	struct failing_transport *ft = c->transport_ctx;
	int ret;

	*stream = NULL;
	*end_stream = false;

	ret = zsock_recv(ft->sock, buf, bufsize, ZSOCK_MSG_DONTWAIT);

	return (ret < 0) ? -errno : ret;
}

static int ft_wait(struct http_client *c, k_timeout_t timeout)
{
	struct failing_transport *ft = c->transport_ctx;
	struct zsock_pollfd pfd = { .fd = ft->sock, .events = ZSOCK_POLLIN };
	int ret;

	ft->waiting = true;
	ret = zsock_poll(&pfd, 1, (int)k_ticks_to_ms_ceil32(timeout.ticks));
	if (ret < 0) {
		ret = -errno;
	} else if (ret == 0) {
		ret = -EAGAIN;
	}

	if (ft->wait_delay_ms > 0) {
		k_sleep(K_MSEC(ft->wait_delay_ms));
	}

	ft->waiting = false;

	return ret;
}

static int ft_close(struct http_client *c)
{
	struct failing_transport *ft = c->transport_ctx;

	if (ft->notify_on_close) {
		ft->notify_on_close = false;
		k_thread_create(&notify_thread, notify_stack, K_THREAD_STACK_SIZEOF(notify_stack),
				ft_notify_fn, c, NULL, NULL, K_PRIO_PREEMPT(8), 0, K_NO_WAIT);
		if (k_thread_join(&notify_thread, K_SECONDS(1)) != 0) {
			ft->close_blocked = true;
		}
	}

	(void)zsock_close(ft->sock);
	ft->sock = -1;

	return 0;
}

static const struct http_client_transport_ops failing_transport_ops = {
	.connect = ft_connect,
	.send = ft_send,
	.recv = ft_recv,
	.wait = ft_wait,
	.close = ft_close,
};

static struct failing_transport ft;

/* A client over the failing transport, with a synchronous session prepared
 * in cfg but not opened.
 */
static void failing_client_init(void)
{
	struct net_sockaddr_in6 *sa = (struct net_sockaddr_in6 *)&cfg.server_addr;
	int ret;

	memset(&ft, 0, sizeof(ft));
	k_sem_init(&ft.stalled, 0, 1);
	ret = http_client_init(&client, HTTP_CLIENT_VERSION_2, &failing_transport_ops, &ft);
	zassert_ok(ret, "client init failed (%d)", ret);

	memset(&cfg, 0, sizeof(cfg));
	sa->sin6_family = NET_AF_INET6;
	sa->sin6_port = net_htons(PEER_PORT);
	sa->sin6_addr = net_in6addr_loopback;
	cfg.addrlen = sizeof(*sa);
	cfg.host = PEER_HOST;
}

/* A synchronous session over the failing transport. */
static void peer_connect_failing(void)
{
	int ret;

	failing_client_init();

	ret = http_client_connect(&client, &cfg);
	zassert_ok(ret, "connect failed (%d)", ret);
	conn = peer_accept(listen_fd);
	zassert_true(conn >= 0, "accept failed (%d)", conn);
	ret = peer_handshake(conn);
	zassert_ok(ret, "handshake failed (%d)", ret);
}

/* After a send cut short at five bytes: the request failed with the send
 * error, the session is closed, and the peer saw those five bytes and then
 * the end of the connection.
 */
static void assert_send_failed(void)
{
	uint8_t byte;
	int ret;

	ret = join_request_thread();
	zassert_equal(ret, -EIO, "request returned %d", ret);
	assert_aborted(&rsp, -EIO);
	zassert_equal(client.session.state, HTTP_CLIENT_SESSION_CLOSED,
		      "session left open after a failed send");

	for (int i = 0; i < 5; i++) {
		ret = zsock_recv(conn, &byte, 1, 0);
		zassert_equal(ret, 1, "byte %d of the cut frame missing (%d)", i, ret);
	}

	ret = zsock_recv(conn, &byte, 1, 0);
	zassert_equal(ret, 0, "more than the cut frame on the wire (%d)", ret);

	ret = start_get("/", &rsp);
	zassert_equal(ret, -ENOTCONN, "session still open (%d)", ret);
}

ZTEST(http2_client_peer, test_peer_send_failure)
{
	peer_connect_failing();

	/* The HEADERS frame of the request gets five bytes onto the wire and
	 * then fails. Nothing else may follow on the connection.
	 */
	ft.fail_after = 5;
	ft.armed = true;

	memset(&req, 0, sizeof(req));
	req.method = HTTP_GET;
	req.url = "/";
	start_request_thread(TEST_REQUEST_TIMEOUT_MS);

	assert_send_failed();
}

ZTEST(http2_client_peer, test_peer_disconnect_send_failure)
{
	uint8_t byte;
	int ret;

	peer_connect_failing();

	memset(&req, 0, sizeof(req));
	req.method = HTTP_GET;
	req.url = "/";
	start_request_thread(TEST_REQUEST_TIMEOUT_MS);
	expect_request(1);
	expect_settings_acks(1);

	/* The RST_STREAM for the request in flight fails after five bytes:
	 * the GOAWAY is not even attempted, and the disconnect still succeeds,
	 * the notifications being a courtesy only.
	 */
	ft.fail_after = 5;
	ft.armed = true;
	ret = http_client_disconnect(&client);
	zassert_ok(ret, "disconnect failed (%d)", ret);
	zassert_equal(client.session.state, HTTP_CLIENT_SESSION_CLOSED, "session not closed");

	ret = join_request_thread();
	zassert_equal(ret, -ECONNABORTED, "request returned %d", ret);
	assert_aborted(&rsp, -ECONNABORTED);

	for (int i = 0; i < 5; i++) {
		ret = zsock_recv(conn, &byte, 1, 0);
		zassert_equal(ret, 1, "byte %d of the cut frame missing (%d)", i, ret);
	}

	ret = zsock_recv(conn, &byte, 1, 0);
	zassert_equal(ret, 0, "more than the cut frame on the wire (%d)", ret);
}

ZTEST(http2_client_peer, test_peer_rst_stream_send_failure)
{
	static const struct http_header bad[] = {
		{ ":status", "abc" },
	};
	int ret;

	peer_connect_failing();

	memset(&req, 0, sizeof(req));
	req.method = HTTP_GET;
	req.url = "/";
	start_request_thread(TEST_REQUEST_TIMEOUT_MS);
	expect_request(1);
	/* The handshake SETTINGS are only acknowledged once the request reads. */
	expect_settings_acks(1);

	/* The malformed response calls for a RST_STREAM, which fails after
	 * five bytes: the whole session ends rather than just the stream.
	 */
	ft.fail_after = 5;
	ft.armed = true;
	ret = peer_send_headers(conn, 1, HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM, bad,
				ARRAY_SIZE(bad));
	zassert_ok(ret, "send HEADERS failed (%d)", ret);

	assert_send_failed();
}

ZTEST(http2_client_peer, test_peer_settings_ack_send_failure)
{
	int ret;

	peer_connect_failing();

	memset(&req, 0, sizeof(req));
	req.method = HTTP_GET;
	req.url = "/";
	start_request_thread(TEST_REQUEST_TIMEOUT_MS);
	expect_request(1);
	expect_settings_acks(1);

	/* The acknowledgment of another SETTINGS frame, sent from the receive
	 * path, fails after five bytes: the session ends without a GOAWAY or
	 * any RST_STREAM on top of the cut frame.
	 */
	ft.fail_after = 5;
	ft.armed = true;
	ret = peer_send_frame(conn, HTTP2_SETTINGS_FRAME, 0, 0, NULL, 0);
	zassert_ok(ret, "send SETTINGS failed (%d)", ret);

	assert_send_failed();
}

ZTEST(http2_client_peer, test_peer_close)
{
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	zsock_close(conn);
	conn = -1;

	wait_done(&rsp);
	assert_aborted(&rsp, -ENOTCONN);

	ret = start_get("/", &rsp);
	zassert_equal(ret, -ENOTCONN, "session still open (%d)", ret);
}

ZTEST(http2_client_peer, test_peer_timeout_async)
{
	int ret;

	peer_connect(true);

	memset(&req, 0, sizeof(req));
	ret = start_request(&req, HTTP_GET, "/", &rsp, SHORT_TIMEOUT_MS);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	wait_done(&rsp);
	assert_aborted(&rsp, -ETIMEDOUT);

	ret = peer_recv_frame_type(conn, HTTP2_RST_STREAM_FRAME, &frame);
	zassert_ok(ret, "no RST_STREAM after timeout (%d)", ret);
	zassert_equal(peer_stats.rst_stream_id, 1, "RST_STREAM on stream %u",
		      peer_stats.rst_stream_id);
	zassert_equal(peer_stats.rst_error, HTTP2_ERROR_CANCEL, "RST_STREAM error %u",
		      peer_stats.rst_error);
}

ZTEST(http2_client_peer, test_peer_timeout_sync)
{
	int ret;

	peer_connect(false);

	memset(&req, 0, sizeof(req));
	ret = start_request(&req, HTTP_GET, "/", &rsp, SHORT_TIMEOUT_MS);
	zassert_equal(ret, -ETIMEDOUT, "unexpected result %d", ret);
	assert_aborted(&rsp, -ETIMEDOUT);

	expect_request(1);
	ret = peer_recv_frame_type(conn, HTTP2_RST_STREAM_FRAME, &frame);
	zassert_ok(ret, "no RST_STREAM after timeout (%d)", ret);
	zassert_equal(peer_stats.rst_stream_id, 1, "RST_STREAM on stream %u",
		      peer_stats.rst_stream_id);
	zassert_equal(peer_stats.rst_error, HTTP2_ERROR_CANCEL, "RST_STREAM error %u",
		      peer_stats.rst_error);
}

ZTEST(http2_client_peer, test_peer_disconnect_pending)
{
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	ret = http_client_disconnect(&client);
	zassert_ok(ret, "disconnect failed (%d)", ret);

	wait_done(&rsp);
	assert_aborted(&rsp, -ECONNABORTED);

	(void)peer_drain(conn);
	zassert_true(peer_stats.rst_stream_seen, "no RST_STREAM for the pending stream");
	zassert_true(peer_stats.goaway_seen, "no GOAWAY on disconnect");
	zassert_equal(peer_stats.goaway_error, HTTP2_ERROR_NO_ERROR, "GOAWAY error %u",
		      peer_stats.goaway_error);
	/* The client accepts no server-initiated streams. */
	zassert_equal(peer_stats.goaway_last_stream, 0, "GOAWAY last stream %u",
		      peer_stats.goaway_last_stream);
}

ZTEST(http2_client_peer, test_peer_trailers)
{
	static const struct http_header trailers[] = {
		{ "x-trailer", "done" },
	};
	const char *value;
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	send_ok(1, 0);
	ret = peer_send_data(conn, 1, 0, (const uint8_t *)"hello", 5);
	zassert_ok(ret, "send DATA failed (%d)", ret);
	ret = peer_send_headers(conn, 1, HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM,
				trailers, ARRAY_SIZE(trailers));
	zassert_ok(ret, "send trailers failed (%d)", ret);

	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
	zassert_equal(rsp.final_count, 1, "final reported %d times", rsp.final_count);
	zassert_equal(rsp.header_blocks, 2, "header blocks %d", rsp.header_blocks);
	zassert_equal(rsp.body_len, 5, "body length %zu", rsp.body_len);
	zassert_mem_equal(rsp.body, "hello", 5, "body mismatch");

	value = test_response_header(&rsp, "x-trailer");
	zassert_not_null(value, "trailer missing");
	zassert_str_equal(value, "done", "trailer value %s", value);
}

ZTEST(http2_client_peer, test_peer_trailers_pseudo_header)
{
	static const struct http_header trailers[] = {
		{ ":status", "200" },
	};
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	send_ok(1, 0);
	ret = peer_send_data(conn, 1, 0, (const uint8_t *)"hello", 5);
	zassert_ok(ret, "send DATA failed (%d)", ret);

	/* Trailers must not carry pseudo-headers. */
	ret = peer_send_headers(conn, 1, HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM,
				trailers, ARRAY_SIZE(trailers));
	zassert_ok(ret, "send trailers failed (%d)", ret);

	assert_stream_error(1, HTTP2_ERROR_PROTOCOL_ERROR, -EPROTO);
	assert_session_usable(3);
}

ZTEST(http2_client_peer, test_peer_client_settings)
{
	bool push_disabled = false;
	bool table_disabled = false;
	bool streams_limited = false;
	uint32_t window = 0;
	const uint8_t *settings;
	size_t len;
	int ret;

	peer_connect(true);

	settings = peer_client_settings(&len);
	zassert_true(len > 0 && len % 6 == 0, "SETTINGS payload of %zu bytes", len);

	for (size_t off = 0; off < len; off += 6) {
		uint16_t id = sys_get_be16(settings + off);
		uint32_t value = sys_get_be32(settings + off + sizeof(uint16_t));

		switch (id) {
		case HTTP2_SETTINGS_ENABLE_PUSH:
			push_disabled = (value == 0);
			break;
		case HTTP2_SETTINGS_HEADER_TABLE_SIZE:
			table_disabled = (value == 0);
			break;
		case HTTP2_SETTINGS_MAX_CONCURRENT_STREAMS:
			streams_limited = (value == MAX_STREAMS);
			break;
		case HTTP2_SETTINGS_INITIAL_WINDOW_SIZE:
			window = value;
			break;
		default:
			break;
		}
	}

	zassert_equal(window, CONFIG_HTTP_CLIENT_HTTP2_WINDOW_SIZE, "stream window %u", window);

	/* A window beyond the protocol's initial size is granted on the
	 * connection with a WINDOW_UPDATE right after SETTINGS.
	 */
	if (CONFIG_HTTP_CLIENT_HTTP2_WINDOW_SIZE > 65535) {
		ret = peer_recv_frame_type(conn, HTTP2_WINDOW_UPDATE_FRAME, &frame);
		zassert_ok(ret, "no WINDOW_UPDATE from client (%d)", ret);
		zassert_equal(frame.stream_id, 0, "WINDOW_UPDATE on stream %u", frame.stream_id);
		zassert_equal(peer_stats.window_update_conn,
			      CONFIG_HTTP_CLIENT_HTTP2_WINDOW_SIZE - 65535,
			      "connection increment %u", peer_stats.window_update_conn);
	}

	/* The client implements neither server push nor a dynamic table. */
	zassert_true(push_disabled, "server push not disabled");
	zassert_true(table_disabled, "dynamic header table not disabled");
	zassert_true(streams_limited, "stream limit not announced");
}

ZTEST(http2_client_peer, test_peer_interim_response)
{
	static const struct http_header early_hints[] = {
		{ ":status", "103" },
		{ "link", "</style.css>; rel=preload" },
	};
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	ret = peer_send_headers(conn, 1, HTTP2_FLAG_END_HEADERS, early_hints,
				ARRAY_SIZE(early_hints));
	zassert_ok(ret, "send 103 failed (%d)", ret);
	send_ok(1, HTTP2_FLAG_END_STREAM);

	wait_done(&rsp);
	zassert_equal(rsp.final_count, 1, "final reported %d times", rsp.final_count);
	zassert_equal(rsp.header_blocks, 2, "header blocks %d", rsp.header_blocks);
	zassert_equal(rsp.block_status[0], 103, "first block status %u", rsp.block_status[0]);
	zassert_equal(rsp.block_status[1], 200, "second block status %u",
		      rsp.block_status[1]);
	zassert_equal(rsp.status, 200, "final status %u", rsp.status);
	zassert_not_null(test_response_header(&rsp, "link"), "link header missing");
}

ZTEST(http2_client_peer, test_peer_bad_status)
{
	static const struct http_header bad[] = {
		{ ":status", "abc" },
	};
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	ret = peer_send_headers(conn, 1, HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM, bad,
				ARRAY_SIZE(bad));
	zassert_ok(ret, "send HEADERS failed (%d)", ret);

	/* A malformed response fails its stream only. */
	assert_stream_error(1, HTTP2_ERROR_PROTOCOL_ERROR, -EPROTO);
	assert_session_usable(3);
}

ZTEST(http2_client_peer, test_peer_data_on_stream_zero)
{
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	ret = peer_send_data(conn, 0, 0, (const uint8_t *)"oops", 4);
	zassert_ok(ret, "send DATA failed (%d)", ret);

	assert_connection_error(-EBADMSG);
}

ZTEST(http2_client_peer, test_peer_settings_bad_length)
{
	uint8_t settings[5] = { 0 };
	int ret;

	peer_connect(true);

	ret = peer_send_frame(conn, HTTP2_SETTINGS_FRAME, 0, 0, settings, sizeof(settings));
	zassert_ok(ret, "send SETTINGS failed (%d)", ret);

	(void)peer_drain(conn);
	zassert_true(peer_stats.goaway_seen, "no GOAWAY from client");

	ret = start_get("/", &rsp);
	zassert_equal(ret, -ENOTCONN, "session still open (%d)", ret);
}

ZTEST(http2_client_peer, test_peer_oversized_header_field)
{
	static char long_value[301];
	const struct http_header headers[] = {
		{ ":status", "200" },
		{ "x-long", long_value },
	};
	int ret;

	memset(long_value, 'a', sizeof(long_value) - 1);

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	ret = peer_send_headers(conn, 1, HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM,
				headers, ARRAY_SIZE(headers));
	zassert_ok(ret, "send HEADERS failed (%d)", ret);

	/* A field larger than the decode buffer costs that response only. */
	assert_stream_error(1, HTTP2_ERROR_INTERNAL_ERROR, -ENOBUFS);
	assert_session_usable(3);
}

ZTEST(http2_client_peer, test_peer_stream_exhaustion)
{
	static struct http_client_request reqs[MAX_STREAMS + 1];
	static struct test_response rsps[MAX_STREAMS + 1];
	static uint8_t bufs[MAX_STREAMS + 1][64];
	int ret;

	peer_connect(true);

	for (int i = 0; i <= MAX_STREAMS; i++) {
		memset(&reqs[i], 0, sizeof(reqs[i]));
		test_response_init(&rsps[i], bufs[i], sizeof(bufs[i]));
	}

	for (int i = 0; i < MAX_STREAMS; i++) {
		ret = start_request(&reqs[i], HTTP_GET, "/", &rsps[i], TEST_REQUEST_TIMEOUT_MS);
		zassert_ok(ret, "GET %d failed (%d)", i, ret);
	}

	ret = start_request(&reqs[MAX_STREAMS], HTTP_GET, "/", &rsps[MAX_STREAMS],
			    TEST_REQUEST_TIMEOUT_MS);
	zassert_equal(ret, -ENOMEM, "request beyond the stream limit returned %d", ret);
	assert_aborted(&rsps[MAX_STREAMS], -ENOMEM);

	for (int i = 0; i < MAX_STREAMS; i++) {
		uint32_t stream_id = 2 * i + 1;

		expect_request(stream_id);
		send_ok(stream_id, HTTP2_FLAG_END_STREAM);
	}

	for (int i = 0; i < MAX_STREAMS; i++) {
		wait_done(&rsps[i]);
		zassert_equal(rsps[i].status, 200, "stream %d status %u", i, rsps[i].status);
		zassert_equal(rsps[i].final_count, 1, "stream %d final count %d", i,
			      rsps[i].final_count);
	}
}

ZTEST(http2_client_peer, test_peer_sliced_response)
{
	static const struct http_header headers[] = {
		{ ":status", "200" },
		{ "content-type", "text/plain" },
	};
	static uint8_t wire[256];
	size_t off = 0;
	int ret, len;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	/* HEADERS and DATA frames back to back, then trickled a few bytes at
	 * a time so that frame headers and HPACK fields straddle receive calls.
	 */
	len = peer_encode_headers(wire + off + HTTP2_FRAME_HEADER_SIZE,
				  sizeof(wire) - off - HTTP2_FRAME_HEADER_SIZE, headers,
				  ARRAY_SIZE(headers));
	zassert_true(len > 0, "encode failed (%d)", len);
	peer_frame_header(wire + off, len, HTTP2_HEADERS_FRAME, HTTP2_FLAG_END_HEADERS, 1);
	off += HTTP2_FRAME_HEADER_SIZE + len;

	peer_frame_header(wire + off, 100, HTTP2_DATA_FRAME, HTTP2_FLAG_END_STREAM, 1);
	off += HTTP2_FRAME_HEADER_SIZE;
	memcpy(wire + off, pattern, 100);
	off += 100;

	ret = peer_send_sliced(conn, wire, off, 5, 2);
	zassert_ok(ret, "sliced send failed (%d)", ret);

	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
	zassert_equal(rsp.final_count, 1, "final reported %d times", rsp.final_count);
	zassert_equal(rsp.body_len, 100, "body length %zu", rsp.body_len);
	zassert_mem_equal(rsp.body, pattern, 100, "body mismatch");
	zassert_not_null(test_response_header(&rsp, "content-type"), "content-type missing");
}

ZTEST(http2_client_peer, test_peer_empty_data_end_stream)
{
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	send_ok(1, 0);
	ret = peer_send_data(conn, 1, HTTP2_FLAG_END_STREAM, NULL, 0);
	zassert_ok(ret, "send DATA failed (%d)", ret);

	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
	zassert_equal(rsp.final_count, 1, "final reported %d times", rsp.final_count);
	zassert_equal(rsp.header_blocks, 1, "header blocks %d", rsp.header_blocks);
	zassert_equal(rsp.body_len, 0, "body length %zu", rsp.body_len);
}

ZTEST(http2_client_peer, test_peer_continuation_aligned)
{
	static const struct http_header rest[] = {
		{ "x-a", "1" },
	};
	static uint8_t block[64];
	const char *value;
	int ret, len;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	/* The header block continues in a CONTINUATION frame, split on a field
	 * boundary. END_STREAM belongs to the HEADERS frame, END_HEADERS to the
	 * last CONTINUATION.
	 */
	ret = peer_send_headers(conn, 1, HTTP2_FLAG_END_STREAM, ok_headers,
				ARRAY_SIZE(ok_headers));
	zassert_ok(ret, "send HEADERS failed (%d)", ret);

	len = peer_encode_headers(block, sizeof(block), rest, ARRAY_SIZE(rest));
	zassert_true(len > 0, "encode failed (%d)", len);
	ret = peer_send_frame(conn, HTTP2_CONTINUATION_FRAME, HTTP2_FLAG_END_HEADERS, 1, block,
			      len);
	zassert_ok(ret, "send CONTINUATION failed (%d)", ret);

	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
	zassert_equal(rsp.final_count, 1, "final reported %d times", rsp.final_count);
	zassert_equal(rsp.header_blocks, 1, "header blocks %d", rsp.header_blocks);

	value = test_response_header(&rsp, "x-a");
	zassert_not_null(value, "x-a missing");
	zassert_str_equal(value, "1", "x-a value %s", value);
}

/* Encode a response header block whose second field is long enough to be cut
 * in several places. Returns the block length, with the offset of that field
 * in field_off.
 */
static int encode_split_block(uint8_t *block, size_t size, int *field_off)
{
	static char long_value[61];
	const struct http_header headers[] = {
		{ ":status", "200" },
		{ "x-long", long_value },
	};
	int len;

	memset(long_value, 'b', sizeof(long_value) - 1);

	*field_off = peer_encode_headers(block, size, headers, 1);
	zassert_true(*field_off > 0, "encode failed (%d)", *field_off);

	len = peer_encode_headers(block, size, headers, ARRAY_SIZE(headers));
	zassert_true(len > *field_off + 8, "encode failed (%d)", len);

	return len;
}

static void assert_split_block_received(void)
{
	const char *value;

	wait_done(&rsp);
	zassert_false(rsp.aborted, "transaction aborted, err %d", rsp.error);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
	zassert_equal(rsp.final_count, 1, "final reported %d times", rsp.final_count);
	zassert_equal(rsp.header_blocks, 1, "header blocks %d", rsp.header_blocks);

	value = test_response_header(&rsp, "x-long");
	zassert_not_null(value, "x-long missing");
	zassert_equal(strlen(value), 60, "x-long length %zu", strlen(value));
}

ZTEST(http2_client_peer, test_peer_continuation_split_field_three_frames)
{
	static uint8_t block[128];
	int ret, len, off, cut1, cut2;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	/* One field spread over a HEADERS and two CONTINUATION frames. */
	len = encode_split_block(block, sizeof(block), &off);
	cut1 = off + 2;
	cut2 = off + 6;

	ret = peer_send_frame(conn, HTTP2_HEADERS_FRAME, HTTP2_FLAG_END_STREAM, 1, block, cut1);
	zassert_ok(ret, "send HEADERS failed (%d)", ret);
	ret = peer_send_frame(conn, HTTP2_CONTINUATION_FRAME, 0, 1, block + cut1, cut2 - cut1);
	zassert_ok(ret, "send CONTINUATION failed (%d)", ret);
	ret = peer_send_frame(conn, HTTP2_CONTINUATION_FRAME, HTTP2_FLAG_END_HEADERS, 1,
			      block + cut2, len - cut2);
	zassert_ok(ret, "send CONTINUATION failed (%d)", ret);

	assert_split_block_received();
}

ZTEST(http2_client_peer, test_peer_continuation_split_field_padded)
{
	static uint8_t block[128];
	static uint8_t wire[256];
	const uint8_t pad_len = 5;
	size_t wire_len = 0;
	int ret, len, off, cut;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	len = encode_split_block(block, sizeof(block), &off);
	cut = off + 3;

	/* The padding of the HEADERS frame sits between the two parts of the
	 * field. Trickled in small pieces, so that the client has to wait for
	 * the CONTINUATION frame header before it can join them.
	 */
	peer_frame_header(wire, 1 + cut + pad_len, HTTP2_HEADERS_FRAME,
			  HTTP2_FLAG_END_STREAM | HTTP2_FLAG_PADDED, 1);
	wire_len += HTTP2_FRAME_HEADER_SIZE;
	wire[wire_len++] = pad_len;
	memcpy(wire + wire_len, block, cut);
	wire_len += cut;
	memset(wire + wire_len, 0, pad_len);
	wire_len += pad_len;

	peer_frame_header(wire + wire_len, len - cut, HTTP2_CONTINUATION_FRAME,
			  HTTP2_FLAG_END_HEADERS, 1);
	wire_len += HTTP2_FRAME_HEADER_SIZE;
	memcpy(wire + wire_len, block + cut, len - cut);
	wire_len += len - cut;

	ret = peer_send_sliced(conn, wire, wire_len, 4, 2);
	zassert_ok(ret, "sliced send failed (%d)", ret);

	assert_split_block_received();
}

ZTEST(http2_client_peer, test_peer_missing_continuation)
{
	static const uint8_t opaque[8] = "12345678";
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	/* The block is left open on a field boundary, then another frame is
	 * sent instead of its continuation.
	 */
	ret = peer_send_headers(conn, 1, HTTP2_FLAG_END_STREAM, ok_headers,
				ARRAY_SIZE(ok_headers));
	zassert_ok(ret, "send HEADERS failed (%d)", ret);
	ret = peer_send_frame(conn, HTTP2_PING_FRAME, 0, 0, opaque, sizeof(opaque));
	zassert_ok(ret, "send PING failed (%d)", ret);

	assert_connection_error(-EBADMSG);
}

ZTEST(http2_client_peer, test_peer_truncated_header_block)
{
	static uint8_t block[128];
	int ret, off;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	/* The block claims to be complete, but ends within a field. */
	(void)encode_split_block(block, sizeof(block), &off);

	ret = peer_send_frame(conn, HTTP2_HEADERS_FRAME,
			      HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM, 1, block, off + 3);
	zassert_ok(ret, "send HEADERS failed (%d)", ret);

	assert_connection_error_code(-EBADMSG, HTTP2_ERROR_COMPRESSION_ERROR);
}

ZTEST(http2_client_peer, test_peer_split_field_wrong_frame)
{
	static uint8_t block[128];
	int ret, len, off, cut;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	/* Only a CONTINUATION frame may follow an unfinished header block. */
	len = encode_split_block(block, sizeof(block), &off);
	cut = off + 3;

	ret = peer_send_frame(conn, HTTP2_HEADERS_FRAME, HTTP2_FLAG_END_STREAM, 1, block, cut);
	zassert_ok(ret, "send HEADERS failed (%d)", ret);
	ret = peer_send_data(conn, 1, 0, block + cut, len - cut);
	zassert_ok(ret, "send DATA failed (%d)", ret);

	assert_connection_error(-EBADMSG);
}

ZTEST(http2_client_peer, test_peer_padded_data)
{
	static const uint8_t padded[] = { 3, 'h', 'i', 0, 0, 0 };
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	send_ok(1, 0);
	ret = peer_send_data(conn, 1, HTTP2_FLAG_END_STREAM | HTTP2_FLAG_PADDED, padded,
			     sizeof(padded));
	zassert_ok(ret, "send DATA failed (%d)", ret);

	/* Neither the Pad Length field nor the padding reaches the application. */
	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
	zassert_equal(rsp.final_count, 1, "final reported %d times", rsp.final_count);
	zassert_equal(rsp.body_len, 2, "body length %zu", rsp.body_len);
	zassert_mem_equal(rsp.body, "hi", 2, "body mismatch");

	/* The connection is still in sync after the padding was skipped. */
	test_response_init(&rsp, body_buf, sizeof(body_buf));
	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET after padded frame failed (%d)", ret);
	expect_request(3);
	send_ok(3, HTTP2_FLAG_END_STREAM);
	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
}

ZTEST(http2_client_peer, test_peer_padded_headers)
{
	static uint8_t payload[64] = { 0 };
	const uint8_t pad_len = 4;
	int ret, len;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	/* Pad Length field, header block fragment, then the padding. */
	payload[0] = pad_len;
	len = peer_encode_headers(payload + 1, sizeof(payload) - 1 - pad_len, ok_headers,
				  ARRAY_SIZE(ok_headers));
	zassert_true(len > 0, "encode failed (%d)", len);
	ret = peer_send_frame(conn, HTTP2_HEADERS_FRAME,
			      HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM | HTTP2_FLAG_PADDED,
			      1, payload, 1 + len + pad_len);
	zassert_ok(ret, "send HEADERS failed (%d)", ret);

	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
	zassert_equal(rsp.final_count, 1, "final reported %d times", rsp.final_count);
	zassert_not_null(test_response_header(&rsp, "server"), "server header missing");

	test_response_init(&rsp, body_buf, sizeof(body_buf));
	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET after padded frame failed (%d)", ret);
	expect_request(3);
	send_ok(3, HTTP2_FLAG_END_STREAM);
	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
}

ZTEST(http2_client_peer, test_peer_empty_continuation)
{
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	/* The block is ended by a CONTINUATION frame without a fragment. The
	 * response must complete without any further data from the peer.
	 */
	ret = peer_send_headers(conn, 1, HTTP2_FLAG_END_STREAM, ok_headers,
				ARRAY_SIZE(ok_headers));
	zassert_ok(ret, "send HEADERS failed (%d)", ret);
	ret = peer_send_frame(conn, HTTP2_CONTINUATION_FRAME, HTTP2_FLAG_END_HEADERS, 1, NULL, 0);
	zassert_ok(ret, "send CONTINUATION failed (%d)", ret);

	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
	zassert_equal(rsp.final_count, 1, "final reported %d times", rsp.final_count);
	zassert_equal(rsp.header_blocks, 1, "header blocks %d", rsp.header_blocks);
}

ZTEST(http2_client_peer, test_peer_priority_headers)
{
	static uint8_t block[64] = { 0 };
	int ret, len;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	/* Five bytes of priority data precede the header block. */
	len = peer_encode_headers(block + HTTP2_HEADERS_FRAME_PRIORITY_LEN,
				  sizeof(block) - HTTP2_HEADERS_FRAME_PRIORITY_LEN, ok_headers,
				  ARRAY_SIZE(ok_headers));
	zassert_true(len > 0, "encode failed (%d)", len);
	ret = peer_send_frame(conn, HTTP2_HEADERS_FRAME,
			      HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM |
			      HTTP2_FLAG_PRIORITY,
			      1, block, HTTP2_HEADERS_FRAME_PRIORITY_LEN + len);
	zassert_ok(ret, "send HEADERS failed (%d)", ret);

	/* The priority field is skipped, its content ignored. */
	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
	zassert_equal(rsp.final_count, 1, "final reported %d times", rsp.final_count);
	zassert_not_null(test_response_header(&rsp, "server"), "server header missing");
}

static void recv_upload(size_t expected_len)
{
	size_t got = 0;
	int ret;

	while (!peer_stats.end_stream_seen) {
		ret = peer_recv_frame_type(conn, HTTP2_DATA_FRAME, &frame);
		zassert_ok(ret, "no DATA from client (%d)", ret);
		zassert_equal(frame.stream_id, 1, "DATA on stream %u", frame.stream_id);
		zassert_true(got + frame.len <= expected_len, "upload too long");
		zassert_mem_equal(frame.payload, upload + got, frame.len,
				  "upload mismatch at %zu", got);
		got += frame.len;
	}

	zassert_equal(got, expected_len, "upload length %zu", got);
}

ZTEST(http2_client_peer, test_peer_upload)
{
	const char *value;
	int ret, count;

	peer_connect(true);

	memset(&req, 0, sizeof(req));
	req.payload = upload;
	req.payload_len = sizeof(upload);
	req.content_type_value = "application/octet-stream";
	ret = start_request(&req, HTTP_PUT, "/upload", &rsp, TEST_REQUEST_TIMEOUT_MS);
	zassert_ok(ret, "PUT failed (%d)", ret);

	count = expect_request(1);
	zassert_false(frame.flags & HTTP2_FLAG_END_STREAM, "END_STREAM on HEADERS with body");

	value = peer_header_value(hdrs, count, ":method");
	zassert_not_null(value, ":method missing");
	zassert_str_equal(value, "PUT", ":method %s", value);

	value = peer_header_value(hdrs, count, "content-length");
	zassert_not_null(value, "content-length missing");
	zassert_str_equal(value, "1000", "content-length %s", value);

	value = peer_header_value(hdrs, count, "content-type");
	zassert_not_null(value, "content-type missing");
	zassert_str_equal(value, "application/octet-stream", "content-type %s", value);

	recv_upload(sizeof(upload));

	send_ok(1, HTTP2_FLAG_END_STREAM);
	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
}

ZTEST(http2_client_peer, test_peer_abort_with_buffered_data)
{
	static uint8_t wire[256];
	size_t off = 0;
	int ret, len;

	peer_connect(false);

	/* Abort from the first body callback. */
	rsp.body_cb_ret = -ECANCELED;
	memset(&req, 0, sizeof(req));
	req.method = HTTP_GET;
	req.url = "/";
	start_request_thread(TEST_REQUEST_TIMEOUT_MS);
	expect_request(1);

	/* Headers and three body chunks arrive in one go, so the chunks after
	 * the aborting one are already buffered when the abort happens.
	 */
	len = peer_encode_headers(wire + HTTP2_FRAME_HEADER_SIZE,
				  sizeof(wire) - HTTP2_FRAME_HEADER_SIZE, ok_headers,
				  ARRAY_SIZE(ok_headers));
	zassert_true(len > 0, "encode failed (%d)", len);
	peer_frame_header(wire, len, HTTP2_HEADERS_FRAME, HTTP2_FLAG_END_HEADERS, 1);
	off = HTTP2_FRAME_HEADER_SIZE + len;

	for (int i = 0; i < 3; i++) {
		peer_frame_header(wire + off, 4, HTTP2_DATA_FRAME,
				  (i == 2) ? HTTP2_FLAG_END_STREAM : 0, 1);
		off += HTTP2_FRAME_HEADER_SIZE;
		memset(wire + off, 'a' + i, 4);
		off += 4;
	}

	ret = peer_send_raw(conn, wire, off);
	zassert_ok(ret, "send failed (%d)", ret);

	ret = join_request_thread();
	zassert_equal(ret, -ECANCELED, "unexpected result %d", ret);
	zassert_equal(rsp.body_chunks, 1, "%d body chunks delivered", rsp.body_chunks);
	zassert_equal(rsp.abort_count, 1, "abort reported %d times", rsp.abort_count);
	zassert_equal(rsp.final_count, 0, "final reported after abort");

	ret = peer_recv_frame_type(conn, HTTP2_RST_STREAM_FRAME, &frame);
	zassert_ok(ret, "no RST_STREAM from client (%d)", ret);
}

ZTEST(http2_client_peer, test_peer_request_header_name_lowercase)
{
	static const struct http_client_header fields[] = {
		{ "X-Custom-Header", "v" },
	};
	bool found = false;
	int ret, count;

	peer_connect(true);

	memset(&req, 0, sizeof(req));
	req.header_fields = fields;
	req.headers_count = ARRAY_SIZE(fields);
	ret = start_request(&req, HTTP_GET, "/", &rsp, TEST_REQUEST_TIMEOUT_MS);
	zassert_ok(ret, "GET failed (%d)", ret);

	count = expect_request(1);

	/* Field names are sent in lowercase, whatever the application wrote. */
	for (int i = 0; i < count; i++) {
		if (strcasecmp(hdrs[i].name, "x-custom-header") == 0) {
			zassert_str_equal(hdrs[i].name, "x-custom-header", "sent as %s",
					  hdrs[i].name);
			found = true;
		}
	}

	zassert_true(found, "custom header missing");

	send_ok(1, HTTP2_FLAG_END_STREAM);
	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
}

ZTEST(http2_client_peer, test_peer_request_header_empty_value)
{
	static const struct http_client_header fields[] = {
		{ "x-empty", "" },
	};
	const char *value;
	int ret, count;

	peer_connect(true);

	memset(&req, 0, sizeof(req));
	req.header_fields = fields;
	req.headers_count = ARRAY_SIZE(fields);
	ret = start_request(&req, HTTP_GET, "/", &rsp, TEST_REQUEST_TIMEOUT_MS);
	zassert_ok(ret, "GET failed (%d)", ret);

	count = expect_request(1);
	value = peer_header_value(hdrs, count, "x-empty");
	zassert_not_null(value, "x-empty missing");
	zassert_equal(strlen(value), 0, "x-empty value %s", value);

	send_ok(1, HTTP2_FLAG_END_STREAM);
	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
}

ZTEST(http2_client_peer, test_peer_content_length_overflow)
{
	static const struct http_header headers[] = {
		{ ":status", "200" },
		{ "content-length", "99999999999999999999999999" },
	};
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	ret = peer_send_headers(conn, 1, HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM,
				headers, ARRAY_SIZE(headers));
	zassert_ok(ret, "send HEADERS failed (%d)", ret);

	assert_stream_error(1, HTTP2_ERROR_PROTOCOL_ERROR, -EPROTO);
	assert_session_usable(3);
}

ZTEST(http2_client_peer, test_peer_settings_large_sliced)
{
	static uint8_t wire[HTTP2_FRAME_HEADER_SIZE + 10 * 6];
	static struct http_client_request req2;
	static struct test_response rsp2;
	static uint8_t buf2[64];
	size_t off = HTTP2_FRAME_HEADER_SIZE;
	int ret;

	peer_connect(true);

	/* Ten parameters delivered a few bytes at a time, so that the frame
	 * is parsed across several reads. The last one must still take effect.
	 */
	peer_frame_header(wire, 10 * 6, HTTP2_SETTINGS_FRAME, 0, 0);
	for (int i = 0; i < 9; i++) {
		sys_put_be16(0x90 + i, wire + off);
		sys_put_be32(i, wire + off + 2);
		off += 6;
	}

	sys_put_be16(HTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, wire + off);
	sys_put_be32(1, wire + off + 2);

	ret = peer_send_sliced(conn, wire, sizeof(wire), 7, 2);
	zassert_ok(ret, "sliced send failed (%d)", ret);
	expect_settings_acks(2);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);

	memset(&req2, 0, sizeof(req2));
	test_response_init(&rsp2, buf2, sizeof(buf2));
	ret = start_request(&req2, HTTP_GET, "/", &rsp2, TEST_REQUEST_TIMEOUT_MS);
	zassert_equal(ret, -EBUSY, "request beyond the peer's limit returned %d", ret);

	expect_request(1);
	send_ok(1, HTTP2_FLAG_END_STREAM);
	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
}

/* Connection-level protocol checks. */

ZTEST(http2_client_peer, test_peer_continuation_other_stream)
{
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	/* The block of stream 1 is continued on stream 3. */
	ret = peer_send_headers(conn, 1, HTTP2_FLAG_END_STREAM, ok_headers,
				ARRAY_SIZE(ok_headers));
	zassert_ok(ret, "send HEADERS failed (%d)", ret);
	ret = peer_send_frame(conn, HTTP2_CONTINUATION_FRAME, HTTP2_FLAG_END_HEADERS, 3, NULL, 0);
	zassert_ok(ret, "send CONTINUATION failed (%d)", ret);

	assert_connection_error_code(-EBADMSG, HTTP2_ERROR_PROTOCOL_ERROR);
}

ZTEST(http2_client_peer, test_peer_goaway_on_stream)
{
	static const uint8_t payload[8];
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	ret = peer_send_frame(conn, HTTP2_GOAWAY_FRAME, 0, 1, payload, sizeof(payload));
	zassert_ok(ret, "send GOAWAY failed (%d)", ret);

	assert_connection_error_code(-EBADMSG, HTTP2_ERROR_PROTOCOL_ERROR);
}

ZTEST(http2_client_peer, test_peer_priority_on_stream_zero)
{
	static const uint8_t payload[5];
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	ret = peer_send_frame(conn, HTTP2_PRIORITY_FRAME, 0, 0, payload, sizeof(payload));
	zassert_ok(ret, "send PRIORITY failed (%d)", ret);

	assert_connection_error_code(-EBADMSG, HTTP2_ERROR_PROTOCOL_ERROR);
}

ZTEST(http2_client_peer, test_peer_priority_bad_length)
{
	static const uint8_t payload[6];
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	/* An error of that stream only. */
	ret = peer_send_frame(conn, HTTP2_PRIORITY_FRAME, 0, 1, payload, sizeof(payload));
	zassert_ok(ret, "send PRIORITY failed (%d)", ret);

	assert_stream_error(1, HTTP2_ERROR_FRAME_SIZE_ERROR, -EPROTO);
	assert_session_usable(3);
}

ZTEST(http2_client_peer, test_peer_headers_on_idle_stream)
{
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	/* Stream 3 was never opened. */
	send_ok(3, HTTP2_FLAG_END_STREAM);

	assert_connection_error_code(-EBADMSG, HTTP2_ERROR_PROTOCOL_ERROR);
}

ZTEST(http2_client_peer, test_peer_data_on_even_stream)
{
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	/* Only the server could open stream 2, and push is disabled. */
	ret = peer_send_data(conn, 2, HTTP2_FLAG_END_STREAM, (const uint8_t *)"hello", 5);
	zassert_ok(ret, "send DATA failed (%d)", ret);

	assert_connection_error_code(-EBADMSG, HTTP2_ERROR_PROTOCOL_ERROR);
}

ZTEST(http2_client_peer, test_peer_rst_stream_bad_length)
{
	static const uint8_t payload[1000];
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	/* Larger than the receive buffer, still a FRAME_SIZE_ERROR. */
	ret = peer_send_frame(conn, HTTP2_RST_STREAM_FRAME, 0, 1, payload, sizeof(payload));
	zassert_ok(ret, "send RST_STREAM failed (%d)", ret);

	assert_connection_error_code(-EBADMSG, HTTP2_ERROR_FRAME_SIZE_ERROR);
}

ZTEST(http2_client_peer, test_peer_frame_too_large)
{
	uint8_t hdr[HTTP2_FRAME_HEADER_SIZE];
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	/* One byte over the default SETTINGS_MAX_FRAME_SIZE; the header alone
	 * is enough to reject it.
	 */
	peer_frame_header(hdr, 16385, HTTP2_DATA_FRAME, 0, 1);
	ret = peer_send_raw(conn, hdr, sizeof(hdr));
	zassert_ok(ret, "send failed (%d)", ret);

	assert_connection_error_code(-EBADMSG, HTTP2_ERROR_FRAME_SIZE_ERROR);
}

ZTEST(http2_client_peer, test_peer_hpack_table_size_update)
{
	/* A size update to 0 may open a block. */
	static const uint8_t block[] = { 0x20, 0x88 };
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	ret = peer_send_frame(conn, HTTP2_HEADERS_FRAME,
			      HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM, 1, block,
			      sizeof(block));
	zassert_ok(ret, "send HEADERS failed (%d)", ret);

	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
}

ZTEST(http2_client_peer, test_peer_hpack_table_size_update_before_ack)
{
	/* A size update to 4096, the protocol default: valid as long as the
	 * server has not acknowledged the advertised 0.
	 */
	static const uint8_t block[] = { 0x3f, 0xe1, 0x1f, 0x88 };
	int ret;

	peer_connect_unacked(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	ret = peer_send_frame(conn, HTTP2_HEADERS_FRAME,
			      HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM, 1, block,
			      sizeof(block));
	zassert_ok(ret, "send HEADERS failed (%d)", ret);

	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
}

ZTEST(http2_client_peer, test_peer_hpack_table_size_update_too_large)
{
	/* A size update to 4097, above the protocol default. */
	static const uint8_t block[] = { 0x3f, 0xe2, 0x1f, 0x88 };
	int ret;

	peer_connect_unacked(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	ret = peer_send_frame(conn, HTTP2_HEADERS_FRAME,
			      HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM, 1, block,
			      sizeof(block));
	zassert_ok(ret, "send HEADERS failed (%d)", ret);

	assert_connection_error_code(-EBADMSG, HTTP2_ERROR_COMPRESSION_ERROR);
}

ZTEST(http2_client_peer, test_peer_hpack_table_size_update_nonzero)
{
	/* A size update to 100, above the advertised 0 once acknowledged. */
	static const uint8_t block[] = { 0x3f, 0x45, 0x88 };
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	ret = peer_send_frame(conn, HTTP2_HEADERS_FRAME,
			      HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM, 1, block,
			      sizeof(block));
	zassert_ok(ret, "send HEADERS failed (%d)", ret);

	assert_connection_error_code(-EBADMSG, HTTP2_ERROR_COMPRESSION_ERROR);
}

ZTEST(http2_client_peer, test_peer_hpack_table_size_update_late)
{
	/* A size update after a field. */
	static const uint8_t block[] = { 0x88, 0x20 };
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	ret = peer_send_frame(conn, HTTP2_HEADERS_FRAME,
			      HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM, 1, block,
			      sizeof(block));
	zassert_ok(ret, "send HEADERS failed (%d)", ret);

	assert_connection_error_code(-EBADMSG, HTTP2_ERROR_COMPRESSION_ERROR);
}

/* Malformed responses, which fail their stream only. */

ZTEST(http2_client_peer, test_peer_empty_header_name)
{
	/* :status 200, then a literal field with an empty name. */
	static const uint8_t block[] = { 0x88, 0x40, 0x00, 0x01, 'v' };
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	ret = peer_send_frame(conn, HTTP2_HEADERS_FRAME,
			      HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM, 1, block,
			      sizeof(block));
	zassert_ok(ret, "send HEADERS failed (%d)", ret);

	assert_stream_error(1, HTTP2_ERROR_PROTOCOL_ERROR, -EPROTO);
	assert_session_usable(3);
}

ZTEST(http2_client_peer, test_peer_status_after_regular_field)
{
	static const struct http_header headers[] = {
		{ "server", "peer" },
		{ ":status", "200" },
	};
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	ret = peer_send_headers(conn, 1, HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM,
				headers, ARRAY_SIZE(headers));
	zassert_ok(ret, "send HEADERS failed (%d)", ret);

	assert_stream_error(1, HTTP2_ERROR_PROTOCOL_ERROR, -EPROTO);
	assert_session_usable(3);
}

ZTEST(http2_client_peer, test_peer_status_twice)
{
	static const struct http_header headers[] = {
		{ ":status", "200" },
		{ ":status", "404" },
	};
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	ret = peer_send_headers(conn, 1, HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM,
				headers, ARRAY_SIZE(headers));
	zassert_ok(ret, "send HEADERS failed (%d)", ret);

	assert_stream_error(1, HTTP2_ERROR_PROTOCOL_ERROR, -EPROTO);
	assert_session_usable(3);
}

ZTEST(http2_client_peer, test_peer_head_padding_only_data)
{
	static const uint8_t padded[] = { 3, 0, 0, 0 };
	int ret;

	peer_connect(true);

	memset(&req, 0, sizeof(req));
	ret = start_request(&req, HTTP_HEAD, "/", &rsp, TEST_REQUEST_TIMEOUT_MS);
	zassert_ok(ret, "HEAD failed (%d)", ret);
	expect_request(1);

	/* A frame of padding alone carries no body, so it may end the stream. */
	send_ok(1, 0);
	ret = peer_send_data(conn, 1, HTTP2_FLAG_END_STREAM | HTTP2_FLAG_PADDED, padded,
			     sizeof(padded));
	zassert_ok(ret, "send DATA failed (%d)", ret);

	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
	zassert_equal(rsp.final_count, 1, "final reported %d times", rsp.final_count);
	zassert_equal(rsp.body_len, 0, "body length %zu", rsp.body_len);

	/* The padding was consumed in full: the session stays in sync. */
	test_response_init(&rsp, body_buf, sizeof(body_buf));
	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET after padded frame failed (%d)", ret);
	expect_request(3);
	send_ok(3, HTTP2_FLAG_END_STREAM);
	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
}

ZTEST(http2_client_peer, test_peer_response_uppercase_name)
{
	/* :status 200, then a literal field named X-Foo. */
	static const uint8_t block[] = { 0x88, 0x40, 0x05, 'X', '-', 'F', 'o', 'o', 0x01, 'v' };
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	ret = peer_send_frame(conn, HTTP2_HEADERS_FRAME,
			      HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM, 1, block,
			      sizeof(block));
	zassert_ok(ret, "send HEADERS failed (%d)", ret);

	assert_stream_error(1, HTTP2_ERROR_PROTOCOL_ERROR, -EPROTO);
	assert_session_usable(3);
}

ZTEST(http2_client_peer, test_peer_interim_fields_not_kept)
{
	static const struct http_header interim[] = {
		{ ":status", "103" },
		{ "content-type", "text/html" },
		{ "content-length", "100" },
	};
	static const struct http_header final_headers[] = {
		{ ":status", "200" },
	};
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	/* Nothing of the interim response applies to the final one, whose
	 * body is not measured against the interim content-length.
	 */
	ret = peer_send_headers(conn, 1, HTTP2_FLAG_END_HEADERS, interim, ARRAY_SIZE(interim));
	zassert_ok(ret, "send interim HEADERS failed (%d)", ret);
	ret = peer_send_headers(conn, 1, HTTP2_FLAG_END_HEADERS, final_headers,
				ARRAY_SIZE(final_headers));
	zassert_ok(ret, "send HEADERS failed (%d)", ret);
	ret = peer_send_data(conn, 1, HTTP2_FLAG_END_STREAM, (const uint8_t *)"hello", 5);
	zassert_ok(ret, "send DATA failed (%d)", ret);

	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
	zassert_false(rsp.content_len_present, "interim content-length kept");
	zassert_equal(rsp.body_len, 5, "body length %zu", rsp.body_len);
	zassert_equal(rsp.final_count, 1, "final reported %d times", rsp.final_count);
}

ZTEST(http2_client_peer, test_peer_response_te_field)
{
	static const struct http_header headers[] = {
		{ ":status", "200" },
		{ "te", "trailers" },
	};
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	ret = peer_send_headers(conn, 1, HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM,
				headers, ARRAY_SIZE(headers));
	zassert_ok(ret, "send HEADERS failed (%d)", ret);

	assert_stream_error(1, HTTP2_ERROR_PROTOCOL_ERROR, -EPROTO);
	assert_session_usable(3);
}

ZTEST(http2_client_peer, test_peer_response_connection_field)
{
	static const struct http_header headers[] = {
		{ ":status", "200" },
		{ "connection", "close" },
	};
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	ret = peer_send_headers(conn, 1, HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM,
				headers, ARRAY_SIZE(headers));
	zassert_ok(ret, "send HEADERS failed (%d)", ret);

	assert_stream_error(1, HTTP2_ERROR_PROTOCOL_ERROR, -EPROTO);
	assert_session_usable(3);
}

ZTEST(http2_client_peer, test_peer_content_length_conflict)
{
	static const struct http_header headers[] = {
		{ ":status", "200" },
		{ "content-length", "5" },
		{ "content-length", "6" },
	};
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	ret = peer_send_headers(conn, 1, HTTP2_FLAG_END_HEADERS, headers, ARRAY_SIZE(headers));
	zassert_ok(ret, "send HEADERS failed (%d)", ret);

	assert_stream_error(1, HTTP2_ERROR_PROTOCOL_ERROR, -EPROTO);
	assert_session_usable(3);
}

ZTEST(http2_client_peer, test_peer_content_length_repeated)
{
	static const struct http_header headers[] = {
		{ ":status", "200" },
		{ "content-length", "5" },
		{ "content-length", "5" },
	};
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	/* The same value twice is tolerated. */
	ret = peer_send_headers(conn, 1, HTTP2_FLAG_END_HEADERS, headers, ARRAY_SIZE(headers));
	zassert_ok(ret, "send HEADERS failed (%d)", ret);
	ret = peer_send_data(conn, 1, HTTP2_FLAG_END_STREAM, (const uint8_t *)"hello", 5);
	zassert_ok(ret, "send DATA failed (%d)", ret);

	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
	zassert_equal(rsp.body_len, 5, "body length %zu", rsp.body_len);
}

ZTEST(http2_client_peer, test_peer_interim_end_stream)
{
	static const struct http_header interim[] = {
		{ ":status", "100" },
	};
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	ret = peer_send_headers(conn, 1, HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM,
				interim, ARRAY_SIZE(interim));
	zassert_ok(ret, "send HEADERS failed (%d)", ret);

	assert_stream_error(1, HTTP2_ERROR_PROTOCOL_ERROR, -EPROTO);
	assert_session_usable(3);
}

ZTEST(http2_client_peer, test_peer_status_missing)
{
	static const struct http_header headers[] = {
		{ "server", "peer" },
	};
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	ret = peer_send_headers(conn, 1, HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM,
				headers, ARRAY_SIZE(headers));
	zassert_ok(ret, "send HEADERS failed (%d)", ret);

	assert_stream_error(1, HTTP2_ERROR_PROTOCOL_ERROR, -EPROTO);
	assert_session_usable(3);
}

ZTEST(http2_client_peer, test_peer_head_with_body)
{
	int ret;

	peer_connect(true);

	memset(&req, 0, sizeof(req));
	ret = start_request(&req, HTTP_HEAD, "/", &rsp, TEST_REQUEST_TIMEOUT_MS);
	zassert_ok(ret, "HEAD failed (%d)", ret);
	expect_request(1);

	send_ok(1, 0);
	ret = peer_send_data(conn, 1, HTTP2_FLAG_END_STREAM, (const uint8_t *)"hello", 5);
	zassert_ok(ret, "send DATA failed (%d)", ret);

	assert_stream_error(1, HTTP2_ERROR_PROTOCOL_ERROR, -EPROTO);
	zassert_equal(rsp.body_chunks, 0, "body delivered on HEAD");
	assert_session_usable(3);
}

ZTEST(http2_client_peer, test_peer_204_with_body)
{
	static const struct http_header no_content[] = {
		{ ":status", "204" },
	};
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	ret = peer_send_headers(conn, 1, HTTP2_FLAG_END_HEADERS, no_content,
				ARRAY_SIZE(no_content));
	zassert_ok(ret, "send HEADERS failed (%d)", ret);
	ret = peer_send_data(conn, 1, HTTP2_FLAG_END_STREAM, (const uint8_t *)"hello", 5);
	zassert_ok(ret, "send DATA failed (%d)", ret);

	assert_stream_error(1, HTTP2_ERROR_PROTOCOL_ERROR, -EPROTO);
	zassert_equal(rsp.body_chunks, 0, "body delivered on 204");
	assert_session_usable(3);
}

ZTEST(http2_client_peer, test_peer_data_before_headers)
{
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	ret = peer_send_data(conn, 1, 0, (const uint8_t *)"hello", 5);
	zassert_ok(ret, "send DATA failed (%d)", ret);

	assert_stream_error(1, HTTP2_ERROR_PROTOCOL_ERROR, -EPROTO);
	zassert_equal(rsp.body_chunks, 0, "body delivered without headers");
	assert_session_usable(3);
}

ZTEST(http2_client_peer, test_peer_trailers_without_end_stream)
{
	static const struct http_header trailers[] = {
		{ "x-trailer", "done" },
	};
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	send_ok(1, 0);
	ret = peer_send_data(conn, 1, 0, (const uint8_t *)"hello", 5);
	zassert_ok(ret, "send DATA failed (%d)", ret);
	ret = peer_send_headers(conn, 1, HTTP2_FLAG_END_HEADERS, trailers, ARRAY_SIZE(trailers));
	zassert_ok(ret, "send trailers failed (%d)", ret);

	assert_stream_error(1, HTTP2_ERROR_PROTOCOL_ERROR, -EPROTO);
	assert_session_usable(3);
}

ZTEST(http2_client_peer, test_peer_content_length_short)
{
	static const struct http_header headers[] = {
		{ ":status", "200" },
		{ "content-length", "100" },
	};
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	ret = peer_send_headers(conn, 1, HTTP2_FLAG_END_HEADERS, headers, ARRAY_SIZE(headers));
	zassert_ok(ret, "send HEADERS failed (%d)", ret);
	ret = peer_send_data(conn, 1, HTTP2_FLAG_END_STREAM, (const uint8_t *)"0123456789", 10);
	zassert_ok(ret, "send DATA failed (%d)", ret);

	/* A truncated body is not reported as a complete response. */
	assert_stream_error(1, HTTP2_ERROR_PROTOCOL_ERROR, -EPROTO);
	assert_session_usable(3);
}

ZTEST(http2_client_peer, test_peer_content_length_exceeded)
{
	static const struct http_header headers[] = {
		{ ":status", "200" },
		{ "content-length", "5" },
	};
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	ret = peer_send_headers(conn, 1, HTTP2_FLAG_END_HEADERS, headers, ARRAY_SIZE(headers));
	zassert_ok(ret, "send HEADERS failed (%d)", ret);
	ret = peer_send_data(conn, 1, 0, (const uint8_t *)"0123456789", 10);
	zassert_ok(ret, "send DATA failed (%d)", ret);

	assert_stream_error(1, HTTP2_ERROR_PROTOCOL_ERROR, -EPROTO);
	assert_session_usable(3);
}

ZTEST(http2_client_peer, test_peer_content_length_no_body)
{
	static const struct http_header headers[] = {
		{ ":status", "200" },
		{ "content-length", "5" },
	};
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	ret = peer_send_headers(conn, 1, HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM,
				headers, ARRAY_SIZE(headers));
	zassert_ok(ret, "send HEADERS failed (%d)", ret);

	assert_stream_error(1, HTTP2_ERROR_PROTOCOL_ERROR, -EPROTO);
	assert_session_usable(3);
}

ZTEST(http2_client_peer, test_peer_head_content_length)
{
	static const struct http_header headers[] = {
		{ ":status", "200" },
		{ "content-length", "100" },
	};
	int ret;

	peer_connect(true);

	memset(&req, 0, sizeof(req));
	ret = start_request(&req, HTTP_HEAD, "/", &rsp, TEST_REQUEST_TIMEOUT_MS);
	zassert_ok(ret, "HEAD failed (%d)", ret);
	expect_request(1);

	/* The response to HEAD describes the body it does not carry. */
	ret = peer_send_headers(conn, 1, HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM,
				headers, ARRAY_SIZE(headers));
	zassert_ok(ret, "send HEADERS failed (%d)", ret);

	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
	zassert_equal(rsp.final_count, 1, "final reported %d times", rsp.final_count);
	zassert_true(rsp.content_len_present, "content length missing");
	zassert_equal(rsp.content_len, 100, "content length %zu", rsp.content_len);
}

/* Request fields. */

static int spilling_headers_cb_round;

static int spilling_headers_cb(const struct http_client_header **headers, size_t *count,
			       bool *more, void *user_data)
{
	static const char *const names[] = { "x-a", "x-b", "x-c", "x-d" };
	static char values[ARRAY_SIZE(names)][101];
	static struct http_client_header fields[ARRAY_SIZE(names)];
	static const struct http_client_header bad = { "x-bad", "a\rb" };

	ARG_UNUSED(user_data);

	if (spilling_headers_cb_round == 0) {
		for (int i = 0; i < ARRAY_SIZE(names); i++) {
			memset(values[i], 'a' + i, sizeof(values[i]) - 1);
			fields[i].name = names[i];
			fields[i].value = values[i];
		}

		*headers = fields;
		*count = ARRAY_SIZE(fields);
		*more = true;
	} else {
		*headers = &bad;
		*count = 1;
		*more = false;
	}

	spilling_headers_cb_round++;

	return 0;
}

ZTEST(http2_client_peer, test_peer_request_headers_cb_invalid_after_spill)
{
	int ret, frames;

	peer_connect(true);

	/* The callback first fills more than one frame, then hands out an
	 * invalid field: the block on the wire cannot be completed and no
	 * other frame may follow it, so the connection is closed as it is.
	 */
	spilling_headers_cb_round = 0;
	memset(&req, 0, sizeof(req));
	req.headers_cb = spilling_headers_cb;
	ret = start_request(&req, HTTP_GET, "/", &rsp, TEST_REQUEST_TIMEOUT_MS);
	zassert_equal(ret, -EINVAL, "invalid field accepted (%d)", ret);
	zassert_equal(spilling_headers_cb_round, 2, "callback called %d times",
		      spilling_headers_cb_round);
	assert_aborted(&rsp, -EINVAL);

	ret = peer_recv_frame_type(conn, HTTP2_HEADERS_FRAME, &frame);
	zassert_ok(ret, "no HEADERS from client (%d)", ret);
	zassert_false(frame.flags & HTTP2_FLAG_END_HEADERS, "block ended");
	frames = peer_drain(conn);
	zassert_equal(frames, 0, "%d frames after the cut block", frames);
	zassert_false(peer_stats.goaway_seen, "GOAWAY within the header block");
	zassert_false(peer_stats.end_stream_seen, "END_STREAM on an incomplete request");

	ret = start_get("/again", &rsp);
	zassert_equal(ret, -ENOTCONN, "session still open (%d)", ret);
}

ZTEST(http2_client_peer, test_peer_request_header_too_large)
{
	static char long_value[2 * CONFIG_HTTP_CLIENT_TX_BUFFER_SIZE];
	static const struct http_client_header fields[] = {
		{ "x-small", "v" },
		{ "x-huge", long_value },
	};
	int ret, frames;

	memset(long_value, 'a', sizeof(long_value) - 1);

	peer_connect(true);

	/* The second field does not fit the TX buffer even on its own, found
	 * out after the first one went out.
	 */
	memset(&req, 0, sizeof(req));
	req.header_fields = fields;
	req.headers_count = ARRAY_SIZE(fields);
	ret = start_request(&req, HTTP_GET, "/", &rsp, TEST_REQUEST_TIMEOUT_MS);
	zassert_equal(ret, -ENOBUFS, "oversized field accepted (%d)", ret);
	assert_aborted(&rsp, -ENOBUFS);

	ret = peer_recv_frame_type(conn, HTTP2_HEADERS_FRAME, &frame);
	zassert_ok(ret, "no HEADERS from client (%d)", ret);
	zassert_false(frame.flags & HTTP2_FLAG_END_HEADERS, "block ended");
	frames = peer_drain(conn);
	zassert_equal(frames, 0, "%d frames after the cut block", frames);
	zassert_false(peer_stats.goaway_seen, "GOAWAY within the header block");
	zassert_false(peer_stats.end_stream_seen, "END_STREAM on an incomplete request");
}

ZTEST(http2_client_peer, test_peer_connect_invalid_host)
{
	static const char *const hosts[] = {
		"example.com/api", "a b", "host\r\n", "host:8080", "[::1]x]", "[::1", "a]b",
	};
	struct net_sockaddr_in6 *sa = (struct net_sockaddr_in6 *)&cfg.server_addr;
	int ret;

	memset(&tcp_ctx, 0, sizeof(tcp_ctx));
	ret = http_client_init_tcp(&client, HTTP_CLIENT_VERSION_2, &tcp_ctx);
	zassert_ok(ret, "client init failed (%d)", ret);

	memset(&cfg, 0, sizeof(cfg));
	sa->sin6_family = NET_AF_INET6;
	sa->sin6_port = net_htons(PEER_PORT);
	sa->sin6_addr = net_in6addr_loopback;
	cfg.addrlen = sizeof(*sa);

	for (size_t i = 0; i < ARRAY_SIZE(hosts); i++) {
		cfg.host = hosts[i];
		ret = http_client_connect(&client, &cfg);
		zassert_equal(ret, -EINVAL, "host %zu accepted (%d)", i, ret);
	}
}

ZTEST(http2_client_peer, test_peer_request_invalid_fields)
{
	static const struct http_client_header space_in_name[] = { { "X Foo", "v" } };
	static const struct http_client_header leading_space[] = { { "x-a", " v" } };
	static const struct http_client_header trailing_tab[] = { { "x-a", "v\t" } };
	static const struct http_client_header ctl_in_value[] = { { "x-a", "a\x01b" } };
	static const struct http_client_header crlf_host[] = { { "Host", "a\r\nx: y" } };
	static const struct http_client_header userinfo_host[] = { { "Host", "user@host" } };
	static const struct http_client_header path_host[] = { { "Host", "host/path" } };
	static const struct http_client_header space_host[] = { { "Host", "a b" } };
	static const struct http_client_header bracket_host[] = { { "Host", "a]b" } };
	static const struct http_client_header brackets_host[] = { { "Host", "ex[am]ple.com" } };
	static const struct http_client_header trailing_host[] = { { "Host", "[::1]x]" } };
	static const struct http_client_header unclosed_host[] = { { "Host", "[::1" } };
	static const struct http_client_header bad_port_host[] = { { "Host", "host:8o80" } };
	static const struct http_client_header empty_port_host[] = { { "Host", "host:" } };
	static const struct http_client_header big_port_host[] = { { "Host", "host:65536" } };
	static const struct http_client_header crlf_value[] = { { "x-a", "a\r\nx-injected: b" } };
	static const struct http_client_header pseudo_name[] = { { ":custom", "value" } };
	/* Longer than the name limit of the client. */
	static char long_name[257];
	static const struct http_client_header long_name_field[] = { { long_name, "v" } };
	const struct http_client_header *const invalid[] = {
		space_in_name, leading_space, trailing_tab,  ctl_in_value,  crlf_host,
		userinfo_host, path_host,     space_host,    bracket_host,  brackets_host,
		trailing_host, unclosed_host, bad_port_host, empty_port_host, big_port_host,
		crlf_value,    pseudo_name,   long_name_field,
	};
	int ret;

	memset(long_name, 'A', sizeof(long_name) - 1);

	peer_connect(true);

	for (size_t i = 0; i < ARRAY_SIZE(invalid); i++) {
		memset(&req, 0, sizeof(req));
		req.header_fields = invalid[i];
		req.headers_count = 1;
		ret = start_request(&req, HTTP_GET, "/", &rsp, TEST_REQUEST_TIMEOUT_MS);
		zassert_equal(ret, -EINVAL, "field %zu accepted (%d)", i, ret);
	}

	/* Nothing reached the peer and the session is still usable; a rejected
	 * request takes no stream identifier.
	 */
	assert_session_usable(1);
}

ZTEST(http2_client_peer, test_peer_request_tab_in_value)
{
	static const struct http_client_header fields[] = {
		{ "x-tab", "a\tb" },
		{ "Host", "[2001:db8::1]:65535" },
	};
	const char *value;
	int ret, count;

	peer_connect(true);

	memset(&req, 0, sizeof(req));
	req.header_fields = fields;
	req.headers_count = ARRAY_SIZE(fields);
	ret = start_request(&req, HTTP_GET, "/", &rsp, TEST_REQUEST_TIMEOUT_MS);
	zassert_ok(ret, "GET failed (%d)", ret);

	count = expect_request(1);
	value = peer_header_value(hdrs, count, "x-tab");
	zassert_not_null(value, "x-tab missing");
	zassert_str_equal(value, "a\tb", "x-tab value %s", value);
	value = peer_header_value(hdrs, count, ":authority");
	zassert_not_null(value, ":authority missing");
	zassert_str_equal(value, "[2001:db8::1]:65535", ":authority %s", value);

	send_ok(1, HTTP2_FLAG_END_STREAM);
	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
}

ZTEST(http2_client_peer, test_peer_request_colon_in_path)
{
	const char *path;
	int ret, count;

	peer_connect(true);

	memset(&req, 0, sizeof(req));
	ret = start_request(&req, HTTP_GET, "items:list", &rsp, TEST_REQUEST_TIMEOUT_MS);
	zassert_ok(ret, "GET failed (%d)", ret);

	count = expect_request(1);
	path = peer_header_value(hdrs, count, ":path");
	zassert_not_null(path, ":path missing");
	zassert_str_equal(path, "/items:list", ":path %s", path);

	send_ok(1, HTTP2_FLAG_END_STREAM);
	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
}

ZTEST(http2_client_peer, test_peer_request_connection_fields_dropped)
{
	static const struct http_client_header fields[] = {
		{ "Connection", "keep-alive" },
		{ "Keep-Alive", "timeout=5" },
		{ "Transfer-Encoding", "chunked" },
		{ "TE", "gzip" },
		{ "x-kept", "1" },
	};
	int ret, count;

	peer_connect(true);

	memset(&req, 0, sizeof(req));
	req.header_fields = fields;
	req.headers_count = ARRAY_SIZE(fields);
	ret = start_request(&req, HTTP_GET, "/", &rsp, TEST_REQUEST_TIMEOUT_MS);
	zassert_ok(ret, "GET failed (%d)", ret);

	count = expect_request(1);
	zassert_is_null(peer_header_value(hdrs, count, "connection"), "connection sent");
	zassert_is_null(peer_header_value(hdrs, count, "keep-alive"), "keep-alive sent");
	zassert_is_null(peer_header_value(hdrs, count, "transfer-encoding"),
			"transfer-encoding sent");
	zassert_is_null(peer_header_value(hdrs, count, "te"), "te sent");
	zassert_not_null(peer_header_value(hdrs, count, "x-kept"), "x-kept missing");

	send_ok(1, HTTP2_FLAG_END_STREAM);
	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
}

ZTEST(http2_client_peer, test_peer_request_content_length_dropped)
{
	static const struct http_client_header fields[] = {
		{ "Content-Length", "999" },
	};
	int ret, count, seen = 0;

	peer_connect(true);

	memset(&req, 0, sizeof(req));
	req.header_fields = fields;
	req.headers_count = ARRAY_SIZE(fields);
	req.payload = upload;
	req.payload_len = sizeof(upload);
	ret = start_request(&req, HTTP_POST, "/", &rsp, TEST_REQUEST_TIMEOUT_MS);
	zassert_ok(ret, "POST failed (%d)", ret);

	/* The library's own Content-Length is the only one. */
	count = expect_request(1);
	for (int i = 0; i < count; i++) {
		if (strcmp(hdrs[i].name, "content-length") == 0) {
			zassert_equal(strtoul(hdrs[i].value, NULL, 10), sizeof(upload),
				      "content-length %s", hdrs[i].value);
			seen++;
		}
	}

	zassert_equal(seen, 1, "content-length sent %d times", seen);

	recv_upload(sizeof(upload));
	send_ok(1, HTTP2_FLAG_END_STREAM);
	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
}

ZTEST(http2_client_peer, test_peer_options_asterisk)
{
	const char *path;
	int ret, count;

	peer_connect(true);

	memset(&req, 0, sizeof(req));
	ret = start_request(&req, HTTP_OPTIONS, "*", &rsp, TEST_REQUEST_TIMEOUT_MS);
	zassert_ok(ret, "OPTIONS failed (%d)", ret);

	count = expect_request(1);
	path = peer_header_value(hdrs, count, ":path");
	zassert_not_null(path, ":path missing");
	zassert_str_equal(path, "*", ":path %s", path);

	send_ok(1, HTTP2_FLAG_END_STREAM);
	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
}

static int host_headers_cb(const struct http_client_header **headers, size_t *count,
			   bool *more, void *user_data)
{
	static const struct http_client_header host = { "Host", "api.example.com" };

	ARG_UNUSED(user_data);

	*headers = &host;
	*count = 1;
	*more = false;

	return 0;
}

ZTEST(http2_client_peer, test_peer_request_host_from_callback_rejected)
{
	int ret;

	peer_connect(true);

	memset(&req, 0, sizeof(req));
	req.headers_cb = host_headers_cb;
	ret = start_request(&req, HTTP_GET, "/", &rsp, TEST_REQUEST_TIMEOUT_MS);
	zassert_equal(ret, -EINVAL, "Host from the callback accepted (%d)", ret);

	assert_session_usable(1);
}

ZTEST(http2_client_peer, test_peer_request_invalid_url_and_content_type)
{
	int ret;

	peer_connect(true);

	memset(&req, 0, sizeof(req));
	ret = start_request(&req, HTTP_GET, "/a\r\nx-injected: 1", &rsp,
			    TEST_REQUEST_TIMEOUT_MS);
	zassert_equal(ret, -EINVAL, "CRLF in the URL accepted (%d)", ret);

	memset(&req, 0, sizeof(req));
	ret = start_request(&req, HTTP_GET, "/a b", &rsp, TEST_REQUEST_TIMEOUT_MS);
	zassert_equal(ret, -EINVAL, "space in the URL accepted (%d)", ret);

	memset(&req, 0, sizeof(req));
	ret = start_request(&req, HTTP_GET, "https://example.com/x", &rsp,
			    TEST_REQUEST_TIMEOUT_MS);
	zassert_equal(ret, -EINVAL, "absolute URL accepted (%d)", ret);

	memset(&req, 0, sizeof(req));
	ret = start_request(&req, HTTP_GET, "/a#frag", &rsp, TEST_REQUEST_TIMEOUT_MS);
	zassert_equal(ret, -EINVAL, "fragment in the URL accepted (%d)", ret);

	memset(&req, 0, sizeof(req));
	req.content_type_value = "text/plain\r\nx-injected: 1";
	ret = start_request(&req, HTTP_GET, "/", &rsp, TEST_REQUEST_TIMEOUT_MS);
	zassert_equal(ret, -EINVAL, "CRLF in the content type accepted (%d)", ret);

	assert_session_usable(1);
}

ZTEST(http2_client_peer, test_peer_stream_ids_exhausted)
{
	int ret;

	peer_connect(true);

	/* The last identifier a client may use was handed out already. */
	client.session.parser.http2.last_stream_id = 0x7fffffff;

	ret = start_get("/", &rsp);
	zassert_equal(ret, -ESHUTDOWN, "request on a used-up connection returned %d", ret);
}

ZTEST(http2_client_peer, test_peer_init_unsupported_version)
{
	int ret;

	memset(&tcp_ctx, 0, sizeof(tcp_ctx));
	ret = http_client_init_tcp(&client, HTTP_CLIENT_VERSION_1_1, &tcp_ctx);
	zassert_equal(ret, -ENOTSUP, "HTTP/1.1 accepted without a protocol layer (%d)", ret);
}

ZTEST(http2_client_peer, test_peer_connect_rejected)
{
	static struct http_client_request req2;
	static struct test_response rsp2;
	static uint8_t buf2[64];
	int ret;

	peer_connect(true);

	/* A request the client cannot express is refused on its own, with the
	 * one already in flight unaffected.
	 */
	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	memset(&req2, 0, sizeof(req2));
	test_response_init(&rsp2, buf2, sizeof(buf2));
	ret = start_request(&req2, HTTP_CONNECT, "example.com:443", &rsp2,
			    TEST_REQUEST_TIMEOUT_MS);
	zassert_equal(ret, -ENOTSUP, "CONNECT accepted (%d)", ret);

	send_ok(1, HTTP2_FLAG_END_STREAM);
	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
}

static int upload_cb(size_t offset, const uint8_t **payload, size_t *len, bool *more,
		     void *user_data)
{
	size_t chunk = MIN(400, sizeof(upload) - offset);

	ARG_UNUSED(user_data);

	*payload = upload + offset;
	*len = chunk;
	*more = (offset + chunk < sizeof(upload));

	return 0;
}

ZTEST(http2_client_peer, test_peer_upload_cb)
{
	int ret, count;

	peer_connect(true);

	memset(&req, 0, sizeof(req));
	req.payload_cb = upload_cb;
	ret = start_request(&req, HTTP_POST, "/upload", &rsp, TEST_REQUEST_TIMEOUT_MS);
	zassert_ok(ret, "POST failed (%d)", ret);

	count = expect_request(1);
	zassert_is_null(peer_header_value(hdrs, count, "content-length"),
			"content-length sent for a callback payload");

	recv_upload(sizeof(upload));

	send_ok(1, HTTP2_FLAG_END_STREAM);
	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
}

#define SETTINGS_MAX_FRAME_SIZE_DEFAULT 16384
#define HPACK_SIZE_UPDATE_ZERO 0x20

ZTEST(http2_client_peer, test_peer_hpack_table_size_setting)
{
	int ret;

	peer_connect(true);

	/* A change of the peer's table size is acknowledged by a size update
	 * opening the next header block (RFC 7541, Section 4.2), and only
	 * that one.
	 */
	send_setting(HTTP2_SETTINGS_HEADER_TABLE_SIZE, 0);
	expect_settings_acks(2);

	/* A request refused before anything is sent does not use it up. */
	memset(&req, 0, sizeof(req));
	ret = start_request(&req, HTTP_GET, "/a b", &rsp, TEST_REQUEST_TIMEOUT_MS);
	zassert_equal(ret, -EINVAL, "space in the URL accepted (%d)", ret);
	test_response_init(&rsp, body_buf, sizeof(body_buf));

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);
	zassert_equal(frame.payload[0], HPACK_SIZE_UPDATE_ZERO, "block opens with 0x%02x",
		      frame.payload[0]);

	send_ok(1, HTTP2_FLAG_END_STREAM);
	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);

	test_response_init(&rsp, body_buf, sizeof(body_buf));
	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(3);
	zassert_not_equal(frame.payload[0], HPACK_SIZE_UPDATE_ZERO, "size update repeated");

	send_ok(3, HTTP2_FLAG_END_STREAM);
	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
}

ZTEST(http2_client_peer, test_peer_sync_request_in_flight)
{
	static struct test_response rsp2;
	static uint8_t buf2[64];
	struct http_client_request req2 = { 0 };
	int ret;

	peer_connect(false);

	/* A synchronous session takes one request at a time: a second one is
	 * refused, and the first is unaffected.
	 */
	memset(&req, 0, sizeof(req));
	req.method = HTTP_GET;
	req.url = "/";
	start_request_thread(TEST_REQUEST_TIMEOUT_MS);
	expect_request(1);

	test_response_init(&rsp2, buf2, sizeof(buf2));
	ret = start_request(&req2, HTTP_GET, "/again", &rsp2, TEST_REQUEST_TIMEOUT_MS);
	zassert_equal(ret, -EBUSY, "second request accepted (%d)", ret);
	assert_aborted(&rsp2, -EBUSY);

	send_ok(1, HTTP2_FLAG_END_STREAM);
	ret = join_request_thread();
	zassert_ok(ret, "request failed (%d)", ret);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
	zassert_equal(rsp.final_count, 1, "final reported %d times", rsp.final_count);
}

/* The recording callback, ending the session from inside the final and the
 * abort notification.
 */
static int disconnecting_response_cb(struct http_client *c,
				     const struct http_client_request *request,
				     struct http_client_response *response,
				     enum http_client_transaction_status status,
				     void *user_data)
{
	int ret = test_response_cb(c, request, response, status, user_data);

	if (status == HTTP_CLIENT_RESPONSE_DATA_FINAL ||
	    status == HTTP_CLIENT_TRANSACTION_ABORTED) {
		int dret = http_client_disconnect(c);

		zassert_ok(dret, "disconnect from the callback failed (%d)", dret);
	}

	return ret;
}

ZTEST(http2_client_peer, test_peer_disconnect_from_final_callback)
{
	int ret;

	peer_connect(false);

	memset(&req, 0, sizeof(req));
	req.method = HTTP_GET;
	req.url = "/";
	req.response_cb = disconnecting_response_cb;
	start_request_thread(TEST_REQUEST_TIMEOUT_MS);
	expect_request(1);

	/* A complete response stays complete when the session is ended from
	 * its final callback.
	 */
	send_ok(1, HTTP2_FLAG_END_STREAM);
	ret = join_request_thread();
	zassert_ok(ret, "request failed (%d)", ret);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
	zassert_equal(rsp.final_count, 1, "final reported %d times", rsp.final_count);
	zassert_false(rsp.aborted, "abort reported, err %d", rsp.error);
	zassert_equal(client.session.state, HTTP_CLIENT_SESSION_CLOSED, "session state %d",
		      client.session.state);

	(void)peer_drain(conn);
	zassert_false(peer_stats.rst_stream_seen, "RST_STREAM for a complete stream");
	zassert_true(peer_stats.goaway_seen, "no GOAWAY on disconnect");
	zassert_equal(peer_stats.goaway_error, HTTP2_ERROR_NO_ERROR, "GOAWAY error %u",
		      peer_stats.goaway_error);
}

ZTEST(http2_client_peer, test_peer_disconnect_from_abort_callback)
{
	uint8_t payload[RST_STREAM_LEN];
	int ret;

	peer_connect(false);

	memset(&req, 0, sizeof(req));
	req.method = HTTP_GET;
	req.url = "/";
	req.response_cb = disconnecting_response_cb;
	start_request_thread(TEST_REQUEST_TIMEOUT_MS);
	expect_request(1);

	/* An aborted transaction is reported once when the session is ended
	 * from its abort callback, with the original reason.
	 */
	sys_put_be32(HTTP2_ERROR_CANCEL, payload);
	ret = peer_send_frame(conn, HTTP2_RST_STREAM_FRAME, 0, 1, payload, sizeof(payload));
	zassert_ok(ret, "send RST_STREAM failed (%d)", ret);

	ret = join_request_thread();
	zassert_equal(ret, -ECONNRESET, "request returned %d", ret);
	zassert_equal(rsp.abort_count, 1, "abort reported %d times", rsp.abort_count);
	zassert_equal(rsp.error, -ECONNRESET, "abort reason %d", rsp.error);
	zassert_equal(client.session.state, HTTP_CLIENT_SESSION_CLOSED, "session state %d",
		      client.session.state);
}

ZTEST(http2_client_peer, test_peer_continuation_too_large)
{
	static uint8_t block[128];
	uint8_t hdr[HTTP2_FRAME_HEADER_SIZE];
	int ret, len, off, cut;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	/* A CONTINUATION frame joined to the field it completes is still
	 * bounded by SETTINGS_MAX_FRAME_SIZE; its header alone is enough to
	 * reject it.
	 */
	len = encode_split_block(block, sizeof(block), &off);
	cut = off + 3;

	ret = peer_send_frame(conn, HTTP2_HEADERS_FRAME, HTTP2_FLAG_END_STREAM, 1, block, cut);
	zassert_ok(ret, "send HEADERS failed (%d)", ret);
	peer_frame_header(hdr, SETTINGS_MAX_FRAME_SIZE_DEFAULT + 1, HTTP2_CONTINUATION_FRAME,
			  HTTP2_FLAG_END_HEADERS, 1);
	ret = peer_send_raw(conn, hdr, sizeof(hdr));
	zassert_ok(ret, "send failed (%d)", ret);

	assert_connection_error_code(-EBADMSG, HTTP2_ERROR_FRAME_SIZE_ERROR);
}

ZTEST(http2_client_peer, test_peer_first_frame_not_settings)
{
	struct net_sockaddr_in6 *sa = (struct net_sockaddr_in6 *)&cfg.server_addr;
	static const uint8_t ping[8] = { 0 };
	int ret;

	memset(&tcp_ctx, 0, sizeof(tcp_ctx));
	ret = http_client_init_tcp(&client, HTTP_CLIENT_VERSION_2, &tcp_ctx);
	zassert_ok(ret, "client init failed (%d)", ret);

	memset(&cfg, 0, sizeof(cfg));
	sa->sin6_family = NET_AF_INET6;
	sa->sin6_port = net_htons(PEER_PORT);
	sa->sin6_addr = net_in6addr_loopback;
	cfg.addrlen = sizeof(*sa);
	cfg.host = PEER_HOST;
	cfg.async = true;

	ret = http_client_connect(&client, &cfg);
	zassert_ok(ret, "connect failed (%d)", ret);
	conn = peer_accept(listen_fd);
	zassert_true(conn >= 0, "accept failed (%d)", conn);
	ret = peer_recv_preface(conn);
	zassert_ok(ret, "preface failed (%d)", ret);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	/* The server preface has to open with SETTINGS (RFC 9113, Section 3.4). */
	ret = peer_send_frame(conn, HTTP2_PING_FRAME, 0, 0, ping, sizeof(ping));
	zassert_ok(ret, "send PING failed (%d)", ret);

	assert_connection_error_code(-EBADMSG, HTTP2_ERROR_PROTOCOL_ERROR);
}

ZTEST(http2_client_peer, test_peer_trailers_content_length)
{
	static const struct http_header trailers[] = {
		{ "content-length", "5" },
	};
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	send_ok(1, 0);
	ret = peer_send_data(conn, 1, 0, (const uint8_t *)"hello", 5);
	zassert_ok(ret, "send DATA failed (%d)", ret);

	/* A framing field has no place in trailers. */
	ret = peer_send_headers(conn, 1, HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM,
				trailers, ARRAY_SIZE(trailers));
	zassert_ok(ret, "send trailers failed (%d)", ret);

	assert_stream_error(1, HTTP2_ERROR_PROTOCOL_ERROR, -EPROTO);
	assert_session_usable(3);
}

static int failing_socket_create(struct http_client *c, int af, int proto)
{
	ARG_UNUSED(c);
	ARG_UNUSED(af);
	ARG_UNUSED(proto);

	return -ENFILE;
}

ZTEST(http2_client_peer, test_peer_socket_create_failure)
{
	struct net_sockaddr_in6 *sa = (struct net_sockaddr_in6 *)&cfg.server_addr;
	int ret;

	/* The error of the socket creation callback is passed through. */
	memset(&tcp_ctx, 0, sizeof(tcp_ctx));
	tcp_ctx.socket_create = failing_socket_create;
	ret = http_client_init_tcp(&client, HTTP_CLIENT_VERSION_2, &tcp_ctx);
	zassert_ok(ret, "client init failed (%d)", ret);

	memset(&cfg, 0, sizeof(cfg));
	sa->sin6_family = NET_AF_INET6;
	sa->sin6_port = net_htons(PEER_PORT);
	sa->sin6_addr = net_in6addr_loopback;
	cfg.addrlen = sizeof(*sa);
	cfg.host = PEER_HOST;

	ret = http_client_connect(&client, &cfg);
	zassert_equal(ret, -ENFILE, "connect returned %d", ret);
	zassert_equal(client.session.state, HTTP_CLIENT_SESSION_CLOSED, "session state %d",
		      client.session.state);
}

ZTEST(http2_client_peer, test_peer_status_uppercase)
{
	/* A literal field named ":STATUS" with the value 200. */
	static const uint8_t block[] = { 0x40, 0x07, ':', 'S', 'T', 'A', 'T', 'U', 'S',
					 0x03, '2', '0', '0' };
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	/* Pseudo-header names are lowercase too (RFC 9113, Section 8.2.1). */
	ret = peer_send_frame(conn, HTTP2_HEADERS_FRAME,
			      HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM, 1, block,
			      sizeof(block));
	zassert_ok(ret, "send HEADERS failed (%d)", ret);

	assert_stream_error(1, HTTP2_ERROR_PROTOCOL_ERROR, -EPROTO);
	assert_session_usable(3);
}

ZTEST(http2_client_peer, test_peer_malformed_fields)
{
	/* Literal fields after an indexed :status 200: a space in the name, a
	 * colon inside the name, a NUL in the value, CR and LF in the value,
	 * and a value starting with a space (RFC 9113, Section 8.2.1).
	 */
	static const uint8_t name_space[] = { 0x88, 0x00, 0x05, 'x', ' ', 'b', 'a', 'd',
					      0x01, '1' };
	static const uint8_t name_colon[] = { 0x88, 0x00, 0x03, 'a', ':', 'b', 0x01, '1' };
	static const uint8_t value_nul[] = { 0x88, 0x00, 0x01, 'x', 0x03, 'a', 0x00, 'b' };
	static const uint8_t value_crlf[] = { 0x88, 0x00, 0x01, 'x', 0x03, 'a', '\r', '\n' };
	static const uint8_t value_space[] = { 0x88, 0x00, 0x01, 'x', 0x02, ' ', 'a' };
	static const struct {
		const uint8_t *block;
		size_t len;
	} blocks[] = {
		{ name_space, sizeof(name_space) }, { name_colon, sizeof(name_colon) },
		{ value_nul, sizeof(value_nul) },   { value_crlf, sizeof(value_crlf) },
		{ value_space, sizeof(value_space) },
	};
	uint32_t stream_id = 1;
	int ret;

	peer_connect(true);

	ARRAY_FOR_EACH(blocks, i) {
		test_response_init(&rsp, body_buf, sizeof(body_buf));
		ret = start_get("/", &rsp);
		zassert_ok(ret, "GET failed (%d)", ret);
		expect_request(stream_id);

		ret = peer_send_frame(conn, HTTP2_HEADERS_FRAME,
				      HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM, stream_id,
				      blocks[i].block, blocks[i].len);
		zassert_ok(ret, "send HEADERS failed (%d)", ret);

		assert_stream_error(stream_id, HTTP2_ERROR_PROTOCOL_ERROR, -EPROTO);
		stream_id += 2;
	}

	assert_session_usable(stream_id);
}

ZTEST(http2_client_peer, test_peer_response_header_exceeds_headers_buffer)
{
	/* An indexed :status 200 and a literal "x" with a 502 byte value, sent
	 * as is: it fits the receive buffer but not the headers buffer, even
	 * empty.
	 */
	static uint8_t block[7 + 502];
	int ret;

	block[0] = 0x88;
	block[1] = 0x00;
	block[2] = 0x01;
	block[3] = 'x';
	block[4] = 0x7f;
	block[5] = 0xf7;
	block[6] = 0x02;
	memset(block + 7, 'a', 502);

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	ret = peer_send_frame(conn, HTTP2_HEADERS_FRAME,
			      HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM, 1, block,
			      sizeof(block));
	zassert_ok(ret, "send HEADERS failed (%d)", ret);

	assert_stream_error(1, HTTP2_ERROR_INTERNAL_ERROR, -ENOBUFS);
	zassert_equal(rsp.header_blocks, 0, "headers reported %d times", rsp.header_blocks);
	assert_session_usable(3);
}

ZTEST(http2_client_peer, test_peer_priority_too_short)
{
	static const uint8_t payload[3] = { 0 };
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	/* Three bytes cannot hold the five-byte priority field. */
	ret = peer_send_frame(conn, HTTP2_HEADERS_FRAME,
			      HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM | HTTP2_FLAG_PRIORITY,
			      1, payload, sizeof(payload));
	zassert_ok(ret, "send HEADERS failed (%d)", ret);

	assert_connection_error_code(-EBADMSG, HTTP2_ERROR_FRAME_SIZE_ERROR);
}

ZTEST(http2_client_peer, test_peer_response_headers_overflow_rejected)
{
	static char names[12][8];
	static char values[12][41];
	struct http_header headers[13];
	int ret;

	headers[0].name = ":status";
	headers[0].value = "200";

	/* Twelve headers of 40 bytes exceed the 512-byte headers buffer, and
	 * the application refuses the first part handed out: the response is
	 * aborted with its error and the rest of the block is dropped.
	 */
	for (int i = 0; i < 12; i++) {
		snprintk(names[i], sizeof(names[i]), "x-h%02d", i);
		memset(values[i], 'a' + i, sizeof(values[i]) - 1);
		headers[i + 1].name = names[i];
		headers[i + 1].value = values[i];
	}

	peer_connect(true);

	rsp.header_cb_ret = -ENOMEM;
	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	ret = peer_send_headers(conn, 1, HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM,
				headers, ARRAY_SIZE(headers));
	zassert_ok(ret, "send HEADERS failed (%d)", ret);

	assert_stream_error(1, HTTP2_ERROR_CANCEL, -ENOMEM);
	zassert_equal(rsp.header_blocks, 1, "headers reported %d times", rsp.header_blocks);
	assert_session_usable(3);
}

ZTEST(http2_client_peer, test_peer_response_headers_overflow_without_status)
{
	static char names[13][8];
	static char values[13][41];
	struct http_header headers[13];
	int ret;

	/* Thirteen headers of 40 bytes exceed the 512-byte headers buffer
	 * before any :status, so the block is malformed when its first part
	 * is handed out.
	 */
	for (int i = 0; i < ARRAY_SIZE(headers); i++) {
		snprintk(names[i], sizeof(names[i]), "x-h%02d", i);
		memset(values[i], 'a' + i, sizeof(values[i]) - 1);
		headers[i].name = names[i];
		headers[i].value = values[i];
	}

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	ret = peer_send_headers(conn, 1, HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM,
				headers, ARRAY_SIZE(headers));
	zassert_ok(ret, "send HEADERS failed (%d)", ret);

	assert_stream_error(1, HTTP2_ERROR_PROTOCOL_ERROR, -EPROTO);
	zassert_equal(rsp.header_blocks, 0, "headers reported %d times", rsp.header_blocks);
	assert_session_usable(3);
}

ZTEST(http2_client_peer, test_peer_timeout_send_failure)
{
	peer_connect_failing();

	memset(&req, 0, sizeof(req));
	req.method = HTTP_GET;
	req.url = "/";
	start_request_thread(SHORT_TIMEOUT_MS);
	expect_request(1);
	/* The handshake SETTINGS are only acknowledged once the request reads. */
	expect_settings_acks(1);

	/* The RST_STREAM sent on the timeout fails after five bytes: the
	 * session ends and the request reports the send error, not the
	 * timeout.
	 */
	ft.fail_after = 5;
	ft.armed = true;

	assert_send_failed();
}

ZTEST(http2_client_peer, test_peer_unknown_pseudo_header)
{
	/* :status 200, then a literal field named ":foo". */
	static const uint8_t block[] = { 0x88, 0x40, 0x04, ':', 'f', 'o', 'o', 0x01, 'v' };
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	/* :status is the only response pseudo-header (RFC 9113, Section 8.3.2). */
	ret = peer_send_frame(conn, HTTP2_HEADERS_FRAME,
			      HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM, 1, block,
			      sizeof(block));
	zassert_ok(ret, "send HEADERS failed (%d)", ret);

	assert_stream_error(1, HTTP2_ERROR_PROTOCOL_ERROR, -EPROTO);
	assert_session_usable(3);
}

ZTEST(http2_client_peer, test_peer_304_with_body)
{
	static const struct http_header not_modified[] = {
		{ ":status", "304" },
	};
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	ret = peer_send_headers(conn, 1, HTTP2_FLAG_END_HEADERS, not_modified,
				ARRAY_SIZE(not_modified));
	zassert_ok(ret, "send HEADERS failed (%d)", ret);
	ret = peer_send_data(conn, 1, HTTP2_FLAG_END_STREAM, (const uint8_t *)"hello", 5);
	zassert_ok(ret, "send DATA failed (%d)", ret);

	assert_stream_error(1, HTTP2_ERROR_PROTOCOL_ERROR, -EPROTO);
	zassert_equal(rsp.body_chunks, 0, "body delivered on 304");
	assert_session_usable(3);
}

ZTEST(http2_client_peer, test_peer_split_field_other_stream)
{
	static uint8_t block[128];
	int ret, len, off, cut;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	/* A field cut in two can only continue on the same stream. */
	len = encode_split_block(block, sizeof(block), &off);
	cut = off + 3;

	ret = peer_send_frame(conn, HTTP2_HEADERS_FRAME, HTTP2_FLAG_END_STREAM, 1, block, cut);
	zassert_ok(ret, "send HEADERS failed (%d)", ret);
	ret = peer_send_frame(conn, HTTP2_CONTINUATION_FRAME, HTTP2_FLAG_END_HEADERS, 3,
			      block + cut, len - cut);
	zassert_ok(ret, "send CONTINUATION failed (%d)", ret);

	assert_connection_error_code(-EBADMSG, HTTP2_ERROR_PROTOCOL_ERROR);
}

ZTEST(http2_client_peer, test_peer_priority_bad_length_on_stream_zero)
{
	static const uint8_t payload[3];
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	/* Wrong length or not, a PRIORITY frame on stream 0 fails the
	 * connection (RFC 9113, Section 6.3).
	 */
	ret = peer_send_frame(conn, HTTP2_PRIORITY_FRAME, 0, 0, payload, sizeof(payload));
	zassert_ok(ret, "send PRIORITY failed (%d)", ret);

	assert_connection_error_code(-EBADMSG, HTTP2_ERROR_PROTOCOL_ERROR);
}

ZTEST(http2_client_peer, test_peer_headers_empty_with_fields)
{
	int ret;

	peer_connect(true);

	ret = start_get("/", &rsp);
	zassert_ok(ret, "GET failed (%d)", ret);
	expect_request(1);

	/* No room for the Pad Length and priority fields the flags announce. */
	ret = peer_send_frame(conn, HTTP2_HEADERS_FRAME,
			      HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_PADDED | HTTP2_FLAG_PRIORITY, 1,
			      NULL, 0);
	zassert_ok(ret, "send HEADERS failed (%d)", ret);

	assert_connection_error_code(-EBADMSG, HTTP2_ERROR_FRAME_SIZE_ERROR);
}

/* Hands the big upload out slowly, 1000 bytes every few milliseconds. */
static int slow_upload_cb(size_t offset, const uint8_t **payload, size_t *len, bool *more,
			  void *user_data)
{
	size_t chunk = MIN(1000, sizeof(big_upload) - offset);

	ARG_UNUSED(user_data);

	k_sleep(K_MSEC(5));

	*payload = big_upload + offset;
	*len = chunk;
	*more = (offset + chunk < sizeof(big_upload));

	return 0;
}

ZTEST(http2_client_peer, test_peer_upload_reset_while_sending)
{
	uint8_t payload[RST_STREAM_LEN];
	int ret;

	peer_connect(true);

	/* Plenty of credit, so that the sender never waits for the window. */
	send_window_update(0, 1000000);

	memset(&req, 0, sizeof(req));
	req.method = HTTP_PUT;
	req.url = "/upload";
	req.payload_cb = slow_upload_cb;
	start_request_thread(TEST_REQUEST_TIMEOUT_MS);
	expect_request(1);
	send_window_update(1, 1000000);

	/* A reset arriving while the request is being sent is acted on at
	 * once: the upload stops short of its end.
	 */
	ret = peer_recv_frame_type(conn, HTTP2_DATA_FRAME, &frame);
	zassert_ok(ret, "no DATA from client (%d)", ret);

	sys_put_be32(HTTP2_ERROR_CANCEL, payload);
	ret = peer_send_frame(conn, HTTP2_RST_STREAM_FRAME, 0, 1, payload, sizeof(payload));
	zassert_ok(ret, "send RST_STREAM failed (%d)", ret);

	ret = join_request_thread();
	zassert_equal(ret, -ECONNRESET, "request returned %d", ret);
	assert_aborted(&rsp, -ECONNRESET);

	(void)peer_drain(conn);
	zassert_true(peer_stats.data_bytes < sizeof(big_upload) / 2,
		     "%zu of %zu bytes sent after the reset", peer_stats.data_bytes,
		     sizeof(big_upload));
	zassert_false(peer_stats.end_stream_seen, "upload completed after the reset");
}

/* Hands the fields out after a pause, with the TX lock held meanwhile. */
static int slow_headers_cb(const struct http_client_header **headers, size_t *count,
			   bool *more, void *user_data)
{
	static const struct http_client_header fields[] = { { "x-slow", "1" } };

	ARG_UNUSED(user_data);

	k_sleep(K_MSEC(100));

	*headers = fields;
	*count = ARRAY_SIZE(fields);
	*more = false;

	return 0;
}

ZTEST(http2_client_peer, test_peer_two_senders_in_order)
{
	int ret, count;

	peer_connect(true);

	/* Two threads send at once; the slow one holds the wire first. The
	 * stream identifiers follow the order the HEADERS frames go out in,
	 * and both requests complete.
	 */
	memset(&req, 0, sizeof(req));
	req.method = HTTP_GET;
	req.url = "/slow";
	req.headers_cb = slow_headers_cb;
	start_request_thread(TEST_REQUEST_TIMEOUT_MS);
	k_sleep(K_MSEC(20));

	memset(&req2, 0, sizeof(req2));
	req2.method = HTTP_GET;
	req2.url = "/fast";
	start_request2_thread();

	count = expect_request(1);
	zassert_str_equal(peer_header_value(hdrs, count, ":path"), "/slow", "stream 1 path");
	count = expect_request(3);
	zassert_str_equal(peer_header_value(hdrs, count, ":path"), "/fast", "stream 3 path");

	ret = join_request_thread();
	zassert_ok(ret, "first request failed (%d)", ret);
	ret = join_request2_thread();
	zassert_ok(ret, "second request failed (%d)", ret);

	send_ok(1, HTTP2_FLAG_END_STREAM);
	send_ok(3, HTTP2_FLAG_END_STREAM);
	wait_done(&rsp);
	wait_done(&rsp2);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
	zassert_equal(rsp2.status, 200, "status %u", rsp2.status);
}

ZTEST(http2_client_peer, test_peer_pings_while_sending)
{
	static const uint8_t opaque[2][HTTP2_PING_FRAME_LEN] = { "ping0001", "ping0002" };
	int ret;

	peer_connect(true);

	/* PINGs received while a request frame is on its way out are
	 * acknowledged once it is, each with its own payload, in order.
	 */
	memset(&req, 0, sizeof(req));
	req.method = HTTP_GET;
	req.url = "/slow";
	req.headers_cb = slow_headers_cb;
	start_request_thread(TEST_REQUEST_TIMEOUT_MS);
	k_sleep(K_MSEC(20));

	ARRAY_FOR_EACH(opaque, i) {
		ret = peer_send_frame(conn, HTTP2_PING_FRAME, 0, 0, opaque[i], sizeof(opaque[i]));
		zassert_ok(ret, "send PING failed (%d)", ret);
	}

	expect_request(1);

	ARRAY_FOR_EACH(opaque, i) {
		ret = peer_recv_frame_type(conn, HTTP2_PING_FRAME, &frame);
		zassert_ok(ret, "no PING ACK %zu from client (%d)", i, ret);
		zassert_true(frame.flags & FLAG_ACK, "PING without ACK flag");
		zassert_equal(frame.len, sizeof(opaque[i]), "PING ACK length %u", frame.len);
		zassert_mem_equal(frame.payload, opaque[i], sizeof(opaque[i]),
				  "PING ACK %zu data mismatch", i);
	}

	ret = join_request_thread();
	zassert_ok(ret, "request failed (%d)", ret);
	send_ok(1, HTTP2_FLAG_END_STREAM);
	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
}

ZTEST(http2_client_peer, test_peer_ping_queue_full_while_sending)
{
	static const uint8_t opaque[HTTP2_CLIENT_DEFERRED_PINGS + 2][HTTP2_PING_FRAME_LEN] = {
		"ping0001", "ping0002", "ping0003", "ping0004", "ping0005", "ping0006",
	};
	int ret;

	peer_connect(true);

	/* More PINGs than can be held back behind a request frame: the most
	 * recent ones are acknowledged, in order, and the connection stays.
	 */
	memset(&req, 0, sizeof(req));
	req.method = HTTP_GET;
	req.url = "/slow";
	req.headers_cb = slow_headers_cb;
	start_request_thread(TEST_REQUEST_TIMEOUT_MS);
	k_sleep(K_MSEC(20));

	ARRAY_FOR_EACH(opaque, i) {
		ret = peer_send_frame(conn, HTTP2_PING_FRAME, 0, 0, opaque[i], sizeof(opaque[i]));
		zassert_ok(ret, "send PING failed (%d)", ret);
	}

	expect_request(1);

	for (size_t i = ARRAY_SIZE(opaque) - HTTP2_CLIENT_DEFERRED_PINGS; i < ARRAY_SIZE(opaque);
	     i++) {
		ret = peer_recv_frame_type(conn, HTTP2_PING_FRAME, &frame);
		zassert_ok(ret, "no PING ACK %zu from client (%d)", i, ret);
		zassert_true(frame.flags & FLAG_ACK, "PING without ACK flag");
		zassert_mem_equal(frame.payload, opaque[i], sizeof(opaque[i]),
				  "PING ACK %zu data mismatch", i);
	}

	/* The older ones go unanswered. */
	ret = peer_set_recv_timeout(conn, 200);
	zassert_ok(ret, "set timeout failed (%d)", ret);
	while (peer_recv_frame(conn, &frame) == 0) {
		zassert_not_equal(frame.type, HTTP2_PING_FRAME, "extra PING ACK");
	}
	ret = peer_set_recv_timeout(conn, PEER_RECV_TIMEOUT_MS);
	zassert_ok(ret, "set timeout failed (%d)", ret);

	ret = join_request_thread();
	zassert_ok(ret, "request failed (%d)", ret);
	send_ok(1, HTTP2_FLAG_END_STREAM);
	wait_done(&rsp);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
}

ZTEST(http2_client_peer, test_peer_settings_flood_while_sending)
{
	static uint8_t frames[(UINT8_MAX + 1) * HTTP2_FRAME_HEADER_SIZE];
	uint8_t byte;
	int ret;

	peer_connect(true);

	/* More SETTINGS than can be acknowledged behind a request frame end
	 * the connection.
	 */
	memset(&req, 0, sizeof(req));
	req.method = HTTP_GET;
	req.url = "/slow";
	req.headers_cb = slow_headers_cb;
	start_request_thread(TEST_REQUEST_TIMEOUT_MS);
	k_sleep(K_MSEC(20));

	for (size_t i = 0; i <= UINT8_MAX; i++) {
		peer_frame_header(frames + i * HTTP2_FRAME_HEADER_SIZE, 0, HTTP2_SETTINGS_FRAME, 0,
				  0);
	}

	ret = peer_send_raw(conn, frames, sizeof(frames));
	zassert_ok(ret, "send SETTINGS failed (%d)", ret);

	ret = join_request_thread();
	zassert_equal(ret, -EBADMSG, "request returned %d", ret);
	assert_aborted(&rsp, -EBADMSG);
	zassert_equal(client.session.state, HTTP_CLIENT_SESSION_CLOSED, "session state %d",
		      client.session.state);
	zassert_equal(client.session.parser.http2.goaway_error, HTTP2_ERROR_ENHANCE_YOUR_CALM,
		      "connection error code %u", client.session.parser.http2.goaway_error);

	(void)peer_drain(conn);
	ret = zsock_recv(conn, &byte, 1, 0);
	zassert_equal(ret, 0, "transport still open (%d)", ret);
}

static K_SEM_DEFINE(upload_gate, 0, 1);

/* Hands a single chunk out once the gate is given. */
static int gated_upload_cb(size_t offset, const uint8_t **payload, size_t *len, bool *more,
			   void *user_data)
{
	ARG_UNUSED(offset);
	ARG_UNUSED(user_data);

	(void)k_sem_take(&upload_gate, K_SECONDS(5));

	*payload = big_upload;
	*len = 1000;
	*more = false;

	return 0;
}

ZTEST(http2_client_peer, test_peer_data_credit_after_settings)
{
	int ret;

	peer_connect(true);

	/* The upload waits at the gate with its HEADERS out. */
	memset(&req, 0, sizeof(req));
	req.method = HTTP_PUT;
	req.url = "/upload";
	req.payload_cb = gated_upload_cb;
	start_request_thread(TEST_REQUEST_TIMEOUT_MS);
	expect_request(1);

	/* A second sender holds the wire for a while. The upload lines up
	 * behind it, and the peer meanwhile takes the stream window away:
	 * the DATA frame must respect that once the SETTINGS is acknowledged.
	 */
	memset(&req2, 0, sizeof(req2));
	req2.method = HTTP_GET;
	req2.url = "/slow";
	req2.headers_cb = slow_headers_cb;
	start_request2_thread();
	k_sleep(K_MSEC(20));

	k_sem_give(&upload_gate);
	k_sleep(K_MSEC(20));
	send_setting(HTTP2_SETTINGS_INITIAL_WINDOW_SIZE, 0);

	expect_request(3);
	expect_settings_acks(2);

	ret = peer_set_recv_timeout(conn, 200);
	zassert_ok(ret, "set timeout failed (%d)", ret);
	while (peer_recv_frame(conn, &frame) == 0) {
		zassert_not_equal(frame.type, HTTP2_DATA_FRAME, "DATA sent without credit");
	}
	ret = peer_set_recv_timeout(conn, PEER_RECV_TIMEOUT_MS);
	zassert_ok(ret, "set timeout failed (%d)", ret);

	send_window_update(1, 2000);
	ret = peer_recv_frame_type(conn, HTTP2_DATA_FRAME, &frame);
	zassert_ok(ret, "no DATA after the window update (%d)", ret);
	zassert_equal(frame.len, 1000, "DATA length %u", frame.len);

	ret = join_request_thread();
	zassert_ok(ret, "upload failed (%d)", ret);
	ret = join_request2_thread();
	zassert_ok(ret, "second request failed (%d)", ret);

	send_ok(1, HTTP2_FLAG_END_STREAM);
	send_ok(3, HTTP2_FLAG_END_STREAM);
	wait_done(&rsp);
	wait_done(&rsp2);
	zassert_equal(rsp.status, 200, "status %u", rsp.status);
	zassert_equal(rsp2.status, 200, "status %u", rsp2.status);
}

ZTEST(http2_client_peer, test_peer_connection_error_while_sending)
{
	uint8_t settings[6] = { 0 };
	uint8_t byte;
	int ret;

	peer_connect(true);

	send_window_update(0, 1000000);

	memset(&req, 0, sizeof(req));
	req.method = HTTP_PUT;
	req.url = "/upload";
	req.payload_cb = slow_upload_cb;
	start_request_thread(TEST_REQUEST_TIMEOUT_MS);
	expect_request(1);
	send_window_update(1, 1000000);

	ret = peer_recv_frame_type(conn, HTTP2_DATA_FRAME, &frame);
	zassert_ok(ret, "no DATA from client (%d)", ret);

	/* A protocol violation ends the session under the sender: the
	 * request fails, and the transport is closed once the sender is
	 * done with it.
	 */
	ret = peer_send_frame(conn, HTTP2_SETTINGS_FRAME, 0, 1, settings, sizeof(settings));
	zassert_ok(ret, "send SETTINGS failed (%d)", ret);

	ret = join_request_thread();
	zassert_equal(ret, -EBADMSG, "request returned %d", ret);
	assert_aborted(&rsp, -EBADMSG);
	zassert_equal(client.session.state, HTTP_CLIENT_SESSION_CLOSED, "session state %d",
		      client.session.state);

	(void)peer_drain(conn);
	zassert_true(peer_stats.goaway_seen, "no GOAWAY from client");
	ret = zsock_recv(conn, &byte, 1, 0);
	zassert_equal(ret, 0, "transport still open (%d)", ret);
}

/* Hands the big upload out very slowly, to keep the sender busy. */
static int very_slow_upload_cb(size_t offset, const uint8_t **payload, size_t *len,
			       bool *more, void *user_data)
{
	size_t chunk = MIN(1000, sizeof(big_upload) - offset);

	ARG_UNUSED(user_data);

	k_sleep(K_MSEC(300));

	*payload = big_upload + offset;
	*len = chunk;
	*more = (offset + chunk < sizeof(big_upload));

	return 0;
}

ZTEST(http2_client_peer, test_peer_upload_timeout_while_sending)
{
	int ret;

	peer_connect(true);

	/* Credit never runs out, so only the deadline can stop an upload the
	 * application hands out too slowly.
	 */
	send_window_update(0, 1000000);
	memset(&req, 0, sizeof(req));
	req.method = HTTP_PUT;
	req.url = "/upload";
	req.payload_cb = very_slow_upload_cb;
	start_request_thread(SHORT_TIMEOUT_MS);
	expect_request(1);
	send_window_update(1, 1000000);

	ret = join_request_thread();
	zassert_equal(ret, -ETIMEDOUT, "request returned %d", ret);
	assert_aborted(&rsp, -ETIMEDOUT);
	zassert_equal(rsp.abort_count, 1, "abort reported %d times", rsp.abort_count);

	ret = peer_recv_frame_type(conn, HTTP2_RST_STREAM_FRAME, &frame);
	zassert_ok(ret, "no RST_STREAM after timeout (%d)", ret);
	zassert_equal(peer_stats.rst_error, HTTP2_ERROR_CANCEL, "RST_STREAM error %u",
		      peer_stats.rst_error);
}

ZTEST(http2_client_peer, test_peer_connect_while_sending)
{
	int ret;

	peer_connect(true);

	memset(&req, 0, sizeof(req));
	req.method = HTTP_PUT;
	req.url = "/upload";
	req.payload_cb = very_slow_upload_cb;
	start_request_thread(TEST_REQUEST_TIMEOUT_MS);
	expect_request(1);

	/* The peer goes away while the sender waits in its callback: the
	 * session ends, but a new one cannot start before the sender has
	 * returned and freed its stream.
	 */
	zsock_close(conn);
	conn = -1;
	wait_done(&rsp);
	assert_aborted(&rsp, -ENOTCONN);

	ret = http_client_connect(&client, &cfg);
	zassert_equal(ret, -EBUSY, "connect with a sender still in flight returned %d", ret);

	ret = join_request_thread();
	zassert_true(ret < 0, "request succeeded on a dead session");

	ret = http_client_connect(&client, &cfg);
	zassert_ok(ret, "connect failed (%d)", ret);
	conn = peer_accept(listen_fd);
	zassert_true(conn >= 0, "accept failed (%d)", conn);
	ret = peer_handshake(conn);
	zassert_ok(ret, "handshake failed (%d)", ret);
	assert_session_usable(1);
}

ZTEST(http2_client_peer, test_peer_disconnect_while_sending)
{
	uint8_t byte;
	int ret;

	peer_connect(true);

	memset(&req, 0, sizeof(req));
	req.method = HTTP_PUT;
	req.url = "/upload";
	req.payload_cb = very_slow_upload_cb;
	start_request_thread(TEST_REQUEST_TIMEOUT_MS);
	expect_request(1);

	/* Disconnecting under a sender ends its request, and by the time the
	 * call returns the transport is closed and the sender has returned.
	 */
	ret = http_client_disconnect(&client);
	zassert_ok(ret, "disconnect failed (%d)", ret);
	zassert_equal(client.session.state, HTTP_CLIENT_SESSION_CLOSED, "session state %d",
		      client.session.state);
	ret = k_thread_join(&req_thread, K_MSEC(50));
	zassert_ok(ret, "disconnect returned with the sender still inside (%d)", ret);

	(void)peer_drain(conn);
	ret = zsock_recv(conn, &byte, 1, 0);
	zassert_equal(ret, 0, "transport still open (%d)", ret);

	ret = join_request_thread();
	zassert_true(ret < 0, "request succeeded on a closed session (%d)", ret);
	assert_aborted(&rsp, -ECONNABORTED);
}

ZTEST(http2_client_peer, test_peer_disconnect_under_stalled_send)
{
	uint8_t byte;
	int ret;

	peer_connect_failing();

	/* The request's HEADERS frame stalls in the transport, as on a peer
	 * that stopped reading, with the sender holding the transport.
	 */
	ft.block_ms = 300;
	memset(&req, 0, sizeof(req));
	req.method = HTTP_GET;
	req.url = "/";
	start_request_thread(TEST_REQUEST_TIMEOUT_MS);
	ret = k_sem_take(&ft.stalled, K_SECONDS(1));
	zassert_ok(ret, "send not stalled");

	/* Disconnecting meanwhile cannot pull the transport from under the
	 * sender: it waits for the frame to be out, the sender closes the
	 * transport, and the call returns with it closed.
	 */
	ret = http_client_disconnect(&client);
	zassert_ok(ret, "disconnect failed (%d)", ret);
	zassert_true(k_uptime_get() - ft.stall_start >= 300,
		     "disconnect returned before the stalled send was over");
	zassert_equal(ft.sock, -1, "transport not closed when disconnect returned");
	zassert_false(client.session.close_pending, "close left pending");
	zassert_equal(client.session.state, HTTP_CLIENT_SESSION_CLOSED, "session state %d",
		      client.session.state);

	ret = join_request_thread();
	zassert_equal(ret, -ECONNABORTED, "request returned %d", ret);
	assert_aborted(&rsp, -ECONNABORTED);

	/* The stalled frame went out whole before the close. */
	ret = peer_recv_frame_type(conn, HTTP2_HEADERS_FRAME, &frame);
	zassert_ok(ret, "HEADERS not received whole (%d)", ret);
	(void)peer_drain(conn);
	ret = zsock_recv(conn, &byte, 1, 0);
	zassert_equal(ret, 0, "transport still open (%d)", ret);
}

ZTEST(http2_client_peer, test_peer_disconnect_while_waiting)
{
	int ret;

	peer_connect_failing();

	/* A synchronous request waiting for its response is aborted by a
	 * disconnect from another thread, which returns only once the waiting
	 * thread is out of http_client_send_request(), however slowly the
	 * transport lets it go.
	 */
	memset(&req, 0, sizeof(req));
	req.method = HTTP_GET;
	req.url = "/";
	start_request_thread(TEST_REQUEST_TIMEOUT_MS);
	expect_request(1);
	k_sleep(K_MSEC(20));
	zassert_true(ft.waiting, "request not waiting for the response");
	ft.wait_delay_ms = 200;

	ret = http_client_disconnect(&client);
	zassert_ok(ret, "disconnect failed (%d)", ret);
	zassert_false(ft.waiting, "disconnect returned with the request still waiting");
	zassert_equal(client.callers, 0, "%u callers left", client.callers);

	ret = join_request_thread();
	zassert_equal(ret, -ECONNABORTED, "request returned %d", ret);
	assert_aborted(&rsp, -ECONNABORTED);
	zassert_equal(client.session.state, HTTP_CLIENT_SESSION_CLOSED, "session state %d",
		      client.session.state);
}

ZTEST(http2_client_peer, test_peer_timeout_disconnect_from_callback)
{
	int ret;

	peer_connect(false);

	memset(&req, 0, sizeof(req));
	req.method = HTTP_GET;
	req.url = "/";
	req.response_cb = disconnecting_response_cb;
	start_request_thread(SHORT_TIMEOUT_MS);
	expect_request(1);

	/* No response: the request times out, and ending the session from
	 * its abort callback does not report it a second time.
	 */
	ret = join_request_thread();
	zassert_equal(ret, -ETIMEDOUT, "request returned %d", ret);
	zassert_equal(rsp.abort_count, 1, "abort reported %d times", rsp.abort_count);
	zassert_equal(rsp.error, -ETIMEDOUT, "abort reason %d", rsp.error);
	zassert_equal(client.session.state, HTTP_CLIENT_SESSION_CLOSED, "session state %d",
		      client.session.state);
}

ZTEST(http2_client_peer, test_peer_session_start_failure)
{
	int ret;

	failing_client_init();

	/* The preface cannot be sent: the transport is closed with the client
	 * lock released, as a notification of it may be waiting for the lock.
	 */
	ft.fail_after = 0;
	ft.armed = true;
	ft.notify_on_close = true;
	ret = http_client_connect(&client, &cfg);
	zassert_equal(ret, -EIO, "connect returned %d", ret);
	zassert_false(ft.close_blocked, "transport closed with the client lock held");
	zassert_equal(ft.sock, -1, "transport not closed");
	zassert_equal(client.session.state, HTTP_CLIENT_SESSION_CLOSED, "session state %d",
		      client.session.state);

	/* The peer saw the connection end, and the session can be opened
	 * again right away.
	 */
	conn = peer_accept(listen_fd);
	zassert_true(conn >= 0, "accept failed (%d)", conn);
	(void)zsock_close(conn);

	ret = http_client_connect(&client, &cfg);
	zassert_ok(ret, "connect failed (%d)", ret);
	conn = peer_accept(listen_fd);
	zassert_true(conn >= 0, "accept failed (%d)", conn);
	ret = peer_recv_preface(conn);
	zassert_ok(ret, "preface failed (%d)", ret);
}

/* Hands the whole payload out at once, stalling the send that follows. */
static int stalling_payload_cb(size_t offset, const uint8_t **payload, size_t *len, bool *more,
			       void *user_data)
{
	ARG_UNUSED(offset);
	ARG_UNUSED(user_data);

	ft.block_ms = 300;

	*payload = big_upload;
	*len = 10;
	*more = false;

	return 0;
}

ZTEST(http2_client_peer, test_peer_reset_during_final_frame)
{
	uint8_t error[RST_STREAM_LEN];
	int ret;

	/* An asynchronous session over the failing transport, received on
	 * this thread by hand.
	 */
	failing_client_init();
	cfg.async = true;
	ret = http_client_connect(&client, &cfg);
	zassert_ok(ret, "connect failed (%d)", ret);
	conn = peer_accept(listen_fd);
	zassert_true(conn >= 0, "accept failed (%d)", conn);
	ret = peer_handshake(conn);
	zassert_ok(ret, "handshake failed (%d)", ret);
	http_client_received_data(&client, 0);

	/* The peer resets the stream while its last frame is on its way out:
	 * the send succeeds, the call still reports how the transaction ended.
	 */
	memset(&req, 0, sizeof(req));
	req.method = HTTP_PUT;
	req.url = "/upload";
	req.payload_cb = stalling_payload_cb;
	start_request_thread(TEST_REQUEST_TIMEOUT_MS);
	expect_request(1);
	ret = k_sem_take(&ft.stalled, K_SECONDS(1));
	zassert_ok(ret, "send not stalled");

	sys_put_be32(HTTP2_ERROR_CANCEL, error);
	ret = peer_send_frame(conn, HTTP2_RST_STREAM_FRAME, 0, 1, error, sizeof(error));
	zassert_ok(ret, "send RST_STREAM failed (%d)", ret);
	k_sleep(K_MSEC(20));
	http_client_received_data(&client, 0);

	ret = join_request_thread();
	zassert_equal(ret, -ECONNRESET, "request returned %d", ret);
	assert_aborted(&rsp, -ECONNRESET);
	zassert_equal(rsp.abort_count, 1, "abort reported %d times", rsp.abort_count);
}

ZTEST(http2_client_peer, test_peer_refused_disconnect_with_request_in_flight)
{
	static struct test_response refused_rsp;
	static uint8_t refused_buf[64];
	struct http_client_request refused = { 0 };
	int ret;

	peer_connect(false);

	memset(&req, 0, sizeof(req));
	req.method = HTTP_GET;
	req.url = "/";
	start_request_thread(TEST_REQUEST_TIMEOUT_MS);
	expect_request(1);

	/* A second request is refused while the first is in flight, and its
	 * callback ends the session: the disconnect waits for the first
	 * request to leave, so it must not run under the client lock.
	 */
	test_response_init(&refused_rsp, refused_buf, sizeof(refused_buf));
	refused.method = HTTP_GET;
	refused.url = "/again";
	refused.response_cb = disconnecting_response_cb;
	ret = http_client_send_request(&client, &refused, TEST_REQUEST_TIMEOUT_MS, &refused_rsp);
	zassert_equal(ret, -EBUSY, "second request accepted (%d)", ret);
	assert_aborted(&refused_rsp, -EBUSY);

	ret = join_request_thread();
	zassert_equal(ret, -ECONNABORTED, "first request returned %d", ret);
	assert_aborted(&rsp, -ECONNABORTED);
	zassert_equal(client.session.state, HTTP_CLIENT_SESSION_CLOSED, "session state %d",
		      client.session.state);
}

ZTEST(http2_client_peer, test_peer_rejected_disconnect_from_callback)
{
	int ret;

	peer_connect(false);

	/* A request refused on its way out is reported once, even when the
	 * session is ended from that callback.
	 */
	memset(&req, 0, sizeof(req));
	req.method = HTTP_GET;
	req.url = "/a b";
	req.response_cb = disconnecting_response_cb;
	ret = http_client_send_request(&client, &req, TEST_REQUEST_TIMEOUT_MS, &rsp);
	zassert_equal(ret, -EINVAL, "space in the URL accepted (%d)", ret);
	zassert_equal(rsp.abort_count, 1, "abort reported %d times", rsp.abort_count);
	zassert_equal(rsp.error, -EINVAL, "abort reason %d", rsp.error);
	zassert_equal(client.session.state, HTTP_CLIENT_SESSION_CLOSED, "session state %d",
		      client.session.state);
}

ZTEST_SUITE(http2_client_peer, NULL, peer_setup, peer_before, peer_after, peer_teardown);
