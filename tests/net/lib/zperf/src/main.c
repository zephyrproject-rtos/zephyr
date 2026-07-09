/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Self-test for the zperf network performance library.
 */

#include <inttypes.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/zperf.h>

#define LOOPBACK_IPV4_ADDR "127.0.0.1"
#define TEST_PORT          5001
#define TEST_DURATION_MS   1000
#define TEST_PACKET_SIZE   256
#define TEST_RATE_KBPS     1000

/* Upper bound for the server callback to fire after the (already completed)
 * upload. The server processes the stream almost immediately, so this only
 * needs to be a safety margin.
 */
#define TEST_TIMEOUT       K_SECONDS(2)

static K_SEM_DEFINE(session_finished, 0, 1);

static struct zperf_results server_results;
static enum zperf_status server_last_status = ZPERF_SESSION_ERROR;

static void server_session_cb(enum zperf_status status,
			      struct zperf_results *result,
			      void *user_data)
{
	ARG_UNUSED(user_data);

	switch (status) {
	case ZPERF_SESSION_STARTED:
		break;
	case ZPERF_SESSION_FINISHED:
		if (result == NULL) {
			/* A FINISHED event must carry results; a missing result
			 * is treated as an error so the failure is explicit.
			 */
			server_last_status = ZPERF_SESSION_ERROR;
		} else {
			memcpy(&server_results, result, sizeof(server_results));
			server_last_status = status;
		}
		k_sem_give(&session_finished);
		break;
	case ZPERF_SESSION_ERROR:
		server_last_status = status;
		k_sem_give(&session_finished);
		break;
	default:
		break;
	}
}

static void fill_upload_params(struct zperf_upload_params *param)
{
	struct net_sockaddr_in peer = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(TEST_PORT),
	};

	zassert_equal(net_addr_pton(NET_AF_INET, LOOPBACK_IPV4_ADDR,
				    &peer.sin_addr), 0,
		      "Failed to parse loopback address");

	memset(param, 0, sizeof(*param));
	memcpy(&param->peer_addr, &peer, sizeof(peer));
	param->duration_ms = TEST_DURATION_MS;
	param->packet_size = TEST_PACKET_SIZE;
	param->rate_kbps = TEST_RATE_KBPS;
	param->unix_offset_us = (uint64_t)k_uptime_get() * USEC_PER_MSEC;
	param->options.priority = -1;
}

static void zperf_before(void *fixture)
{
	ARG_UNUSED(fixture);

	k_sem_reset(&session_finished);
	memset(&server_results, 0, sizeof(server_results));
	server_last_status = ZPERF_SESSION_ERROR;
}

static void zperf_after(void *fixture)
{
	ARG_UNUSED(fixture);

	/* Only one server runs per test; stopping the idle one returns
	 * -EALREADY, which is harmless here. This makes sure the server is
	 * always torn down, even if the test aborted early.
	 */
	(void)zperf_udp_download_stop();
	(void)zperf_tcp_download_stop();
}

ZTEST(zperf_api, test_udp_upload_download)
{
	struct zperf_download_params download_param = {
		.port = TEST_PORT,
	};
	struct zperf_upload_params upload_param;
	struct zperf_results client_results = { 0 };
	int ret;

	ret = zperf_udp_download(&download_param, server_session_cb, NULL);
	zassert_ok(ret, "Failed to start UDP server (%d)", ret);

	fill_upload_params(&upload_param);

	/* Blocking upload for TEST_DURATION_MS. */
	ret = zperf_udp_upload(&upload_param, &client_results);
	zassert_ok(ret, "UDP upload failed (%d)", ret);

	/* Client side: we sent packets and collected the server statistics. */
	zassert_true(client_results.nb_packets_sent > 0,
		     "Client did not send any UDP packets");
	zassert_equal(client_results.packet_size, TEST_PACKET_SIZE,
		      "Unexpected client packet size %u",
		      client_results.packet_size);
	zassert_true(client_results.client_time_in_us > 0,
		     "Client transfer time not reported");
	zassert_true(client_results.nb_packets_rcvd > 0,
		     "Server did not report any received packets to client");
	zassert_true(client_results.nb_packets_rcvd <=
			     client_results.nb_packets_sent,
		     "Server received more packets (%u) than were sent (%u)",
		     client_results.nb_packets_rcvd,
		     client_results.nb_packets_sent);

	/* Server side: wait for the FINISHED callback and verify its stats. */
	ret = k_sem_take(&session_finished, TEST_TIMEOUT);
	zassert_ok(ret, "Timed out waiting for UDP server session to finish");
	zassert_equal(server_last_status, ZPERF_SESSION_FINISHED,
		      "UDP server session did not finish cleanly (status %d)",
		      server_last_status);
	zassert_true(server_results.nb_packets_rcvd > 0,
		     "UDP server did not receive any packets");
	zassert_true(server_results.total_len > 0,
		     "UDP server did not report any received data");
	zassert_true(server_results.time_in_us > 0,
		     "UDP server did not report a transfer time");
}

ZTEST(zperf_api, test_tcp_upload_download)
{
	struct zperf_download_params download_param = {
		.port = TEST_PORT,
	};
	struct zperf_upload_params upload_param;
	struct zperf_results client_results = { 0 };
	int ret;

	ret = zperf_tcp_download(&download_param, server_session_cb, NULL);
	zassert_ok(ret, "Failed to start TCP server (%d)", ret);

	fill_upload_params(&upload_param);

	/* Blocking upload for TEST_DURATION_MS. */
	ret = zperf_tcp_upload(&upload_param, &client_results);
	zassert_ok(ret, "TCP upload failed (%d)", ret);

	/* Client side stats. */
	zassert_true(client_results.nb_packets_sent > 0,
		     "Client did not send any TCP packets");
	zassert_true(client_results.total_len > 0,
		     "Client did not transfer any TCP data");
	zassert_equal(client_results.packet_size, TEST_PACKET_SIZE,
		      "Unexpected client packet size %u",
		      client_results.packet_size);
	zassert_true(client_results.client_time_in_us > 0,
		     "Client transfer time not reported");

	/* Server side: the FINISHED callback fires when the client closes. */
	ret = k_sem_take(&session_finished, TEST_TIMEOUT);
	zassert_ok(ret, "Timed out waiting for TCP server session to finish");
	zassert_equal(server_last_status, ZPERF_SESSION_FINISHED,
		      "TCP server session did not finish cleanly (status %d)",
		      server_last_status);
	zassert_true(server_results.total_len > 0,
		     "TCP server did not report any received data");

	/* TCP is reliable, so the server must have received exactly what the
	 * client sent.
	 */
	zassert_equal(server_results.total_len, client_results.total_len,
		      "TCP server received %" PRIu64 " bytes, client sent %"
		      PRIu64, server_results.total_len,
		      client_results.total_len);
}

ZTEST_SUITE(zperf_api, NULL, NULL, zperf_before, zperf_after, NULL);
