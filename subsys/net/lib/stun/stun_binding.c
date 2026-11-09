/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Kirill Shypachov
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * STUN Binding transaction, RFC 8489 section 6: one request, its retransmissions, and
 * the response. No sockets and no clock: the caller moves the bytes and tells
 * the time.
 */

#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <zephyr/net/net_ip.h>
#include <zephyr/net/stun.h>
#include <zephyr/sys/byteorder.h>

/* The transaction id is the last field of the header (RFC 8489 section 5). */
#define TXID_OFFSET (STUN_HEADER_SIZE - STUN_TXID_SIZE)

/* RFC 8489 section 6.2.1: the initial RTO, Rc and Rm. */
#define RTO_MS         ((uint32_t)CONFIG_STUN_BINDING_RTO_MS)
#define MAX_TRANSMITS  ((uint32_t)CONFIG_STUN_BINDING_MAX_TRANSMITS)
#define FINAL_WAIT_RTO ((uint32_t)CONFIG_STUN_BINDING_FINAL_WAIT_RTO)

/* No interval, the RTO doubled up to Rc - 1 times or Rm times the RTO, exceeds
 * the signed difference deadline_reached() takes.
 */
BUILD_ASSERT(((uint64_t)RTO_MS << (MAX_TRANSMITS - 1U)) <= INT32_MAX);
BUILD_ASSERT((uint64_t)RTO_MS * FINAL_WAIT_RTO <= INT32_MAX);

/* Can the transaction talk to this server? Its family has to be one this build
 * has a stack for, and it has to have a port and an address that is not the
 * unspecified one, 0.0.0.0 or ::.
 */
static int server_addr_check(const struct net_sockaddr *server)
{
	if (IS_ENABLED(CONFIG_NET_IPV4) && server->sa_family == NET_AF_INET) {
		if (net_sin(server)->sin_port == 0U ||
		    net_ipv4_is_addr_unspecified(&net_sin(server)->sin_addr)) {
			return -EINVAL;
		}
		return 0;
	}
	if (IS_ENABLED(CONFIG_NET_IPV6) && server->sa_family == NET_AF_INET6) {
		if (net_sin6(server)->sin6_port == 0U ||
		    net_ipv6_is_addr_unspecified(&net_sin6(server)->sin6_addr)) {
			return -EINVAL;
		}
		return 0;
	}
	return -EAFNOSUPPORT;
}

/* Keep a socket address. Only as much as its family defines is read: the caller
 * may have passed a struct net_sockaddr_in.
 */
static void sockaddr_copy(struct net_sockaddr_storage *dst, const struct net_sockaddr *src)
{
	memset(dst, 0, sizeof(*dst));
	if (IS_ENABLED(CONFIG_NET_IPV4) && src->sa_family == NET_AF_INET) {
		memcpy(dst, src, sizeof(struct net_sockaddr_in));
	}
	if (IS_ENABLED(CONFIG_NET_IPV6) && src->sa_family == NET_AF_INET6) {
		memcpy(dst, src, sizeof(struct net_sockaddr_in6));
	}
}

/* Has the clock reached the deadline? Both wrap around together. */
static bool deadline_reached(uint32_t now_ms, uint32_t deadline_ms)
{
	return (int32_t)(now_ms - deadline_ms) >= 0;
}

/* How long to wait, in milliseconds, after the transmission the transaction has
 * just made, number b->attempt (RFC 8489 section 6.2.1): the RTO doubled for
 * every transmission before it, or Rm times the RTO after the last one.
 */
static uint32_t retransmit_interval_ms(const struct stun_binding *b)
{
	if (b->attempt >= MAX_TRANSMITS) {
		return RTO_MS * FINAL_WAIT_RTO;
	}
	return RTO_MS << (b->attempt - 1U);
}

/* Fill the step with an order to send the request of the transaction. */
static void step_set_send(const struct stun_binding *b, struct stun_txn_step *step)
{
	step->action = STUN_TXN_SEND;
	step->tx = b->req;
	step->tx_len = b->req_len;
}

/* End the transaction as failed and keep the reason. */
static void binding_set_failed(struct stun_binding *b, enum stun_binding_fail why)
{
	b->state = STUN_BINDING_FAILED;
	b->fail = why;
}

/* Build the request aside, copy it and the server into the transaction, set the
 * first deadline and give the request to send.
 */
int stun_binding_start(struct stun_binding *b, const struct net_sockaddr *server,
		       const uint8_t txid[STUN_TXID_SIZE], const char *software, uint32_t now_ms,
		       struct stun_txn_step *step)
{
	uint8_t msg[STUN_BINDING_REQUEST_SIZE];
	struct stun_builder req;
	int rc;

	if (step != NULL) {
		memset(step, 0, sizeof(*step));
	}
	if (b == NULL || server == NULL || txid == NULL || step == NULL) {
		return -EINVAL;
	}
	rc = server_addr_check(server);
	if (rc != 0) {
		return rc;
	}
	if (b->state == STUN_BINDING_IN_FLIGHT) {
		return -EALREADY;
	}

