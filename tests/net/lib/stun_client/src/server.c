/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Kirill Shypachov
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#include "server.h"

/* The DNS message (RFC 1035 section 4.1): the header, the flags the answers set
 * in its second and third byte, and what follows a name in a question and in a
 * resource record.
 */
#define DNS_HEADER_SIZE      12U
#define DNS_FLAGS_OFFSET     2U
#define DNS_ANCOUNT_OFFSET   6U
#define DNS_FLAG_RESPONSE    0x80U
#define DNS_RCODE_NO_NAME    3U
#define DNS_QUESTION_TAIL    4U  /* QTYPE and QCLASS */
#define DNS_RECORD_HEAD      12U /* name pointer, TYPE, CLASS, TTL and RDLENGTH */
#define DNS_NAME_POINTER     0xC00CU /* to the name of the question */
#define DNS_TYPE_A           1U
#define DNS_TYPE_AAAA        28U
#define DNS_CLASS_IN         1U
#define DNS_TTL_S            60U

struct test_server server;

static K_THREAD_STACK_DEFINE(server_stack, 4096);
static struct k_thread server_thread;

/* sizeof the socket address structure of the family. */
net_socklen_t addr_len(int family)
{
	return (family == NET_AF_INET) ? sizeof(struct net_sockaddr_in)
				       : sizeof(struct net_sockaddr_in6);
}

/* Zero sa, then set the family, the port and 127.0.0.1 or ::1. */
void set_loopback(struct net_sockaddr *sa, int family, uint16_t port)
{
	memset(sa, 0, sizeof(*sa));
	if (family == NET_AF_INET) {
		net_sin(sa)->sin_family = NET_AF_INET;
		net_sin(sa)->sin_port = net_htons(port);
		net_sin(sa)->sin_addr.s4_addr[0] = 127;
		net_sin(sa)->sin_addr.s4_addr[3] = 1;
	} else if (IS_ENABLED(CONFIG_NET_IPV6)) {
		net_sin6(sa)->sin6_family = NET_AF_INET6;
		net_sin6(sa)->sin6_port = net_htons(port);
		net_sin6(sa)->sin6_addr.s6_addr[15] = 1;
	}
}

/* Read sin_port or sin6_port by the family, in host byte order. */
uint16_t port_of(const struct net_sockaddr *sa)
{
	if (IS_ENABLED(CONFIG_NET_IPV6) && sa->sa_family == NET_AF_INET6) {
		return net_ntohs(net_sin6(sa)->sin6_port);
	}
	return net_ntohs(net_sin(sa)->sin_port);
}

/* Open a UDP socket of the family and assert that it opened. */
int udp_socket(int family)
{
	int sock = zsock_socket(family, NET_SOCK_DGRAM, NET_IPPROTO_UDP);

	zassert_true(sock >= 0, "socket: %d", errno);
	return sock;
}

/* Bind a socket to the loopback address and the port, and add it to the set the
 * server polls.
 */
static void server_listen(int family, uint16_t port)
{
	struct net_sockaddr_storage addr = {0};
	int sock = udp_socket(family);

	set_loopback(net_sad(&addr), family, port);
	zassert_ok(zsock_bind(sock, net_sad(&addr), addr_len(family)), "bind: %d", errno);
	zassert_true(server.count < SERVER_SOCKS_MAX);
	server.fds[server.count].fd = sock;
	server.fds[server.count].events = ZSOCK_POLLIN;
	server.ports[server.count] = port;
	server.count++;
	if (port == OTHER_PORT) {
		server.sock_other = sock;
	}
}

/* A response to the transaction `txid`, telling the client its address is `addr`. */
static size_t make_response(uint8_t *out, size_t cap, const uint8_t *txid, uint16_t type,
			    const struct net_sockaddr *addr)
{
	struct stun_builder b;
	int len;

	if (stun_builder_init(&b, out, cap, type, txid) != 0) {
		return 0U;
	}
	if (type == STUN_BINDING_ERROR_RESPONSE) {
		(void)stun_add_error_code(&b, 500, "Server Error");
	} else {
		(void)stun_add_xor_mapped_address(&b, addr);
	}
	len = stun_finish_plain(&b);
	return (len > 0) ? (size_t)len : 0U;
}

/* What must not end a query, each of them saying the client is at an address
 * it is not at: something that is not STUN, a response to another transaction,
 * and a response to this one that comes from another port than the client
 * wrote to.
 */
