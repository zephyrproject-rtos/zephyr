/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Kirill Shypachov
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * STUN client over UDP sockets: the Binding transaction of stun_binding.c with
 * what it leaves to its caller: a socket, the system clock and the random
 * number generator.
 */

#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/stun.h>
#include <zephyr/net/stun_client.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

/* The module of stun.c, and its rules: what the network makes the client do is
 * a debug record at most.
 */
LOG_MODULE_DECLARE(net_stun, CONFIG_STUN_LOG_LEVEL);

/* An attribute is a header and its value (RFC 8489 section 14); the value of
 * FINGERPRINT is a CRC-32 (section 14.7).
 */
#define ATTR_HEADER_SIZE      4U
#define FINGERPRINT_VALUE_LEN 4U

/* The request of a transaction has room for the header, a SOFTWARE attribute
 * and FINGERPRINT: a description that does not fit would fail every query.
 */
#define SOFTWARE_MAX_LEN                                                                           \
	(STUN_BINDING_REQUEST_SIZE - STUN_HEADER_SIZE - ATTR_HEADER_SIZE -                         \
	 (ATTR_HEADER_SIZE + FINGERPRINT_VALUE_LEN))

BUILD_ASSERT(sizeof(CONFIG_STUN_CLIENT_SOFTWARE) - 1U <= SOFTWARE_MAX_LEN,
	     "CONFIG_STUN_CLIENT_SOFTWARE does not fit into a Binding request");

/* Is server_len enough for the family the address claims to be of? An unknown
 * family is left for the transaction to refuse.
 */
static bool server_len_ok(const struct net_sockaddr *server, net_socklen_t server_len)
{
	if (server_len < sizeof(server->sa_family)) {
		return false;
	}
	if (server->sa_family == NET_AF_INET) {
		return server_len >= sizeof(struct net_sockaddr_in);
	}
	if (server->sa_family == NET_AF_INET6) {
		return server_len >= sizeof(struct net_sockaddr_in6);
	}
	return true;
}

/* Wait until the socket has something to read or `wait_ms` have passed, and
 * give what arrives to the transaction. Returns 0, or a negative errno if the
 * socket has failed.
 */
static int wait_and_receive(int sock, struct stun_binding *txn, uint32_t wait_ms)
{
	struct zsock_pollfd pfd = {.fd = sock, .events = ZSOCK_POLLIN};
	uint8_t rx[CONFIG_STUN_CLIENT_RX_BUF_SIZE];
	struct net_sockaddr_storage from;
	net_socklen_t from_len = sizeof(from);
	ssize_t len;
	int rc;

	rc = zsock_poll(&pfd, 1, (int)MIN(wait_ms, (uint32_t)INT32_MAX));
	if (rc < 0) {
		return -errno;
	}
	if (rc == 0) {
		return 0;
	}
	if ((pfd.revents & ZSOCK_POLLNVAL) != 0) {
		return -EBADF;
	}
	if ((pfd.revents & ZSOCK_POLLIN) == 0) {
		/* An event, and nothing to read: going on would spin. The
		 * result is the error the socket holds, -EIO if it holds none.
		 */
		int so_error = 0;
		net_socklen_t so_error_len = sizeof(so_error);

		if (zsock_getsockopt(sock, ZSOCK_SOL_SOCKET, ZSOCK_SO_ERROR, &so_error,
				     &so_error_len) == 0 &&
		    so_error > 0) {
			return -so_error;
		}
		return -EIO;
	}

	memset(&from, 0, sizeof(from));
	len = zsock_recvfrom(sock, rx, sizeof(rx), ZSOCK_MSG_DONTWAIT | ZSOCK_MSG_TRUNC,
			     net_sad(&from), &from_len);
	if (len < 0) {
		return (errno == EAGAIN || errno == EWOULDBLOCK) ? 0 : -errno;
	}
	/* MSG_TRUNC reports the length of the datagram, not of what was copied:
	 * a datagram that did not fit is not a message this client can judge
	 * (RFC 8489 section 5: the message fills the datagram), so it is not the
	 * answer. The transaction goes on waiting.
	 */
	if ((size_t)len > sizeof(rx)) {
		LOG_DBG("datagram of %d bytes does not fit %zu: ignored", (int)len, sizeof(rx));
		return 0;
	}

	/* What is not the server's answer is dropped here: the transaction
	 * goes on waiting. The caller is not told why; the log is.
	 */
	rc = stun_binding_on_datagram(txn, net_sad(&from), rx, (size_t)len);
	if (rc < 0) {
		LOG_DBG("datagram of %d bytes ignored (%d)", (int)len, rc);
	}
	return 0;
}

/* Draw a transaction id, start a Binding transaction and carry out its steps on
 * the socket until it ends or timeout_ms have passed.
 */