	/* A Binding request to a STUN server needs no credentials; FINGERPRINT
	 * lets the receiver tell it from other traffic on its port. It is built
	 * aside, so that a refused one leaves a finished transaction as it was.
	 */
	rc = stun_builder_init(&req, msg, sizeof(msg), STUN_BINDING_REQUEST, txid);
	if (rc != 0) {
		return rc;
	}
	if (software != NULL) {
		rc = stun_add_software(&req, software);
		if (rc != 0) {
			return rc;
		}
	}
	rc = stun_finish_plain(&req);
	if (rc < 0) {
		return rc;
	}

	b->req_len = (size_t)rc;
	memcpy(b->req, msg, b->req_len);
	sockaddr_copy(&b->server, server);
	memset(&b->mapped, 0, sizeof(b->mapped));
	b->state = STUN_BINDING_IN_FLIGHT;
	b->fail = STUN_BINDING_FAIL_NONE;
	b->error_code = 0U;
	b->attempt = 1U;
	b->deadline_ms = now_ms + retransmit_interval_ms(b);
	step_set_send(b, step);
	return 0;
}

/* Give the step for the time now: wait until the deadline, send the request
 * again and move the deadline, or fail after the last wait.
 */
int stun_binding_next_step(struct stun_binding *b, uint32_t now_ms, struct stun_txn_step *step)
{
	if (step != NULL) {
		memset(step, 0, sizeof(*step));
	}
	if (b == NULL || step == NULL) {
		return -EINVAL;
	}

	switch (b->state) {
	case STUN_BINDING_IDLE:
		return -EPERM;
	case STUN_BINDING_DONE:
		step->action = STUN_TXN_DONE;
		return 0;
	case STUN_BINDING_FAILED:
		step->action = STUN_TXN_FAILED;
		return 0;
	default:
		break;
	}

	if (!deadline_reached(now_ms, b->deadline_ms)) {
		step->action = STUN_TXN_WAIT;
		step->wait_until_ms = b->deadline_ms;
		return 0;
	}
	if (b->attempt >= MAX_TRANSMITS) {
		binding_set_failed(b, STUN_BINDING_FAIL_TIMEOUT);
		step->action = STUN_TXN_FAILED;
		return 0;
	}

	/* The interval is counted from this transmission (RFC 8489 section 6.2.1: the
	 * intervals are between transmissions, and the final wait follows the
	 * last one). A caller that comes late moves the rest of the schedule
	 * with it; it does not get to send several times in a row to catch up.
	 */
	b->attempt++;
	b->deadline_ms = now_ms + retransmit_interval_ms(b);
	step_set_send(b, step);
	return 0;
}

/* True for a Binding response, success or error, with the transaction id of the
 * request. The sender is not looked at.
 */
bool stun_binding_is_response(const struct stun_binding *b, const uint8_t *buf, size_t len)
{
	enum stun_class cls;
	uint16_t type;

	if (b == NULL || b->state == STUN_BINDING_IDLE || !stun_has_valid_header(buf, len)) {
		return false;
	}

	type = sys_get_be16(buf);
	cls = stun_msg_type_get_class(type);
	if (stun_msg_type_get_method(type) != STUN_METHOD_BINDING ||
	    (cls != STUN_CLASS_SUCCESS && cls != STUN_CLASS_ERROR)) {
		return false;
	}
	/* The request holds the transaction id where the response does. */
	return memcmp(buf + TXID_OFFSET, b->req + TXID_OFFSET, STUN_TXID_SIZE) == 0;
}

/* Take the answer of the server and end the transaction with its mapped address
 * or with a failure. Any other datagram is refused and changes nothing.
 */
int stun_binding_on_datagram(struct stun_binding *b, const struct net_sockaddr *from,
			     const uint8_t *buf, size_t len)
{
	struct stun_msg msg;

	if (b == NULL || from == NULL || buf == NULL) {
		return -EINVAL;
	}
	if (b->state != STUN_BINDING_IN_FLIGHT) {
		return -EALREADY;
	}

	/* Whatever is not the server's answer is ignored: a datagram from
	 * elsewhere must neither end the transaction nor plant an address in it.
	 */
	if (!stun_binding_is_response(b, buf, len)) {
		return -ENOENT;
	}
	if (!net_sockaddr_cmp(from, net_sad(&b->server))) {
		return -EACCES;
	}
	if (stun_parse(buf, len, &msg) != 0 ||
	    (msg.has_fingerprint && stun_verify_fingerprint(&msg) != 0)) {
		return -EBADMSG;
	}

	/* The server has answered, and that ends the transaction one way or the
	 * other (RFC 8489 sections 6.3.3 and 6.3.4).
	 */
	if (stun_unknown_attrs(&msg, NULL, 0U, NULL, 0U) != 0) {
		binding_set_failed(b, STUN_BINDING_FAIL_BAD_RESPONSE);
	} else if (stun_msg_type_get_class(msg.type) == STUN_CLASS_ERROR) {
		binding_set_failed(b, STUN_BINDING_FAIL_ERROR_RESPONSE);
		b->error_code = msg.has_error_code ? msg.error_code : 0U;
	} else if (stun_get_xor_mapped_address(&msg, net_sad(&b->mapped)) == 0) {
		b->state = STUN_BINDING_DONE;
	} else {
		/* No XOR-MAPPED-ADDRESS, or one of a family this build has no
		 * stack for.
		 */
		memset(&b->mapped, 0, sizeof(b->mapped));
		binding_set_failed(b, STUN_BINDING_FAIL_BAD_RESPONSE);
	}
	return 0;
}
