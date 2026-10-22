/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <string.h>

#include <zephyr/net/socket.h>
#include <zephyr/net/sntp.h>
#include <zephyr/net/sntp_server.h>
#include <zephyr/sys/clock.h>
#include <zephyr/ztest.h>

#include "sntp_pkt.h"

#define SERVER_IPV4_ADDR "127.0.0.1"
#define SERVER_IPV6_ADDR "::1"

/* The server has to answer well within this, it only touches the clock */
#define REPLY_TIMEOUT_MS 500

/* An arbitrary time to set the clock to, 2025-01-01 00:00:00 UTC */
#define TEST_TIME_S 1735689600

static const uint8_t test_ref_id[4] = {'T', 'E', 'S', 'T'};

#define TEST_STRATUM   3
#define TEST_PRECISION (-7)

static int client_socket(net_sa_family_t family, struct net_sockaddr_storage *server,
			 net_socklen_t *server_len)
{
	int sock;

	memset(server, 0, sizeof(*server));

	if (family == NET_AF_INET) {
		struct net_sockaddr_in *addr4 = net_sin(net_sad(server));

		addr4->sin_family = NET_AF_INET;
		addr4->sin_port = net_htons(SNTP_PORT);
		zassert_equal(zsock_inet_pton(NET_AF_INET, SERVER_IPV4_ADDR, &addr4->sin_addr), 1,
			      "Cannot convert " SERVER_IPV4_ADDR);
		*server_len = sizeof(*addr4);
	} else {
		struct net_sockaddr_in6 *addr6 = net_sin6(net_sad(server));

		addr6->sin6_family = NET_AF_INET6;
		addr6->sin6_port = net_htons(SNTP_PORT);
		zassert_equal(zsock_inet_pton(NET_AF_INET6, SERVER_IPV6_ADDR, &addr6->sin6_addr), 1,
			      "Cannot convert " SERVER_IPV6_ADDR);
		*server_len = sizeof(*addr6);
	}

	sock = zsock_socket(family, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	zassert_true(sock >= 0, "Cannot create client socket (%d)", -errno);

	return sock;
}

/* Send the given datagram to the server and return the reply length, or -1 if
 * the server stayed silent.
 */
static int query(net_sa_family_t family, const void *request, size_t request_len,
		 struct sntp_pkt *reply)
{
	struct net_sockaddr_storage server;
	net_socklen_t server_len;
	struct zsock_pollfd fds[1];
	int sock, ret;

	sock = client_socket(family, &server, &server_len);

	ret = zsock_sendto(sock, request, request_len, 0, net_sad(&server), server_len);
	zassert_equal(ret, (int)request_len, "Cannot send request (%d)", -errno);

	fds[0].fd = sock;
	fds[0].events = ZSOCK_POLLIN;

	ret = zsock_poll(fds, 1, REPLY_TIMEOUT_MS);
	if (ret <= 0) {
		(void)zsock_close(sock);
		return -1;
	}

	ret = zsock_recv(sock, reply, sizeof(*reply), 0);
	zassert_true(ret >= 0, "Cannot receive reply (%d)", -errno);

	(void)zsock_close(sock);

	return ret;
}

static struct sntp_pkt make_request(void)
{
	struct sntp_pkt request;

	memset(&request, 0, sizeof(request));
	request.li = SNTP_LEAP_INDICATOR_NONE;
	request.vn = SNTP_VERSION_NUMBER;
	request.mode = SNTP_MODE_CLIENT;
	request.tx_tm_s = net_htonl(0x11223344);
	request.tx_tm_f = net_htonl(0x55667788);

	return request;
}

/* The unsynchronized checks have to run before the clock source is set, so
 * both halves of the transition live in one test.
 */
ZTEST(sntp_server, test_01_unsynchronized_then_synchronized)
{
	struct sntp_pkt request = make_request();
	struct sntp_pkt reply;
	struct timespec now = {.tv_sec = TEST_TIME_S, .tv_nsec = 0};
	uint32_t tx_tm_s;
	int ret;

	ret = query(NET_AF_INET, &request, sizeof(request), &reply);
	zassert_equal(ret, sizeof(reply), "No reply from the server");

	zexpect_equal(reply.mode, SNTP_MODE_SERVER, "Reply is not in server mode (%u)", reply.mode);
	zexpect_equal(reply.li, SNTP_LEAP_INDICATOR_CLOCK_INVALID,
		      "Unsynchronized server did not set the leap indicator (%u)", reply.li);
	zexpect_equal(reply.stratum, SNTP_STRATUM_UNSYNC,
		      "Unsynchronized server did not report stratum %u (%u)", SNTP_STRATUM_UNSYNC,
		      reply.stratum);

	zassert_ok(sys_clock_settime(SYS_CLOCK_REALTIME, &now), "Cannot set the system clock");
	sntp_server_clock_source(test_ref_id, TEST_STRATUM, TEST_PRECISION);

	ret = query(NET_AF_INET, &request, sizeof(request), &reply);
	zassert_equal(ret, sizeof(reply), "No reply from the server");

	zexpect_equal(reply.li, SNTP_LEAP_INDICATOR_NONE,
		      "Synchronized server still reports an invalid clock (%u)", reply.li);
	zexpect_equal(reply.stratum, TEST_STRATUM, "Wrong stratum (%u)", reply.stratum);
	zexpect_equal(reply.precision, TEST_PRECISION, "Wrong precision (%d)", reply.precision);
	zexpect_mem_equal(&reply.ref_id, test_ref_id, sizeof(test_ref_id), "Wrong reference id");

	tx_tm_s = net_ntohl(reply.tx_tm_s);
	zexpect_within(tx_tm_s, (uint32_t)(TEST_TIME_S + OFFSET_1970_JAN_1), 2U,
		       "Transmit timestamp %u is not the time we set", tx_tm_s);
	zexpect_equal(reply.ref_tm_s, reply.tx_tm_s,
		      "Reference timestamp was not taken when the clock source was set");
}

ZTEST(sntp_server, test_02_request_fields_are_echoed)
{
	struct sntp_pkt request = make_request();
	struct sntp_pkt reply;
	int ret;

	request.vn = 4;
	request.poll = 6;

	ret = query(NET_AF_INET, &request, sizeof(request), &reply);
	zassert_equal(ret, sizeof(reply), "No reply from the server");

	zexpect_equal(reply.vn, request.vn, "Version was not echoed (%u)", reply.vn);
	zexpect_equal(reply.poll, request.poll, "Poll interval was not echoed (%u)", reply.poll);
	zexpect_equal(reply.orig_tm_s, request.tx_tm_s, "Originate timestamp seconds not echoed");
	zexpect_equal(reply.orig_tm_f, request.tx_tm_f, "Originate timestamp fraction not echoed");
}

ZTEST(sntp_server, test_03_both_address_families_are_served)
{
	struct sntp_pkt request = make_request();
	struct sntp_pkt reply;

	zexpect_equal(query(NET_AF_INET, &request, sizeof(request), &reply), sizeof(reply),
		      "No reply on IPv4");

	memset(&reply, 0, sizeof(reply));

	zexpect_equal(query(NET_AF_INET6, &request, sizeof(request), &reply), sizeof(reply),
		      "No reply on IPv6");
}

/* Replying to anything but a client request would let two servers talk to each
 * other forever.
 */
ZTEST(sntp_server, test_04_non_client_modes_are_ignored)
{
	struct sntp_pkt request = make_request();
	struct sntp_pkt reply;

	request.mode = SNTP_MODE_SERVER;
	zexpect_equal(query(NET_AF_INET, &request, sizeof(request), &reply), -1,
		      "Server replied to a server mode datagram");

	request.mode = 6; /* NTP control */
	zexpect_equal(query(NET_AF_INET, &request, sizeof(request), &reply), -1,
		      "Server replied to an NTP control datagram");

	request.mode = SNTP_MODE_CLIENT;
	request.vn = 0;
	zexpect_equal(query(NET_AF_INET, &request, sizeof(request), &reply), -1,
		      "Server replied to a version 0 datagram");
}

ZTEST(sntp_server, test_05_short_datagram_is_ignored)
{
	struct sntp_pkt request = make_request();
	struct sntp_pkt reply;

	zexpect_equal(query(NET_AF_INET, &request, sizeof(request) - 1, &reply), -1,
		      "Server replied to a truncated request");
}

#if defined(CONFIG_SNTP)
/* Cross check the replies against the in-tree SNTP client, which is what most
 * users will point at this server.
 */
ZTEST(sntp_server, test_06_zephyr_sntp_client_accepts_our_reply)
{
	struct timespec now = {.tv_sec = TEST_TIME_S, .tv_nsec = 0};
	struct net_sockaddr_storage server;
	net_socklen_t server_len;
	struct sntp_time s_time;
	struct sntp_ctx ctx;
	int sock;

	zassert_ok(sys_clock_settime(SYS_CLOCK_REALTIME, &now), "Cannot set the system clock");
	sntp_server_clock_source(test_ref_id, TEST_STRATUM, TEST_PRECISION);

	/* Reuse the address setup, the client opens its own socket */
	sock = client_socket(NET_AF_INET, &server, &server_len);
	(void)zsock_close(sock);

	zassert_ok(sntp_init(&ctx, net_sad(&server), server_len), "Cannot init the SNTP client");

	zassert_ok(sntp_query(&ctx, REPLY_TIMEOUT_MS, &s_time), "SNTP client query failed");
	zexpect_within((uint32_t)s_time.seconds, (uint32_t)TEST_TIME_S, 2U,
		       "SNTP client got %llu instead of the time we set", s_time.seconds);

	sntp_close(&ctx);
}
#endif /* CONFIG_SNTP */

ZTEST_SUITE(sntp_server, NULL, NULL, NULL, NULL, NULL);
