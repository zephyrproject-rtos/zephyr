/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Kirill Shypachov
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "server.h"

struct test_server server;

static K_THREAD_STACK_DEFINE(server_stack, 4096);
static struct k_thread server_thread;

net_socklen_t addr_len(int family)
{
	return (family == NET_AF_INET) ? sizeof(struct net_sockaddr_in)
				       : sizeof(struct net_sockaddr_in6);
}

void set_loopback(struct net_sockaddr *sa, int family, uint16_t port)
{
	memset(sa, 0, sizeof(*sa));
	if (family == NET_AF_INET) {
		net_sin(sa)->sin_family = NET_AF_INET;
		net_sin(sa)->sin_port = net_htons(port);
		net_sin(sa)->sin_addr.s4_addr[0] = 127;
		net_sin(sa)->sin_addr.s4_addr[3] = 1;
	} else {
#if defined(CONFIG_NET_IPV6)
		net_sin6(sa)->sin6_family = NET_AF_INET6;
		net_sin6(sa)->sin6_port = net_htons(port);
		net_sin6(sa)->sin6_addr.s6_addr[15] = 1;
#endif
	}
}

uint16_t port_of(const struct net_sockaddr *sa)
{
#if defined(CONFIG_NET_IPV6)
	if (sa->sa_family == NET_AF_INET6) {
		return net_ntohs(net_sin6(sa)->sin6_port);
	}
#endif
	return net_ntohs(net_sin(sa)->sin_port);
}

int udp_socket(int family)
{
	int sock = zsock_socket(family, NET_SOCK_DGRAM, NET_IPPROTO_UDP);

	zassert_true(sock >= 0, "socket: %d", errno);
	return sock;
}

static void server_listen(int family, uint16_t port)
{
	struct net_sockaddr addr;
	int sock = udp_socket(family);

	set_loopback(&addr, family, port);
	zassert_ok(zsock_bind(sock, &addr, addr_len(family)), "bind: %d", errno);
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

	stun_builder_init(&b, out, cap, type, txid);
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
	static const uint8_t kNotStun[24] = {0x80, 0x60, 0x12, 0x34};
	uint8_t other_txid[STUN_TXID_SIZE];
	struct net_sockaddr lie;
	uint8_t out[128];
	size_t len;

	set_loopback(&lie, from->sa_family, LIE_PORT);
	(void)zsock_sendto(sock, kNotStun, sizeof(kNotStun), 0, from, from_len);

	memcpy(other_txid, req->txid, sizeof(other_txid));
	other_txid[0] ^= 0x01;
	len = make_response(out, sizeof(out), other_txid, STUN_BINDING_SUCCESS_RESPONSE, &lie);
	(void)zsock_sendto(sock, out, len, 0, from, from_len);

	len = make_response(out, sizeof(out), req->txid, STUN_BINDING_SUCCESS_RESPONSE, &lie);
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
	struct net_sockaddr lie;
	struct stun_builder b;
	int len, room;

	set_loopback(&lie, from->sa_family, LIE_PORT);
	memset(pad, 0x5A, sizeof(pad));
	stun_builder_init(&b, out, CONFIG_STUN_CLIENT_RX_BUF_SIZE, STUN_BINDING_SUCCESS_RESPONSE,
			  req->txid);
	(void)stun_add_xor_mapped_address(&b, &lie);
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

static void serve_one(int sock, uint16_t port)
{
	struct net_sockaddr from;
	net_socklen_t from_len = sizeof(from);
	struct stun_msg m;
	const uint8_t *val;
	uint16_t vlen;
	uint8_t req[256];
	uint8_t out[128];
	atomic_val_t idx;
	ssize_t n;
	size_t len;

	n = zsock_recvfrom(sock, req, sizeof(req), ZSOCK_MSG_DONTWAIT, &from, &from_len);
	if (n <= 0 || stun_parse(req, (size_t)n, &m) != 0 || m.type != STUN_BINDING_REQUEST) {
		return;
	}
	idx = atomic_inc(&server.requests);
	if (idx < 2) {
		memcpy(server.txid[idx], m.txid, STUN_TXID_SIZE);
	}
	atomic_set(&server.last_port, port);
	atomic_set(&server.last_client_port, port_of(&from));
	atomic_set(&server.software_seen,
		   stun_find_attr(req, (size_t)n, &m, STUN_ATTR_SOFTWARE, &val, &vlen) == 0);

	switch ((enum server_mode)atomic_get(&server.mode)) {
	case MODE_SILENT:
		return;
	case MODE_ANSWER_SECOND:
		if (idx == 0) {
			return;
		}
		break;
	case MODE_ERROR_500:
		len = make_response(out, sizeof(out), m.txid, STUN_BINDING_ERROR_RESPONSE, &from);
		(void)zsock_sendto(sock, out, len, 0, &from, from_len);
		return;
	case MODE_NOISE_FIRST:
		send_noise(sock, &m, &from, from_len);
		break;
	case MODE_OVERSIZED:
		send_oversized(sock, &m, &from, from_len);
		return;
	default:
		break;
	}

	len = make_response(out, sizeof(out), m.txid, STUN_BINDING_SUCCESS_RESPONSE, &from);
	(void)zsock_sendto(sock, out, len, 0, &from, from_len);
}

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
	k_thread_create(&server_thread, server_stack, K_THREAD_STACK_SIZEOF(server_stack),
			server_main, NULL, NULL, NULL, K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
}

void server_reset(void)
{
	atomic_set(&server.mode, MODE_ANSWER);
	atomic_set(&server.requests, 0);
	atomic_set(&server.last_port, 0);
	atomic_set(&server.last_client_port, 0);
	atomic_set(&server.software_seen, 0);
	memset(server.txid, 0, sizeof(server.txid));
	atomic_set(&server.sent_len, 0);
}
