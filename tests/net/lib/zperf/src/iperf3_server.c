/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/* The iperf3 server, driven by a scripted client that speaks the protocol
 * over plain sockets. This reaches what the zperf client never sends: tests
 * the server must refuse, a foreign data connection, a client that goes away,
 * and hand-built UDP sequences.
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/zperf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#include "zperf_iperf3.h"

#define PORT          5201
#define LOOPBACK_ADDR "127.0.0.1"

/* Replies from the server are immediate; this only bounds a failure */
#define REPLY_TIMEOUT_MS 2000

static const char cookie[ZPERF_IPERF3_COOKIE_SIZE] = "abcdefghijklmnopqrstuvwxyz2345672345";
static const char other_cookie[ZPERF_IPERF3_COOKIE_SIZE] = "zyxwvutsrqponmlkjihgfedcba7654327654";

static K_SEM_DEFINE(started, 0, 1);
static K_SEM_DEFINE(finished, 0, 1);
static K_SEM_DEFINE(failed, 0, 1);
static K_SEM_DEFINE(upload_done, 0, 1);

static struct zperf_results server_results;
static enum zperf_status upload_status;

/* Sockets a test has open, so that one failing half way through does not
 * leave them to exhaust the ones after it. Tests close theirs through
 * close_sock(), never directly, so that no descriptor is closed twice.
 */
static int open_socks[8];
static size_t open_socks_count;

static int track_sock(int sock)
{
	zassert_true(open_socks_count < ARRAY_SIZE(open_socks));
	open_socks[open_socks_count++] = sock;

	return sock;
}

static void close_sock(int sock)
{
	for (size_t i = 0; i < open_socks_count; i++) {
		if (open_socks[i] == sock) {
			open_socks[i] = open_socks[--open_socks_count];
			zsock_close(sock);
			return;
		}
	}
}

static void server_cb(enum zperf_status status, struct zperf_results *result, void *user_data)
{
	ARG_UNUSED(user_data);

	switch (status) {
	case ZPERF_SESSION_STARTED:
		k_sem_give(&started);
		break;
	case ZPERF_SESSION_FINISHED:
		server_results = *result;
		k_sem_give(&finished);
		break;
	case ZPERF_SESSION_ERROR:
		k_sem_give(&failed);
		break;
	default:
		break;
	}
}

static void upload_cb(enum zperf_status status, struct zperf_results *result, void *user_data)
{
	ARG_UNUSED(result);
	ARG_UNUSED(user_data);

	if (status == ZPERF_SESSION_FINISHED || status == ZPERF_SESSION_ERROR) {
		upload_status = status;
		k_sem_give(&upload_done);
	}
}

static void peer_addr(struct net_sockaddr_in *addr, uint16_t port)
{
	memset(addr, 0, sizeof(*addr));
	addr->sin_family = NET_AF_INET;
	addr->sin_port = net_htons(port);
	zassert_ok(net_addr_pton(NET_AF_INET, LOOPBACK_ADDR, &addr->sin_addr));
}

static int client_socket(int type, int proto, uint32_t timeout_ms)
{
	struct zsock_timeval tv = {
		.tv_sec = timeout_ms / MSEC_PER_SEC,
		.tv_usec = (timeout_ms % MSEC_PER_SEC) * USEC_PER_MSEC,
	};
	struct net_sockaddr_in addr;
	int sock;

	sock = zsock_socket(NET_AF_INET, type, proto);
	zassert_true(sock >= 0, "socket failed (%d)", errno);
	track_sock(sock);
	zassert_ok(zsock_setsockopt(sock, ZSOCK_SOL_SOCKET, ZSOCK_SO_RCVTIMEO, &tv, sizeof(tv)));

	peer_addr(&addr, PORT);
	zassert_ok(zsock_connect(sock, (struct net_sockaddr *)&addr, sizeof(addr)),
		   "connect failed (%d)", errno);

	return sock;
}

static int tcp_connect(void)
{
	return client_socket(NET_SOCK_STREAM, NET_IPPROTO_TCP, REPLY_TIMEOUT_MS);
}

static void send_bytes(int sock, const void *buf, size_t len)
{
	zassert_ok(zperf_iperf3_send_all(sock, buf, len, 0));
}