static void send_noise(int sock, const struct stun_msg *req, const struct net_sockaddr *from,
		       net_socklen_t from_len)
{
	/* The first bytes of an RTP packet (version 2, payload type 96), then zeros. */
	static const uint8_t not_stun[24] = {0x80, 0x60, 0x12, 0x34};
	uint8_t other_txid[STUN_TXID_SIZE];
	struct net_sockaddr_storage lie = {0};
	uint8_t out[128];
	size_t len;

	set_loopback(net_sad(&lie), from->sa_family, LIE_PORT);
	(void)zsock_sendto(sock, not_stun, sizeof(not_stun), 0, from, from_len);

	memcpy(other_txid, req->txid, sizeof(other_txid));
	other_txid[0] ^= 0x01;
	len = make_response(out, sizeof(out), other_txid, STUN_BINDING_SUCCESS_RESPONSE,
			    net_sad(&lie));
	(void)zsock_sendto(sock, out, len, 0, from, from_len);

	len = make_response(out, sizeof(out), req->txid, STUN_BINDING_SUCCESS_RESPONSE,
			    net_sad(&lie));
	(void)zsock_sendto(server.sock_other, out, len, 0, from, from_len);
}

/* A datagram of CONFIG_STUN_CLIENT_RX_BUF_SIZE + 4 bytes whose first
 * CONFIG_STUN_CLIENT_RX_BUF_SIZE bytes are a well-formed success response
 * that says the client is at `lie`, padded with a comprehension-optional
 * attribute nobody knows. As a whole the datagram is not a STUN message
 * (RFC 8489 section 5: the header says how long the message is, and the
 * message fills the datagram); only its truncated view is.
 */
static void send_oversized(int sock, const struct stun_msg *req, const struct net_sockaddr *from,
			   net_socklen_t from_len)
{
	static uint8_t out[CONFIG_STUN_CLIENT_RX_BUF_SIZE + 4U];
	static uint8_t pad[CONFIG_STUN_CLIENT_RX_BUF_SIZE];
	struct net_sockaddr_storage lie = {0};
	struct stun_builder b;
	int len, room;

	set_loopback(net_sad(&lie), from->sa_family, LIE_PORT);
	memset(pad, 0x5A, sizeof(pad));
	if (stun_builder_init(&b, out, CONFIG_STUN_CLIENT_RX_BUF_SIZE,
			      STUN_BINDING_SUCCESS_RESPONSE, req->txid) != 0) {
		atomic_set(&server.sent_len, -1);
		return;
	}
	(void)stun_add_xor_mapped_address(&b, net_sad(&lie));
	/* Fill up to the buffer size exactly: this attribute, then FINGERPRINT. */
	room = CONFIG_STUN_CLIENT_RX_BUF_SIZE - stun_builder_len(&b) - 4 - 8;
	(void)stun_add_attr(&b, 0x8FFFU, pad, (size_t)room);
	len = stun_finish_plain(&b);
	if (len != CONFIG_STUN_CLIENT_RX_BUF_SIZE) {
		atomic_set(&server.sent_len, -1); /* the test sees that the setup failed */
		return;
	}
	memset(out + len, 0xA5, 4U);
	atomic_set(&server.sent_len, len + 4);
	(void)zsock_sendto(sock, out, (size_t)len + 4U, 0, from, from_len);
}

/* Receive one DNS query and answer it: for DUAL_HOST with an A record of
 * 127.0.0.1 or an AAAA record of ::1, for anything else with "no such name".
 */
static void serve_dns(int sock)
{
	/* DUAL_HOST as DNS writes it: every label after its length, then the root. */
	static const uint8_t name[] = {4, 'd', 'u', 'a', 'l', 4, 't', 'e', 's', 't', 0};
	static const uint8_t loopback4[sizeof(struct net_in_addr)] = {127, 0, 0, 1};
	/* The address ::1: every byte zero but the last. */
	static const uint8_t loopback6[sizeof(struct net_in6_addr)] = {[15] = 1};
	const size_t question_len = DNS_HEADER_SIZE + sizeof(name) + DNS_QUESTION_TAIL;
	struct net_sockaddr_storage from = {0};
	net_socklen_t from_len = sizeof(from);
	uint8_t msg[DNS_HEADER_SIZE + sizeof(name) + DNS_QUESTION_TAIL + DNS_RECORD_HEAD +
		    sizeof(loopback6)];
	const uint8_t *rdata = NULL;
	uint16_t type = 0U;
	size_t rdlen = 0U;
	size_t len = 0U;
	ssize_t n;

	/* A query longer than the one for DUAL_HOST is cut and gets "no such name". */
	n = zsock_recvfrom(sock, msg, question_len, ZSOCK_MSG_DONTWAIT, net_sad(&from),
			   &from_len);
	if (n < (ssize_t)DNS_HEADER_SIZE) {
		return;
	}
	len = (size_t)n;

	if (len == question_len && memcmp(&msg[DNS_HEADER_SIZE], name, sizeof(name)) == 0) {
		type = sys_get_be16(&msg[DNS_HEADER_SIZE + sizeof(name)]);
		if (type == DNS_TYPE_A) {
			rdata = loopback4;
			rdlen = sizeof(loopback4);
		} else if (type == DNS_TYPE_AAAA) {
			rdata = loopback6;
			rdlen = sizeof(loopback6);
		}
	}

	msg[DNS_FLAGS_OFFSET] |= DNS_FLAG_RESPONSE;
	if (rdata == NULL) {
		msg[DNS_FLAGS_OFFSET + 1U] = DNS_RCODE_NO_NAME;
	} else {
		sys_put_be16(1U, &msg[DNS_ANCOUNT_OFFSET]);
		sys_put_be16(DNS_NAME_POINTER, &msg[len]);
		sys_put_be16(type, &msg[len + 2U]);
		sys_put_be16(DNS_CLASS_IN, &msg[len + 4U]);
		sys_put_be32(DNS_TTL_S, &msg[len + 6U]);
		sys_put_be16((uint16_t)rdlen, &msg[len + 10U]);
		memcpy(&msg[len + DNS_RECORD_HEAD], rdata, rdlen);
		len += DNS_RECORD_HEAD + rdlen;
	}
	(void)zsock_sendto(sock, msg, len, 0, net_sad(&from), from_len);
}

