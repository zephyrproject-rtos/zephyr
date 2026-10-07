/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Kirill Shypachov
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * STUN client over UDP sockets: the Binding transaction of stun_binding.c with
 * what it leaves to its caller — a socket, the system clock and the random
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

/* The request of a transaction has room for the header, a SOFTWARE attribute
 * and FINGERPRINT: a description that does not fit would fail every query.
 */
#define SOFTWARE_MAX_LEN (STUN_BINDING_REQUEST_SIZE - STUN_HEADER_SIZE - 4U - 8U)

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
	struct net_sockaddr from;
	net_socklen_t from_len = sizeof(from);
	ssize_t n;
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
		/* An event, and nothing to read: going on would spin. */
		return -EIO;
	}

	memset(&from, 0, sizeof(from));
	n = zsock_recvfrom(sock, rx, sizeof(rx), ZSOCK_MSG_DONTWAIT | ZSOCK_MSG_TRUNC, &from,
			   &from_len);
	if (n < 0) {
		return (errno == EAGAIN || errno == EWOULDBLOCK) ? 0 : -errno;
	}
	/* MSG_TRUNC reports the length of the datagram, not of what was copied:
	 * a datagram that did not fit is not a message this client can judge
	 * (RFC 8489 §5: the message fills the datagram), so it is not the
	 * answer. The transaction goes on waiting.
	 */
	if ((size_t)n > sizeof(rx)) {
		LOG_DBG("datagram of %d bytes does not fit %zu: ignored", (int)n, sizeof(rx));
		return 0;
	}

	/* What is not the server's answer is dropped here: the transaction
	 * counts it and goes on waiting. The caller is not told why; the log is.
	 */
	rc = stun_binding_on_datagram(txn, &from, rx, (size_t)n);
	if (rc < 0) {
		LOG_DBG("datagram of %d bytes ignored (%d)", (int)n, rc);
	}
	return 0;
}

int stun_client_query(int sock, const struct net_sockaddr *server, net_socklen_t server_len,
		      uint32_t timeout_ms, struct net_sockaddr *mapped)
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

	/* RFC 8489 §5: the transaction id is what an off-path attacker would
	 * have to guess to forge a response.
	 */
	rc = sys_csrand_get(txid, sizeof(txid));
	if (rc != 0) {
		LOG_ERR("no random transaction id (%d)", rc);
		return -EIO;
	}

	memset(&txn, 0, sizeof(txn));
	start = k_uptime_get_32();
	rc = stun_binding_start(&txn, server, txid, software, NULL, start, &step);
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
		rc = stun_binding_advance(&txn, now, &step);
		if (rc != 0) {
			return rc;
		}
	}
}

int stun_client_simple(const char *host, uint16_t port, uint32_t timeout_ms,
		       struct net_sockaddr *mapped)
{
	const struct zsock_addrinfo hints = {
		.ai_family = NET_AF_UNSPEC,
		.ai_socktype = NET_SOCK_DGRAM,
	};
	struct zsock_addrinfo *res = NULL;
	char service[sizeof("65535")];
	int sock;
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

	sock = zsock_socket(res->ai_family, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	if (sock < 0) {
		rc = -errno;
	} else {
		rc = stun_client_query(sock, res->ai_addr, res->ai_addrlen, timeout_ms, mapped);
		(void)zsock_close(sock);
	}
	zsock_freeaddrinfo(res);
	return rc;
}