static int8_t recv_state(int sock)
{
	int8_t state;

	zassert_equal(zsock_recv(sock, &state, sizeof(state), 0), sizeof(state),
		      "no state from the server (%d)", errno);

	return state;
}

static void expect_state(int sock, int8_t expected)
{
	int8_t state = recv_state(sock);

	zassert_equal(state, expected, "state %d, expected %d", state, expected);
}

static void expect_closed(int sock)
{
	uint8_t byte;

	zassert_equal(zsock_recv(sock, &byte, sizeof(byte), 0), 0, "connection still open");
}

static void send_state(int sock, int8_t state)
{
	send_bytes(sock, &state, sizeof(state));
}

static void send_json(int sock, const char *json)
{
	uint8_t len[ZPERF_IPERF3_JSON_LEN_SIZE];

	sys_put_be32(strlen(json), len);
	send_bytes(sock, len, sizeof(len));
	send_bytes(sock, json, strlen(json));
}

/* Start a test up to the point where the server wants a data stream */
static int start_control(const char *params)
{
	int ctrl = tcp_connect();

	send_bytes(ctrl, cookie, sizeof(cookie));
	expect_state(ctrl, ZPERF_IPERF3_PARAM_EXCHANGE);
	send_json(ctrl, params);

	return ctrl;
}

static void expect_refusal(const char *params, int32_t error)
{
	uint8_t reply[ZPERF_IPERF3_SERVER_ERROR_SIZE - 1];
	int ctrl = start_control(params);

	expect_state(ctrl, ZPERF_IPERF3_SERVER_ERROR);
	zassert_ok(zperf_iperf3_recv_all(ctrl, reply, sizeof(reply)));
	zassert_equal((int32_t)sys_get_be32(reply), error, "refused with %d for %s",
		      (int32_t)sys_get_be32(reply), params);
	expect_closed(ctrl);
	close_sock(ctrl);
}

/* After TEST_END: give the server our results, and return its own */
static void finish_test(int ctrl, struct zperf_iperf3_stream_stats *server)
{
	static uint8_t buf[512];
	struct zperf_iperf3_stream_stats ours = { .duration_us = 1 };
	uint32_t len;
	int ret;

	send_state(ctrl, ZPERF_IPERF3_TEST_END);
	expect_state(ctrl, ZPERF_IPERF3_EXCHANGE_RESULTS);

	ret = zperf_iperf3_results_encode(&ours, true, buf, sizeof(buf));
	zassert_true(ret > 0);
	send_bytes(ctrl, buf, ret);

	zassert_ok(zperf_iperf3_recv_all(ctrl, buf, ZPERF_IPERF3_JSON_LEN_SIZE));
	len = sys_get_be32(buf);
	zassert_true(len > 0 && len < sizeof(buf), "results of %u bytes", len);
	zassert_ok(zperf_iperf3_recv_all(ctrl, buf, len));
	buf[len] = '\0';

	/* What an iperf3 client requires of a receiver's results */
	zassert_not_null(strstr((char *)buf, "\"sender_has_retransmits\":-1"), "%s", buf);
	zassert_not_null(strstr((char *)buf, "\"id\":1,"), "%s", buf);
	zassert_not_null(strstr((char *)buf, "\"retransmits\":-1"), "%s", buf);
	zassert_ok(zperf_iperf3_results_decode((char *)buf, len, server));

	expect_state(ctrl, ZPERF_IPERF3_DISPLAY_RESULTS);
	send_state(ctrl, ZPERF_IPERF3_IPERF_DONE);
}

static void fill_upload(struct zperf_upload_params *param)
{
	struct net_sockaddr_in addr;

	peer_addr(&addr, PORT);
	memset(param, 0, sizeof(*param));
	memcpy(&param->peer_addr_storage, &addr, sizeof(addr));
	param->duration_ms = 500;
	param->packet_size = 256;
	param->rate_kbps = 1000;
	param->options.priority = -1;
}

static void download(bool tcp, bool udp)
{
	struct zperf_download_params param = { .port = PORT };

	if (tcp) {
		zassert_ok(zperf_tcp_download(&param, server_cb, NULL));
	}

	if (udp) {
		zassert_ok(zperf_udp_download(&param, server_cb, NULL));
	}
}