int stun_client_query(int sock, const struct net_sockaddr *server, net_socklen_t server_len,
		      uint32_t timeout_ms, struct net_sockaddr_storage *mapped)
{
	const char *software =
		(sizeof(CONFIG_STUN_CLIENT_SOFTWARE) > 1U) ? CONFIG_STUN_CLIENT_SOFTWARE : NULL;
	uint8_t txid[STUN_TXID_SIZE];
	struct stun_txn_step step;
	struct stun_binding txn;
	uint32_t start, now;
	int rc;

	if (server == NULL || mapped == NULL || !server_len_ok(server, server_len)) {
		return -EINVAL;
	}

	/* RFC 8489 section 5: the transaction id is what an off-path attacker would
	 * have to guess to forge a response.
	 */
	rc = sys_csrand_get(txid, sizeof(txid));
	if (rc != 0) {
		LOG_ERR("no random transaction id (%d)", rc);
		return -EIO;
	}

	memset(&txn, 0, sizeof(txn));
	start = k_uptime_get_32();
	rc = stun_binding_start(&txn, server, txid, software, start, &step);
	if (rc != 0) {
		return rc;
	}

	for (;;) {
		uint32_t wait_ms;

		switch (step.action) {
		case STUN_TXN_SEND:
			if (zsock_sendto(sock, step.tx, step.tx_len, 0, server, server_len) < 0) {
				return -errno;
			}
			break;
		case STUN_TXN_WAIT:
			/* The deadline may have passed since the step was made:
			 * the difference of the two clock readings is signed.
			 */
			now = k_uptime_get_32();
			wait_ms = (uint32_t)MAX((int32_t)(step.wait_until_ms - now), 0);
			if (timeout_ms != 0U) {
				uint32_t elapsed = now - start;

				wait_ms = (elapsed < timeout_ms)
						  ? MIN(wait_ms, timeout_ms - elapsed)
						  : 0U;
			}
			rc = wait_and_receive(sock, &txn, wait_ms);
			if (rc != 0) {
				return rc;
			}
			break;
		case STUN_TXN_DONE:
			*mapped = txn.mapped;
			return 0;
		case STUN_TXN_FAILED:
			if (txn.fail == STUN_BINDING_FAIL_TIMEOUT) {
				return -ETIMEDOUT;
			}
			if (txn.fail == STUN_BINDING_FAIL_ERROR_RESPONSE) {
				LOG_DBG("the server answered with error %u", txn.error_code);
			} else {
				LOG_DBG("the answer of the server cannot be used");
			}
			return -EPROTO;
		default:
			return -EIO;
		}

		now = k_uptime_get_32();
		if (timeout_ms != 0U && now - start >= timeout_ms &&
		    txn.state == STUN_BINDING_IN_FLIGHT) {
			return -ETIMEDOUT;
		}
		rc = stun_binding_next_step(&txn, now, &step);
		if (rc != 0) {
			return rc;
		}
	}
}

/* Open a UDP socket of the family of the address, connect it to the address, run
 * stun_client_query() on it and close it.
 */
static int query_addr(const struct zsock_addrinfo *ai, uint32_t timeout_ms,
		      struct net_sockaddr_storage *mapped)
{
	int sock = zsock_socket(ai->ai_family, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	int rc = 0;

	if (sock < 0) {
		return -errno;
	}

	/* A connected socket receives only the datagrams of the server. */
	if (zsock_connect(sock, ai->ai_addr, ai->ai_addrlen) < 0) {
		rc = -errno;
	} else {
		rc = stun_client_query(sock, ai->ai_addr, ai->ai_addrlen, timeout_ms, mapped);
	}
	(void)zsock_close(sock);
	return rc;
}

/* Resolve the host and run query_addr() on the addresses found, one after
 * another, until one of them succeeds. Each address gets an equal share of the
 * time that is left of timeout_ms, or the whole schedule if timeout_ms is 0.
 */
int stun_client_simple(const char *host, uint16_t port, uint32_t timeout_ms,
		       struct net_sockaddr_storage *mapped)
{
	const struct zsock_addrinfo hints = {
		.ai_family = NET_AF_UNSPEC,
		.ai_socktype = NET_SOCK_DGRAM,
	};
	struct zsock_addrinfo *res = NULL;
	char service[sizeof("65535")];
	uint32_t addrs_left = 0U;
	uint32_t start = 0U;
	int rc;

	if (host == NULL || mapped == NULL) {
		return -EINVAL;
	}

	snprintk(service, sizeof(service), "%u", (port != 0U) ? port : STUN_CLIENT_DEFAULT_PORT);

	/* The resolver reports DNS_EAI_* codes, which are not errno values and
	 * would be taken for some: one errno stands for all of them.
	 */
	rc = zsock_getaddrinfo(host, service, &hints, &res);
	if (rc != 0 || res == NULL) {
		LOG_DBG("%s does not resolve (%d)", host, rc);
		return -EHOSTUNREACH;
	}

	for (const struct zsock_addrinfo *ai = res; ai != NULL; ai = ai->ai_next) {
		addrs_left++;
	}

	start = k_uptime_get_32();
	for (const struct zsock_addrinfo *ai = res; ai != NULL; ai = ai->ai_next) {
		uint32_t share_ms = 0U;

		if (timeout_ms != 0U) {
			uint32_t elapsed = k_uptime_get_32() - start;
			uint32_t left_ms = (elapsed < timeout_ms) ? timeout_ms - elapsed : 0U;

			/* 0 would stand for the whole schedule. */
			share_ms = MAX(left_ms / addrs_left, 1U);
		}

		rc = query_addr(ai, share_ms, mapped);
		if (rc == 0) {
			break;
		}
		LOG_DBG("an address of %s gave no answer (%d)", host, rc);
		addrs_left--;
	}
	zsock_freeaddrinfo(res);
	return rc;
}
