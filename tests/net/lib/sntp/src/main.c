/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Dev It Wise
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Tests for what the SNTP client accepts and rejects in a server's response.
 *
 * parse_response() is static with no test-only export, so it is reached
 * the way a client reaches it: bind to the SNTP port on loopback, let
 * the client run a real sntp_init()/sntp_send_async(), read its request
 * to learn the ephemeral port, and answer with a built response.
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/sntp.h>
#include <zephyr/ztest.h>

#include "sntp_pkt.h"

#define SNTP_PORT 123

#define MODE_CLIENT 3
#define MODE_SERVER 4
#define VERSION     4

/* Seconds between the NTP epoch and 1 Jan 1970, which parse_response()
 * subtracts when the transmit timestamp's top bit is set.
 */
#define OFFSET_1970_JAN_1 2208988800U

/* The loopback datagram is delivered inside zsock_sendto(), so it is
 * already waiting by the time the client polls for it.
 */
#define RECV_TIMEOUT_MS 100

static int server_fd = -1;

/* A well-formed response; a test overrides only the field it exercises.
 * The originate timestamp is left zero, filled in by exchange().
 */
static struct sntp_pkt response_template(void)
{
	struct sntp_pkt pkt = {0};

	pkt.li = 0;
	pkt.vn = VERSION;
	pkt.mode = MODE_SERVER;
	pkt.stratum = 2;
	pkt.poll = 4;
	pkt.precision = -6;
	pkt.rx_tm_s = net_htonl(OFFSET_1970_JAN_1);
	pkt.tx_tm_s = net_htonl(OFFSET_1970_JAN_1 + 1);

	return pkt;
}

/* Run one query/response exchange. echo_originate false leaves the
 * originate timestamp as the caller set it, to test the replay check.
 */
static int exchange(struct sntp_pkt *response, bool echo_originate, struct sntp_time *ts)
{
	struct net_sockaddr_in server = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(SNTP_PORT),
		.sin_addr = {{{127, 0, 0, 1}}},
	};
	struct net_sockaddr_in client;
	net_socklen_t client_len = sizeof(client);
	struct sntp_pkt request;
	struct sntp_ctx ctx;
	int ret;

	ret = sntp_init(&ctx, (struct net_sockaddr *)&server, sizeof(server));
	zassert_ok(ret, "sntp_init failed: %d", ret);

	ret = sntp_send_async(&ctx);
	zassert_ok(ret, "sntp_send_async failed: %d", ret);

	ret = zsock_recvfrom(server_fd, &request, sizeof(request), 0,
			     (struct net_sockaddr *)&client, &client_len);
	zassert_equal(ret, (int)sizeof(request), "no request from the client: %d", ret);

	if (echo_originate) {
		response->orig_tm_s = net_htonl((uint32_t)ctx.expected_orig_ts.seconds);
		response->orig_tm_f = net_htonl(ctx.expected_orig_ts.fraction);
	}

	ret = zsock_sendto(server_fd, response, sizeof(*response), 0,
			   (struct net_sockaddr *)&client, client_len);
	zassert_equal(ret, (int)sizeof(*response), "could not send the response: %d", ret);

	ret = sntp_recv_response(&ctx, RECV_TIMEOUT_MS, ts);

	sntp_close(&ctx);

	return ret;
}

static int query_with_precision(int8_t precision)
{
	struct sntp_pkt response = response_template();
	struct sntp_time ts;

	response.precision = precision;

	return exchange(&response, true, &ts);
}

/* precision is a shift count on a 32-bit value; checks both range
 * boundaries, one step to either side.
 */
ZTEST(sntp_response, test_precision_range)
{
	zassert_ok(query_with_precision(0), "precision 0 should be accepted");
	zassert_ok(query_with_precision(-6), "precision -6 should be accepted");
	zassert_ok(query_with_precision(-31), "precision -31 should be accepted");
	zassert_ok(query_with_precision(10), "precision 10 should be accepted");

	zassert_equal(query_with_precision(-32), -EINVAL,
		      "precision -32 shifts by the width of the type");
	zassert_equal(query_with_precision(INT8_MIN), -EINVAL,
		      "precision -128 shifts past the width of the type");
	zassert_equal(query_with_precision(11), -EINVAL,
		      "precision 11 is above the range the client accepts");
	zassert_equal(query_with_precision(INT8_MAX), -EINVAL,
		      "precision 127 is above the range the client accepts");
}