ZTEST(zperf_iperf3, test_tcp_and_udp_share_the_listener)
{
	struct zperf_download_params other_port = { .port = PORT + 1 };
	struct zperf_upload_params param;
	struct zperf_results results;

	download(true, false);

	/* One listener, so one port for both */
	zassert_equal(zperf_udp_download(&other_port, server_cb, NULL), -EBUSY);
	download(false, true);
	zassert_equal(zperf_tcp_download(&other_port, server_cb, NULL), -EALREADY);

	fill_upload(&param);
	zassert_ok(zperf_tcp_upload(&param, &results));
	zassert_ok(k_sem_take(&finished, K_SECONDS(2)));
	zassert_ok(zperf_udp_upload(&param, &results));
	zassert_ok(k_sem_take(&finished, K_SECONDS(2)));
}

ZTEST(zperf_iperf3, test_disabled_protocol_is_refused)
{
	struct zperf_upload_params param;
	struct zperf_results results;

	download(true, false);
	fill_upload(&param);

	zassert_equal(zperf_udp_upload(&param, &results), -ENOTSUP);

	/* And the other protocol still works afterwards */
	zassert_ok(zperf_tcp_upload(&param, &results));
}

ZTEST(zperf_iperf3, test_unsupported_tests_are_refused)
{
	static const char *const params[] = {
		"{\"tcp\":true,\"parallel\":2}",
		"{\"tcp\":true,\"reverse\":true}",
		"{\"tcp\":true,\"bidirectional\":true}",
		"{\"tcp\":true,\"omit\":1}",
		"{\"sctp\":true}",
	};

	download(true, true);

	ARRAY_FOR_EACH(params, i) {
		expect_refusal(params[i], ZPERF_IPERF3_IEUNIMP);
	}

	/* Messages that are not parameters at all */
	expect_refusal("not json", ZPERF_IPERF3_IERECVPARAMS);
}

ZTEST(zperf_iperf3, test_oversized_message_is_refused)
{
	uint8_t reply[ZPERF_IPERF3_SERVER_ERROR_SIZE - 1];
	uint8_t len[ZPERF_IPERF3_JSON_LEN_SIZE];
	int ctrl;

	download(true, false);

	ctrl = tcp_connect();
	send_bytes(ctrl, cookie, sizeof(cookie));
	expect_state(ctrl, ZPERF_IPERF3_PARAM_EXCHANGE);

	sys_put_be32(0xffffff, len);
	send_bytes(ctrl, len, sizeof(len));

	expect_state(ctrl, ZPERF_IPERF3_SERVER_ERROR);
	zassert_ok(zperf_iperf3_recv_all(ctrl, reply, sizeof(reply)));
	zassert_equal(sys_get_be32(reply), ZPERF_IPERF3_IERECVPARAMS);
	close_sock(ctrl);
}

ZTEST(zperf_iperf3, test_tcp_test_by_hand)
{
	struct zperf_iperf3_stream_stats server;
	static uint8_t payload[1000];
	int ctrl;
	int data;

	download(true, false);

	ctrl = start_control("{\"tcp\":true,\"time\":1,\"parallel\":1,\"len\":1000}");
	expect_state(ctrl, ZPERF_IPERF3_CREATE_STREAMS);

	/* A data connection for some other test is turned away */
	data = tcp_connect();
	send_bytes(data, other_cookie, sizeof(other_cookie));
	expect_state(data, ZPERF_IPERF3_ACCESS_DENIED);
	close_sock(data);

	/* The right one starts the test. Its payload arrives in the same
	 * write as the cookie, and must not be lost.
	 */
	data = tcp_connect();
	memcpy(payload, cookie, sizeof(cookie));
	send_bytes(data, payload, sizeof(payload));
	expect_state(ctrl, ZPERF_IPERF3_TEST_START);
	expect_state(ctrl, ZPERF_IPERF3_TEST_RUNNING);
	zassert_ok(k_sem_take(&started, K_SECONDS(1)));

	send_bytes(data, payload, sizeof(payload));
	send_bytes(data, payload, sizeof(payload));

	/* Let some time pass, which native_sim only does while waiting */
	k_msleep(10);
	finish_test(ctrl, &server);

	/* Everything after the cookie */
	zassert_equal(server.bytes, 3 * sizeof(payload) - sizeof(cookie), "%llu bytes",
		      server.bytes);
	zassert_true(server.duration_us > 0);

	zassert_ok(k_sem_take(&finished, K_SECONDS(1)));
	zassert_equal(server_results.total_len, server.bytes);

	close_sock(data);
	close_sock(ctrl);
}

