/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Kirill Shypachov
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * STUN Binding transaction, RFC 8489 §6: one request, its retransmissions, and
 * the response. No sockets and no clock: the caller moves the bytes and tells
 * the time.
 */

#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <zephyr/net/net_ip.h>
#include <zephyr/net/stun.h>
#include <zephyr/sys/byteorder.h>

/* The limits of a policy: an RTO that still doubles 15 times inside 32 bits. */
#define RTO_MAX_MS    60000U
#define TRANSMITS_MAX 16U

/* RFC 8489 §6.2.1: RTO 500 ms, Rc 7, Rm 16. */
static const struct stun_txn_policy default_policy = {
	.rto_ms = 500U,
	.max_transmits = 7U,
	.final_wait_rto = 16U,
};

static bool policy_ok(const struct stun_txn_policy *p)
{
	return p->rto_ms >= 1U && p->rto_ms <= RTO_MAX_MS && p->max_transmits >= 1U &&
	       p->max_transmits <= TRANSMITS_MAX && p->final_wait_rto >= 1U;
}

/* Can the transaction talk to this server? Its family has to be one this build
 * has a stack for, and it has to have a port.
 */
static int server_check(const struct net_sockaddr *server)
{
	if (server->sa_family == NET_AF_INET) {
		return (net_sin(server)->sin_port != 0U) ? 0 : -EINVAL;
	}
#if defined(CONFIG_NET_IPV6)
	if (server->sa_family == NET_AF_INET6) {
		return (net_sin6(server)->sin6_port != 0U) ? 0 : -EINVAL;
	}
#endif
	return -EAFNOSUPPORT;
}

/* Keep the address and port of an endpoint. Only as much as its family defines
 * is read: the caller may have passed a struct net_sockaddr_in.
 */
static void endpoint_copy(struct net_sockaddr *dst, const struct net_sockaddr *src)
{
	memset(dst, 0, sizeof(*dst));
	if (src->sa_family == NET_AF_INET) {
		net_sin(dst)->sin_family = NET_AF_INET;
		net_sin(dst)->sin_port = net_sin(src)->sin_port;
		net_sin(dst)->sin_addr = net_sin(src)->sin_addr;
	}
#if defined(CONFIG_NET_IPV6)
	if (src->sa_family == NET_AF_INET6) {
		net_sin6(dst)->sin6_family = NET_AF_INET6;
		net_sin6(dst)->sin6_port = net_sin6(src)->sin6_port;
		net_sin6(dst)->sin6_addr = net_sin6(src)->sin6_addr;
	}
#endif
}

static bool endpoint_equal(const struct net_sockaddr *a, const struct net_sockaddr *b)
{
	if (a->sa_family != b->sa_family) {
		return false;
	}
	if (a->sa_family == NET_AF_INET) {
		return net_sin(a)->sin_port == net_sin(b)->sin_port &&
		       memcmp(&net_sin(a)->sin_addr, &net_sin(b)->sin_addr,
			      sizeof(struct net_in_addr)) == 0;
	}
#if defined(CONFIG_NET_IPV6)
	if (a->sa_family == NET_AF_INET6) {
		return net_sin6(a)->sin6_port == net_sin6(b)->sin6_port &&
		       memcmp(&net_sin6(a)->sin6_addr, &net_sin6(b)->sin6_addr,
			      sizeof(struct net_in6_addr)) == 0;
	}
#endif
	return false;
}

/* Has the clock reached the deadline? Both wrap around together. */
static bool reached(uint32_t now_ms, uint32_t deadline_ms)
{
	return (int32_t)(now_ms - deadline_ms) >= 0;
}

/* How long to wait after transmission number `attempt` (RFC 8489 §6.2.1): the
 * RTO doubled for every transmission before it, or Rm times the RTO after the
 * last one.
 */
static uint32_t wait_after(const struct stun_binding *b)
{
	if (b->attempt >= b->policy.max_transmits) {
		return b->policy.rto_ms * b->policy.final_wait_rto;
	}
	return b->policy.rto_ms << (b->attempt - 1U);
}

static void step_send(const struct stun_binding *b, struct stun_txn_step *step)
{
	step->action = STUN_TXN_SEND;
	step->tx = b->req;
	step->tx_len = b->req_len;
}

static void fail(struct stun_binding *b, enum stun_binding_fail why)
{
	b->state = STUN_BINDING_FAILED;
	b->fail = why;
}

