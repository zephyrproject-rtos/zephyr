/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/snmp.h>
#include <zephyr/net/socket.h>
#include <zephyr/ztest.h>

#include "snmp_ber.h"

#define REPLY_TIMEOUT_MS 500

/* GetRequest, version 2c, community "public", request-id 1, for 1.3.6.1.7.1.1.0 */
static const uint8_t get_a[] = {0x30, 0x25, 0x02, 0x01, 0x01, 0x04, 0x06, 0x70, 0x75, 0x62,
				0x6C, 0x69, 0x63, 0xA0, 0x18, 0x02, 0x01, 0x01, 0x02, 0x01,
				0x00, 0x02, 0x01, 0x00, 0x30, 0x0D, 0x30, 0x0B, 0x06, 0x07,
				0x2B, 0x06, 0x01, 0x07, 0x01, 0x01, 0x00, 0x05, 0x00};

/* The same request with community "wrong" */
static const uint8_t get_a_wrong[] = {0x30, 0x24, 0x02, 0x01, 0x01, 0x04, 0x05, 0x77, 0x72, 0x6F,
				      0x6E, 0x67, 0xA0, 0x18, 0x02, 0x01, 0x01, 0x02, 0x01, 0x00,
				      0x02, 0x01, 0x00, 0x30, 0x0D, 0x30, 0x0B, 0x06, 0x07, 0x2B,
				      0x06, 0x01, 0x07, 0x01, 0x01, 0x00, 0x05, 0x00};

/* A second loopback address, to check which one the agent answers from */
#define SECOND_IPV4_ADDR "127.0.0.2"

static const char *server_ipv4 = "127.0.0.1";

/* Address the IPv4 client binds to, or NULL to let the stack choose. On
 * loopback an unbound client is given the destination as its own source,
 * so the reply's destination then selects the same address and a check of
 * the reply source proves nothing; a bound client makes the two differ.
 */
static const char *client_ipv4;

static int client_socket(net_sa_family_t family, struct net_sockaddr_storage *server,
			 net_socklen_t *server_len)
{
	int sock;

	memset(server, 0, sizeof(*server));

	if (family == NET_AF_INET) {
		struct net_sockaddr_in *addr4 = net_sin(net_sad(server));

		addr4->sin_family = NET_AF_INET;
		addr4->sin_port = net_htons(CONFIG_SNMP_PORT);
		zassert_equal(zsock_inet_pton(NET_AF_INET, server_ipv4, &addr4->sin_addr), 1);
		*server_len = sizeof(*addr4);
	} else {
		struct net_sockaddr_in6 *addr6 = net_sin6(net_sad(server));

		addr6->sin6_family = NET_AF_INET6;
		addr6->sin6_port = net_htons(CONFIG_SNMP_PORT);
		zassert_equal(zsock_inet_pton(NET_AF_INET6, "::1", &addr6->sin6_addr), 1);
		*server_len = sizeof(*addr6);
	}

	sock = zsock_socket(family, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	zassert_true(sock >= 0, "Cannot create client socket (%d)", -errno);

	if (family == NET_AF_INET && client_ipv4 != NULL) {
		struct net_sockaddr_in local = {.sin_family = NET_AF_INET};

		zassert_equal(zsock_inet_pton(NET_AF_INET, client_ipv4, &local.sin_addr), 1);
		zassert_ok(zsock_bind(sock, (struct net_sockaddr *)&local, sizeof(local)),
			   "Cannot bind client to %s (%d)", client_ipv4, -errno);
	}

	return sock;
}

/* Send a request and return the reply length, or -1 when none arrived. */
static int query(net_sa_family_t family, const uint8_t *request, size_t request_len, uint8_t *reply,
		 size_t reply_size)
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

	{
		struct net_sockaddr_storage from;
		net_socklen_t from_len = sizeof(from);

		ret = zsock_recvfrom(sock, reply, reply_size, 0, net_sad(&from), &from_len);
		zassert_true(ret >= 0, "Cannot receive reply (%d)", -errno);

		/* RFC 1157 4.1: the reply comes from the address the request went to */
		if (family == NET_AF_INET) {
			zassert_mem_equal(&net_sin(net_sad(&from))->sin_addr,
					  &net_sin(net_sad(&server))->sin_addr,
					  sizeof(struct net_in_addr),
					  "Reply source differs from the request destination");
		}
	}

	(void)zsock_close(sock);

	return ret;
}