static void udp_datagram(int sock, uint64_t seq)
{
	uint8_t buf[100] = { 0 };

	zperf_iperf3_udp_hdr_put(buf, 1000000 + seq * 1000, seq, true);
	zassert_equal(zsock_send(sock, buf, sizeof(buf), 0), sizeof(buf));
}

ZTEST(zperf_iperf3, test_udp_test_by_hand)
{
	static const uint64_t seqs[] = { 1, 2, 4, 3, 6 };
	struct zperf_iperf3_stream_stats server;
	uint8_t reply[ZPERF_IPERF3_UDP_CONNECT_SIZE];
	int ctrl;
	int data;

	download(false, true);

	ctrl = start_control("{\"udp\":true,\"time\":1,\"parallel\":1,\"len\":100,"
			     "\"udp_counters_64bit\":1}");
	expect_state(ctrl, ZPERF_IPERF3_CREATE_STREAMS);

	data = client_socket(NET_SOCK_DGRAM, NET_IPPROTO_UDP, REPLY_TIMEOUT_MS);
	zassert_equal(zsock_send(data, zperf_iperf3_udp_connect_msg,
				 ZPERF_IPERF3_UDP_CONNECT_SIZE, 0),
		      ZPERF_IPERF3_UDP_CONNECT_SIZE);
	zassert_equal(zsock_recv(data, reply, sizeof(reply), 0), sizeof(reply));
	zassert_mem_equal(reply, zperf_iperf3_udp_connect_reply, sizeof(reply));

	expect_state(ctrl, ZPERF_IPERF3_TEST_START);
	expect_state(ctrl, ZPERF_IPERF3_TEST_RUNNING);

	/* A repeated set-up datagram is not data */
	zassert_equal(zsock_send(data, zperf_iperf3_udp_connect_msg,
				 ZPERF_IPERF3_UDP_CONNECT_SIZE, 0),
		      ZPERF_IPERF3_UDP_CONNECT_SIZE);

	ARRAY_FOR_EACH(seqs, i) {
		udp_datagram(data, seqs[i]);
	}

	k_msleep(10);
	finish_test(ctrl, &server);

	/* 4 opens a gap, 3 closes it, 6 opens another */
	zassert_equal(server.packets, 6);
	zassert_equal(server.errors, 1);
	zassert_equal(server.bytes, 500);

	zassert_ok(k_sem_take(&finished, K_SECONDS(1)));
	zassert_equal(server_results.nb_packets_rcvd, 5);
	zassert_equal(server_results.nb_packets_lost, 1);
	zassert_equal(server_results.nb_packets_outorder, 1);
	zassert_equal(server_results.packet_size, 100);

	close_sock(data);
	close_sock(ctrl);
}

ZTEST(zperf_iperf3, test_second_client_is_denied)
{
	struct zperf_upload_params param;
	int ctrl;

	download(true, false);
	fill_upload(&param);
	param.duration_ms = 1000;

	zassert_ok(zperf_tcp_upload_async(&param, upload_cb, NULL));
	zassert_ok(k_sem_take(&started, K_SECONDS(2)));

	ctrl = tcp_connect();
	expect_state(ctrl, ZPERF_IPERF3_ACCESS_DENIED);
	expect_closed(ctrl);
	close_sock(ctrl);

	/* The running test is not disturbed */
	zassert_ok(k_sem_take(&upload_done, K_SECONDS(4)));
	zassert_equal(upload_status, ZPERF_SESSION_FINISHED);
	zassert_ok(k_sem_take(&finished, K_SECONDS(1)));
}