ZTEST(sntp_response, test_well_formed_response_is_accepted)
{
	struct sntp_pkt response = response_template();
	/* A sentinel rather than a zero-initializer: uptime 0 is a valid
	 * reading, so only "the field changed" proves it was populated.
	 */
	struct sntp_time ts = {.uptime_us = UINT64_MAX};

	zassert_ok(exchange(&response, true, &ts), "a valid response was rejected");
	zassert_not_equal(ts.uptime_us, UINT64_MAX, "the uptime timestamp was not filled in");
}

/* A stratum of zero is a kiss-o'-death packet: the server is telling
 * the client to stop asking, not reporting a time.
 */
ZTEST(sntp_response, test_kiss_o_death_is_rejected)
{
	struct sntp_pkt response = response_template();
	struct sntp_time ts;

	response.stratum = 0;

	zassert_equal(exchange(&response, true, &ts), -EBUSY, "kiss-o'-death was not reported");
}

ZTEST(sntp_response, test_non_server_mode_is_rejected)
{
	struct sntp_pkt response = response_template();
	struct sntp_time ts;

	response.mode = MODE_CLIENT;

	zassert_equal(exchange(&response, true, &ts), -EINVAL,
		      "a response in client mode was accepted");
}

ZTEST(sntp_response, test_zero_transmit_timestamp_is_rejected)
{
	struct sntp_pkt response = response_template();
	struct sntp_time ts;

	response.tx_tm_s = 0;
	response.tx_tm_f = 0;

	zassert_equal(exchange(&response, true, &ts), -EINVAL,
		      "a response with no transmit timestamp was accepted");
}

/* The originate timestamp is the client's own transmit timestamp echoed
 * back, and a response that does not carry it belongs to some other
 * exchange.
 */
ZTEST(sntp_response, test_originate_timestamp_must_match)
{
	struct sntp_pkt response = response_template();
	struct sntp_time ts;

	response.orig_tm_s = net_htonl(1);
	response.orig_tm_f = net_htonl(1);

	zassert_equal(exchange(&response, false, &ts), -ERANGE,
		      "a response for another exchange was accepted");
}

/* The server claims to have sent its answer before the request reached
 * it, which makes the round-trip computation meaningless.
 */
ZTEST(sntp_response, test_reversed_server_timestamps_are_rejected)
{
	struct sntp_pkt response = response_template();
	struct sntp_time ts;

	response.rx_tm_s = net_htonl(OFFSET_1970_JAN_1 + 100);
	response.tx_tm_s = net_htonl(OFFSET_1970_JAN_1 + 1);

	zassert_equal(exchange(&response, true, &ts), -EINVAL,
		      "reversed server timestamps were accepted");
}

static void *setup(void)
{
	struct net_sockaddr_in addr = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(SNTP_PORT),
		.sin_addr = {{{127, 0, 0, 1}}},
	};
	/* Bounds the server's wait for the client's request, so a client
	 * that fails to send fails the test instead of hanging the run.
	 */
	struct timeval timeout = {.tv_sec = 1, .tv_usec = 0};

	server_fd = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	zassert_true(server_fd >= 0, "could not open the server socket");

	zassert_ok(zsock_bind(server_fd, (struct net_sockaddr *)&addr, sizeof(addr)),
		   "could not bind the server socket");

	zassert_ok(zsock_setsockopt(server_fd, ZSOCK_SOL_SOCKET, ZSOCK_SO_RCVTIMEO, &timeout,
				     sizeof(timeout)),
		   "could not set the server socket's receive timeout");

	return NULL;
}

static void teardown(void *fixture)
{
	ARG_UNUSED(fixture);

	(void)zsock_close(server_fd);
	server_fd = -1;
}

ZTEST_SUITE(sntp_response, NULL, setup, NULL, NULL, teardown);