static void check_reply(const uint8_t *reply, size_t len)
{
	struct snmp_ber_reader outer, msg, pdu, list, vb;
	const uint8_t *community;
	size_t community_len, pdu_len;
	int32_t val;
	uint8_t tag;

	snmp_ber_reader_init(&outer, reply, len);
	zassert_ok(snmp_ber_read_sequence(&outer, SNMP_BER_TAG_SEQUENCE, &msg));
	zassert_ok(snmp_ber_read_int32(&msg, &val));
	zassert_equal(val, 1, "Version");
	zassert_ok(
		snmp_ber_read_octets(&msg, SNMP_BER_TAG_OCTET_STRING, &community, &community_len));
	zassert_equal(community_len, 6);
	zassert_ok(snmp_ber_read_header(&msg, &tag, &pdu_len));
	zassert_equal(tag, SNMP_BER_TAG_RESPONSE);
	snmp_ber_reader_init(&pdu, &msg.data[msg.pos], pdu_len);
	zassert_ok(snmp_ber_read_int32(&pdu, &val));
	zassert_equal(val, 1, "Request id");
	zassert_ok(snmp_ber_read_int32(&pdu, &val));
	zassert_equal(val, 0, "Error status");
	zassert_ok(snmp_ber_read_int32(&pdu, &val));
	zassert_ok(snmp_ber_read_sequence(&pdu, SNMP_BER_TAG_SEQUENCE, &list));
	zassert_ok(snmp_ber_read_sequence(&list, SNMP_BER_TAG_SEQUENCE, &vb));
	zassert_ok(snmp_ber_skip(&vb));
	zassert_ok(snmp_ber_read_int32(&vb, &val));
	zassert_equal(val, 1, "Value of test scalar a");
}

ZTEST(snmp_transport, test_lifecycle_and_exchange)
{
	uint8_t reply[128];
	int ret;

	zassert_equal(snmp_agent_stop(), -EALREADY, "Stopping a stopped agent");

	zassert_ok(snmp_agent_start());
	zassert_equal(snmp_agent_start(), -EALREADY, "Starting twice");

	ret = query(NET_AF_INET, get_a, sizeof(get_a), reply, sizeof(reply));
	zassert_true(ret > 0, "No reply over IPv4");
	check_reply(reply, ret);

	ret = query(NET_AF_INET6, get_a, sizeof(get_a), reply, sizeof(reply));
	zassert_true(ret > 0, "No reply over IPv6");
	check_reply(reply, ret);

	ret = query(NET_AF_INET, get_a_wrong, sizeof(get_a_wrong), reply, sizeof(reply));
	zassert_equal(ret, -1, "Wrong community must be ignored");

	/* Garbage is ignored and does not break the service */
	ret = query(NET_AF_INET, reply, 3, reply, sizeof(reply));
	zassert_equal(ret, -1);

	ret = query(NET_AF_INET, get_a, sizeof(get_a), reply, sizeof(reply));
	zassert_true(ret > 0, "No reply after garbage");

	zassert_ok(snmp_agent_stop());
	ret = query(NET_AF_INET, get_a, sizeof(get_a), reply, sizeof(reply));
	zassert_equal(ret, -1, "A stopped agent must not answer");

	zassert_ok(snmp_agent_start(), "Restart after stop");
	ret = query(NET_AF_INET, get_a, sizeof(get_a), reply, sizeof(reply));
	zassert_true(ret > 0, "No reply after restart");
	zassert_ok(snmp_agent_stop());
}

ZTEST(snmp_transport, test_reply_source_address)
{
	struct net_in_addr first, second;
	struct net_if *iface;
	uint8_t reply[128];
	int ret;

	zassert_equal(zsock_inet_pton(NET_AF_INET, "127.0.0.1", &first), 1);
	zassert_equal(zsock_inet_pton(NET_AF_INET, SECOND_IPV4_ADDR, &second), 1);
	zassert_not_null(net_if_ipv4_addr_lookup(&first, &iface), "No loopback address");
	zassert_not_null(net_if_ipv4_addr_add(iface, &second, NET_ADDR_MANUAL, 0),
			 "Cannot add " SECOND_IPV4_ADDR);

	zassert_ok(snmp_agent_start());

	/* From the first address to the second, then the other way round: the
	 * second request must not be answered from the first one's destination.
	 */
	client_ipv4 = "127.0.0.1";
	server_ipv4 = SECOND_IPV4_ADDR;
	ret = query(NET_AF_INET, get_a, sizeof(get_a), reply, sizeof(reply));
	zassert_true(ret > 0, "No reply on the second address");
	check_reply(reply, ret);

	client_ipv4 = SECOND_IPV4_ADDR;
	server_ipv4 = "127.0.0.1";
	ret = query(NET_AF_INET, get_a, sizeof(get_a), reply, sizeof(reply));
	zassert_true(ret > 0, "No reply on the first address");
	check_reply(reply, ret);
	client_ipv4 = NULL;

	zassert_ok(snmp_agent_stop());
	zassert_true(net_if_ipv4_addr_rm(iface, &second));
}

ZTEST_SUITE(snmp_transport, NULL, NULL, NULL, NULL, NULL);