/* Receive one datagram; if it is a Binding request, record it and answer as the
 * mode says. A request over another family than mode_family is answered.
 */
static void serve_one(int sock, uint16_t port)
{
	struct net_sockaddr_storage from = {0};
	net_socklen_t from_len = sizeof(from);
	struct stun_msg m;
	const uint8_t *val;
	uint16_t vlen;
	uint8_t req[256];
	uint8_t out[128];
	atomic_val_t only = atomic_get(&server.mode_family);
	enum server_mode mode = (enum server_mode)atomic_get(&server.mode);
	atomic_val_t idx;
	ssize_t n;
	size_t len;

	if (port == DNS_PORT) {
		serve_dns(sock);
		return;
	}

	n = zsock_recvfrom(sock, req, sizeof(req), ZSOCK_MSG_DONTWAIT, net_sad(&from), &from_len);
	if (n <= 0 || stun_parse(req, (size_t)n, &m) != 0 || m.type != STUN_BINDING_REQUEST) {
		return;
	}
	idx = atomic_inc(&server.requests);
	if (idx < 2) {
		memcpy(server.txid[idx], m.txid, STUN_TXID_SIZE);
	}
	atomic_set(&server.last_port, port);
	atomic_set(&server.last_client_port, port_of(net_sad(&from)));
	atomic_set(&server.software_seen, stun_find_attr(&m, STUN_ATTR_SOFTWARE, &val, &vlen) == 0);

	if (only != NET_AF_UNSPEC && only != net_sad(&from)->sa_family) {
		mode = MODE_ANSWER;
	}

	switch (mode) {
	case MODE_SILENT:
		return;
	case MODE_ANSWER_SECOND:
		if (idx == 0) {
			return;
		}
		break;
	case MODE_ERROR_500:
		len = make_response(out, sizeof(out), m.txid, STUN_BINDING_ERROR_RESPONSE,
				    net_sad(&from));
		(void)zsock_sendto(sock, out, len, 0, net_sad(&from), from_len);
		return;
	case MODE_NOISE_FIRST:
		send_noise(sock, &m, net_sad(&from), from_len);
		break;
	case MODE_OVERSIZED:
		send_oversized(sock, &m, net_sad(&from), from_len);
		return;
	default:
		break;
	}

	len = make_response(out, sizeof(out), m.txid, STUN_BINDING_SUCCESS_RESPONSE,
			    net_sad(&from));
	(void)zsock_sendto(sock, out, len, 0, net_sad(&from), from_len);
}

/* The thread of the server: poll the sockets forever and serve each one that
 * has a datagram.
 */
static void server_main(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	for (;;) {
		if (zsock_poll(server.fds, server.count, -1) <= 0) {
			continue;
		}
		for (int i = 0; i < server.count; i++) {
			if ((server.fds[i].revents & ZSOCK_POLLIN) != 0) {
				serve_one(server.fds[i].fd, server.ports[i]);
			}
		}
	}
}

/* On the first call bind the sockets, SERVER_PORT for every enabled family,
 * OTHER_PORT and, with the resolver, DNS_PORT, and create the thread.
 */
void server_start(void)
{
	static bool started;

	if (started) {
		return;
	}
	started = true;

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		server_listen(NET_AF_INET, SERVER_PORT);
	}
	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		server_listen(NET_AF_INET6, SERVER_PORT);
	}
	server_listen(FAMILY, OTHER_PORT);
	if (IS_ENABLED(CONFIG_DNS_RESOLVER)) {
		server_listen(NET_AF_INET, DNS_PORT);
	}
	k_thread_create(&server_thread, server_stack, K_THREAD_STACK_SIZEOF(server_stack),
			server_main, NULL, NULL, NULL, K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
}

/* Set the mode to MODE_ANSWER for every family and zero the counters and the
 * recorded transaction ids.
 */
void server_reset(void)
{
	atomic_set(&server.mode, MODE_ANSWER);
	atomic_set(&server.mode_family, NET_AF_UNSPEC);
	atomic_set(&server.requests, 0);
	atomic_set(&server.last_port, 0);
	atomic_set(&server.last_client_port, 0);
	atomic_set(&server.software_seen, 0);
	memset(server.txid, 0, sizeof(server.txid));
	atomic_set(&server.sent_len, 0);
}