int stun_binding_start(struct stun_binding *b, const struct net_sockaddr *server,
		       const uint8_t txid[STUN_TXID_SIZE], const char *software,
		       const struct stun_txn_policy *policy, uint32_t now_ms,
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
	if (policy == NULL) {
		policy = &default_policy;
	}
	if (!policy_ok(policy)) {
		return -EINVAL;
	}
	rc = server_check(server);
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
	stun_builder_init(&req, msg, sizeof(msg), STUN_BINDING_REQUEST, txid);
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
	endpoint_copy(&b->server, server);
	b->policy = *policy;
	memset(&b->mapped, 0, sizeof(b->mapped));
	b->state = STUN_BINDING_IN_FLIGHT;
	b->fail = STUN_BINDING_FAIL_NONE;
	b->error_code = 0U;
	b->rx_rejected = 0U;
	b->attempt = 1U;
	b->deadline_ms = now_ms + wait_after(b);
	step_send(b, step);
	return 0;
}

int stun_binding_advance(struct stun_binding *b, uint32_t now_ms, struct stun_txn_step *step)
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

	if (!reached(now_ms, b->deadline_ms)) {
		step->action = STUN_TXN_WAIT;
		step->wait_until_ms = b->deadline_ms;
		return 0;
	}
	if (b->attempt >= b->policy.max_transmits) {
		fail(b, STUN_BINDING_FAIL_TIMEOUT);
		step->action = STUN_TXN_FAILED;
		return 0;
	}

	/* The interval is counted from this transmission (RFC 8489 §6.2.1: the
	 * intervals are between transmissions, and the final wait follows the
	 * last one). A caller that comes late moves the rest of the schedule
	 * with it; it does not get to send several times in a row to catch up.
	 */
	b->attempt++;
	b->deadline_ms = now_ms + wait_after(b);
	step_send(b, step);
	return 0;
}

bool stun_binding_owns(const struct stun_binding *b, const uint8_t *buf, size_t len)
{
	enum stun_class cls;
	uint16_t type;

	if (b == NULL || b->state == STUN_BINDING_IDLE || !stun_is_message(buf, len)) {
		return false;
	}

	type = sys_get_be16(buf);
	cls = stun_msg_class(type);
	if (stun_msg_method(type) != STUN_METHOD_BINDING ||
	    (cls != STUN_CLASS_SUCCESS && cls != STUN_CLASS_ERROR)) {
		return false;
	}
	/* The request holds the transaction id where the response does. */
	return memcmp(buf + 8, b->req + 8, STUN_TXID_SIZE) == 0;
}

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

	/* Whatever is not the server's answer is counted and otherwise ignored:
	 * a datagram from elsewhere must neither end the transaction nor plant
	 * an address in it.
	 */
	if (!stun_binding_owns(b, buf, len)) {
		b->rx_rejected++;
		return -ENOENT;
	}
	if (!endpoint_equal(from, &b->server)) {
		b->rx_rejected++;
		return -EACCES;
	}
	if (stun_parse(buf, len, &msg) != 0 ||
	    (msg.has_fingerprint && stun_verify_fingerprint(buf, len, &msg) != 0)) {
		b->rx_rejected++;
		return -EBADMSG;
	}

	/* The server has answered, and that ends the transaction one way or the
	 * other (RFC 8489 §6.3.3, §6.3.4).
	 */
	if (stun_unknown_attrs(buf, len, &msg, NULL, 0U, NULL, 0U) != 0) {
		fail(b, STUN_BINDING_FAIL_BAD_RESPONSE);
	} else if (stun_msg_class(msg.type) == STUN_CLASS_ERROR) {
		fail(b, STUN_BINDING_FAIL_ERROR_RESPONSE);
		b->error_code = msg.has_error_code ? msg.error_code : 0U;
	} else if (stun_get_xor_mapped_address(buf, len, &msg, &b->mapped) == 0) {
		b->state = STUN_BINDING_DONE;
	} else {
		/* No XOR-MAPPED-ADDRESS, or one of a family this build has no
		 * stack for.
		 */
		memset(&b->mapped, 0, sizeof(b->mapped));
		fail(b, STUN_BINDING_FAIL_BAD_RESPONSE);
	}
	return 0;
}

void stun_binding_cancel(struct stun_binding *b)
{
	if (b != NULL && b->state == STUN_BINDING_IN_FLIGHT) {
		fail(b, STUN_BINDING_FAIL_CANCELLED);
	}
}
