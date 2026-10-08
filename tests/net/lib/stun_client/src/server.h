/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Kirill Shypachov
 * SPDX-License-Identifier: Apache-2.0
 *
 * A scripted STUN server for the tests of the client: it runs in a thread of
 * the test and is reached over the loopback interface.
 */
#ifndef STUN_CLIENT_TEST_SERVER_H_
#define STUN_CLIENT_TEST_SERVER_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/stun.h>
#include <zephyr/net/stun_client.h>
#include <zephyr/sys/atomic.h>

/* The server listens on the port STUN has by default, and on one more. */
#define SERVER_PORT STUN_CLIENT_DEFAULT_PORT
#define OTHER_PORT  34781

/* A port nothing in the test has: an address with it is a lie. */
#define LIE_PORT 9

/* The tests that are about no family in particular run over IPv4 where there is
 * an IPv4 stack, and over IPv6 where there is not.
 */
#if defined(CONFIG_NET_IPV4)
#define FAMILY NET_AF_INET
#define HOST   "127.0.0.1"
#else
#define FAMILY NET_AF_INET6
#define HOST   "::1"
#endif

/* What the server does with the requests it receives. */
enum server_mode {
	MODE_ANSWER,        /* a success response with the address it saw */
	MODE_SILENT,        /* nothing */
	MODE_ANSWER_SECOND, /* nothing to the first request, an answer to the second */
	MODE_ERROR_500,     /* an error response */
	MODE_NOISE_FIRST,   /* three datagrams that are not the answer, then the answer */
	MODE_OVERSIZED,     /* a datagram longer than the client's buffer, with a lie in it */
};

#define SERVER_SOCKS_MAX 3

struct test_server {
	struct zsock_pollfd fds[SERVER_SOCKS_MAX];
	uint16_t ports[SERVER_SOCKS_MAX];
	int count;
	int sock_other; /* the one bound to OTHER_PORT */
	atomic_t mode;
	atomic_t requests;
	atomic_t last_port;              /* the port the last request came to */
	atomic_t last_client_port;       /* the port the last request came from */
	atomic_t software_seen;          /* the last request carried SOFTWARE */
	atomic_t sent_len;               /* length of the last datagram sent */
	uint8_t txid[2][STUN_TXID_SIZE]; /* of the first two requests */
};

extern struct test_server server;

/* Bind the sockets of the server and start its thread, the first time it is
 * called; the suites of the test share one server.
 */
void server_start(void);

/* Back to answering every request, with nothing counted. */
void server_reset(void);

/* A UDP socket of a family; the test fails if there is none to be had. */
int udp_socket(int family);

/* The size of a socket address of a family. */
net_socklen_t addr_len(int family);

/* The loopback address of a family, with a port. */
void set_loopback(struct net_sockaddr *sa, int family, uint16_t port);

/* The port of a socket address, in host byte order. */
uint16_t port_of(const struct net_sockaddr *sa);

#endif /* STUN_CLIENT_TEST_SERVER_H_ */