ZTEST(zperf_iperf3, test_client_going_away_frees_the_server)
{
	struct zperf_upload_params param;
	struct zperf_results results;
	int ctrl;
	int data;

	download(true, false);

	ctrl = start_control("{\"tcp\":true,\"time\":1,\"parallel\":1}");
	expect_state(ctrl, ZPERF_IPERF3_CREATE_STREAMS);
	data = tcp_connect();
	send_bytes(data, cookie, sizeof(cookie));
	expect_state(ctrl, ZPERF_IPERF3_TEST_START);
	expect_state(ctrl, ZPERF_IPERF3_TEST_RUNNING);

	close_sock(ctrl);
	zassert_ok(k_sem_take(&failed, K_SECONDS(2)));
	close_sock(data);

	/* The next test is served */
	fill_upload(&param);
	zassert_ok(zperf_tcp_upload(&param, &results));
}

ZTEST(zperf_iperf3, test_stopping_the_server_ends_the_test)
{
	int ctrl;
	int data;

	download(true, false);

	ctrl = start_control("{\"tcp\":true,\"time\":1,\"parallel\":1}");
	expect_state(ctrl, ZPERF_IPERF3_CREATE_STREAMS);
	data = tcp_connect();
	send_bytes(data, cookie, sizeof(cookie));
	expect_state(ctrl, ZPERF_IPERF3_TEST_START);
	expect_state(ctrl, ZPERF_IPERF3_TEST_RUNNING);

	zassert_ok(zperf_tcp_download_stop());

	expect_state(ctrl, ZPERF_IPERF3_SERVER_TERMINATE);
	expect_closed(ctrl);
	zassert_ok(k_sem_take(&failed, K_SECONDS(1)));

	close_sock(data);
	close_sock(ctrl);
}

ZTEST(zperf_iperf3, test_idle_client_is_dropped)
{
	int ctrl;

	download(true, false);

	/* Connected, but never sends its cookie */
	ctrl = client_socket(NET_SOCK_STREAM, NET_IPPROTO_TCP,
			     (CONFIG_NET_ZPERF_IPERF3_IDLE_TIMEOUT + 2) * MSEC_PER_SEC);
	expect_closed(ctrl);
	close_sock(ctrl);
}

ZTEST(zperf_iperf3, test_upload_bound_to_an_interface)
{
	struct zperf_upload_params param;
	struct zperf_results results;

	download(true, true);
	fill_upload(&param);

	/* The control connection and the data stream are both bound to the
	 * interface, before they connect.
	 */
	zassert_true(net_if_get_name(net_if_get_default(), param.if_name,
				     sizeof(param.if_name)) > 0);

	zassert_ok(zperf_tcp_upload(&param, &results));
	zassert_ok(k_sem_take(&finished, K_SECONDS(2)));
	zassert_true(server_results.total_len > 0);

	zassert_ok(zperf_udp_upload(&param, &results));
	zassert_ok(k_sem_take(&finished, K_SECONDS(2)));
	zassert_true(results.nb_packets_rcvd > 0);
}

ZTEST(zperf_iperf3, test_multicast_is_refused)
{
	struct zperf_download_params param = { .port = PORT };
	struct net_sockaddr_in *addr = net_sin(net_sad(&param.addr_storage));
	struct zperf_upload_params upload;
	struct zperf_results results;

	addr->sin_family = NET_AF_INET;
	zassert_ok(net_addr_pton(NET_AF_INET, "224.0.0.1", &addr->sin_addr));
	zassert_equal(zperf_udp_download(&param, server_cb, NULL), -ENOTSUP);

	fill_upload(&upload);
	net_sin(net_sad(&upload.peer_addr_storage))->sin_addr = addr->sin_addr;
	zassert_equal(zperf_udp_upload(&upload, &results), -ENOTSUP);
}

static void iperf3_before(void *fixture)
{
	ARG_UNUSED(fixture);

	k_sem_reset(&started);
	k_sem_reset(&finished);
	k_sem_reset(&failed);
	k_sem_reset(&upload_done);
	memset(&server_results, 0, sizeof(server_results));
}

static void iperf3_after(void *fixture)
{
	ARG_UNUSED(fixture);

	while (open_socks_count > 0) {
		close_sock(open_socks[0]);
	}

	(void)zperf_tcp_download_stop();
	(void)zperf_udp_download_stop();
}

ZTEST_SUITE(zperf_iperf3, NULL, NULL, iperf3_before, iperf3_after, NULL);
